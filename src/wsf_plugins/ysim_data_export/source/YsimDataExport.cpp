// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2003-2015 The Boeing Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************
// ****************************************************************************
// 二次开发（ext-01）：ysim_data_export 仿真数据出口插件
// 规格来源：.trae/specs/ext-01-data-export/spec.md §7（阶段 A）/ §8（阶段 B）
//
// 设计意图：
//   1. 零侵入：只读订阅 WsfObserver 事件（PlatformAdded/MoverUpdated/...），
//      不修改任何 WSF 核心源码，插件可随时从 mission_plugins 目录移除。
//   2. 帧驱动：WsfObserver::AdvanceTime 每帧触发（等价用户方案中的"FrameComplete"），
//      配合 MoverUpdated 实现平台状态增量推送（死区 + 降采样，性能红线）。
//   3. 线程模型：仿真回调线程只做"取数→拼 JSON→入队"（微秒级），
//      各通道收发由独立网络线程完成，绝不阻塞仿真推进。
//   4. 多通道适配器（阶段 B）：消息层统一为 JSON 行（UTF-8 + '\n'），
//      通道层按适配器插拔——TCP 行协议 / WebSocket（最小 RFC 6455 服务端，
//      零第三方依赖）/ UDP 组播；新客户端接入即快照补推当前全量态势。
//   5. 结构化落盘（阶段 C）：与通道平行的 Recorder 持久化 Sink——
//      CSV 时序（platform_states/events/metrics 三文件，UTF-8 with BOM）+
//      SQLite 快照库（platforms/states/events/metrics 四表，静态链入
//      3rd_party sqlite-3.32.3）；评估指标表为平台级聚合 + 运行级汇总，
//      交战类指标待 Track/Engagement 数据源批次扩展。
//   6. 适配器热切换（阶段 D）：ControlChannel 控制通道（TCP 行协议命令，
//      默认禁用）——运行期 add_tcp/add_ws/add_udp/remove/list 增删数据
//      通道不重启仿真；通道列表读写锁保护（仿真线程共享读/控制线程独占写）。
//
// 挂载方式：DLL 经 swdev_plugin_install 部署至 mission_plugins/，
//           mission.exe 启动时由 WsfPluginManager 自动扫描加载
//           （AFSIM 2.9 无 require_plugin 命令，目录自动加载即原生机制）。
//
// 想定配置（场景级命令，任意位置一次；端口 0 = 禁用对应通道）：
//   ysim_data_export
//      tcp_port           31600    # TCP 通道（port 为其兼容别名）
//      websocket_port     31601    # WebSocket 通道
//      udp_address        239.16.16.16  # UDP 目的地址（224-239 段按组播处理）
//      udp_port           31602    # UDP 通道
//      udp_ttl            4        # 组播 TTL（跨网段跳数）
//      heartbeat_interval 5 s      # 心跳周期（仿真时间）
//      deadband           2 m      # 位置死区：位移小于该值跳过推送
//      min_update_interval 0.2 s   # 同一平台两次推送的最小间隔（降采样）
//      record             true     # 结构化落盘总开关（默认 false 显式启用）
//      record_directory   ysim_export  # 输出根目录（相对 CWD）
//      record_csv         true     # CSV Sink 子开关
//      record_sqlite      true     # SQLite Sink 子开关
//      control_port       31603    # 热切换控制通道（默认 0=禁用）
//      start_paused       true     # 起步暂停：仿真首帧即暂停，等控制通道 resume（默认 false）
//   end_ysim_data_export
//
// 命令行（选项必须置于文件名之前，框架约定）：
//   mission.exe --ysim-port <n> --ysim-ws-port <n> --ysim-udp-port <n>
//               --ysim-record <dir> --ysim-control-port <n> -rt scenario.txt
//
// 热切换控制命令（连接 control_port 后逐行发送）：
//   add_tcp <port> / add_ws <port> / add_udp <address> <port> [ttl]
//   remove <port> / list / help
//   pause / resume / set_rate <0.1-100> / status（仿真运行控制，e2e 控制面板）
// *****************************************************************************

// Winsock 头必须最先包含：防止其他头文件间接引入 windows.h 时拉入旧版 winsock.h
#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include "UtCallback.hpp"
#include "UtInput.hpp"
#include "UtInputBlock.hpp"
#include "UtMath.hpp"
#include "UtMemory.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "WsfMover.hpp"
#include "WsfMoverObserver.hpp"
#include "WsfPlatform.hpp"
#include "WsfPlatformObserver.hpp"
#include "WsfPlugin.hpp"
#include "WsfScenario.hpp"
#include "WsfScenarioExtension.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationExtension.hpp"
#include "WsfSimulationObserver.hpp"
#include "WsfStandardApplication.hpp"

// 二次开发（ext-01 阶段 C）：SQLite 静态库（3rd_party 包，CMake 按构建树解压约定引入）
// 二次开发（修复 2026-10-19）：SQLite 可选编译——CMake 三态选项 YSIM_SQLITE_SUPPORT
// 未命中且非 ON 时定义 YSIM_NO_SQLITE（SQLite Sink 编译期剔除，CSV/推流不受影响）
#ifndef YSIM_NO_SQLITE
#include "sqlite3.h"
#endif

namespace
{
//! 插件扩展名（同时是注册键与想定命令名）
const std::string cEXTENSION_NAME = "ysim_data_export";

//! 发送队列上限：客户端长期不读时丢弃最旧消息，防止内存无限增长
const size_t cMAX_QUEUE_SIZE = 10000;

// 二次开发（ext-01）：命令行参数错误异常
// 原因：WsfStandardApplication::InvalidCommandLineArgument 是嵌套类，未从 wsf.dll 导出，
//       插件侧无法链接；故从可内联链接的 WsfApplication::Exception 本地派生，
//       返回码置 1，与框架原生行为一致（mission 主循环捕获后打印 fatal 并以该码退出）
class CommandLineError : public WsfApplication::Exception
{
public:
   explicit CommandLineError(const std::string& aWhat)
      : Exception(aWhat)
   {
      mReturnCode = 1;
   }
};

//! JSON 字符串转义：平台名/类型/侧别来自想定文件，可能含引号等特殊字符
std::string JsonEscape(const std::string& aText)
{
   std::string out;
   out.reserve(aText.size() + 2);
   for (char c : aText)
   {
      // 二次开发（ext-01）：仅转义 JSON 规定的控制字符与引号/反斜杠
      switch (c)
      {
      case '"':
         out += "\\\"";
         break;
      case '\\':
         out += "\\\\";
         break;
      case '\n':
         out += "\\n";
         break;
      case '\r':
         out += "\\r";
         break;
      case '\t':
         out += "\\t";
         break;
      default:
         if (static_cast<unsigned char>(c) < 0x20)
         {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
            out += buf;
         }
         else
         {
            out += c;
         }
         break;
      }
   }
   return out;
}

//! 进程内 Winsock 一次性初始化
//! WSACleanup 刻意不调用：插件 DLL 卸载顺序不可控，进程退出由系统回收资源更安全
bool EnsureWinsock()
{
   static bool initialized = []()
   {
      WSADATA data;
      return WSAStartup(MAKEWORD(2, 2), &data) == 0;
   }();
   return initialized;
}

// ----------------------------------------------------------------------------
// 落盘辅助（阶段 C 结构化落盘：CSV 转义 / 目录创建 / 运行时间戳）
// ----------------------------------------------------------------------------

//! CSV 字段转义（RFC 4180）：含逗号/引号/换行的字段引号包裹，内部引号双写
std::string CsvEscape(const std::string& aText)
{
   bool needQuote = false;
   for (char c : aText)
   {
      if ((c == ',') || (c == '"') || (c == '\n') || (c == '\r'))
      {
         needQuote = true;
         break;
      }
   }
   if (!needQuote)
   {
      return aText;
   }
   std::string out;
   out.reserve(aText.size() + 2);
   out += '"';
   for (char c : aText)
   {
      if (c == '"')
      {
         out += "\"\"";
      }
      else
      {
         out += c;
      }
   }
   out += '"';
   return out;
}

//! 逐级创建目录（正反斜杠均支持；已存在视为成功）
//! 仅落盘初始化时调用一次，失败由调用方降级处理
bool EnsureDirectory(const std::string& aPath)
{
   if (aPath.empty())
   {
      return false;
   }
   std::string cur;
   for (size_t i = 0; i < aPath.size(); ++i)
   {
      if ((aPath[i] == '\\') || (aPath[i] == '/'))
      {
         if (!cur.empty())
         {
            CreateDirectoryA(cur.c_str(), nullptr); // 已存在时失败无害
         }
      }
      cur += aPath[i];
   }
   return (CreateDirectoryA(cur.c_str(), nullptr) != 0) ||
          (GetFileAttributesA(cur.c_str()) != INVALID_FILE_ATTRIBUTES);
}

//! 当前本地时间戳（运行子目录名：run_YYYYMMDD_HHMMSS_mmm，毫秒防同秒冲突）
std::string NowStamp()
{
   auto                now = std::chrono::system_clock::now();
   auto                ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
   std::time_t         tt  = std::chrono::system_clock::to_time_t(now);
   std::tm             tmv{};
   localtime_s(&tmv, &tt);
   char buf[40];
   std::snprintf(buf,
                 sizeof(buf),
                 "run_%04d%02d%02d_%02d%02d%02d_%03d",
                 tmv.tm_year + 1900,
                 tmv.tm_mon + 1,
                 tmv.tm_mday,
                 tmv.tm_hour,
                 tmv.tm_min,
                 tmv.tm_sec,
                 static_cast<int>(ms.count()));
   return std::string(buf);
}

// ----------------------------------------------------------------------------
// WebSocket 辅助（RFC 6455 最小实现，零第三方依赖）
// ----------------------------------------------------------------------------

//! SHA-1 摘要（RFC 3174）：仅用于握手 Accept 计算，输入为一次性短串，无需流式接口
std::string Sha1(const std::string& aInput)
{
   uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
   std::string msg = aInput;
   // 二次开发（ext-01）：手工补位（0x80 + 零填充 + 64 位大端长度），凑足 64 字节分组
   uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8;
   msg += static_cast<char>(0x80);
   while (msg.size() % 64 != 56)
   {
      msg += '\0';
   }
   for (int i = 7; i >= 0; --i)
   {
      msg += static_cast<char>((bitLen >> (i * 8)) & 0xFF);
   }
   for (size_t off = 0; off < msg.size(); off += 64)
   {
      uint32_t w[80];
      for (int i = 0; i < 16; ++i)
      {
         size_t p = off + static_cast<size_t>(i) * 4;
         w[i]     = (static_cast<uint32_t>(static_cast<unsigned char>(msg[p])) << 24) |
                (static_cast<uint32_t>(static_cast<unsigned char>(msg[p + 1])) << 16) |
                (static_cast<uint32_t>(static_cast<unsigned char>(msg[p + 2])) << 8) |
                static_cast<uint32_t>(static_cast<unsigned char>(msg[p + 3]));
      }
      for (int i = 16; i < 80; ++i)
      {
         uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
         w[i]       = (v << 1) | (v >> 31);
      }
      uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
      for (int i = 0; i < 80; ++i)
      {
         uint32_t f, k;
         if (i < 20)
         {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
         }
         else if (i < 40)
         {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
         }
         else if (i < 60)
         {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
         }
         else
         {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
         }
         uint32_t t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
         e          = d;
         d          = c;
         c          = (b << 30) | (b >> 2);
         b          = a;
         a          = t;
      }
      h[0] += a;
      h[1] += b;
      h[2] += c;
      h[3] += d;
      h[4] += e;
   }
   std::string out;
   out.reserve(20);
   for (int i = 0; i < 5; ++i)
   {
      // 二次开发（缺陷修复）：逐字节大端输出。移位窗口必须 8 位对齐（j=24/16/8/0），
      // 原 j=31/23/15/7 与 &0xFF 组合错位（j=31 仅取最高 1 位），导致 Accept 全错、浏览器拒绝握手
      for (int j = 24; j >= 0; j -= 8)
      {
         out += static_cast<char>((h[i] >> j) & 0xFF);
      }
   }
   return out;
}

//! Base64 编码（RFC 4648）：握手 Accept 值的输出表示
std::string Base64Encode(const std::string& aInput)
{
   static const char cTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   std::string       out;
   out.reserve((aInput.size() + 2) / 3 * 4);
   size_t i = 0;
   while (i + 3 <= aInput.size())
   {
      uint32_t v = (static_cast<uint32_t>(static_cast<unsigned char>(aInput[i])) << 16) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(aInput[i + 1])) << 8) |
                   static_cast<uint32_t>(static_cast<unsigned char>(aInput[i + 2]));
      out += cTable[(v >> 18) & 0x3F];
      out += cTable[(v >> 12) & 0x3F];
      out += cTable[(v >> 6) & 0x3F];
      out += cTable[v & 0x3F];
      i += 3;
   }
   size_t rem = aInput.size() - i;
   if (rem == 1)
   {
      uint32_t v = static_cast<uint32_t>(static_cast<unsigned char>(aInput[i])) << 16;
      out += cTable[(v >> 18) & 0x3F];
      out += cTable[(v >> 12) & 0x3F];
      out += "==";
   }
   else if (rem == 2)
   {
      uint32_t v = (static_cast<uint32_t>(static_cast<unsigned char>(aInput[i])) << 16) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(aInput[i + 1])) << 8);
      out += cTable[(v >> 18) & 0x3F];
      out += cTable[(v >> 12) & 0x3F];
      out += cTable[(v >> 6) & 0x3F];
      out += '=';
   }
   return out;
}

//! 提取 HTTP 请求头字段值（头名大小写不敏感）
std::string GetHttpHeader(const std::string& aRequest, const std::string& aName)
{
   auto lower = [](const std::string& aText)
   {
      std::string out(aText);
      for (char& c : out)
      {
         c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      }
      return out;
   };
   std::string needle = lower(aName + ":");
   std::string hay    = lower(aRequest);
   size_t      pos    = hay.find(needle);
   if (pos == std::string::npos)
   {
      return "";
   }
   pos += needle.size();
   while (pos < aRequest.size() && ((aRequest[pos] == ' ') || (aRequest[pos] == '\t')))
   {
      ++pos;
   }
   size_t end = aRequest.find("\r\n", pos);
   if (end == std::string::npos)
   {
      end = aRequest.size();
   }
   return aRequest.substr(pos, end - pos);
}

//! WebSocket 握手 Accept 值：Base64(SHA1(客户端 Key + RFC 6455 固定 GUID))
std::string WsAcceptKey(const std::string& aClientKey)
{
   static const char cGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
   return Base64Encode(Sha1(aClientKey + cGuid));
}

//! 封装一个 WebSocket 文本帧（服务端→客户端不掩码；7/16/64 位长度编码）
std::string WsTextFrame(const std::string& aPayload)
{
   std::string frame;
   frame.reserve(aPayload.size() + 10);
   frame += static_cast<char>(0x81); // FIN=1 + opcode=1（文本帧）
   size_t len = aPayload.size();
   if (len < 126)
   {
      frame += static_cast<char>(len);
   }
   else if (len < 65536)
   {
      frame += static_cast<char>(126);
      frame += static_cast<char>((len >> 8) & 0xFF);
      frame += static_cast<char>(len & 0xFF);
   }
   else
   {
      frame += static_cast<char>(127);
      for (int i = 7; i >= 0; --i)
      {
         frame += static_cast<char>((len >> (i * 8)) & 0xFF);
      }
   }
   frame += aPayload;
   return frame;
}

// ----------------------------------------------------------------------------
// 通道适配器抽象基类（阶段 B 统一消息中间件骨架）
// ----------------------------------------------------------------------------

//! 通道适配器基类：统一"仿真线程入队 → 独立网络线程消费广播"骨架
//! 派生类只实现 Run() 网络线程主体（accept/握手/组播等通道差异逻辑）
class Channel
{
public:
   explicit Channel(const std::string& aName)
      : mName(aName)
   {
   }
   virtual ~Channel() { Stop(); }
   Channel(const Channel&) = delete;
   Channel& operator=(const Channel&) = delete;

   //! 启动通道网络线程（通道参数经派生类构造函数注入）
   virtual bool Start() = 0;

   //! 排空队列后停机（保证 sim_complete 等尾帧送达客户端）
   void FlushAndStop()
   {
      if (mRunning.load())
      {
         // 网络线程循环节拍 20ms，预留 150ms 足以完成一轮队列排空
         std::this_thread::sleep_for(std::chrono::milliseconds(150));
      }
      Stop();
   }

   //! 停止网络线程（Complete 与析构兜底均调用，幂等）
   void Stop()
   {
      if (!mRunning.exchange(false))
      {
         return;
      }
      if (mThread.joinable())
      {
         mThread.join();
      }
   }

   //! 仿真回调线程调用：消息入队（含 '\n' 行结束符）
   void Enqueue(const std::string& aMessage)
   {
      if (!mRunning.load())
      {
         return; // 通道未启动/已停止：直接丢弃，绝不阻塞回调
      }
      std::lock_guard<std::mutex> lock(mQueueMutex);
      // 队列上限保护：丢最旧，防止慢客户端拖垮内存
      if (mQueue.size() >= cMAX_QUEUE_SIZE)
      {
         mQueue.pop_front();
      }
      mQueue.push_back(aMessage);
   }

   //! 是否仍在运行（bind 失败等异步降级后为 false）
   bool IsRunning() const { return mRunning.load(); }

   //! 通道监听端口（阶段 D 热切换：remove/list 按端口索引通道）
   virtual int Port() const = 0;

   const std::string& Name() const { return mName; }

protected:
   //! 启动网络线程（派生类 Start() 完成参数校验后调用）
   bool StartThread()
   {
      if (!EnsureWinsock())
      {
         return false;
      }
      if (mRunning.exchange(true))
      {
         return true; // 已在运行，防止重复启动
      }
      try
      {
         mThread = std::thread(&Channel::Run, this);
         return true;
      }
      catch (...)
      {
         // 线程创建失败（资源耗尽）：禁用该通道，不影响仿真本体
         mRunning = false;
         return false;
      }
   }

   //! 取出全部待发消息（仅网络线程调用）
   std::vector<std::string> DrainQueue()
   {
      std::vector<std::string> out;
      std::lock_guard<std::mutex> lock(mQueueMutex);
      if (!mQueue.empty())
      {
         out.assign(std::make_move_iterator(mQueue.begin()), std::make_move_iterator(mQueue.end()));
         mQueue.clear();
      }
      return out;
   }

   //! 新客户端接入时的快照补推来源（由仿真扩展注入，返回待发 JSON 行）
   std::function<std::vector<std::string>()> mSnapshotProvider;

   std::thread             mThread;         // 网络线程（accept + 广播）
   std::atomic<bool>       mRunning{false}; // 线程运行标志（Stop 幂等的依据）
   std::deque<std::string> mQueue;          // 待发消息队列（仿真线程生产，网络线程消费）
   std::mutex              mQueueMutex;     // 队列互斥锁
   std::string             mName;           // 通道名（日志定位用）

private:
   //! 网络线程主体（派生类实现）
   virtual void Run() = 0;
};

// ----------------------------------------------------------------------------
// TCP 通道适配器（阶段 A 逻辑重构为适配器形态 + 快照补推）
// ----------------------------------------------------------------------------

//! 流式 socket 公共选项：发送超时 200ms + 关闭 Nagle（TCP/WebSocket 通道共用）
void ApplyStreamSocketOptions(SOCKET aClient)
{
   // 发送超时 200ms：慢客户端最多拖住网络线程 200ms，绝不波及仿真线程
   int sendTimeoutMs = 200;
   setsockopt(aClient, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));
   // 关闭 Nagle：行协议/帧协议小包低延迟直达
   int noDelay = 1;
   setsockopt(aClient, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
}

class TcpChannel : public Channel
{
public:
   explicit TcpChannel(int aPort)
      : Channel("tcp")
      , mPort(aPort)
   {
   }

   bool Start() override { return StartThread(); }

   //! 通道监听端口（热切换 remove/list 索引用）
   int Port() const override { return mPort; }

   //! 注入快照提供者（新客户端接入即补推当前全量态势）
   void SetSnapshotProvider(std::function<std::vector<std::string>()> aProvider)
   {
      mSnapshotProvider = std::move(aProvider);
   }

private:
   //! 网络线程主循环：accept 新客户端（先补推快照）+ 广播队列消息
   void Run() override
   {
      // 1) 建立监听 socket
      mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (mListenSocket == INVALID_SOCKET)
      {
         auto out = ut::log::warning() << "ysim_data_export: tcp channel failed to create listen socket.";
         mRunning = false;
         return;
      }

      // 二次开发（ext-01）：SO_REUSEADDR 支持蒙特卡洛多轮运行间快速重绑端口
      int reuseAddr = 1;
      setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));

      sockaddr_in addr;
      ZeroMemory(&addr, sizeof(addr));
      addr.sin_family      = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port        = htons(static_cast<unsigned short>(mPort));
      if ((bind(mListenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) ||
          (listen(mListenSocket, 8) == SOCKET_ERROR))
      {
         // 端口被占用等：仅告警，仿真照常运行（该通道降级为不可用）
         auto out = ut::log::warning() << "ysim_data_export: tcp channel bind/listen failed on port " << mPort << ".";
         out.AddNote() << "TCP data export disabled for this run.";
         closesocket(mListenSocket);
         mListenSocket = INVALID_SOCKET;
         mRunning      = false;
         return;
      }

      // 2) 主循环：select 20ms 超时兼作节拍器（无消息时低 CPU 占用）
      while (mRunning.load())
      {
         // 2.1 非阻塞检查新连接
         fd_set readSet;
         FD_ZERO(&readSet);
         FD_SET(mListenSocket, &readSet);
         timeval timeout{0, 20000};
         if (select(0, &readSet, nullptr, nullptr, &timeout) > 0)
         {
            SOCKET client = accept(mListenSocket, nullptr, nullptr);
            if (client != INVALID_SOCKET)
            {
               ApplyStreamSocketOptions(client);
               // 二次开发（ext-01 阶段 B）：快照补推——接入即发送当前全量态势，
               // 中途接入的客户端不再"等下一帧才有画面"（发送失败静默，随后广播剔除）
               if (mSnapshotProvider)
               {
                  for (const std::string& msg : mSnapshotProvider())
                  {
                     send(client, msg.data(), static_cast<int>(msg.size()), 0);
                  }
               }
               // 客户端列表仅网络线程访问，无需加锁
               mClients.push_back(client);
            }
         }

         // 2.2 取出待发消息并向所有客户端广播
         std::vector<std::string> messages = DrainQueue();
         if (!messages.empty())
         {
            Broadcast(messages);
         }
      }

      // 3) 线程退出清理：关闭全部客户端连接与监听 socket
      for (SOCKET s : mClients)
      {
         closesocket(s);
      }
      mClients.clear();
      closesocket(mListenSocket);
      mListenSocket = INVALID_SOCKET;
   }

   //! 客户端 socket 选项已抽为自由函数 ApplyStreamSocketOptions（TCP/WS 通道共用）

   //! 广播一批消息；发送失败（断开/超时）的客户端被剔除，best-effort 语义
   void Broadcast(const std::vector<std::string>& aMessages)
   {
      for (size_t i = 0; i < mClients.size();)
      {
         bool alive = true;
         for (const std::string& msg : aMessages)
         {
            int sent = send(mClients[i], msg.data(), static_cast<int>(msg.size()), 0);
            if (sent == SOCKET_ERROR)
            {
               // 超时/连接重置：best-effort 丢弃该消息；硬错误则剔除该客户端
               int err = WSAGetLastError();
               if ((err != WSAETIMEDOUT) && (err != WSAEWOULDBLOCK))
               {
                  alive = false;
               }
               break;
            }
         }
         if (alive)
         {
            ++i;
         }
         else
         {
            closesocket(mClients[i]);
            mClients.erase(mClients.begin() + static_cast<long>(i));
         }
      }
   }

   int                  mPort;                    // 监听端口
   SOCKET               mListenSocket = INVALID_SOCKET; // 监听 socket
   std::vector<SOCKET>  mClients;                 // 已连接客户端（仅网络线程访问）
};

// ----------------------------------------------------------------------------
// WebSocket 通道适配器（最小 RFC 6455 服务端，单向推送）
// ----------------------------------------------------------------------------

class WebSocketChannel : public Channel
{
public:
   explicit WebSocketChannel(int aPort)
      : Channel("websocket")
      , mPort(aPort)
   {
   }

   bool Start() override { return StartThread(); }

   //! 通道监听端口（热切换 remove/list 索引用）
   int Port() const override { return mPort; }

   void SetSnapshotProvider(std::function<std::vector<std::string>()> aProvider)
   {
      mSnapshotProvider = std::move(aProvider);
   }

private:
   //! 网络线程主循环：accept+握手（快照补推）/ 服务上行帧 / 广播文本帧
   void Run() override
   {
      // 1) 建立监听 socket（与 TCP 通道同构）
      mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (mListenSocket == INVALID_SOCKET)
      {
         auto out = ut::log::warning() << "ysim_data_export: websocket channel failed to create listen socket.";
         mRunning = false;
         return;
      }
      int reuseAddr = 1;
      setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));
      sockaddr_in addr;
      ZeroMemory(&addr, sizeof(addr));
      addr.sin_family      = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port        = htons(static_cast<unsigned short>(mPort));
      if ((bind(mListenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) ||
          (listen(mListenSocket, 8) == SOCKET_ERROR))
      {
         auto out = ut::log::warning() << "ysim_data_export: websocket channel bind/listen failed on port " << mPort << ".";
         out.AddNote() << "WebSocket data export disabled for this run.";
         closesocket(mListenSocket);
         mListenSocket = INVALID_SOCKET;
         mRunning      = false;
         return;
      }

      // 2) 主循环：读集合覆盖监听 socket 与全部已握手客户端
      while (mRunning.load())
      {
         fd_set readSet;
         FD_ZERO(&readSet);
         FD_SET(mListenSocket, &readSet);
         for (const auto& kv : mClients)
         {
            FD_SET(kv.first, &readSet);
         }
         timeval timeout{0, 20000};
         if (select(0, &readSet, nullptr, nullptr, &timeout) > 0)
         {
            if (FD_ISSET(mListenSocket, &readSet))
            {
               AcceptClient();
            }
            // 服务已握手客户端的上行数据（close/ping 处理；文本帧忽略）
            for (auto it = mClients.begin(); it != mClients.end();)
            {
               if (FD_ISSET(it->first, &readSet) && (!ServiceClient(it->first, it->second)))
               {
                  closesocket(it->first);
                  it = mClients.erase(it);
                  continue;
               }
               ++it;
            }
         }

         // 3) 队列消息广播：每条 JSON 行封装为一个文本帧
         std::vector<std::string> messages = DrainQueue();
         if (!messages.empty())
         {
            std::vector<std::string> frames;
            frames.reserve(messages.size());
            for (const std::string& msg : messages)
            {
               frames.push_back(WsTextFrame(StripNewline(msg)));
            }
            Broadcast(frames);
         }
      }

      // 4) 清理
      for (auto& kv : mClients)
      {
         closesocket(kv.first);
      }
      mClients.clear();
      closesocket(mListenSocket);
      mListenSocket = INVALID_SOCKET;
   }

   //! 剥离 JSON 行尾的 '\n'（WebSocket 帧自带边界，无需行结束符）
   static std::string StripNewline(const std::string& aMessage)
   {
      if (!aMessage.empty() && (aMessage.back() == '\n'))
      {
         return aMessage.substr(0, aMessage.size() - 1);
      }
      return aMessage;
   }

   //! 接入新客户端：完成 HTTP Upgrade 握手 → 快照补推 → 加入客户端表
   void AcceptClient()
   {
      SOCKET client = accept(mListenSocket, nullptr, nullptr);
      if (client == INVALID_SOCKET)
      {
         return;
      }
      if (!Handshake(client))
      {
         // 握手失败（超时/非 WebSocket 请求）：直接断开
         closesocket(client);
         return;
      }
      ApplyStreamSocketOptions(client);
      // 二次开发（ext-01 阶段 B）：握手成功即快照补推当前全量态势
      if (mSnapshotProvider)
      {
         for (const std::string& msg : mSnapshotProvider())
         {
            std::string frame = WsTextFrame(StripNewline(msg));
            send(client, frame.data(), static_cast<int>(frame.size()), 0);
         }
      }
      mClients[client] = std::string(); // 上行帧缓冲（仅网络线程访问）
   }

   //! HTTP Upgrade 握手：5 秒内收齐请求头并回 101（含 Sec-WebSocket-Accept）
   bool Handshake(SOCKET aClient)
   {
      std::string request;
      auto        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      bool        complete = false;
      while (!complete && (std::chrono::steady_clock::now() < deadline) && mRunning.load())
      {
         fd_set readSet;
         FD_ZERO(&readSet);
         FD_SET(aClient, &readSet);
         timeval timeout{0, 100000}; // 100ms 轮询粒度
         if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0)
         {
            continue;
         }
         char buf[1024];
         int  got = recv(aClient, buf, sizeof(buf), 0);
         if (got <= 0)
         {
            return false; // 对端关闭
         }
         request.append(buf, got);
         complete = (request.find("\r\n\r\n") != std::string::npos);
      }
      if (!complete)
      {
         return false; // 超时未收齐请求头
      }
      std::string key = GetHttpHeader(request, "Sec-WebSocket-Key");
      if (key.empty())
      {
         return false; // 非 WebSocket 升级请求
      }
      std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                             "Upgrade: websocket\r\n"
                             "Connection: Upgrade\r\n"
                             "Sec-WebSocket-Accept: " +
                             WsAcceptKey(key) + "\r\n\r\n";
      return send(aClient, response.data(), static_cast<int>(response.size()), 0) != SOCKET_ERROR;
   }

   //! 服务客户端上行数据：仅处理 close（断开）与 ping（应答 pong），其余帧忽略
   //! @return false 表示客户端应剔除（连接关闭/协议异常/超长防御）
   bool ServiceClient(SOCKET aClient, std::string& aBuffer)
   {
      char tmp[4096];
      int  got = recv(aClient, tmp, sizeof(tmp), 0);
      if (got <= 0)
      {
         return false; // 对端关闭或错误
      }
      aBuffer.append(tmp, got);
      // 防御：客户端异常发送超长数据（>1MB）直接断开
      if (aBuffer.size() > (1u << 20))
      {
         return false;
      }
      // 逐帧解析（本通道为单向推送：文本/二进制等数据帧一律忽略）
      while (true)
      {
         if (aBuffer.size() < 2)
         {
            return true; // 帧头未收齐
         }
         unsigned b0      = static_cast<unsigned char>(aBuffer[0]);
         unsigned b1      = static_cast<unsigned char>(aBuffer[1]);
         bool     masked  = (b1 & 0x80) != 0;
         unsigned opcode  = b0 & 0x0F;
         uint64_t len     = b1 & 0x7F;
         size_t   headerLen = 2;
         if (len == 126)
         {
            if (aBuffer.size() < 4)
            {
               return true;
            }
            len       = (static_cast<uint64_t>(static_cast<unsigned char>(aBuffer[2])) << 8) | static_cast<unsigned char>(aBuffer[3]);
            headerLen = 4;
         }
         else if (len == 127)
         {
            if (aBuffer.size() < 10)
            {
               return true;
            }
            len = 0;
            for (int i = 0; i < 8; ++i)
            {
               len = (len << 8) | static_cast<unsigned char>(aBuffer[2 + i]);
            }
            headerLen = 10;
         }
         if (len > (1u << 20))
         {
            return false; // 超长帧防御
         }
         size_t maskLen = masked ? 4 : 0;
         if (aBuffer.size() < headerLen + maskLen + static_cast<size_t>(len))
         {
            return true; // 帧未收齐
         }
         if (opcode == 0x8)
         {
            return false; // 客户端主动关闭
         }
         if (opcode == 0x9)
         {
            // ping → 空 pong 应答（RFC 6455 心跳；空载荷为通用可接受实现）
            char pong[2] = {static_cast<char>(0x8A), 0};
            send(aClient, pong, 2, 0);
         }
         aBuffer.erase(0, headerLen + maskLen + static_cast<size_t>(len));
      }
   }

   //! 广播一批帧；发送失败的客户端被剔除，best-effort 语义
   void Broadcast(const std::vector<std::string>& aFrames)
   {
      for (auto it = mClients.begin(); it != mClients.end();)
      {
         bool alive = true;
         for (const std::string& frame : aFrames)
         {
            int sent = send(it->first, frame.data(), static_cast<int>(frame.size()), 0);
            if (sent == SOCKET_ERROR)
            {
               int err = WSAGetLastError();
               if ((err != WSAETIMEDOUT) && (err != WSAEWOULDBLOCK))
               {
                  alive = false;
               }
               break;
            }
         }
         if (alive)
         {
            ++it;
         }
         else
         {
            closesocket(it->first);
            it = mClients.erase(it);
         }
      }
   }

   int mPort;                                     // 监听端口
   SOCKET mListenSocket = INVALID_SOCKET;         // 监听 socket
   std::map<SOCKET, std::string> mClients;        // 已握手客户端及其上行帧缓冲（仅网络线程访问）
};

// ----------------------------------------------------------------------------
// UDP 通道适配器（组播/单播自适应发送端）
// ----------------------------------------------------------------------------

class UdpChannel : public Channel
{
public:
   UdpChannel(const std::string& aAddress, int aPort, int aTtl)
      : Channel("udp")
      , mAddress(aAddress)
      , mPort(aPort)
      , mTtl(aTtl)
   {
   }

   bool Start() override { return StartThread(); }

   //! 通道端口（热切换 remove/list 索引用；UDP 为目的端口）
   int Port() const override { return mPort; }

private:
   //! 网络线程主循环：每条队列消息封装为一个数据报发送
   void Run() override
   {
      // 1) 建立 UDP socket 并解析目的地址
      SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (sock == INVALID_SOCKET)
      {
         auto out = ut::log::warning() << "ysim_data_export: udp channel failed to create socket.";
         mRunning = false;
         return;
      }
      sockaddr_in dest;
      ZeroMemory(&dest, sizeof(dest));
      dest.sin_family = AF_INET;
      dest.sin_port   = htons(static_cast<unsigned short>(mPort));
      if (inet_pton(AF_INET, mAddress.c_str(), &dest.sin_addr) != 1)
      {
         auto out = ut::log::warning() << "ysim_data_export: invalid udp_address '" << mAddress << "'.";
         out.AddNote() << "UDP data export disabled for this run.";
         closesocket(sock);
         mRunning = false;
         return;
      }

      // 2) 组播地址（224-239 段）：设置 TTL 与本机环回（环回开启支持同机验证）
      unsigned firstOctet = ntohl(dest.sin_addr.s_addr) >> 24;
      if ((firstOctet >= 224) && (firstOctet <= 239))
      {
         int ttl = mTtl;
         setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl));
         int loop = 1;
         setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char*>(&loop), sizeof(loop));
      }

      // 3) 发送循环：队列空时节拍休眠，非空时立即批量外发
      bool warnOnce = false;
      while (mRunning.load())
      {
         std::vector<std::string> messages = DrainQueue();
         if (messages.empty())
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
         }
         for (const std::string& msg : messages)
         {
            // UDP 数据报长度防御（应用层消息均 <1KB，正常不会触发）
            if (msg.size() > 60000)
            {
               continue;
            }
            if (sendto(sock, msg.data(), static_cast<int>(msg.size()), 0, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) ==
                SOCKET_ERROR)
            {
               if (!warnOnce)
               {
                  // 只告警一次防刷屏（组播路由不可达等环境问题）
                  auto out = ut::log::warning() << "ysim_data_export: udp sendto failed (WSA error " << WSAGetLastError() << ").";
                  out.AddNote() << "Further UDP errors suppressed.";
                  warnOnce = true;
               }
            }
         }
      }
      closesocket(sock);
   }

   std::string mAddress; // 目的地址（组播组或单播 IP）
   int         mPort;    // 目的端口
   int         mTtl;     // 组播 TTL
};

// ----------------------------------------------------------------------------
// 结构化落盘器（阶段 C）：与通道平行的持久化 Sink
// ----------------------------------------------------------------------------

//! 落盘记录种类
enum class RecordKind
{
   cSTATE,  // 平台状态时序样本（platform_states.csv / states 表）
   cEVENT,  // 生命周期事件（events.csv / events 表）
   cMETRIC  // 评估指标（metrics.csv / metrics 表）
};

//! 落盘记录（平铺超集：单队列保序，落盘线程按 kind 分流写表）
//! 仿真回调线程构造入队（纳秒级填字段，无格式化），落盘线程消费
struct Record
{
   RecordKind         kind = RecordKind::cEVENT;
   double             time = 0.0;          // 仿真时间 [s]（state/event）
   unsigned long long platformIdx = 0;     // 平台索引（运行级指标为 0）
   std::string        name;                // 平台名 / 指标名
   std::string        name2;               // metric 记录的平台名（运行级指标为空）
   std::string        type;                // 平台类型（state；added 事件携带供快照表）
   std::string        side;                // 侧别（state/metric）
   std::string        text;                // 事件名（event）/ 指标单位（metric）
   double             lat        = 0.0;    // 纬度 [deg]
   double             lon        = 0.0;    // 经度 [deg]
   double             altM       = 0.0;    // 高度 [m]
   double             speedMps   = 0.0;    // 速度 [m/s]
   double             headingDeg = 0.0;    // 航向 [deg]
   double             pitchDeg   = 0.0;    // 俯仰 [deg]
   double             rollDeg    = 0.0;    // 滚转 [deg]
   double             value      = 0.0;    // 指标数值（metric）
};

//! platforms 快照表行缓存（断点状态恢复数据源，仅落盘线程访问）
struct PlatformRow
{
   double      addedTime   = 0.0;
   double      removedTime = -1.0e30; // 哨兵=仍在场
   std::string name;
   std::string type;
   std::string side;
   bool        hasState = false;
   double      lat = 0.0, lon = 0.0, altM = 0.0, speedMps = 0.0, headingDeg = 0.0, pitchDeg = 0.0, rollDeg = 0.0;
};

//! 结构化落盘器：仿真线程入队 → 独立落盘线程批量写 CSV + SQLite
//! 性能红线与通道一致：回调零阻塞（上限 10000 丢最旧），失败仅告警不拖垮仿真
class Recorder
{
public:
   Recorder(const std::string& aRootDir, bool aCsvEnabled, bool aSqliteEnabled)
      : mRootDir(aRootDir)
      , mCsvEnabled(aCsvEnabled)
      , mSqliteEnabled(aSqliteEnabled)
   {
   }

   ~Recorder() { Stop(); }
   Recorder(const Recorder&) = delete;
   Recorder& operator=(const Recorder&) = delete;

   //! 初始化落盘资源并启动落盘线程（仿真线程 Initialize 内调用）
   //! 目录/文件/库打开失败仅告警并降级对应 Sink，返回值不阻断仿真
   bool Start()
   {
      mRunDir = mRootDir + "/" + NowStamp();
      if (!EnsureDirectory(mRunDir))
      {
         auto out = ut::log::warning() << "ysim_data_export: recorder failed to create directory '" << mRunDir << "'.";
         out.AddNote() << "Data recording disabled for this run.";
         return false;
      }
      if (mCsvEnabled && (!OpenCsvFiles()))
      {
         mCsvEnabled = false;
      }
      if (mSqliteEnabled && (!OpenDatabase()))
      {
         mSqliteEnabled = false;
      }
      mRunning = true;
      try
      {
         mThread = std::thread(&Recorder::Run, this);
      }
      catch (...)
      {
         // 线程创建失败（资源耗尽）：禁用落盘，不影响仿真本体
         mRunning = false;
         CloseResources();
         return false;
      }
      return true;
   }

   //! 仿真回调线程调用：记录入队（含运行期事件/状态/指标）
   void Enqueue(Record&& aRecord)
   {
      if (!mRunning.load())
      {
         return;
      }
      std::lock_guard<std::mutex> lock(mQueueMutex);
      if (mQueue.size() >= cMAX_QUEUE_SIZE)
      {
         mQueue.pop_front(); // 队列上限保护：丢最旧
      }
      mQueue.push_back(std::move(aRecord));
   }

   //! 排空队列后停机（Complete 尾部调用，保证指标与结束事件落盘）
   void FlushAndStop()
   {
      if (mRunning.load())
      {
         // 落盘线程循环节拍 20ms，预留 150ms 足以完成一轮批量写
         std::this_thread::sleep_for(std::chrono::milliseconds(150));
      }
      Stop();
   }

   //! 停止落盘线程（Complete 与析构兜底均调用，幂等）
   void Stop()
   {
      if (!mRunning.exchange(false))
      {
         return;
      }
      if (mThread.joinable())
      {
         mThread.join();
      }
   }

   const std::string& RunDir() const { return mRunDir; }

private:
   //! 打开三个 CSV 文件并写入 BOM + 表头（UTF-8 with BOM：Excel/pandas 双友好）
   bool OpenCsvFiles()
   {
      mStatesFile = OpenCsvFile("platform_states.csv", "time,platform_idx,name,type,side,lat,lon,alt_m,speed_mps,heading_deg,pitch_deg,roll_deg");
      mEventsFile = OpenCsvFile("events.csv", "time,event,platform_idx,name");
      mMetricsFile = OpenCsvFile("metrics.csv", "metric,platform_idx,name,side,value,unit");
      if ((mStatesFile == nullptr) || (mEventsFile == nullptr) || (mMetricsFile == nullptr))
      {
         auto out = ut::log::warning() << "ysim_data_export: recorder failed to open CSV files under '" << mRunDir << "'.";
         out.AddNote() << "CSV output disabled for this run.";
         return false;
      }
      return true;
   }

   //! 打开单个 CSV 文件（二进制写 + UTF-8 BOM + 表头行）
   FILE* OpenCsvFile(const char* aFileName, const char* aHeader)
   {
      std::string path = mRunDir + "/" + aFileName;
      FILE*       f    = std::fopen(path.c_str(), "wb");
      if (f == nullptr)
      {
         return nullptr;
      }
      const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
      std::fwrite(bom, 1, 3, f);
      std::fprintf(f, "%s\n", aHeader);
      return f;
   }

   //! 打开 SQLite 库并建表/prepared 语句（journal/synchronous 关闭：分析导出吞吐优先）
   //! 二次开发（修复 2026-10-19）：无 SQLite 编译（YSIM_NO_SQLITE）时恒返回 false——
   //! Start() 按既有降级语义自动关闭 SQLite Sink，不产生告警噪声（编译期已告知）
   bool OpenDatabase()
   {
#ifndef YSIM_NO_SQLITE
      std::string path = mRunDir + "/ysim_export.db";
      if (sqlite3_open(path.c_str(), &mDb) != SQLITE_OK)
      {
         auto out = ut::log::warning() << "ysim_data_export: recorder failed to open sqlite database '" << path << "'.";
         out.AddNote() << (mDb ? sqlite3_errmsg(mDb) : "out of memory");
         sqlite3_close(mDb);
         mDb = nullptr;
         return false;
      }
      sqlite3_exec(mDb, "PRAGMA journal_mode=OFF; PRAGMA synchronous=OFF;", nullptr, nullptr, nullptr);
      const char* schema =
         "CREATE TABLE IF NOT EXISTS platforms("
         "platform_idx INTEGER PRIMARY KEY, name TEXT, type TEXT, side TEXT,"
         "added_time REAL, removed_time REAL, lat REAL, lon REAL, alt_m REAL,"
         "speed_mps REAL, heading_deg REAL, pitch_deg REAL, roll_deg REAL);"
         "CREATE TABLE IF NOT EXISTS states("
         "time REAL, platform_idx INTEGER, name TEXT, type TEXT, side TEXT,"
         "lat REAL, lon REAL, alt_m REAL, speed_mps REAL, heading_deg REAL, pitch_deg REAL, roll_deg REAL);"
         "CREATE TABLE IF NOT EXISTS events(time REAL, event TEXT, platform_idx INTEGER, name TEXT);"
         "CREATE TABLE IF NOT EXISTS metrics(metric TEXT, platform_idx INTEGER, name TEXT, side TEXT, value REAL, unit TEXT);";
      char* errMsg = nullptr;
      if ((sqlite3_exec(mDb, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) ||
          (sqlite3_prepare_v2(mDb, "INSERT INTO states VALUES(?,?,?,?,?,?,?,?,?,?,?,?);", -1, &mInsertState, nullptr) != SQLITE_OK) ||
          (sqlite3_prepare_v2(mDb, "INSERT INTO events VALUES(?,?,?,?);", -1, &mInsertEvent, nullptr) != SQLITE_OK) ||
          (sqlite3_prepare_v2(mDb, "INSERT INTO metrics VALUES(?,?,?,?,?,?);", -1, &mInsertMetric, nullptr) != SQLITE_OK))
      {
         auto out = ut::log::warning() << "ysim_data_export: recorder sqlite schema/prepare failed.";
         if (errMsg != nullptr)
         {
            out.AddNote() << errMsg;
            sqlite3_free(errMsg);
         }
         CloseResources();
         mSqliteEnabled = false;
         return false;
      }
      return true;
#else
      return false; // 编译期剔除：恒降级（SQLite Sink 不可用）
#endif
   }

   //! 落盘线程主体：批量消费队列 → CSV + SQLite 双写 → 收尾落快照表
   void Run()
   {
      while (mRunning.load())
      {
         std::vector<Record> batch = DrainQueue();
         if (batch.empty())
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
         }
#ifndef YSIM_NO_SQLITE
         if (mDb != nullptr)
         {
            sqlite3_exec(mDb, "BEGIN", nullptr, nullptr, nullptr);
         }
#endif
         for (const Record& rec : batch)
         {
            WriteRecord(rec);
         }
#ifndef YSIM_NO_SQLITE
         if (mDb != nullptr)
         {
            sqlite3_exec(mDb, "COMMIT", nullptr, nullptr, nullptr);
         }
#endif
         // 每批 fflush：仿真中途崩溃时已写数据不丢
         if (mStatesFile != nullptr)
         {
            std::fflush(mStatesFile);
         }
         if (mEventsFile != nullptr)
         {
            std::fflush(mEventsFile);
         }
         if (mMetricsFile != nullptr)
         {
            std::fflush(mMetricsFile);
         }
      }
      // 收尾：platforms 快照表（元信息 + 最终状态，断点恢复数据源）
      WritePlatformRows();
      CloseResources();
   }

   //! 取出全部待写记录（仅落盘线程调用）
   std::vector<Record> DrainQueue()
   {
      std::vector<Record> out;
      std::lock_guard<std::mutex> lock(mQueueMutex);
      if (!mQueue.empty())
      {
         out.assign(std::make_move_iterator(mQueue.begin()), std::make_move_iterator(mQueue.end()));
         mQueue.clear();
      }
      return out;
   }

   //! 单条记录分流写入（CSV + SQLite + 快照行缓存维护）
   void WriteRecord(const Record& aRec)
   {
      // 1) platforms 快照行缓存（事件建行/移除收口，状态刷新最终状态）
      if ((aRec.kind == RecordKind::cEVENT) && (aRec.text == "platform_added"))
      {
         PlatformRow& row  = mPlatformRows[aRec.platformIdx];
         row.addedTime     = aRec.time;
         row.name          = aRec.name;
         row.type          = aRec.type;
         row.side          = aRec.side;
      }
      else if ((aRec.kind == RecordKind::cEVENT) && (aRec.text == "platform_removed"))
      {
         auto it = mPlatformRows.find(aRec.platformIdx);
         if (it != mPlatformRows.end())
         {
            it->second.removedTime = aRec.time;
         }
      }
      else if (aRec.kind == RecordKind::cSTATE)
      {
         PlatformRow& row = mPlatformRows[aRec.platformIdx];
         if (row.name.empty())
         {
            // 防御：added 事件被队列上限丢弃时按 state 补建行
            row.addedTime = aRec.time;
            row.name      = aRec.name;
            row.type      = aRec.type;
            row.side      = aRec.side;
         }
         row.hasState    = true;
         row.lat         = aRec.lat;
         row.lon         = aRec.lon;
         row.altM        = aRec.altM;
         row.speedMps    = aRec.speedMps;
         row.headingDeg  = aRec.headingDeg;
         row.pitchDeg    = aRec.pitchDeg;
         row.rollDeg     = aRec.rollDeg;
      }

      // 2) CSV 写入（文本字段按 RFC 4180 转义）
      if (mCsvEnabled)
      {
         switch (aRec.kind)
         {
         case RecordKind::cSTATE:
            std::fprintf(mStatesFile,
                         "%.6f,%llu,%s,%s,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                         aRec.time,
                         aRec.platformIdx,
                         CsvEscape(aRec.name).c_str(),
                         CsvEscape(aRec.type).c_str(),
                         CsvEscape(aRec.side).c_str(),
                         aRec.lat,
                         aRec.lon,
                         aRec.altM,
                         aRec.speedMps,
                         aRec.headingDeg,
                         aRec.pitchDeg,
                         aRec.rollDeg);
            break;
         case RecordKind::cEVENT:
            std::fprintf(mEventsFile,
                         "%.6f,%s,%llu,%s\n",
                         aRec.time,
                         CsvEscape(aRec.text).c_str(),
                         aRec.platformIdx,
                         CsvEscape(aRec.name).c_str());
            break;
         case RecordKind::cMETRIC:
            std::fprintf(mMetricsFile,
                         "%s,%llu,%s,%s,%.6f,%s\n",
                         CsvEscape(aRec.name).c_str(),
                         aRec.platformIdx,
                         CsvEscape(aRec.name2).c_str(),
                         CsvEscape(aRec.side).c_str(),
                         aRec.value,
                         CsvEscape(aRec.text).c_str());
            break;
         }
      }

      // 3) SQLite 写入（prepared 语句逐参绑定后 step+reset，失败仅忽略，best-effort）
#ifndef YSIM_NO_SQLITE
      if (mDb != nullptr)
      {
         switch (aRec.kind)
         {
         case RecordKind::cSTATE:
            // 逐参绑定（C++14 兼容，无变参模板）：time/idx/name/type/side + 7 个状态量
            sqlite3_bind_double(mInsertState, 1, aRec.time);
            sqlite3_bind_int64(mInsertState, 2, static_cast<sqlite3_int64>(aRec.platformIdx));
            sqlite3_bind_text(mInsertState, 3, aRec.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(mInsertState, 4, aRec.type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(mInsertState, 5, aRec.side.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(mInsertState, 6, aRec.lat);
            sqlite3_bind_double(mInsertState, 7, aRec.lon);
            sqlite3_bind_double(mInsertState, 8, aRec.altM);
            sqlite3_bind_double(mInsertState, 9, aRec.speedMps);
            sqlite3_bind_double(mInsertState, 10, aRec.headingDeg);
            sqlite3_bind_double(mInsertState, 11, aRec.pitchDeg);
            sqlite3_bind_double(mInsertState, 12, aRec.rollDeg);
            sqlite3_step(mInsertState);
            sqlite3_reset(mInsertState);
            break;
         case RecordKind::cEVENT:
            sqlite3_bind_double(mInsertEvent, 1, aRec.time);
            sqlite3_bind_text(mInsertEvent, 2, aRec.text.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(mInsertEvent, 3, static_cast<sqlite3_int64>(aRec.platformIdx));
            sqlite3_bind_text(mInsertEvent, 4, aRec.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(mInsertEvent);
            sqlite3_reset(mInsertEvent);
            break;
         case RecordKind::cMETRIC:
            sqlite3_bind_text(mInsertMetric, 1, aRec.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(mInsertMetric, 2, static_cast<sqlite3_int64>(aRec.platformIdx));
            sqlite3_bind_text(mInsertMetric, 3, aRec.name2.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(mInsertMetric, 4, aRec.side.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(mInsertMetric, 5, aRec.value);
            sqlite3_bind_text(mInsertMetric, 6, aRec.text.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(mInsertMetric);
            sqlite3_reset(mInsertMetric);
            break;
         }
      }
#endif
   }

   //! 收尾写 platforms 快照表（元信息 + 最终状态；未移除平台 removed_time 记 -1）
   //! 二次开发（修复 2026-10-19）：无 SQLite 编译时空实现（成员 mDb 已条件剔除）
   void WritePlatformRows()
   {
#ifndef YSIM_NO_SQLITE
      if ((mDb == nullptr) || mPlatformRows.empty())
      {
         return;
      }
      sqlite3_exec(mDb, "BEGIN", nullptr, nullptr, nullptr);
      sqlite3_stmt* stmt = nullptr;
      if (sqlite3_prepare_v2(mDb,
                             "INSERT OR REPLACE INTO platforms VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);",
                             -1,
                             &stmt,
                             nullptr) == SQLITE_OK)
      {
         for (const auto& kv : mPlatformRows)
         {
            const PlatformRow& row = kv.second;
            double removed = (row.removedTime > -1.0e29) ? row.removedTime : -1.0;
            sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(kv.first));
            sqlite3_bind_text(stmt, 2, row.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, row.type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, row.side.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 5, row.addedTime);
            sqlite3_bind_double(stmt, 6, removed);
            sqlite3_bind_double(stmt, 7, row.hasState ? row.lat : -1.0e30);
            sqlite3_bind_double(stmt, 8, row.hasState ? row.lon : -1.0e30);
            sqlite3_bind_double(stmt, 9, row.hasState ? row.altM : -1.0e30);
            sqlite3_bind_double(stmt, 10, row.hasState ? row.speedMps : -1.0e30);
            sqlite3_bind_double(stmt, 11, row.hasState ? row.headingDeg : -1.0e30);
            sqlite3_bind_double(stmt, 12, row.hasState ? row.pitchDeg : -1.0e30);
            sqlite3_bind_double(stmt, 13, row.hasState ? row.rollDeg : -1.0e30);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
         }
         sqlite3_finalize(stmt);
      }
      sqlite3_exec(mDb, "COMMIT", nullptr, nullptr, nullptr);
#endif
   }

   //! 释放全部落盘资源（文件句柄 + prepared 语句 + 数据库连接）
   void CloseResources()
   {
#ifndef YSIM_NO_SQLITE
      if (mInsertState != nullptr)
      {
         sqlite3_finalize(mInsertState);
         mInsertState = nullptr;
      }
      if (mInsertEvent != nullptr)
      {
         sqlite3_finalize(mInsertEvent);
         mInsertEvent = nullptr;
      }
      if (mInsertMetric != nullptr)
      {
         sqlite3_finalize(mInsertMetric);
         mInsertMetric = nullptr;
      }
      if (mDb != nullptr)
      {
         sqlite3_close(mDb);
         mDb = nullptr;
      }
#endif
      if (mStatesFile != nullptr)
      {
         std::fclose(mStatesFile);
         mStatesFile = nullptr;
      }
      if (mEventsFile != nullptr)
      {
         std::fclose(mEventsFile);
         mEventsFile = nullptr;
      }
      if (mMetricsFile != nullptr)
      {
         std::fclose(mMetricsFile);
         mMetricsFile = nullptr;
      }
   }

   std::string   mRootDir;                   // 输出根目录（配置）
   std::string   mRunDir;                    // 本轮运行子目录（时间戳命名）
   bool          mCsvEnabled    = true;      // CSV Sink 开关（失败自动降级）
   bool          mSqliteEnabled = true;      // SQLite Sink 开关（失败自动降级）
   FILE*         mStatesFile    = nullptr;   // platform_states.csv
   FILE*         mEventsFile    = nullptr;   // events.csv
   FILE*         mMetricsFile   = nullptr;   // metrics.csv
#ifndef YSIM_NO_SQLITE
   sqlite3*      mDb            = nullptr;   // ysim_export.db 连接
   sqlite3_stmt* mInsertState   = nullptr;   // states 表 prepared 插入
   sqlite3_stmt* mInsertEvent   = nullptr;   // events 表 prepared 插入
   sqlite3_stmt* mInsertMetric  = nullptr;   // metrics 表 prepared 插入
#endif
   std::map<unsigned long long, PlatformRow> mPlatformRows; // 快照行缓存（仅落盘线程）
   std::thread             mThread;          // 落盘线程
   std::atomic<bool>       mRunning{false};  // 线程运行标志
   std::deque<Record>      mQueue;           // 待写记录队列（仿真线程生产，落盘线程消费）
   std::mutex              mQueueMutex;      // 队列互斥锁
};
} // namespace

namespace ysim
{
namespace dataexport
{
//! 插件配置数据：由 ApplicationExtension（命令行）与 ScenarioExtension（想定块）共同填充
struct InputData
{
   int         mTcpPort              = 31600;        // TCP 通道端口（0=禁用）
   int         mWsPort               = 31601;        // WebSocket 通道端口（0=禁用）
   int         mUdpPort              = 31602;        // UDP 通道端口（0=禁用）
   std::string mUdpAddress           = "239.16.16.16"; // UDP 目的地址（224-239 段按组播）
   int         mUdpTtl               = 4;            // 组播 TTL
   double      mHeartbeatInterval    = 5.0;          // 心跳周期 [s]（仿真时间）
   double      mDeadband             = 2.0;          // 位置死区 [m]：位移小于该值跳过推送
   double      mMinUpdateInterval    = 0.2;          // 同一平台两次推送最小间隔 [s]（降采样）
   bool        mTcpPortFromCmdLine   = false;        // TCP 端口已被 --ysim-port 锁定
   bool        mWsPortFromCmdLine    = false;        // WS 端口已被 --ysim-ws-port 锁定
   bool        mUdpPortFromCmdLine   = false;        // UDP 端口已被 --ysim-udp-port 锁定
   // 二次开发（ext-01 阶段 C）：结构化落盘配置（默认关闭：写文件是有副作用动作，显式启用）
   bool        mRecord               = false;        // 落盘总开关
   std::string mRecordDirectory      = "ysim_export"; // 输出根目录（相对 CWD）
   bool        mRecordCsv            = true;         // CSV Sink 开关（record=true 时生效）
   bool        mRecordSqlite         = true;         // SQLite Sink 开关（record=true 时生效）
   bool        mRecordFromCmdLine    = false;        // 落盘已被 --ysim-record 锁定
   // 二次开发（ext-01 阶段 D）：热切换控制通道（默认 0=禁用）
   int         mControlPort          = 0;            // 控制端口（add/remove/list 命令）
   bool        mControlPortFromCmdLine = false;      // 控制端口已被 --ysim-control-port 锁定
   bool        mStartPaused          = false;       // 起步暂停（首帧即暂停，等控制通道 resume）
};

//! 单平台评估指标聚合器（阶段 C：仿真回调线程内联累计，无锁）
//! 与推送流同源同节流——落盘数据与实时流严格一致
struct PlatformStats
{
   double             addedTime   = 0.0;
   double             removedTime = -1.0e30;  // 哨兵=仍在场
   unsigned long long samples     = 0;        // 状态样本数
   double             speedSum    = 0.0;      // 速度累计（求平均）
   double             speedMax    = 0.0;      // 最大速度
   double             distanceM   = 0.0;      // 累计航程（相邻样本位移和，节流下近似）
   bool               hasLast     = false;    // 是否已有上一采样点
   double             lastLat     = 0.0;      // 上一采样纬度 [rad]
   double             lastLon     = 0.0;      // 上一采样经度 [rad]
   double             lastAlt     = 0.0;      // 上一采样高度 [m]
   std::string        name;
   std::string        type;
   std::string        side;
};

//! 单平台推送节流状态（死区 + 降采样的判据缓存）
struct PlatformThrottle
{
   double mLastTime = -1.0e30; // 上次推送的仿真时间（极小值哨兵=从未推送）
   double mLastLat  = 0.0;     // 上次推送的纬度 [rad]
   double mLastLon  = 0.0;     // 上次推送的经度 [rad]
   double mLastAlt  = 0.0;     // 上次推送的高度 [m]
};

// ----------------------------------------------------------------------------
// 帧构造器：全部走 snprintf 单模板（消灭多行 "+" 拼接丢引号的历史问题）
// ----------------------------------------------------------------------------

//! 简单帧（sim_start/sim_complete/heartbeat）：type + time
std::string BuildSimpleFrame(const char* aType, double aTime)
{
   char buf[128];
   std::snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"time\":%.6f}\n", aType, aTime);
   return std::string(buf);
}

//! 平台加入帧：元信息（id/名称/类型/侧别）
std::string BuildPlatformAdded(double aTime, WsfPlatform* aPlatformPtr)
{
   char buf[1024];
   int  n = std::snprintf(buf,
                         sizeof(buf),
                         "{\"type\":\"platform_added\",\"time\":%.6f,\"id\":%llu,"
                         "\"name\":\"%s\",\"platform_type\":\"%s\",\"side\":\"%s\"}\n",
                         aTime,
                         static_cast<unsigned long long>(aPlatformPtr->GetIndex()),
                         JsonEscape(aPlatformPtr->GetName()).c_str(),
                         JsonEscape(aPlatformPtr->GetType()).c_str(),
                         JsonEscape(aPlatformPtr->GetSide()).c_str());
   // 返回空串表示超长截断（防御极端平台名），调用方跳过推送
   return ((n > 0) && (n < static_cast<int>(sizeof(buf)))) ? std::string(buf) : std::string();
}

//! 平台移除帧
std::string BuildPlatformRemoved(double aTime, WsfPlatform* aPlatformPtr)
{
   char buf[512];
   int  n = std::snprintf(buf,
                         sizeof(buf),
                         "{\"type\":\"platform_removed\",\"time\":%.6f,\"id\":%llu,\"name\":\"%s\"}\n",
                         aTime,
                         static_cast<unsigned long long>(aPlatformPtr->GetIndex()),
                         JsonEscape(aPlatformPtr->GetName()).c_str());
   return ((n > 0) && (n < static_cast<int>(sizeof(buf)))) ? std::string(buf) : std::string();
}

//! 平台状态帧：WGS84 经纬度（度）、高度（米）、速度（m/s）、姿态（度）
std::string BuildStateFrame(double          aTime,
                            WsfPlatform*    aPlatformPtr,
                            double          aLatDeg,
                            double          aLonDeg,
                            double          aAltM,
                            double          aSpeedMps,
                            double          aHeadingDeg,
                            double          aPitchDeg,
                            double          aRollDeg)
{
   char buf[1024];
   int  n = std::snprintf(buf,
                         sizeof(buf),
                         "{\"type\":\"state\",\"time\":%.6f,\"id\":%llu,"
                         "\"name\":\"%s\",\"platform_type\":\"%s\",\"side\":\"%s\","
                         "\"lat\":%.6f,\"lon\":%.6f,\"alt_m\":%.6f,\"speed_mps\":%.6f,"
                         "\"heading_deg\":%.6f,\"pitch_deg\":%.6f,\"roll_deg\":%.6f}\n",
                         aTime,
                         static_cast<unsigned long long>(aPlatformPtr->GetIndex()),
                         JsonEscape(aPlatformPtr->GetName()).c_str(),
                         JsonEscape(aPlatformPtr->GetType()).c_str(),
                         JsonEscape(aPlatformPtr->GetSide()).c_str(),
                         aLatDeg,
                         aLonDeg,
                         aAltM,
                         aSpeedMps,
                         aHeadingDeg,
                         aPitchDeg,
                         aRollDeg);
   return ((n > 0) && (n < static_cast<int>(sizeof(buf)))) ? std::string(buf) : std::string();
}

// ----------------------------------------------------------------------------
// 仿真扩展：Observer 订阅 + 多通道数据外发的运行时载体
// ----------------------------------------------------------------------------

class SimulationExtension;

// ----------------------------------------------------------------------------
// 热切换控制通道（阶段 D）：TCP 行协议命令，运行期增删数据通道不重启仿真
// ----------------------------------------------------------------------------

//! 控制通道：独立线程 accept 控制客户端，解析命令并操作 SimulationExtension 通道列表
//! 命令集（空格分隔 token）：add_tcp <port> / add_ws <port> / add_udp <addr> <port> [ttl]
//!                           remove <port> / list / help
//! 响应 JSON 行：{"type":"control_result","ok":...,"detail":"..."} / {"type":"channel_list",...}
class ControlChannel
{
public:
   ControlChannel(int aPort, SimulationExtension* aOwner)
      : mPort(aPort)
      , mOwner(aOwner)
   {
   }

   ~ControlChannel() { Stop(); }
   ControlChannel(const ControlChannel&) = delete;
   ControlChannel& operator=(const ControlChannel&) = delete;

   //! 启动控制线程（bind 失败仅告警，仿真照常）
   bool Start()
   {
      if (!EnsureWinsock())
      {
         return false;
      }
      if (mRunning.exchange(true))
      {
         return true;
      }
      try
      {
         mThread = std::thread(&ControlChannel::Run, this);
         return true;
      }
      catch (...)
      {
         mRunning = false;
         return false;
      }
   }

   //! 停止控制线程（Complete 与析构兜底均调用，幂等）
   void Stop()
   {
      if (!mRunning.exchange(false))
      {
         return;
      }
      if (mThread.joinable())
      {
         mThread.join();
      }
   }

private:
   void Run();

   //! 处理一行命令，返回响应 JSON 行（不含 '\n'）
   std::string HandleCommand(const std::string& aLine);

   int                mPort;     // 控制监听端口
   SimulationExtension* mOwner;  // 通道列表持有者（热切换操作目标）
   std::thread        mThread;   // 控制线程
   std::atomic<bool>  mRunning{false};
};

class SimulationExtension : public WsfSimulationExtension
{
public:
   explicit SimulationExtension(const InputData& aData)
      : mData(aData)
   {
   }

   ~SimulationExtension() override
   {
      // 兜底：Complete() 未被调用（异常路径）时也保证网络线程随扩展销毁而终止
      if (mControlChannel != nullptr)
      {
         mControlChannel->Stop();
      }
      std::unique_lock<std::shared_timed_mutex> lock(mChannelsMutex);
      for (auto& channel : mChannels)
      {
         channel->Stop();
      }
   }

   //! 仿真初始化：订阅只读观察者事件并按配置装配通道适配器
   bool Initialize() override
   {
      // 二次开发（ext-01）：订阅观察者是零侵入采集的全部入口
      // 事件来源：WsfPlatformObserver / WsfMoverObserver / WsfSimulationObserver
      WsfSimulation& sim = GetSimulation();
      mCallbacks.Add(WsfObserver::PlatformAdded(&sim).Connect(&SimulationExtension::OnPlatformAdded, this));
      mCallbacks.Add(WsfObserver::PlatformDeleted(&sim).Connect(&SimulationExtension::OnPlatformDeleted, this));
      mCallbacks.Add(WsfObserver::MoverUpdated(&sim).Connect(&SimulationExtension::OnMoverUpdated, this));
      mCallbacks.Add(WsfObserver::AdvanceTime(&sim).Connect(&SimulationExtension::OnAdvanceTime, this));

      // 快照提供者：新客户端接入时补推当前全量态势（元信息 + 最近状态帧）
      auto snapshot = [this]() { return BuildSnapshot(); };

      // 二次开发（ext-01 阶段 B）：按配置装配通道适配器（可同时多通道；0=禁用）
      // 启动失败（端口占用等）由通道自身告警，仿真本体照常运行
      if (mData.mTcpPort > 0)
      {
         auto channel   = ut::make_unique<TcpChannel>(mData.mTcpPort);
         channel->SetSnapshotProvider(snapshot);
         channel->Start();
         mChannels.push_back(std::move(channel));
      }
      if (mData.mWsPort > 0)
      {
         auto channel   = ut::make_unique<WebSocketChannel>(mData.mWsPort);
         channel->SetSnapshotProvider(snapshot);
         channel->Start();
         mChannels.push_back(std::move(channel));
      }
      if (mData.mUdpPort > 0)
      {
         auto channel = ut::make_unique<UdpChannel>(mData.mUdpAddress, mData.mUdpPort, mData.mUdpTtl);
         channel->Start();
         mChannels.push_back(std::move(channel));
      }

      // 二次开发（ext-01 阶段 C）：按配置装配结构化落盘器（默认关闭，record true 启用）
      // 启动失败（目录/文件/数据库打开失败）由 Recorder 自身告警降级，仿真本体照常运行
      if (mData.mRecord)
      {
         mRecorder = ut::make_unique<Recorder>(mData.mRecordDirectory, mData.mRecordCsv, mData.mRecordSqlite);
         if (mRecorder->Start())
         {
            auto out = ut::log::info() << "ysim_data_export: recorder started, output directory '" << mRecorder->RunDir() << "'.";
         }
         else
         {
            mRecorder.reset();
         }
      }

      // 二次开发（e2e 控制面板）：start_paused 置位（OnAdvanceTime 仿真首帧消费）
      mStartPauseArmed.store(mData.mStartPaused);

      // 二次开发（ext-01 阶段 D）：热切换控制通道（control_port > 0 启用，默认禁用）
      if (mData.mControlPort > 0)
      {
         mControlChannel = ut::make_unique<ControlChannel>(mData.mControlPort, this);
         if (mControlChannel->Start())
         {
            auto out = ut::log::info() << "ysim_data_export: control channel started on port " << mData.mControlPort << ".";
            out.AddNote() << "Hot-swap commands: add_tcp/add_ws/add_udp/remove/list/pause/resume/set_rate/status/help.";
         }
         else
         {
            mControlChannel.reset();
         }
      }
      return true;
   }

   //! 仿真开始：通知外部客户端数据流开始
   void Start() override
   {
      Publish(BuildSimpleFrame("sim_start", GetSimulation().GetSimTime()));
      RecordEvent(GetSimulation().GetSimTime(), "sim_start", nullptr);
   }

   //! 仿真结束：推送结束帧并排空各通道队列后停机（保证客户端收到完整数据流）
   void Complete(double aSimTime) override
   {
      Publish(BuildSimpleFrame("sim_complete", aSimTime));
      RecordEvent(aSimTime, "sim_complete", nullptr);
      // 二次开发（ext-01 阶段 C）：收口前生成评估指标（保证指标落盘）
      GenerateMetrics(aSimTime);
      // 二次开发（ext-01 阶段 D）：先停控制通道（不再接受热切换命令）
      if (mControlChannel != nullptr)
      {
         mControlChannel->Stop();
      }
      // 二次开发（ext-01 阶段 D）：统一排空等待后并行停机
      // 原实现逐通道串行 FlushAndStop（各含 150ms 排空等待）→ 尾延迟 5×150ms 累加，
      // 帧率基准暴露（600s 想定 +24% 墙钟）；改为单次等待 100ms（3 个网络节拍冗余）
      // 后逐通道 Stop（join 并行返回），尾延迟收敛到 ~100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      {
         std::unique_lock<std::shared_timed_mutex> lock(mChannelsMutex);
         for (auto& channel : mChannels)
         {
            channel->Stop();
         }
      }
      if (mRecorder != nullptr)
      {
         mRecorder->Stop();
      }
   }

   // -------------------------------------------------------------------------
   // 二次开发（ext-01 阶段 D）：热切换公共接口（控制线程调用，操作通道列表）
   // -------------------------------------------------------------------------

   //! 运行期新增数据通道（type: "tcp"/"ws"/"udp"）；返回 false 时 aDetail 带失败原因
   bool AddChannel(const std::string& aType, const std::string& aAddress, int aPort, int aTtl, std::string& aDetail)
   {
      // 锁外构造并启动：bind 失败（端口占用等）不触碰通道列表
      auto snapshot = [this]() { return BuildSnapshot(); };
      std::unique_ptr<Channel> channel;
      if (aType == "tcp")
      {
         auto c = ut::make_unique<TcpChannel>(aPort);
         c->SetSnapshotProvider(snapshot);
         channel = std::move(c);
      }
      else if (aType == "ws")
      {
         auto c = ut::make_unique<WebSocketChannel>(aPort);
         c->SetSnapshotProvider(snapshot);
         channel = std::move(c);
      }
      else if (aType == "udp")
      {
         channel = ut::make_unique<UdpChannel>(aAddress, aPort, aTtl);
      }
      else
      {
         aDetail = "unknown channel type: " + aType;
         return false;
      }
      // 端口冲突预检：同端口通道已在列表则拒绝（bind 失败路径由 Start 自行告警）
      {
         std::shared_lock<std::shared_timed_mutex> lock(mChannelsMutex);
         for (const auto& existing : mChannels)
         {
            if (existing->Port() == aPort)
            {
               aDetail = "port " + std::to_string(aPort) + " already in use by " + existing->Name() + " channel";
               return false;
            }
         }
      }
      if (!channel->Start())
      {
         aDetail = "channel start failed (port " + std::to_string(aPort) + " bind/listen error)";
         return false;
      }
      {
         std::unique_lock<std::shared_timed_mutex> lock(mChannelsMutex);
         mChannels.push_back(std::move(channel));
      }
      aDetail = aType + " channel added on port " + std::to_string(aPort);
      auto out = ut::log::info() << "ysim_data_export: hot-swap " << aDetail << ".";
      return true;
   }

   //! 按端口移除数据通道（锁内摘链、锁外停机——FlushAndStop 含 150ms 排空不可持锁）
   bool RemoveChannelByPort(int aPort, std::string& aDetail)
   {
      if (aPort == mData.mControlPort)
      {
         aDetail = "control port cannot be removed";
         return false;
      }
      std::unique_ptr<Channel> removed;
      {
         std::unique_lock<std::shared_timed_mutex> lock(mChannelsMutex);
         auto it = std::find_if(mChannels.begin(), mChannels.end(),
                                [aPort](const std::unique_ptr<Channel>& aCh) { return aCh->Port() == aPort; });
         if (it == mChannels.end())
         {
            aDetail = "no channel on port " + std::to_string(aPort);
            return false;
         }
         removed = std::move(*it);
         mChannels.erase(it);
      }
      const std::string name = removed->Name();
      removed->FlushAndStop(); // 锁外排空停机（此期间仿真 Publish 已不再投递该通道）
      aDetail = name + " channel removed from port " + std::to_string(aPort);
      auto out = ut::log::info() << "ysim_data_export: hot-swap " << aDetail << ".";
      return true;
   }

   //! 当前通道清单（JSON 数组文本，供控制通道 list 命令）
   std::vector<std::pair<std::string, int>> ListChannels() const
   {
      std::vector<std::pair<std::string, int>> out;
      std::shared_lock<std::shared_timed_mutex> lock(mChannelsMutex);
      for (const auto& channel : mChannels)
      {
         out.emplace_back(channel->Name(), channel->Port());
      }
      return out;
   }

private:
   //! 消息发布：同时投递到全部已装配通道（shared_lock 只读——与热切换增删并发安全）
   void Publish(const std::string& aFrame)
   {
      if (aFrame.empty())
      {
         return;
      }
      std::shared_lock<std::shared_timed_mutex> lock(mChannelsMutex);
      for (auto& channel : mChannels)
      {
         channel->Enqueue(aFrame);
      }
   }

   //! 平台加入仿真：推送生命周期事件并缓存元信息帧（快照补推用）
   void OnPlatformAdded(double aSimTime, WsfPlatform* aPlatformPtr)
   {
      mThrottle[aPlatformPtr->GetIndex()] = PlatformThrottle{};
      // 二次开发（ext-01 阶段 C）：指标聚合器建档 + 生命周期事件落盘
      PlatformStats& stats = mStats[aPlatformPtr->GetIndex()];
      stats.addedTime      = aSimTime;
      stats.name           = aPlatformPtr->GetName();
      stats.type           = aPlatformPtr->GetType();
      stats.side           = aPlatformPtr->GetSide();
      RecordEvent(aSimTime, "platform_added", aPlatformPtr);
      std::string frame    = BuildPlatformAdded(aSimTime, aPlatformPtr);
      if (!frame.empty())
      {
         {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            mMetaFrames[aPlatformPtr->GetIndex()] = frame;
         }
         Publish(frame);
      }
   }

   //! 平台移出仿真（删除/损毁）：推送事件并清理全部缓存
   void OnPlatformDeleted(double aSimTime, WsfPlatform* aPlatformPtr)
   {
      // 二次开发（ext-01 阶段 C）：指标聚合器收口 + 生命周期事件落盘
      auto statsIt = mStats.find(aPlatformPtr->GetIndex());
      if (statsIt != mStats.end())
      {
         statsIt->second.removedTime = aSimTime;
      }
      RecordEvent(aSimTime, "platform_removed", aPlatformPtr);
      mThrottle.erase(aPlatformPtr->GetIndex());
      {
         std::lock_guard<std::mutex> lock(mCacheMutex);
         mMetaFrames.erase(aPlatformPtr->GetIndex());
         mStateFrames.erase(aPlatformPtr->GetIndex());
      }
      Publish(BuildPlatformRemoved(aSimTime, aPlatformPtr));
   }

   //! 平台运动学更新：核心态势数据帧（增量推送：死区 + 降采样双重过滤）
   void OnMoverUpdated(double aSimTime, WsfMover* aMoverPtr)
   {
      WsfPlatform* platformPtr = aMoverPtr->GetPlatform();
      if (platformPtr == nullptr)
      {
         return;
      }

      // 过滤一（降采样）：距上次推送不足最小间隔则跳过，防止高频回调打满网络
      PlatformThrottle& throttle = mThrottle[platformPtr->GetIndex()];
      if (aSimTime - throttle.mLastTime < mData.mMinUpdateInterval)
      {
         return;
      }

      double lat = 0.0;
      double lon = 0.0;
      double alt = 0.0;
      platformPtr->GetLocationLLA(lat, lon, alt);

      // 过滤二（死区）：粗略位移估计小于死区阈值则跳过（静止平台不重复推送）
      // 近似公式：北向 dN=Δlat*R，东向 dE=Δlon*R*cos(lat)，垂直 dU=Δalt
      if (throttle.mLastTime > -1.0e29)
      {
         double dLat = lat - throttle.mLastLat;
         double dLon = lon - throttle.mLastLon;
         double dN   = dLat * 6371000.0;
         double dE   = dLon * 6371000.0 * std::cos(lat);
         double dU   = alt - throttle.mLastAlt;
         if (std::sqrt(dN * dN + dE * dE + dU * dU) < mData.mDeadband)
         {
            return;
         }
      }

      throttle.mLastTime = aSimTime;
      throttle.mLastLat  = lat;
      throttle.mLastLon  = lon;
      throttle.mLastAlt  = alt;

      // 姿态：NED 航向/俯仰/滚转 [rad]，IO 边界统一换算为度（内核量纲红线）
      double heading = 0.0;
      double pitch   = 0.0;
      double roll    = 0.0;
      platformPtr->GetOrientationNED(heading, pitch, roll);

      // 组装 state 帧（WGS84 经纬度换算为度）并缓存最近一帧（快照补推用）
      std::string frame = BuildStateFrame(aSimTime,
                                          platformPtr,
                                          lat * UtMath::cDEG_PER_RAD,
                                          lon * UtMath::cDEG_PER_RAD,
                                          alt,
                                          platformPtr->GetSpeed(),
                                          heading * UtMath::cDEG_PER_RAD,
                                          pitch * UtMath::cDEG_PER_RAD,
                                          roll * UtMath::cDEG_PER_RAD);
      if (!frame.empty())
      {
         {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            mStateFrames[platformPtr->GetIndex()] = frame;
         }
         Publish(frame);
      }

      // 二次开发（ext-01 阶段 C）：状态样本落盘 + 指标聚合（与推送流同源同节流）
      RecordState(aSimTime,
                  platformPtr,
                  lat * UtMath::cDEG_PER_RAD,
                  lon * UtMath::cDEG_PER_RAD,
                  alt,
                  platformPtr->GetSpeed(),
                  heading * UtMath::cDEG_PER_RAD,
                  pitch * UtMath::cDEG_PER_RAD,
                  roll * UtMath::cDEG_PER_RAD);
      PlatformStats& stats = mStats[platformPtr->GetIndex()];
      if (stats.name.empty())
      {
         // 防御：added 事件先于订阅生效等极端时序时补建档
         stats.addedTime = aSimTime;
         stats.name      = platformPtr->GetName();
         stats.type      = platformPtr->GetType();
         stats.side      = platformPtr->GetSide();
      }
      const double speed = platformPtr->GetSpeed();
      stats.samples += 1;
      stats.speedSum += speed;
      if (speed > stats.speedMax)
      {
         stats.speedMax = speed;
      }
      if (stats.hasLast)
      {
         // 航程累计：相邻采样位移和（与死区同式的近似公式）
         double dLat = lat - stats.lastLat;
         double dLon = lon - stats.lastLon;
         double dN   = dLat * 6371000.0;
         double dE   = dLon * 6371000.0 * std::cos(lat);
         double dU   = alt - stats.lastAlt;
         stats.distanceM += std::sqrt(dN * dN + dE * dE + dU * dU);
      }
      stats.hasLast = true;
      stats.lastLat = lat;
      stats.lastLon = lon;
      stats.lastAlt = alt;
      mTotalStateSamples += 1;
   }

   //! 仿真时间推进（每帧触发，等价"帧完成"回调）：按周期推送心跳帧
   void OnAdvanceTime(double aSimTime)
   {
      // 二次开发（e2e 控制面板）：start_paused——仿真首帧即刻暂停时钟，等待控制通道
      // resume 指令（首帧时平台已全部就绪，WS 快照可补推完整初始态势）
      if (mStartPauseArmed.exchange(false))
      {
         PauseByControl();
         auto out = ut::log::info() << "ysim_data_export: start_paused in effect, simulation paused at t="
                                    << aSimTime << " s (send 'resume' via control channel).";
      }
      if (aSimTime - mLastHeartbeat >= mData.mHeartbeatInterval)
      {
         mLastHeartbeat = aSimTime;
         Publish(BuildSimpleFrame("heartbeat", aSimTime));
      }
   }

   //! 二次开发（e2e 控制面板）：控制通道暂停仿真（时钟停摆 + 状态记录）
   //! 线程安全依据：Pause/Resume 最终作用于时钟源标志（WsfClockSource::StopClock/
   //! StartClock 为平凡 bool 写入），且 SimulationPausing/Resuming 观察者全源码树无订阅者，
   //! 控制线程直调安全（官方 DIS 亦在网络线程跨线程调用 SetClockRate，同源先例）
   void PauseByControl()
   {
      GetSimulation().Pause();
      mPausedByControl.store(true);
   }

   //! 二次开发（e2e 控制面板）：控制通道恢复仿真
   void ResumeByControl()
   {
      GetSimulation().Resume();
      mPausedByControl.store(false);
   }

   //! 二次开发（e2e 控制面板）：控制通道视角的暂停状态（status 命令上报）
   bool IsPausedByControl() const
   {
      return mPausedByControl.load();
   }

   // -------------------------------------------------------------------------
   // 二次开发（ext-01 阶段 C）：落盘投递与指标生成（mRecorder 为空时零开销直返）
   // -------------------------------------------------------------------------

   //! 生命周期事件落盘（平台级事件携带元信息供 platforms 快照表建档）
   void RecordEvent(double aSimTime, const char* aEvent, WsfPlatform* aPlatformPtr)
   {
      if (mRecorder == nullptr)
      {
         return;
      }
      Record rec;
      rec.kind  = RecordKind::cEVENT;
      rec.time  = aSimTime;
      rec.text  = aEvent;
      if (aPlatformPtr != nullptr)
      {
         rec.platformIdx = aPlatformPtr->GetIndex();
         rec.name        = aPlatformPtr->GetName();
         rec.type        = aPlatformPtr->GetType();
         rec.side        = aPlatformPtr->GetSide();
      }
      mRecorder->Enqueue(std::move(rec));
   }

   //! 状态样本落盘（与推送流同源同节流，落盘数据与实时流严格一致）
   void RecordState(double       aSimTime,
                    WsfPlatform* aPlatformPtr,
                    double       aLatDeg,
                    double       aLonDeg,
                    double       aAltM,
                    double       aSpeedMps,
                    double       aHeadingDeg,
                    double       aPitchDeg,
                    double       aRollDeg)
   {
      if (mRecorder == nullptr)
      {
         return;
      }
      Record rec;
      rec.kind        = RecordKind::cSTATE;
      rec.time        = aSimTime;
      rec.platformIdx = aPlatformPtr->GetIndex();
      rec.name        = aPlatformPtr->GetName();
      rec.type        = aPlatformPtr->GetType();
      rec.side        = aPlatformPtr->GetSide();
      rec.lat         = aLatDeg;
      rec.lon         = aLonDeg;
      rec.altM        = aAltM;
      rec.speedMps    = aSpeedMps;
      rec.headingDeg  = aHeadingDeg;
      rec.pitchDeg    = aPitchDeg;
      rec.rollDeg     = aRollDeg;
      mRecorder->Enqueue(std::move(rec));
   }

   //! 单条指标落盘（行式 key-value schema，后续交战类指标直接插行扩展）
   void RecordMetric(const std::string& aMetric,
                     unsigned long long aPlatformIdx,
                     const std::string& aPlatformName,
                     const std::string& aSide,
                     double             aValue,
                     const char*        aUnit)
   {
      if (mRecorder == nullptr)
      {
         return;
      }
      Record rec;
      rec.kind        = RecordKind::cMETRIC;
      rec.name        = aMetric;
      rec.platformIdx = aPlatformIdx;
      rec.name2       = aPlatformName;
      rec.side        = aSide;
      rec.value       = aValue;
      rec.text        = aUnit;
      mRecorder->Enqueue(std::move(rec));
   }

   //! 评估指标生成（Complete 收口前调用）：平台级聚合 + 运行级汇总
   void GenerateMetrics(double aSimTime)
   {
      if (mRecorder == nullptr)
      {
         return;
      }
      // 1) 平台级指标
      std::map<std::string, int> sideCounts;
      for (const auto& kv : mStats)
      {
         const PlatformStats& s = kv.second;
         double endTime = (s.removedTime > -1.0e29) ? s.removedTime : aSimTime;
         RecordMetric("survival_time", kv.first, s.name, s.side, endTime - s.addedTime, "s");
         RecordMetric("state_samples", kv.first, s.name, s.side, static_cast<double>(s.samples), "count");
         RecordMetric("avg_speed", kv.first, s.name, s.side, (s.samples > 0) ? (s.speedSum / s.samples) : 0.0, "m/s");
         RecordMetric("max_speed", kv.first, s.name, s.side, s.speedMax, "m/s");
         RecordMetric("distance_m", kv.first, s.name, s.side, s.distanceM, "m");
         ++sideCounts[s.side];
      }
      // 2) 运行级指标（platform_idx=0，name/side 为空）
      RecordMetric("platform_count", 0, "", "", static_cast<double>(mStats.size()), "count");
      for (const auto& kv : sideCounts)
      {
         RecordMetric("side_count_" + kv.first, 0, "", kv.first, static_cast<double>(kv.second), "count");
      }
      RecordMetric("sim_duration", 0, "", "", aSimTime, "s");
      RecordMetric("total_state_samples", 0, "", "", static_cast<double>(mTotalStateSamples), "count");
   }

   //! 全量态势快照：各平台元信息帧 + 最近状态帧（新客户端接入补推用）
   //! 网络线程调用，与仿真线程回调经 mCacheMutex 互斥
   std::vector<std::string> BuildSnapshot()
   {
      std::vector<std::string> out;
      std::lock_guard<std::mutex> lock(mCacheMutex);
      for (const auto& kv : mMetaFrames)
      {
         out.push_back(kv.second);
      }
      for (const auto& kv : mStateFrames)
      {
         out.push_back(kv.second);
      }
      return out;
   }

   InputData mData;                                    // 配置快照（构造时拷贝）
   // 二次开发（ext-01 阶段 D）：通道列表读写锁（仿真线程 Publish 共享读/控制线程增删独占写）
   mutable std::shared_timed_mutex mChannelsMutex;
   std::vector<std::unique_ptr<Channel>> mChannels;    // 已装配通道适配器（TCP/WS/UDP）
   // 二次开发（e2e 控制面板）：控制通道访问运行控制方法（HandleCommand 命令分发）
   friend class ControlChannel;
   std::unique_ptr<ControlChannel>    mControlChannel; // 热切换控制通道（阶段 D，默认空）
   std::atomic<bool> mStartPauseArmed{false};          // start_paused 待触发（Initialize 置位，仿真首帧消费）
   std::atomic<bool> mPausedByControl{false};          // 控制通道暂停状态（status 命令上报）
   // 二次开发（ext-01 阶段 C）：结构化落盘器与指标聚合状态
   std::unique_ptr<Recorder>           mRecorder;      // 落盘器（record=false 时为空）
   std::map<size_t, PlatformStats>     mStats;         // 各平台指标聚合器（仅仿真线程访问，无锁）
   unsigned long long                  mTotalStateSamples = 0; // 运行级状态样本总数
   UtCallbackHolder                   mCallbacks;      // 观察者订阅（析构自动断开）
   std::map<size_t, PlatformThrottle> mThrottle;       // 各平台推送节流状态
   std::mutex                         mCacheMutex;     // 快照缓存互斥锁（仿真线程写/网络线程读）
   std::map<size_t, std::string>      mMetaFrames;     // 各平台元信息帧缓存（快照补推）
   std::map<size_t, std::string>      mStateFrames;    // 各平台最近状态帧缓存（快照补推）
   double                             mLastHeartbeat = -1.0e30; // 上次心跳时间（哨兵=立即首发）
};

// ----------------------------------------------------------------------------
// ControlChannel 实现（阶段 D：需 SimulationExtension 完整类型，故延后至类定义之后）
// ----------------------------------------------------------------------------

//! 控制线程主循环：accept 控制客户端 → 逐行读命令 → 回响应（select 20ms 节拍）
void ControlChannel::Run()
{
   SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (listenSock == INVALID_SOCKET)
   {
      auto out = ut::log::warning() << "ysim_data_export: control channel failed to create socket.";
      mRunning = false;
      return;
   }
   int reuseAddr = 1;
   setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));
   sockaddr_in addr;
   ZeroMemory(&addr, sizeof(addr));
   addr.sin_family      = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port        = htons(static_cast<unsigned short>(mPort));
   if ((bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) ||
       (listen(listenSock, 1) == SOCKET_ERROR))
   {
      auto out = ut::log::warning() << "ysim_data_export: control channel bind/listen failed on port " << mPort << ".";
      out.AddNote() << "Hot-swap control disabled for this run.";
      closesocket(listenSock);
      mRunning = false;
      return;
   }

   // 主循环：单客户端串行服务（控制命令低频，无需多客户端并发）
   std::string lineBuf;
   while (mRunning.load())
   {
      fd_set readSet;
      FD_ZERO(&readSet);
      FD_SET(listenSock, &readSet);
      timeval timeout{0, 20000};
      if (select(0, &readSet, nullptr, nullptr, &timeout) > 0)
      {
         SOCKET client = accept(listenSock, nullptr, nullptr);
         if (client == INVALID_SOCKET)
         {
            continue;
         }
         ApplyStreamSocketOptions(client);
         auto out = ut::log::info() << "ysim_data_export: control client connected.";
         // 客户端会话：逐行读命令并应答，断开或停机时结束会话回到 accept
         lineBuf.clear();
         while (mRunning.load())
         {
            fd_set cmdSet;
            FD_ZERO(&cmdSet);
            FD_SET(client, &cmdSet);
            timeval cmdTimeout{0, 20000};
            if (select(0, &cmdSet, nullptr, nullptr, &cmdTimeout) <= 0)
            {
               continue; // 无数据：回到停机检查
            }
            char chunk[1024];
            int  got = recv(client, chunk, sizeof(chunk), 0);
            if (got <= 0)
            {
               break; // 客户端断开
            }
            lineBuf.append(chunk, static_cast<size_t>(got));
            // 按 '\n' 切行处理（残留半行留在缓冲）
            size_t nl;
            while ((nl = lineBuf.find('\n')) != std::string::npos)
            {
               std::string line = lineBuf.substr(0, nl);
               lineBuf.erase(0, nl + 1);
               // 去除 '\r' 与首尾空白
               if (!line.empty() && line.back() == '\r')
               {
                  line.pop_back();
               }
               if (line.empty())
               {
                  continue;
               }
               std::string response = HandleCommand(line);
               response += '\n';
               if (send(client, response.data(), static_cast<int>(response.size()), 0) == SOCKET_ERROR)
               {
                  break; // 应答失败：结束会话
               }
            }
         }
         closesocket(client);
      }
   }
   closesocket(listenSock);
}

//! 命令分发：add_tcp/add_ws/add_udp/remove/list/help（空格分隔 token）
std::string ControlChannel::HandleCommand(const std::string& aLine)
{
   // token 切分（连续空白视作单分隔符）
   std::vector<std::string> tokens;
   std::string              cur;
   for (char c : aLine)
   {
      if ((c == ' ') || (c == '\t'))
      {
         if (!cur.empty())
         {
            tokens.push_back(cur);
            cur.clear();
         }
      }
      else
      {
         cur += c;
      }
   }
   if (!cur.empty())
   {
      tokens.push_back(cur);
   }
   if (tokens.empty())
   {
      return "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"empty command\"}";
   }

   const std::string& cmd = tokens[0];
   std::string        detail;
   char               buf[512];

   if ((cmd == "add_tcp") || (cmd == "add_ws"))
   {
      int port = 0;
      if ((tokens.size() == 2) && (std::sscanf(tokens[1].c_str(), "%d", &port) == 1) && (port > 0) && (port < 65536))
      {
         bool ok = mOwner->AddChannel((cmd == "add_tcp") ? "tcp" : "ws", "", port, 0, detail);
         std::snprintf(buf, sizeof(buf), "{\"type\":\"control_result\",\"ok\":%s,\"detail\":\"%s\"}",
                       ok ? "true" : "false", JsonEscape(detail).c_str());
         return buf;
      }
      return "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"usage: add_tcp|add_ws <port>\"}";
   }
   if (cmd == "add_udp")
   {
      // add_udp <address> <port> [ttl]
      int port = 0;
      int ttl  = 4;
      if ((tokens.size() >= 3) && (std::sscanf(tokens[2].c_str(), "%d", &port) == 1) && (port > 0) && (port < 65536))
      {
         if (tokens.size() >= 4)
         {
            std::sscanf(tokens[3].c_str(), "%d", &ttl);
         }
         bool ok = mOwner->AddChannel("udp", tokens[1], port, ttl, detail);
         std::snprintf(buf, sizeof(buf), "{\"type\":\"control_result\",\"ok\":%s,\"detail\":\"%s\"}",
                       ok ? "true" : "false", JsonEscape(detail).c_str());
         return buf;
      }
      return "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"usage: add_udp <address> <port> [ttl]\"}";
   }
   if (cmd == "remove")
   {
      int port = 0;
      if ((tokens.size() == 2) && (std::sscanf(tokens[1].c_str(), "%d", &port) == 1))
      {
         bool ok = mOwner->RemoveChannelByPort(port, detail);
         std::snprintf(buf, sizeof(buf), "{\"type\":\"control_result\",\"ok\":%s,\"detail\":\"%s\"}",
                       ok ? "true" : "false", JsonEscape(detail).c_str());
         return buf;
      }
      return "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"usage: remove <port>\"}";
   }
   if (cmd == "list")
   {
      std::string channels;
      for (const auto& kv : mOwner->ListChannels())
      {
         if (!channels.empty())
         {
            channels += ",";
         }
         std::snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"port\":%d}", kv.first.c_str(), kv.second);
         channels += buf;
      }
      std::snprintf(buf, sizeof(buf), "{\"type\":\"channel_list\",\"channels\":[%s]}", channels.c_str());
      return buf;
   }
   // 二次开发（e2e 控制面板）：仿真运行控制命令（pause/resume/set_rate/status）
   if (cmd == "pause")
   {
      mOwner->PauseByControl();
      return "{\"type\":\"control_result\",\"ok\":true,\"detail\":\"paused\"}";
   }
   if (cmd == "resume")
   {
      mOwner->ResumeByControl();
      return "{\"type\":\"control_result\",\"ok\":true,\"detail\":\"resumed\"}";
   }
   if (cmd == "set_rate")
   {
      double rate = 0.0;
      if ((tokens.size() == 2) && (std::sscanf(tokens[1].c_str(), "%lf", &rate) == 1) &&
          (rate >= 0.1) && (rate <= 100.0))
      {
         mOwner->GetSimulation().SetClockRate(rate);
         std::snprintf(buf, sizeof(buf), "{\"type\":\"control_result\",\"ok\":true,\"detail\":\"clock_rate=%g\"}",
                       mOwner->GetSimulation().GetClockRate());
         return buf;
      }
      return "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"usage: set_rate <0.1-100>\"}";
   }
   if (cmd == "status")
   {
      auto& sim = mOwner->GetSimulation();
      std::snprintf(buf, sizeof(buf),
                    "{\"type\":\"sim_status\",\"state\":\"%s\",\"paused\":%s,\"clock_rate\":%g,"
                    "\"sim_time\":%.3f,\"platform_count\":%u}",
                    sim.GetStateString().c_str(),
                    mOwner->IsPausedByControl() ? "true" : "false",
                    sim.GetClockRate(),
                    sim.GetSimTime(),
                    static_cast<unsigned>(sim.GetPlatformCount()));
      return buf;
   }
   if (cmd == "help")
   {
      return "{\"type\":\"control_result\",\"ok\":true,\"detail\":\"commands: add_tcp <port> | add_ws <port> | "
             "add_udp <address> <port> [ttl] | remove <port> | list | pause | resume | "
             "set_rate <0.1-100> | status\"}";
   }
   std::snprintf(buf, sizeof(buf), "{\"type\":\"control_result\",\"ok\":false,\"detail\":\"unknown command: %s\"}",
                 JsonEscape(cmd).c_str());
   return buf;
}
} // namespace dataexport
} // namespace ysim

// ----------------------------------------------------------------------------
// 应用扩展 + 场景扩展：注册链路与想定配置块解析
// ----------------------------------------------------------------------------

namespace
{
//! 场景扩展：解析 ysim_data_export 配置块，并向每个新仿真注册仿真扩展
class ScenarioExtension : public WsfScenarioExtension
{
public:
   explicit ScenarioExtension(const ysim::dataexport::InputData& aData)
      : mData(aData)
   {
   }

   //! 解析场景级命令 ysim_data_export { ... } end_ysim_data_export
   bool ProcessInput(UtInput& aInput) override
   {
      std::string command = aInput.GetCommand();
      if (command == cEXTENSION_NAME)
      {
         // UtInputBlock 空终止符自动推导为 "end_" + 当前命令名
         UtInputBlock block(aInput);
         while (block.ReadCommand())
         {
            std::string subCommand = aInput.GetCommand();
            if ((subCommand == "port") || (subCommand == "tcp_port"))
            {
               // 阶段 B 兼容：port 为 tcp_port 的别名；0 = 禁用 TCP 通道
               int port = 0;
               aInput.ReadValue(port);
               if ((port < 0) || (port > 65535))
               {
                  throw UtInput::BadValue(aInput, "tcp_port must be in the range [0,65535] (0 disables the channel).");
               }
               // 二次开发（ext-01）：命令行 --ysim-port 优先于想定配置
               if (!mData.mTcpPortFromCmdLine)
               {
                  mData.mTcpPort = port;
               }
            }
            else if (subCommand == "websocket_port")
            {
               int port = 0;
               aInput.ReadValue(port);
               if ((port < 0) || (port > 65535))
               {
                  throw UtInput::BadValue(aInput, "websocket_port must be in the range [0,65535] (0 disables the channel).");
               }
               if (!mData.mWsPortFromCmdLine)
               {
                  mData.mWsPort = port;
               }
            }
            else if (subCommand == "udp_port")
            {
               int port = 0;
               aInput.ReadValue(port);
               if ((port < 0) || (port > 65535))
               {
                  throw UtInput::BadValue(aInput, "udp_port must be in the range [0,65535] (0 disables the channel).");
               }
               if (!mData.mUdpPortFromCmdLine)
               {
                  mData.mUdpPort = port;
               }
            }
            else if (subCommand == "udp_address")
            {
               // UDP 目的地址：224-239 段按组播处理，其余按单播
               aInput.ReadValue(mData.mUdpAddress);
            }
            else if (subCommand == "udp_ttl")
            {
               aInput.ReadValue(mData.mUdpTtl);
               if ((mData.mUdpTtl < 1) || (mData.mUdpTtl > 255))
               {
                  throw UtInput::BadValue(aInput, "udp_ttl must be in the range [1,255].");
               }
            }
            else if (subCommand == "heartbeat_interval")
            {
               aInput.ReadValueOfType(mData.mHeartbeatInterval, UtInput::cTIME);
            }
            else if (subCommand == "deadband")
            {
               aInput.ReadValueOfType(mData.mDeadband, UtInput::cLENGTH);
            }
            else if (subCommand == "min_update_interval")
            {
               aInput.ReadValueOfType(mData.mMinUpdateInterval, UtInput::cTIME);
            }
            // 二次开发（ext-01 阶段 C）：结构化落盘配置关键字
            else if (subCommand == "record")
            {
               // 落盘总开关：写文件是有副作用动作，默认关闭显式启用
               bool enabled = false;
               aInput.ReadValue(enabled);
               if (!mData.mRecordFromCmdLine)
               {
                  mData.mRecord = enabled;
               }
            }
            else if (subCommand == "record_directory")
            {
               // 输出根目录（相对 CWD；每轮运行在其中建时间戳子目录）
               // 命令行 --ysim-record <dir> 指定的目录优先于想定配置
               if (!mData.mRecordFromCmdLine)
               {
                  aInput.ReadValue(mData.mRecordDirectory);
               }
               else
               {
                  std::string ignored;
                  aInput.ReadValue(ignored);
               }
            }
            else if (subCommand == "record_csv")
            {
               aInput.ReadValue(mData.mRecordCsv);
            }
            else if (subCommand == "record_sqlite")
            {
               aInput.ReadValue(mData.mRecordSqlite);
            }
            // 二次开发（e2e 控制面板）：起步暂停（仿真首帧即暂停，等待控制通道 resume）
            else if (subCommand == "start_paused")
            {
               aInput.ReadValue(mData.mStartPaused);
            }
            // 二次开发（ext-01 阶段 D）：热切换控制通道端口（0=禁用，默认禁用）
            else if (subCommand == "control_port")
            {
               int port = 0;
               aInput.ReadValue(port);
               if (!mData.mControlPortFromCmdLine)
               {
                  mData.mControlPort = port;
               }
            }
            else
            {
               throw UtInput::BadValue(aInput, "Unknown ysim_data_export subcommand: " + subCommand);
            }
         }
      }
      else
      {
         return false;
      }
      return true;
   }

   //! 每个仿真创建时（含蒙特卡洛多轮运行）注册新的仿真扩展实例
   void SimulationCreated(WsfSimulation& aSimulation) override
   {
      aSimulation.RegisterExtension(GetExtensionName(), ut::make_unique<ysim::dataexport::SimulationExtension>(mData));
   }

private:
   ysim::dataexport::InputData mData; // 配置（命令行阶段先填充，想定块可补全）
};

//! 应用扩展：承载命令行参数并向场景注册场景扩展
class ApplicationExtension : public WsfApplicationExtension
{
public:
   //! 场景创建：注册场景扩展（配置数据由本扩展持有并向下传递）
   void ScenarioCreated(WsfScenario& aScenario) override
   {
      aScenario.RegisterExtension(cEXTENSION_NAME, ut::make_unique<ScenarioExtension>(mData));
   }

   //! 命令行处理：--ysim-port / --ysim-ws-port / --ysim-udp-port <n>
   //! @return 消耗的参数个数（0 表示非本插件参数）
   int ProcessCommandLine(WsfStandardApplication::Options& aOptions, int aArgc, char* aArgv[]) override
   {
      std::string arg((aArgc > 0) ? aArgv[0] : "");
      // 端口值解析（含范围校验），失败抛框架异常
      auto parsePort = [&aArgc, &aArgv, &arg]() -> int
      {
         if (aArgc < 2)
         {
            throw CommandLineError(arg + " requires a port value");
         }
         int port = std::atoi(aArgv[1]);
         if ((port <= 0) || (port > 65535))
         {
            throw CommandLineError(std::string("invalid ") + arg + " value: " + aArgv[1]);
         }
         return port;
      };
      if (arg == "--ysim-port")
      {
         mData.mTcpPort            = parsePort();
         mData.mTcpPortFromCmdLine = true;
         return 2;
      }
      if (arg == "--ysim-ws-port")
      {
         mData.mWsPort            = parsePort();
         mData.mWsPortFromCmdLine = true;
         return 2;
      }
      if (arg == "--ysim-udp-port")
      {
         mData.mUdpPort            = parsePort();
         mData.mUdpPortFromCmdLine = true;
         return 2;
      }
      // 二次开发（ext-01 阶段 C）：启用结构化落盘并指定输出根目录
      if (arg == "--ysim-record")
      {
         if (aArgc < 2)
         {
            throw CommandLineError(arg + " requires a directory path");
         }
         mData.mRecordDirectory   = aArgv[1];
         mData.mRecord            = true;
         mData.mRecordFromCmdLine = true;
         return 2;
      }
      // 二次开发（ext-01 阶段 D）：热切换控制通道端口
      if (arg == "--ysim-control-port")
      {
         mData.mControlPort            = parsePort();
         mData.mControlPortFromCmdLine = true;
         return 2;
      }
      return 0;
   }

private:
   ysim::dataexport::InputData mData; // 命令行阶段填充的配置（先于场景创建）
};
} // namespace

// ----------------------------------------------------------------------------
// 插件注册入口（WsfPluginManager 经导出符号自动调用）
// ----------------------------------------------------------------------------

void Register_ysim_data_export(WsfApplication& aApplication)
{
   if (!aApplication.ExtensionIsRegistered(cEXTENSION_NAME))
   {
      aApplication.RegisterExtension(cEXTENSION_NAME, ut::make_unique<ApplicationExtension>());
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
      Register_ysim_data_export(aApplication);
   }
}
