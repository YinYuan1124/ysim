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

#include "LanguagePrefWidget.hpp"

wizard::LanguagePrefWidget::LanguagePrefWidget(QWidget* parent /*= nullptr*/)
   : PrefWidgetT<LanguagePrefObject>(parent)
{
   mUi.setupUi(this);
   // 二次开发（i18n）：填充语言下拉框。每项的 userData 携带 QLocale 语言名
   // （"en" / "zh_CN"），持久化时只存该标识，显示文本不参与存储。
   // 注意："中文（简体）" 是目标语言自身的名称，按惯例不翻译、恒定显示。
   // 二次开发（i18n）：必须用 QStringLiteral（UTF-16 宽字面量）而非 fromUtf8(窄字面量)：
   // MSVC 默认执行字符集是系统代码页(GBK)，窄字面量会被转成 GBK 字节，
   // fromUtf8 再解析 GBK 字节即产生乱码；QStringLiteral 全程 Unicode 无此问题。
   mUi.mLanguageComboBox->addItem(QObject::tr("English"), "en");
   mUi.mLanguageComboBox->addItem(QStringLiteral("中文（简体）"), "zh_CN");
}

void wizard::LanguagePrefWidget::showEvent(QShowEvent* aEvent)
{
   // 二次开发（i18n）：偏好对象先于本页面构造并已加载设置，
   // 因此每次页面显示时必须从偏好对象同步一次到控件。
   ReadPreferenceData(mPrefObjectPtr->GetPreferences());
}

void wizard::LanguagePrefWidget::ReadPreferenceData(const LanguagePrefData& aPrefData)
{
   // 二次开发（i18n）：按存储的 locale 标识（userData）选中对应条目
   const int index = mUi.mLanguageComboBox->findData(aPrefData.mLanguage);
   if (index >= 0)
   {
      mUi.mLanguageComboBox->setCurrentIndex(index);
   }
}

void wizard::LanguagePrefWidget::WritePreferenceData(LanguagePrefData& aPrefData)
{
   // 二次开发（i18n）：把用户所选条目的 locale 标识写回偏好对象（Settings.ini）
   aPrefData.mLanguage = mUi.mLanguageComboBox->currentData().toString();
}
