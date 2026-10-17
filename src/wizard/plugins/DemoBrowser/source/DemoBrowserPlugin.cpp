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

#include "DemoBrowserPlugin.hpp"

#include <QMainWindow>
#include <QMenu>

#include "WkfAction.hpp"
#include "WkfMainWindow.hpp"
#include "WkfStartupDialog.hpp"
#include "WkfStartupWidgetWrapper.hpp"

WKF_PLUGIN_DEFINE_SYMBOLS(DemoBrowser::Plugin, "Demo Browser", "Provides access to demos and test scenarios", "wizard")

DemoBrowser::Plugin::Plugin(const QString& aPluginName, const size_t aUniqueId)
   : wizard::Plugin(aPluginName, aUniqueId)
{
   wkf::MainWindow* mainWindowPtr = wkfEnv.GetMainWindow();

   QMenu*       help   = mainWindowPtr->FindOrCreateMenu("Help");
   wkf::Action* action = new wkf::Action(tr("Demo Browser"), mainWindowPtr); // 二次开发（i18n）：Help 菜单动作文本 tr() 化（类无 Q_OBJECT，落 wkf::Plugin 上下文，与启动按钮一致）
   help->addAction(action);
   connect(action, &QAction::triggered, [this]() { Show(); });
}

QList<wkf::StartupWidgetWrapper*> DemoBrowser::Plugin::GetStartupWidgets() const
{
   QPushButton*               demoBrowserButtonPtr = new QPushButton(tr("Demo Browser")); // 二次开发（i18n）
   wkf::StartupWidgetWrapper* demoBrowserStartupWidget =
      new wkf::StartupWidgetWrapper(wkf::StartupDialogLayouts::cNEW, demoBrowserButtonPtr);
   connect(demoBrowserButtonPtr, &QPushButton::clicked, this, &DemoBrowser::Plugin::Show);
   connect(demoBrowserButtonPtr, &QPushButton::clicked, demoBrowserStartupWidget, &wkf::StartupWidgetWrapper::CloseStartDialog);
   QList<wkf::StartupWidgetWrapper*> startupWidgets;
   startupWidgets.append(demoBrowserStartupWidget);
   return startupWidgets;
}

void DemoBrowser::Plugin::Show()
{
   if (!mDialog)
   {
      mDialog = new DemoBrowserDialog(wkfEnv.GetMainWindow());
   }
   mDialog->show();
   mDialog->raise();
}
