#include "stdafx.h"
#include "HPSHandlers.h"
#include "KeyPathUtils.h"

///////////////////////////////////////////
///// SegmentSelectedHandler ///////
///////////////////////////////////////////

SegmentSelectedHandler::SegmentSelectedHandler() : HPS::EventHandler()/*, m_hpsWidget(nullptr)*/ { m_bUpdated = false; }

SegmentSelectedHandler ::~SegmentSelectedHandler() { Shutdown(); }

void SegmentSelectedHandler::setView(CHPSView* a_view) { m_hpsView = a_view; }
void SegmentSelectedHandler::setLuninateBridge(HoopsLuminateBridge* a_bridge) { m_luminateBridge = a_bridge; }

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
            m_hpsView->GetCanvas().GetWindowKey().GetHighlightControl().Unhighlight(hok);
            m_hpsView->GetCanvas().GetWindowKey().GetHighlightControl().Highlight(keyPath, hok, true);
            m_hpsView->GetCanvas().Update();

            // Serialize the key path.
            HPS::UTF8 serializedKeyPath = KeyPathUtils::serializeRawKeyPath(keyPath, m_hpsView->GetCanvas().GetFrontView().GetAttachedModel().GetSegmentKey());

            // Get current faces diffuse color.
            RED::Color diffuseColor;
            std::string diffuseTextureName;
            hoops_luminate_bridge::HoopsLuminateBridgeHPS::getFacesColor(
                keyPath, HPS::Material::Channel::DiffuseColor, diffuseColor, diffuseTextureName);

            // Send selected segment key path and diffuse color.
            hoops_luminate_bridge::SelectedHPSSegmentInfoPtr selectedInfo(
                new hoops_luminate_bridge::SelectedHPSSegmentInfo(ownerSegment.GetInstanceID(), ownerSegment, serializedKeyPath, diffuseColor));
            m_luminateBridge->setSelectedSegmentInfo(selectedInfo);

            m_bUpdated = true;
        }
    }

    return HandleResult::Handled;
}
