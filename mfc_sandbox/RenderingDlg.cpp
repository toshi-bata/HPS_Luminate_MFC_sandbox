﻿#include "stdafx.h"
#include "RenderingDlg.h"
#include "afxdialogex.h"

#include "hoops_license.h"
#include <hoops_luminate_bridge/LuminateRCTest.h>
#include <REDFactory.h>
#include <REDIShape.h>
#include <REDIREDFile.h>
#include <REDIDataManager.h>
#include <REDIMaterialController.h>

using namespace hoops_luminate_bridge;

IMPLEMENT_DYNAMIC(RenderingDlg, CDialogEx)

RED::Object* window;

RenderingDlg::RenderingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RENDERING_DIALOG, pParent),
	m_hpsView((CHPSView*)pParent),
	m_luminateBridge(nullptr),
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

	// Setup event handler
	m_segmentSelectedHandler.setView(m_hpsView);
	HPS::Database::GetEventDispatcher().Subscribe(m_segmentSelectedHandler, HPS::Object::ClassID<HPS::HighlightEvent>());

	return TRUE;  // return TRUE unless you set the focus to a control
}

void RenderingDlg::OnCancel()
{
	HPS::Database::GetEventDispatcher().UnSubscribe(HPS::Object::ClassID<HPS::HighlightEvent>());

	if (0 != m_timerID)
	{
		KillTimer(m_timerID);
		m_timerID = 0;
	}

	m_luminateBridge->shutdown();

	m_hpsView->GetCanvas().GetFrontView().GetOperatorControl().Pop();

	m_hpsView->SetProgPos(0);

	DestroyWindow();
}

void RenderingDlg::initLuminateBridge()
{
	// Initialize Luminate HPS bridge
	HPS::View view = m_hpsView->GetCanvas().GetFrontView();
	m_luminateBridge = new HoopsLuminateBridgeHPS(&view);

	RECT rcl;
	::GetClientRect(m_hWnd, &rcl);

	int width = rcl.right;
	int height = rcl.bottom;

	std::string filepath = "";
	m_luminateBridge->initialize(HOOPS_LICENSE, m_hWnd, width, height, filepath);

	m_luminateBridge->syncScene();

	m_luminateBridge->draw();

	m_segmentSelectedHandler.setLuninateBridge(m_luminateBridge);
}

void RenderingDlg::stopFrameTracing()
{
	RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
	RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();

	if (iresourceManager->GetState().GetNumber() > 1) {
		RED::IWindow* iwin = m_luminateBridge->getWindow()->As<RED::IWindow>();
		iwin->FrameTracingStop();

		iresourceManager->BeginState();
	}

	m_luminateBridge->resetFrame();
}

void RenderingDlg::applyMaterial()
{
	// Get material
	RED::String iMatPath;
	int matId = m_hpsView->GetMateralId();
	switch (matId)
	{
	case 0: iMatPath = "..\\Resources\\MaterialLibrary\\metal_brass.red"; break;
	case 1: iMatPath = "..\\Resources\\MaterialLibrary\\metal_brushed_gold.red"; break;
	case 2: iMatPath = "..\\Resources\\MaterialLibrary\\metal_chrome.red"; break;
	default: break;
	}
	
	// Apply material
	HoopsLuminateBridge* bridge = (HoopsLuminateBridge*)m_luminateBridge;
	RED::Object* selectedTransformNode = bridge->getSelectedLuminateTransformNode();

	if (selectedTransformNode != nullptr) {
		RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
		RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();
		RED::IShape* iShape = selectedTransformNode->As<RED::IShape>();

		RED::Object* libraryMaterial = nullptr;
		{
			// As a new material will be created, some images will be created as well.
			// Or images operations are immediate and should occur without ongoing rendering.
			// Thus we need to stop current frame tracing.
			stopFrameTracing();

			// create the file instance
			RED::Object* file = RED::Factory::CreateInstance(CID_REDFile);
			RED::IREDFile* ifile = file->As<RED::IREDFile>();

			// load the file
			RED::StreamingPolicy policy;
			RED::FileHeader fheader;
			RED::FileInfo finfo;
			RED::Vector< unsigned int > contexts;

			RC_CHECK(ifile->Load(iMatPath, iresourceManager->GetState(), policy, fheader, finfo, contexts));

			// release the file
			RC_CHECK(RED::Factory::DeleteInstance(file, iresourceManager->GetState()));

			// retrieve the data manager
			RED::IDataManager* idatamgr = iresourceManager->GetDataManager()->As<RED::IDataManager>();

			// parse the loaded contexts looking for the first material.
			for (unsigned int c = 0; c < contexts.size(); ++c) {
				unsigned int mcount;
				RC_CHECK(idatamgr->GetMaterialsCount(mcount, contexts[c]));

				if (mcount > 0) {
					RC_CHECK(idatamgr->GetMaterial(libraryMaterial, contexts[c], 0));
					break;
				}
			}
		}

		if (libraryMaterial != nullptr) {
			// Clone the material to be able to change its properties without altering the library one.
			RED::Object* clonedMaterial;
			RC_CHECK(iresourceManager->CloneMaterial(clonedMaterial, libraryMaterial, iresourceManager->GetState()));

			iShape->SetMaterial(clonedMaterial, iresourceManager->GetState());

			m_luminateBridge->resetFrame();
		}
	}
}

void RenderingDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nullptr == m_luminateBridge)
		initLuminateBridge();

	if (m_segmentSelectedHandler.m_bUpdated)
	{
		applyMaterial();
		m_segmentSelectedHandler.m_bUpdated = false;
	}

	FrameStatistics stats = m_luminateBridge->getFrameStatistics();
	int renderingProgress = stats.renderingProgress * 100.f;
	if (renderingProgress < 100)
	{
		m_hpsView->SetProgPos(renderingProgress);
		m_luminateBridge->draw();
	}
	else
	{
		KillTimer(m_timerID);
		m_timerID = 0;
	
		m_luminateBridge->draw();
	}

	CDialogEx::OnTimer(nIDEvent);
}


void RenderingDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (nullptr == m_luminateBridge)
		return;

	m_luminateBridge->resize(cx, cy);
}