// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2021 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************
#include "RvPlatformHistoryBehaviorColorWidget.hpp"

#include <QCoreApplication> // 二次开发（i18n）：类无 Q_OBJECT，显式上下文翻译所需

#include "UtCast.hpp"
#include "UtQtColorButton.hpp"
#include "WkfUtils.hpp"
#include "platform_history/WkfPlatformHistoryPrefObject.hpp"

RvPlatformHistory::PlatformHistoryBehaviorColorWidget::PlatformHistoryBehaviorColorWidget()
   : wkf::PlatformHistoryColorWidget()
{
   mUi.setupUi(this);

   mUi.mAddBehavior->setIcon(QIcon::fromTheme("add"));
   mUi.mRemoveBehavior->setIcon(QIcon::fromTheme("remove"));

   connect(mUi.mAddBehavior,
           &QPushButton::clicked,
           this,
           &RvPlatformHistory::PlatformHistoryBehaviorColorWidget::BehaviorColorAdded);
   connect(mUi.mRemoveBehavior,
           &QPushButton::clicked,
           this,
           &RvPlatformHistory::PlatformHistoryBehaviorColorWidget::BehaviorColorRemoved);
}

void RvPlatformHistory::PlatformHistoryBehaviorColorWidget::WritePreferenceData(wkf::TraceLinesPrefData& aPrefData)
{
   aPrefData.mBehaviorTraceLineColor.clear();
   for (int i = 0; i < mUi.mBehaviorColorTable->rowCount(); ++i)
   {
      QTableWidgetItem* item        = mUi.mBehaviorColorTable->item(i, 0);
      auto*             colorButton = dynamic_cast<UtQtColorButton*>(mUi.mBehaviorColorTable->cellWidget(i, 1));
      if (item && colorButton)
      {
         aPrefData.mBehaviorTraceLineColor.emplace(item->text().toStdString(), colorButton->GetColor());
      }
   }
   aPrefData.mBehaviorDefaultColor = mUi.mNoBehaviorColorButton->GetColor();
}

void RvPlatformHistory::PlatformHistoryBehaviorColorWidget::BehaviorColorAdded()
{
   // Check to see if new name is unique
   QString behaviorName = mUi.mBehaviorName_tx->text();
   if (behaviorName.contains(" "))
   {
      QMessageBox::information(nullptr, QCoreApplication::translate("RvPlatformHistory::PlatformHistoryBehaviorColorWidget", "Invalid Behavior Entry"), QCoreApplication::translate("RvPlatformHistory::PlatformHistoryBehaviorColorWidget", "Behavior name can't include space characters.")); // 二次开发（i18n）：提示框接入翻译通道
   }
   else if (!behaviorName.isEmpty())
   {
      bool unique = true;
      for (int i = 0; i < mUi.mBehaviorColorTable->rowCount(); ++i)
      {
         if (behaviorName == mUi.mBehaviorColorTable->item(i, 0)->text())
         {
            unique = false;
            break;
         }
      }

      if (unique)
      {
         // if it is unique add a row to the table.
         int row = mUi.mBehaviorColorTable->rowCount();
         mUi.mBehaviorColorTable->setRowCount(row + 1);

         QTableWidgetItem* item = new QTableWidgetItem();
         item->setText(behaviorName);
         item->setFlags(item->flags() & ~Qt::ItemIsEditable);
         mUi.mBehaviorColorTable->setItem(row, 0, item);

         UtQtColorButton* colorButton = new UtQtColorButton(mUi.mBehaviorColorTable);
         // default color.
         colorButton->SetColor(QColor(128, 128, 128, 255));
         mUi.mBehaviorColorTable->setCellWidget(row, 1, colorButton);

         mUi.mBehaviorColorTable->scrollToBottom();
         mUi.mBehaviorColorTable->resizeRowsToContents();
      }
      else
      {
         QMessageBox::information(nullptr,
                                  QCoreApplication::translate("RvPlatformHistory::PlatformHistoryBehaviorColorWidget", "Duplicate Behavior Entry"), // 二次开发（i18n）：标题接入翻译通道
                                  QCoreApplication::translate("RvPlatformHistory::PlatformHistoryBehaviorColorWidget", "Behavior %1 already exists, unable to add team.").arg(behaviorName)); // 二次开发（i18n）：正文接入翻译通道（参数化）
      }
   }
}

void RvPlatformHistory::PlatformHistoryBehaviorColorWidget::BehaviorColorRemoved()
{
   for (QTableWidgetItem* item : mUi.mBehaviorColorTable->selectedItems())
   {
      mUi.mBehaviorColorTable->removeRow(item->row());
   }
}

void RvPlatformHistory::PlatformHistoryBehaviorColorWidget::SetBehaviorColors(const std::map<std::string, QColor>& aBehaviorColors)
{
   int row = 0;
   mUi.mBehaviorColorTable->clearContents();
   mUi.mBehaviorColorTable->setRowCount(ut::cast_to_int(aBehaviorColors.size()));
   for (auto& colorIter : aBehaviorColors)
   {
      QTableWidgetItem* behaviorItem = new QTableWidgetItem();
      behaviorItem->setText(QString::fromStdString(colorIter.first));
      behaviorItem->setFlags(behaviorItem->flags() & ~Qt::ItemIsEditable);
      mUi.mBehaviorColorTable->setItem(row, 0, behaviorItem);

      UtQtColorButton* colorButton = new UtQtColorButton(mUi.mBehaviorColorTable);
      colorButton->SetColor(colorIter.second);
      mUi.mBehaviorColorTable->setCellWidget(row, 1, colorButton);

      row++;
   }
}

void RvPlatformHistory::PlatformHistoryBehaviorColorWidget::ReadPreferenceData(const wkf::TraceLinesPrefData& aPrefData)
{
   SetBehaviorColors(aPrefData.mBehaviorTraceLineColor);
   mUi.mNoBehaviorColorButton->SetColor(aPrefData.mBehaviorDefaultColor);
}
