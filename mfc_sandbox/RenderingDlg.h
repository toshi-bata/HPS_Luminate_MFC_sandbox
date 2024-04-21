#pragma once
#include "Resource.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include "LuminateBridgeWapper.h"

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
	UINT m_timerID;
	LuminateBridgeWapper* m_pLuminateBridgeWrapper;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
