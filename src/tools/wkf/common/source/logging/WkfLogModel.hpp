// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2020 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************
#ifndef WKF_LOG_MODEL_HPP
#define WKF_LOG_MODEL_HPP

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "UtLogMessage.hpp"
// 二次开发（i18n）：SetChineseDisplay 跨 DLL（wizard_core）调用，须从 wkf_common 导出
#include "wkf_common_export.h"

namespace wkf
{
namespace log
{
constexpr int cMSG_TYPE_ROLE     = Qt::UserRole;     // Stores a QStringList
constexpr int cMSG_SEVERITY_ROLE = Qt::UserRole + 1; // Stores an int

// 二次开发（i18n）：设置日志界面是否按中文显示消息行（默认 false 显示英文）。
// 由应用启动时按语言偏好注入（如 wizard 的 Runner 在 zh_CN 时开启）；
// wkf 层不读取任何全局语言配置，保持与具体应用解耦。
WKF_COMMON_EXPORT void SetChineseDisplay(bool aEnabled) noexcept;

//! The MasterModel contains data on every ut::log::Message received.
//! The FilterModel (see below) sorts and filters these messages for the user.
class MasterModel : public QObject
{
   Q_OBJECT

public:
   MasterModel();

   ~MasterModel() override;

   //! Queues aMessage to be added to the model.
   //! Messages will be added to the model when @see ApplyQueuedMessages is called.
   void QueueMessage(const ut::log::Message& aMessage) noexcept;

   //! Adds all queued messages to the model.
   void ApplyQueuedMessages() noexcept;

   //! Clears the model and message queue.
   void ClearMessages() noexcept;

   QAbstractItemModel* GetModel() noexcept;

signals:
   //! Indicates that the error or warning counts should be updated.
   //! The arguments are the new totals, not the deltas.
   void ErrorCountChanged(int aNewTotal);
   void WarningCountChanged(int aNewTotal);

private:
   void AddMessageTo(const ut::log::Message& aMessage, QStandardItem& aParent) noexcept;

   //! Returns a newly allocated model item.
   QStandardItem* MakeItem(const ut::log::Message& aMessage) const noexcept;

   QStandardItemModel    mBaseModel;
   QList<QStandardItem*> mQueue;

   int mErrorCount   = 0;
   int mWarningCount = 0;
};

//! Indicates the different ways the user can filter and sort the messages they view.
struct FilterSettings
{
   QStringList mTypes;

   enum class Sorting
   {
      Chronological,
      Type,
      Alphabetical
   } mSorting           = Sorting::Chronological;
   bool mReverseSorting = false;

   QString mFind;
};

//! Filters the messages in the MasterModel for the user.
class FilterModel final : public QSortFilterProxyModel
{
   Q_OBJECT

public:
   //! aMaster is the model to filter.
   FilterModel(MasterModel& aMaster) noexcept;

   void AddType(QString aType) noexcept;
   void RemoveType(const QString& aType) noexcept;

   void SetSortingMethod(FilterSettings::Sorting aSorting) noexcept;
   void SetSortingReversed(bool aReverse) noexcept;

   void Find(QString aString) noexcept;

private:
   bool filterAcceptsRow(int aRow, const QModelIndex& aParent) const override;

   bool lessThan(const QModelIndex& aLeft, const QModelIndex& aRight) const override;

   bool SearchChildren(const QModelIndex& aParent) const noexcept;

   int         GetSeverity(int aRow) const noexcept;
   QString     GetText(int aRow) const noexcept;
   QStringList GetTypes(int aRow) const noexcept;

   //! If mSettings.mReverseSorting is enabled, reverses aRetval.
   //! This is here to improve readability.
   bool Reverse(bool aRetval) const noexcept;

   FilterSettings mSettings;
};
} // namespace log
} // namespace wkf

#endif
