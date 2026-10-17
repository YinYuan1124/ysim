// ****************************************************************************
// CUI//REL TO USA ONLY
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2017-2018 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "MoverCreatorApplication.hpp"
#include "UtPath.hpp"

#include <QTranslator> // 二次开发（i18n）：中文翻译加载

int main(int argc, char* argv[])
{
   // Add library path for Qt plugins. Must be done before QApplication is created.
   // Get the executable path
   UtPath binDir = UtPath::GetExePath();
   // Trim the name of the executable
   binDir.Up();

   QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
   QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
   Designer::MoverCreatorApplication app(argc, argv);

   // 二次开发（i18n）：安装中文翻译器，与 wizard 共用 build/Release/i18n 下的 qm 文件。
   // qm 存在即启用中文，删除 qm 即回退英文，无需语言偏好设置。
   {
      const QString i18nDir = QCoreApplication::applicationDirPath() + "/i18n";
      auto* appTranslator = new QTranslator(&app);
      if (appTranslator->load("wizard_zh_CN", i18nDir))
      {
         app.installTranslator(appTranslator);
      }
      // Qt 内置控件的翻译（OK/取消按钮、颜色对话框等）
      auto* qtTranslator = new QTranslator(&app);
      if (qtTranslator->load("qtbase_zh_CN", i18nDir))
      {
         app.installTranslator(qtTranslator);
      }
   }

   int retVal = app.Execute();
   return retVal;
}
