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
#include "WkSimPrefWidget.hpp"

#include <QMessageBox>

#include "WkSimPrefObject.hpp"

warlock::SimPrefWidget::SimPrefWidget(QWidget* parent)
   : wkf::PrefWidgetT<SimPrefObject>(parent)
{
   mUi.setupUi(this);

   mUi.multicastHelpPushButton->setIcon(QIcon::fromTheme("help"));
   mUi.interfaceHelpPushButton->setIcon(QIcon::fromTheme("help"));

   connect(mUi.multicastHelpPushButton, &QPushButton::clicked, this, &SimPrefWidget::MulticastHelpButton);
   connect(mUi.interfaceHelpPushButton, &QPushButton::clicked, this, &SimPrefWidget::InterfaceHelpButton);

   // 二次开发（i18n）：时钟率组合框项 tr() 化（存取走 currentData/findData 不受影响）
   mUi.simClockRateComboBox->addItem(tr("1 hz"), 1.0);
   mUi.simClockRateComboBox->addItem(tr("5 hz"), 5.0);
   mUi.simClockRateComboBox->addItem(tr("10 hz"), 10.0);
   mUi.simClockRateComboBox->addItem(tr("20 hz"), 20.0);
   mUi.simClockRateComboBox->addItem(tr("30 hz"), 30.0);
   mUi.simClockRateComboBox->addItem(tr("60 hz"), 60.0);

   // Deferred Start Time should not be less than 1 second
   mUi.deferredStartTimeLineEdit->SetBounds(1.0, std::numeric_limits<double>::max());
}

void warlock::SimPrefWidget::WritePreferenceData(SimPrefData& aPrefData)
{
   bool enableDIS = mUi.disGroupBox->isChecked();
   if (enableDIS && !ValidateInputs())
   {
      enableDIS = false;
      // 二次开发（i18n）：警告框标题与文本 tr() 化
      QMessageBox::warning(nullptr,
                           tr("Network Config Error"),
                           QString(
                              tr("The DIS Settings specified in Preferences are not valid.\n Disabling DIS Settings.")));
      // immediately uncheck the preference box when there is invalid DIS setting input
      mUi.disGroupBox->setChecked(enableDIS);
   }

   aPrefData.startPaused          = mUi.startPausedCheckBox->isChecked();
   aPrefData.platformsDraggable   = mUi.draggablePlatformsCheckBox->isChecked();
   aPrefData.enableDIS            = enableDIS;
   aPrefData.multicastIp          = mUi.ipAddressLineEdit->text();
   aPrefData.netId                = mUi.netIdLineEdit->text();
   aPrefData.port                 = mUi.portSpinBox->value();
   aPrefData.siteId               = mUi.siteIdSpinBox->value();
   aPrefData.applicationId        = mUi.applicationIdSpinBox->value();
   aPrefData.exerciseId           = mUi.exerciseIdSpinBox->value();
   aPrefData.enabledDeferredStart = mUi.deferredStartGroupBox->isChecked();
   aPrefData.deferredStartTimeSec = mUi.deferredStartTimeLineEdit->GetValue();
   aPrefData.clockRate            = mUi.simClockRateComboBox->currentData().toDouble();
}

bool warlock::SimPrefWidget::ValidateInputs()
{
   return (mUi.ipAddressLineEdit->hasAcceptableInput() && mUi.netIdLineEdit->hasAcceptableInput());
}

void warlock::SimPrefWidget::MulticastHelpButton()
{
   // 二次开发（i18n）：帮助框标题与文本 tr() 化（IP 段为技术值不译）
   QMessageBox::information(nullptr,
                            tr("Multicast Addressing"),
                            tr("The multicast address must be the same for all connected simulations.\nValid address are "
                               "in the range 224.0.0.0 to 239.255.255.255."));
}

void warlock::SimPrefWidget::InterfaceHelpButton()
{
   // 二次开发（i18n）：帮助框标题与文本 tr() 化（接口地址为运行时数据不译）
   QStringList list = UtQtNetwork::GetMulticastIpv4Interfaces();
   QString     text(tr("The interface address tells Warlock which network card to use for sending messages. This will be "
                       "different for different computers.\nAvailable addresses are:\n"));
   if (list.empty())
   {
      text.append(tr("No Valid Interfaces Detected."));
   }
   else
   {
      for (const auto& addr : list)
      {
         text.append(addr + "\n");
      }
   }
   text.append(tr("Note: Not all available address will work for multicast. This depends on your device configuration."));
   QMessageBox::information(nullptr, tr("Interface Address"), text);
}

void warlock::SimPrefWidget::showEvent(QShowEvent*)
{
   // This is necessary because the pref widget isn't constructed until after settings are loaded into pref object
   ReadPreferenceData(mPrefObjectPtr->GetPreferences());
}

void warlock::SimPrefWidget::ReadPreferenceData(const SimPrefData& aPrefData)
{
   mUi.startPausedCheckBox->setChecked(aPrefData.startPaused);
   mUi.draggablePlatformsCheckBox->setChecked(aPrefData.platformsDraggable);
   mUi.disGroupBox->setChecked(aPrefData.enableDIS);
   mUi.ipAddressLineEdit->setText(aPrefData.multicastIp);
   mUi.netIdLineEdit->setText(aPrefData.netId);
   mUi.portSpinBox->setValue(aPrefData.port);
   mUi.siteIdSpinBox->setValue(aPrefData.siteId);
   mUi.applicationIdSpinBox->setValue(aPrefData.applicationId);
   mUi.exerciseIdSpinBox->setValue(aPrefData.exerciseId);
   mUi.deferredStartGroupBox->setChecked(aPrefData.enabledDeferredStart);
   mUi.deferredStartTimeLineEdit->SetValue(aPrefData.deferredStartTimeSec);

   int index = mUi.simClockRateComboBox->findData(aPrefData.clockRate);
   if (index == -1)
   {
      // 30 Hz should be present in the list.  This case only happens if a user modified the INI file
      //  by hand and entered a value other than one of the supported ones.
      mUi.simClockRateComboBox->setCurrentIndex(mUi.simClockRateComboBox->findData(30.0));
   }
   else
   {
      mUi.simClockRateComboBox->setCurrentIndex(index);
   }
}
