// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2003-2015 The Boeing Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef CONTEXTDOCUMENTATIONCONTROL_HPP
#define CONTEXTDOCUMENTATIONCONTROL_HPP
#include <QDockWidget>

namespace wizard
{
class BrowserWidget;

// 二次开发（i18n）：补充 Q_OBJECT 宏，使 tr() 拥有独立的翻译 context
class ContextDocumentationControl : public QDockWidget
{
   Q_OBJECT

public:
   explicit ContextDocumentationControl();
   ~ContextDocumentationControl() override = default;

   BrowserWidget* mBrowserPtr;
};
} // namespace wizard


#endif
