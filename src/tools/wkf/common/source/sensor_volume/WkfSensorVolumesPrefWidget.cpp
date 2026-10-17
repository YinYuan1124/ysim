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
#include "WkfSensorVolumesPrefWidget.hpp"

#include <assert.h>
#include <iostream>

#include <QCheckBox>
#include <QColorDialog>
#include <QCoreApplication> // 二次开发（i18n）：自由函数内 translate 显式翻译所需
#include <QInputDialog>

#include "UtQtColorButton.hpp"
#include "WkfSensorVolumesPrefObject.hpp"

namespace
{
class ColorAndEnable : public QWidget
{
public:
   ColorAndEnable(QWidget* aParent)
      : QWidget(aParent)
   {
      mCheckBox    = new QCheckBox(this);
      mColorButton = new UtQtColorButton(this);
      mColorButton->ShowAlpha(true);
      QHBoxLayout* layout = new QHBoxLayout(this);
      layout->addSpacing(10);
      layout->addWidget(mCheckBox);
      layout->addWidget(mColorButton);
      layout->setMargin(0);
   }
   void   setCheckState(bool aState) { mCheckBox->setChecked(aState); }
   void   setColor(const QColor& aColor) { mColorButton->SetColor(aColor); }
   bool   isChecked() const { return mCheckBox->isChecked(); }
   QColor getColor() const { return mColorButton->GetColor(); }

private:
   QCheckBox*       mCheckBox;
   UtQtColorButton* mColorButton;
};

void BuildRow(QTableWidget*  aWidget,
              const QString& aName,
              bool           aShowFaces,
              const QColor&  aFaceColor,
              bool           aShowEdges,
              const QColor&  aEdgeColor,
              bool           aShowProjections,
              const QColor&  aProjColor)
{
   int row = aWidget->rowCount();
   aWidget->insertRow(row);
   aWidget->setCellWidget(row, 0, new QLabel(aName, aWidget));

   auto* faces = new ColorAndEnable(aWidget);
   faces->setCheckState(aShowFaces);
   faces->setColor(aFaceColor);
   aWidget->setCellWidget(row, 1, faces);

   auto* edges = new ColorAndEnable(aWidget);
   edges->setCheckState(aShowEdges);
   edges->setColor(aEdgeColor);
   aWidget->setCellWidget(row, 2, edges);

   auto* projections = new ColorAndEnable(aWidget);
   projections->setCheckState(aShowProjections);
   projections->setColor(aProjColor);
   aWidget->setCellWidget(row, 3, projections);

   aWidget->resizeRowToContents(row);
}

void AddMode(QTableWidget* aWidget, const std::set<std::string>& aNames, wkf::SensorVolumesPrefWidget::EntryMethod aEntryMethod)
{
   QStringList undefinedModes;

   for (const auto& it : aNames)
   {
      bool found = false;
      for (int i = 0; i < aWidget->rowCount(); ++i)
      {
         auto label = dynamic_cast<QLabel*>(aWidget->cellWidget(i, 0));
         if (label)
         {
            if (label->text().toStdString() == it)
            {
               found = true;
               break;
            }
         }
      }
      if (!found)
      {
         undefinedModes.push_back(QString::fromStdString(it));
      }
   }

   undefinedModes.push_back("");

   bool    ok = true;
   QString mode;
   // 二次开发（i18n）：输入对话框标题与标签翻译化。此处为自由函数，tr() 不可用，
   // 改用 QCoreApplication::translate 显式指定 wkf::PrefWidget 上下文（该类无 Q_OBJECT，
   // 成员函数内 tr() 亦落基类上下文，保持一致）。模式名列表/用户输入值为键，不译。
   if (aEntryMethod == wkf::SensorVolumesPrefWidget::EntryMethod::cCOMBOBOX)
   {
      mode = QInputDialog::getItem(aWidget,
                                   QCoreApplication::translate("wkf::PrefWidget", "Select a mode"),
                                   QCoreApplication::translate("wkf::PrefWidget", "Mode:"),
                                   undefinedModes,
                                   0,
                                   true,
                                   &ok);
   }
   else if (aEntryMethod == wkf::SensorVolumesPrefWidget::EntryMethod::cLINEEDIT)
   {
      mode = QInputDialog::getText(aWidget,
                                   QCoreApplication::translate("wkf::PrefWidget", "Enter a mode"),
                                   QCoreApplication::translate("wkf::PrefWidget", "Mode:"),
                                   QLineEdit::Normal,
                                   "",
                                   &ok);
   }
   if ((ok) && !mode.isEmpty())
   {
      int defaultMode = -1;
      for (int i = 0; i < aWidget->rowCount(); ++i)
      {
         auto label = dynamic_cast<QLabel*>(aWidget->cellWidget(i, 0));
         if (label)
         {
            if (label->text().toStdString() == "default")
            {
               defaultMode = i;
               break;
            }
         }
      }
      if (defaultMode >= 0)
      {
         auto faceW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(defaultMode, 1));
         auto edgeW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(defaultMode, 2));
         auto projW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(defaultMode, 3));
         if (faceW && edgeW && projW)
         {
            BuildRow(aWidget,
                     mode,
                     faceW->isChecked(),
                     faceW->getColor(),
                     edgeW->isChecked(),
                     edgeW->getColor(),
                     projW->isChecked(),
                     projW->getColor());
         }
      }
   }
}

QString GetRowName(QTableWidget* aWidget, int aRow)
{
   auto label = dynamic_cast<QLabel*>(aWidget->cellWidget(aRow, 0));
   if (label)
   {
      return label->text();
   }
   return "";
}
} // namespace

wkf::SensorVolumesPrefWidget::SensorVolumesPrefWidget(EntryMethod aEntryMethod)
   : PrefWidgetT<SensorVolumesPrefObject>()
   , mEntryMethod(aEntryMethod)
{
   mUi.setupUi(this);

   mUi.sensorTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
   mUi.weaponTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

   connect(mUi.addSensorButton, &QPushButton::clicked, this, &wkf::SensorVolumesPrefWidget::AddSensorMode);
   connect(mUi.deleteSensorButton, &QPushButton::clicked, this, &wkf::SensorVolumesPrefWidget::DeleteSensorMode);
   connect(mUi.addWeaponButton, &QPushButton::clicked, this, &wkf::SensorVolumesPrefWidget::AddWeaponMode);
   connect(mUi.deleteWeaponButton, &QPushButton::clicked, this, &wkf::SensorVolumesPrefWidget::DeleteWeaponMode);
   connect(mUi.sensorTableWidget, &QTableWidget::currentCellChanged, this, &wkf::SensorVolumesPrefWidget::SensorCellChanged);
   connect(mUi.weaponTableWidget, &QTableWidget::currentCellChanged, this, &wkf::SensorVolumesPrefWidget::WeaponCellChanged);
}

void wkf::SensorVolumesPrefWidget::WritePreferenceData(SensorVolumesPrefData& aPrefData)
{
   aPrefData.mModeMap.clear();

   auto writePrefHelper = [&aPrefData](int aRow, bool weapon, QTableWidget* aWidget)
   {
      auto label = dynamic_cast<QLabel*>(aWidget->cellWidget(aRow, 0));
      auto faceW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(aRow, 1));
      auto edgeW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(aRow, 2));
      auto projW = dynamic_cast<ColorAndEnable*>(aWidget->cellWidget(aRow, 3));

      if (label && faceW && edgeW && projW)
      {
         std::string name           = label->text().toStdString();
         bool        showFaces      = faceW->isChecked();
         QColor      faceColor      = faceW->getColor();
         bool        showEdges      = edgeW->isChecked();
         QColor      edgeColor      = edgeW->getColor();
         bool        showProjection = projW->isChecked();
         QColor      projColor      = projW->getColor();

         aPrefData.mModeMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name, weapon),
            std::forward_as_tuple(showFaces, faceColor, showEdges, edgeColor, showProjection, projColor));
      }
   };

   for (int i = 0; i < mUi.sensorTableWidget->rowCount(); ++i)
   {
      writePrefHelper(i, false, mUi.sensorTableWidget);
   }

   for (int i = 0; i < mUi.weaponTableWidget->rowCount(); ++i)
   {
      writePrefHelper(i, true, mUi.weaponTableWidget);
   }

   // 二次开发（i18n）：组合框项翻译后 currentText() 与英文比较会失配，改按 currentIndex 映射
   //（.ui 中 mVolumeType 项顺序 0-5 与 SensorVolumesPrefData::DrawMode 枚举值一一对应）
   switch (mUi.mVolumeType->currentIndex())
   {
   case 0:
      aPrefData.mDrawMode = SensorVolumesPrefData::cSLEW_VOLUME;
      break;
   case 1:
      aPrefData.mDrawMode = SensorVolumesPrefData::cCUE_VOLUME;
      break;
   case 2:
      aPrefData.mDrawMode = SensorVolumesPrefData::cSCAN_VOLUME;
      break;
   case 3:
      aPrefData.mDrawMode = SensorVolumesPrefData::cFIELD_OF_VIEW;
      break;
   case 4:
      aPrefData.mDrawMode = SensorVolumesPrefData::cBEAM_WIDTH;
      break;
   default:
      aPrefData.mDrawMode = SensorVolumesPrefData::cCALCULATED;
      break;
   }
}

void wkf::SensorVolumesPrefWidget::ReadPreferenceData(const SensorVolumesPrefData& aPrefData)
{
   mUi.sensorTableWidget->clearContents();
   mUi.sensorTableWidget->setRowCount(0);
   mUi.weaponTableWidget->clearContents();
   mUi.weaponTableWidget->setRowCount(0);

   for (const auto& it : aPrefData.mModeMap)
   {
      if (it.first.second)
      {
         BuildRow(mUi.weaponTableWidget,
                  QString::fromStdString(it.first.first),
                  it.second.mShowFaces,
                  it.second.mFaceColor,
                  it.second.mShowEdges,
                  it.second.mEdgeColor,
                  it.second.mShowProjection,
                  it.second.mProjColor);
      }
      else
      {
         BuildRow(mUi.sensorTableWidget,
                  QString::fromStdString(it.first.first),
                  it.second.mShowFaces,
                  it.second.mFaceColor,
                  it.second.mShowEdges,
                  it.second.mEdgeColor,
                  it.second.mShowProjection,
                  it.second.mProjColor);
      }
   }
   mUi.sensorTableWidget->resizeRowsToContents();
   mUi.weaponTableWidget->resizeRowsToContents();
   // 二次开发（i18n）：组合框项翻译后 setCurrentText 按英文设置会失配，改按 currentIndex 恢复
   switch (aPrefData.mDrawMode)
   {
   case (SensorVolumesPrefData::cSLEW_VOLUME):
      mUi.mVolumeType->setCurrentIndex(0);
      break;
   case (SensorVolumesPrefData::cCUE_VOLUME):
      mUi.mVolumeType->setCurrentIndex(1);
      break;
   case (SensorVolumesPrefData::cSCAN_VOLUME):
      mUi.mVolumeType->setCurrentIndex(2);
      break;
   case (SensorVolumesPrefData::cFIELD_OF_VIEW):
      mUi.mVolumeType->setCurrentIndex(3);
      break;
   case (SensorVolumesPrefData::cBEAM_WIDTH):
      mUi.mVolumeType->setCurrentIndex(4);
      break;
   case (SensorVolumesPrefData::cCALCULATED):
      mUi.mVolumeType->setCurrentIndex(5);
      break;
   }
}

void wkf::SensorVolumesPrefWidget::AddSensorMode()
{
   AddMode(mUi.sensorTableWidget, mSensorModeNameSet, mEntryMethod);
}

void wkf::SensorVolumesPrefWidget::DeleteSensorMode()
{
   mUi.sensorTableWidget->removeRow(mUi.sensorTableWidget->currentRow());
   mUi.deleteSensorButton->setEnabled(false);
}

void wkf::SensorVolumesPrefWidget::AddWeaponMode()
{
   AddMode(mUi.weaponTableWidget, mWeaponModeNameSet, mEntryMethod);
}

void wkf::SensorVolumesPrefWidget::DeleteWeaponMode()
{
   mUi.weaponTableWidget->removeRow(mUi.weaponTableWidget->currentRow());
   mUi.deleteSensorButton->setEnabled(false);
}

void wkf::SensorVolumesPrefWidget::SensorCellChanged(int aRow, int aColumn)
{
   if (GetRowName(mUi.sensorTableWidget, aRow) != "default")
   {
      mUi.deleteSensorButton->setEnabled(true);
   }
   else
   {
      mUi.deleteSensorButton->setEnabled(false);
   }
}

void wkf::SensorVolumesPrefWidget::WeaponCellChanged(int aRow, int aColumn)
{
   if (GetRowName(mUi.weaponTableWidget, aRow) != "default")
   {
      mUi.deleteWeaponButton->setEnabled(true);
   }
   else
   {
      mUi.deleteWeaponButton->setEnabled(false);
   }
}

void wkf::SensorVolumesPrefWidget::AddSensorModeName(const std::string& aName)
{
   if (aName != "default")
   {
      mSensorModeNameSet.insert(aName);
   }
}

void wkf::SensorVolumesPrefWidget::AddWeaponModeName(const std::string& aName)
{
   if (aName != "default")
   {
      mWeaponModeNameSet.insert(aName);
   }
}
