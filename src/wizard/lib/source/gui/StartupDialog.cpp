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
#include "StartupDialog.hpp"

#include "ActionManager.hpp"
#include "ProjectWorkspace.hpp"
#include "RunManager.hpp"

wizard::StartupDialog::StartupDialog(QStringList&    aInputFiles,
                                     QWidget*        aParent,
                                     Qt::WindowFlags aFlags /*= Qt::WindowFlags()*/)
   : wkf::StartupDialog<QStringList>(aInputFiles, aParent, aFlags)
{
   // 二次开发（i18n）：文档链接文本与 New 按钮文本 tr() 化（链接目标文件名为键，不译）
   mDocLinks    = {{"wizard.html", tr("Main Page")},
                {"wizard_users_guide.html", tr("User's Guide")},
                {"wizard_reference_guide.html", tr("Reference Guide")},
                {"wizard_change_log.html", tr("Change Log")},
                {"main_page.html", tr("AFSIM Main Page")}};
   auto* newPtr = mUi.buttonBox->addButton(tr("New..."), QDialogButtonBox::DestructiveRole);
   // According to Qt docs, clicking a button defined with a "destructive" role should close the dialog, but it does
   // not. This is reported in https://bugreports.qt.io/browse/QTBUG-67168. Hence the need to call QDialog::done.
   connect(newPtr,
           &QPushButton::clicked,
           this,
           [this]()
           {
              done(0);
              ProjectWorkspace::Instance()->GetActionManager()->NewProject();
           });
}

QStringList wizard::StartupDialog::Browse()
{
   // If we have a previous scenario, open the Browse Scenario dialog to the same directory that scenario is in.
   QString directory = wizRunMgr.GetMostRecentDirectory();

   // This does not use wkf::getOpenFileNames due to wkf::Environment not being set up yet,
   // instead the Browse button is disabled directly to prevent file browsing
   // 二次开发（i18n）：文件对话框标题与类型过滤器 tr() 化（扩展名为协议键，原样保留）
   return QFileDialog::getOpenFileNames(this,
                                        tr("Select Scenario File(s)"),
                                        directory,
                                        tr("AFSIM Input(*.afproj *.txt);; AFSIM Project(*.afproj);; All Files (*.*)"));
}

void wizard::StartupDialog::RemoveRecent(const QStringList& aRecent)
{
   wizRunMgr.RemoveRecentScenario(aRecent);
}

void wizard::StartupDialog::ClearRecents()
{
   wizRunMgr.ClearRecentScenarios();
}

const QList<QStringList>& wizard::StartupDialog::GetRecents() const
{
   return wizRunMgr.GetRecentScenarios();
}

QString wizard::StartupDialog::GetRecentLabelDisplayName(const QStringList& aFileList, bool aDisplayFullPath) const
{
   return wizRunMgr.GetScenarioDisplayName(aFileList, aDisplayFullPath);
}

void wizard::StartupDialog::BuildRecentContextMenu(QMenu& aMenu, const QStringList& aLink)
{
   // 二次开发（i18n）：最近列表右键菜单动作文本 tr() 化（上下文 wizard::StartupDialog）
   connect(aMenu.addAction(tr("Open File")),
           &QAction::triggered,
           [aLink]()
           {
              for (const auto& file : aLink)
              {
                 QDesktopServices::openUrl(QUrl::fromLocalFile(file));
              }
           });
}

void wizard::StartupDialog::PopulateLabels()
{
   // 二次开发（i18n）：启动对话框文本统一包裹 tr()，由 QTranslator 按语言翻译
   mUi.recentsGroupBox->setTitle(tr("Recent Projects"));
   mUi.newGroupBox->setTitle(tr("Open Project"));
   mUi.filesLineEdit->setPlaceholderText(tr("Drag and drop or enter scenario file(s)..."));
   mUi.appDesc->setText(tr("<p style=\"font - size:10pt; \">Wizard is the AFSIM scenario editor.  It can be used to view "
                           "scenarios, launch a virtual or constructive simulation, and launch post-process viewers.</p>"));
   mUi.appIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/wizard_logo_icon.png")));
   mUi.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Open"));
}

bool wizard::StartupDialog::ValidateSelection()
{
   // Check to see if all files were found
   for (const auto& file : mSelection)
   {
      QFileInfo info(file);
      if (!info.exists())
      {
         // 二次开发（i18n）：警告框标题与正文 tr() 化（拼接串改 %1 参数化）
         QMessageBox::warning(this, tr("File Not Found"), tr("File: %1 was not found.").arg(file));
         return false;
      }
   }
   return true;
}

QStringList wizard::StartupDialog::ParseDroppedUrls(const QList<QUrl>& urls) const
{
   QStringList filePaths;
   foreach (QUrl url, urls)
   {
      if (url.isLocalFile())
      {
         filePaths.push_back(url.toLocalFile());
      }
   }

   return filePaths;
}
