#pragma once

class CHPSView;
class CHPSFrame;

class CHPSDoc : public CDocument
{
public:
	virtual	~CHPSDoc();
	virtual BOOL		OnNewDocument();
	virtual BOOL		OnOpenDocument(LPCTSTR lpszPathName);

	HPS::Model			GetModel() const { return _model; }
	HPS::CADModel		GetCADModel() const { return _cadModel; }
	HPS::CameraKit		GetDefaultCamera() const { return _defaultCamera; }

	// Helper method to get the attached CView
	CHPSView *			GetCHPSView();

	// Helper method to get CHPSFrame
	CHPSFrame *			GetCHPSFrame();

	void SetProgress(const int pos, const int remTime);

#ifdef USING_EXCHANGE
	void				ImportConfiguration(HPS::UTF8Array const & configuration);
#endif

private:

	// Shared method used to create a new HPS::Model
	void				CreateNewModel();
	bool				ImportHSFFile(LPCTSTR lpszPathName);
	bool				ImportSTLFile(LPCTSTR lpszPathName);
	bool				ImportOBJFile(LPCTSTR lpszPathName);
	bool				ImportPointCloudFile(LPCTSTR lpszPathName);

#ifdef USING_EXCHANGE
	bool				ImportExchangeFile(LPCTSTR lpszPathName, HPS::Exchange::ImportOptionsKit const & options = HPS::Exchange::ImportOptionsKit());
#endif

#ifdef USING_PARASOLID
	bool				ImportParasolidFile(LPCTSTR lpszPathName, HPS::Parasolid::ImportOptionsKit const & options = HPS::Parasolid::ImportOptionsKit());
#endif

#ifdef USING_DWG
	bool				ImportDWGFile(LPCTSTR lpszPathName, HPS::DWG::ImportOptionsKit const & options = HPS::DWG::ImportOptionsKit::GetDefault());
#endif

private:
	HPS::Model			_model;
	HPS::CADModel		_cadModel;
	HPS::CameraKit		_defaultCamera;

protected:
	CHPSDoc();
	DECLARE_DYNCREATE(CHPSDoc);
	DECLARE_MESSAGE_MAP()
};

extern CHPSDoc * GetCHPSDoc();
