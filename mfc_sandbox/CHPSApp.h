#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

#define WM_MFC_SANDBOX_INITIALIZE_BROWSERS (WM_USER + 100)
#define WM_MFC_SANDBOX_ADD_PROPERTY (WM_USER + 101)
#define WM_MFC_SANDBOX_FLUSH_PROPERTIES (WM_USER + 102)
#define WM_MFC_SANDBOX_UNSET_ATTRIBUTE (WM_USER + 103)


class CHPSApp : public CWinAppEx
{
public:
	CHPSApp();

	virtual BOOL	InitInstance();
	virtual int		ExitInstance();
	void			SetAppLook(UINT look) { _appLook = look; }
	UINT			GetAppLook() { return _appLook; }
	void			addToRecentFileList(CString path);
	void			setFontDirectory(const char* fontDir);

private:
	UINT							_appLook;
	HPS::World *					_world;

protected:
	DECLARE_MESSAGE_MAP()
};

extern CHPSApp theApp;
