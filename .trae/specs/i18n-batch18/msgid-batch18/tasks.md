# 第十八批执行任务清单（wsf 库零散目录）

依赖 spec.md §2/§3 的决策完备明细，按序执行：

## 任务 1：UtMsgCatalog.hpp 新增 17 个 ID 常量
- [x] 0x0002 段末尾（MSG_TBM_MASS_NOT_CONSISTENT 之后）插入 mover 域补遗 4 常量：MSG_NAV_ERROR_INS_REPORT(0x0002007E)、MSG_PF_FIND_CLOSEST_FAIL(0x0002007F)、MSG_PG_NOT_ATTACHED(0x00020080)、MSG_RN_INSERT_AREA_FAIL(0x00020081)
- [x] 0x0003 段末尾（MSG_OTH_MTI_DUP_TABLE 之后）插入 ext 域 1 常量：MSG_EXT_SENSOR_INIT_FAIL(0x00030032)
- [x] 0x0005 段末尾（MSG_DIS_FACTORY_FIRST_PDU 之后）插入 12 常量：MSG_DIS_ACTION_RESP_NOT_JOIN(0x0005002A) … MSG_XIO_CANNOT_TX_MSG(0x00050035)

## 任务 2：UtMsgCatalog.cpp 登记 17 条目录条目（ID 升序）
- [x] 0x0002 段 4 条插入 MSG_TBM_MASS_NOT_CONSISTENT 条目后（cpp:804-808 之后、传感器段注释之前）
- [x] 0x0003 段 1 条插入 MSG_OTH_MTI_DUP_TABLE 条目后（cpp:1101-1105 之后、交战段注释之前）
- [x] 0x0005 段 12 条插入 MSG_DIS_FACTORY_FIRST_PDU 条目后（cpp:1833-1837 之后、工具/UI 段注释之前）

## 任务 3：调用点替换（18 处 / 14 文件）
- [x] 11 个文件补 `#include "UtMsgCatalog.hpp"`（前缀注释 + 字母序，锚点见 spec.md §3.4）
- [x] mover 4 处：WsfNavigationErrors.cpp:207（保留 auto out）、WsfPathFinder.cpp:1016（保留 auto out）、WsfPathGuidance.cpp:111、WsfRouteNetwork.cpp:1077
- [x] ext 1 处：WsfExtEmission.cpp:473（保留 auto out）
- [x] dis 8 处：WsfDisActionResponse.cpp:99（保留 auto out）、WsfDisActionResponseR.cpp:140（保留 auto out）、WsfDisInterface.cpp:410/1539（共用 ID）/2883（保留 auto out）/2900（保留 auto out）、WsfDisPlaybackDevice.cpp:60/69（均保留 auto out）
- [x] event_pipe 1 处：WsfEventPipeInterface.cpp:457（保留 auto out，已有 include）
- [x] xio 1 处：WsfXIO_Subscription.cpp:39
- [x] xio_sim 3 处：WsfXIO_AutoDisMapping.cpp:706（保留 auto out）、WsfXIO_DisExtension.cpp:48、WsfXIO_CommComponent.cpp:107（保留 auto out）
- [x] 每处上方加 `// 二次开发（i18n）：错误消息 ID 化 —— 无运行时参数` 注释；AddNote 附注保持英文原样

## 任务 4：BOM 循环剥离 + 编译验证
- [x] 对全部 16 个修改文件（hpp/cpp/14 业务文件去重）执行循环剥离 BOM 脚本，确保单层 UTF-8 BOM
- [x] `cmake --build "e:\Project\ysim\build" --config Release --target wsf mission` exit 0 无 error

## 任务 5：记账三件套
- [x] doc/2026-10-15修改日志.md 顶部插第十八批章节（Edit 锚点勿吞后续章节标题行）
- [x] .trae/skills/afsim-dev/SKILL.md 累计口径 → 317 条 / 341 处 / 分十八批，追加⑱批明细
- [x] project_memory.md 累计口径同步

## 验收标准
- 编译 exit 0；`FormatEnTagged(ut::msg::MSG_` 在 wsf 库新增 18 处
- 日志落盘仍为英文 + [E:ID] 前缀（双通道不变）
- 累计口径 317 条 / 341 处 / 十八批，三处记账一致
