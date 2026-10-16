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
// ****************************************************************************
// Updated by Infoscitex, a DCS Company.
// ****************************************************************************

#include "dis/WsfDisAcknowledgeR.hpp"

#include "Dis.hpp"
#include "UtLog.hpp"
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
#include "WsfDisObserver.hpp"
#include "dis/WsfDisUtil.hpp"

WsfDisAcknowledgeR::WsfDisAcknowledgeR(WsfDisInterface* aInterfacePtr)
   : DisAcknowledgeR()
   , mInterfacePtr(aInterfacePtr)
{
   SetOriginatingEntity(DisEntityId(Dis::GetSite(), Dis::GetApplication(), 0));
}

WsfDisAcknowledgeR::WsfDisAcknowledgeR(DisPdu& aPdu, GenI& aGenI, WsfDisInterface* aInterfacePtr)
   : DisAcknowledgeR(aPdu, aGenI)
   , mInterfacePtr(aInterfacePtr)
{
}

// virtual
WsfDisAcknowledgeR::~WsfDisAcknowledgeR() {}

// virtual
void WsfDisAcknowledgeR::TimedOut()
{
   if (WsfDisUtil::DebugEnabled(mInterfacePtr))
   {
      // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
      const ut::msg::Args args{{"cls", "WsfDisAcknowledgeR"}};
      ut::msg::EmitWarning(ut::msg::MSG_W_TIMEDOUT_UNHANDLED, args);
   }
}

// virtual
int WsfDisAcknowledgeR::Process()
{
   const int DELETE_PDU = 1;
   WsfObserver::DisAcknowledgeRReceived (&WsfDisUtil::GetSimulation(mInterfacePtr))(mInterfacePtr, *this);
   return DELETE_PDU;
}
