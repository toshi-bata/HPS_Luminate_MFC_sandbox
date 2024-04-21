#include "stdafx.h"
#include "RenderingDlg.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(RenderingDlg, CDialogEx)

RED::Object* window;

RenderingDlg::RenderingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RENDERING_DIALOG, pParent),
	m_hpsView((CHPSView*)pParent),
	m_pLuminateBridgeWrapper(nullptr),
	m_timerID(0)
{
	Create(IDD_RENDERING_DIALOG, pParent);
}

RenderingDlg::~RenderingDlg()
{
}

void RenderingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(RenderingDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL RenderingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Setup timmer
	m_timerID = 1;
	UINT interval = 100;
	m_timerID = SetTimer(m_timerID, interval, NULL);
	
	// Setup operator
	m_hpsView->GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::HighlightOperator(HPS::MouseButtons::ButtonLeft()));

	return TRUE;  // return TRUE unless you set the focus to a control
}

void RenderingDlg::OnCancel()
{
	if (0 != m_timerID)
	{
		KillTimer(m_timerID);
		m_timerID = 0;
	}

	delete m_pLuminateBridgeWrapper;

	m_hpsView->SetupDefaultOperators();

	m_hpsView->SetProgress(0, 0);

	DestroyWindow();
}

void RenderingDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Initialize Luminate bridge if it is not there
	if (nullptr == m_pLuminateBridgeWrapper)
	{
		m_pLuminateBridgeWrapper = new LuminateBridgeWapper(m_hpsView);

		RECT rcl;
		::GetClientRect(m_hWnd, &rcl);

		std::string filepath = "";
		m_pLuminateBridgeWrapper->Initialize(m_hWnd, rcl.right, rcl.bottom);
	}

	m_pLuminateBridgeWrapper->Update();

	// Progress rendering
	FrameStatistics stats = m_pLuminateBridgeWrapper->getFrameStatistics();
	int renderingProgress = stats.renderingProgress * 100.f;
	if (100 >= renderingProgress)
	{
		m_hpsView->SetProgress(renderingProgress, stats.remainingTimeMilliseconds / 1000.0);
		m_pLuminateBridgeWrapper->Draw();
	}
	else
	{
		KillTimer(m_timerID);
		m_timerID = 0;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void RenderingDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (nullptr == m_pLuminateBridgeWrapper)
		return;

	m_pLuminateBridgeWrapper->Resize(cx, cy);
}
