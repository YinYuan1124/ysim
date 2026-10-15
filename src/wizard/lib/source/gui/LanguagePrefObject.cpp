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

#include "LanguagePrefObject.hpp"

#include <QStandardPaths>

wizard::LanguagePrefObject::LanguagePrefObject(QObject* parent /*= nullptr*/)
   : wkf::PrefObjectT<LanguagePrefData>(parent, cNAME)
{
}

void wizard::LanguagePrefObject::Apply() {}

wizard::LanguagePrefData wizard::LanguagePrefObject::ReadSettings(QSettings& aSettings) const
{
   // 二次开发（i18n）：从 Settings.ini 的 [LanguageOptions] 分组读取 language 键；
   // 缺省回退 mDefaultPrefs（即 "en" 英文）。
   LanguagePrefData pData;
   pData.mLanguage = aSettings.value("language", mDefaultPrefs.mLanguage).toString();
   return pData;
}

void wizard::LanguagePrefObject::SaveSettingsP(QSettings& aSettings) const
{
   // 二次开发（i18n）：把用户当前选择的语言写入 Settings.ini。
   aSettings.setValue("language", mCurrentPrefs.mLanguage);
}

QString wizard::LanguagePrefObject::GetCurrentLanguage()
{
   // 二次开发（i18n）：直接从设置文件读取语言，不经过偏好框架。
   // 该函数在启动早期被调用（wkf::Environment 与各 PrefObject 构造之前），
   // 以便在任何 UI 创建前安装 QTranslator，保证界面语言自始生效。
   const QString settingsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Settings.ini";
   QSettings    settings(settingsFile, QSettings::IniFormat);
   settings.beginGroup(cNAME);
   const QString lang = settings.value("language", "en").toString();
   settings.endGroup();
   return lang;
}
