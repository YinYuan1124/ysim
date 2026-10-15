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

#include "Runner.hpp"

// 二次开发（i18n）：语言诊断输出依赖 ut::log（Console/日志可见）
#include "UtLog.hpp"

#include <QMessageBox>
#include <QResource>
#include <QStandardPaths>
#include <QTranslator>

#include "Environment.hpp"
#include "Factory.hpp"
#include "LanguagePrefObject.hpp"
#include "RunEnvManager.hpp"
#include "RunManager.hpp"
#include "Signals.hpp"
#include "StartupDialog.hpp"
#include "Undo.hpp"
#include "WkfEnvironment.hpp"
#include "WkfMainWindow.hpp"
#include "WkfVtkEnvironment.hpp"

wizard::Runner::Runner(int&        argc,
                       char*       argv[],
                       const char* aCompanyStr,
                       const char* aCompanyDomainStr,
                       const char* aProductNameStr,
                       const char* aProductVersionStr)
{
   // Get the executable path
   // Trim the name of the executable
   mBinDir = UtPath::GetExePath();
   mBinDir.Up();

   QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
   QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
   mApp = ut::make_unique<QApplication>(argc, argv);
   mApp->setQuitOnLastWindowClosed(true);
   mApp->setWindowIcon(QIcon(":icons/wizard_logo_icon.png"));
   qApp->setOrganizationName(aCompanyStr);
   qApp->setOrganizationDomain(aCompanyDomainStr);
   qApp->setApplicationName(aProductNameStr);
   qApp->setApplicationDisplayName(aProductNameStr);
   qApp->setApplicationVersion(aProductVersionStr);
}

void wizard::Runner::run()
{
   // 二次开发（i18n）：按用户语言偏好安装翻译器，必须发生在任何 UI 创建之前。
   // 语言设置直接从 Settings.ini 读取（见 LanguagePrefObject::GetCurrentLanguage），
   // 因为此刻偏好框架尚未初始化。
   const QString curLang = LanguagePrefObject::GetCurrentLanguage();
   if (curLang == "zh_CN")
   {
      // 翻译文件（.qm）位于 exe 同目录的 i18n/ 子目录下
      const QString i18nDir = mApp->applicationDirPath() + "/i18n";
      // 程序自身的翻译（wizard 界面文本）
      auto* appTranslator = new QTranslator(mApp.get());
      const bool appLoaded = appTranslator->load("wizard_zh_CN", i18nDir);
      if (appLoaded)
      {
         mApp->installTranslator(appTranslator);
      }
      // Qt 内置对话框的翻译（OK/取消按钮、颜色选择器等）
      auto* qtTranslator = new QTranslator(mApp.get());
      const bool qtLoaded = qtTranslator->load("qtbase_zh_CN", i18nDir);
      if (qtLoaded)
      {
         mApp->installTranslator(qtTranslator);
      }
      // 二次开发（i18n）调试：输出语言读取与翻译文件加载状态，便于排错（Console/日志可见）
      auto out = ut::log::info() << "i18n diagnostic: language=" << curLang.toStdString()
                                    << ", i18nDir=" << i18nDir.toStdString()
                                    << ", wizard_zh_CN.qm loaded=" << (appLoaded ? "true" : "false")
                                    << ", qtbase_zh_CN.qm loaded=" << (qtLoaded ? "true" : "false");
   }
   else
   {
      // 二次开发（i18n）调试：输出当前语言值（非中文分支）
      auto out = ut::log::info() << "i18n diagnostic: language=" << curLang.toStdString() << " (en branch, no translator installed)";
   }

   std::string errorString;
   // Given the path to the executable determine the other directory paths
   if (!RunEnvManager::Initialize(mBinDir, errorString))
   {
      QMessageBox::critical(nullptr, QObject::tr("AFSIM Wizard"), QObject::tr(errorString.c_str()), QMessageBox::Ok);
      return;
   }
   // Expected locations for rcc files ...
   // - Install: bin/../resources/rcc directory
   // - Debug:   Debug/../rcc/ directory
   mRccPath = (RunEnvManager::GetDataDir() + "../rcc/wizard.rcc").GetSystemPath();
   if (!QResource::registerResource(QString::fromStdString(mRccPath)))
   {
      // paths for developer
      mRccPath = (RunEnvManager::GetBinDir() + "../rcc/wizard.rcc").GetSystemPath();
      if (!QResource::registerResource(QString::fromStdString(mRccPath)))
      {
         mRccPath = (RunEnvManager::GetBinDir() + "rcc/wizard.rcc").GetSystemPath();
         QResource::registerResource(QString::fromStdString(mRccPath));
      }
   }

   new Undo();

   wkf::VtkEnvironment::Create(new Factory);
   wkf::Environment::Create("wizard", "wiz");
   // Redirect standard output
   Environment::Create();
   RunManager::Create();

   wkfEnv.StartUp();
   wizEnv.UpdateMenus();
   wkfEnv.GetMainWindow()->setWindowTitle("wizard");
   wkfEnv.GetMainWindow()->centralWidget()->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

   if (!qApp->arguments().empty())
   {
      wizRunMgr.LoadScenario(qApp->arguments());
   }
   else
   {
      QStringList files;
      // launch dialog if no scenario was specified.
      StartupDialog dialog(files, nullptr, Qt::MSWindowsFixedSizeDialogHint);
      if (dialog.exec() != QDialog::Rejected)
      {
         wizRunMgr.LoadScenario(files);
      }
   }

   mApp->exec();

   // cleanup the environment
   wizRunMgr.Shutdown();
   wizEnv.Shutdown();
   wkfEnv.Shutdown();
   return;
}

wizard::Runner::~Runner()
{
   // Unregister the QResource from initialization
   QResource::unregisterResource(QString::fromStdString(mRccPath));
}
