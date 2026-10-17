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

#ifndef RvStartupDialog_HPP
#define RvStartupDialog_HPP

#include "RvExport.hpp"
#include "WkfStartupDialog.hpp"

#include <QCoreApplication> // 二次开发（i18n）：类无 Q_OBJECT，显式上下文翻译所需

namespace rv
{
class RV_EXPORT StartupDialog : public wkf::StartupDialog<QString>
{
public:
   StartupDialog(QString& aRecordingFile, QWidget* aParent = nullptr, Qt::WindowFlags aFlags = Qt::WindowFlags());

private:
   QString               Browse() override;
   void                  RemoveRecent(const QString& aRecent) override;
   void                  ClearRecents() override;
   const QList<QString>& GetRecents() const override;
   QString               GetRecentLabelDisplayName(const QString& aFile, bool aDisplayFullPath) const override;
   QString               RunActionText() const override
   {
      return QCoreApplication::translate("rv::StartupDialog", "Open Recording"); // 二次开发（i18n）：接入翻译通道
   }
   void                  PopulateLabels() override;

   bool    ValidateSelection() override;
   QString ParseDroppedUrls(const QList<QUrl>& urls) const override;
};
} // namespace rv
#endif // RvStartupDialog_HPP
