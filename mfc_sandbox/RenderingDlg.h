#pragma once
#include "Resource.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>
#include "HPSHandlers.h"

using namespace hoops_luminate_bridge;

class RenderingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RenderingDlg)

public:
	RenderingDlg(CWnd* pParent = nullptr);
	virtual ~RenderingDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RENDERING_DIALOG };
#endif
private:
	CHPSView* m_hpsView;
	char m_cHpsDir[MAX_PATH] = { '\0' };
	UINT m_timerID;
	HoopsLuminateBridgeHPS* m_luminateBridge;
	SegmentSelectedHandler m_segmentSelectedHandler;
	int m_bSyncCamera;
	int m_iLightingModeId;

	void initLuminateBridge();

	void stopFrameTracing();
	void applyMaterial();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
