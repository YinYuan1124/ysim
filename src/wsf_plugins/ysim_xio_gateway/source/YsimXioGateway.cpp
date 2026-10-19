// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2003-2015 The Boeing Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// *****************************************************************************
// *****************************************************************************
// 二次开发（ext-02）：ysim_xio_gateway XIO 协议网关插件
// 规格来源：.trae/specs/ext-02-protocol-gateway/spec.md §4（阶段 A）
//
// 设计意图：
//   1. XIO 生态接入：通过 WsfXIO_Extension::Find(simulation) 取得
//      WsfXIO_Interface（mission 主程序已注册 xio_interface 扩展，
//      想定含非平凡 xio_interface 块时激活），零侵入挂接 XIO 网络。
//   2. 私有 Packet 族（packetio 序列化，ID 350-352 位于"应用 2 段"之后
//      官方预留未用区，见 WsfXIO_Packet.hpp 头部 ID 预留表）：
//        YsimGwEventPkt(350)   下行：生命周期事件（sim_start/平台增删/sim_complete）
//        YsimGwStatePkt(351)   下行：平台位置姿态时序（WGS84 度）
//        YsimGwCommandPkt(352) 上行：外部指令（command+arg，本批登记+日志，
//                              指令注入仿真执行属后续批次）
//   3. 数据源：WsfObserver 只读订阅（与 ext-01 同范式），死区+降采样节流；
//      发送走 SendToAllTCP（XIO 可靠连接，UDP 心跳自动发现由 XIO 原生处理）。
//   4. 想定未启用 xio_interface 时（Find 返回 null）仅告警闲置，不阻断仿真。
//
// 挂载方式：DLL 经 swdev_plugin_install 部署至 mission_plugins/，
//           mission.exe 启动时由 WsfPluginManager 自动扫描加载。
//
// 想定配置（本插件无自有配置块；XIO 组网由原生 xio_interface 块控制）：
//   xio_interface
//      multicast 235.7.11.27 10.     # UDP 心跳发现组
//      port 3002                     # TCP 监听端口
//      time_to_live 8
//   end_xio_interface
// *****************************************************************************

#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "UtCallback.hpp"
#include "UtCallbackHolder.hpp"
#include "UtInput.hpp"
#include "UtMath.hpp"
#include "UtMemory.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "WsfMover.hpp"
#include "WsfMoverObserver.hpp"
#include "WsfPlatform.hpp"
#include "WsfPlatformObserver.hpp"
#include "WsfPlatformTypes.hpp"
#include "WsfPlugin.hpp"
#include "WsfScenario.hpp"
#include "WsfScenarioExtension.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationExtension.hpp"
#include "WsfSimulationObserver.hpp"
// 二次开发（ext-02 平台级指令集）：set_route/set_speed 需 RouteMover 族 API
#include "mover/WsfPath.hpp"
#include "mover/WsfRoute.hpp"
#include "mover/WsfRouteMover.hpp"
#include "mover/WsfWaypoint.hpp"
#include "xio_sim/WsfXIO_Extension.hpp"
#include "xio/WsfXIO_Interface.hpp"
#include "xio/WsfXIO_Packet.hpp"
#include "xio/WsfXIO_PacketRegistry.hpp"
// 二次开发（ext-02）：operator& 序列化模板的实体定义在 PakSerializeImpl.hpp，
// 各 XIO 头文件仅含 PakSerializeFwd.hpp 前置声明——不显式包含会触发 LNK2019
// （MSVC 对"仅声明无定义的 inline 模板"外链到库内实例化符号，插件侧不可见）
#include "PakSerializeImpl.hpp"

namespace
{
//! 插件扩展名（注册键）
const std::string cEXTENSION_NAME = "ysim_xio_gateway";

//! 按空白切分指令参数串（平台级指令参数空格分隔：<平台名> <参数...>）
std::vector<std::string> SplitTokens(const std::string& aText)
{
   std::vector<std::string> tokens;
   std::istringstream      stream(aText);
   std::string             token;
   while (stream >> token)
   {
      tokens.push_back(token);
   }
   return tokens;
}

// ----------------------------------------------------------------------------
// 私有 XIO Packet 族（阶段 A：下行事件/状态 + 上行指令）
// ID 350-352 位于官方预留未用段（WsfXIO_Packet.hpp：350+ unused）
// ----------------------------------------------------------------------------

//! 下行：生命周期事件帧
class YsimGwEventPkt : public WsfXIO_Packet
{
public:
   //! 事件枚举（序列化为 int32；避免跨端枚举尺寸分歧）
   enum Event : int32_t
   {
      cSIM_START        = 0,
      cPLATFORM_ADDED   = 1,
      cPLATFORM_REMOVED = 2,
      cSIM_COMPLETE     = 3
   };

   XIO_DEFINE_PACKET(YsimGwEventPkt, WsfXIO_Packet, 350)
   {
      using namespace PakSerialization;
      aBuff& mEvent& mPlatformIdx& mName& mType& mSide& mTime;
   }

   int32_t     mEvent      = 0;  // 事件枚举
   uint32_t    mPlatformIdx = 0; // 平台索引
   std::string mName;            // 平台名（added/removed 有效）
   std::string mType;            // 平台类型（added 有效）
   std::string mSide;            // 侧别（added 有效）
   float       mTime = 0.0f;     // 仿真时间 [s]
};

//! 下行：平台状态时序帧（WGS84，弧度→度在发送边界）
class YsimGwStatePkt : public WsfXIO_Packet
{
public:
   XIO_DEFINE_PACKET(YsimGwStatePkt, WsfXIO_Packet, 351)
   {
      using namespace PakSerialization;
      aBuff& mPlatformIdx& mName& mSide& mTime& mLat& mLon& mAltM& mSpeedMps& mHeadingDeg& mPitchDeg& mRollDeg;
   }

   uint32_t    mPlatformIdx = 0;
   std::string mName;
   std::string mSide;
   float       mTime       = 0.0f; // 仿真时间 [s]
   float       mLat        = 0.0f; // 纬度 [deg]
   float       mLon        = 0.0f; // 经度 [deg]
   float       mAltM       = 0.0f; // 高度 [m]
   float       mSpeedMps   = 0.0f; // 速度 [m/s]
   float       mHeadingDeg = 0.0f; // 航向 [deg]
   float       mPitchDeg   = 0.0f; // 俯仰 [deg]
   float       mRollDeg    = 0.0f; // 滚转 [deg]
};

//! 上行：外部指令帧（线程安全入队，仿真线程帧回调出队执行——指令注入闭环）
class YsimGwCommandPkt : public WsfXIO_Packet
{
public:
   XIO_DEFINE_PACKET(YsimGwCommandPkt, WsfXIO_Packet, 352)
   {
      using namespace PakSerialization;
      aBuff& mCommand& mArg& mTime;
   }
   std::string mCommand;      // 指令名（pause/resume/get_status/ping）
   std::string mArg;          // 指令参数（自由文本）
   float       mTime = 0.0f;  // 客户端时间戳 [s]
};

//! 下行：指令执行回执帧（指令注入闭环：每条上行指令必有一条 ack）
class YsimGwAckPkt : public WsfXIO_Packet
{
public:
   //! 回执状态枚举（序列化为 int32）
   enum Status : int32_t
   {
      cOK      = 0, // 执行成功
      cERROR   = 1, // 执行失败
      cUNKNOWN = 2  // 未知指令
   };
   XIO_DEFINE_PACKET(YsimGwAckPkt, WsfXIO_Packet, 353)
   {
      using namespace PakSerialization;
      aBuff& mCommand& mStatus& mDetail& mSimTime& mPlatformCount;
   }
   std::string mCommand;           // 对应指令名
   int32_t     mStatus    = 0;     // Status 枚举
   std::string mDetail;            // 人类可读执行结果（英文，保持日志语言策略）
   float       mSimTime   = 0.0f;  // 执行时刻仿真时间 [s]（pause/resume 冻结验证锚点）
   uint32_t    mPlatformCount = 0; // 执行时刻在场平台数（get_status 携带）
};

// ----------------------------------------------------------------------------
// 平台推送节流状态（与 ext-01 同策略：死区 + 降采样）
// ----------------------------------------------------------------------------
struct PlatformThrottle
{
   double mLastLat      = 0.0; // 上一采样纬度 [rad]
   double mLastLon      = 0.0; // 上一采样经度 [rad]
   double mLastAlt      = 0.0; // 上一采样高度 [m]
   double mLastPushTime = -1.0e30;
   bool   mHasLast      = false;
};

//! 仿真扩展：XIO 网关主体（观察者订阅 → 私有 Packet 广播）
class GatewaySimulationExtension : public WsfSimulationExtension
{
public:
   GatewaySimulationExtension() = default;

   ~GatewaySimulationExtension() override = default;

   //! 扩展依赖声明：本扩展需在 XIO 仿真服务之后初始化（保证 Find 能取到接口）
   void AddedToSimulation() override {}

   //! 初始化：取 XIO 接口、注册私有 Packet、订阅观察者
   bool Initialize() override
   {
      // 二次开发（ext-02）：取 XIO 接口（想定未启用 xio_interface 时为 null，
      // 插件告警闲置——网关是可选增强，不构成仿真运行的前置条件）
      mInterfacePtr = WsfXIO_Extension::Find(GetSimulation());
      if (mInterfacePtr == nullptr)
      {
         auto out = ut::log::warning() << "ysim_xio_gateway: xio_interface is not enabled in the scenario.";
         out.AddNote() << "XIO gateway disabled (add an 'xio_interface' block to enable).";
         return true;
      }

      // 注册私有 Packet（PakProcessor 注册表：双端 ID 一致即互通）
      mInterfacePtr->RegisterPacket<YsimGwEventPkt>(YsimGwEventPkt::cPACKET_ID, "YsimGwEventPkt", new YsimGwEventPkt{});
      mInterfacePtr->RegisterPacket<YsimGwStatePkt>(YsimGwStatePkt::cPACKET_ID, "YsimGwStatePkt", new YsimGwStatePkt{});
      mInterfacePtr->RegisterPacket<YsimGwCommandPkt>(YsimGwCommandPkt::cPACKET_ID, "YsimGwCommandPkt", new YsimGwCommandPkt{});
      mInterfacePtr->RegisterPacket<YsimGwAckPkt>(YsimGwAckPkt::cPACKET_ID, "YsimGwAckPkt", new YsimGwAckPkt{});

      // 订阅上行指令 Packet（XIO 线程回调：仅入队，不触碰仿真状态——
      // 仿真对象非线程安全，执行必须切换到仿真线程，见 OnAdvanceTime 出队）
      mCallbacks.Add(mInterfacePtr->Connect(&GatewaySimulationExtension::HandleCommand, this));

      // 观察者订阅（仿真线程回调，与 ext-01 同范式）
      WsfSimulation& sim = GetSimulation();
      mCallbacks.Add(WsfObserver::PlatformAdded(&sim).Connect(&GatewaySimulationExtension::OnPlatformAdded, this));
      mCallbacks.Add(WsfObserver::PlatformDeleted(&sim).Connect(&GatewaySimulationExtension::OnPlatformDeleted, this));
      mCallbacks.Add(WsfObserver::MoverUpdated(&sim).Connect(&GatewaySimulationExtension::OnMoverUpdated, this));
      // 帧回调：驱动"连接后补发 sim_start"（见 OnAdvanceTime）
      mCallbacks.Add(WsfObserver::AdvanceTime(&sim).Connect(&GatewaySimulationExtension::OnAdvanceTime, this));
      auto out = ut::log::info() << "ysim_xio_gateway: registered XIO packets (event=350, state=351, command=352).";
      return true;
   }

   //! 仿真开始：置 sim_start 待发标志（实际发送延后到首个可靠连接建立后的帧，
   //! 见 OnAdvanceTime——Start 阶段客户端往往尚未完成 XIO 握手，此时广播必然丢失）
   void Start() override
   {
      mSimStartPending = (mInterfacePtr != nullptr);
   }

   //! 仿真结束：广播 sim_complete 事件
   void Complete(double aSimTime) override
   {
      if (mInterfacePtr == nullptr)
      {
         return;
      }
      YsimGwEventPkt pkt;
      pkt.mEvent = YsimGwEventPkt::cSIM_COMPLETE;
      pkt.mTime  = static_cast<float>(aSimTime);
      mInterfacePtr->SendToAllTCP(pkt);
      // 二次开发（ext-02）：sim_complete 是末帧包，发送后短暂等待让 XIO 网络
      // 线程（PakThreadedIO）完成 flush——进程退出竞态会吞掉末包，客户端收不到
      // 完成事件无法收尾（端到端实测教训）
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
   }

   //! 已收到的上行指令数（冒烟验证用：客户端发 N 条 → mission 侧计数对齐）
   unsigned long long ReceivedCommandCount() const { return mReceivedCommands.load(); }

private:
   //! 帧回调（仿真线程）：驱动两件事——
   //!   1. 指令注入闭环：出队并执行上行指令（每帧轮询；注意暂停态时钟冻结
   //!      但 mission 主循环 while(IsActive()) 仍空转、AdvanceTime 观察者仍
   //!      每轮触发，故 pause 期间 resume 指令照常出队执行，不会死锁）
   //!   2. 首个可靠连接建立后补发 sim_start + 既有平台快照
   //!      （连接前发生的事件广播即丢：t=0 初始平台添加于 XIO 握手完成前——
   //!      快照补推与 ext-01 阶段 B"新客户端接入即收全量态势"机制对齐）
   void OnAdvanceTime(double aSimTime)
   {
      if (mInterfacePtr == nullptr)
      {
         return;
      }
      DrainPendingCommands(aSimTime);
      if (!mSimStartPending)
      {
         return;
      }
      if (mInterfacePtr->GetReliableConnections().empty())
      {
         return; // 尚无客户端接入：继续等待（等待期间时间不推进也每帧触发）
      }
      mSimStartPending = false;
      YsimGwEventPkt pkt;
      pkt.mEvent = YsimGwEventPkt::cSIM_START;
      pkt.mTime  = static_cast<float>(aSimTime);
      mInterfacePtr->SendToAllTCP(pkt);
      // 既有平台快照补推：对每个在场平台补发 platform_added（重建节流表）
      WsfSimulation& sim = GetSimulation();
      for (unsigned int i = 0; i < sim.GetPlatformCount(); ++i)
      {
         WsfPlatform* platformPtr = sim.GetPlatformEntry(i);
         if (platformPtr != nullptr)
         {
            SendPlatformAdded(aSimTime, platformPtr);
         }
      }
   }

   //! 平台加入：广播 platform_added 事件帧
   void OnPlatformAdded(double aSimTime, WsfPlatform* aPlatformPtr)
   {
      if (mInterfacePtr == nullptr)
      {
         return;
      }
      SendPlatformAdded(aSimTime, aPlatformPtr);
   }

   //! platform_added 帧构造+广播（事件流与快照补推共用）
   void SendPlatformAdded(double aSimTime, WsfPlatform* aPlatformPtr)
   {
      mThrottle[aPlatformPtr->GetIndex()] = PlatformThrottle{};
      YsimGwEventPkt pkt;
      pkt.mEvent      = YsimGwEventPkt::cPLATFORM_ADDED;
      pkt.mPlatformIdx = static_cast<uint32_t>(aPlatformPtr->GetIndex());
      pkt.mName       = aPlatformPtr->GetName();
      pkt.mType       = aPlatformPtr->GetType();
      pkt.mSide       = aPlatformPtr->GetSide();
      pkt.mTime       = static_cast<float>(aSimTime);
      mInterfacePtr->SendToAllTCP(pkt);
   }

   //! 平台移除：广播 platform_removed 事件帧并清理节流状态
   void OnPlatformDeleted(double aSimTime, WsfPlatform* aPlatformPtr)
   {
      if (mInterfacePtr == nullptr)
      {
         return;
      }
      mThrottle.erase(aPlatformPtr->GetIndex());
      YsimGwEventPkt pkt;
      pkt.mEvent      = YsimGwEventPkt::cPLATFORM_REMOVED;
      pkt.mPlatformIdx = static_cast<uint32_t>(aPlatformPtr->GetIndex());
      pkt.mName       = aPlatformPtr->GetName();
      pkt.mTime       = static_cast<float>(aSimTime);
      mInterfacePtr->SendToAllTCP(pkt);
   }

   //! 平台状态更新：死区+降采样节流后广播 state 帧
   //! 注意：回调签名首参为 WsfMover*（WsfObserver::MoverUpdated 约定）
   void OnMoverUpdated(double aSimTime, WsfMover* aMoverPtr)
   {
      if (mInterfacePtr == nullptr)
      {
         return;
      }
      WsfPlatform* aPlatformPtr = aMoverPtr->GetPlatform();
      if (aPlatformPtr == nullptr)
      {
         return;
      }
      double lat = 0.0;
      double lon = 0.0;
      double alt = 0.0;
      aPlatformPtr->GetLocationLLA(lat, lon, alt);

      // 死区 + 降采样（与 ext-01 相同参数语义）
      PlatformThrottle& th = mThrottle[aPlatformPtr->GetIndex()];
      if (th.mHasLast)
      {
         const double cDEADBAND_M      = 2.0;
         const double cMIN_INTERVAL_S  = 0.2;
         double       dLat             = lat - th.mLastLat;
         double       dLon             = lon - th.mLastLon;
         double       dN               = dLat * 6371000.0;
         double       dE               = dLon * 6371000.0 * std::cos(lat);
         double       dU               = alt - th.mLastAlt;
         bool         moved            = std::sqrt(dN * dN + dE * dE + dU * dU) >= cDEADBAND_M;
         bool         intervalElapsed  = (aSimTime - th.mLastPushTime) >= cMIN_INTERVAL_S;
         if ((!moved) || (!intervalElapsed))
         {
            return;
         }
      }
      th.mHasLast      = true;
      th.mLastLat      = lat;
      th.mLastLon      = lon;
      th.mLastAlt      = alt;
      th.mLastPushTime = aSimTime;

      double heading = 0.0;
      double pitch   = 0.0;
      double roll    = 0.0;
      aPlatformPtr->GetOrientationNED(heading, pitch, roll);

      YsimGwStatePkt pkt;
      pkt.mPlatformIdx = static_cast<uint32_t>(aPlatformPtr->GetIndex());
      pkt.mName       = aPlatformPtr->GetName();
      pkt.mSide       = aPlatformPtr->GetSide();
      pkt.mTime       = static_cast<float>(aSimTime);
      pkt.mLat        = static_cast<float>(lat * UtMath::cDEG_PER_RAD);
      pkt.mLon        = static_cast<float>(lon * UtMath::cDEG_PER_RAD);
      pkt.mAltM       = static_cast<float>(alt);
      pkt.mSpeedMps   = static_cast<float>(aPlatformPtr->GetSpeed());
      pkt.mHeadingDeg = static_cast<float>(heading * UtMath::cDEG_PER_RAD);
      pkt.mPitchDeg   = static_cast<float>(pitch * UtMath::cDEG_PER_RAD);
      pkt.mRollDeg    = static_cast<float>(roll * UtMath::cDEG_PER_RAD);
      mInterfacePtr->SendToAllTCP(pkt);
   }

   //! 待执行指令（XIO 线程生产 → 仿真线程消费；声明须先于使用它的成员函数）
   struct PendingCommand
   {
      std::string mCommand;
      std::string mArg;
      float       mTime = 0.0f;
   };

   //! 上行指令接收（XIO 回调线程）：仅线程安全入队 + 日志登记
   //! （执行在仿真线程帧回调 DrainPendingCommands——线程边界纪律：
   //! XIO 线程绝不触碰 WsfSimulation/WsfPlatform 等非线程安全对象）
   void HandleCommand(YsimGwCommandPkt& aPkt)
   {
      mReceivedCommands += 1;
      {
         std::lock_guard<std::mutex> lock(mCommandMutex);
         mPendingCommands.push_back({aPkt.mCommand, aPkt.mArg, aPkt.mTime});
      }
      auto out = ut::log::info() << "ysim_xio_gateway: received command '" << aPkt.mCommand << "' arg '" << aPkt.mArg << "'.";
   }

   //! 指令出队执行（仿真线程，每帧调用）：清空待执行队列并逐条回执
   void DrainPendingCommands(double aSimTime)
   {
      std::deque<PendingCommand> batch;
      {
         std::lock_guard<std::mutex> lock(mCommandMutex);
         batch.swap(mPendingCommands);
      }
      for (const PendingCommand& cmd : batch)
      {
         ExecuteCommand(cmd, aSimTime);
      }
   }

   //! 单条指令执行（仿真线程）：指令集 pause/resume/get_status/ping
   void ExecuteCommand(const PendingCommand& aCmd, double aSimTime)
   {
      YsimGwAckPkt ack;
      ack.mCommand       = aCmd.mCommand;
      ack.mSimTime       = static_cast<float>(aSimTime);
      ack.mPlatformCount = static_cast<uint32_t>(GetSimulation().GetPlatformCount());
      if (aCmd.mCommand == "pause")
      {
         GetSimulation().Pause();
         ack.mStatus = YsimGwAckPkt::cOK;
         ack.mDetail = "simulation paused";
      }
      else if (aCmd.mCommand == "resume")
      {
         GetSimulation().Resume();
         ack.mStatus = YsimGwAckPkt::cOK;
         ack.mDetail = "simulation resumed";
      }
      else if (aCmd.mCommand == "get_status")
      {
         ack.mStatus = YsimGwAckPkt::cOK;
         char detail[128];
         snprintf(detail, sizeof(detail), "sim_time=%.2f platforms=%u state=%s",
                  aSimTime, ack.mPlatformCount, GetSimulation().GetStateString().c_str());
         ack.mDetail = detail;
      }
      else if (aCmd.mCommand == "ping")
      {
         ack.mStatus = YsimGwAckPkt::cOK;
         ack.mDetail = "pong " + aCmd.mArg;
      }
      // ---- 平台级指令集（spec §4.5）：参数空格分隔，arg[0] 恒为目标平台名 ----
      else if (aCmd.mCommand == "set_speed")
      {
         // 用法：set_speed <平台名> <速度 m/s>
         std::vector<std::string> args = SplitTokens(aCmd.mArg);
         WsfPlatform*   platformPtr = (args.size() >= 1) ? GetSimulation().GetPlatformByName(args[0].c_str()) : nullptr;
         WsfRouteMover* moverPtr    = (platformPtr != nullptr) ? dynamic_cast<WsfRouteMover*>(platformPtr->GetMover()) : nullptr;
         if (args.size() < 2 || platformPtr == nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "platform '" + (args.empty() ? std::string() : args[0]) + "' not found";
         }
         else if (moverPtr == nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "platform '" + args[0] + "' mover is not route-capable";
         }
         else
         {
            moverPtr->GoToSpeed(aSimTime, atof(args[1].c_str()), 5.0, true);
            ack.mStatus = YsimGwAckPkt::cOK;
            ack.mDetail = "speed of '" + args[0] + "' set to " + args[1] + " m/s";
         }
      }
      else if (aCmd.mCommand == "set_route")
      {
         // 用法：set_route <平台名> <纬度 deg> <经度 deg> <高度 m> [速度 m/s]
         // 单航点航路整体替换（重规划：现有航路作废，直飞目标点）
         std::vector<std::string> args = SplitTokens(aCmd.mArg);
         WsfPlatform*   platformPtr = (args.size() >= 1) ? GetSimulation().GetPlatformByName(args[0].c_str()) : nullptr;
         WsfRouteMover* moverPtr    = (platformPtr != nullptr) ? dynamic_cast<WsfRouteMover*>(platformPtr->GetMover()) : nullptr;
         if (args.size() < 5 || platformPtr == nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "usage: set_route <name> <lat> <lon> <alt_m> [speed_mps]";
         }
         else if (moverPtr == nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "platform '" + args[0] + "' mover is not route-capable";
         }
         else
         {
            // 双航点航路：首点=当前位置，末点=目标（参数为角度，需转弧度——
            // WsfWaypoint 纬经度为弧度制；且 RouteMover 以航路首点为起始锚，
            // 单航点会瞬移到目标点，首点取当前位置消除瞬移）
            double curLatRad = 0.0, curLonRad = 0.0, curAltM = 0.0;
            platformPtr->GetLocationLLA(curLatRad, curLonRad, curAltM);
            double speedMps = (args.size() >= 6) ? atof(args[5].c_str()) : WsfPath::cUSE_PREVIOUS;
            WsfRoute route;
            route.Append(WsfWaypoint(curLatRad, curLonRad, curAltM));
            route.Append(WsfWaypoint(atof(args[1].c_str()) * UtMath::cRAD_PER_DEG,
                                     atof(args[2].c_str()) * UtMath::cRAD_PER_DEG,
                                     atof(args[3].c_str()), speedMps));
            if (moverPtr->SetRoute(aSimTime, route))
            {
               ack.mStatus = YsimGwAckPkt::cOK;
               ack.mDetail = "route of '" + args[0] + "' set to " + args[1] + "," + args[2] + "," + args[3];
            }
            else
            {
               ack.mStatus = YsimGwAckPkt::cERROR;
               ack.mDetail = "SetRoute rejected for '" + args[0] + "'";
            }
         }
      }
      else if (aCmd.mCommand == "add_platform")
      {
         // 用法：add_platform <平台名> <平台类型>——运行期动态创建（态势注入）
         std::vector<std::string> args = SplitTokens(aCmd.mArg);
         if (args.size() < 2)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "usage: add_platform <name> <type>";
         }
         else if (GetSimulation().GetPlatformByName(args[0].c_str()) != nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "platform '" + args[0] + "' already exists";
         }
         else
         {
            // WsfObjectTypeList::Clone 按类型名克隆新实例（运行期创建的标准路径）
            WsfPlatform* platformPtr = WsfPlatformTypes::Get(GetSimulation().GetScenario()).Clone(WsfStringId(args[1].c_str()));
            if (platformPtr == nullptr)
            {
               ack.mStatus = YsimGwAckPkt::cERROR;
               ack.mDetail = "platform type '" + args[1] + "' not found";
            }
            else
            {
               platformPtr->SetName(WsfStringId(args[0].c_str()));
               // AddPlatform 全托管：Initialize→Initialize2→PlatformAdded 观察者通知
               GetSimulation().AddPlatform(aSimTime, platformPtr);
               ack.mStatus = YsimGwAckPkt::cOK;
               ack.mDetail = "platform '" + args[0] + "' (" + args[1] + ") added";
            }
         }
      }
      else if (aCmd.mCommand == "delete_platform")
      {
         // 用法：delete_platform <平台名>——运行期动态删除（态势移除）
         std::vector<std::string> args = SplitTokens(aCmd.mArg);
         WsfPlatform* platformPtr = (args.size() >= 1) ? GetSimulation().GetPlatformByName(args[0].c_str()) : nullptr;
         if (args.size() < 1 || platformPtr == nullptr)
         {
            ack.mStatus = YsimGwAckPkt::cERROR;
            ack.mDetail = "platform '" + (args.empty() ? std::string() : args[0]) + "' not found";
         }
         else
         {
            // DeletePlatform 内部调度当帧一次性事件→PlatformDeleted 观察者→内存回收
            GetSimulation().DeletePlatform(aSimTime, platformPtr, true);
            ack.mStatus = YsimGwAckPkt::cOK;
            ack.mDetail = "platform '" + args[0] + "' deleted";
         }
      }
      else
      {
         ack.mStatus = YsimGwAckPkt::cUNKNOWN;
         ack.mDetail = "unknown command '" + aCmd.mCommand + "'";
      }
      mInterfacePtr->SendToAllTCP(ack);
      auto out = ut::log::info() << "ysim_xio_gateway: executed command '" << aCmd.mCommand
                                 << "' status=" << ack.mStatus << " detail='" << ack.mDetail << "'.";
   }

   WsfXIO_Interface*                 mInterfacePtr = nullptr; // XIO 接口（未启用时 null）
   UtCallbackHolder                  mCallbacks;              // 观察者+Packet 订阅（析构自动断开）
   std::map<size_t, PlatformThrottle> mThrottle;              // 各平台推送节流状态
   std::atomic<unsigned long long>   mReceivedCommands{0};    // 上行指令计数（冒烟验证）
   bool                              mSimStartPending = false; // sim_start 待发（连接后补发）
   std::mutex                        mCommandMutex;           // 指令队列锁（XIO 线程↔仿真线程）
   std::deque<PendingCommand>        mPendingCommands;        // 待执行指令队列
};

// ----------------------------------------------------------------------------
// 应用/场景扩展注册（插件标准扩展链；本插件无自有配置块与命令行）
// ----------------------------------------------------------------------------
class GatewayScenarioExtension : public WsfScenarioExtension
{
public:
   GatewayScenarioExtension() = default;

   //! 每个仿真创建时（含蒙特卡洛多轮运行）注册新的仿真扩展实例
   void SimulationCreated(WsfSimulation& aSimulation) override
   {
      aSimulation.RegisterExtension(cEXTENSION_NAME, ut::make_unique<GatewaySimulationExtension>());
   }
};

class GatewayApplicationExtension : public WsfApplicationExtension
{
public:
   GatewayApplicationExtension() = default;

   //! 场景创建：注册场景扩展
   void ScenarioCreated(WsfScenario& aScenario) override
   {
      aScenario.RegisterExtension(cEXTENSION_NAME, ut::make_unique<GatewayScenarioExtension>());
   }
};
} // namespace

// ----------------------------------------------------------------------------
// 插件注册入口（WsfPluginManager 经导出符号自动调用）
// ----------------------------------------------------------------------------

void Register_ysim_xio_gateway(WsfApplication& aApplication)
{
   if (!aApplication.ExtensionIsRegistered(cEXTENSION_NAME))
   {
      aApplication.RegisterExtension(cEXTENSION_NAME, ut::make_unique<GatewayApplicationExtension>());
   }
}

extern "C"
{
   UT_PLUGIN_EXPORT void WsfPluginVersion(UtPluginVersion& aVersion)
   {
      aVersion = UtPluginVersion(WSF_PLUGIN_API_MAJOR_VERSION, WSF_PLUGIN_API_MINOR_VERSION, WSF_PLUGIN_API_COMPILER_STRING);
   }

   UT_PLUGIN_EXPORT void WsfPluginSetup(WsfApplication& aApplication)
   {
      Register_ysim_xio_gateway(aApplication);
   }
}
