#pragma once

#include "CHPSSegmentBrowserPane.h"
#include "CHPSPropertiesPane.h"
#include "CHPSModelBrowserPane.h"
#include "CHPSConfigurationPane.h"

class CHPSFrame : public CFrameWndEx
{
protected:
	virtual ~CHPSFrame();
	virtual BOOL		PreCreateWindow(CREATESTRUCT& cs);

public:
	void SetPropertiesPaneVisibility(bool state);
	bool GetPropertiesPaneVisibility() const;

	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	CHPSFrame();
	DECLARE_DYNCREATE(CHPSFrame)

	CMFCRibbonBar			m_wndRibbonBar;
	CHPSSegmentBrowserPane	m_segmentBrowserPane;
	CHPSPropertiesPane		m_propertiesPane;
	CTabbedPane				m_tabbedPane;
	CHPSModelBrowserPane	m_modelBrowserPane;
	CHPSConfigurationPane	m_configurationPane;

	afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void		OnApplicationLook(UINT id);
	afx_msg void		OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg LRESULT		InitializeBrowsers(WPARAM w, LPARAM l);
	afx_msg LRESULT		AddProperty(WPARAM w, LPARAM l);
	afx_msg void		OnModesSegmentBrowser();
	afx_msg void		OnUpdateModesSegmentBrowser(CCmdUI *pCmdUI);
	afx_msg LRESULT		FlushProperties(WPARAM w, LPARAM l);
	afx_msg void		OnModesModelBrowser();
	afx_msg void		OnUpdateModesModelBrowser(CCmdUI *pCmdUI);
	afx_msg void		OnComboSelectionLevel();
	afx_msg LRESULT		UnsetAttribute(WPARAM w, LPARAM l);

	DECLARE_MESSAGE_MAP()

private:
	HPS::KeyboardEvent	BuildKeyboardEvent(HPS::KeyboardEvent::Action action, UINT button);
};


