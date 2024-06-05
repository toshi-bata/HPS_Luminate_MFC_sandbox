#include "stdafx.h"
#include "CHPSApp.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include "sprk.h"
#include "SandboxHighlightOp.h"
#include "CProgressDialog.h"
#include <WinUser.h>
#include <vector>
#include "RenderingDlg.h"

#ifdef USING_PUBLISH
#include "sprk_publish.h"
#endif

using namespace HPS;

IMPLEMENT_DYNCREATE(CHPSView, CView)

BEGIN_MESSAGE_MAP(CHPSView, CView)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_EDIT_COPY, &CHPSView::OnEditCopy)
	ON_COMMAND(ID_FILE_OPEN, &CHPSView::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE_AS, &CHPSView::OnFileSaveAs)
	ON_COMMAND(ID_OPERATORS_ORBIT, &CHPSView::OnOperatorsOrbit)
	ON_COMMAND(ID_OPERATORS_PAN, &CHPSView::OnOperatorsPan)
	ON_COMMAND(ID_OPERATORS_ZOOM_AREA, &CHPSView::OnOperatorsZoomArea)
	ON_COMMAND(ID_OPERATORS_FLY, &CHPSView::OnOperatorsFly)
	ON_COMMAND(ID_OPERATORS_HOME, &CHPSView::OnOperatorsHome)
	ON_COMMAND(ID_OPERATORS_ZOOM_FIT, &CHPSView::OnOperatorsZoomFit)
	ON_COMMAND(ID_OPERATORS_SELECT_POINT, &CHPSView::OnOperatorsSelectPoint)
	ON_COMMAND(ID_OPERATORS_SELECT_AREA, &CHPSView::OnOperatorsSelectArea)
	ON_COMMAND(ID_MODES_SIMPLE_SHADOW, &CHPSView::OnModesSimpleShadow)
	ON_COMMAND(ID_MODES_SMOOTH, &CHPSView::OnModesSmooth)
	ON_COMMAND(ID_MODES_HIDDEN_LINE, &CHPSView::OnModesHiddenLine)
	ON_COMMAND(ID_MODES_EYE_DOME_LIGHTING, &CHPSView::OnModesEyeDome)
	ON_COMMAND(ID_MODES_FRAME_RATE, &CHPSView::OnModesFrameRate)
	ON_COMMAND(ID_USER_CODE_1, &CHPSView::OnUserCode1)
	ON_COMMAND(ID_USER_CODE_2, &CHPSView::OnUserCode2)
	ON_COMMAND(ID_USER_CODE_3, &CHPSView::OnUserCode3)
	ON_COMMAND(ID_USER_CODE_4, &CHPSView::OnUserCode4)
	ON_WM_MOUSELEAVE()
	ON_UPDATE_COMMAND_UI(ID_MODES_SIMPLE_SHADOW, &CHPSView::OnUpdateRibbonBtnSimpleShadow)
	ON_UPDATE_COMMAND_UI(ID_MODES_FRAME_RATE, &CHPSView::OnUpdateRibbonBtnFrameRate)
	ON_UPDATE_COMMAND_UI(ID_MODES_SMOOTH, &CHPSView::OnUpdateRibbonBtnSmooth)
	ON_UPDATE_COMMAND_UI(ID_MODES_HIDDEN_LINE, &CHPSView::OnUpdateRibbonBtnHiddenLine)
	ON_UPDATE_COMMAND_UI(ID_MODES_EYE_DOME_LIGHTING, &CHPSView::OnUpdateRibbonBtnEyeDome)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_ORBIT, &CHPSView::OnUpdateRibbonBtnOrbitOp)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_PAN, &CHPSView::OnUpdateRibbonBtnPanOp)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_ZOOM_AREA, &CHPSView::OnUpdateRibbonBtnZoomAreaOp)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_FLY, &CHPSView::OnUpdateRibbonBtnFlyOp)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_SELECT_POINT, &CHPSView::OnUpdateRibbonBtnPointOp)
	ON_UPDATE_COMMAND_UI(ID_OPERATORS_SELECT_AREA, &CHPSView::OnUpdateRibbonBtnAreaOp)
	ON_COMMAND(ID_BUTTON_START, &CHPSView::OnButtonStart)
	ON_COMMAND(ID_CHECK_PRESERVE_COLOR, &CHPSView::OnCheckPreserveColor)
	ON_UPDATE_COMMAND_UI(ID_CHECK_PRESERVE_COLOR, &CHPSView::OnUpdateCheckPreserveColor)
	ON_COMMAND(ID_CHECK_OVERRIDE_MATERIAL, &CHPSView::OnCheckOverrideMaterial)
	ON_UPDATE_COMMAND_UI(ID_CHECK_OVERRIDE_MATERIAL, &CHPSView::OnUpdateCheckOverrideMaterial)
	ON_COMMAND(ID_CHECK_SYNC_CAMERA, &CHPSView::OnCheckSyncCamera)
	ON_UPDATE_COMMAND_UI(ID_CHECK_SYNC_CAMERA, &CHPSView::OnUpdateCheckSyncCamera)
	ON_COMMAND(ID_BUTTON_LOAD_ENV_MAP, &CHPSView::OnButtonLoadEnvMap)
END_MESSAGE_MAP()

CHPSView::CHPSView()
	: _enableSimpleShadows(false),
	_enableFrameRate(false),
	_displayResourceMonitor(false),
	_capsLockState(false),
	_eyeDome(false)
{
	_operatorStates[SandboxOperators::OrbitOperator] = true;
	_operatorStates[SandboxOperators::PanOperator] = false;
	_operatorStates[SandboxOperators::ZoomAreaOperator] = false;
	_operatorStates[SandboxOperators::PointOperator] = false;
	_operatorStates[SandboxOperators::AreaOperator] = false;

	_capsLockState = IsCapsLockOn();

	strcpy(m_cHdriFilePath, "");
}

CHPSView::~CHPSView()
{
	_canvas.Delete();
}

BOOL CHPSView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Setup Window class to work with HPS rendering.  The REDRAW flags prevent
	// flickering when resizing, and OWNDC allocates a single device context to for this window.
	cs.lpszClass = AfxRegisterWndClass(CS_OWNDC|CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW);
	return CView::PreCreateWindow(cs);
}

static bool has_called_initial_update = false;

void CHPSView::OnInitialUpdate()
{
	has_called_initial_update = true;

	// Only perform HPS::Canvas initialization once since we're reusing the same MFC Window each time.
	if (_canvas.Type() == HPS::Type::None)
	{
		//! [window_handle]
		// Setup to use the DX11 driver
		HPS::ApplicationWindowOptionsKit		windowOpts;
		windowOpts.SetDriver(HPS::Window::Driver::Default3D);

		// Create our Sprockets Canvas with the specified driver
		_canvas = HPS::Factory::CreateCanvas(reinterpret_cast<HPS::WindowHandle>(m_hWnd), "mfc_sandbox_canvas", windowOpts);
		//! [window_handle]
		
		//! [attach_view]
		// Create a new Sprockets View and attach it to our Sprockets.Canvas
		HPS::View view = HPS::Factory::CreateView("mfc_sandbox_view");
		_canvas.AttachViewAsLayout(view);
		//! [attach_view]

		// Setup scene startup values
		SetupSceneDefaults();
	}

	UpdateEyeDome(false);

	_canvas.Update(HPS::Window::UpdateType::Refresh);

	HPS::Rendering::Mode mode = GetCanvas().GetFrontView().GetRenderingMode();
	UpdateRenderingMode(mode);
}

void CHPSView::UpdateEyeDome(bool update)
{
	HPS::WindowKey window = _canvas.GetWindowKey();
	window.GetPostProcessEffectsControl().SetEyeDomeLighting(_eyeDome);

	auto visual_effects_control = window.GetVisualEffectsControl();
	visual_effects_control.SetEyeDomeLightingEnabled(_eyeDome);

	if (update)
		_canvas.Update(HPS::Window::UpdateType::Refresh);
}

void CHPSView::OnPaint()
{
	//! [on_paint]
	// Update our HPS Canvas.  A refresh is needed as the window size may have changed.
	if (has_called_initial_update)
		_canvas.Update(HPS::Window::UpdateType::Refresh);
	//! [on_paint]

	// Invoke BeginPaint/EndPaint.  This must be called when providing OnPaint handler.
	CPaintDC dc(this);
}

void CHPSView::SetMainDistantLight(HPS::Vector const & lightDirection)
{
	HPS::DistantLightKit	light;
	light.SetDirection(lightDirection);
	light.SetCameraRelative(true);
	SetMainDistantLight(light);
}

void CHPSView::SetMainDistantLight(HPS::DistantLightKit const & light)
{
	// Delete previous light before inserting new one
	if (_mainDistantLight.Type() != HPS::Type::None)
		_mainDistantLight.Delete();
	_mainDistantLight = GetCanvas().GetFrontView().GetSegmentKey().InsertDistantLight(light);
}

void CHPSView::SetupSceneDefaults()
{
	// Attach CHPSDoc created model
	GetCanvas().GetFrontView().AttachModel(GetDocument()->GetModel());

	// Set default operators.
	SetupDefaultOperators();

	// Subscribe _errorHandler to handle errors
	HPS::Database::GetEventDispatcher().Subscribe(_errorHandler, HPS::Object::ClassID<HPS::ErrorEvent>());

	// Subscribe _warningHandler to handle warnings
	HPS::Database::GetEventDispatcher().Subscribe(_warningHandler, HPS::Object::ClassID<HPS::WarningEvent>());

	// AxisTriad
	View view = GetCanvas().GetFrontView();
	view.GetAxisTriadControl().SetVisibility(true).SetInteractivity(true);
	GetCanvas().Update();
}

//! [setup_operators]
void CHPSView::SetupDefaultOperators()
{
	// Orbit is on top and will be replaced when the operator is changed
	GetCanvas().GetFrontView().GetOperatorControl()
		.Push(new HPS::MouseWheelOperator(), Operator::Priority::Low)
		.Push(new HPS::ZoomOperator(MouseButtons::ButtonMiddle()))
		.Push(new HPS::PanOperator(MouseButtons::ButtonRight()))
		.Push(new HPS::OrbitOperator(MouseButtons::ButtonLeft()));
}
//! [setup_operators]

void CHPSView::ActivateCapture(HPS::Capture & capture)
{
	HPS::View newView = capture.Activate();
	auto newViewSegment = newView.GetSegmentKey();
	HPS::CameraKit newCamera;
	newViewSegment.ShowCamera(newCamera);

	newCamera.UnsetNearLimit();
	auto defaultCameraWithoutNearLimit = HPS::CameraKit::GetDefault().UnsetNearLimit();
	if (newCamera == defaultCameraWithoutNearLimit)
	{
		HPS::View oldView = GetCanvas().GetFrontView();
		HPS::CameraKit oldCamera;
		oldView.GetSegmentKey().ShowCamera(oldCamera);

		newViewSegment.SetCamera(oldCamera);
		newView.FitWorld();
	}

	AttachViewWithSmoothTransition(newView);
}

#ifdef USING_EXCHANGE
void CHPSView::ActivateCapture(HPS::ComponentPath & capture_path)
{
	HPS::Exchange::Capture capture = (HPS::Exchange::Capture)(capture_path.Front());
	HPS::View newView = capture.Activate(capture_path);
	auto newViewSegment = newView.GetSegmentKey();
	HPS::CameraKit newCamera;
	newViewSegment.ShowCamera(newCamera);

	newCamera.UnsetNearLimit();
	auto defaultCameraWithoutNearLimit = HPS::CameraKit::GetDefault().UnsetNearLimit();
	if (newCamera == defaultCameraWithoutNearLimit)
	{
		HPS::View oldView = GetCanvas().GetFrontView();
		HPS::CameraKit oldCamera;
		oldView.GetSegmentKey().ShowCamera(oldCamera);

		newViewSegment.SetCamera(oldCamera);
		newView.FitWorld();
	}

	AttachViewWithSmoothTransition(newView);
}
#endif

void CHPSView::AttachView(HPS::View & newView)
{
	HPS::CADModel cadModel = GetCHPSDoc()->GetCADModel();
	if (!cadModel.Empty())
	{
		cadModel.ResetVisibility(_canvas);
		_canvas.GetWindowKey().GetHighlightControl().UnhighlightEverything();
	}

	_preZoomToKeyPathCamera.Reset();
	_zoomToKeyPath.Reset();

	HPS::View oldView = GetCanvas().GetFrontView();

	GetCanvas().AttachViewAsLayout(newView);

	HPS::OperatorPtrArray operators;
	auto oldViewOperatorCtrl = oldView.GetOperatorControl();
	auto newViewOperatorCtrl = newView.GetOperatorControl();
	oldViewOperatorCtrl.Show(HPS::Operator::Priority::Low, operators);
	newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::Low);
	oldViewOperatorCtrl.Show(HPS::Operator::Priority::Default, operators);
	newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::Default);
	oldViewOperatorCtrl.Show(HPS::Operator::Priority::High, operators);
	newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::High);

	SetMainDistantLight();

	oldView.Delete();
}

void CHPSView::AttachViewWithSmoothTransition(HPS::View & newView)
{
	HPS::View oldView = GetCanvas().GetFrontView();
	HPS::CameraKit oldCamera;
	oldView.GetSegmentKey().ShowCamera(oldCamera);

	auto newViewSegment = newView.GetSegmentKey();
	HPS::CameraKit newCamera;
	newViewSegment.ShowCamera(newCamera);

	AttachView(newView);

	newViewSegment.SetCamera(oldCamera);
	newView.SmoothTransition(newCamera);
}

void CHPSView::Update()
{
	_canvas.Update();
}

void CHPSView::Unhighlight()
{
	HPS::HighlightOptionsKit highlightOptions;
	highlightOptions.SetStyleName("highlight_style").SetNotification(true);

	_canvas.GetWindowKey().GetHighlightControl().Unhighlight(highlightOptions);
	HPS::Database::GetEventDispatcher().InjectEvent(HPS::HighlightEvent(HPS::HighlightEvent::Action::Unhighlight, HPS::SelectionResults(), highlightOptions));
	HPS::Database::GetEventDispatcher().InjectEvent(HPS::ComponentHighlightEvent(HPS::ComponentHighlightEvent::Action::Unhighlight, GetCanvas(), 0, HPS::ComponentPath(), highlightOptions));
}

void CHPSView::UnhighlightAndUpdate()
{
	Unhighlight();
	Update();
}

void CHPSView::ZoomToKeyPath(HPS::KeyPath const & keyPath)
{
	HPS::BoundingKit bounding;
	if (keyPath.ShowNetBounding(true, bounding))
	{
		_zoomToKeyPath = keyPath;

		HPS::View frontView = _canvas.GetFrontView();
		frontView.GetSegmentKey().ShowCamera(_preZoomToKeyPathCamera);

		HPS::CameraKit fittedCamera;
		frontView.ComputeFitWorldCamera(bounding, fittedCamera);
		frontView.SmoothTransition(fittedCamera);
	}
}

void CHPSView::RestoreCamera()
{
	if (!_preZoomToKeyPathCamera.Empty())
	{
		_canvas.GetFrontView().SmoothTransition(_preZoomToKeyPathCamera);
		HPS::Database::Sleep(500);

		InvalidateZoomKeyPath();
		InvalidateSavedCamera();
	}
}

void CHPSView::InvalidateZoomKeyPath()
{
	_zoomToKeyPath.Reset();
}

void CHPSView::InvalidateSavedCamera()
{
	_preZoomToKeyPathCamera.Reset();
}

HPS::ModifierKeys CHPSView::MapModifierKeys(UINT flags)
{
	HPS::ModifierKeys	modifier;

	// Map shift and control modifiers to HPS::InputEvent modifiers
	if ((flags & MK_SHIFT) != 0)
	{
		if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
			modifier.LeftShift(true);
		else
			modifier.RightShift(true);
	}

	if ((flags & MK_CONTROL) != 0)
	{
		if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
			modifier.LeftControl(true);
		else
			modifier.RightControl(true);
	}

	if (_capsLockState)
		modifier.CapsLock(true);

	return modifier;
}

HPS::MatrixKit const & CHPSView::GetPixelToWindowMatrix()
{
	CRect currentWindowRect;
	GetWindowRect(&currentWindowRect);
	ScreenToClient(&currentWindowRect);

	if (_windowRect.IsRectNull() || !EqualRect(&_windowRect, &currentWindowRect))
	{
		CopyRect(&_windowRect, &currentWindowRect);
		KeyArray key_array;
		key_array.push_back(_canvas.GetWindowKey());
		KeyPath(key_array).ComputeTransform(HPS::Coordinate::Space::Pixel, HPS::Coordinate::Space::Window, _pixelToWindowMatrix);
	}

	return _pixelToWindowMatrix;
}

//! [build_mouse_event]
HPS::MouseEvent CHPSView::BuildMouseEvent(HPS::MouseEvent::Action action, HPS::MouseButtons buttons, CPoint cpoint, UINT flags, size_t click_count, float scalar)
{
	// Convert location to window space
	HPS::Point				p(static_cast<float>(cpoint.x), static_cast<float>(cpoint.y), 0);
	p = GetPixelToWindowMatrix().Transform(p);

	if(action == HPS::MouseEvent::Action::Scroll)
		return HPS::MouseEvent(action, scalar, p, MapModifierKeys(flags), click_count); 
	else
		return HPS::MouseEvent(action, p, buttons, MapModifierKeys(flags), click_count);
}
//! [build_mouse_event]

//! [left_button_down]
void CHPSView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonLeft(), point, nFlags, 1));

	CView::OnLButtonDown(nFlags, point);
}
//! [left_button_down]

void CHPSView::OnLButtonUp(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonUp, HPS::MouseButtons::ButtonLeft(), point, nFlags, 0));

	ReleaseCapture();
	SetCursor(theApp.LoadStandardCursor(IDC_ARROW));

	CView::OnLButtonUp(nFlags, point);
}

void CHPSView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonLeft(), point, nFlags, 2));

	ReleaseCapture();

	CView::OnLButtonUp(nFlags, point);
}

void CHPSView::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect wndRect;
	GetWindowRect(&wndRect);
	ScreenToClient(&wndRect);

	if (wndRect.PtInRect(point) || (nFlags & MK_LBUTTON) || (nFlags & MK_RBUTTON))
	{
		_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
			BuildMouseEvent(HPS::MouseEvent::Action::Move, HPS::MouseButtons(), point, nFlags, 0));
	}
	else
	{
		if(!(nFlags & MK_LBUTTON) && GetCapture() != NULL)
			OnLButtonUp(nFlags, point);
		if(!(nFlags & MK_RBUTTON) && GetCapture() != NULL)
			OnRButtonUp(nFlags, point);
	}

	CView::OnMouseMove(nFlags, point);
}

void CHPSView::OnRButtonDown(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonRight(), point, nFlags, 1));

	CView::OnRButtonDown(nFlags, point);
}

void CHPSView::OnRButtonUp(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonUp, HPS::MouseButtons::ButtonRight(), point, nFlags, 0));

	SetCursor(theApp.LoadStandardCursor(IDC_ARROW));

	CView::OnRButtonUp(nFlags, point);
}

void CHPSView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonRight(), point, nFlags, 2));

	ReleaseCapture();

	CView::OnLButtonUp(nFlags, point);
}

void CHPSView::OnMButtonDown(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonMiddle(), point, nFlags, 1));

	CView::OnMButtonDown(nFlags, point);
}

void CHPSView::OnMButtonUp(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonUp, HPS::MouseButtons::ButtonMiddle(), point, nFlags, 0));

	SetCursor(theApp.LoadStandardCursor(IDC_ARROW));

	CView::OnMButtonUp(nFlags, point);
}

void CHPSView::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::ButtonDown, HPS::MouseButtons::ButtonMiddle(), point, nFlags, 2));

	ReleaseCapture();

	CView::OnLButtonUp(nFlags, point);
}

BOOL CHPSView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	HPS::WindowHandle hwnd;
	static_cast<ApplicationWindowKey>(_canvas.GetWindowKey()).GetWindowOptionsControl().ShowWindowHandle(hwnd);
	::ScreenToClient(reinterpret_cast<HWND>(hwnd), &point);
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(
		BuildMouseEvent(HPS::MouseEvent::Action::Scroll, HPS::MouseButtons(), point, nFlags, 0, static_cast<float>(zDelta)));

	return CView::OnMouseWheel(nFlags, zDelta, point);
}

void CHPSView::UpdateOperatorStates(SandboxOperators op)
{
	for (auto it = _operatorStates.begin(), e = _operatorStates.end(); it != e; ++it)
		(*it).second = false;

	_operatorStates[op] = true;
}

void CHPSView::OnOperatorsOrbit()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::OrbitOperator(MouseButtons::ButtonLeft()));
	UpdateOperatorStates(SandboxOperators::OrbitOperator);
}


void CHPSView::OnOperatorsPan()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::PanOperator(MouseButtons::ButtonLeft()));
	UpdateOperatorStates(SandboxOperators::PanOperator);
}


void CHPSView::OnOperatorsZoomArea()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::ZoomBoxOperator(MouseButtons::ButtonLeft()));
	UpdateOperatorStates(SandboxOperators::ZoomAreaOperator);
}


void CHPSView::OnOperatorsFly()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::FlyOperator());
	UpdateOperatorStates(SandboxOperators::FlyOperator);
}

//! [OnOperatorsHome]
void CHPSView::OnOperatorsHome()
{
	CHPSDoc * doc = GetCHPSDoc();
	HPS::CADModel cadModel = doc->GetCADModel();
	try
	{
		if (!cadModel.Empty())
			AttachViewWithSmoothTransition(cadModel.ActivateDefaultCapture().FitWorld());
		else
			_canvas.GetFrontView().SmoothTransition(doc->GetDefaultCamera());
	}
	catch(HPS::InvalidSpecificationException const &)
	{
		// SmoothTransition() can throw if there is no model attached
	}
}
//! [OnOperatorsHome]

//! [OnOperatorsZoomFit]
void CHPSView::OnOperatorsZoomFit()
{
	HPS::View frontView = GetCanvas().GetFrontView();
	HPS::CameraKit zoomFitCamera;
	if (!_zoomToKeyPath.Empty())
	{
		HPS::BoundingKit bounding;
		_zoomToKeyPath.ShowNetBounding(true, bounding);
		frontView.ComputeFitWorldCamera(bounding, zoomFitCamera);
	}
	else
		frontView.ComputeFitWorldCamera(zoomFitCamera);
	frontView.SmoothTransition(zoomFitCamera);
}
//! [OnOperatorsZoomFit]

//! [enable_point_select_operator]
void CHPSView::OnOperatorsSelectPoint()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new SandboxHighlightOperator(this));
	UpdateOperatorStates(SandboxOperators::PointOperator);
}
//! [enable_point_select_operator]


void CHPSView::OnOperatorsSelectArea()
{
	GetCanvas().GetFrontView().GetOperatorControl().Pop();
	GetCanvas().GetFrontView().GetOperatorControl().Push(new HPS::HighlightAreaOperator(MouseButtons::ButtonLeft()));
	UpdateOperatorStates(SandboxOperators::AreaOperator);
}


//! [OnModesSimpleShadow]
void CHPSView::OnModesSimpleShadow()
{
	// Toggle state and bail early if we're disabling
	_enableSimpleShadows = !_enableSimpleShadows;
	if (_enableSimpleShadows == false)
	{
		GetCanvas().GetFrontView().GetSegmentKey().GetVisualEffectsControl().SetSimpleShadow(false);
		_canvas.Update();
		return;
	}

	UpdatePlanes();
}
//! [OnModesSimpleShadow]


//! [OnModesSmooth]
void CHPSView::OnModesSmooth()
{
	if (!_smoothRendering)
	{
		_canvas.GetFrontView().SetRenderingMode(Rendering::Mode::Phong);

		if (GetCHPSDoc()->GetCADModel().Type() == HPS::Type::DWGCADModel)
			_canvas.GetFrontView().GetSegmentKey().GetVisibilityControl().SetLines(true);


		_canvas.Update();
		_smoothRendering = true;
	}
}
//! [OnModesSmooth]

//! [OnModesHiddenLine]
void CHPSView::OnModesHiddenLine()
{
	if (_smoothRendering)
	{
		_canvas.GetFrontView().SetRenderingMode(Rendering::Mode::FastHiddenLine);
		_canvas.Update();
		_smoothRendering = false;

		// fixed framerate is not compatible with hidden line mode
		if (_enableFrameRate)
		{
			_canvas.SetFrameRate(0);
			_enableFrameRate = false;
		}
	}
}
//! [OnModesHiddenLine]

//! [OnModesEyeDome]
void CHPSView::OnModesEyeDome()
{
	_eyeDome = !_eyeDome;

	UpdateEyeDome(true);
}
//! [OnModesEyeDome]

//! [OnModesFrameRate]

void CHPSView::OnModesFrameRate()
{
	const float					frameRate = 20.0f;

	// Toggle frame rate and set.  Note that 0 disables frame rate.
	_enableFrameRate = !_enableFrameRate;
	if (_enableFrameRate)
		_canvas.SetFrameRate(frameRate);
	else
		_canvas.SetFrameRate(0);

	// fixed framerate is not compatible with hidden line mode
	if(!_smoothRendering)
	{
		_canvas.GetFrontView().SetRenderingMode(Rendering::Mode::Phong);
		_smoothRendering = true;
	}

	_canvas.Update();
}
//! [OnModesFrameRate]



void CHPSView::OnEditCopy()
{
	HPS::Hardcopy::GDI::ExportOptionsKit exportOptionsKit;
	HPS::Hardcopy::GDI::ExportClipboard(GetCanvas().GetWindowKey(), exportOptionsKit);
}

void CHPSView::OnFileOpen()
{
	CString filter = _T("HOOPS Stream Files (*.hsf)|*.hsf|StereoLithography Files (*.stl)|*.stl|WaveFront Files (*.obj)|*.obj|Point Cloud Files (*.ptx, *.pts, *.xyz)|*.ptx;*.pts;*.xyz|");
#ifdef USING_EXCHANGE
	auto const create_exchange_filter = []() {
		using ExchangeFilterDesc = std::pair<CString, std::vector<CString>>;
        std::vector<ExchangeFilterDesc> const exchange_files_list {
			{_T("3D Studio Files"),				{_T("*.3ds")}},
			{_T("3D Manufacturing Files"),		{_T("*.3mf")}},
			{_T("3DXML Files"),					{_T("*.3dxml")}},
			{_T("ACIS SAT Files"),				{_T("*.sat"), _T("*.sab")}},
			{_T("CADDS Files"),					{_T("*_pd")}},
			{_T("CATIA V4 Files"),				{_T("*.model"), _T("*.dlv"), _T("*.exp"), _T("*.session")}},
			{_T("CATIA V5 Files"),				{_T("*.CATPart"), _T("*.CATProduct"), _T("*.CATShape"), _T("*.CATDrawing")}},
			{_T("CGR Files"),					{_T("*.cgr")}},
			{_T("Collada Files"),				{_T("*.dae")}},
			{_T("Creo (ProE) Files"),			{_T("*.prt"), _T("*.prt.*"), _T("*.neu"), _T("*.neu.*"), _T("*.asm"), _T("*.asm.*"), _T("*.xas"), _T("*.xpr")}},
			{_T("FBX Files"),					{_T("*.fbx")}},
			{_T("GLTF Files"),					{_T("*.gltf"), _T("*.glb")}},
			{_T("I-DEAS Files"),				{_T("*.arc"), _T("*.unv"), _T("*.mf1"), _T("*.prt"), _T("*.pkg")}},
			{_T("IFC Files"),					{_T("*.ifc"), _T("*.ifczip")}},
			{_T("IGES Files"),					{_T("*.igs"), _T("*.iges")}},
			{_T("Inventor Files"),				{_T("*.ipt"), _T("*.iam")}},
			{_T("JT Files"),					{_T("*.jt")}},
			{_T("KMZ Files"),					{_T("*.kmz")}},
			{_T("Navisworks Files"),			{_T("*.nwd")}},
			{_T("NX (Unigraphics) Files"),		{_T("*.prt")}},
			{_T("PDF Files"),					{_T("*.pdf")}},
			{_T("PRC Files"),					{_T("*.prc")}},
			{_T("Parasolid Files"),				{_T("*.x_t"), _T("*.xmt"), _T("*.x_b"), _T("*.xmt_txt")}},
			{_T("Revit Files"),					{_T("*.rvt")}},
			{_T("Rhino Files"),					{_T("*.3dm")}},
			{_T("SolidEdge Files"),				{_T("*.par"), _T("*.asm"), _T("*.pwd"), _T("*.psm")}},
			{_T("SolidWorks Files"),			{_T("*.sldprt"), _T("*.sldasm"), _T("*.sldfpp"), _T("*.asm")}},
			{_T("STEP Files"),					{_T("*.stp"), _T("*.step"), _T("*.stpz"), _T("*.stp.z")}},
			{_T("STL Files"),					{_T("*.stl")}},
			{_T("Universal 3D Files"),			{_T("*.u3d")}},
			{_T("VDA Files"),					{_T("*.vda")}},
			{_T("VRML Files"),					{_T("*.wrl"), _T("*.vrml")}},
			{_T("XVL Files"),					{_T("*.xv3"), _T("*.xv0")}}
		};

		auto const concatenate = [](std::vector<CString> const& a_list, CString const& a_separator) -> CString {
			CString result;
            bool first_pass = true;
            for (auto const& item: a_list) {
                if (first_pass == false) {
					result += a_separator;
				}
				result += item;
                first_pass = false;
			}
            return result;
        };

        auto const concatenate_all_extensions = [&exchange_files_list, concatenate](CString const& a_separator) {
            CString result;
            bool first_pass = true;
            for (auto const& file_desc: exchange_files_list) {
                if (first_pass == false) {
                    result += a_separator;
                }
                result += concatenate(file_desc.second, a_separator);
				first_pass = false;
            }
            return result;
        };

		CString exchange_filter = _T("All CAD Files (");
        exchange_filter += concatenate_all_extensions(_T(", "));
        exchange_filter += _T(")|");
        exchange_filter += concatenate_all_extensions(_T(";"));
		
		for (auto const& file_desc: exchange_files_list) {
            exchange_filter += _T("|") + file_desc.first + _T(" (");
            exchange_filter += concatenate(file_desc.second, _T(", "));
            exchange_filter += _T(")|");
            exchange_filter += concatenate(file_desc.second, _T(";"));
        }

		exchange_filter += _T("|");
		return exchange_filter;
	};
    auto const exchange_filter = create_exchange_filter();
	CString filter_org = filter;
	filter = _T("");
	filter.Append(exchange_filter);
	filter.Append(filter_org);
#endif

#if defined(USING_PARASOLID)
	CString parasolid_filter = _T("Parasolid Files (*.x_t, *.x_b, *.xmt_txt, *.xmt_bin)|*.x_t;*.x_b;*.xmt_txt;*.xmt_bin|");
	filter.Append(parasolid_filter);
#endif

#if defined (USING_DWG)
	CString dwg_filter = _T("DWG Files (*.dwg, *.dxf)|*.dwg;*.dxf|");
	filter.Append(dwg_filter);
#endif

	CString all_files = _T("All Files (*.*)|*.*||");
	filter.Append(all_files);
	CFileDialog dlg(TRUE, _T(".hsf"), NULL, OFN_HIDEREADONLY, filter, NULL);

	if (dlg.DoModal() == IDOK)
		GetDocument()->OnOpenDocument(dlg.GetPathName());

	// AxisTriad and NaviCube
	View view = GetCanvas().GetFrontView();
	view.GetAxisTriadControl().SetVisibility(true).SetInteractivity(true);
	GetCanvas().Update();
}

void CHPSView::OnFileSaveAs()
{
	CString filter = _T("HOOPS Stream Files (*.hsf)|*.hsf|PDF (*.pdf)|*.pdf|Postscript Files (*.ps)|*.ps|JPEG Image File(*.jpeg)|*.jpeg|PNG Image Files (*.png)|*.png||");
	CFileDialog dlg(FALSE, _T(".hsf"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN, filter, NULL);

	if (dlg.DoModal())
	{
		CString pathname;
		pathname = dlg.GetPathName();
		CStringA ansiPath(pathname);

		char ext[5];
		ext[0]= '\0';

		char const * tmp = ::strrchr(ansiPath, '.');
		if(!tmp)
		{
			strcpy_s(ext, ansiPath);
			return;
		}

		++tmp;
		size_t len = strlen(tmp);
		for (size_t i = 0; i <= len; ++i){
			ext[i] = static_cast<char>(tolower(tmp[i]));
		}

		if (strcmp(ext, "hsf") == 0)
		{
			//! [export_hsf]
			HPS::Stream::ExportOptionsKit eok;
			HPS::SegmentKey exportFromHere;

			HPS::Model model = _canvas.GetFrontView().GetAttachedModel();
			if (model.Type() == HPS::Type::None)
				exportFromHere = _canvas.GetFrontView().GetSegmentKey();
			else
				exportFromHere = model.GetSegmentKey();

			HPS::Stream::ExportNotifier notifier;
			HPS::IOResult status = IOResult::Failure;
			try
			{
				notifier = HPS::Stream::File::Export(ansiPath, exportFromHere, eok); 
				CProgressDialog edlg(GetDocument(), notifier, true);
				edlg.DoModal();
				status = notifier.Status();
			}
			//! [export_hsf]
			catch (HPS::IOException const & e)
			{
				char what[1024];
				sprintf_s(what, 1024, "HPS::Stream::File::Export threw an exception: %s", e.what());
				MessageBoxA(NULL, what, NULL, 0);
			}
			if (status != HPS::IOResult::Success && status != HPS::IOResult::Canceled)
				GetParentFrame()->MessageBox(L"HPS::Stream::File::Export encountered an error", _T("File export error"), MB_ICONERROR | MB_OK);

		}
		else if (strcmp(ext, "pdf") == 0)
		{
			bool export_2d_pdf = true;
#ifdef USING_PUBLISH
			PDFExportDialog pdf_export;
			if (pdf_export.DoModal() == IDOK)
				export_2d_pdf = pdf_export.Export2DPDF();
			else
				return;

			if (!export_2d_pdf)
			{
				//! [export_3d_pdf]
				try 
				{
					HPS::Publish::ExportOptionsKit export_kit;
					CADModel cad_model = GetDocument()->GetCADModel();
					if (cad_model.Type() == HPS::Type::ExchangeCADModel)
						HPS::Publish::File::ExportPDF(cad_model, ansiPath, export_kit);
					else
					{
						HPS::SprocketPath sprocket_path(GetCanvas(), GetCanvas().GetAttachedLayout(), GetCanvas().GetFrontView(), GetCanvas().GetFrontView().GetAttachedModel());
						HPS::Publish::File::ExportPDF(sprocket_path.GetKeyPath(), ansiPath, export_kit);
					}
				}
				catch (HPS::IOException const & e)
				{
					char what[1024];
					sprintf_s(what, 1024, "HPS::Publish::File::Export threw an exception: %s", e.what());
					MessageBoxA(NULL, what, NULL, 0);
				}
				//! [export_3d_pdf]
			}
#endif
			if (export_2d_pdf)
			{
				//! [export_2d_pdf]
				try 
				{
					HPS::Hardcopy::File::Export(ansiPath, HPS::Hardcopy::File::Driver::PDF,
								_canvas.GetWindowKey(), HPS::Hardcopy::File::ExportOptionsKit::GetDefault());
				}
				catch (HPS::IOException const & e)
				{
					char what[1024];
					sprintf_s(what, 1024, "HPS::Hardcopy::File::Export threw an exception: %s", e.what());
					MessageBoxA(NULL, what, NULL, 0);
				}
				//! [export_2d_pdf]
			}
		}
		else if (strcmp(ext, "ps") == 0)
		{
			try 
			{
				HPS::Hardcopy::File::Export(ansiPath, HPS::Hardcopy::File::Driver::Postscript,
						_canvas.GetWindowKey(), HPS::Hardcopy::File::ExportOptionsKit::GetDefault());
			}
			catch (HPS::IOException const & e)
			{
				char what[1024];
				sprintf_s(what, 1024, "HPS::Hardcopy::File::Export threw an exception: %s", e.what());
				MessageBoxA(NULL, what, NULL, 0);
			}
		}
		else if (strcmp(ext, "jpeg") == 0)
		{
			HPS::Image::ExportOptionsKit eok;
			eok.SetFormat(Image::Format::Jpeg);

			try { HPS::Image::File::Export(ansiPath, _canvas.GetWindowKey(), eok); }
			catch (HPS::IOException const & e)
			{
				char what[1024];
				sprintf_s(what, 1024, "HPS::Image::File::Export threw an exception: %s", e.what());
				MessageBoxA(NULL, what, NULL, 0);
			}
		}
		else if (strcmp(ext, "png") == 0)
		{
			try { HPS::Image::File::Export(ansiPath, _canvas.GetWindowKey(), HPS::Image::ExportOptionsKit::GetDefault()); }
			catch (HPS::IOException const & e)
			{
				char what[1024];
				sprintf_s(what, 1024, "HPS::Image::File::Export threw an exception: %s", e.what());
				MessageBoxA(NULL, what, NULL, 0);
			}
		}
	}
}

void CHPSView::UpdateRenderingMode(HPS::Rendering::Mode mode)
{
	if (mode == HPS::Rendering::Mode::Phong)
		_smoothRendering = true;
	else if (mode == HPS::Rendering::Mode::FastHiddenLine)
		_smoothRendering = false;
}

void CHPSView::UpdatePlanes()
{
	if(_enableSimpleShadows)
	{
		GetCanvas().GetFrontView().SetSimpleShadow(true);
	
		const float					opacity = 0.3f;
		const unsigned int			resolution = 512;
		const unsigned int			blurring = 20;

		HPS::SegmentKey viewSegment = GetCanvas().GetFrontView().GetSegmentKey();

		// Set opacity in simple shadow color
		HPS::RGBAColor color(0.4f, 0.4f, 0.4f, opacity);
		if (viewSegment.GetVisualEffectsControl().ShowSimpleShadowColor(color))
			color.alpha = opacity;

		viewSegment.GetVisualEffectsControl()
			.SetSimpleShadow(_enableSimpleShadows, resolution, blurring)
			.SetSimpleShadowColor(color);
		_canvas.Update();
	}
}

void CHPSView::OnUpdateRibbonBtnSimpleShadow(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_enableSimpleShadows);
}

void CHPSView::OnUpdateRibbonBtnFrameRate(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_enableFrameRate);
}

void CHPSView::OnUpdateRibbonBtnSmooth(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_smoothRendering);
}

void CHPSView::OnUpdateRibbonBtnHiddenLine(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(!_smoothRendering);
}

void CHPSView::OnUpdateRibbonBtnEyeDome(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_eyeDome);
}

void CHPSView::OnUpdateRibbonBtnOrbitOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::OrbitOperator]);
}

void CHPSView::OnUpdateRibbonBtnPanOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::PanOperator]);
}

void CHPSView::OnUpdateRibbonBtnZoomAreaOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::ZoomAreaOperator]);
}

void CHPSView::OnUpdateRibbonBtnFlyOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::FlyOperator]);
}

void CHPSView::OnUpdateRibbonBtnPointOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::PointOperator]);
}

void CHPSView::OnUpdateRibbonBtnAreaOp(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(_operatorStates[SandboxOperators::AreaOperator]);
}

void CHPSView::OnKillFocus(CWnd* /*pNewWnd*/)
{
	_canvas.GetWindowKey().GetEventDispatcher().InjectEvent(HPS::FocusLostEvent());
}

void CHPSView::OnSetFocus(CWnd* /*pOldWnd*/)
{
	_capsLockState = IsCapsLockOn();
}

//! [user_code]
void CHPSView::OnUserCode1()
{
	// Toggle display of resource monitor using the DebuggingControl
	_displayResourceMonitor = !_displayResourceMonitor;
	_canvas.GetWindowKey().GetDebuggingControl().SetResourceMonitor(_displayResourceMonitor);

	GetCanvas().Update();
}

void CHPSView::OnUserCode2()
{
	// TODO: Add your command handler code here
}

void CHPSView::OnUserCode3()
{
    // TODO: Add your command handler code here
}

void CHPSView::OnUserCode4()
{
    // TODO: Add your command handler code here
}

//! [user_code]

void CHPSView::OnButtonStart()
{
	// Set Perspective projection
	HPS::SegmentKey viewSK = GetCanvas().GetFrontView().GetSegmentKey();
	HPS::CameraKit camera;
	viewSK.ShowCamera(camera);

	camera.SetProjection(HPS::Camera::Projection::Perspective);
	viewSK.SetCamera(camera);

	GetCanvas().Update();

	// Set default
	m_bSegmentSelected = false;

	// Show dialog
	RenderingDlg* pDlg = new RenderingDlg(this);
	pDlg->ShowWindow(SW_SHOW);
}

void CHPSView::SetProgress(int pos, const int remTime)
{
	GetDocument()->SetProgress(pos, remTime);
}

void CHPSView::OnCheckPreserveColor()
{
	m_bPreserveColor = !m_bPreserveColor;
}

void CHPSView::OnUpdateCheckPreserveColor(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bPreserveColor);
}

void CHPSView::OnCheckOverrideMaterial()
{
	m_bOverrideMaterial = !m_bOverrideMaterial;
}

void CHPSView::OnUpdateCheckOverrideMaterial(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bOverrideMaterial);
}

void CHPSView::SetLightingModeId(const int id)
{
	m_iLightingModeId = id;
}

void CHPSView::OnCheckSyncCamera()
{
	m_bSyncCamera = !m_bSyncCamera;
}

void CHPSView::OnUpdateCheckSyncCamera(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bSyncCamera);
}


void CHPSView::OnButtonLoadEnvMap()
{
	CString filter = _T("HDRI Files (*.hdr)|*.hdr||");
	CFileDialog dlg(TRUE, _T(".*"), NULL, OFN_HIDEREADONLY, filter, NULL);
	if (dlg.DoModal() != IDOK)
		return;

	CString filePath = dlg.GetPathName();
	strcpy(m_cHdriFilePath, CStringA(filePath).GetBuffer());

	m_iLightingModeId = 2;
	
	GetDocument()->SetLighingModeSelItem(m_iLightingModeId);
}

void CHPSView::SetUpVector(const int id)
{
	m_rootUpVector = id;

	HPS::SegmentKey key = GetCanvas().GetFrontView().GetAttachedModel().GetSegmentKey();

	// reset modelling matrix
	HPS::MatrixKit modellingMatrix = HPS::MatrixKit();
	key.SetModellingMatrix(modellingMatrix);

	switch (m_rootUpVector) {
	case 0: // X up
		modellingMatrix.Rotate(90.0f, 0.0f, 0.0f);
		break;
	case 1: // Y up = default
		break;
	case 2: // Z up
		modellingMatrix.Rotate(0.0f, 90.0f, 0.0f);
		break;
	case 3: // -X up
		modellingMatrix.Rotate(-90.0f, 0.0f, 0.0f);
		break;
	case 4: // -Y up
		modellingMatrix.Rotate(180.0f, 0.0f, 0.0f);
		break;
	case 5: // -Z up
		modellingMatrix.Rotate(0.0f, -90.0f, 0.0f);
		break;
	}

	// set up rotation and rotation based on up vector
	modellingMatrix.Scale(m_rootScale, m_rootScale, m_rootScale);

	// set root matrix
	key.SetModellingMatrix(modellingMatrix);

	// fit camera to scale
	GetCanvas().Update();
}


