// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2017 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "WkfSharedMapPreferencesWidget.hpp"

#include <QComboBox>
#include <QCoreApplication> // 二次开发（i18n）：MapDisplayName 动态翻译所需
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <osgDB/ReadFile>
#include <osgEarth/MapNode>

#include "WkfEnvironment.hpp"
#include "WkfSharedMapPreferencesObject.hpp"

// 二次开发（i18n）：地图名为持久化数据键（QSettings/ProfileMap 均存英文原名），显示与键分离——
// 键保持英文原样，渲染点经 "MapName" 伪上下文动态翻译为显示名；用户自定义地图名（无译文）原样显示。
static QString MapDisplayName(const QString& aKey)
{
   return QCoreApplication::translate("MapName", aKey.toUtf8().constData());
}


wkf::SharedMapPreferencesWidget::SharedMapPreferencesWidget(QWidget* aParent, Qt::WindowFlags aFlags)
   : wkf::PrefWidgetT<wkf::SharedMapPreferencesObject>(aParent, aFlags)
{
   if (objectName().isEmpty())
   {
      setObjectName("WkfSharedMapPreferencesWidget");
   }
   // 二次开发（i18n）：首选项页面标题与控件文本 tr() 化
   setWindowTitle(tr("Map Definitions"));
   setLayout(new QVBoxLayout(this));

   auto* label     = new QLabel(tr("Preferences on this page are shared among all AFSIM applications"), this);
   QFont labelFont = label->font();
   labelFont.setBold(true);
   label->setFont(labelFont);
   layout()->addWidget(label);

   QHBoxLayout* hbox = new QHBoxLayout();
   layout()->addItem(hbox);
   hbox->addWidget(new QLabel(tr("Maps"), this));
   QPushButton* newMap = new QPushButton(tr("Add New Map"), this);
   connect(newMap, &QPushButton::clicked, this, &SharedMapPreferencesWidget::AddNewMap);
   layout()->addWidget(newMap);
   hbox->addWidget(newMap);

   mMapTreePtr = new QTreeWidget(this);
   layout()->addWidget(mMapTreePtr);

   layout()->addWidget(new QLabel(tr("Profiles"), this));

   mProfileTreePtr = new QTreeWidget(this);
   layout()->addWidget(mProfileTreePtr);

   mMapTreePtr->setContextMenuPolicy(Qt::CustomContextMenu);
   mMapTreePtr->setSortingEnabled(false);
   connect(mMapTreePtr, &QWidget::customContextMenuRequested, this, &wkf::SharedMapPreferencesWidget::ContextMenuEvent);

   mProfileTreePtr->setColumnCount(2);
   mProfileTreePtr->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
   mProfileTreePtr->header()->close();
   mProfileTreePtr->clear();
   mMapTreePtr->setColumnCount(1);
   mMapTreePtr->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
   mMapTreePtr->header()->close();
   mMapTreePtr->clear();
}

//! @details this override does a strange thing in the fact that it saves
//! the maps before calling the PrefObject's RestoreDefaults function to
//! restore the default map profiles.
void wkf::SharedMapPreferencesWidget::RestoreDefaults()
{
   // store off the map data
   SharedMapPreferencesData data;
   WritePreferenceData(data);
   mPrefObjectPtr->SetPreferenceData(QVariant::fromValue(data));

   // restore the profiles
   mPrefObjectPtr->RestoreDefaults();
}

void wkf::SharedMapPreferencesWidget::WritePreferenceData(SharedMapPreferencesData& aPrefData)
{
   aPrefData.mMaps.clear();
   for (int i = 0; i < mMapTreePtr->invisibleRootItem()->childCount(); ++i)
   {
      QTreeWidgetItem* map     = mMapTreePtr->invisibleRootItem()->child(i);
      QString          name    = map->data(0, Qt::UserRole + 1).value<QString>();
      auto&            mapData = aPrefData.mMaps[name.toStdString()];

      mapData.mProjection = map->data(0, Qt::UserRole + 2).value<QString>().toStdString();
      mapData.mLatOffset  = map->data(0, Qt::UserRole + 3).toFloat();
      mapData.mLock       = map->data(0, Qt::UserRole + 4).toBool();

      if (map->childCount() > 0)
      {
         mapData.mFile = map->child(0)->text(0).toStdString();
      }
   }
   aPrefData.mProfileMap.clear();
   for (int i = 0; i < mProfileTreePtr->invisibleRootItem()->childCount(); ++i)
   {
      QTreeWidgetItem* profile = mProfileTreePtr->invisibleRootItem()->child(i);
      auto             type =
         static_cast<SharedMapPreferencesData::MapType>(profile->data(0, Qt::UserRole + 1).value<int>()); // profile idx
      QComboBox* combo = dynamic_cast<QComboBox*>(mProfileTreePtr->itemWidget(profile, 1));
      // 二次开发（i18n）：取 itemData 中的英文键（currentText 显示名翻译后为中文，不可作持久化键）
      QString    name  = combo->currentData().toString();
      aPrefData.mProfileMap.emplace(type, name.toStdString());
   }
}

void wkf::SharedMapPreferencesWidget::ReadPreferenceData(const SharedMapPreferencesData& aPrefData)
{
   QTreeWidgetItem* child;
   while ((child = mMapTreePtr->invisibleRootItem()->takeChild(0)) != nullptr)
   {
      delete child;
   }
   for (auto& it : aPrefData.mMaps)
   {
      QString          name = it.first.c_str();
      // 二次开发（i18n）：树显示名经 MapDisplayName 翻译；键（UserRole+1）保持英文原文
      QTreeWidgetItem* map  = new QTreeWidgetItem(mMapTreePtr->invisibleRootItem(), QStringList(MapDisplayName(name)));
      if (it.second.mLock)
      {
         QFont font = map->font(0);
         font.setItalic(true);
         map->setFont(0, font);
      }
      map->setData(0, Qt::UserRole, cMAP);
      map->setData(0, Qt::UserRole + 1, QString(it.first.c_str()));
      map->setData(0, Qt::UserRole + 2, QString(it.second.mProjection.c_str()));
      map->setData(0, Qt::UserRole + 3, it.second.mLatOffset);
      map->setData(0, Qt::UserRole + 4, it.second.mLock);
      map->setToolTip(0, it.second.mFile.c_str());
      if (!it.second.mFile.empty())
      {
         QTreeWidgetItem* file = new QTreeWidgetItem(map, QStringList(it.second.mFile.c_str()));
         file->setData(0, Qt::UserRole, cMAPFILE);
         file->setHidden(true);
      }
   }
   UpdateProfiles();
}

void wkf::SharedMapPreferencesWidget::UpdateProfiles()
{
   const wkf::SharedMapPreferencesData& prefs = mPrefObjectPtr->GetPreferences();
   QTreeWidgetItem*                     child;
   while ((child = mProfileTreePtr->invisibleRootItem()->takeChild(0)) != nullptr)
   {
      delete child;
   }

   for (auto& it : prefs.mProfileMap)
   {
      wkf::SharedMapPreferencesData::MapType ptype = it.first;
      QString                                map   = it.second.c_str();
      QTreeWidgetItem*                       profile =
         new QTreeWidgetItem(mProfileTreePtr->invisibleRootItem(), QStringList(ProfileTypeLookup(ptype)));
      profile->setData(0, Qt::UserRole + 1, ptype);

      QComboBox* combo = new QComboBox(this);
      mProfileTreePtr->setItemWidget(profile, 1, combo);

      // 二次开发（i18n）：组合框双通道——显示名经 MapDisplayName 翻译、itemData 存英文键；
      // 恢复用 findData(键) 匹配（原 setCurrentText 在显示名翻译后会与英文键失配）
      for (int i = 0; i < mMapTreePtr->invisibleRootItem()->childCount(); ++i)
      {
         QString key = mMapTreePtr->invisibleRootItem()->child(i)->data(0, Qt::UserRole + 1).toString();
         combo->addItem(MapDisplayName(key));
         combo->setItemData(i, key);
      }
      combo->setCurrentIndex(combo->findData(map));
   }
}

QString wkf::SharedMapPreferencesWidget::ProfileTypeLookup(wkf::SharedMapPreferencesData::MapType aType)
{
   // 二次开发（i18n）：Profile 树项显示文本 tr() 化（default 分支为不可达防御代码，保持原样）
   switch (aType)
   {
   case (wkf::SharedMapPreferencesData::IMMERSIVE):
      return tr("Immersive");
      break;
   case (wkf::SharedMapPreferencesData::NAVIGATIONAL):
      return tr("Navigational");
      break;
   default:
      return "UNKNOWN";
      break;
   }
}

void wkf::SharedMapPreferencesWidget::ContextMenuEvent(const QPoint& aPoint)
{
   QTreeWidgetItem* item = mMapTreePtr->itemAt(aPoint);
   if (item != nullptr)
   {
      int role = item->data(0, Qt::UserRole).toInt();
      switch (role)
      {
      case (cMAP):
      {
         if (!item->data(0, Qt::UserRole + 4).toBool())
         {
            QMenu   popup(this);
            // 二次开发（i18n）：右键菜单动作文本 tr() 化（比较用同一变量，译文一致不失配）
            QString renameMap = tr("Rename");
            QString removemap = tr("Remove");
            popup.addAction(renameMap);
            popup.addAction(removemap);
            QAction* action = popup.exec(mMapTreePtr->mapToGlobal(aPoint));
            if (nullptr != action)
            {
               if (action->text() == renameMap)
               {
                  RenameMap(item);
               }
               else if (action->text() == removemap)
               {
                  RemoveMap(item);
               }
            }
         }
         break;
      }
      default:
         break;
      }
   }
}


void wkf::SharedMapPreferencesWidget::AddNewMap()
{
   // ask for the map file
   // 二次开发（i18n）：文件对话框标题与过滤器 tr() 化
   QString fileName = wkf::getOpenFileName(this, tr("Open Map File"), QString(), tr("Map Files (*.ive *.osg *.osgb *.earth)"));
   if (!fileName.isEmpty())
   {
      osg::Node* file = osgDB::readNodeFile(fileName.toStdString());
      if (file)
      {
         osgEarth::MapNode* mn = dynamic_cast<osgEarth::MapNode*>(file);
         if (mn) // if it is osgearth
         {
            if (mn->getMap())
            {
               std::string sname = mn->getMap()->getName();
               // try to determine the projection parameters.. add a right click rename option
               float lonOffset  = 0.0f;
               bool  geocentric = mn->getMap()->isGeocentric();
               if (!geocentric)
               {
                  const osgEarth::SpatialReference* srs = mn->getMap()->getSRS();
                  if (srs)
                  {
                     QString qwkt = srs->getWKT().c_str();
                     int     idx  = qwkt.indexOf("PARAMETER[\"central_meridian\",");
                     if (idx > 0)
                     {
                        idx += 29;
                        int     endidx = qwkt.indexOf("]", idx);
                        QString substr = qwkt.mid(idx, endidx - idx);
                        lonOffset      = substr.toFloat();
                     }
                  }
               }
               QDialog     dialog(this);
               QFormLayout form(&dialog);
               QLineEdit   nameEdit(&dialog);
               nameEdit.setText(sname.c_str());
               form.addRow(new QLabel(tr("Map Name:")), &nameEdit);

               QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
               form.addRow(&buttonBox);
               QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
               QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

               if (dialog.exec() == QDialog::Accepted)
               {
                  sname = nameEdit.text().toStdString();
                  if (!sname.empty())
                  {
                     QString          name = GetUniqueName(sname.c_str());
                     // 二次开发（i18n）：显示名经 MapDisplayName（自定义名无译文原样显示）
                     QTreeWidgetItem* map  = new QTreeWidgetItem(mMapTreePtr->invisibleRootItem(), QStringList(MapDisplayName(name)));
                     map->setData(0, Qt::UserRole, cMAP);
                     map->setData(0, Qt::UserRole + 1, name);
                     if (!geocentric)
                     {
                        map->setData(0, Qt::UserRole + 2, QString("CME_EQR"));
                     }
                     else
                     {
                        map->setData(0, Qt::UserRole + 2, QString("CME_WCS"));
                     }
                     map->setData(0, Qt::UserRole + 3, lonOffset);
                     map->setData(0, Qt::UserRole + 4, false);
                     map->setToolTip(0, fileName);

                     QTreeWidgetItem* file = new QTreeWidgetItem(map, QStringList(fileName));
                     file->setData(0, Qt::UserRole, cMAPFILE);
                     file->setHidden(true);
                  }
               }

               // make it with name, geocentric, and lonOffset
               file = 0;
               UpdateProfiles();
               return;
            }
            else
            {
               // 二次开发（i18n）：警告框标题与文本 tr() 化
               QMessageBox::warning(this, tr("File not usable"), tr("OsgEarth file does not contain a map: ") + fileName);
               file = 0;
               return;
            }
         }
         QDialog     dialog(this);
         QFormLayout form(&dialog);
         QLineEdit   nameEdit(&dialog);
         form.addRow(new QLabel(tr("Map Name:")), &nameEdit);
         QComboBox projection(&dialog);
         // 二次开发（i18n）：组合框项 tr() 化；改用索引设置/判断，避免项文本翻译后 currentText 比较失配
         projection.addItem(tr("Round"));
         projection.addItem(tr("Flat"));
         projection.setCurrentIndex(0);
         form.addRow(new QLabel(tr("Projection:")), &projection);
         QSpinBox offset(&dialog);
         offset.setMinimum(-180);
         offset.setMaximum(180);
         offset.setValue(0);
         form.addRow(new QLabel(tr("Lon. Offset (deg):")), &offset);
         QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
         form.addRow(&buttonBox);
         QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
         QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

         if (dialog.exec() == QDialog::Accepted)
         {
            QString name = nameEdit.text();

            if (!name.isEmpty())
            {
               name = GetUniqueName(name);

               QTreeWidgetItem* map = new QTreeWidgetItem(mMapTreePtr->invisibleRootItem(), QStringList(name));
               map->setData(0, Qt::UserRole, cMAP);
               map->setData(0, Qt::UserRole + 1, name);
               // 二次开发（i18n）：组合框项翻译后文本比较失配，改用索引判断（Round=0/Flat=1）
               if (projection.currentIndex() == 1)
               {
                  map->setData(0, Qt::UserRole + 2, QString("CME_EQR"));
               }
               else
               {
                  map->setData(0, Qt::UserRole + 2, QString("CME_WCS"));
               }
               map->setData(0, Qt::UserRole + 3, offset.value());
               map->setData(0, Qt::UserRole + 4, false);
               map->setToolTip(0, fileName);

               QTreeWidgetItem* file = new QTreeWidgetItem(map, QStringList(fileName));
               file->setData(0, Qt::UserRole, cMAPFILE);
               file->setHidden(true);
            }
         }
         UpdateProfiles();
         // if no projection parameters, ask the user for that information and make it
         file = 0;
         return;
      }
      // 二次开发（i18n）：警告框标题与文本 tr() 化
      QMessageBox::warning(this, tr("File not usable"), tr("Could not load ") + fileName);
   }
}

void wkf::SharedMapPreferencesWidget::RemoveMap(QTreeWidgetItem* aMapItem)
{
   QString text = aMapItem->data(0, Qt::UserRole + 1).toString();
   mMapTreePtr->invisibleRootItem()->removeChild(aMapItem);
   if (mMapTreePtr->invisibleRootItem()->childCount() == 0)
   {
      QString name      = "None";
      QString identname = "    ";
      identname += MapDisplayName(name); // 二次开发（i18n）：显示名翻译，键保持英文
      QTreeWidgetItem* map  = new QTreeWidgetItem(mMapTreePtr->invisibleRootItem(), QStringList(identname));
      QFont            font = map->font(0);
      font.setItalic(true);
      map->setFont(0, font);
      map->setData(0, Qt::UserRole, cMAP);
      map->setData(0, Qt::UserRole + 1, name);
      map->setData(0, Qt::UserRole + 2, QString("CME_WCS"));
      map->setData(0, Qt::UserRole + 3, 0.0f);
      map->setData(0, Qt::UserRole + 4, true);
   }
   UpdateProfiles();
}

void wkf::SharedMapPreferencesWidget::RenameMap(QTreeWidgetItem* aItem)
{
   QDialog     dialog(this);
   QFormLayout form(&dialog);
   QLineEdit   nameEdit(&dialog);
   nameEdit.setText(aItem->data(0, Qt::UserRole + 1).toString());
   form.addRow(new QLabel(tr("Map Name:")), &nameEdit);

   QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
   form.addRow(&buttonBox);
   QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
   QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

   if (dialog.exec() == QDialog::Accepted)
   {
      QString name = nameEdit.text();
      if (!name.isEmpty())
      {
         name = GetUniqueName(name);
         aItem->setText(0, MapDisplayName(name)); // 二次开发（i18n）：显示名翻译，键保持英文
         aItem->setData(0, Qt::UserRole + 1, name);
      }
   }
   UpdateProfiles();
}

QString wkf::SharedMapPreferencesWidget::GetUniqueName(const QString& aName)
{
   if (!NameIsUnique(aName))
   {
      int     idx  = 0;
      QString name = aName + "_" + QString::number(idx);
      while (!NameIsUnique(name))
      {
         idx++;
         name = aName + "_" + QString::number(idx);
      }
      return name;
   }
   return aName;
}

bool wkf::SharedMapPreferencesWidget::NameIsUnique(const QString& aName)
{
   int mapscount = mMapTreePtr->invisibleRootItem()->childCount();
   for (int i = 0; i < mapscount; ++i)
   {
      if (mMapTreePtr->invisibleRootItem()->child(i)->data(0, Qt::UserRole + 1).toString() == aName)
      {
         return false;
      }
   }
   return true;
}
