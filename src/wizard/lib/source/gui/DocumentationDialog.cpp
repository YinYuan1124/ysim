// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2003-2015 The Boeing Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "DocumentationDialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>

#include "WkfEnvironment.hpp"

wizard::DocumentationDialog::DocumentationDialog(wkf::MainWindow* parent, Qt::WindowFlags f /*= Qt::WindowFlags()*/)
   : wkf::DocumentationDialog(parent, f)
{
}

void wizard::DocumentationDialog::PopulateDialog()
{
   QFileInfo fi(wkfEnv.GetDocumentationDir());

   QHBoxLayout* hLayout = new QHBoxLayout();
   QVBoxLayout* vLayout = new QVBoxLayout();
   hLayout->addLayout(vLayout);

   QLabel* image = new QLabel(this);
   image->setMaximumSize(QSize(128, 128));
   //   image->setPixmap(QPixmap(QString::fromUtf8(":/icons/aer.ico")));
   image->setScaledContents(true);
   hLayout->addWidget(image);

   if (fi.isDir())
   {
      QString dir(fi.absoluteFilePath().append("/html/docs/"));
      // 二次开发（i18n）：文档链接文本经基类 tr() 翻译（本类无 Q_OBJECT，
      // 显式限定 wkf::DocumentationDialog 上下文，四应用共用同一份译文）
      CreateDocLink(dir + "wizard.html", wkf::DocumentationDialog::tr("Main Page"), vLayout);
      CreateDocLink(dir + "wizard_users_guide.html", wkf::DocumentationDialog::tr("User's Guide"), vLayout);
      CreateDocLink(dir + "wizard_reference_guide.html", wkf::DocumentationDialog::tr("Reference Guide"), vLayout);
      CreateDocLink(dir + "wizard_change_log.html", wkf::DocumentationDialog::tr("Change Log"), vLayout);
      CreateDocLink(dir + "main_page.html", wkf::DocumentationDialog::tr("AFSIM Main Page"), vLayout);
   }

   if (vLayout->count() == 0)
   {
      // 二次开发（i18n）：无文档提示 tr() 化
      QLabel* labelPtr = new QLabel(wkf::DocumentationDialog::tr("<p style=\"font-size:10pt;\"><i>No documentation found</i></p>"), this);
      vLayout->addWidget(labelPtr);
      vLayout->addStretch();
   }

   GetLayout()->addLayout(hLayout);

   QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
   buttonBox->setObjectName(QStringLiteral("buttonBox"));
   buttonBox->setOrientation(Qt::Horizontal);
   buttonBox->setStandardButtons(QDialogButtonBox::Ok);
   connect(buttonBox, &QDialogButtonBox::accepted, this, [this] { hide(); });
   GetLayout()->addWidget(buttonBox);
}
