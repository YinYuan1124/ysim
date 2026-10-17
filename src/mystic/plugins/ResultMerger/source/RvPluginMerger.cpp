// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2019 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "RvPluginMerger.hpp"

#include <QCoreApplication> // 二次开发（i18n）：显式上下文翻译所需

#include "WkfEnvironment.hpp"
#include "WkfMainWindow.hpp"

WKF_PLUGIN_DEFINE_SYMBOLS(
   RvMerger::Plugin,
   "Recording Merger",
   "Provides an interface for combining recordings from a distributed simulation into a single .aer file.",
   "mystic")

RvMerger::Plugin::Plugin(const QString& aPluginName, const size_t aUniqueId)
   : rv::Plugin(aPluginName, aUniqueId)
   , mDialog(nullptr)
   , mMerger(nullptr)
{
   auto* mainWindow = wkfEnv.GetMainWindow();

   if (mainWindow != nullptr)
   {
      mainWindow->AddDialogToToolMenu(&mDialog, QCoreApplication::translate("RvMerger::Plugin", "Merge Recordings...")); // 二次开发（i18n）：工具菜单项接入翻译通道
   }

   connect(&mDialog, &MergeDialog::Merge, this, &Plugin::BeginMerge);
   connect(&mDialog, &MergeDialog::MergeCanceled, &mMerger, &QThread::requestInterruption);

   connect(
      &mMerger,
      &MergerThread::RunError,
      this,
      [this](const std::string& aErrorMessage)
      {
         QMessageBox::critical(&mDialog, QCoreApplication::translate("RvMerger::Plugin", "Error Merging Recordings"), QString::fromStdString(aErrorMessage)); // 二次开发（i18n）：标题接入翻译通道（错误消息来自底层保持原文）
         mDialog.Abort();
      },
      Qt::BlockingQueuedConnection);

   connect(&mMerger, &QThread::finished, &mDialog, &MergeDialog::MergeFinished);
}

void RvMerger::Plugin::BeginMerge(const QStringList& aInputs, const QString& aOutput) noexcept
{
   mMerger.Reset();

   for (const QString& input : aInputs)
   {
      mMerger.AddInputFile(input.toStdString());
   }

   mMerger.SetOutputFile(aOutput.toStdString());

   mMerger.start();
}

void RvMerger::Plugin::CancelMerge() noexcept
{
   mMerger.requestInterruption();
}
