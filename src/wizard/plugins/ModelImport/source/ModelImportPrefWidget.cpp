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

#include "ModelImportPrefWidget.hpp"

#include <QMessageBox>

#include "ModelImportUtility.hpp"
#include "WkfEnvironment.hpp"

ModelImport::PrefWidget::PrefWidget(QWidget* aParent, Qt::WindowFlags aFlag)
   : wkf::PrefWidgetT<ModelImport::PrefObject>(aParent, aFlag)
{
   mUI.setupUi(this);

   // Connect browse
   connect(mUI.browsePushButton, &QPushButton::clicked, this, &ModelImport::PrefWidget::BrowseForDirectory);
   connect(mUI.updatePushButton, &QPushButton::clicked, this, &ModelImport::PrefWidget::PromptForUpdate);
   connect(mUI.reloadPushButton, &QPushButton::clicked, this, &ModelImport::PrefWidget::ReloadJson);
}

// ================================================================================================

void ModelImport::PrefWidget::ReadPreferenceData(const PrefDataType& aPrefData)
{
   mUI.pathLineEdit->setText(aPrefData.mPath);
   mUI.metaFileLineEdit->setText(aPrefData.mMetaFile);
   mUI.importsFileLineEdit->setText(aPrefData.mImportsFile);
   mUI.errorsFileLineEdit->setText(aPrefData.mErrorsFile);
   mUI.viewModeSelector->setCurrentText(aPrefData.mDisplayMode);
}

// ================================================================================================

void ModelImport::PrefWidget::WritePreferenceData(PrefDataType& aPrefData)
{
   aPrefData.mImportsFile = mUI.importsFileLineEdit->text();
   aPrefData.mErrorsFile  = mUI.errorsFileLineEdit->text();
   aPrefData.mDisplayMode = mUI.viewModeSelector->currentText();

   if (QFile::exists(mUI.pathLineEdit->text()))
   {
      aPrefData.mPath = mUI.pathLineEdit->text();

      if (QFile::exists(ModelImport::Path(aPrefData.mPath, mUI.metaFileLineEdit->text())))
      {
         aPrefData.mMetaFile = mUI.metaFileLineEdit->text();
      }
      else if (QMessageBox::Yes ==
               QMessageBox::question(
                  nullptr,
                  QString(tr("Model Import Beta")),
                  QString(tr("The file \"%1\" could not be located. Would you like Wizard to generate it for you?"))
                     .arg(aPrefData.mMetaFile)))
      {
         emit UpdateWithOverwrite();
      }
      else
      {
         emit ClearData();
      }
   }
   else if (!mUI.pathLineEdit->text().isEmpty())
   {
      // 二次开发（i18n）：警告框标题与文本 tr() 化（%1 为用户输入路径）
      QMessageBox::warning(nullptr,
                           tr("Model Import Beta"),
                           QString(tr("The path \"%1\" does not exist.")).arg(mUI.pathLineEdit->text()));
   }
}

// ================================================================================================

void ModelImport::PrefWidget::BrowseForDirectory()
{
   // 二次开发（i18n）：目录对话框标题 tr() 化
   QString dir = wkf::getExistingDirectory(this, tr("Model Import Search Directory"), mUI.pathLineEdit->text());

   if (!dir.isEmpty())
   {
      mUI.pathLineEdit->setText(dir);
   }
}

// ================================================================================================

void ModelImport::PrefWidget::PromptForUpdate()
{
   ApplyChanges();
   if (QFile::exists(ModelImport::Path(mUI.pathLineEdit->text(), mUI.metaFileLineEdit->text())))
   {
      // 二次开发（i18n）：对话框标题/文本与按钮 tr() 化（按钮判定走指针比较不受影响）
      QMessageBox dialog(QMessageBox::Icon::Question,
                         tr("Overwrite Model Mapping File?"),
                         tr("How should Wizard handle the existing Model Mapping File?"));

      QPushButton* mergeButton     = dialog.addButton(tr("Merge"), QMessageBox::ButtonRole());
      QPushButton* overwriteButton = dialog.addButton(tr("Overwrite"), QMessageBox::ButtonRole());
      dialog.addButton(tr("Cancel"), QMessageBox::ButtonRole::RejectRole);

      dialog.exec();

      if (dialog.clickedButton() == mergeButton)
      {
         emit UpdateWithMerge();
      }
      else if (dialog.clickedButton() == overwriteButton)
      {
         emit UpdateWithOverwrite();
      }
   }
   else
   {
      emit UpdateWithOverwrite();
   }
}
