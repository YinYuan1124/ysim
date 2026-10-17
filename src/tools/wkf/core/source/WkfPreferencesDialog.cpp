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

#include "WkfPreferencesDialog.hpp"

#include <QCoreApplication> // 二次开发（i18n）：QCoreApplication::translate 分类键动态翻译
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>

#include "WkfAction.hpp"
#include "WkfConfigurationObject.hpp"
#include "WkfDebugPrefWidget.hpp"
#include "WkfEnvironment.hpp"
#include "WkfGeneralPrefWidget.hpp"
#include "WkfKeyBindingsWidget.hpp"
#include "WkfMainWindow.hpp"
#include "WkfMilStdIconPrefWidget.hpp"
#include "WkfObserver.hpp"
#include "WkfOverlayUpdaterPrefWidget.hpp"
#include "WkfPlugin.hpp"
#include "WkfPluginManager.hpp"
#include "WkfSharedMapPreferencesWidget.hpp"
#include "WkfTeamVisibilityPrefWidget.hpp"
#include "WkfUnitsWidget.hpp"

wkf::PreferencesDialog::PreferencesDialog(MainWindow* parent, Qt::WindowFlags f /* = Qt::WindowFlags() */)
   : Dialog(parent, new Action(QIcon::fromTheme("wrench"), tr("Preferences..."), nullptr), "Options", f) // 二次开发（i18n）：菜单动作文本汉化
{
   mUI.setupUi(this);
   mTriggerActionPtr->setShortcut(QKeySequence(QKeySequence::Preferences));

   mStandardWidgets << new UnitsWidget(this);
   mStandardWidgets << new GeneralPrefWidget(this);
   mStandardWidgets << new SharedMapPreferencesWidget(this);
   mStandardWidgets << new DebugPrefWidget(this);
   mStandardWidgets << new MilStdIconPrefWidget(this);
   mStandardWidgets << new KeyBindingsWidget(this);
   mStandardWidgets << new TeamVisibilityPrefWidget(this);
   mStandardWidgets << new OverlayUpdaterPrefWidget(this);

   // Connect button box signals to dialog slots
   connect(mUI.buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
   connect(mUI.buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);
   connect(mUI.buttonBox->button(QDialogButtonBox::RestoreDefaults),
           &QPushButton::clicked,
           this,
           &PreferencesDialog::HandleRestoreDefaultsAction);

   connect(mUI.treeWidget, &QTreeWidget::currentItemChanged, this, &PreferencesDialog::TreeWidgetCurrentItemChanged);
}

void wkf::PreferencesDialog::closeEvent(QCloseEvent* aEvent)
{
   reject();
}

void wkf::PreferencesDialog::showEvent(QShowEvent* aEvent)
{
   // Build up the pages for the Preference Dialog, that the Dialog is populated when shown
   ConstructPreferencesDialog();
}

void wkf::PreferencesDialog::ShowPreferencePage(const QString& aPage)
{
   // Build up the pages for the Preference Dialog, so mUI.treeWidget->findItems() can find the requested page
   ConstructPreferencesDialog();

   // 二次开发（i18n）：页面标题翻译后 findItems 传英文标题失配。改为取各页面 objectName
   //（uic class，即其标题的翻译上下文）将页面标题键翻译成当前语言后与树标签比对——
   // 已译页面译成中文命中、未译页面 translate 原样返回英文同样命中，查找/显示通道对齐。
   for (int i = 0; i < mUI.treeWidget->topLevelItemCount(); ++i)
   {
      auto* categoryPtr = mUI.treeWidget->topLevelItem(i);
      for (int j = 0; j < categoryPtr->childCount(); ++j)
      {
         auto* itemPtr   = categoryPtr->child(j);
         QString pageCtx = itemPtr->data(0, Qt::UserRole + 1).toString();
         if (QCoreApplication::translate(pageCtx.toUtf8().constData(), aPage.toUtf8().constData()) ==
             itemPtr->text(0))
         {
            show();
            mUI.treeWidget->setCurrentItem(itemPtr);
            return;
         }
      }
   }
}

void wkf::PreferencesDialog::HandleRestoreDefaultsAction()
{
   QMessageBox prompt(this);

   // 二次开发（i18n）：恢复默认确认框文本 tr() 化
   prompt.setWindowTitle(tr("Restore Defaults"));
   prompt.setText(tr("Would you like to restore the defaults to all of the Preferences pages or only on the currently "
                     "displayed page?"));
   prompt.setIcon(QMessageBox::Question);
   prompt.setStandardButtons(QMessageBox::Cancel | QMessageBox::RestoreDefaults | QMessageBox::Reset);
   prompt.setDefaultButton(QMessageBox::Cancel);

   prompt.button(QMessageBox::Reset)->setText(tr("All"));
   prompt.button(QMessageBox::RestoreDefaults)->setText(tr("Current"));

   const int userChoice = prompt.exec();
   switch (userChoice)
   {
   // Restore Defaults only to the current page.
   case QMessageBox::RestoreDefaults:
   {
      auto wid = mUI.stackedWidget->currentWidget();
      if (wid)
      {
         auto* pw = mUI.stackedWidget->findChild<PrefWidget*>(wid->objectName());
         if (pw)
         {
            pw->RestoreDefaults();
         }
      }
      break;
   }
   // Restore Defaults to all of the pages
   case QMessageBox::Reset:
   {
      for (auto pw : mUI.stackedWidget->findChildren<PrefWidget*>())
      {
         pw->RestoreDefaults();
      }

      break;
   }
   case QMessageBox::Cancel:
   default:
      break;
   }
}

void wkf::PreferencesDialog::TreeWidgetCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
   if (current != nullptr)
   {
      int index = current->data(0, Qt::UserRole).toInt();
      mUI.stackedWidget->setCurrentIndex(index);
   }
}

void wkf::PreferencesDialog::ConstructPreferencesDialog()
{
   if (!mInitialized)
   {
      mInitialized = true;

      QList<PrefWidget*> prefWidgets = mStandardWidgets;

      auto plugins = wkfEnv.GetPluginManager()->GetLoadedPlugins();
      for (auto& p : plugins)
      {
         prefWidgets.append(p.second->GetPreferencesWidgets());
      }

      AddPrefWidgets(prefWidgets);

      // Sort the plugins in tree widget alphabetically
      mUI.treeWidget->sortItems(0, Qt::AscendingOrder);

      // Set "General" item as the current item.
      for (int i = 0; i < mUI.treeWidget->topLevelItemCount(); ++i)
      {
         auto* categoryPtr = mUI.treeWidget->topLevelItem(i);
         // 二次开发（i18n）：树标签翻译后文本比较失配，改用分类键（UserRole+2）比较
         if (categoryPtr->data(0, Qt::UserRole + 2).toString() == "Application")
         {
            for (int index = 0; index < categoryPtr->childCount(); ++index)
            {
               auto* itemPtr = categoryPtr->child(index);
               // 二次开发（i18n）：页面标题翻译后文本比较失配，改用控件 objectName 键（UserRole+1）比较
               if (itemPtr->data(0, Qt::UserRole + 1).toString() == "WkfGeneralPrefWidget")
               {
                  mUI.treeWidget->setCurrentItem(itemPtr);
                  break;
               }
            }
            categoryPtr->setExpanded(true);
            break;
         }
      }

      mUI.treeWidget->resizeColumnToContents(0);

      // Connect signals to SavePreferences AFTER dialog is constructed to ensure correct order of slot execution
      connect(this, &QDialog::accepted, this, &PreferencesDialog::SavePreferences);
      connect(mUI.buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::SavePreferences);
      connect(mUI.buttonBox->button(QDialogButtonBox::RestoreDefaults),
              &QPushButton::clicked,
              this,
              &PreferencesDialog::SavePreferences);
   }
}

void wkf::PreferencesDialog::SavePreferences()
{
   emit wkfEnv.GetMainWindow()->GetConfigurationObject()->SaveSettings(wkfEnv.GetUserSettingsFile(),
                                                                       wkfEnv.GetSharedSettingsFile());
   Observer::SaveSettings(wkfEnv.GetUserSettingsFile());
}

void wkf::PreferencesDialog::AddPrefWidgets(const QList<PrefWidget*>& aPrefWidgetList)
{
   auto SetScrollAreaInfo = [&](PrefWidget* aPrefWidget, QScrollArea* aScrollArea, QTreeWidgetItem* aTreeWidgetItem)
   {
      aScrollArea->setObjectName(aPrefWidget->objectName());
      aScrollArea->setWidget(aPrefWidget);
      aScrollArea->setWidgetResizable(true);
      aScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      aScrollArea->horizontalScrollBar()->setEnabled(false);
      aScrollArea->setMinimumWidth(aPrefWidget->minimumSizeHint().width());
      aTreeWidgetItem->setData(0, Qt::UserRole, mUI.stackedWidget->addWidget(aScrollArea));
      // 二次开发（i18n）：存控件 objectName 作为查找键（页面标题已走翻译通道显示，文本比较会失配）
      aTreeWidgetItem->setData(0, Qt::UserRole + 1, aPrefWidget->objectName());
      ConnectWidget(aPrefWidget);
   };

   for (const auto prefWidget : aPrefWidgetList)
   {
      QString name       = prefWidget->windowTitle();
      QString category   = prefWidget->GetCategoryHint();
      bool    isPriority = prefWidget->GetCategoryHintPriority();

      // If no category is provide, create a top-level item with the name
      if (category.isEmpty())
      {
         QTreeWidgetItem* topLevelItemPtr = new QTreeWidgetItem(QStringList(name));
         mUI.treeWidget->insertTopLevelItem(0, topLevelItemPtr);
         SetScrollAreaInfo(prefWidget, new QScrollArea(this), topLevelItemPtr);
      }
      else // Category is provided
      {
         bool newCategory = false;

         QTreeWidgetItem* categoryItemPtr = nullptr;
         // Check to see if an item with the specified category already exists
         for (int i = 0; i < mUI.treeWidget->topLevelItemCount(); ++i)
         {
            auto* topLevelItemPtr = mUI.treeWidget->topLevelItem(i);
            // 二次开发（i18n）：分类标签翻译后文本比较失配，改用分类键（UserRole+2）比较
            if (topLevelItemPtr->data(0, Qt::UserRole + 2).toString() == category)
            {
               categoryItemPtr = topLevelItemPtr;
               break;
            }
         }
         // An item with the specified category was not found, so create it
         if (categoryItemPtr == nullptr)
         {
            newCategory = true;
            // 二次开发（i18n）：分类标签走翻译通道显示（"Category" 专用上下文），
            // 分类键（英文）存 UserRole+2 供查找比较（查找通道与显示通道分离）
            categoryItemPtr = new QTreeWidgetItem(
               QStringList(QCoreApplication::translate("Category", category.toUtf8().constData())));
            categoryItemPtr->setData(0, Qt::UserRole + 2, category);
            mUI.treeWidget->insertTopLevelItem(0, categoryItemPtr);
         }

         // Create the entry for this pref widget with the category item as the parent
         QTreeWidgetItem* childItem = new QTreeWidgetItem(categoryItemPtr, QStringList(name));
         categoryItemPtr->addChild(childItem);
         SetScrollAreaInfo(prefWidget, new QScrollArea(this), childItem);

         // Sort the elements within the category, then take the first child and
         // make sure that widget is shown when the category item is selected.
         categoryItemPtr->sortChildren(0, Qt::AscendingOrder);

         // If the childItem/Pref Widget is marked as the priority Pref Widget, make the category Item
         // show the Pref Widget when category is selected
         if (isPriority)
         {
            categoryItemPtr->setData(0, Qt::UserRole, childItem->data(0, Qt::UserRole));
            // Mark that a priority Pref Widget was set.
            categoryItemPtr->setData(0, Qt::UserRole + 1, true);
         }
         // If this is a new Category and the childItem was not priority
         // OR
         // If a priority childItem has not been set
         else if (newCategory || !categoryItemPtr->data(0, Qt::UserRole + 1).toBool())
         {
            // we want the Category to show the first item in the list
            categoryItemPtr->setData(0, Qt::UserRole, categoryItemPtr->child(0)->data(0, Qt::UserRole));
         }
      }
   }
}

void wkf::PreferencesDialog::ConnectWidget(PrefWidget* aPrefWidgetPtr)
{
   connect(this, &QDialog::accepted, aPrefWidgetPtr, &PrefWidget::ApplyChanges);
   connect(mUI.buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, aPrefWidgetPtr, &PrefWidget::ApplyChanges);
   connect(this, &QDialog::rejected, aPrefWidgetPtr, &PrefWidget::RejectChanges);
}
