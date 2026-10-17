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
#include "ModelBrowserPlugin.hpp"

#include <QMenu>
#include <QString>
#include <osg/Node>

#include "ModelBrowserWidget.hpp"
#include "WkfEnvironment.hpp"
#include "WkfMainWindow.hpp"
#include "WkfVtkEnvironment.hpp"

WKF_PLUGIN_DEFINE_SYMBOLS(
   ModelBrowser::Plugin,
   "Model Browser",
   "The Model Browser plugin adds an option to the Tools menu to open a dockable window to view and modify models.",
   "all")

ModelBrowser::Plugin::Plugin(const QString& aPluginName, const size_t aUniqueId)
   : wkf::Plugin(aPluginName, aUniqueId)
   , mDockWidget(nullptr)
{
   wkf::MainWindow* mainWindowPtr = wkfEnv.GetMainWindow();

   QMenu* toolsMenuPtr = mainWindowPtr->GetMenuByName("Tools");
   if (toolsMenuPtr)
   {
      // 二次开发（i18n）：Tools 菜单项 tr() 化（上下文 ModelBrowser::Plugin）
      wkf::Action* dlgActionPtr = new wkf::Action(QIcon::fromTheme("airplane"), tr("Model Viewer..."), mainWindowPtr);
      connect(dlgActionPtr, &QAction::triggered, this, &Plugin::Launch);
      toolsMenuPtr->addAction(dlgActionPtr);
   }
   QMenu* devMenuPtr = mainWindowPtr->GetMenuByName("Developer");
   if (devMenuPtr)
   {
      // 二次开发（i18n）：Developer 菜单项 tr() 化
      wkf::Action* reload = new wkf::Action(QIcon::fromTheme(""), tr("Reload Model Database"), mainWindowPtr);
      connect(reload, &QAction::triggered, this, &Plugin::Reload);
      devMenuPtr->addAction(reload);
   }
}

QList<wkf::Tip> ModelBrowser::Plugin::GetTips() const
{
   QList<wkf::Tip> tips;
   // 二次开发（i18n）：tips 标题/正文/链接文本 tr() 化（文档路径为 URL 键，不译）
   tips.append(wkf::Tip(tr("Model Viewer"),
                        tr("You can view existing platform models and load in additional platform models using the Model "
                           "Browser located in the Tools menu."),
                        "wkf_plugin/" + wkfEnv.GetApplicationPrefix() + "_model_viewer.html",
                        tr("Model Viewer")));
   return tips;
}

void ModelBrowser::Plugin::Launch()
{
   if (!mDockWidget)
   {
      mDockWidget = new wkf::DockWidget("model viewer", wkfEnv.GetMainWindow()->centralWidget(), Qt::WindowFlags());
      mDockWidget->setWidget(new Widget(mDockWidget));
      // 二次开发（i18n）：停靠窗标题 tr() 化
      mDockWidget->setWindowTitle(tr("Model Viewer"));
   }
   mDockWidget->show();
}

void ModelBrowser::Plugin::Reload()
{
   vaEnv.GetModelDatabase()->Reset();
   wkfEnv.LoadModelDatabase();
}
