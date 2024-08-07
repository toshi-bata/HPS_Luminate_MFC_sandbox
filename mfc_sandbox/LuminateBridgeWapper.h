#pragma once
#include "sprk.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>
#include "HPSHandlers.h"

using namespace hoops_luminate_bridge;

class LuminateBridgeWapper : 
	public HPS::Operator
{
public:
	LuminateBridgeWapper(CHPSView* view);
	~LuminateBridgeWapper();

private:
	CHPSView* m_pHpsView;
	int m_iLightingModeId;
	char m_cHdriFilePath[MAX_PATH];
	bool m_bSyncCamera;
	HoopsLuminateBridgeHPS* m_luminateBridge;
	SegmentSelectedHandler m_segmentSelectedHandler;

	void LuminateBridgeWapper::stopFrameTracing();

public:
	bool Initialize(void* a_osHandle, int a_windowWidth, int a_windowHeight);
	bool Draw() { return m_luminateBridge->draw(); }
	bool Resize(int a_windowWidth, int a_windowHeight) { return m_luminateBridge->resize(a_windowWidth, a_windowHeight); }
	FrameStatistics getFrameStatistics() { return m_luminateBridge->getFrameStatistics(); }

	void ApplyMaterial();
	void Update();

};

