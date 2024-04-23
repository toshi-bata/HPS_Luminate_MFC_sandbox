#include "stdafx.h"
#include "LuminateBridgeWapper.h"
#include "hoops_license.h"

#include <hoops_luminate_bridge/LuminateRCTest.h>
#include <REDFactory.h>
#include <REDIShape.h>
#include <REDIREDFile.h>
#include <REDIDataManager.h>
#include <REDIMaterialController.h>
#include <REDIMaterialControllerProperty.h>

using namespace hoops_luminate_bridge;

LuminateBridgeWapper::LuminateBridgeWapper(CHPSView* view) :
	m_pHpsView(view), m_iLightingModeId(0), m_bSyncCamera(false)
{
	// Get HPS install dir
	char* path = getenv("HVISUALIZE_INSTALL_DIR");

	strcat(m_cHpsDir, path);

	int lastId = strlen(m_cHpsDir) - 1;
	if ('\\' != m_cHpsDir[lastId])
		strcat(m_cHpsDir, "\\");

}

LuminateBridgeWapper::~LuminateBridgeWapper()
{
	HPS::Database::GetEventDispatcher().UnSubscribe(HPS::Object::ClassID<HPS::HighlightEvent>());

	m_luminateBridge->shutdown();
}

bool LuminateBridgeWapper::Initialize(void* a_osHandle, int a_windowWidth, int a_windowHeight)
{
	// Get parameters
	m_iLightingModeId = m_pHpsView->GetLightingModeId();

	// Initialize Luminate HPS bridge
	HPS::View view = m_pHpsView->GetCanvas().GetFrontView();
	m_luminateBridge = new HoopsLuminateBridgeHPS(&view);

	std::string filepath = "";
	m_luminateBridge->initialize(HOOPS_LICENSE, a_osHandle, a_windowWidth, a_windowHeight, filepath);

	m_luminateBridge->syncScene();

	switch (m_iLightingModeId)
	{
	case 1: m_luminateBridge->setSunSkyLightEnvironment(); break;
	default: break;
	}

	// Setup event handler
	m_segmentSelectedHandler.setView(m_pHpsView, m_luminateBridge);
	HPS::Database::GetEventDispatcher().Subscribe(m_segmentSelectedHandler, HPS::Object::ClassID<HPS::HighlightEvent>());
	
	return m_luminateBridge->draw();
}

void LuminateBridgeWapper::stopFrameTracing()
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

void LuminateBridgeWapper::ApplyMaterial()
{
	// Get material
	RED::String redfilename = RED::String(m_cHpsDir);
	redfilename.Add(RED::String("samples\\hoops_luminate_widgets\\Resources\\MaterialLibrary\\"));

	int matTypeId = m_pHpsView->GetMateralTypeId();
	int matId = m_pHpsView->GetMateralId();
	switch (matTypeId)
	{
		case 0:
		{
			switch (matId)
			{
			case 0: redfilename.Add(RED::String("glass_clear_glass.red")); break;
			default: break;
			} break;
		}
		case 1:
		{
			switch (matId)
			{
			case 0: redfilename.Add(RED::String("jewellery_amber.red")); break;
			case 1: redfilename.Add(RED::String("jewellery_diamond_(white).red")); break;
			case 2: redfilename.Add(RED::String("jewellery_emerald_(green).red")); break;
			case 3: redfilename.Add(RED::String("jewellery_onyx.red")); break;
			case 4: redfilename.Add(RED::String("jewellery_pearl.red")); break;
			case 5: redfilename.Add(RED::String("jewellery_ruby.red")); break;
			case 6: redfilename.Add(RED::String("jewellery_sapphire_(blue).red")); break;
			case 7: redfilename.Add(RED::String("jewellery_topaz_(yellow).red")); break;
			default: break;
			} break;
		}
		case 2:
		{
			switch (matId)
			{
			case 0: redfilename.Add(RED::String("metal_brass.red")); break;
			case 1: redfilename.Add(RED::String("metal_brushed_gold.red")); break;
			case 2: redfilename.Add(RED::String("metal_chrome.red")); break;
			case 3: redfilename.Add(RED::String("metal_copper.red")); break;
			case 4: redfilename.Add(RED::String("metal_iron.red")); break;
			case 5: redfilename.Add(RED::String("metal_polished_gold.red")); break;
			case 6: redfilename.Add(RED::String("metal_silver.red")); break;
			case 7: redfilename.Add(RED::String("metal_titanium.red")); break;
			default: break;
			} break;
		}
		case 3:
		{
			switch (matId)
			{
				case 0: redfilename.Add(RED::String("plastic_mat_plastic.red")); break;
				case 1: redfilename.Add(RED::String("plastic_shiny_plastic.red")); break;
				case 2: redfilename.Add(RED::String("plastic_transparent_plastic.red")); break;
				default: break;
			} break;
		}
	}

	// Get potion
	bool bOverrideMaterial = m_pHpsView->GetOverrideMaterial();
	bool bPreserveColor = m_pHpsView->GetPreserveColor();

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

			RC_CHECK(ifile->Load(redfilename, iresourceManager->GetState(), policy, fheader, finfo, contexts));

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

			if (bPreserveColor)
			{
				// Duplicate the material controller
				RED_RC returnCode;
				RED::Object* materialController = iresourceManager->GetMaterialController(libraryMaterial);
				RED::Object* clonedMaterialController = RED::Factory::CreateMaterialController(*resourceManager,
					clonedMaterial,
					"Realistic",
					"",
					"Tunable realistic material",
					"Realistic",
					"Redway3d",
					returnCode);

				RC_CHECK(returnCode);
				RED::IMaterialController* clonedIMaterialController =
					clonedMaterialController->As<RED::IMaterialController>();
				RC_CHECK(clonedIMaterialController->CopyFrom(*materialController, clonedMaterial));

				// Set HPS segment diffuse color as Luminate diffuse + reflection.

				RED::Object* diffuseColorProperty = clonedIMaterialController->GetProperty(RED_MATCTRL_DIFFUSE_COLOR);
				RED::IMaterialControllerProperty* iDiffuseColorProperty =
					diffuseColorProperty->As<RED::IMaterialControllerProperty>();
				iDiffuseColorProperty->SetColor(m_luminateBridge->getSelectedSegmentInfo()->m_diffuseColor,
					iresourceManager->GetState());

			}

			RED::Object* currentMaterial;
			iShape->GetMaterial(currentMaterial);

			if (bOverrideMaterial)
			{
				iShape->SetMaterial(clonedMaterial, iresourceManager->GetState());
			}
			else
			{
				RED::IMaterial* currentIMaterial = currentMaterial->As<RED::IMaterial>();
				RC_CHECK(currentIMaterial->CopyFrom(*clonedMaterial, iresourceManager->GetState()));
			}

			m_luminateBridge->resetFrame();
		}
	}
}

void LuminateBridgeWapper::Update()
{
	// Sync camera
	if (m_bSyncCamera != m_pHpsView->GetSyncCamera())
	{
		m_bSyncCamera = m_pHpsView->GetSyncCamera();
		m_luminateBridge->setSyncCamera(m_bSyncCamera);
	}

	// Apply Lighting mode
	if (m_iLightingModeId != m_pHpsView->GetLightingModeId())
	{
		m_iLightingModeId = m_pHpsView->GetLightingModeId();

		switch (m_iLightingModeId)
		{
		case 0: m_luminateBridge->setDefaultLightEnvironment(); break;
		case 1: m_luminateBridge->setSunSkyLightEnvironment(); break;
		default: break;
		}
	}

	// Apply material
	if (m_pHpsView->GetSegmentSelected())
	{
		ApplyMaterial();
		m_pHpsView->SetSegmentSelected(false);
	}

}
