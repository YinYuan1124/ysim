// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2016 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************
#ifndef WKFPLUGINMANAGERDIALOG_HPP
#define WKFPLUGINMANAGERDIALOG_HPP

class QListWidgetItem;
#include <map>
#include <memory>

#include "WkfDialog.hpp"
#include "ui_WkfPluginManagerDialog.h"

namespace wkf
{
class PluginManager;

class PluginManagerDialog : public Dialog
{
   // 二次开发（i18n）：补 Q_OBJECT 使 tr() 翻译上下文命中本类（菜单触发动作文本汉化）
   Q_OBJECT
public:
   PluginManagerDialog(MainWindow* parent, Qt::WindowFlags f = Qt::WindowFlags());

private:
   void Initialize() override;

   void PluginListCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
   void PluginListOpenFolder();
   void AutoStartClicked(bool checked);
   void AutoStartAllClicked(bool checked);

   void closeEvent(QCloseEvent* event) override;
   void accept() override;
   void reject() override;

   Ui::WkfPluginManagerDialogUi mUi;
   PluginManager* const         mPluginManagerPtr;
   std::map<QString, bool>      mAutoStartChanges; // Plugin name -> auto start
};
} // namespace wkf
#endif
