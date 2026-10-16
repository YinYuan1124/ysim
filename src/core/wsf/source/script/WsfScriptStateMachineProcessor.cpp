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

#include "script/WsfScriptStateMachineProcessor.hpp"

#include "UtInput.hpp"
#include "UtLog.hpp"
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
#include "WsfPlatform.hpp"
#include "script/WsfScriptContext.hpp"
#include "script/WsfScriptMessageHandler.hpp"

WsfScriptStateMachineProcessor::WsfScriptStateMachineProcessor(WsfScenario& aScenario)
   : WsfScriptProcessor(aScenario)
   , mStateMachine(*mContextPtr, this)
   , mStateMachineVarPtr(nullptr)
{
   mStateMachineVarPtr = mContextPtr->GetContext().RegisterVariable("STATE_MACHINE", "WsfStateMachine");
}

WsfScriptStateMachineProcessor::WsfScriptStateMachineProcessor(const WsfScriptStateMachineProcessor& aSrc)
   : WsfScriptProcessor(aSrc)
   , mStateMachine(aSrc.mStateMachine, *mContextPtr, this)
   , mStateMachineVarPtr(nullptr)
{
   mStateMachineVarPtr = mContextPtr->GetContext().RegisterVariable("STATE_MACHINE", "WsfStateMachine");
}

// virtual
WsfScriptStateMachineProcessor::~WsfScriptStateMachineProcessor() {}

// virtual
WsfProcessor* WsfScriptStateMachineProcessor::Clone() const
{
   return new WsfScriptStateMachineProcessor(*this);
}

// virtual
int WsfScriptStateMachineProcessor::Evaluate(int aCurrentState)
{
   return mStateMachine.Evaluate(aCurrentState);
}

// virtual
bool WsfScriptStateMachineProcessor::Initialize(double aSimTime)
{
   bool ok = WsfScriptProcessor::Initialize(aSimTime);
   ok &= mStateMachine.Initialize(aSimTime);

   if (mStateMachineVarPtr == nullptr)
   {
      // 二次开发（i18n）：错误消息 ID 化 —— 无运行时参数
      ut::log::error() << ut::msg::FormatEnTagged(ut::msg::MSG_SCRIPT_SMP_INVALID_INIT, {});
      return false;
   }
   mStateMachineVarPtr->GetPointer()->SetAppObject(static_cast<void*>(&mStateMachine), GetPlatform()->GetReferenceCount());
   return ok;
}

// virtual
bool WsfScriptStateMachineProcessor::ProcessInput(UtInput& aInput)
{
   bool myCommand = true;
   if (mStateMachine.ProcessInput(aInput))
   {
   }
   else
   {
      myCommand = WsfScriptProcessor::ProcessInput(aInput);
   }
   return myCommand;
}

//! Invoke the 'on_message' handler if one has been defined.
//! @param aSimTime The current simulation time.
//! @param aMessage The message being processed.
//! @returns 'true' if the message was processed or 'false' if not.
// virtual
bool WsfScriptStateMachineProcessor::ProcessMessage(double aSimTime, const WsfMessage& aMessage)
{
   return mMessageHandlerPtr->ProcessMessage(aSimTime, aMessage);
}

//! Invoke the 'on_update' script if one has been defined.
//! @param aSimTime The current simulation time.
// virtual
void WsfScriptStateMachineProcessor::Update(double aSimTime)
{
   WsfScriptProcessor::Update(aSimTime);
}
