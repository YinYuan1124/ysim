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

#ifndef LANGUAGEPREFWIDGET_HPP
#define LANGUAGEPREFWIDGET_HPP

#include "LanguagePrefObject.hpp"
#include "WkfPrefWidget.hpp"
#include "ui_LanguagePrefWidget.h"

namespace wizard
{
// 二次开发（i18n）：Preferences 对话框中的"语言"设置页面。
// 继承 wkf::PrefWidgetT 模板基类，绑定 LanguagePrefObject；
// 由 PreferencesDialog 统一创建、显示、读写偏好数据。
class LanguagePrefWidget : public wkf::PrefWidgetT<LanguagePrefObject>
{
public:
   LanguagePrefWidget(QWidget* parent = nullptr);
   ~LanguagePrefWidget() override = default;

   // 归入 Preferences 对话框的 "Application" 分类页
   QString GetCategoryHint() const override { return "Application"; }

private:
   // 页面每次显示时刷新控件状态（偏好可能在页面构造后被加载）
   void showEvent(QShowEvent* aEvent) override;
   // 偏好框架回调：LanguagePrefObject → 界面控件
   void ReadPreferenceData(const LanguagePrefData& aPrefData) override;
   // 偏好框架回调：界面控件 → LanguagePrefObject（用户点 OK 时触发）
   void WritePreferenceData(LanguagePrefData& aPrefData) override;

   // Qt Designer 生成的界面控件（语言下拉框等）
   Ui::LanguagePrefWidget mUi;
};
} // namespace wizard

#endif
