#ifndef _BEM_Rosetta_GUI_BEM_Rosetta_GUI_h
#define _BEM_Rosetta_GUI_BEM_Rosetta_GUI_h

class RichTextView2 : public RichTextView {
public:
	RichTextView2() {
		zoomlevel = 5;
		Background(Color(245, 245, 245));
	}
	virtual void Layout() {
		RichTextView::Layout();
		PageWidth(int(zoomlevel*GetSize().cx));
	}
	double zoomlevel;
};

template <class T>
class WithRectEnter : public T {
public:
	void SetRectangle(StaticRectangle &_rect, Function <void(StaticRectangle *)>_WhenEnter) {
		rect = &_rect;
		WhenEnter = _WhenEnter;
		rect->Hide();
	}
	virtual void MouseEnter(Point , dword) {
		if (!rect)
			return;
		rect->Show();
		if (WhenEnter)
			WhenEnter(rect);
	}

private:
	StaticRectangle *rect = 0;
	Function <void(StaticRectangle *)>WhenEnter;
};

template <class T>
class WithFocus : public T {
public:
	virtual void GotFocus() {WhenFocus();}
	Function <void()>WhenFocus;
};

class FreqSelector : public StaticRect {
public:
	typedef FreqSelector CLASSNAME;
	
	FreqSelector();
	
	void Init(Function <void()>WhenAction, Function <void(StaticRectangle*)>WhenSetRectangle) {
		OnAction = WhenAction;
		OnSetRectangle = WhenSetRectangle;
	}
	
	void Clear() {
		edits.Clear();
		rects.Clear();
		Rect rc = add.GetRect();
		rc.right = rc.Width();
		rc.left = 0;
		add.SetRect(rc);
		pos = 0;
		Layout();
	}
	void HideCtrls() {
		for (int i = 0; i < edits.GetCount(); ++i)
			rects[i].Hide();	
	}
	int GetCount() 			{return edits.GetCount();}
	double Get(int id) 		{return ~edits[id];}
	bool IsSelected(int id)	{return rects[id].IsVisible();}
	
	int IsRect(StaticRectangle *rect) {
		for (int i = 0; i < rects.GetCount(); ++i) {
			if (&rects[i] == rect)
				return i;
		}
		return -1;
	}
	void Set(int id, double val) {
		edits[id] <<= val;
		edits[id].WhenAction();
	} 
	
	void AddField(double val = Null) {
		WithRectEnter<EditDouble> &edit = edits.Add();
		StaticRectangle &rect = rects.Add();
		edit.WhenAction = OnAction;
		edit <<= val;
		edit.SetRectangle(rect, [&](StaticRectangle *rect) {OnSetRectangle(rect);});
		rect.SetColor(LtBlue());
		rect.SetWidth(fThick);
		Add(rect.LeftPos(pos*fWidth,   fWidth).TopPos(0, 25));
		Add(edit.LeftPos(pos*fWidth + fThick, fWidth-2*fThick).TopPos(3, 19));
		Rect rc = add.GetRect();
		rc.right = (pos+1)*fWidth + rc.Width();
		rc.left = (pos+1)*fWidth;
		add.SetRect(rc);
		edit.SetFocus();
		pos++;
		Layout();
	}
	void CheckFocus(double freq) {
		for (int i = 0; i < edits.GetCount(); ++i) {
			if (abs(double(~edits[i]) - freq) < 0.000001) {
				OnSetRectangle(&rects[i]);
				break;
			}
		}
	}

private:
	Upp::Array<WithRectEnter<EditDouble>> edits;
	Upp::Array<StaticRectangle> rects;
	Button add;
	int pos = 0;
	int fWidth = 40, fThick = 3;
	
	Function <void()>OnAction;
	Function <void(StaticRectangle*)>OnSetRectangle;
};


String TabText(const TabCtrl &tab);

#define LOGTAB(tab) LOG(Format("Tab %s, Set %s, file: %s, line: %d", #tab, TabText(tab), __FILE__, __LINE__))


#include <ScatterDraw/Unpedantic.h>
#define LAYOUTFILE <BEMRosetta/BEMRosetta/main.lay>
#include <CtrlCore/lay.h>
#include <ScatterDraw/Pedantic.h>

#include "arrange.h"

enum DataToShow {DATA_A, DATA_B, DATA_FORCE_SC, DATA_FORCE_FK, DATA_FORCE_EX, DATA_RAO, DATA_STS, DATA_STS2};
enum DataToPlot {PLOT_A, PLOT_AINF, PLOT_A0, PLOT_B, PLOT_FORCE_SC_MA, PLOT_FORCE_SC_PH,
				 PLOT_FORCE_FK_MA, PLOT_FORCE_FK_PH, PLOT_FORCE_EX_MA, PLOT_FORCE_EX_PH, 
				 PLOT_RAO_MA, PLOT_RAO_PH, PLOT_Z_MA, PLOT_Z_PH, PLOT_TFS_MA, PLOT_TFS_PH};


String ForceExtSafe(String fileName, String ext);
   
class HydroSource : public DataSource {
public:
	HydroSource() : data(0) {}
	HydroSource(Hydro &_data, int i, int _j_h, DataToPlot _dataToPlot, bool _show_w, bool _ndim) {
		Init(_data, i, _j_h, _dataToPlot, _show_w, _ndim);
	}
	bool Init(Hydro *_data, int i, int _j_dof, DataToPlot _dataToPlot, bool _show_w, bool _ndim) 	{
		return Init(*_data, i, _j_dof, _dataToPlot, _show_w, _ndim);
	}
	bool Init(const Hydro &_data, int _idf, int _j_dof, DataToPlot _dataToPlot, bool _show_w, bool _ndim) {
		data = &_data;
		dataToPlot = _dataToPlot;
		if (_idf >= _data.dofOrder.GetCount())
			return false;
		idf = _data.dofOrder[_idf];
		if (dataToPlot == PLOT_A || dataToPlot == PLOT_AINF || dataToPlot == PLOT_A0 || dataToPlot == PLOT_B || dataToPlot == PLOT_Z_MA || dataToPlot == PLOT_Z_PH) {
			if (_j_dof >= _data.dofOrder.GetCount())
				return false;
			_j_dof = _data.dofOrder[_j_dof];
		}
		jdf = _j_dof;
		show_w = _show_w;
		ndim = _ndim;
		if (IsNullData())
			return false;
		return true;
	}
	inline bool IsNullData() {
		ASSERT(data != 0);
		switch (dataToPlot) {
		case PLOT_A:			return IsNull(data->A[0](idf, jdf));
		case PLOT_AINF:			return IsNull(data->Awinf(idf, jdf));
		case PLOT_A0:			return IsNull(data->Aw0(idf, jdf));
		case PLOT_B:			return IsNull(data->B[0](idf, jdf));
		case PLOT_FORCE_SC_MA:	return IsNull(data->sc.ma[jdf](0, idf));
		case PLOT_FORCE_SC_PH:	return IsNull(data->sc.ph[jdf](0, idf));
		case PLOT_FORCE_FK_MA:	return IsNull(data->fk.ma[jdf](0, idf));
		case PLOT_FORCE_FK_PH:	return IsNull(data->fk.ph[jdf](0, idf));
		case PLOT_FORCE_EX_MA:	return IsNull(data->ex.ma[jdf](0, idf));
		case PLOT_FORCE_EX_PH:	return IsNull(data->ex.ph[jdf](0, idf));
		case PLOT_RAO_MA:		return IsNull(data->rao.ma[jdf](0, idf));
		case PLOT_RAO_PH:		return IsNull(data->rao.ph[jdf](0, idf));
		case PLOT_TFS_MA:		return data->sts[idf][jdf].TFS.IsEmpty();
		case PLOT_TFS_PH:		return data->sts[idf][jdf].TFS.IsEmpty();
		case PLOT_Z_MA:			return IsNull(data->A[0](idf, idf)) || IsNull(data->B[0](idf, jdf)) || data->Awinf.size() == 0 || IsNull(data->Awinf(idf, jdf));
		case PLOT_Z_PH:			return IsNull(data->A[0](idf, idf)) || IsNull(data->B[0](idf, jdf)) || data->Awinf.size() == 0 || IsNull(data->Awinf(idf, jdf));
		default:				NEVER();	return true;
		}
	}
	virtual inline double y(int64 id) {
		ASSERT(data != 0);
		switch (dataToPlot) {
		case PLOT_A:			return data->A_(ndim, int(id), idf, jdf);
		case PLOT_AINF:			return data->Awinf_(ndim, idf, jdf);
		case PLOT_A0:			return data->Aw0_(ndim, idf, jdf);
		case PLOT_B:			return data->B_(ndim, int(id), idf, jdf);
		case PLOT_FORCE_SC_MA:	return data->F_ma_(ndim, data->sc, jdf, int(id), idf);
		case PLOT_FORCE_SC_PH:	return data->sc.ph[jdf](int(id), idf);
		case PLOT_FORCE_FK_MA:	return data->F_ma_(ndim, data->fk, jdf, int(id), idf);
		case PLOT_FORCE_FK_PH:	return data->fk.ph[jdf](int(id), idf);
		case PLOT_FORCE_EX_MA:	return data->F_ma_(ndim, data->ex, jdf, int(id), idf);
		case PLOT_FORCE_EX_PH:	return data->ex.ph[jdf](int(id), idf);
		case PLOT_RAO_MA:		return data->F_ma_(ndim, data->rao, jdf, int(id), idf);
		case PLOT_RAO_PH:		return data->rao.ph[jdf](int(id), idf);
		case PLOT_TFS_MA:		return std::abs(data->TFS_(ndim, int(id), idf, jdf));
		case PLOT_TFS_PH:		return std::arg(data->TFS_(ndim, int(id), idf, jdf));
		case PLOT_Z_MA:			return std::abs(data->Z(ndim, int(id), idf, jdf));
		case PLOT_Z_PH:			return std::arg(data->Z(ndim, int(id), idf, jdf));
		default:				NEVER();	return Null;
		}
	}
	virtual inline double x(int64 id) {
		ASSERT(data != 0);
		
		if ((dataToPlot == PLOT_AINF || dataToPlot == PLOT_A0) && id == 1)
			id = data->Nf - 1;
		
		if (show_w) {
			if (dataToPlot == PLOT_A0)
				return 0;
			return data->w[int(id)];
		} else {
			if (dataToPlot == PLOT_AINF)
				return 0;
			return data->T[int(id)];
		}
	}
	virtual int64 GetCount() const {
		ASSERT(data != 0); 
		
		if (dataToPlot == PLOT_AINF)
			return show_w  ? 2 : 1;		
		if (dataToPlot == PLOT_A0)
			return !show_w ? 2 : 1;
		return data->Nf;
	}
	
private:
	const Hydro *data;
	int idf, jdf;
	DataToPlot dataToPlot;
	bool show_w, ndim;
};


class MenuOptions : public WithMenuOptions<StaticRect> {
public:
	typedef MenuOptions CLASSNAME;
	MenuOptions() : bem(0) {}
	void Init(BEMData &bem);
	void Load();
	void OnSave();
	bool IsChanged();
	
private:
	BEMData *bem;
};

class MenuAbout : public WithMenuAbout<StaticRect> {
public:
	typedef MenuAbout CLASSNAME;
	void Init();
	void HelpHandler(String &str);
};

class MainSummary : public WithMainSummary<StaticRect> {
public:
	typedef MainSummary CLASSNAME;
	void Init();
	void Clear();
};

class MainSummaryCoeff : public MainSummary {
public:
	typedef MainSummaryCoeff CLASSNAME;

	void Report(const Hydro &data, int id);
};

class MainOutput : public WithMainOutput<StaticRect> {
public:
	typedef MainOutput CLASSNAME;
	void Init();
	void Print(String str);
};

class MainArrange : public WithMainArrange<StaticRect> {
public:
	typedef MainArrange CLASSNAME;
	void Init();
	void Clear();
	void Load(Upp::Array<HydroClass> &hydro, const Vector<int> &ids);
	void Remove(int c);
	
private:
	Upp::Array<ArrangeDOF> arrangeDOF;
};
	
class MainView : public WithMainView<StaticRect> {
public:
	typedef MainView CLASSNAME;
	
	MainView() {}
	void Init(const WithMenuPlotMesh<StaticRect> &menuPlot, const ArrayCtrl &_array);
	void CalcEnvelope();
	void OnPaint();
	const WithMenuPlotMesh<StaticRect> &GetMenuPlot() const {return *menuPlot;}
	void SetPaintSelect(bool _paintSelect)					{paintSelect = _paintSelect;}
	
	VolumeEnvelope env;
	
private:
	const WithMenuPlotMesh<StaticRect> *menuPlot = 0;
	const ArrayCtrl *arrayModel = 0;
	bool paintSelect = true;
};


class MainViewDataEach : public StaticRect {
public:
	typedef MainViewDataEach CLASSNAME;
	
	MainViewDataEach() {}
	void Init(MeshData &_mesh, MainView &mainView);
	void OnRefresh();
	
	TabCtrl tab;
	Splitter moved, movedUnder;
	WithMainPlotList<StaticRect> arrayFacetsAll2, arrayNodesMoved,
								 arrayFacetsUnder, arrayNodesUnder;
	StatusBar status;
	
	class DataSourceFacets : public Convert {
	public:
		DataSourceFacets() : pmesh(0), col(0), all(true) {}
		void Init(MeshData &_mesh, int _col, bool _all);
		Value Format(const Value& q) const;
		inline const MeshData &GetMesh()	{return *pmesh;}
		
	private:
		MeshData *pmesh;
		int col;
		bool all;
	};
	class DataSourceNodes : public Convert {
	public:
		DataSourceNodes() : pmesh(0), xyz(0), origMovedUnder(0) {}
		void Init(MeshData &_mesh, int _xyz, int _origMovedUnder);
		Value Format(const Value& q) const;
		
	private:
		MeshData *pmesh;
		int xyz;
		int origMovedUnder;
	};
	
	Upp::Array<DataSourceFacets> dataSourceFacetsAll, dataSourceFacetsUnder;
	Upp::Array<DataSourceNodes> dataSourceNodesMoved;

private:
	void OnTimer();
	TimeCallback timeCallback;
	void UpdateStatus(bool under);
	Vector<int> selectedPanels, selectedNodes;  
	int lastSel = -1;
};

class MainViewData : public StaticRect {
public:
	typedef MainViewData CLASSNAME;
	
	void Init();
	void OnAddedModel(MainView &mainView);
	void OnRefresh();
	void Clear();
	
private:
	TabCtrl tab;
	Upp::Array<MainViewDataEach> models;
};

class MainSummaryMesh : public MainSummary {
public:
	typedef MainSummaryCoeff CLASSNAME;

	void Report(const Upp::Array<MeshData> &surfs, int id);
};

class MainStiffness : public WithMainStiffness<StaticRect> {
public:
	typedef MainStiffness CLASSNAME;
	
	void Init();
	void Clear();
	bool Load(Upp::Array<HydroClass> &hydros, const Vector<int> &ids);
	void Load(Upp::Array<MeshData> &surfs, const Vector<int> &ids);
	
private:
	void AddPrepare(int &row0, int &icol0, String name, int icase, String bodyName, int ibody, int idc);
	void Add(String name, int icase, const Eigen::MatrixXd &K, bool button, int idc);
	void Add(String name, int icase, String bodyName, int ibody, const Hydro &hydro, int idc);
};

class MainBEM;

class MainPlot : public StaticRect {
public:
	typedef MainPlot CLASSNAME;
	
	void Init(bool vert);
	void Init(int idf, double jdf_ih, DataToShow dataToShow);
	bool Load(const Upp::Array<HydroClass> &hydro, const MainBEM &mbm, const Vector<int> &ids);
	bool Load(const Hydro &hy, const MainBEM &mbm);
	void LoadEach(const Hydro &hy, int id, bool &loaded, int idc = -1);
	void Clear();
	void RefreshScatter()	{scatt.Refresh();	scatP.Refresh();}
	
	Upp::Array<HydroSource> ABFZ_source, ABFZ_source2;
	Upp::Array<HydroSource> Ainf_source, A0_source;
	
	Upp::Array<HydroSource> TFS_source, TFS_source2;
		
	int plot_idf, plot_jdf;
	double heading;
	DataToShow dataToShow;
	
	bool dim;
	int markW;
	bool show_w;
	
	ScatterCtrl scatt, scatP;
	Splitter splitter;

private:
	bool isInit = false;	
};

class MainABForce : public StaticRect {
public:
	typedef MainABForce CLASSNAME;
	
	void Init(DataToShow dataToShow);
	void Clear();
	bool Load(BEMData &bem, const Vector<int> &ids);
	
	TabCtrl tab;
	Upp::Array<Upp::Array<MainPlot>> plots;

private:
	int selTab;
	bool isFilling;
	DataToShow dataToShow;
};

class MainStateSpacePlot : public StaticRect {
public:
	typedef MainStateSpacePlot CLASSNAME;
	
	void Init(int _idf, int _jdf);
	bool Load(Upp::Array<HydroClass> &hydro, const Vector<int> &ids, const MainBEM &mbm);
	void InitArray(ArrayCtrl &array);
	
	MainPlot mainPlot;
	Splitter splitterTab;
	TabCtrl tab;
	Upp::Array<ArrayCtrl> arrays;
};

class MainStateSpace : public WithMainStateSpace<StaticRect> {
public:
	typedef MainStateSpace CLASSNAME;
	
	void Init();
	void Clear();
	void Init(ArrayCtrl &array);
	bool Load(BEMData &bem, const Vector<int> &ids);
	
	Upp::Array<Upp::Array<MainStateSpacePlot>> plots;
	
private:
	int selTab;
	bool isFilling;
};

typedef class MainABForce MainRAO;

class MainMesh : public WithMain<StaticRect> {
public:
	typedef MainMesh CLASSNAME;
	
	void Init();
	void InitSerialize(bool ret);
	
	void AfterLoad(String file);
	bool OnLoad();
	void OnRemove();
	void OnRemoveSelected(bool all);
	bool OnConvertMesh();
	void OnUpdate(bool forceMoved);
	void OnHealing();
	void OnImage(int axis);
	void OnOpt();
	void OnMenuProcessArraySel();
	void OnMenuConvertArraySel();
	
	void LoadSelTab(BEMData &bem);
		
	void Jsonize(JsonIO &json);
		
	WithMenuMesh<StaticRect> menuOpen;
	WithMenuConvertMesh<StaticRect> menuConvert;
	WithMenuPlotMesh<StaticRect> menuPlot;
	WithMenuMeshStability<StaticRect> menuProcess;
	
	bool GetShowMesh()			{return menuPlot.showMesh;}
	bool GetShowUnderwater()	{return menuPlot.showUnderwater;}

private:	
	MainView mainView;
	MainViewData mainViewData;
	SplitterButton mainVAll;
	MainSummaryMesh mainSummary;
	MainStiffness mainStiffness;

	Upp::Array<Option> optionsPlot;
	
	virtual void DragAndDrop(Point p, PasteClip& d);
	virtual bool Key(dword key, int count);
};

class MainMeshW : public TopWindow {
public:
	typedef MainMeshW CLASSNAME;
	
	void Init(MainMesh &_mesh) {
		LoadFromJson(mesh, StoreAsJson(_mesh));
		mesh.Init();
		Add(mesh.SizePos());
		Title(t_("BEMRosetta Mesh Viewer")).Sizeable().Zoomable();
	}
	
	virtual void Close() {
        delete this;
    }
	
	MainMesh mesh;
};

class MainNemoh : public WithNemoh<StaticRect> {
public:
	typedef MainNemoh CLASSNAME;

	void Init(const BEMData &bem);
	void InitSerialize(bool ret);
	
	void Load(const BEMData &bem);
	void Load(const NemohCal &data);
	void Save(NemohCal &data);
	
	void Jsonize(JsonIO &json);
	
private:
	bool OnLoad();
	bool OnSave(const BEMData &bem);
	void OnCursor();
	void arrayOnCursor();
	void arrayUpdateCursor();
	void arrayClear();
	void arrayOnAdd();
	void arrayOnDuplicate();
	void arrayOnRemove();
	void InitArray();
	
	virtual void DragAndDrop(Point p, PasteClip& d);
	virtual bool Key(dword key, int count);
};


class MainSetupFOAMM : public WithMainStateSpaceSetup<StaticRect> {
public:
	typedef MainSetupFOAMM CLASSNAME;
	
	void Init();
	
	void Load()				{WhenSelArrayCases();}
	void WhenSelArrayModel(int id, BEMData &bem);
	void WhenSelArrayCases();
	void WhenArrayCases();
	void WhenArrayFreq();
	String Check(double fromFreq, double toFreq, String freqs);
	
	void WhenFocus(StaticRectangle *rect);
	void OnPainter(Painter &w, ScatterCtrl *scat);
	void OnMouse(Point p, dword, ScatterCtrl::MouseAction action, ScatterCtrl *scat);
	//void OnMove(Point p);
	
	bool Get(Vector<int> &ibs, Vector<int> &idfs, Vector<int> &jdfs,
		Vector<double> &froms, Vector<double> &tos, Vector<Vector<double>> &freqs); 
	void Clear();
	
	MainPlot plots;
	
	FreqSelector selector;

private:
	Upp::Array<Option> options;
	StaticRectangle *rectActual = 0;
	int id = -1;
};

class MenuFOAMM : public WithMenuStateSpace<StaticRect> {
public:
	typedef MenuFOAMM CLASSNAME;
	
	void Init(MainBEM &_mainBEM, BEMData &bem, MainSetupFOAMM &setup);
	bool OnFOAMM(MainSetupFOAMM &setup);
	
	void Clear();

private:
	bool isCancelled;
};

class MainBEM : public WithMain<StaticRect> {
public:
	typedef MainBEM CLASSNAME;
	
	void Init();
	void InitSerialize(bool ret);

	bool OnLoad();
	bool OnLoadFile(String file);
	bool OnConvert();
	void OnOpt();
	void OnRemove();
	void OnRemoveSelected(bool all);
	void OnJoin();
	void OnSymmetrize();
	void OnA0();
	void OnAinf();
	void OnDescription();
	void OnMenuConvertArraySel();
		
	void Jsonize(JsonIO &json);
		
	WithMenuOpen<StaticRect> menuOpen;
	WithMenuConvert<StaticRect> menuConvert;
	WithMenuPlot<StaticRect> menuPlot;
	MenuFOAMM menuFOAMM;
	
	MainSummaryCoeff mainSummary;
	MainArrange mainArrange;
	MainABForce mainA;
	MainABForce mainB;
	MainABForce mainForceSC, mainForceFK, mainForceEX;
	MainRAO mainRAO;
	MainStateSpace mainStateSpace;
	MainStiffness mainStiffness;
	MainSetupFOAMM mainSetupFOAMM;
		
private:
	ScatterCtrl &GetSelScatter();
	MainABForce &GetSelABForce();
	MainStateSpace &GetSelStateSpace();
	void LoadSelTab(BEMData &bem);
	
	virtual void DragAndDrop(Point p, PasteClip& d);
	virtual bool Key(dword key, int count);
	String BEMFile(String fileFolder) const;
};

class MainBEMW : public TopWindow {
public:
	typedef MainBEMW CLASSNAME;
	
	void Init(MainBEM &_bem) {
		LoadFromJson(bem, StoreAsJson(_bem));
		bem.Init();
		Add(bem.SizePos());
		Title(t_("BEMRosetta BEM Coefficients Processing")).Sizeable().Zoomable();
	}
	
	virtual void Close() {
        delete this;
    }
	
	MainBEM bem;
};


class Main : public TopWindow {
public:
	typedef Main CLASSNAME;
	
	Main() : closed(false) {}
	virtual ~Main() noexcept;
	virtual void Close();
	void CloseMain(bool store);

	void Init();

	void OptionsUpdated();

	bool LoadSerializeJson();
	bool StoreSerializeJson();
	
	void Jsonize(JsonIO &json);

	BEMData bem;
	
	void Status(String str = String(), int time = 2000)	{
		if (!str.IsEmpty()) 
			bar.Temporary(str, time);
		else
			bar.EndTemporary();
		ProcessEvents();
	}
	
private:
	TabCtrl tab;
	int lastTab;
	Button tabWindow;
	
	MainNemoh mainNemoh;
	MainBEM mainBEM;
	MainOutput mainOutput;
	MainMesh mainMesh;
	
	MenuOptions menuOptions;
	MenuAbout menuAbout;
	
	bool closed;
	
	StatusBar bar;
};

ArrayCtrl &ArrayModel_Init(ArrayCtrl &array, bool push = false);
void ArrayModel_Add(ArrayCtrl &array, String codeStr, String title, String fileName, int id);
void ArrayModel_Add(ArrayCtrl &array, String codeStr, String title, String fileName, int id, 
					Upp::Array<Option> &option, Function <void()>OnPush);
int ArrayModel_Id(const ArrayCtrl &array);
int ArrayModel_Id(const ArrayCtrl &array, int row);
int ArrayModel_IdMesh(const ArrayCtrl &array);
int ArrayModel_IdMesh(const ArrayCtrl &array, int row);
int ArrayModel_IdHydro(const ArrayCtrl &array);
int ArrayModel_IdHydro(const ArrayCtrl &array, int row);
Vector<int> ArrayModel_IdsHydro(const ArrayCtrl &array);		
Vector<int> ArrayModel_IdsMesh(const ArrayCtrl &array);
void ArrayModel_IdsHydroDel(ArrayCtrl &array, const Vector<int> &ids);
void ArrayModel_RowsHydroDel(ArrayCtrl &array, const Vector<int> &ids);
bool ArrayModel_IsVisible(const ArrayCtrl &array, int row);
const Color& ArrayModel_GetColor(const ArrayCtrl &array, int row);
String ArrayModel_GetFileName(ArrayCtrl &array, int row = -1);
		
Main &ma(Main *m = 0);
BEMData &Bem();
void Status(String str = String(), int time = 2000);

const Color &GetColorId(int id);
	
#endif
