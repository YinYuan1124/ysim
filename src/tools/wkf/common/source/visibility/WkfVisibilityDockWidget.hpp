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

#ifndef WKFVISIBILITYDOCKWIDGET_HPP
#define WKFVISIBILITYDOCKWIDGET_HPP

#include "wkf_common_export.h"

#include <QDockWidget>

#include "WkfTeamVisibilityWidget.hpp"
#include "WkfTypeVisibilityWidget.hpp"
#include "ui_WkfVisibilityDockWidget.h"

namespace wkf
{
class WKF_COMMON_EXPORT VisibilityDockWidget : public QDockWidget
{
   Q_OBJECT // 二次开发（i18n）：补 Q_OBJECT 使 tr() 上下文命中本类
public:
   VisibilityDockWidget(QWidget* aParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
   void Update();

protected:
   void VisibilityWidgetChanged(int aIndex);

   Ui::WkfVisibilityDockWidget mUI;
   TeamVisibilityWidget*       mTeamWidget;
   TypeVisibilityWidget*       mTypeWidget;
};
} // namespace wkf
#endif // !VISIBILITYDOCKWIDGET_HPP
