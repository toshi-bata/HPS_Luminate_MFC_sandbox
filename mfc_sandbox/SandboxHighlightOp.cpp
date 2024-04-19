#include "stdafx.h"
#include "CHPSApp.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include "SandboxHighlightOp.h"

HPS::Selection::Level SandboxHighlightOperator::SelectionLevel = HPS::Selection::Level::Entity;

SandboxHighlightOperator::SandboxHighlightOperator(
	CHPSView * in_cview)
	: HPS::SelectOperator(HPS::MouseButtons::ButtonLeft(), HPS::ModifierKeys()), cview(in_cview) {}

SandboxHighlightOperator::~SandboxHighlightOperator() {} 

//! [OnMouseDown]
bool SandboxHighlightOperator::OnMouseDown(
	HPS::MouseState const & in_state)
{
	auto sel_opts = GetSelectionOptions();
	sel_opts.SetLevel(SandboxHighlightOperator::SelectionLevel);
	SetSelectionOptions(sel_opts);

	if (IsMouseTriggered(in_state) && HPS::SelectOperator::OnMouseDown(in_state))
	{
		HighlightCommon();
		return true;
	}
	return false;
}
//! [OnMouseDown]

bool SandboxHighlightOperator::OnTouchDown(
	HPS::TouchState const & in_state)
{
	auto sel_opts = GetSelectionOptions();
	sel_opts.SetLevel(SandboxHighlightOperator::SelectionLevel);
	SetSelectionOptions(sel_opts);

	if (HPS::SelectOperator::OnTouchDown(in_state))
	{
		HighlightCommon();
		return true;
	}
	return false;
}

void SandboxHighlightOperator::HighlightCommon()
{
	cview->Unhighlight();

	HPS::SelectionResults selection_results = GetActiveSelection();
	size_t selected_count = selection_results.GetCount();
	if (selected_count > 0)
	{
		HPS::CADModel cad_model = cview->GetDocument()->GetCADModel();
        
		//! [build_highlight_style]
		HPS::HighlightOptionsKit highlight_options(HPS::HighlightOptionsKit::GetDefault());
		highlight_options.SetStyleName("highlight_style");
		highlight_options.SetSubentityHighlighting(SelectionLevel == HPS::Selection::Level::Subentity);
		highlight_options.SetOverlay(HPS::Drawing::Overlay::InPlace);
		//! [build_highlight_style]
        
		//! [highlight_cad_model]
		if (!cad_model.Empty())
		{
			// since we have a CADModel, we want to highlight the components, not just the Visualize geometry
			HPS::SelectionResultsIterator it = selection_results.GetIterator();
			HPS::Canvas canvas = cview->GetCanvas();
			while (it.IsValid())
			{
				HPS::ComponentPath component_path = cad_model.GetComponentPath(it.GetItem());
				if (!component_path.Empty())
				{
					// Make the selected component get highlighted in the model browser
					highlight_options.SetNotification(true); 
					component_path.Highlight(canvas, highlight_options);

					// if we selected PMI, highlight the associated components (if any)
					HPS::Component const & leaf_component = component_path.Front();
					if (leaf_component.HasComponentType(HPS::Component::ComponentType::ExchangePMIMask))
					{
						// Only highlight the Visualize geometry for the associated components, don't highlight the associated components in the model browser
						highlight_options.SetNotification(false);
						for (auto const & reference : leaf_component.GetReferences())
							HPS::ComponentPath(1, &reference).Highlight(canvas, highlight_options);
					}
				}
				it.Next();
			}
		}
		//! [highlight_cad_model]
		//! [highlight_geometry]
		else
		{
			// since there is no CADModel, just highlight the Visualize geometry
			cview->GetCanvas().GetWindowKey().GetHighlightControl().Highlight(selection_results, highlight_options);
			HPS::Database::GetEventDispatcher().InjectEvent(HPS::HighlightEvent(HPS::HighlightEvent::Action::Highlight, selection_results, highlight_options));
		}
		//! [highlight_geometry]
	}

	cview->Update();
}
