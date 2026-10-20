// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2020 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "WkfTipContainer.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include "WkfEnvironment.hpp"
#include "WkfPlugin.hpp"
#include "WkfPluginManager.hpp"
#include "WkfTip.hpp"

wkf::TipContainer::TipContainer(QObject* aParentPtr)
   : QObject(aParentPtr)
{
   connect(&wkfEnv,
           &Environment::Initialize,
           this,
           [this]()
           {
              ReadTipsFromFile();
              QueryPluginsForTips();
           });
}

void wkf::TipContainer::ReadTipsFromFile()
{
   // Grab tips from qrc
   // Note: to add a new tip, go to tips.txt in the ui directory and write a new tip
   QFile file(":/tips.txt");
   if (file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      QTextStream in(&file);
      QString     allText = in.readAll();
      // tips are delimited with "==="
      mTips = allText.split("===");

      // Remove whitespace at the beginning and end of the tip
      for (auto& tip : mTips)
      {
         tip = tip.trimmed();
         // 二次开发（i18n）：qrc tips.txt 条目接入翻译通道（整条富文本为翻译单元）。
         // 译文中 <b>/<a doclink="..."> 标签与 doclink 键须原样保留，
         // doclink→href 的目录替换（UpdateDocLinks）在翻译之后进行，不受影响。
         tip = QCoreApplication::translate("wkf::TipContainer", tip.toUtf8().constData());
      }

      UpdateDocLinks();
   }
}

void wkf::TipContainer::QueryPluginsForTips()
{
   QFileInfo fi(wkfEnv.GetDocumentationDir());
   if (fi.isDir())
   {
      QString dir(fi.absoluteFilePath().append("/html/docs/"));
      // Get the list of all the shortcuts from plugins
      auto plugins = wkfEnv.GetPluginManager()->GetLoadedPlugins();
      for (const auto& p : plugins)
      {
         auto tips = p.second->GetTips();
         for (auto& tip : tips)
         {
            // 二次开发（i18n）：尾句模板 tr() 化（链接与文档名动态填入）；标题/正文/链接文本
            // 由各插件 GetTips 内 tr() 化后传入。
            QString tail = tr("For more information visit the \n<a href=\"%1\">%2</a> documentation.")
                              .arg(dir + tip.docLocation, tip.docDescription);
            QString text = "<b>" + tip.title + "</b> \n\n" + tip.text + "\n\n" + tail;
            mTips.append(text);
         }
      }
   }
}

void wkf::TipContainer::UpdateDocLinks()
{
   // Update a href links with the documentation directory
   QFileInfo fi(wkfEnv.GetDocumentationDir());
   if (fi.isDir())
   {
      QString dir(fi.absoluteFilePath().append("/html/docs/"));
      for (auto& tip : mTips)
      {
         tip.replace("<a doclink=\"", "<a href=\"" + dir);
      }
   }
}
