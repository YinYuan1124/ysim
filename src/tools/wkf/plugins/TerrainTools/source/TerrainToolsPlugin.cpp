// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2019 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#include "TerrainToolsPlugin.hpp"

#include <QAction>
#include <QMenu>

#include "LOS_Ruler.hpp"
#include "UtQtMemory.hpp"
#include "VaUtils.hpp"
#include "WkfAttachmentRubberband.hpp"
#include "WkfEntity.hpp"
#include "WkfEnvironment.hpp"
#include "WkfMainWindow.hpp"
#include "WkfPointOfInterest.hpp"
#include "WkfScenario.hpp"
#include "WkfViewer.hpp"
#include "WkfVtkEnvironment.hpp"

WKF_PLUGIN_DEFINE_SYMBOLS(TerrainTools::Plugin,
                          "Terrain Tools",
                          "The Terrain Tools plugin provides tools that query the terrain, such as line-of-sight.",
                          "all")

TerrainTools::Plugin::Plugin(const QString& aPluginName, const size_t aUniqueId)
   : wkf::Plugin(aPluginName, aUniqueId)
{
}

void TerrainTools::Plugin::BuildEntityContextMenu(QMenu* aMenu, wkf::Entity* aEntityPtr)
{
   if (vaEnv.GetStandardViewer())
   {
      if (!mActiveRulerPtr)
      {
         QString  name = aEntityPtr->GetName().c_str();
         QAction* measureAction =
            ut::qt::make_qt_ptr<QAction>(QIcon::fromTheme("eye"), tr("Line-of-sight from %1...").arg(name), aMenu); // 二次开发（i18n）：实体右键菜单文本 tr() 化
         QObject::connect(measureAction, &QAction::triggered, this, [=]() { LineOfSightFrom(aEntityPtr); });
         aMenu->addAction(measureAction);

         mSrcEntity = aEntityPtr;
      }
   }
}

void TerrainTools::Plugin::LineOfSightFrom(const vespa::VaLocationReference& aLocRef)
{
   auto* viewerPtr = vaEnv.GetStandardViewer();
   if (viewerPtr)
   {
      if (!mActiveRulerPtr)
      {
         if (aLocRef.GetEntity())
         {
            wkf::AttachmentRubberband* r =
               viewerPtr->BeginRubberband(*aLocRef.GetEntity(),
                                          std::bind(&Plugin::RubberbandUpdater, this, std::placeholders::_1),
                                          std::bind(&Plugin::MakeLOS, this, std::placeholders::_1),
                                          tr("Click on destination point for the Line-of-sight.")); // 二次开发（i18n）：状态栏提示 tr() 化（类无 Q_OBJECT，落 wkf::Plugin 上下文）
            RubberbandUpdater(*r);
         }
      }
   }
}

void TerrainTools::Plugin::MakeLOS(const wkf::AttachmentRubberband& aRubberband)
{
   if (mSrcEntity)
   {
      wkf::Entity* p = dynamic_cast<wkf::Entity*>(aRubberband.GetHoverEntity());
      if ((p != nullptr) && (p != mSrcEntity))
      {
         double h;
         double d;
         p->GetPosition().CalculateHeadingAndDistance(mSrcEntity->GetPosition(), h, d);
         if (d < 1000000)
         {
            wkf::Entity& src   = *mSrcEntity;
            LOS_Ruler*   ruler = vespa::make_attachment<LOS_Ruler>(src, *p, *vaEnv.GetStandardViewer());
            vespa::VaAttachment::LoadAttachment(*ruler);
         }
      }
   }
}

void TerrainTools::Plugin::RubberbandUpdater(wkf::AttachmentRubberband& aRubberband)
{
   static UtoColor rangedout(160, 160, 160);
   static UtoColor valid(255, 178, 128);
   static UtoColor invalid(140, 134, 96);

   vespa::VaPosition p1 = aRubberband.GetHoverLocation();
   vespa::VaPosition p2 = mSrcEntity->GetPosition();
   double            h;
   double            d;
   p1.CalculateHeadingAndDistance(p2, h, d);
   if (d > 1000000)
   {
      aRubberband.SetLineColor(rangedout);
   }
   else
   {
      wkf::Entity* p = dynamic_cast<wkf::Entity*>(aRubberband.GetHoverEntity());
      if ((p != nullptr) && (p != mSrcEntity))
      {
         aRubberband.SetLineColor(valid);
      }
      else
      {
         aRubberband.SetLineColor(invalid);
      }
   }
}
