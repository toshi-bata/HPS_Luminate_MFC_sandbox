#pragma once
#include <map>

class CHPSDoc;

// Class to handle errors
class MyErrorHandler: public HPS::EventHandler
{
public:
	MyErrorHandler() : HPS::EventHandler() {}
	virtual ~MyErrorHandler() { Shutdown(); }

	// Override to provide behavior for an error event
	virtual HandleResult Handle(HPS::Event const * in_event)
	{
		ASSERT(in_event!=NULL);
		HPS::ErrorEvent const * error = static_cast<HPS::ErrorEvent const *>(in_event);
		HPS::UTF8 msg = HPS::UTF8("Error: ") + error->message + HPS::UTF8("\n");
		HPS::WCharArray wmsg;
		msg.ToWStr(wmsg);		
		OutputDebugString(&wmsg[0]);
		return HandleResult::Handled;
	}
};


// Class to handle warnings
class MyWarningHandler: public HPS::EventHandler
{
public:
	MyWarningHandler() : HPS::EventHandler() {}
	virtual ~MyWarningHandler() { Shutdown(); }

	// Override to provide behavior for a warning event
	virtual HandleResult Handle(HPS::Event const * in_event)
	{
		ASSERT(in_event!=NULL);
		HPS::WarningEvent const * warning = static_cast<HPS::WarningEvent const *>(in_event);
		HPS::UTF8 msg = HPS::UTF8("Warning: ") + warning->message + HPS::UTF8("\n");
		HPS::WCharArray wmsg;
		msg.ToWStr(wmsg);		
		OutputDebugString(&wmsg[0]);
		return HandleResult::Handled;
	}

};

enum class SandboxOperators
{
	OrbitOperator,
	PanOperator,
	ZoomAreaOperator,
	PointOperator,
	AreaOperator,
	FlyOperator
};

class CHPSView : public CView
{
public:
	virtual						~CHPSView();
	CHPSDoc *					GetDocument() const	{ return reinterpret_cast<CHPSDoc*>(m_pDocument); }

	// PreCreateWindow is used to setup our Window class, specifying settings
	// such as CS_HREDRAW to prevent flicker when resizing.
	virtual BOOL				PreCreateWindow(CREATESTRUCT& cs);

	// OnInitialUpdate is called the first time after the view is constructed.
	// HPS.Canvas initialization happens here.
	virtual void				OnInitialUpdate();

	// The OnDraw method is not needed since we're using OnPaint().
	// However, it's an abstract method so we have to implement it.
	virtual void				OnDraw(CDC* /*pDC*/) {}	

	// Returns the single HPS::Canvas attached to our HPS canvas
	HPS::Canvas					GetCanvas() { return _canvas; }

	// Sets the direction for a camera-relative, colorless, main distant light in the scene.
	void						SetMainDistantLight(HPS::Vector const & lightDirection = HPS::Vector(1, 0, -1.5f));

	// Sets a main distant light in the scene.
	void						SetMainDistantLight(HPS::DistantLightKit const & light);

	// Sets HPS scene defaults for new and opened documents.
	void						SetupSceneDefaults();

	// Sets the default operator stack on the current view
	void						SetupDefaultOperators();

	// Transition smoothly from the current capture to this new capture
	void						ActivateCapture(HPS::Capture & capture);
#ifdef USING_EXCHANGE
	// Transition smoothly from the current capture to this new capture by using a Component Path
	void						ActivateCapture(HPS::ComponentPath & capture_path);
#endif
	// Swap the current view with newView w/o any transition
	void						AttachView(HPS::View & newView);

	// Transition smoothly from the current view to the newView
	void						AttachViewWithSmoothTransition(HPS::View & newView);

	// Perform asynchronous update on canvas
	void						Update();

	// Unhighlight things highlighted with the standard highlight style
	void						Unhighlight();

	// Unhighlight things highlighted with the standard highlight style
	void						UnhighlightAndUpdate();

	// Smoothly transitions the camera to the bounding of the specified key path
	void						ZoomToKeyPath(HPS::KeyPath const & keyPath);

	// Smoothly transition to the camera preceding zooming to a segment
	void						RestoreCamera();

	// Invalidate the key path used for fit world (go back to using the model)
	void						InvalidateZoomKeyPath();

	// Invalidate the saved camera without transitioning back to it
	void						InvalidateSavedCamera();

protected:
	CHPSView();
	DECLARE_DYNCREATE(CHPSView)

private:
	// Constructs HPS.InputEvent.ModifierKeys from MFC flags
	HPS::ModifierKeys		MapModifierKeys(UINT flags);

	// Helper method used to construct a HPS::MouseEvent
	HPS::MouseEvent			BuildMouseEvent(HPS::MouseEvent::Action action, HPS::MouseButtons buttons, CPoint cpoint, UINT flags, size_t click_count, float scalar = 0);

	// Helper method used to keep track of which operator is currently active
	void					UpdateOperatorStates(SandboxOperators op);

	/// HPS canvas associated with this MFC View
	HPS::Canvas 				_canvas;

	/// Error handler associated with this MFC View
	MyErrorHandler				_errorHandler;

	/// Warning handler associated with this MFC View
	MyWarningHandler			_warningHandler;

	/// Main distant light in Sprockets view.
	HPS::DistantLightKey		_mainDistantLight;

	// KeyPath used for ZoomToSegment
	HPS::KeyPath				_zoomToKeyPath;

	// Camera in use prior to ZoomToSegment
	HPS::CameraKit				_preZoomToKeyPathCamera;

	// Whether Caps Lock is engaged or not
	bool						_capsLockState;
	inline bool					IsCapsLockOn()
	{
		if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
			return true;
		return false;
	}

	// Transform to convert pixels to window space
	CRect						_windowRect;
	HPS::MatrixKit				_pixelToWindowMatrix;
	HPS::MatrixKit const &		GetPixelToWindowMatrix();

	/// Ribbon button states
	bool								_enableSimpleShadows;
	bool								_enableFrameRate;
	bool								_displayResourceMonitor;
	bool								_smoothRendering;
	std::map<SandboxOperators, bool>	_operatorStates;

	// Used for rendering mode bookkeeping on initial update
	void						UpdateRenderingMode(HPS::Rendering::Mode mode);

	bool _eyeDome;

	void UpdateEyeDome(bool update);

	// Luminate bridge integration
	bool m_bSyncCamera = false;
	int m_iMaterialTypeID = 0;
	int m_iMaterialID = 0;
	bool m_bSegmentSelected = false;
	bool m_bOverrideMaterial = false;
	bool m_bPreserveColor = false;
	char m_cHdriFilePath[MAX_PATH];

	int m_iLightingModeId = 0;
	int m_rootUpVector = 1;
	double m_rootScale = 1.0;

public:
	void SetProgress(const int pos, const int remTime);
	bool GetSyncCamera() { return m_bSyncCamera; }
	void SetSegmentSelected(const bool selected) { m_bSegmentSelected = selected; }
	bool GetSegmentSelected() { return m_bSegmentSelected; }
	void SetMaterialTypeId(const int id) { m_iMaterialTypeID = id; }
	int GetMateralTypeId() { return m_iMaterialTypeID; }
	void SetMaterialId(const int id) { m_iMaterialID = id; }
	int GetMateralId() { return m_iMaterialID; }
	bool GetOverrideMaterial() { return m_bOverrideMaterial; }
	bool GetPreserveColor() { return m_bPreserveColor; }

	void SetLightingModeId(const int id);
	int GetLightingModeId() { return m_iLightingModeId; }
	char* GetHdriFilePath() { return m_cHdriFilePath; }
	void SetUpVector(const int id);

public:
	//{{AFX_MSG(CHPSView)
	afx_msg void OnEditCopy();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileOpen();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
	afx_msg void OnOperatorsOrbit();
	afx_msg void OnOperatorsPan();
	afx_msg void OnOperatorsZoomArea();
	afx_msg void OnOperatorsFly();
	afx_msg void OnOperatorsHome();
	afx_msg void OnOperatorsZoomFit();
	afx_msg void OnOperatorsSelectPoint();
	afx_msg void OnOperatorsSelectArea();
	afx_msg void OnModesSimpleShadow();
	afx_msg void OnModesSmooth();
	afx_msg void OnModesHiddenLine();
	afx_msg void OnModesEyeDome();
	afx_msg void OnModesFrameRate();
	afx_msg void UpdatePlanes();
	afx_msg void OnUserCode1();
	afx_msg void OnUserCode2();
	afx_msg void OnUserCode3();
	afx_msg void OnUserCode4();
	afx_msg void OnUpdateRibbonBtnSimpleShadow(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnFrameRate(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnSmooth(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnHiddenLine(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnEyeDome(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnOrbitOp(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnPanOp(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnZoomAreaOp(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnFlyOp(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnPointOp(CCmdUI * pCmdUI);
	afx_msg void OnUpdateRibbonBtnAreaOp(CCmdUI * pCmdUI);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	afx_msg void OnButtonStart();
	afx_msg void OnCheckPreserveColor();
	afx_msg void OnUpdateCheckPreserveColor(CCmdUI* pCmdUI);
	afx_msg void OnCheckOverrideMaterial();
	afx_msg void OnUpdateCheckOverrideMaterial(CCmdUI* pCmdUI);
	afx_msg void OnUpdateComboLightingMode(CCmdUI* pCmdUI);
	afx_msg void OnCheckSyncCamera();
	afx_msg void OnUpdateCheckSyncCamera(CCmdUI* pCmdUI);
	afx_msg void OnButtonLoadEnvMap();
};


