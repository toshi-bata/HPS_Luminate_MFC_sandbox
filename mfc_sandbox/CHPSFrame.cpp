#include "stdafx.h"
#include "CHPSView.h"
#include "CHPSApp.h"
#include "CHPSFrame.h"
#include "SandboxHighlightOp.h"
#include <imm.h>


IMPLEMENT_DYNCREATE(CHPSFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CHPSFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CHPSFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CHPSFrame::OnUpdateApplicationLook)
	ON_MESSAGE(WM_MFC_SANDBOX_INITIALIZE_BROWSERS, &CHPSFrame::InitializeBrowsers)
	ON_MESSAGE(WM_MFC_SANDBOX_ADD_PROPERTY, &CHPSFrame::AddProperty)
	ON_MESSAGE(WM_MFC_SANDBOX_FLUSH_PROPERTIES, &CHPSFrame::FlushProperties)
	ON_MESSAGE(WM_MFC_SANDBOX_UNSET_ATTRIBUTE, &CHPSFrame::UnsetAttribute)
	ON_COMMAND(ID_OPERATOR_SEGMENT_BROWSER, &CHPSFrame::OnModesSegmentBrowser)
	ON_UPDATE_COMMAND_UI(ID_OPERATOR_SEGMENT_BROWSER, &CHPSFrame::OnUpdateModesSegmentBrowser)
	ON_COMMAND(ID_OPERATOR_MODEL_BROWSER, &CHPSFrame::OnModesModelBrowser)
	ON_UPDATE_COMMAND_UI(ID_OPERATOR_MODEL_BROWSER, &CHPSFrame::OnUpdateModesModelBrowser)
	ON_COMMAND(ID_COMBO_SEL_LEVEL, &CHPSFrame::OnComboSelectionLevel)
	ON_COMMAND(ID_COMBO_MATERIAL, &CHPSFrame::OnComboMaterial)
	ON_COMMAND(ID_COMBO_LIGHTING_MODE, &CHPSFrame::OnComboLightingMode)
	ON_COMMAND(ID_COMBO_MAT_TYPE, &CHPSFrame::OnComboMatType)
END_MESSAGE_MAP()

CHPSFrame::CHPSFrame()
{
	theApp.SetAppLook(theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_WINDOWS_7));
}

CHPSFrame::~CHPSFrame()
{
}

int CHPSFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	OnApplicationLook(theApp.GetAppLook());
	m_wndRibbonBar.Create(this);
	m_wndRibbonBar.LoadFromResource(IDR_RIBBON);

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Switch the order of document name and application name on the window title bar. This
	// improves the usability of the taskbar because the document name is visible with the thumbnail.
	ModifyStyle(0, FWS_PREFIXTITLE);

	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI;

	if (!m_segmentBrowserPane.Create(L"Segment Browser", this, CRect(0, 0, 250, 250), TRUE, ID_VIEW_SEGMENTBROWSER, dwStyle | CBRS_LEFT))
	{
		TRACE0("Failed to create Segment Browser pane\n");
		return FALSE; // failed to create
	}

	if (!m_propertiesPane.Create(_T("Properties"), this, CRect(0, 0, 250, 250), TRUE, ID_VIEW_PROPERTIES, dwStyle | CBRS_LEFT))
	{
		TRACE0("Failed to create Properties pane\n");
		return FALSE; // failed to create
	}

	if (!m_modelBrowserPane.Create(L"Model Browser", this, CRect(0, 0, 250, 250), TRUE, ID_VIEW_MODELBROWSER, dwStyle | CBRS_RIGHT))
	{
		TRACE0("Failed to create Model Browser pane\n");
		return FALSE; // failed to create
	}

	if (!m_configurationPane.Create(L"Configurations", this, CRect(0, 0, 250, 250), TRUE, ID_VIEW_CONFIGURATIONS, dwStyle | CBRS_RIGHT))
	{
		TRACE0("Failed to create Configurations pane\n");
		return FALSE; // failed to create
	}

	if (!m_tabbedPane.Create(L"", this, CRect(0, 0, 250, 250), TRUE, ID_VIEW_TABBED_BROWSER, dwStyle | CBRS_RIGHT))
	{
		TRACE0("Failed to create Tabbed pane\n");
		return FALSE; // failed to create
	}

	EnableDocking(CBRS_LEFT | CBRS_RIGHT);

	m_tabbedPane.EnableDocking(CBRS_LEFT | CBRS_RIGHT);
	m_modelBrowserPane.EnableDocking(CBRS_LEFT | CBRS_RIGHT);
	m_configurationPane.EnableDocking(CBRS_LEFT | CBRS_RIGHT);
	DockPane(&m_tabbedPane);
	m_tabbedPane.AddTab(&m_modelBrowserPane, TRUE, TRUE);
	m_tabbedPane.AddTab(&m_configurationPane, TRUE, FALSE);

	m_tabbedPane.ShowTab(&m_modelBrowserPane, FALSE, FALSE, FALSE);
	m_tabbedPane.ShowTab(&m_configurationPane, FALSE, FALSE, FALSE);

	m_segmentBrowserPane.EnableDocking(CBRS_LEFT | CBRS_RIGHT);
	DockPane(&m_segmentBrowserPane);
	m_propertiesPane.EnableDocking(CBRS_ALIGN_ANY);
	m_propertiesPane.DockToWindow(&m_segmentBrowserPane, CBRS_ALIGN_BOTTOM);

	m_segmentBrowserPane.ShowPane(FALSE, FALSE, FALSE);
	m_propertiesPane.ShowPane(FALSE, FALSE, FALSE);
	m_tabbedPane.ShowPane(FALSE, FALSE, FALSE);

	auto element = m_wndRibbonBar.FindByID(ID_COMBO_SEL_LEVEL);
	if (element != nullptr)
	{
		CMFCRibbonComboBox * combo = reinterpret_cast<CMFCRibbonComboBox *>(element);
		combo->AddItem(_T("Segment"));
		combo->AddItem(_T("Entity"));
		combo->AddItem(_T("Subentity"));
		combo->SelectItem((int)SandboxHighlightOperator::SelectionLevel);
	}

	// Prepare rendering progress bar
	CMFCRibbonProgressBar* pProgress = (CMFCRibbonProgressBar*)m_wndRibbonBar.FindByID(ID_PROGRESS_RENDER);
	pProgress->SetRange(0, 100);

	// Prepare Material Type combo box
	{
		CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_MAT_TYPE);
		pCombo->AddItem(_T("Glass"));
		pCombo->AddItem(_T("Jewellery"));
		pCombo->AddItem(_T("Metal"));
		pCombo->AddItem(_T("Plastic"));
		pCombo->SelectItem(0);
	}

	// Prepare Material combo box
	setMaterialCombo(0);

	// Prepare Lighting Mode combo box
	{
		CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_LIGHTING_MODE);
		pCombo->AddItem(_T("Default"));
		pCombo->AddItem(_T("Sun Sky Model"));
		pCombo->AddItem(_T("Environment Map"));
		pCombo->SelectItem(0);
	}
	return 0;
}

BOOL CHPSFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CFrameWndEx::PreCreateWindow(cs))
		return FALSE;

	return TRUE;
}

void CHPSFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.SetAppLook(id);

	switch (theApp.GetAppLook())
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(TRUE);
		break;

	default:
		switch (theApp.GetAppLook())
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
	theApp.WriteInt(_T("ApplicationLook"), theApp.GetAppLook());
}

void CHPSFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.GetAppLook() == pCmdUI->m_nID);
}

LRESULT CHPSFrame::InitializeBrowsers(WPARAM /*w*/, LPARAM /*l*/)
{
	m_segmentBrowserPane.Init();
	m_modelBrowserPane.Init();
	m_configurationPane.Init();
	m_propertiesPane.Flush();
	return 0;
}

LRESULT CHPSFrame::AddProperty(WPARAM w, LPARAM l)
{
	auto treeItem = reinterpret_cast<MFCSceneTreeItem *>(w);
	auto itemType = static_cast<HPS::SceneTree::ItemType>(l);
	if (itemType == HPS::SceneTree::ItemType::None)
		m_propertiesPane.AddProperty(treeItem);
	else
		m_propertiesPane.AddProperty(treeItem, itemType);
	return 0;
}

LRESULT CHPSFrame::FlushProperties(WPARAM /*w*/, LPARAM /*l*/)
{
	m_propertiesPane.Flush();
	return 0;
}

LRESULT CHPSFrame::UnsetAttribute(WPARAM w, LPARAM /*l*/)
{
	auto treeItem = reinterpret_cast<MFCSceneTreeItem *>(w);
	m_propertiesPane.UnsetAttribute(treeItem);
	return 0;
}

void CHPSFrame::OnModesSegmentBrowser()
{
	if (m_segmentBrowserPane.IsVisible() || m_propertiesPane.IsVisible())
	{
		m_segmentBrowserPane.ShowPane(FALSE, FALSE, FALSE);
		m_propertiesPane.ShowPane(FALSE, FALSE, FALSE);
	}
	else
	{
		m_segmentBrowserPane.ShowPane(TRUE, TRUE, TRUE);
		m_propertiesPane.ShowPane(FALSE, FALSE, FALSE);
	}

	Invalidate();
	RecalcLayout();
}

void CHPSFrame::OnUpdateModesSegmentBrowser(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_segmentBrowserPane.IsVisible());
}

void CHPSFrame::OnModesModelBrowser()
{
	if (m_modelBrowserPane.IsVisible() || m_configurationPane.IsVisible())
	{
		m_tabbedPane.ShowTab(&m_modelBrowserPane, FALSE, FALSE, FALSE);
		m_tabbedPane.ShowTab(&m_configurationPane, FALSE, FALSE, FALSE);
	}
	else
	{
		m_tabbedPane.ShowTab(&m_modelBrowserPane, TRUE, TRUE, TRUE);
		m_tabbedPane.ShowTab(&m_configurationPane, TRUE, TRUE, TRUE);
	}

	Invalidate();
	RecalcLayout();
	m_tabbedPane.RecalcLayout();
}

void CHPSFrame::OnUpdateModesModelBrowser(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_modelBrowserPane.IsVisible());
}

void CHPSFrame::SetPropertiesPaneVisibility(bool state)
{
	BOOL flag = (state ? TRUE : FALSE);
	m_propertiesPane.ShowPane(flag, flag, flag);

	if (state)
		m_propertiesPane.DockToWindow(&m_segmentBrowserPane, CBRS_ALIGN_BOTTOM);
}

bool CHPSFrame::GetPropertiesPaneVisibility() const
{
	return (m_propertiesPane.IsVisible() != FALSE);
}

void CHPSFrame::OnComboSelectionLevel()
{
	auto comboBox = (CMFCRibbonComboBox const *)m_wndRibbonBar.FindByID(ID_COMBO_SEL_LEVEL);
	
	if (comboBox)
	{		
		int selection = comboBox->GetCurSel();
		SandboxHighlightOperator::SelectionLevel = (HPS::Selection::Level)selection;
	}
}

HPS::KeyboardEvent CHPSFrame::BuildKeyboardEvent(HPS::KeyboardEvent::Action action, UINT button)
{
	HPS::KeyboardCodeArray codes;
	HPS::ModifierKeys modifiers;

	if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
		modifiers.LeftControl(true);
	if (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
		modifiers.RightControl(true);
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
		modifiers.LeftShift(true);
	if (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
		modifiers.RightShift(true);
	if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
		modifiers.CapsLock(true);

	codes.push_back(HPS::KeyboardCode(button));
	return HPS::KeyboardEvent(action, codes, modifiers);
}

BOOL CHPSFrame::PreTranslateMessage(MSG* pMsg)
{
	if ((WM_LBUTTONDBLCLK == pMsg->message) && (pMsg->hwnd == m_wndRibbonBar))
	{
		CPoint point(pMsg->pt);
		m_wndRibbonBar.ScreenToClient(&point);
		CMFCRibbonBaseElement* pHit = m_wndRibbonBar.HitTest(point);
		if (pHit && pHit->IsKindOf(RUNTIME_CLASS(CMFCRibbonApplicationButton)))
		{
			//intercept double click on the main button to prevent shutdown
			return TRUE; // no further dispatch
		}
	}

    else if (pMsg->message == WM_KEYDOWN)
    {
        CHPSView * view = reinterpret_cast<CHPSView *>(GetActiveView());
        UINT keycode = (UINT)pMsg->wParam;
        if (keycode == VK_PROCESSKEY)
            keycode = ImmGetVirtualKey(pMsg->hwnd);
        view->GetCanvas().GetWindowKey().GetEventDispatcher().InjectEvent(BuildKeyboardEvent(HPS::KeyboardEvent::Action::KeyDown, keycode));
    }

	else if (pMsg->message == WM_KEYUP)
	{
		CHPSView * view = reinterpret_cast<CHPSView *>(GetActiveView());
		view->GetCanvas().GetWindowKey().GetEventDispatcher().InjectEvent(BuildKeyboardEvent(HPS::KeyboardEvent::Action::KeyUp, (UINT)pMsg->wParam));
	}

	return CFrameWndEx::PreTranslateMessage(pMsg);
}

void CHPSFrame::SetProgress(const int pos, const int remTime)
{
	CMFCRibbonProgressBar* pProgress = (CMFCRibbonProgressBar*)m_wndRibbonBar.FindByID(ID_PROGRESS_RENDER);
	pProgress->SetPos(pos);
	
	int hour = remTime / 3600;
	int min = (remTime % 3600) / 60;
	int sec = remTime % 60;
	wchar_t wcRemTime[256];
	swprintf(wcRemTime, _T("%dh %dm %ds"), hour, min, sec);
	CMFCRibbonEdit* pEdit = (CMFCRibbonEdit*)m_wndRibbonBar.FindByID(ID_EDIT_REM_TIME);
	pEdit->SetEditText(wcRemTime);
}

void CHPSFrame::OnComboMatType()
{
	CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_MAT_TYPE);
	int id = pCombo->GetCurSel();

	setMaterialCombo(id);

	CHPSView* view = reinterpret_cast<CHPSView*>(GetActiveView());
	view->SetMaterialTypeId(id);
	view->SetMaterialId(0);
}

void CHPSFrame::OnComboMaterial()
{
	CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_MATERIAL);
	int id = pCombo->GetCurSel();

	CHPSView* view = reinterpret_cast<CHPSView*>(GetActiveView());
	view->SetMaterialId(id);
}


void CHPSFrame::OnComboLightingMode()
{
	CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_LIGHTING_MODE);
	int id = pCombo->GetCurSel();

	CHPSView* view = reinterpret_cast<CHPSView*>(GetActiveView());
	view->SetLightingModeId(id);
}

void CHPSFrame::setMaterialCombo(const int matTypeId)
{
	CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_MATERIAL);
	pCombo->RemoveAllItems();

	switch (matTypeId)
	{
		case 0:
		{
			pCombo->AddItem(_T("Clear_glass"));
		} break;
		case 1:
		{
			pCombo->AddItem(_T("Amber"));
			pCombo->AddItem(_T("Diamond_(white)"));
			pCombo->AddItem(_T("Emerald_(green)"));
			pCombo->AddItem(_T("Onyx"));
			pCombo->AddItem(_T("Pearl"));
			pCombo->AddItem(_T("Ruby"));
			pCombo->AddItem(_T("Sapphire_(blue)"));
			pCombo->AddItem(_T("Topaz_(yellow)"));
		} break;
		case 2:
		{
			pCombo->AddItem(_T("Brass"));
			pCombo->AddItem(_T("Brushed_Gold"));
			pCombo->AddItem(_T("Chrome"));
			pCombo->AddItem(_T("Copper"));
			pCombo->AddItem(_T("Iron"));
			pCombo->AddItem(_T("Polished_Gold"));
			pCombo->AddItem(_T("Silver"));
			pCombo->AddItem(_T("Titanium"));
		} break;
		case 3:
		{
			pCombo->AddItem(_T("Mat_plastic"));
			pCombo->AddItem(_T("Shiny_plastic"));
			pCombo->AddItem(_T("Transparent_plastic"));
		} break;
		default: break;
	}

	pCombo->SelectItem(0);
}

void CHPSFrame::SetLighingModeSelItem(const int id)
{
	CMFCRibbonComboBox* pCombo = (CMFCRibbonComboBox*)m_wndRibbonBar.FindByID(ID_COMBO_LIGHTING_MODE);
	pCombo->SelectItem(id);
}