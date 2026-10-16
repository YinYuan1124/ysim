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

#include "WsfEW_EA.hpp"

#include <algorithm>
#include <cassert>
#include <memory>

#include "UtInput.hpp"
#include "UtInputBlock.hpp"
#include "UtLog.hpp"
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
#include "WsfEW_Effect.hpp"
#include "WsfEW_FalseTargetEffect.hpp"
#include "WsfEW_XmtrComponent.hpp"
#include "WsfFalseTarget.hpp"
#include "WsfStringId.hpp"

// =================================================================================================
//! Convenience method to get the electronic attack object for the specified transmitter.
// static
WsfEW_EA_EP* WsfEW_EA::GetElectronicAttack(const WsfEM_Xmtr& aXmtr)
{
   WsfEW_XmtrComponent* componentPtr = WsfEW_XmtrComponent::Find(aXmtr);
   return (componentPtr != nullptr) ? componentPtr->GetElectronicAttack() : nullptr;
}

// =================================================================================================
//! Convenience method to set the electronic attack object for the specified transmitter.
// static
void WsfEW_EA::SetElectronicAttack(WsfEM_Xmtr& aXmtr, WsfEW_EA_EP* aElectronicAttackPtr)
{
   WsfEW_XmtrComponent* componentPtr = WsfEW_XmtrComponent::FindOrCreate(aXmtr);
   componentPtr->SetElectronicAttack(aElectronicAttackPtr);
}

// =================================================================================================
WsfEW_EA::WsfEW_EA()
   : WsfEW_EA_EP()
   , mTechniquePtrMap()
{
}

// =================================================================================================
WsfEW_EA::WsfEW_EA(const WsfEW_EA& aSrc)
   : WsfEW_EA_EP(aSrc)
   , mTechniquePtrMap()
{
   for (TechniqueIdToPtrMapConstIter techIter = aSrc.mTechniquePtrMap.begin(); techIter != aSrc.mTechniquePtrMap.end();
        ++techIter)
   {
      mTechniquePtrMap[techIter->second->GetNameId()] = dynamic_cast<EA_Technique*>(techIter->second->Clone());
   }
}

// =================================================================================================
// virtual
WsfEW_EA::~WsfEW_EA()
{
   for (unsigned int i = 0; i < mTechniques.size(); ++i)
   {
      TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(mTechniques[i]);
      if (techIter != mTechniquePtrMap.end())
      {
         delete techIter->second;
      }
   }
}

// =================================================================================================
// virtual
WsfEW_EA_EP* WsfEW_EA::Clone() const
{
   return new WsfEW_EA(*this);
}

// =================================================================================================
bool WsfEW_EA::Initialize(WsfSimulation& aSimulation, WsfEM_XmtrRcvr* aXmtrRcvrPtr)
{
   bool success = true;

   for (unsigned int techIndex = 0; techIndex != mTechniques.size(); ++techIndex)
   {
      WsfEW_Technique* techniquePtr = GetTechnique(mTechniques[techIndex]);
      techniquePtr->SetEA_EP_Ptr(this);
   }

   // NOTE: must have EA pointer variable already set
   // in the techniques before calling
   success &= WsfEW_EA_EP::Initialize(aSimulation, aXmtrRcvrPtr);

   return success;
}

bool WsfEW_EA::AddTechnique(WsfEW_Technique* aTechniquePtr)
{
   bool                     added(false);
   EA_Technique*            eaTechPtr(dynamic_cast<EA_Technique*>(aTechniquePtr));
   WsfEW_Types::TechniqueId techId(aTechniquePtr->GetNameId());
   if ((eaTechPtr != nullptr) && (mTechniquePtrMap.find(techId) == mTechniquePtrMap.end()))
   {
      mTechniquePtrMap[(aTechniquePtr->GetNameId())] = eaTechPtr;
      added                                          = WsfEW_EA_EP::AddTechnique(eaTechPtr);
   }
   return added;
}

WsfEW_Technique* WsfEW_EA::GetTechnique(WsfEW_Types::TechniqueId aTechniqueId) const
{
   EA_Technique*                       techPtr = nullptr;
   TechniqueIdToPtrMap::const_iterator idIter  = mTechniquePtrMap.find(aTechniqueId);
   if (idIter != mTechniquePtrMap.end())
   {
      techPtr = (*idIter).second;
   }
   return techPtr;
}

WsfEW_Technique* WsfEW_EA::GetTechnique(unsigned int aIndex) const
{
   EA_Technique* techPtr = nullptr;
   if (aIndex < mTechniques.size())
   {
      TechniqueIdToPtrMap::const_iterator idIter = mTechniquePtrMap.find(mTechniques[aIndex]);
      if (idIter != mTechniquePtrMap.end())
      {
         techPtr = (*idIter).second;
      }
   }
   return techPtr;
}

//! Selects a technique for use based on technique index for the specified System Type
//!
//! @param aIndex Specifies the index of the technique in the list (0 - size mTechniques-1).
//! @param aSystemFunction Indicates what system type that the effect will be applied to (i.e. Comms, Sensors, etc.).
//! @return Returns true if the technique was successfully selected.
// virtual
bool WsfEW_EA::SelectTechnique(unsigned int aIndex, WsfEW_Effect::SystemFunction aSystemFunction)
{
   if (aIndex < mTechniques.size())
   {
      TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(mTechniques[aIndex]);
      if (techIter != mTechniquePtrMap.end())
      {
         if (techIter->second->CanApplyTechnique(aSystemFunction))
         {
            if (PrivateSetTechnique(mTechniques[aIndex], true))
            {
               if (mDebug)
               {
                  auto out = ut::log::debug() << "Selected technique for system type.";
                  out.AddNote() << "Technique: " << techIter->second->GetName();
                  out.AddNote() << "System Type: " << aSystemFunction;
               }
            }
            else
            {
               if (mDebug)
               {
                  auto out = ut::log::debug() << "Selected technique for system type already selected.";
                  out.AddNote() << "Technique: " << techIter->second->GetName();
                  out.AddNote() << "System Type: " << aSystemFunction;
               }
            }
         }
         else
         {
            if (mDebug)
            {
               // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
               auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_SEL_APPLY_IDX);
               out.AddNote() << "Index: " << aIndex;
               out.AddNote() << "System Type: " << aSystemFunction;
            }
            return false;
         }
      }
      else
      {
         if (mDebug)
         {
            // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
            auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_SEL_NO_VALID_IDX);
            out.AddNote() << "Index: " << aIndex;
            out.AddNote() << "System Type: " << aSystemFunction;
         }
         return false;
      }
   }
   else
   {
      if (mDebug)
      {
         // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
         auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_SEL_IDX_TOO_LARGE);
         out.AddNote() << "Index: " << aIndex;
         out.AddNote() << "Max Index: " << mTechniques.size();
      }
      return false;
   }
   return true;
}

//! Selects a technique for use based on technique Id for the specified System Type
//!
//! @param aId Specifies the Id of the technique in the list.
//! @param aSystemFunction Indicates what system type that the effect will be applied to (i.e. Comms, Sensors, etc.).
//! @return Returns true if the technique was successfully selected.
// virtual
bool WsfEW_EA::SelectTechnique(WsfEW_Types::TechniqueId aId, WsfEW_Effect::SystemFunction aSystemFunction)
{
   TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(aId);
   if (techIter != mTechniquePtrMap.end())
   {
      if (techIter->second->CanApplyTechnique(aSystemFunction))
      {
         if (PrivateSetTechnique(aId, true))
         {
            if (mDebug)
            {
               auto out = ut::log::debug() << "Selected technique for system type.";
               out.AddNote() << "Technique: " << techIter->second->GetName();
               out.AddNote() << "System Type: " << aSystemFunction;
            }
         }
         else
         {
            if (mDebug)
            {
               auto out = ut::log::debug() << "Selected technique for system type already selected.";
               out.AddNote() << "Technique: " << techIter->second->GetName();
               out.AddNote() << "System Type: " << aSystemFunction;
            }
         }
      }
      else
      {
         if (mDebug)
         {
            // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
            auto logWarning =
               ut::log::warning()
               << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_SEL_APPLY_AN);
            logWarning.AddNote() << "System Type: " << aSystemFunction;
            logWarning.AddNote() << "Technique" << techIter->second->GetName();
         }
         return false;
      }
   }
   else
   {
      if (mDebug)
      {
         // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
         auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_SEL_NOT_FOUND_ST);
         out.AddNote() << "Technique: " << aId;
         out.AddNote() << "System Type: " << aSystemFunction;
      }
      return false;
   }
   return true;
}

//! Deselect a technique for use based on technique index for the specified System Type
//!
//! @param aIndex Specifies the index of the technique in the list (0 - size mTechniques-1).
//! @param aSystemFunction Indicates what system type that the effect will be applied to (i.e. Comms, Sensors, etc.).
//! @return Returns true if the technique was successfully deselected.
// virtual
bool WsfEW_EA::DeselectTechnique(unsigned int aIndex, WsfEW_Effect::SystemFunction aSystemFunction)
{
   if (aIndex < mTechniques.size())
   {
      TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(mTechniques[aIndex]);
      if (techIter != mTechniquePtrMap.end())
      {
         if (techIter->second->CanApplyTechnique(aSystemFunction))
         {
            if (PrivateSetTechnique(mTechniques[aIndex], false))
            {
               if (mDebug)
               {
                  auto out = ut::log::debug() << "Deselected technique for system type.";
                  out.AddNote() << "Technique: " << techIter->second->GetName();
                  out.AddNote() << "System Type: " << aSystemFunction;
               }
            }
            else
            {
               if (mDebug)
               {
                  auto out = ut::log::debug() << "Deselected technique for system type already selected.";
                  out.AddNote() << "Technique: " << techIter->second->GetName();
                  out.AddNote() << "System Type: " << aSystemFunction;
               }
            }
         }
         else
         {
            if (mDebug)
            {
               // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
               auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_DESEL_APPLY_IDX);
               out.AddNote() << "Technique: " << techIter->second->GetName();
               out.AddNote() << "Index: " << aIndex;
               out.AddNote() << "System Type: " << aSystemFunction;
            }
            return false;
         }
      }
      else
      {
         if (mDebug)
         {
            // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
            auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_DESEL_NO_VALID_ST);
            out.AddNote() << "Index: " << aIndex;
            out.AddNote() << "System Type: " << aSystemFunction;
         }
         return false;
      }
   }
   else
   {
      if (mDebug)
      {
         // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
         auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_DESEL_IDX_TOO_LARGE);
         out.AddNote() << "Index: " << aIndex;
         out.AddNote() << "Max Index: " << mTechniques.size();
      }
      return false;
   }
   return true;
}

//! Deselect a technique for use based on technique Id for the specified System Type
//!
//! @param aId Specifies the Id of the technique in the list.
//! @param aSystemFunction Indicates what system type that the effect will be applied to (i.e. Comms, Sensors, etc.).
//! @return Returns true if the technique was successfully deselected.
// virtual
bool WsfEW_EA::DeselectTechnique(WsfEW_Types::TechniqueId aId, WsfEW_Effect::SystemFunction aSystemFunction)
{
   TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(aId);
   if (techIter != mTechniquePtrMap.end())
   {
      if (techIter->second->CanApplyTechnique(aSystemFunction))
      {
         if (PrivateSetTechnique(aId, false))
         {
            if (mDebug)
            {
               auto out = ut::log::debug() << "Deselected technique.";
               out.AddNote() << "Technique: " << techIter->second->GetName();
               out.AddNote() << "Id: " << aId;
               out.AddNote() << "System Type: " << aSystemFunction;
            }
         }
         else
         {
            if (mDebug)
            {
               auto out = ut::log::debug() << "Deselected technique due to already being selected.";
               out.AddNote() << "Technique: " << techIter->second->GetName();
               out.AddNote() << "Id: " << aId;
               out.AddNote() << "System Type: " << aSystemFunction;
            }
         }
      }
      else
      {
         if (mDebug)
         {
            // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
            auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_DESEL_APPLY_FAIL);
            out.AddNote() << "Technique: " << techIter->second->GetName();
            out.AddNote() << "Id: " << aId;
            out.AddNote() << "System Type: " << aSystemFunction;
         }
         return false;
      }
   }
   else
   {
      if (mDebug)
      {
         // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
         auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EW_DESEL_NOT_FOUND);
         out.AddNote() << "Technique: " << aId;
         out.AddNote() << "System Type: " << aSystemFunction;
      }
      return false;
   }
   return true;
}


//! Set a jamming modulation technique on an active spot using the supplied criteria.
//!
//! @param aSimTime            The current simulation time.
//! @param aTechniqueId        The assigned technique Id to set.
//! @param aOnOff              The default state of the technique.
//! @param aDebug              The debug state 'true' or 'false'.
//! @param aEffectId           The assigned effect Id to set.
//! @param aJammingDeltaGain   The assigned jamming modulation value.
//! @param aSystemTypeId       The assigned System Type Id of the target types this technique be applied to.
//! @param aSystemFunctionId   The assigned System Function Id of the target types this technique be applied to.
//! @return The request was successful.
// virtual
bool WsfEW_EA::SetDeltaGainTechnique(double      aSimTime,
                                     WsfStringId aTechniqueId,
                                     bool        aOnOff,
                                     bool        aDebug,
                                     WsfStringId aEffectId,
                                     double      aJammingDeltaGain,
                                     WsfStringId aSystemTypeId,
                                     WsfStringId aSystemFunctionId)
{
   // set default return value
   bool setTechnique(false);

   EA_Technique*                 techniquePtr(nullptr);
   TechniqueIdToPtrMap::iterator techIter = mTechniquePtrMap.find(aTechniqueId);
   if (techIter != mTechniquePtrMap.end())
   {
      techniquePtr = techIter->second;
   }
   else
   {
      techniquePtr = new EA_Technique();
      if (techniquePtr != nullptr)
      {
         techniquePtr->SetName(aTechniqueId);

         mTechniques.push_back(techniquePtr->GetNameId());
         mTechniquePtrMap[(techniquePtr->GetNameId())] = techniquePtr;
      }
   }

   if (techniquePtr != nullptr)
   {
      techniquePtr->SetDebug(aDebug);
      techniquePtr->SetDefaultOn(aOnOff);
      techniquePtr->SetEA_EP_Ptr(this);
   }
   else
   {
      if (mDebug)
      {
         // 二次开发（i18n）：warning 级消息 ID 化（落盘英文+[W:ID]，界面中文）
         auto out = ut::log::warning() << ut::msg::FormatWEnTagged(ut::msg::MSG_W_EA_SETDELTAGAIN_FAIL);
         out.AddNote() << "Technique: " << aTechniqueId;
      }
      return false;
   }

   setTechnique =
      techniquePtr->SetDeltaGainEffect(aSimTime, aEffectId, aDebug, aJammingDeltaGain, aSystemTypeId, aSystemFunctionId);

   setTechnique &= techniquePtr->Initialize(*GetSimulation());

   return setTechnique;
}

/*void  WsfEW_EA::AggregateEffects(const EA_Effect aUniqueEffect, EA_Effect& aAggregatedEffect)
{
   // Determine aggregate J/S effect  -- this may already be accounted for in the jamming ERP?
   if (aUniqueEffect.JammingLevel > aAggregatedEffect.JammingLevel)
      aAggregatedEffect.JammingLevel = aUniqueEffect.JammingLevel;


   // Determine maximum error introduced
   if (aUniqueEffect.Error.AzimuthError > aAggregatedEffect.Error.AzimuthError)
      aAggregatedEffect.Error.AzimuthError = aUniqueEffect.Error.AzimuthError;

   if (aUniqueEffect.Error.ElevationError > aAggregatedEffect.Error.ElevationError)
      aAggregatedEffect.Error.ElevationError = aUniqueEffect.Error.ElevationError;

   if (aUniqueEffect.Error.RangeError > aAggregatedEffect.Error.RangeError)
      aAggregatedEffect.Error.RangeError = aUniqueEffect.Error.RangeError;

   // Determine track effect

   // cMAINTAIN_TRACK introduces no track effect
   if (aUniqueEffect.TrackBehavior == cMAINTAIN_TRACK)
            aAggregatedEffect.TrackBehavior = aAggregatedEffect.TrackBehavior;

   // TrackError introduces a track error provide a DropTrack has not been encountered
   if (aUniqueEffect.TrackBehavior == TrackError && aAggregatedEffect.TrackBehavior != DropTrack)
            aAggregatedEffect.TrackBehavior = TrackError;

   // Forces a DropTrack effect
   if (aUniqueEffect.TrackBehavior == DropTrack)
            aAggregatedEffect.TrackBehavior = DropTrack;

   return;
}*/

//*************************************************************************
// virtual
WsfEW_Technique* WsfEW_EA::EA_Technique::Clone() const
{
   return new EA_Technique(*this);
}

bool WsfEW_EA::EA_Technique::Initialize(WsfSimulation& aSimulation)
{
   return WsfEW_Technique::Initialize(aSimulation);
}

bool WsfEW_EA::EA_Technique::ProcessInput(UtInput& aInput)
{
   bool        myCommand = true;
   std::string command(aInput.GetCommand());

   if ((command == "mitigation_class") || (command == "mitigation_class_name"))
   {
      std::string mitigationClassName;
      aInput.ReadValue(mitigationClassName);
      mMitigationClassId = mitigationClassName;
   }
   else
   {
      myCommand = WsfEW_Technique::ProcessInput(aInput);
   }

   return myCommand;
}
