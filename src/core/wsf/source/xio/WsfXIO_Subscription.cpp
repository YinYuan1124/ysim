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

#include "xio/WsfXIO_Subscription.hpp"

#include "UtLog.hpp"
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
#include "xio/WsfXIO_Connection.hpp"
#include "xio/WsfXIO_PacketRegistry.hpp"

WsfXIO_Subscription::WsfXIO_Subscription(WsfXIO_Connection* aConnectionPtr)
   : mConnectionPtr(aConnectionPtr)
   , mUserCancelled(false)
{
}

WsfXIO_Subscription::WsfXIO_Subscription(WsfXIO_RequestDataPkt& aRequest)
   : mUserCancelled(false)
{
   mConnectionPtr = (WsfXIO_Connection*)aRequest.GetSender();
   mId.first      = mConnectionPtr->GetConnectionId();
   mId.second     = aRequest.mSubscriptionRequestId;
   if (aRequest.mReliable)
   {
      mDataConnectionPtr = mConnectionPtr;
   }
   else
   {
      mDataConnectionPtr = mConnectionPtr->GetLinkedConnection();
      if (mDataConnectionPtr == nullptr)
      {
         // 二次开发（i18n）：错误消息 ID 化 —— 无运行时参数
         ut::log::error() << ut::msg::FormatEnTagged(ut::msg::MSG_XIO_NO_LINKED_CONN, {});
         mDataConnectionPtr = mConnectionPtr;
      }
   }
}

WsfXIO_Subscription::~WsfXIO_Subscription() {}

void WsfXIO_Subscription::Initialize(WsfXIO_Service* aProviderPtr)
{
   mProviderPtr = aProviderPtr;
}

void WsfXIO_Subscription::SendResponse(WsfXIO_ResponsePkt& aPkt)
{
   aPkt.mFromProvider = true;
   aPkt.mRequestId    = GetRequestId();
   GetConnection().Send(aPkt);
}

WsfXIO_Interface& WsfXIO_Subscription::GetInterface() const
{
   return mConnectionPtr->GetInterface();
}
