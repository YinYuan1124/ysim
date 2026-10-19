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
// 二次开发（ext-02 阶段 B）：ysim_dis_gateway DIS 协议网关插件
// 规格来源：.trae/specs/ext-02-protocol-gateway/spec.md §6.3（批次 B1）
//
// 设计意图：
//   1. Entity Type 七段码映射表（国产装备枚举）集中注入：把 AFSIM 平台类型名
//      → DIS Entity Type 七段码的映射表经官方导出接口
//      WsfExtInterface::GetObjectToEntityTypeMap()（返回可写 map 引用）批量
//      注入，零侵入 WSF 核心源码。映射查询链（WsfExtData::GetEntityType）沿
//      平台类型继承链回退——注册到基类（如 AIR_MOVER_FIGHTER）即可被全部
//      派生类型继承。
//   2. 注入策略：仅插入不存在键（想定 dis_interface 块内显式 entity_type
//      命令优先，插件补缺不覆盖）。
//   3. 注入时序：AddedToSimulation 阶段注入（扩展生命周期中最早的数据可用
//      阶段，必然早于平台 PlatformAdded → WsfDisPlatform 构造查表）；
//      Initialize 阶段幂等补注双保险 + DIS 设备判定（未配置设备的想定下
//      告警闲置，不阻断仿真——降级回归锚点）。
//   4. 七段码取值口径：kind/domain/category 三段按 SISO REF-010 语义落位，
//      country 统一 36（中国），subcategory 及以后为语义合理的自拟段
//      （表内逐条中文注释标注），供外部 DIS 态势显示与识别使用。
//
// 挂载方式：DLL 经 swdev_plugin_install 部署至 mission_plugins/，
//           mission.exe 启动时由 WsfPluginManager 自动扫描加载。
//
// 想定配合（本插件无自有配置块；DIS 组网由原生 dis_interface 块控制）：
//   dis_interface
//      exercise      1
//      site          25
//      application   1
//      multicast     224.2.25.54 127.0.0.1
//      port          3225
//      autostart
//      # 可选：显式映射优先于插件表（不被覆盖）
//      entity_type   BLUE_FIGHTER_AIR 1:2:225:2:1:1:0
//   end_dis_interface
// *****************************************************************************

#include <cstdint>
#include <string>

#include "UtMemory.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "WsfPlugin.hpp"
#include "WsfScenario.hpp"
#include "WsfScenarioExtension.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationExtension.hpp"
#include "WsfStringId.hpp"
// DIS 接口发现（WsfDisInterface::Find / HasDevice）与映射表存储
// （WsfExtInterface::Find / GetObjectToEntityTypeMap）——均为 WSF_EXPORT
// 官方导出扩展点，插件侧仅消费不改动
#include "dis/WsfDisInterface.hpp"
#include "ext/WsfExtEntityType.hpp"
#include "ext/WsfExtInterface.hpp"

namespace
{
//! 插件扩展名（注册键）
const std::string cEXTENSION_NAME = "ysim_dis_gateway";

// ----------------------------------------------------------------------------
// 国产装备 Entity Type 七段码映射表（批次 B1 数据资产）
//
// 字段序：EntityKind / Domain / Country / Category / Subcategory / Specific / Extra
// 取值口径：
//   - EntityKind：1=平台 2=弹药 3=生命体（SISO REF-010）
//   - Domain：1=陆 2=空 3=水面 4=水下（SISO REF-010）
//   - Country：36=中国（SISO 国家码，全表统一）
//   - Category：按 REF-010 大类语义落位；Subcategory 及以后为自拟段
//     （语义合理即可，外部系统按约定解析）
// 键为 AFSIM 平台类型名：查询沿类型继承链回退，基类映射可被派生类型继承
// ----------------------------------------------------------------------------
struct TypeMappingEntry
{
   const char* mTypeName;    // AFSIM 平台类型名（映射键）
   uint8_t     mEntityKind;  // 第 1 段：实体种类
   uint8_t     mDomain;      // 第 2 段：领域
   uint16_t    mCountry;     // 第 3 段：国家码（36=中国）
   uint8_t     mCategory;    // 第 4 段：大类
   uint8_t     mSubcategory; // 第 5 段：子类（自拟段）
   uint8_t     mSpecific;    // 第 6 段：具体型（自拟段）
   uint8_t     mExtra;       // 第 7 段：扩展（自拟段）
   const char* mNote;        // 中文说明
};

const TypeMappingEntry cTYPE_MAPPINGS[] = {
   // —— 空中装备（kind=1 平台，domain=2 空域）——
   {"AIR_MOVER_FIGHTER", 1, 2, 36, 2, 1, 0, 0, "通用歼击机基类（air_to_air 想定继承链兜底键）"},
   {"BLUE_FIGHTER_AIR", 1, 2, 36, 2, 1, 1, 0, "蓝方歼击机（想定显式 entity_type 优先级验证键）"},
   {"RED_FIGHTER_AIR", 1, 2, 36, 2, 1, 2, 0, "红方歼击机"},
   {"FIGHTER", 1, 2, 36, 2, 1, 0, 0, "歼击机（通用类型名）"},
   {"ATTACKER", 1, 2, 36, 3, 1, 0, 0, "攻击机（对地打击类）"},
   {"BOMBER", 1, 2, 36, 4, 1, 0, 0, "轰炸机"},
   {"AEW_C2", 1, 2, 36, 6, 1, 0, 0, "预警指挥机（特种功能类）"},
   {"TRANSPORT", 1, 2, 36, 1, 5, 0, 0, "军用运输机"},
   {"UAV", 1, 2, 36, 9, 1, 0, 0, "无人机（侦察/察打类）"},
   {"HELICOPTER", 1, 2, 36, 20, 1, 0, 0, "直升机"},
   // —— 弹药（kind=2 弹药）——
   {"MISSILE", 2, 2, 36, 1, 1, 0, 0, "空射导弹"},
   {"SAM_MISSILE", 2, 3, 36, 1, 2, 0, 0, "地空/舰空导弹"},
   // —— 海上装备（kind=1 平台）——
   {"SURFACE_SHIP", 1, 3, 36, 1, 1, 0, 0, "水面舰艇"},
   {"CARRIER", 1, 3, 36, 8, 1, 0, 0, "航空母舰"},
   {"SUBMARINE", 1, 4, 36, 6, 1, 0, 0, "潜艇"},
   // —— 陆上装备（kind=1 平台，domain=1 陆域）——
   {"SAM_LAUNCHER", 1, 1, 36, 8, 3, 0, 0, "地空导弹发射单元"},
   {"RADAR", 1, 1, 36, 12, 1, 0, 0, "地面雷达站"},
   {"EW_RADAR", 1, 1, 36, 12, 2, 0, 0, "电子战/干扰站"},
   {"ARMOR", 1, 1, 36, 1, 1, 0, 0, "主战装甲车辆"},
   // —— 生命体（kind=3）——
   {"INFANTRY", 3, 1, 36, 1, 1, 0, 0, "步兵分队"},
};

//! 幂等注入映射表：仅插入不存在键（想定显式 entity_type 命令优先，插件补缺不覆盖）
//! @return 本次实际插入的条数（重复调用时已存在键计 0）
size_t InjectEntityTypeMappings(WsfSimulation& aSimulation)
{
   WsfExtInterface* extPtr = WsfExtInterface::Find(aSimulation);
   if (extPtr == nullptr)
   {
      // 理论不可达（mission 的 xio_interface 注册链无条件创建 ext_interface 仿真扩展），
      // 防御性返回
      return 0;
   }
   auto&  typeMap  = extPtr->GetObjectToEntityTypeMap();
   size_t injected = 0;
   for (const auto& entry : cTYPE_MAPPINGS)
   {
      WsfStringId typeId = entry.mTypeName;
      if (typeMap.find(typeId) == typeMap.end())
      {
         typeMap.emplace(typeId,
                         WsfExtEntityType(entry.mEntityKind,
                                          entry.mDomain,
                                          entry.mCountry,
                                          entry.mCategory,
                                          entry.mSubcategory,
                                          entry.mSpecific,
                                          entry.mExtra));
         ++injected;
      }
   }
   return injected;
}

//! 仿真扩展：DIS 网关主体（映射表集中注入）
class DisGatewaySimulationExtension : public WsfSimulationExtension
{
public:
   DisGatewaySimulationExtension() = default;
   ~DisGatewaySimulationExtension() override = default;

   //! 最早数据阶段注入：必然早于平台 PlatformAdded（WsfDisPlatform 构造时查表
   //! 定 EntityState 七段码），保证 t=0 初始平台即用上插件映射
   void AddedToSimulation() override
   {
      mInjectedCount = InjectEntityTypeMappings(GetSimulation());
   }

   //! 设备判定（降级回归锚点）+ 幂等补注双保险
   bool Initialize() override
   {
      // 幂等补注：覆盖"AddedToSimulation 时扩展对象尚未就绪"的极端时序
      size_t extra = InjectEntityTypeMappings(GetSimulation());

      // DIS 设备判定：无输入/输出设备 = 想定未启用 DIS 组网，映射表无消费方
      // ——告警闲置不阻断（同 ysim_xio_gateway 范式）
      WsfDisInterface* disPtr = WsfDisInterface::Find(GetSimulation());
      if (disPtr == nullptr || !disPtr->HasDevice())
      {
         auto out = ut::log::warning() << "ysim_dis_gateway: dis_interface has no device configured.";
         out.AddNote() << "Entity type mappings injected (" << (mInjectedCount + extra)
                       << " entries) but unused (add a 'dis_interface' block with multicast to enable).";
         return true;
      }
      auto out = ut::log::info() << "ysim_dis_gateway: injected entity type mappings.";
      out.AddNote() << "Entries: " << (mInjectedCount + extra) << " (country=36 CN, scenario-explicit "
                    << "entity_type commands take precedence).";
      return true;
   }

private:
   size_t mInjectedCount = 0; // AddedToSimulation 阶段注入条数（日志口径）
};

//! 场景扩展：仿真创建时挂仿真扩展
class DisGatewayScenarioExtension : public WsfScenarioExtension
{
public:
   void SimulationCreated(WsfSimulation& aSimulation) override
   {
      aSimulation.RegisterExtension(cEXTENSION_NAME, ut::make_unique<DisGatewaySimulationExtension>());
   }
};

//! 应用扩展：注册场景扩展 + 声明对 dis_interface 的软依赖（保序不强制）
class DisGatewayApplicationExtension : public WsfApplicationExtension
{
public:
   void ScenarioCreated(WsfScenario& aScenario) override
   {
      aScenario.RegisterExtension(cEXTENSION_NAME, ut::make_unique<DisGatewayScenarioExtension>());
   }

   void AddedToApplication(WsfApplication& aApplication) override
   {
      // 软依赖：仅保证本插件扩展的初始化顺序晚于 dis_interface（注入的映射表
      // 消费方），不强制宿主必须注册 dis_interface（mission 的 xio_interface
      // 注册链已无条件注册，其他宿主缺省时本插件注入无害闲置）
      aApplication.ExtensionDepends(cEXTENSION_NAME, "dis_interface", false);
   }
};
} // namespace

// ----------------------------------------------------------------------------
// 插件注册入口（WsfPluginManager 经导出符号自动调用）
// ----------------------------------------------------------------------------

// B2 字段补齐模块注册入口（YsimDisFieldPatch.cpp——同 DLL 双扩展驻留）
void Register_ysim_dis_field_patch(WsfApplication& aApplication);

void Register_ysim_dis_gateway(WsfApplication& aApplication)
{
   if (!aApplication.ExtensionIsRegistered(cEXTENSION_NAME))
   {
      aApplication.RegisterExtension(cEXTENSION_NAME, ut::make_unique<DisGatewayApplicationExtension>());
   }
   // 批次 B2：Fire/Detonation/Emission PDU 字段补齐（钩子挂载，独立扩展名）
   Register_ysim_dis_field_patch(aApplication);
}

extern "C"
{
   UT_PLUGIN_EXPORT void WsfPluginVersion(UtPluginVersion& aVersion)
   {
      aVersion = UtPluginVersion(WSF_PLUGIN_API_MAJOR_VERSION, WSF_PLUGIN_API_MINOR_VERSION, WSF_PLUGIN_API_COMPILER_STRING);
   }

   UT_PLUGIN_EXPORT void WsfPluginSetup(WsfApplication& aApplication)
   {
      Register_ysim_dis_gateway(aApplication);
   }
}
