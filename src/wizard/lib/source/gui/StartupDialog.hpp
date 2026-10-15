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

#ifndef StartupDialog_HPP
#define StartupDialog_HPP

#include "ViExport.hpp"
#include "WkfStartupDialog.hpp"

namespace wizard
{
// 二次开发（i18n）：补充 Q_OBJECT 宏，使 tr() 拥有独立的翻译 context；
// 缺失时 tr() 会退化为基类 wkf::StartupDialog 的 context，本类新译文无法命中。
class VI_EXPORT StartupDialog : public wkf::StartupDialog<QStringList>
{
   Q_OBJECT

public:
   StartupDialog(QStringList& aInputFiles, QWidget* aParent = nullptr, Qt::WindowFlags aFlags = Qt::WindowFlags());

private:
   QStringList               Browse() override;
   void                      RemoveRecent(const QStringList& aRecent) override;
   void                      ClearRecents() override;
   const QList<QStringList>& GetRecents() const override;
   QString GetRecentLabelDisplayName(const QStringList& aFileList, bool aDisplayFullPath) const override;
   // 二次开发（i18n）：启动按钮文本包裹 tr()
   QString RunActionText() const override { return tr("Open Project"); }
   void    BuildRecentContextMenu(QMenu& aMenu, const QStringList& aLink) override;
   void    PopulateLabels() override;

   bool        ValidateSelection() override;
   QStringList ParseDroppedUrls(const QList<QUrl>& urls) const override;
};
} // namespace wizard
#endif // StartupDialog_HPP
