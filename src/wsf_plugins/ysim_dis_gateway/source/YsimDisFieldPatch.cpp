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
// 二次开发（ext-02 阶段 B）：ysim_dis_gateway DIS 协议网关插件——字段补齐模块
// 规格来源：.trae/specs/ext-02-protocol-gateway/spec.md §6.4（批次 B2）
//
// 设计意图：
//   1. Fire/Detonation/Emission PDU 的发包侧字段缺口补齐（缺口底账见
//      spec §6.4.1）：Warhead/Fuse 恒 0（mil 侧 warhead 映射表默认空且
//      Fuse 从不设置）、Emission 的 Emitter Name/Function 恒 0
//      （emitter_type 映射默认空）。
//   2. 补齐路径：官方发包前钩子改字段（非补发 PutPdu）——三个钩子均为
//      WSF_EXPORT 官方导出，PutPdu 发出前调用、可写字段：
//        - WsfDisWeaponEvents::ProcessFirePDU（公开 UtCallbackListN）
//        - WsfDisWeaponEvents::ProcessDetonationPDU（同上）
//        - WsfDisEmission::OverrideTypeAndFunction（静态回调，查表后调用，
//          引用参数改写即落 PDU）
//   3. 回调语义：补缺不覆盖——仅当字段现值为 0（想定未显式配置）时按
//      插件表写入；想定 mil_ext_interface 显式 warhead / dis_interface
//      显式 emitter_type 命令优先。
//   4. 发现链：WsfDisInterface::Find(sim) → WsfMilDisInterface::Find(*disPtr)
//      → GetWeaponEvents()；任一环节缺失（想定未启用 DIS / 宿主未载
//      wsf_mil）则告警闲置，不阻断仿真（降级回归锚点）。
//   5. 线程模型：三个回调全部在仿真线程（观察者链内）触发，无锁；
//      Connect 返回的 unique_ptr 句柄由仿真扩展成员持有，仿真析构时
//      自动断链。
//
// 取值口径（数据表）：
//   - 表1（Warhead/Fuse）：SISO REF-011 Munition Warhead 枚举 1000 =
//     High Explosive（杀爆战斗部通用值统一取 1000）；Fuse 枚举 1 =
//     Proximity（近炸）、2 = Impact（触发）。键为弹药七段码前五段
//     （kind/domain/country/category/subcategory），具体型号细分属
//     后续数据治理批次。
//   - 表2（Emitter Name/Function）：Emitter Name 无国产官方码，取自拟
//     段 36xxx（与 country=36 呼应，外部系统按约定解析）；Emitter
//     Function 按 REF-011 枚举语义就近取值（5=搜索/警戒类、6=干扰类）。
//     键为传感器部件类型名（WsfPlatformPart::GetTypeId——注意是部件
//     类型名而非平台类型名，且无继承链回退，精确匹配）。
//
// 与 YsimDisGateway.cpp（B1 七段码映射注入）同 DLL 驻留：注册入口
// WsfPluginSetup 在该文件中统一导出，本模块经 Register_ysim_dis_field_patch
// 挂载，扩展名 ysim_dis_field_patch 独立于 B1 的 ysim_dis_gateway。
// *****************************************************************************

#include <cstdint>
#include <memory>

#include "UtMemory.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "WsfPlugin.hpp"
#include "WsfScenario.hpp"
#include "WsfScenarioExtension.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationExtension.hpp"
#include "WsfStringId.hpp"
#include "WsfPlatformPart.hpp"
// DIS 接口发现（WsfDisInterface::Find/HasDevice）与 Emission 静态回调钩子
#include "dis/WsfDisInterface.hpp"
#include "dis/WsfDisEmission.hpp"
// wsf_mil 侧武器事件钩子（ProcessFirePDU/ProcessDetonationPDU 发包前回调）
#include "WsfMilDisInterface.hpp"
#include "dis/WsfDisWeaponEvents.hpp"
// DIS PDU 库（大端序列化对象，字段可写）
#include "DisDetonation.hpp"
#include "DisEntityType.hpp"
#include "DisFire.hpp"

namespace
{
//! 本模块扩展名（注册键，独立于 B1 的 ysim_dis_gateway）
const std::string cFIELD_EXTENSION_NAME = "ysim_dis_field_patch";

// ----------------------------------------------------------------------------
// 表1：弹药七段码前五段 → (Warhead, Fuse) 映射（批次 B2 数据资产）
//
// 键序：EntityKind / Domain / Country / Category / Subcategory
// 值口径：Warhead 统一 1000（SISO REF-011 High Explosive 杀爆通用）；
//         Fuse 1=近炸（Proximity）2=触发（Impact）
// 域（Domain）按发射平台域：1=陆基 2=空射 3=舰射 4=水下发射
// ----------------------------------------------------------------------------
struct WarheadFuseEntry
{
   uint8_t  mEntityKind;  // 第 1 段：实体种类（2=弹药）
   uint8_t  mDomain;      // 第 2 段：发射域
   uint16_t mCountry;     // 第 3 段：国家码（36=中国）
   uint8_t  mCategory;    // 第 4 段：大类
   uint8_t  mSubcategory; // 第 5 段：子类
   uint16_t mWarhead;     // REF-011 Warhead 枚举（1000=HE）
   uint16_t mFuse;        // REF-011 Fuse 枚举（1=近炸 2=触发）
   const char* mNote;     // 中文说明
};

const WarheadFuseEntry cWARHEAD_FUSE[] = {
   // B2 验证想定命中键：SA-10C 显式七段码 2:1:36:1:2（陆基地空导弹）
   {2, 1, 36, 1, 2, 1000, 1, "地空导弹：杀爆战斗部 + 近炸引信"},
   {2, 2, 36, 1, 1, 1000, 2, "空空导弹：杀爆战斗部 + 触发引信"},
   {2, 2, 36, 1, 2, 1000, 1, "空地导弹：杀爆战斗部 + 近炸引信"},
   {2, 3, 36, 1, 2, 1000, 1, "舰空导弹：杀爆战斗部 + 近炸引信"},
   {2, 4, 36, 1, 1, 1000, 2, "鱼雷：杀爆战斗部 + 触发引信"},
   {2, 1, 36, 8, 1, 1000, 2, "火箭弹/炮兵弹药：杀爆 + 触发"},
};

// ----------------------------------------------------------------------------
// 表2：传感器部件类型名 → (Emitter Name, Emitter Function) 映射
//
// 键为传感器/通信部件类型名（WsfPlatformPart::GetTypeId 返回值，精确
// 匹配无继承链）；Emitter Name 为自拟段 36xxx（与 country=36 呼应）；
// Emitter Function 按 REF-011 语义就近取值（5=搜索/警戒 6=干扰）
// ----------------------------------------------------------------------------
struct EmitterEntry
{
   const char* mTypeName;  // 部件类型名（映射键）
   uint16_t    mEmitterName;  // DIS Emitter Name 枚举（自拟段 36xxx）
   uint8_t     mFunction;     // DIS Emitter Function 枚举（语义就近）
   const char* mNote;         // 中文说明
};

const EmitterEntry cEMITTERS[] = {
   // B2 验证想定命中键：SA-10C_LAUNCHER 的雷达部件用库类型 WSF_RADAR_SENSOR
   {"WSF_RADAR_SENSOR", 36001, 5, "通用雷达传感器（库类型）：搜索/警戒"},
   {"RADAR", 36001, 5, "自定义雷达类型名（兜底键）：搜索/警戒"},
   {"EW_RADAR", 36002, 6, "电子战雷达/干扰站"},
   {"AEW_C2", 36003, 5, "预警指挥机雷达（部件名场景）"},
   {"SEARCH_RADAR", 36001, 5, "搜索雷达类型名（兜底键）"},
};

//! 按弹药七段码前五段查表（specific/extra 不参与——类目级映射）
const WarheadFuseEntry* FindWarheadFuse(const DisEntityType& aType)
{
   for (const auto& entry : cWARHEAD_FUSE)
   {
      if ((aType.GetEntityKind() == entry.mEntityKind) && (aType.GetDomain() == entry.mDomain) &&
          (aType.GetCountry() == entry.mCountry) && (aType.GetCategory() == entry.mCategory) &&
          (aType.GetSubcategory() == entry.mSubcategory))
      {
         return &entry;
      }
   }
   return nullptr;
}

//! 按部件类型名查 Emitter 表
const EmitterEntry* FindEmitter(WsfStringId aTypeId)
{
   for (const auto& entry : cEMITTERS)
   {
      if (aTypeId == WsfStringId(entry.mTypeName))
      {
         return &entry;
      }
   }
   return nullptr;
}

//! 仿真扩展：DIS 字段补齐主体（三个发包前钩子的挂载与回调）
class DisFieldPatchSimulationExtension : public WsfSimulationExtension
{
public:
   DisFieldPatchSimulationExtension() = default;
   ~DisFieldPatchSimulationExtension() override = default;

   //! 钩子连接：Initialize 阶段发现链就绪（dis_interface 依赖序在前），
   //! Fire/Detonation/Emission 均为运行期事件，Initialize 连接绰绰有余
   bool Initialize() override
   {
      // 环节一：DIS 接口与设备判定（同 B1 范式——无设备 = 未启用 DIS 组网）
      WsfDisInterface* disPtr = WsfDisInterface::Find(GetSimulation());
      if (disPtr == nullptr || !disPtr->HasDevice())
      {
         auto out = ut::log::warning() << "ysim_dis_field_patch: dis_interface has no device configured.";
         out.AddNote() << "PDU field patch hooks not attached (add a 'dis_interface' block with multicast to enable).";
         return true;
      }

      // 环节二：wsf_mil 武器事件组件（Fire/Detonation 钩子宿主）
      WsfMilDisInterface* milPtr = WsfMilDisInterface::Find(*disPtr);
      if (milPtr == nullptr || milPtr->GetWeaponEvents() == nullptr)
      {
         auto out = ut::log::warning() << "ysim_dis_field_patch: WsfMilDisInterface not available.";
         out.AddNote() << "Fire/Detonation hooks not attached (host application must load wsf_mil).";
         // Emission 钩子与 mil 无关，仍然挂载
         AttachEmitterHook();
         return true;
      }

      // Fire/Detonation 钩子：PutPdu 发出前调用，补缺不覆盖
      mFireCallback = milPtr->GetWeaponEvents()->ProcessFirePDU.Connect(&DisFieldPatchSimulationExtension::OnFire, this);
      mDetonationCallback =
         milPtr->GetWeaponEvents()->ProcessDetonationPDU.Connect(&DisFieldPatchSimulationExtension::OnDetonation, this);
      AttachEmitterHook();

      auto out = ut::log::info() << "ysim_dis_field_patch: PDU field patch hooks attached.";
      out.AddNote() << "Warhead/Fuse entries: " << (sizeof(cWARHEAD_FUSE) / sizeof(cWARHEAD_FUSE[0]))
                    << ", emitter entries: " << (sizeof(cEMITTERS) / sizeof(cEMITTERS[0]))
                    << " (scenario-explicit warhead/emitter_type take precedence).";
      return true;
   }

private:
   //! Emission 钩子挂载（独立方法：宿主无 wsf_mil 时 Fire/Detonation 闲置但 Emission 仍可用）
   void AttachEmitterHook()
   {
      mEmitterCallback = WsfDisEmission::OverrideTypeAndFunction.Connect(&DisFieldPatchSimulationExtension::OnEmitter, this);
   }

   //! Fire PDU 发包前回调：Warhead/Fuse 补缺（仅当现值为 0 时查表写入）
   void OnFire(DisFire& aPdu)
   {
      if ((aPdu.GetWarhead() != 0) && (aPdu.GetFuse() != 0))
      {
         return; // 想定 mil_ext_interface 显式配置完整，插件不干预
      }
      const WarheadFuseEntry* hit = FindWarheadFuse(aPdu.GetWeaponType());
      if (hit != nullptr)
      {
         if (aPdu.GetWarhead() == 0)
         {
            aPdu.SetWarhead(hit->mWarhead);
         }
         if (aPdu.GetFuse() == 0)
         {
            aPdu.SetFuse(hit->mFuse);
         }
      }
   }

   //! Detonation PDU 发包前回调：Warhead/Fuse 补缺（与 Fire 同表同语义）
   void OnDetonation(DisDetonation& aPdu)
   {
      if ((aPdu.GetWarhead() != 0) && (aPdu.GetFuse() != 0))
      {
         return;
      }
      const WarheadFuseEntry* hit = FindWarheadFuse(aPdu.GetWeaponType());
      if (hit != nullptr)
      {
         if (aPdu.GetWarhead() == 0)
         {
            aPdu.SetWarhead(hit->mWarhead);
         }
         if (aPdu.GetFuse() == 0)
         {
            aPdu.SetFuse(hit->mFuse);
         }
      }
   }

   //! Emission 系统记录回调（查表后、落 PDU 前调用）：Emitter Name/Function 补缺。
   //! aType/aFunction 为引用参数——改写即经 SetName/SetFunction 落入 Emission PDU
   void OnEmitter(WsfPlatformPart* aPartPtr, DisSystem* /*aSystemPtr*/, DisEnum16& aType, DisEnum8& aFunction)
   {
      if ((aType != 0) && (aFunction != 0))
      {
         return; // 想定 dis_interface 显式 emitter_type 配置完整
      }
      if (aPartPtr == nullptr)
      {
         return; // 防御：调用方恒传有效部件，这里兜底
      }
      const EmitterEntry* hit = FindEmitter(aPartPtr->GetTypeId());
      if (hit != nullptr)
      {
         if (aType == 0)
         {
            aType = hit->mEmitterName;
         }
         if (aFunction == 0)
         {
            aFunction = hit->mFunction;
         }
      }
   }

   //! 回调句柄（unique_ptr 持有——扩展析构自动断链，仿真线程内触发无需锁）
   std::unique_ptr<UtCallbackN<void(DisFire&)>>                 mFireCallback;
   std::unique_ptr<UtCallbackN<void(DisDetonation&)>>           mDetonationCallback;
   std::unique_ptr<UtCallbackN<void(WsfPlatformPart*, DisSystem*, DisEnum16&, DisEnum8&)>> mEmitterCallback;
};

//! 场景扩展：仿真创建时挂仿真扩展
class DisFieldPatchScenarioExtension : public WsfScenarioExtension
{
public:
   void SimulationCreated(WsfSimulation& aSimulation) override
   {
      aSimulation.RegisterExtension(cFIELD_EXTENSION_NAME, ut::make_unique<DisFieldPatchSimulationExtension>());
   }
};

//! 应用扩展：注册场景扩展 + 声明对 dis_interface 的软依赖（保序：本扩展
//! Initialize 晚于 dis_interface，发现链必然就绪；不强制宿主必须注册）
class DisFieldPatchApplicationExtension : public WsfApplicationExtension
{
public:
   void ScenarioCreated(WsfScenario& aScenario) override
   {
      aScenario.RegisterExtension(cFIELD_EXTENSION_NAME, ut::make_unique<DisFieldPatchScenarioExtension>());
   }

   void AddedToApplication(WsfApplication& aApplication) override
   {
      aApplication.ExtensionDepends(cFIELD_EXTENSION_NAME, "dis_interface", false);
   }
};
} // namespace

//! 模块注册入口（由 YsimDisGateway.cpp 的 WsfPluginSetup 统一调用——
//! 同 DLL 双扩展：B1 映射注入 ysim_dis_gateway + B2 字段补齐 ysim_dis_field_patch）
void Register_ysim_dis_field_patch(WsfApplication& aApplication)
{
   if (!aApplication.ExtensionIsRegistered(cFIELD_EXTENSION_NAME))
   {
      aApplication.RegisterExtension(cFIELD_EXTENSION_NAME, ut::make_unique<DisFieldPatchApplicationExtension>());
   }
}
