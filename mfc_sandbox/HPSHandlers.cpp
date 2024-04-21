#include "stdafx.h"
//#include <HPSWidget.h>
//#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>
#include "KeyPathUtils.h"

#include "HPSHandlers.h"

///////////////////////////////////////////
///// SegmentSelectedHandler ///////
///////////////////////////////////////////

SegmentSelectedHandler::SegmentSelectedHandler() : HPS::EventHandler()/*, m_hpsWidget(nullptr)*/ {}

SegmentSelectedHandler ::~SegmentSelectedHandler() { Shutdown(); }

void SegmentSelectedHandler::setView(CHPSView* a_view, HoopsLuminateBridge* a_bridge) { m_mHpsView = a_view; m_luminateBridge = a_bridge; }

HPS::EventHandler::HandleResult SegmentSelectedHandler::Handle(HPS::Event const* in_event)
{
    //if (m_hpsWidget == nullptr)
    //    return HandleResult::NotHandled;

    HPS::HighlightEvent const* highlightEvent = static_cast<HPS::HighlightEvent const*>(in_event);

    HPS::SelectionResults selectionResults = highlightEvent->results;
    if (selectionResults.GetCount() == 1) {
        HPS::SelectionResultsIterator iter = selectionResults.GetIterator();
        HPS::SelectionItem selectionItem = iter.GetItem();
        HPS::Key key;
        HPS::KeyPath keyPath;
        selectionItem.ShowSelectedItem(key);
        selectionItem.ShowPath(keyPath);

        if (key.Type() == HPS::Type::ShellKey) {
            HPS::SegmentKey ownerSegment = key.Owner();

            HPS::HighlightOptionsKit hok;
            hok.SetStyleName(HPS::UTF8("select"));
            m_mHpsView->GetCanvas().GetWindowKey().GetHighlightControl().Unhighlight(hok);
            m_mHpsView->GetCanvas().GetWindowKey().GetHighlightControl().Highlight(keyPath, hok, true);
            m_mHpsView->GetCanvas().Update();

            // Serialize the key path.
            HPS::UTF8 serializedKeyPath = KeyPathUtils::serializeRawKeyPath(keyPath, m_mHpsView->GetCanvas().GetFrontView().GetAttachedModel().GetSegmentKey());

            // Get current faces diffuse color.
            RED::Color diffuseColor;
            std::string diffuseTextureName;
            hoops_luminate_bridge::HoopsLuminateBridgeHPS::getFacesColor(
                keyPath, HPS::Material::Channel::DiffuseColor, diffuseColor, diffuseTextureName);

            // Send selected segment key path and diffuse color.
            hoops_luminate_bridge::SelectedHPSSegmentInfoPtr selectedInfo(
                new hoops_luminate_bridge::SelectedHPSSegmentInfo(ownerSegment.GetInstanceID(), ownerSegment, serializedKeyPath, diffuseColor));
            m_luminateBridge->setSelectedSegmentInfo(selectedInfo);

            m_mHpsView->SetSegmentSelected(true);
        }
    }

    return HandleResult::Handled;
}
