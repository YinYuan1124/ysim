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

#ifndef WKFDOCUMENTATIONDIALOG_HPP
#define WKFDOCUMENTATIONDIALOG_HPP

#include "wkf_export.h"

#include <QVBoxLayout>

#include "WkfDialog.hpp"
#include "ui_WkfDocumentationDialog.h"

namespace wkf
{
class MainWindow;

class WKF_EXPORT DocumentationDialog : public Dialog
{
   // 二次开发（i18n）：补 Q_OBJECT 使 tr() 翻译上下文命中本类（菜单触发动作文本汉化）
   Q_OBJECT
public:
   DocumentationDialog(MainWindow* parent, Qt::WindowFlags f = Qt::WindowFlags());
   ~DocumentationDialog() override = default;

protected:
   virtual void PopulateDialog();

   void         CreateDocLink(const QString& link, const QString& text, QLayout* layout);
   QVBoxLayout* GetLayout() const;

private:
   void showEvent(QShowEvent* aEvent) override;

   Ui::WkfDocumentationDialog mUi;

   bool mIsPopulated;
};
} // namespace wkf
#endif
