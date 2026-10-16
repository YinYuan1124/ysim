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

#include "dis/WsfDisSetRecordR.hpp"

#include "Dis.hpp"
#include "DisIO.hpp"
#include "DisPduEnums.hpp"
#include "UtLog.hpp"
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
#include "WsfDisObserver.hpp"
#include "WsfSimulation.hpp"
#include "dis/WsfDisRecordR.hpp"
#include "dis/WsfDisUtil.hpp"

WsfDisSetRecordR::WsfDisSetRecordR(WsfDisInterface* aInterfacePtr)
   : DisSetRecordR()
   , mInterfacePtr(aInterfacePtr)
{
   SetReliabilityService(WsfDisUtil::GetReliabilityService(mInterfacePtr));
   SetRequestId(WsfDisUtil::GetNextRequestNumber(mInterfacePtr)); // Serialized identifier handle.
   SetOriginatingEntity(DisEntityId(Dis::GetSite(), Dis::GetApplication(), 0));
}

WsfDisSetRecordR::WsfDisSetRecordR(DisPdu& aPdu, GenI& aGenI, WsfDisInterface* aInterfacePtr)
   : DisSetRecordR(aPdu, aGenI)
   , mInterfacePtr(aInterfacePtr)
{
}

// virtual
WsfDisSetRecordR::~WsfDisSetRecordR() {}

// virtual
void WsfDisSetRecordR::TimedOut()
{
   if (WsfDisUtil::DebugEnabled(mInterfacePtr))
   {
      // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
      const ut::msg::Args args{{"cls", "WsfDisSetRecordR"}};
      ut::msg::EmitWarning(ut::msg::MSG_W_TIMEDOUT_UNHANDLED, args);
   }
}

WsfDisRecordR* WsfDisSetRecordR::ProduceReply()
{
   WsfDisRecordR* replyPtr = new WsfDisRecordR(mInterfacePtr);

   replyPtr->SetRequestId(GetRequestId());
   replyPtr->SetReceivingEntity(GetOriginatingEntity());

   // Per the DIS Standard, the response shall NOT require acknowledgment.
   replyPtr->SetReliabilityService(DisEnum::Pdu::Reliability::Unacknowledged);

   PopulateReply(replyPtr);

   return replyPtr;
}

bool WsfDisSetRecordR::PopulateReply(WsfDisRecordR* aReplyPtr)
{
   bool processed = false;
   if (WsfDisUtil::DebugEnabled(mInterfacePtr))
   {
      // For now, our only response is to indicate we cannot respond.
      // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
      ut::msg::EmitWarning(ut::msg::MSG_W_SETR_POPULATE_NOT_IMPL);
      processed = true;
   }
   return processed;
}

// virtual
int WsfDisSetRecordR::Process()
{
   const int DELETE_PDU = 1;

   // bool actionNeeded = false;
   bool satisfied = false;
   bool repliedTo = false;

   WsfObserver::DisSetRecordRReceived (&WsfDisUtil::GetSimulation(mInterfacePtr))(mInterfacePtr, *this);

   if (WsfDisUtil::DebugEnabled(mInterfacePtr))
   {
      double simTime2 = WsfDisUtil::GetSimTime(mInterfacePtr);
      double pduTime  = WsfDisUtil::GetTimeFromPdu(mInterfacePtr, *this);

      auto out = ut::log::debug() << "WsfDisSetRecordR: PDU received and processed.";
      out.AddNote() << "Sim T = " << simTime2;
      out.AddNote() << "PDU T = " << pduTime;

      if (satisfied)
      {
         out.AddNote() << "Satisfied.";
      }
      if (repliedTo)
      {
         out.AddNote() << "Reply sent.";
      }
      if (satisfied && repliedTo)
      {
         out.AddNote() << "Complete Transaction.";
      }
   }

   return DELETE_PDU;
}

bool WsfDisSetRecordR::ProcessSetRecordRequest()
{
   bool success = false;
   if (WsfDisUtil::DebugEnabled(mInterfacePtr))
   {
      // Our only action currently is to indicate we can't do this yet.
      // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
      ut::msg::EmitWarning(ut::msg::MSG_W_SETR_NOT_PROCESSING);
      success = true;
   }
   return success;
}
