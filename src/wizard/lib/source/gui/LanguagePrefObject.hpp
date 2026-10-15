// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2018 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef LANGUAGEPREFOBJECT_HPP
#define LANGUAGEPREFOBJECT_HPP

#include <QSettings>
#include <QString>

#include "WkfPrefObject.hpp"

namespace wizard
{
// 二次开发（i18n）：界面语言偏好数据结构。
// mLanguage 存储 QLocale 语言名（"en" = 英文，"zh_CN" = 简体中文）。
struct LanguagePrefData
{
   QString mLanguage{"en"};
};

// 二次开发（i18n）：界面语言偏好对象。
// 继承 wkf::PrefObjectT 模板基类，由 Preferences 对话框框架自动持久化到
// %APPDATA%/.../Settings.ini 的 [LanguageOptions] 分组下。
class LanguagePrefObject : public wkf::PrefObjectT<LanguagePrefData>
{
   Q_OBJECT

public:
   // 偏好分组名，对应 Settings.ini 中的 [LanguageOptions] 节
   static constexpr const char* cNAME = "LanguageOptions";

   LanguagePrefObject(QObject* parent = nullptr);
   ~LanguagePrefObject() override = default;

   // 语言切换无需即时应用（重启生效），故 Apply 为空实现
   void Apply() override;

   // 以下两个函数由 PrefObject 基类在加载/保存偏好时回调
   LanguagePrefData ReadSettings(QSettings& aSettings) const override;
   void           SaveSettingsP(QSettings& aSettings) const override;

   // 静态工具：直接从 Settings.ini 读取当前语言设置。
   // 用途：在 Runner::run() 启动早期（PrefObject 尚未构造、任何 UI 未创建时）
   // 需要先安装 QTranslator，因此必须绕过偏好框架直接读文件。
   static QString GetCurrentLanguage();
};
} // namespace wizard

Q_DECLARE_METATYPE(wizard::LanguagePrefData)
#endif
