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

#ifndef WKFABOUTDIALOG_HPP
#define WKFABOUTDIALOG_HPP

#include "wkf_export.h"

#include <QDialog>
#include <QLabel>
#include <QTextBrowser>

#include "WkfDialog.hpp"
#include "ui_WkfAboutDialog.h"

namespace wkf
{
class MainWindow;

class WKF_EXPORT AboutDialog : public Dialog
{
   // 二次开发（i18n）：补 Q_OBJECT 使 tr() 翻译上下文命中本类（菜单触发动作文本汉化）
   Q_OBJECT
public:
   AboutDialog(MainWindow* parent, Qt::WindowFlags f = Qt::WindowFlags());
   ~AboutDialog() override;

protected:
   QLabel* GetInfoLabel();

private:
   void ArtworkButtonHandler();
   void LicenseButtonHandler();
   void SystemInfoButtonHandler();

   void         PopulateArtworkDialog(QTextBrowser& aTextBrowser);
   void         PopulateSystemInfoDialog(QTextBrowser& aTextBrowser);
   virtual void PopulateSystemInfoDialogApplication(QTextBrowser& aTextBrowser) {}

   Ui::WkfAboutDialog mUi;

   QDialog* mArtworkDialog;
   QDialog* mLicenseDialog;
   QDialog* mSystemInfoDialog;
};
} // namespace wkf

#endif