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
#include "WkfOrbitPrefWidget.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>

#include "WkfOrbitPrefObject.hpp"

wkf::OrbitPrefWidget::OrbitPrefWidget(bool aRealtime)
   : wkf::PrefWidgetT<wkf::OrbitPrefObject>()
{
   setWindowTitle(tr("Orbits")); // 二次开发（i18n）：页面标题 tr() 化

   QFormLayout* form = new QFormLayout(this);
   setLayout(form);
   if (aRealtime)
   {
      // 二次开发（i18n）：表单标签 tr() 化
      mFadeout = new QSpinBox(this);
      mFadeout->setRange(1, 99999);
      form->addRow(tr("Fadeout (sec):"), mFadeout);
      mColorMode = new QComboBox(this);
      // 二次开发（i18n）：颜色模式组合框项 tr() 化（存取走 currentData 不受翻译影响）
      mColorMode->addItem(tr("White"), OrbitPrefData::eWHITE);
      mColorMode->addItem(tr("Team"), OrbitPrefData::eTEAM);
      mColorMode->addItem(tr("Name"), OrbitPrefData::eNAMEHASH);
      mColorMode->addItem(tr("Scenario"), OrbitPrefData::eSCENARIO);
      form->addRow(tr("Color:"), mColorMode);
   }
   mLineWidth = new QSpinBox(this);
   mLineWidth->setRange(1, 6);
   form->addRow(tr("Line Width:"), mLineWidth);
   mPeriods = new QSpinBox(this);
   mPeriods->setRange(1, 16);
   form->addRow(tr("Periods:"), mPeriods);
}

void wkf::OrbitPrefWidget::WritePreferenceData(OrbitPrefData& aPrefData)
{
   if (mFadeout)
   {
      aPrefData.mFadeout = mFadeout->value();
   }
   if (mColorMode)
   {
      aPrefData.mColorMode = static_cast<OrbitPrefData::ColorMode>(mColorMode->currentData().toInt());
   }
   aPrefData.mLineWidth = mLineWidth->value();
   aPrefData.mPeriods   = mPeriods->value();
}

void wkf::OrbitPrefWidget::ReadPreferenceData(const OrbitPrefData& aPrefData)
{
   if (mFadeout)
   {
      mFadeout->setValue(aPrefData.mFadeout);
   }
   if (mColorMode)
   {
      // 二次开发（i18n）：组合框项已翻译，setCurrentText 按英文文本设置会失配，改用 findData 按数据恢复
      int index = mColorMode->findData(static_cast<int>(aPrefData.mColorMode));
      if (index != -1)
      {
         mColorMode->setCurrentIndex(index);
      }
   }
   mLineWidth->setValue(aPrefData.mLineWidth);
   mPeriods->setValue(aPrefData.mPeriods);
}
