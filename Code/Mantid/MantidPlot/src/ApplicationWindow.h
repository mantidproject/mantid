/***************************************************************************
    File                 : ApplicationWindow.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : QtiPlot's main window

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef APPLICATION_H
#define APPLICATION_H

#include <QMainWindow>
#include <q3listview.h>
#include <QFile>
#include <QSplitter>
#include <QDesktopServices>
#include <QBuffer>
#include <QLocale>
#include <QSet>
#include "Table.h"
#include "ScriptingEnv.h"
#include "Scripted.h"
#include "Script.h"

class QPixmap;
class QCloseEvent;
class QDropEvent;
class QTimerEvent;
class QDragEnterEvent;
class QTranslator;
class QDockWidget;
class QAction;
class QActionGroup;
class QLineEdit;
class QTranslator;
class QToolButton;
class QShortcut;
class QMenu;
class QToolBar;
//class QAssistantClient;
class QLocale;
class QMdiArea;
class QUndoView;

class Matrix;
class Table;
class Graph;
class ScalePicker;
class Graph3D;
class Note;
class MultiLayer;
class FunctionDialog;
class Folder;
class FolderListItem;
class FolderListView;
class Plot3DDialog;
class TableStatistics;
class CurveRangeDialog;
class LegendWidget;
class ArrowMarker;
class ImageMarker;
class TextEditor;
class AssociationsDialog;
class MantidMatrix;

//Mantid
class MantidUI;
class ScriptingWindow;
class ScriptManagerWidget;

/**
 * \brief QtiPlot's main window.
 *
 * This class contains the main part of the user interface as well as the central project management facilities.
 *
 * It manages all MdiSubWindow MDI Windows in a project, knows about their organization in Folder objects
 * and contains the parts of the project explorer not implemented in Folder, FolderListItem or FolderListView.
 *
 * Furthermore, it is responsible for displaying most MDI Windows' context menus and opening all sorts of dialogs.
 *
 * \section future Future Plans
 * Split out the project management part into a new Project class.
 * If MdiSubWindow maintains a reference to its parent Project, it should be possible to have its subclasses
 * display their own context menus and dialogs.
 * This is necessary for implementing new plot types or even completely new MdiSubWindow subclasses in plug-ins.
 * It will also make ApplicationWindow more manageable by removing those parts not directly related to the main window.
 *
 * Project would also take care of basic project file reading/writing (using Qt's XML framework), but delegate most of
 * the work to MdiSubWindow and its subclasses. This is necessary for providing save/restore of classes implemented in
 * plug-ins. Support for foreign formats on the other hand could go into import/export classes (which could also be
 * implemented in plug-ins). Those would interface directly with Project and the MyWidgets it manages. Thus, in addition
 * to supporting QtXML-based save/restore, Project, MdiSubWindow and subclasses will also have to provide generalized
 * save/restore methods/constructors.
 *
 * Maybe split out the project explorer into a new ProjectExplorer class, depending on how much code is left
 * in ApplicationWindow after the above reorganizations. Think about whether a Model/View approach can be
 * used for Project/ProjectExplorer.
 */
class ApplicationWindow: public QMainWindow, public Scripted
{
    Q_OBJECT
public:
    ApplicationWindow(bool factorySettings = false);
	ApplicationWindow(const QStringList& l);
	~ApplicationWindow();

	enum ShowWindowsPolicy{HideAll, ActiveFolder, SubFolders};
	enum WindowType{NoWindow, TableWindow, MatrixWindow, MultiLayerWindow, NoteWindow, Plot3DWindow};
	enum MatrixToTableConversion{Direct, XYZ, YXZ};
	enum EndLineChar{LF, CRLF, CR};
	enum Analysis{NoAnalysis, Integrate, Diff, FitLinear, FitGauss, FitLorentz, FitSigmoidal};

	FolderListView *lv, *folders;
	QDockWidget *logWindow;

	/*! Generates a new unique name starting with string /param name.
	You can force the output to be a name different from /param name,
	even if 'name' is not used in the project, by setting /param increment = true (the default)
	*/
	QString generateUniqueName(const QString& name, bool increment = true);
	void saveFitFunctions(const QStringList& lst);

	//! \name User custom actions
	//@{
	void loadCustomActions();
	void reloadCustomActions();
    void removeCustomAction(QAction *);
	void addCustomAction(QAction *, const QString& parentName, int index = -1);
    QList<QAction *> customActionsList(){return d_user_actions;};
	QList<QMenu *> customizableMenusList();

  
  //------ Mantid-------
  void addUserMenu(const QString &);  //Mantid
  void addUserMenuAction(const QString & parentMenu, const QString & itemName, const QString & itemData); //Mantid
  void removeUserMenu(const QString &);  //Mantid
  void removeUserMenuAction(const QString & menu, const QString & action); //Mantid
  const QList<QMenu*> & getCustomMenus() const; //Mantid
  ScriptingWindow* getScriptWindowHandle() { return scriptingWindow; }
  bool getMenuSettingsFlag(const QString & menu_item);
  //-------------------
  	//@}

	QList<QMenu *> menusList();
	QList<QToolBar *> toolBarsList();

	MdiSubWindow *activeWindow(WindowType type = NoWindow);

	int matrixUndoStackSize(){return d_matrix_undo_stack_size;};
	void setMatrixUndoStackSize(int size);

	QString endOfLine();
	bool autoUpdateTableValues(){return d_auto_update_table_values;};
	void setAutoUpdateTableValues(bool on = true);
	void enablesaveNexus(const QString& wsName);
		
public slots:
	//! \name Projects and Project Files
	//@{
	void open();
	ApplicationWindow* open(const QString& fn, bool factorySettings = false, bool newProject = true);
	ApplicationWindow* openProject(const QString& fn, bool factorySettings = false, bool newProject = true);
	ApplicationWindow* importOPJ(const QString& fn, bool factorySettings = false, bool newProject = true);
	///Load nexus file from File->Load Menu
	void loadNexus();
	///Load Raw File from File->Load Menu
	void loadRaw();
  /// Load mantid data files using generic load algorithm
  void loadDataFile();
	/**
	 * \brief Create a new project from a data file.
	 *
	 * \param fn is read as a data file with the default column separator (as set by the user)
	 * and inserted as a table into a new, empty project.
	 * This table is then plotted with the Graph::LineSymbols style.
	 */
	ApplicationWindow * plotFile(const QString& fn);

	/**
	 * \brief Create a new project from a script file.
	 *
	 * \param fn is read as a Python script file and loaded in the command script window.
	 * \param execute specifies if the script should be executed after opening.
	 * \param quit If true then the application will quit after execution of the script
	 */
         ApplicationWindow * loadScript(const QString& fn, bool execute = false, bool quit = false);

	QList<MdiSubWindow *> windowsList();
	void updateWindowLists(MdiSubWindow *w);
	/*!
    Arranges all the visible project windows in a cascade pattern.
    */
	void cascade();

	void saveProjectAs(const QString& fileName = QString(), bool compress = false);
	bool saveProject(bool compress = false);

	//! Set the project status to modifed
	void modifiedProject();
	//! Set the project status to saved (not modified)
	void savedProject();
	//! Set the project status to modified and save 'w' as the last modified widget
	void modifiedProject(MdiSubWindow *w);
	//@}

	//! \name Settings
	//@{
	void readSettings();
	void saveSettings();
	void setSaveSettings(bool autoSaving, int min);
	void changeAppStyle(const QString& s);
	void changeAppFont(const QFont& f);
	void updateAppFonts();
	void setAppColors(const QColor& wc,const QColor& pc,const QColor& tpc, bool force = false);

	QLocale locale(){return d_locale;};
	void setLocale(const QLocale& l){d_locale = l;};

	void initWindow();
	//@}

	//! \name Multilayer Plots
	//@{
	MultiLayer* multilayerPlot(int c, int r, int style);
	MultiLayer* multilayerPlot(Table* w, const QStringList& colList, int style, int startRow = 0, int endRow = -1);
	//! used when restoring a plot from a project file
	MultiLayer* multilayerPlot(const QString& caption, int layers = 1, int rows = 1, int cols = 1);
	//! used by the plot wizard
	MultiLayer* multilayerPlot(const QStringList& colList);
	void connectMultilayerPlot(MultiLayer *g);
	void addLayer();
	void deleteLayer();

	//! Creates a new spectrogram graph
  	MultiLayer* plotSpectrogram(Matrix *m, Graph::CurveType type);
  	MultiLayer* plotGrayScale(Matrix *m = 0);
  	MultiLayer* plotContour(Matrix *m = 0);
  	MultiLayer* plotColorMap(Matrix *m = 0);
  	MultiLayer* plotImage(Matrix *m = 0);
  	MultiLayer* plotNoContourColorMap(Matrix *m = 0);

	//! Rearrange the layersin order to fit to the size of the plot window
  	void autoArrangeLayers();
	void initMultilayerPlot(MultiLayer* g, const QString& name);
	void polishGraph(Graph *g, int style);
	void plot2VerticalLayers();
	void plot2HorizontalLayers();
	void plot4Layers();
	void plotStackedLayers();
	void plotStackedHistograms();
	//@}

	//! \name 3D Data Plots
	//@{
    Graph3D* newPlot3D();
	Graph3D* openMatrixPlot3D(const QString& caption, const QString& matrix_name,
							 double xl,double xr,double yl,double yr,double zl,double zr);
	Graph3D* plotXYZ(Table* table,const QString& zColName, int type);
		//when reading from .qti file
	Graph3D* dataPlot3D(const QString& caption,const QString& formula,
						double xl, double xr, double yl, double yr, double zl, double zr);
	Graph3D* openPlotXYZ(const QString& caption,const QString& formula,
 						double xl, double xr, double yl, double yr, double zl, double zr);
	//@}

	//! \name Surface Plots
	//@{
    Graph3D* plotSurface(const QString& formula, double xl, double xr,
					   double yl, double yr, double zl, double zr, int columns = 40, int rows = 30);
	Graph3D* plotParametricSurface(const QString& xFormula, const QString& yFormula,
						const QString& zFormula, double ul, double ur, double vl, double vr,
						int columns, int rows, bool uPeriodic, bool vPeriodic);

	void connectSurfacePlot(Graph3D *plot);
	void newSurfacePlot();
	void editSurfacePlot();
	void remove3DMatrixPlots(Matrix *m);
	void updateMatrixPlots(MdiSubWindow *);
	void add3DData();
	void change3DData();
	void change3DData(const QString& colName);
	void change3DMatrix();
	void change3DMatrix(const QString& matrix_name);
	void insertNew3DData(const QString& colName);
	void add3DMatrixPlot();
	void insert3DMatrixPlot(const QString& matrix_name);
	void initPlot3D(Graph3D *plot);
	void customPlot3D(Graph3D *plot);
	void setPlot3DOptions();

	void plot3DWireframe();
	void plot3DHiddenLine();
	void plot3DPolygons();
	void plot3DWireSurface();

	Graph3D* plot3DMatrix(Matrix *m = 0, int style = 5);

	void plot3DRibbon();
	void plot3DScatter();
	void plot3DTrajectory();
	void plot3DBars();
	//@}

	//! \name User-defined Functions
	//@{
	MultiLayer * newFunctionPlot(QStringList &formulas, double start, double end, int points = 100, const QString& var = "x", int type = 0);

	FunctionDialog* functionDialog();
	FunctionDialog* showFunctionDialog();
	FunctionDialog* showFunctionDialog(Graph * g, int curve);
	void addFunctionCurve();
	void clearSurfaceFunctionsList();
	void clearLogInfo();
	void clearParamFunctionsList();
	void clearPolarFunctionsList();
	void updateFunctionLists(int type, QStringList &formulas);
	void updateSurfaceFuncList(const QString& s);
	//@}

	//! \name Matrices
	//@{
	//! Creates a new empty matrix
	Matrix* newMatrix(int rows = 32, int columns = 32);
	//! To be used when opening a project file only!
	Matrix* newMatrix(const QString& caption, int r, int c);
	Matrix* matrix(const QString& name);
	Matrix* convertTableToMatrix();
	Matrix* tableToMatrix(Table* t);
	void initMatrix(Matrix* m, const QString& caption);
	void transposeMatrix();
	void invertMatrix();
	void matrixDeterminant();
	void flipMatrixVertically();
	void flipMatrixHorizontally();
	void rotateMatrix90();
	void rotateMatrixMinus90();
	void viewMatrixImage();
	void viewMatrixTable();
	void exportMatrix();
	void setMatrixGrayScale();
	void setMatrixRainbowScale();
	void viewMatrixColumnRow();
	void viewMatrixXY();
    void matrixDirectFFT();
    void matrixInverseFFT();
	//@}

	//! \name Tables
	//@{
	//! Creates an empty table
	Table* newTable();
	//! Used when loading a table from a project file
	Table* newTable(const QString& caption,int r, int c);
	Table* newTable(int r, int c, const QString& name = QString(),const QString& legend = QString());
	Table* newTable(const QString& caption, int r, int c, const QString& text);
	/**
	 * \brief Create a Table which is initially hidden; used to return the result of an analysis operation.
	 *
	 * \param name window name (compare MdiSubWindow::MdiSubWindow)
	 * \param label window label (compare MdiSubWindow::MdiSubWindow)
	 * \param r number of rows
	 * \param c number of columns
     * \param text tab/newline - seperated initial content; may be empty
	 */
	Table* newHiddenTable(const QString& name, const QString& label, int r, int c, const QString& text=QString());
	Table* table(const QString& name);
	Table* convertMatrixToTableDirect();
	Table* convertMatrixToTableXYZ();
	Table* convertMatrixToTableYXZ();
	Table* matrixToTable(Matrix* m, MatrixToTableConversion conversionType = Direct);
	QList<MdiSubWindow *> tableList();
    //! Returns true if the project contains tables
	bool hasTable();
	//! Returns a list containing the names of all tables in the project
	QStringList tableNames();

	void connectTable(Table* w);
	void initTable(Table* w, const QString& caption);
	void customTable(Table* w);
	void customizeTables(const QColor& bgColor,const QColor& textColor,
						const QColor& headerColor,const QFont& textFont,
						const QFont& headerFont, bool showComments);

	void importASCII();
	void importASCII(const QStringList& files, int import_mode, const QString& local_column_separator, int local_ignored_lines, bool local_rename_columns,
        bool local_strip_spaces, bool local_simplify_spaces, bool local_import_comments, bool update_dec_separators,
        QLocale local_separators, const QString& local_comment_string, bool import_read_only, int endLineChar,const QString& sepforloadAscii);
	void exportAllTables(const QString& sep, bool colNames, bool colComments, bool expSelection);
	void exportASCII(const QString& tableName, const QString& sep, bool colNames, bool colComments, bool expSelection);

	//! recalculate selected cells of current table
	void recalculateTable();

	TableStatistics *newTableStatistics(Table *base, int type, QList<int>,
	    const QString &caption=QString::null);
	//@}

	//! \name Graphs
	//@{
	void setPreferences(Graph* g);
	void setGraphDefaultSettings(bool autoscale,bool scaleFonts,bool resizeLayers,bool antialiasing);
	void setLegendDefaultSettings(int frame, const QFont& font,
							 const QColor& textCol, const QColor& backgroundCol);
	void setArrowDefaultSettings(double lineWidth,  const QColor& c, Qt::PenStyle style,
								int headLength, int headAngle, bool fillHead);

	void plotL();
	void plotP();
	void plotLP();
	void plotPie();
	void plotVerticalBars();
	void plotHorizontalBars();
	void plotArea();
	void plotVertSteps();
	void plotHorSteps();
	void plotSpline();
	void plotVerticalDropLines();
	MultiLayer* plotHistogram();
	MultiLayer* plotHistogram(Matrix *m);
	void plotVectXYXY();
	void plotVectXYAM();
	void plotBoxDiagram();

    //! Check whether a table is valid for a 3D plot and display an appropriate error if not
    bool validFor3DPlot(Table *table);
    //! Check whether a table is valid for a 2D plot and display an appropriate error if not
    bool validFor2DPlot(Table *table);
    //! Generate a new 2D graph
    MultiLayer* generate2DGraph(Graph::CurveType type);
	//@}

	//! \name Image Analysis
	//@{
	void intensityTable();
	void pixelLineProfile();
	void loadImage();
	void loadImage(const QString& fn);
  	Matrix* importImage(const QString& = QString());
	//@}

	//! \name Export and Print
	//@{
	void exportLayer();
	void exportGraph();
	void exportAllGraphs();
	void exportPDF();
	void print();
	void printAllPlots();
	//@}

	QStringList columnsList(Table::PlotDesignation plotType = Table::All);

	void undo();
	void redo();

	//! \name MDI Windows
	//@{
	MdiSubWindow* clone(MdiSubWindow* w = 0);
	void rename();
	void renameWindow();

	//!  Called when the user presses F2 and an item is selected in lv.
	void renameWindow(Q3ListViewItem *item, int, const QString &s);

	//!  Checks weather the new window name is valid and modifies the name.
	bool setWindowName(MdiSubWindow *w, const QString &text);

	void maximizeWindow(Q3ListViewItem * lbi = 0);
	void maximizeWindow(MdiSubWindow *w);
	void minimizeWindow(MdiSubWindow *w = 0);
    //! Changes the geometry of the active MDI window
    void setWindowGeometry(int x, int y, int w, int h);

	void updateWindowStatus(MdiSubWindow* );

	bool hidden(QWidget* window);
	void closeActiveWindow();
	void closeWindow(MdiSubWindow* window);

	//!  Does all the cleaning work before actually deleting a window!
	void removeWindowFromLists(MdiSubWindow* w);

	void hideWindow(MdiSubWindow* window);
	void hideWindow();
	void hideActiveWindow();
	void activateWindow();
	void activateWindow(MdiSubWindow *);
	void repaintWindows();
	//@}

	//! Show about dialog
	static void about();
	//! Return a version string ("QtiPlot x.y.z")
	static QString versionString();
	void removeCurves(const QString& name);
	QStringList dependingPlots(const QString& caption);
	QStringList depending3DPlots(Matrix *m);
	QStringList multilayerDependencies(QWidget *w);

	void saveAsTemplate(MdiSubWindow* w = 0, const QString& = QString());
	void openTemplate();
	MdiSubWindow* openTemplate(const QString& fn);

	QString windowGeometryInfo(MdiSubWindow *w);
	void restoreWindowGeometry(ApplicationWindow *app, MdiSubWindow *w, const QString s);
	void restoreApplicationGeometry();
	void resizeActiveWindow();
	void resizeWindow();

	//! \name List View in Project Explorer
	//@{
	void setListView(const QString& caption,const QString& view);
	void renameListViewItem(const QString& oldName,const QString& newName);
	void setListViewDate(const QString& caption,const QString& date);
	QString listViewDate(const QString& caption);
	void setListViewSize(const QString& caption,const QString& size);
	void setListViewLabel(const QString& caption,const QString& label);
	//@}

	void updateColNames(const QString& oldName, const QString& newName);
	void updateTableNames(const QString& oldName, const QString& newName);
	void changeMatrixName(const QString& oldName, const QString& newName);
	void updateCurves(Table *t, const QString& name);

	void showTable(const QString& curve);
	void showTable(int i);

	void addColToTable();
	void cutSelection();
	void copySelection();
	void copyMarker();
	void pasteSelection();
	void clearSelection();
	void copyActiveLayer();

	void newProject();

	//! Creates a new empty multilayer plot
	MultiLayer* newGraph(const QString& caption = tr("Graph"));

	//! \name Reading from a Project File
	//@{
	
	Table* openTable(ApplicationWindow* app, const QStringList &flist);
	TableStatistics* openTableStatistics(const QStringList &flist);
	Graph3D* openSurfacePlot(ApplicationWindow* app, const QStringList &lst);
	Graph* openGraph(ApplicationWindow* app, MultiLayer *plot, const QStringList &list);
	void openRecentProject(int index);
	//@}

	//! \name Table Tools
	//@{
	void sortSelection();
	void sortActiveTable();
	void normalizeSelection();
	void normalizeActiveTable();
	void correlate();
	void autoCorrelate();
	void convolute();
	void deconvolute();
	void clearTable();
	void goToRow();
	void goToColumn();
	//@}

	//! \name Plot Tools
	//@{
	void newLegend();
	void addTimeStamp();
	void drawLine();
	void drawArrow();
	void drawPoints();
	void addText();
	void disableAddText();
	void addImage();
	void zoomIn();
	void zoomOut();
	void setAutoScale();
	void showRangeSelectors();
	void showCursor();
	void showScreenReader();
	void pickPointerCursor();
	void disableTools();
	void selectMultiPeak();
	void pickDataTool( QAction* action );

	void updateLog(const QString& result);
	//@}

	//! \name Fitting
	//@{
	void deleteFitTables();
	void fitLinear();
	void fitSigmoidal();
	void fitGauss();
	void fitLorentz();
	void fitMultiPeak(int profile);
	void fitMultiPeakGauss();
	void fitMultiPeakLorentz();
	//@}

	//! \name Calculus
	//@{
	void integrate();
	void differentiate();
	void analysis(Analysis operation);
	void analyzeCurve(Graph *g, Analysis operation, const QString& curveTitle);
	void showDataSetDialog(Analysis operation);
	//@}

	void addErrorBars();
        void defineErrorBars(const QString& name,int type,const QString& percent,int direction,bool drawAll);
	void defineErrorBars(const QString& curveName,const QString& errColumnName, int direction);
	void removeErrorBars();
	void removeErrorBars(const QString& name);
	void movePoints();
	void removePoints();

	//! \name Event Handlers
	//@{
	void closeEvent( QCloseEvent*);
	void timerEvent ( QTimerEvent *e);
	void dragEnterEvent( QDragEnterEvent* e );
    void dragMoveEvent( QDragMoveEvent* e );//Mantid
	void dropEvent( QDropEvent* e );
	void customEvent( QEvent* e);
	//@}

	//! \name Dialogs
	//@{
	void showFindDialogue();
	//! Show plot style dialog for the active MultiLayer / activeGraph / specified curve or the activeGraph options dialog if no curve is specified (curveKey = -1).
	void showPlotDialog(int curveKey = -1);
	QDialog* showScaleDialog();
	QDialog* showPlot3dDialog();
	AxesDialog* showScalePageFromAxisDialog(int axisPos);
	AxesDialog* showAxisPageFromAxisDialog(int axisPos);
	void showAxisDialog();
	void showGridDialog();
	void showGeneralPlotDialog();
  void showLogWindow(bool show);
	void showResults(bool ok);
	void showResults(const QString& s, bool ok=true);
	void showTextDialog();
	void showLineDialog();
	void showTitleDialog();
	void showExportASCIIDialog();
	void showCurvesDialog();
	void showCurveRangeDialog();
	CurveRangeDialog* showCurveRangeDialog(Graph *g, int curve);
	AssociationsDialog* showPlotAssociations(int curve);

	void showAxisTitleDialog();
	void showColumnOptionsDialog();
	void showRowsDialog();
	void showDeleteRowsDialog();
	void showColsDialog();
	void showColMenu(int c);
	void showColumnValuesDialog();

	void showGraphContextMenu();
	void showTableContextMenu(bool selection);
	void showWindowContextMenu();
	void customWindowTitleBarMenu(MdiSubWindow *w, QMenu *menu);
	void showCurveContextMenu(int curveKey);
	void showCurvePlotDialog();
	void showCurveWorksheet();
    void showCurveWorksheet(Graph *g, int curveIndex);
	void showWindowPopupMenu(Q3ListViewItem *it, const QPoint &p, int);

	//! Connected to the context menu signal from lv; it's called when there are several items selected in the list
	void showListViewSelectionMenu(const QPoint &p);

	//! Connected to the context menu signal from lv; it's called when there are no items selected in the list
	void showListViewPopupMenu(const QPoint &p);

	void showScriptWindow();
        void showScriptInterpreter();
	void showMoreWindows();
	void showMarkerPopupMenu();
	void showHelp();
	static void showStandAloneHelp();
	void chooseHelpFolder();
	void showPlotWizard();
	void showFitPolynomDialog();
	void showIntegrationDialog();
	void showInterpolationDialog();
	void showExpGrowthDialog();
	void showExpDecayDialog();
	void showExpDecayDialog(int type);
	void showTwoExpDecayDialog();
	void showExpDecay3Dialog();
	void showRowStatistics();
	void showColStatistics();
	void showFitDialog();
	void showImageDialog();
	void showLayerDialog();
	void showPreferencesDialog();
	void showMatrixDialog();
	void showMatrixSizeDialog();
	void showMatrixValuesDialog();
	void showSmoothSavGolDialog();
	void showSmoothFFTDialog();
	void showSmoothAverageDialog();
    void showSmoothDialog(int m);
	void showFilterDialog(int filter);
	void lowPassFilterDialog();
	void highPassFilterDialog();
	void bandPassFilterDialog();
	void bandBlockFilterDialog();
	void showFFTDialog();
	void showColorMapDialog();
	//@}

	void translateCurveHor();
	void translateCurveVert();

	//! Removes the curve identified by a key stored in the data() of actionRemoveCurve.
	void removeCurve();
	void hideCurve();
	void hideOtherCurves();
	void showAllCurves();
	void setCurveFullRange();

	void setAscValues();
	void setRandomValues();
	void setXCol();
	void setYCol();
	void setZCol();
	void setXErrCol();
	void setYErrCol();
	void setLabelCol();
	void disregardCol();
	void setReadOnlyCol();
	void setReadOnlyColumns();
	void setReadWriteColumns();
	void swapColumns();
	void moveColumnRight();
	void moveColumnLeft();
	void moveColumnFirst();
	void moveColumnLast();

	void updateConfirmOptions(bool askTables, bool askMatrixes, bool askPlots2D, bool askPlots3D, bool askNotes,bool askInstrWindow);
	

	//! \name Plot3D Tools
	//@{
	void toggle3DAnimation(bool on = true);
	 //! Turns perspective mode on or off
  	void togglePerspective(bool on = true);
  	//! Resets rotation of 3D plots to default values
  	void resetRotation();
  	//! Finds best layout for the 3D plot
  	void fitFrameToLayer();
	void setFramed3DPlot();
	void setBoxed3DPlot();
	void removeAxes3DPlot();
	void removeGrid3DPlot();
	void setHiddenLineGrid3DPlot();
	void setLineGrid3DPlot();
	void setPoints3DPlot();
	void setCrosses3DPlot();
	void setCones3DPlot();
	void setBars3DPlot();
	void setFilledMesh3DPlot();
	void setEmptyFloor3DPlot();
	void setFloorData3DPlot();
	void setFloorIso3DPlot();
	void setFloorGrid3DPlot(bool on);
	void setCeilGrid3DPlot(bool on);
	void setRightGrid3DPlot(bool on);
	void setLeftGrid3DPlot(bool on);
	void setFrontGrid3DPlot(bool on);
	void setBackGrid3DPlot(bool on);
	void pickPlotStyle( QAction* action );
	void pickCoordSystem( QAction* action);
	void pickFloorStyle( QAction* action);
	void custom3DActions(QMdiSubWindow *w);
	void custom3DGrids(int grids);
	//@}

	void updateRecentProjectsList();

	//!  connected to the done(bool) signal of the http object
	//void receivedVersionFile(bool error);
	//!  called when the user presses the actionCheckUpdates
	//void searchForUpdates();

	//! Open support page in external browser
	//void showSupportPage();
	//! Open donation page in external browser
	//void showDonationsPage();
	//! Open QtiPlot homepage in external browser
	void showHomePage();
	//! Open forums page at berliOS in external browser
	//void showForums();
	//! Open bug tracking system at berliOS in external browser
	void showBugTracker();
	//! Show download page in external browser
	//void downloadManual();
	//! Show translations page in external browser
	//void downloadTranslation();
#ifdef QTIPLOT_DEMO
	//! Shown when the user tries to save the project.
	void showDemoVersionMessage();
#endif

	void parseCommandLineArguments(const QStringList& args);
	void createLanguagesList();
	void switchToLanguage(int param);
	void switchToLanguage(const QString& locale);

	bool alreadyUsedName(const QString& label);
	bool projectHas2DPlots();

	//! Returns a pointer to the window named "name"
	MdiSubWindow* window(const QString& name);

	//! Returns a list with the names of all the matrices in the project
	QStringList matrixNames();

	/// returns a list of all the mantid matrix objects in the project
	QStringList mantidmatrixNames();


	//! \name Notes
	//@{
 	//! Creates a new empty note window
	Note* newNote(const QString& caption = QString());
	Note* openNote(ApplicationWindow* app, const QStringList &flist);
	void saveNoteAs();
	//@}

	//! \name Folders
	//@{
	//! Returns a pointer to the current folder in the project
	Folder* currentFolder(){return current_folder;};
	//! Adds a new folder to the project
	void addFolder();
	Folder* addFolder(QString name, Folder* parent = NULL);
	//! Deletes the current folder
	void deleteFolder();

	//! Ask confirmation from user, deletes the folder f if user confirms and returns true, otherwise returns false;
	bool deleteFolder(Folder *f);

	//! Deletes the currently selected items from the list view #lv.
	void deleteSelectedItems();
	//! Hides the currently selected windows from the list view #lv.
	void hideSelectedWindows();
    //! Show the currently selected windows from the list view #lv.
	void showSelectedWindows();

	//! Sets all items in the folders list view to be desactivated (QPixmap = folder_closed_xpm)
	void desactivateFolders();

	//! Changes the current folder. Returns true if successfull
	bool changeFolder(Folder *newFolder, bool force = false);

	//! Changes the current folder when the user changes the current item in the QListView "folders"
	void folderItemChanged(Q3ListViewItem *it);
	//! Changes the current folder when the user double-clicks on a folder item in the QListView "lv"
	void folderItemDoubleClicked(Q3ListViewItem *it);

	//!  creates and opens the context menu of a folder list view item
	/**
	 * \param it list view item
	 * \param p mouse global position
	 * \param fromFolders: true means that the user clicked right mouse buttom on an item from QListView "folders"
	 *					   false means that the user clicked right mouse buttom on an item from QListView "lv"
	 */
	void showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, bool fromFolders);

	//!  connected to the SIGNAL contextMenuRequested from the list views
	void showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, int);

	//!  starts renaming the selected folder by creating a built-in text editor
	void startRenameFolder();

	//!  starts renaming the selected folder by creating a built-in text editor
	void startRenameFolder(Q3ListViewItem *item);

	//!  checks weather the new folder name is valid and modifies the name
	void renameFolder(Q3ListViewItem *it, int col, const QString &text);

	//!  forces showing all windows in the current folder and subfolders, depending on the user's viewing policy
	void showAllFolderWindows();

	//!  forces hidding all windows in the current folder and subfolders, depending on the user's viewing policy
	void hideAllFolderWindows();

	//!  hides all windows in folder f
	void hideFolderWindows(Folder *f);

	//!  pops up folder information
	void folderProperties();

	//!  pops up information about the selected window item
	void windowProperties();

	//!  pops up information about the current project
	void projectProperties();

	//! Pops up a file dialog and invokes appendProject(const QString&) on the result.
	void appendProject();
	//! Open the specified project file and add it as a subfolder to the parentFolder or to the current folder if no parent folder is specified.
	Folder* appendProject(const QString& file_name, Folder* parentFolder = 0);
	void saveAsProject();
	void saveFolderAsProject(Folder *f);
	void saveFolder(Folder *folder, const QString& fn, bool compress = false);

	//!  adds a folder list item to the list view "lv"
	void addFolderListViewItem(Folder *f);

	//!  adds a widget list item to the list view "lv"
	void addListViewItem(MdiSubWindow *w);

	//!  hides or shows windows in the current folder and changes the view windows policy
	void setShowWindowsPolicy(int p);

	//!  returns a pointer to the root project folder
	Folder* projectFolder();

	//!  used by the findDialog
	void find(const QString& s, bool windowNames, bool labels, bool folderNames,
			  bool caseSensitive, bool partialMatch, bool subfolders);

	//!  initializes the list of items dragged by the user
	void dragFolderItems(QList<Q3ListViewItem *> items){draggedItems = items;};

	//!  Drop the objects in the list draggedItems to the folder of the destination item
	void dropFolderItems(Q3ListViewItem *dest);

	//!  moves a folder item to another
	/**
	 * \param src source folder item
	 * \param dest destination folder item
	 */
	void moveFolder(FolderListItem *src, FolderListItem *dest);
	//!  copies a folder to another
	/**
	 * \param src source folder
	 * \param dest destination folder
     */
	bool copyFolder(Folder *src, Folder *dest);

	void foldersMenuActivated( int id );
	//@}

	//! \name Scripting
	//@{
	//! execute all notes marked auto-exec
	void executeNotes();
	//! show scripting language selection dialog
	void showScriptingLangDialog();
	//! print to scripting console (if available) or to stdout
        void scriptPrint(const QString &text,bool error = false, bool timestamp = false);
	//! switches to the given scripting language; 
	bool setScriptingLanguage(const QString &lang);

	void scriptsDirPathChanged(const QString& path);
	//@}

	void showToolBarsMenu();
  void savetoNexusFile();

  //Slot for writing to log window
  void writeToLogWindow(const QString& message,bool error = false);
  /// Write an error message to the log window (convenience slot)
  void writeErrorToLogWindow(const QString& message);
  /// execute loadraw asynchronously
  void executeLoadRawAsynch(const QString& fileName,const QString& wsName ) ;
  
  /// execute loadnexus asynchronously
  void executeLoadNexusAsynch(const QString& fileName,const QString& wsName ) ;

  /// execute loadraw/nexus without popingup load dialogs.
  void executeloadAlgorithm(const QString&, const QString&, const QString&);

  /// slot to execute download datafiles algorithm - called  from ICat interface
  void executeDownloadDataFiles(const std::vector<std::string>&,const std::vector<long long>&);
 
signals:
	void modified();
	void resultsContextMenu();

private:
	virtual QMenu * createPopupMenu(){return NULL;};
	///void open spectrogram plot from project
	Spectrogram*  openSpectrogram(Graph*ag,const std::string &wsName,const QStringList &lst);
	Matrix* openMatrix(ApplicationWindow* app, const QStringList &flist);
	void openMantidMatrix(const QStringList &lst);
	MantidMatrix* newMantidMatrix(const QString& wsName,int lower,int upper);
	void openScriptWindow(const QStringList &list);
	void populateMantidTreeWdiget(const QString &s);
	void openInstrumentWindow(const QStringList &list);
	/// this method saves the data on project save
	void savedatainNexusFormat(const std::string& wsName,const std::string & fileName);


private slots:
    //! \name Initialization
	//@{
	void insertTranslatedStrings();
	void translateActionsStrings();
	void init(bool factorySettings = false);
	void initGlobalConstants();
	void createActions();
	void initMainMenu();
	void initToolBars();
	void initPlot3DToolBar();
	void disableActions();
	void customColumnActions();
	void disableToolbars();
	void customToolBars(QMdiSubWindow* w);
  void customMultilayerToolButtons(MultiLayer* w);
	void customMenu(QMdiSubWindow* w);
	void windowActivated(QMdiSubWindow *w);
	//@}

	void analysisMenuAboutToShow();
	void fileMenuAboutToShow();
	void editMenuAboutToShow();
	void matrixMenuAboutToShow();
	void plotMenuAboutToShow();
	void plotDataMenuAboutToShow();
	void tableMenuAboutToShow();
	void windowsMenuAboutToShow();
	void windowsMenuActivated( int id );

	//! \name Font Format Functions
	//@{
	void enableTextEditor(Graph *g);
	void setFormatBarFont(const QFont &);
	void setFontSize(int);
	void setFontFamily(const QFont &);
	void setItalicFont(bool);
	void setBoldFont(bool);
    void insertSuperscript();
    void insertSubscript();
    void underline();
    void insertGreekSymbol();
    void insertGreekMajSymbol();
    void insertMathSymbol();
	//@}

	void showCustomActionDialog();
  void showUserDirectoryDialog();
	void performCustomAction(QAction *);
  void runPythonScript(const QString & code, bool quiet=false);

	void hideSelectedColumns();
	void showAllColumns();
	void closedLastCopiedLayer(){lastCopiedLayer = NULL;};

  /// context menu for log window
  void showLogWindowContextMenu(const QPoint &p);
  /// context menu for scripting console
  void showScriptConsoleContextMenu(const QPoint &p);
  ///
  void showMantidConcepts();

   /// show MantidPlot Help webpage
  void showmantidplotHelp();

  /// for zooming the selected graph using mouse drag tool
  void magnify();

  /// Handler for ICat login menu 
  void ICatLogin();
  /// Handler for ICat search menu
  void ICatIsisSearch();
  /// Handler for ICatMyData serch menu
  void ICatMyDataSearch();
// Handler for ICat Logout
  void ICatLogout();

  void ICatAdvancedSearch();

  /// method to create widgets from mantid qt;
  void setGeometry(QMdiSubWindow* usr_win,QWidget* user_interface);


  ///
  void showalgorithmDescriptions();

// TODO: a lot of this stuff should be private
public:
	//! End of line convention used for copy/paste operations and when exporting tables/matrices to ASCII files.
	EndLineChar d_eol;
    //! Flag telling if the in-place editing of 2D plot labels is enabled
    bool d_in_place_editing;
	QString d_python_config_folder;
	QString d_translations_folder;
	//! Flag telling if the application is opening a project file or not
	bool d_opening_file;
    QString customActionsDirPath;
	bool d_matrix_tool_bar, d_file_tool_bar, d_table_tool_bar, d_column_tool_bar, d_edit_tool_bar;
	bool d_plot_tool_bar, d_plot3D_tool_bar, d_display_tool_bar, d_format_tool_bar;
	bool d_backup_files;
	WindowType d_init_window_type;
	QRect d_script_win_rect, d_app_rect;
	bool d_script_win_on_top;
        bool d_script_win_arrow;
	bool d_inform_rename_table;
	QString d_export_col_separator;
	bool d_export_col_names, d_export_table_selection, d_export_col_comment;

    bool d_thousands_sep;
    //! Last selected filter in export image dialog
    QString d_image_export_filter;
    bool d_keep_plot_aspect;
    int d_export_vector_size;
    bool d_export_transparency;
    int d_export_quality;
    int d_export_resolution;
    bool d_export_color;
	//! Locale used to specify the decimal separators in imported ASCII files
	QLocale d_ASCII_import_locale;
	//! End of line convention used to import ASCII files.
	EndLineChar d_ASCII_end_line;
    //! Last selected filter in import ASCII dialog
    QString d_ASCII_file_filter, d_ASCII_comment_string;
	bool d_import_dec_separators, d_ASCII_import_comments, d_ASCII_import_read_only, d_ASCII_import_preview;
	int d_ASCII_import_mode, d_preview_lines;
	//! Specifies if only the Tables/Matrices in the current folder should be displayed in the Add/remove curve dialog.
	bool d_show_current_folder;
	bool d_scale_plots_on_print, d_print_cropmarks;
	bool d_show_table_comments;
	bool d_extended_plot_dialog;
	bool d_extended_import_ASCII_dialog;
	bool d_extended_export_dialog;
	bool d_extended_open_dialog;
	bool generateUniformFitPoints;
	bool generatePeakCurves;
	int peakCurvesColor;
	//! User defined size for the Add/Remove curves dialog
	QSize d_add_curves_dialog_size;

	//! Scale the errors output in fit operations with reduced chi^2
	bool fit_scale_errors;

	//! Number of points in a generated fit curve
	int fitPoints;

	//! Calculate only 2 points in a generated linear fit function curve
	bool d_2_linear_fit_points;

	bool pasteFitResultsToPlot;

	//! Write fit output information to Result Log
	bool writeFitResultsToLog;

	//! precision used for the output of the fit operations
	int fit_output_precision;

	//! default precision to be used for all other operations than fitting
	int d_decimal_digits;

	//! pointer to the current folder in the project
	Folder *current_folder;
	//! Describes which windows are shown when the folder becomes the current folder
	ShowWindowsPolicy show_windows_policy;
	enum {MaxRecentProjects = 10};
	//! File version code used when opening project files (= maj * 100 + min * 10 + patch)
	int d_file_version;

	QColor workspaceColor, panelsColor, panelsTextColor;
	QString appStyle, workingDir;

	//! Path to the folder where the last template file was opened/saved
	QString templatesDir;
	bool smooth3DMesh, autoScaleFonts, autoResizeLayers, askForSupport, autoSearchUpdates;
	bool confirmCloseTable, confirmCloseMatrix, confirmClosePlot2D, confirmClosePlot3D,confirmCloseInstrWindow;
	bool confirmCloseFolder, confirmCloseNotes;
	bool titleOn, autoSave, drawBackbones, allAxesOn, autoscale2DPlots, antialiasing2DPlots;
	QString xaxisScale, yaxisScale, zaxisScale;

	int majTicksStyle, minTicksStyle, legendFrameStyle, autoSaveTime, axesLineWidth, canvasFrameWidth;
	QColor legendBackground, legendTextColor, defaultArrowColor;
	int defaultArrowHeadLength, defaultArrowHeadAngle;
	double defaultArrowLineWidth, defaultCurveLineWidth;
	bool defaultArrowHeadFill;
	Qt::PenStyle defaultArrowLineStyle;
	int majTicksLength, minTicksLength, defaultPlotMargin;
	int defaultCurveStyle, defaultSymbolSize;
  bool applyCurveStyleToMantid; ///< if true defaultCurveStyle, defaultSymbolSize are applyed to MantidCurves
	QFont appFont, plot3DTitleFont, plot3DNumbersFont, plot3DAxesFont;
	QFont tableTextFont, tableHeaderFont, plotAxesFont, plotLegendFont, plotNumbersFont, plotTitleFont;
	QColor tableBkgdColor, tableTextColor, tableHeaderColor;
	QString projectname,columnSeparator, helpFilePath, appLanguage;
	QString configFilePath, fitPluginsPath, fitModelsPath, asciiDirPath, imagesDirPath, scriptsDirPath;
	int ignoredLines, savingTimerId, plot3DResolution, recentMenuID;
	bool renameColumns, strip_spaces, simplify_spaces;
	QStringList recentProjects;
	bool saved, showPlot3DProjection, showPlot3DLegend, orthogonal3DPlots, autoscale3DPlots;
	QStringList plot3DColors, locales;
	QStringList functions; //user-defined functions;
	QStringList xFunctions, yFunctions, rFunctions, thetaFunctions; // user functions for parametric and polar plots
	QStringList surfaceFunc; //user-defined surface functions;
    QStringList d_param_surface_func; //user-defined parametric surface functions;
	//! List of tables and matrices renamed in order to avoid conflicts when appending a project to a folder
	QStringList renamedTables;
  // List of removed interfaces
  QStringList removed_interfaces;
  // List of PyQt interfaces to be added to the Interfaces menu
	QStringList pyqt_interfaces;

	//! \name variables used when user copy/paste markers
	//@{
	LegendWidget *d_text_copy;
	ArrowMarker *d_arrow_copy;
	ImageMarker *d_image_copy;
	//@}

	//! Equals true if an automatical search for updates was performed on start-up otherwise is set to false;
	bool autoSearchUpdatesRequest;

	//! The scripting language to use for new projects.
	QString defaultScriptingLang;

private:
	MdiSubWindow *d_active_window;
    TextEditor *d_text_editor;
    QLocale d_locale;
	// Flag telling if table values should be automatically recalculated when values in a column are modified.
	bool d_auto_update_table_values;
    int d_matrix_undo_stack_size;

	//! Workaround for the new colors introduced in rev 447
	int convertOldToNewColorIndex(int cindex);

	//! Stores the pointers to the dragged items from the FolderListViews objects
	QList<Q3ListViewItem *> draggedItems;

	Graph *lastCopiedLayer;
	QSplitter *explorerSplitter;

//	QAssistantClient *assistant;
	ScriptingWindow *scriptingWindow; //Mantid
        Script *m_iface_script;
	QTranslator *appTranslator, *qtTranslator;
	QDockWidget *explorerWindow, *undoStackWindow;
	QTextEdit *results;
#ifdef SCRIPTING_CONSOLE
	QDockWidget *consoleWindow;
	QTextEdit *console;
        QDockWidget *m_interpreterDock;
        ScriptManagerWidget *m_scriptInterpreter;
#endif
	QMdiArea *d_workspace;

  QToolBar *fileTools, *plotTools, *tableTools, *columnTools, *plot3DTools, *displayBar, *editTools, *plotMatrixBar;
	QToolBar *formatToolBar;
	QToolButton *btnResults;
	QWidgetList *hiddenWindows;
	QLineEdit *info;

	QMenu *windowsMenu, *foldersMenu, *view, *graph, *fileMenu, *format, *edit, *recent;
	QMenu *help, *plot2DMenu, *analysisMenu, *multiPeakMenu, *icat;
	QMenu *matrixMenu, *plot3DMenu, *plotDataMenu, *tablesDepend, *scriptingMenu;
	QMenu *tableMenu, *fillMenu, *normMenu, *newMenu, *exportPlotMenu, *smoothMenu, *filterMenu, *decayMenu,*saveMenu,*openMenu;

	QAction *actionEditCurveRange, *actionCurveFullRange, *actionShowAllCurves, *actionHideCurve, *actionHideOtherCurves;
	QAction *actionEditFunction, *actionRemoveCurve, *actionShowCurveWorksheet, *actionShowCurvePlotDialog;
    QAction *actionNewProject, *actionNewNote, *actionNewTable, *actionNewFunctionPlot,*actionSaveFile;
    QAction *actionNewSurfacePlot, *actionNewMatrix, *actionNewGraph, *actionNewFolder;
    QAction *actionOpen, *actionLoadImage, *actionSaveProject, *actionSaveProjectAs, *actionImportImage,*actionLoadFile,*actionOpenProj,*actionOpenRaw,*actionOpenNexus;
      QAction *actionLoad, *actionUndo, *actionRedo;
    QAction *actionCopyWindow, *actionShowAllColumns, *actionHideSelectedColumns;
    QAction *actionCutSelection, *actionCopySelection, *actionPasteSelection, *actionClearSelection;
    QAction *actionShowExplorer, *actionShowLog, *actionAddLayer, *actionShowLayerDialog, *actionAutomaticLayout,*actionclearAllMemory;
	QAction *actionICatLogin,*actionICatSearch,*actionMydataSearch,*actionICatLogout,*actionAdvancedSearch;
#ifdef SCRIPTING_CONSOLE
    QAction *actionShowConsole;
#endif
    QAction *actionSwapColumns, *actionMoveColRight, *actionMoveColLeft, *actionMoveColFirst, *actionMoveColLast;
    QAction *actionExportGraph, *actionExportAllGraphs, *actionPrint, *actionPrintAllPlots, *actionShowExportASCIIDialog;
    QAction *actionExportPDF, *actionReadOnlyCol;
  QAction *actionCloseAllWindows, *actionClearLogInfo, *actionClearConsole, *actionShowPlotWizard, *actionShowConfigureDialog;
    QAction *actionShowCurvesDialog, *actionAddErrorBars, *actionRemoveErrorBars, *actionAddFunctionCurve, *actionUnzoom, *actionNewLegend, *actionAddImage, *actionAddText;
    QAction *actionPlotL, *actionPlotP, *actionPlotLP, *actionPlotVerticalDropLines, *actionPlotSpline;
    QAction *actionPlotVertSteps, *actionPlotHorSteps, *actionPlotVerticalBars;
	QAction *actionPlotHorizontalBars, *actionPlotArea, *actionPlotPie, *actionPlotVectXYAM, *actionPlotVectXYXY;
    QAction *actionPlotHistogram, *actionPlotStackedHistograms, *actionPlot2VerticalLayers, *actionPlot2HorizontalLayers, *actionPlot4Layers, *actionPlotStackedLayers;
    QAction *actionPlot3DRibbon, *actionPlot3DBars, *actionPlot3DScatter, *actionPlot3DTrajectory;
    QAction *actionShowColStatistics, *actionShowRowStatistics, *actionShowIntDialog, *actionIntegrate;
    QAction *actionDifferentiate, *actionFitLinear, *actionShowFitPolynomDialog;
    QAction *actionShowExpDecayDialog, *actionShowTwoExpDecayDialog, *actionShowExpDecay3Dialog;
    QAction *actionFitExpGrowth, *actionFitSigmoidal, *actionFitGauss, *actionFitLorentz, *actionShowFitDialog;
    QAction *actionShowAxisDialog, *actionShowTitleDialog;
    QAction *actionShowColumnOptionsDialog, *actionShowColumnValuesDialog, *actionShowColsDialog, *actionShowRowsDialog;
    QAction *actionTableRecalculate;
    QAction *actionAbout, *actionShowHelp, *actionChooseHelpFolder,*actionMantidConcepts,*actionMantidAlgorithms,*actionmantidplotHelp;
    QAction *actionRename, *actionCloseWindow, *actionConvertTable;
    QAction *actionAddColToTable, *actionDeleteLayer, *actionInterpolate;
    QAction *actionResizeActiveWindow, *actionHideActiveWindow;
    QAction *actionShowMoreWindows, *actionPixelLineProfile, *actionIntensityTable;
    QAction *actionShowLineDialog, *actionShowImageDialog, *actionShowTextDialog;
    QAction *actionActivateWindow, *actionMinimizeWindow, *actionMaximizeWindow, *actionHideWindow, *actionResizeWindow;
    QAction *actionEditSurfacePlot, *actionAdd3DData;
	QAction *actionMatrixDeterminant, *actionSetMatrixProperties, *actionConvertMatrixXYZ, *actionConvertMatrixYXZ;
	QAction *actionSetMatrixDimensions, *actionConvertMatrixDirect, *actionSetMatrixValues, *actionTransposeMatrix, *actionInvertMatrix;
	QAction *actionPlot3DWireFrame, *actionPlot3DHiddenLine, *actionPlot3DPolygons, *actionPlot3DWireSurface;
  QAction *actionColorMap, *actionContourMap, *actionGrayMap, *actionNoContourColorMap;
	QAction *actionDeleteFitTables, *actionShowGridDialog, *actionTimeStamp;
	QAction *actionSmoothSavGol, *actionSmoothFFT, *actionSmoothAverage, *actionFFT;
	QAction *actionLowPassFilter, *actionHighPassFilter, *actionBandPassFilter, *actionBandBlockFilter;
	QAction *actionSortTable, *actionSortSelection, *actionNormalizeSelection;
	QAction *actionNormalizeTable, *actionConvolute, *actionDeconvolute, *actionCorrelate, *actionAutoCorrelate;
	QAction *actionTranslateHor, *actionTranslateVert, *actionSetAscValues, *actionSetRandomValues;
	QAction *actionSetXCol, *actionSetYCol, *actionSetZCol, *actionSetLabelCol, *actionDisregardCol, *actionSetXErrCol, *actionSetYErrCol;
	QAction *actionBoxPlot, *actionMultiPeakGauss, *actionMultiPeakLorentz, *actionCheckUpdates;
	QAction *actionDonate, *actionHomePage, *actionDownloadManual, *actionTechnicalSupport, *actionTranslations;
	QAction *actionHelpForums, *actionHelpBugReports;
	QAction *actionShowPlotDialog, *actionShowScaleDialog, *actionOpenTemplate, *actionSaveTemplate;
	QAction *actionNextWindow, *actionPrevWindow;
	QAction *actionScriptingLang,*actionClearTable, *actionGoToRow, *actionGoToColumn;
	QAction *actionNoteExecute, *actionNoteExecuteAll, *actionNoteEvaluate, *actionSaveNote;
  QAction *actionShowScriptWindow, *actionShowScriptInterpreter;
	QAction *actionAnimate, *actionPerspective, *actionFitFrame, *actionResetRotation;
    QAction *actionDeleteRows, *actionDrawPoints;
	QAction *btnCursor, *btnSelect, *btnPicker, *btnRemovePoints, *btnMovePoints, /* *btnPeakPick,*/ *btnMultiPeakPick;
	QAction *btnZoomIn, *btnZoomOut, *btnPointer, *btnLine, *btnArrow;
	QAction *actionFlipMatrixVertically, *actionFlipMatrixHorizontally, *actionRotateMatrix;
	QAction *actionViewMatrixImage, *actionViewMatrix, *actionExportMatrix;
    QAction *actionMatrixGrayScale, *actionMatrixRainbowScale, *actionMatrixCustomScale, *actionRotateMatrixMinus;
    QAction *actionMatrixXY, *actionMatrixColumnRow, *actionImagePlot, *actionToolBars;
    QAction *actionMatrixFFTDirect, *actionMatrixFFTInverse;
	QAction *actionFontBold, *actionFontItalic, *actionFontBox, *actionFontSize;
	QAction *actionSuperscript, *actionSubscript, *actionUnderline, *actionGreekSymbol, *actionCustomActionDialog, *actionManageDirs;
	QAction *actionGreekMajSymbol, *actionMathSymbol;
	QAction *Box, *Frame, *None;
    QAction *front, *back, *right, *left, *ceil, *floor, *floordata, *flooriso, *floornone;
    QAction *wireframe, *hiddenline, *polygon, *filledmesh, *pointstyle, *barstyle, *conestyle, *crossHairStyle;
    QAction *actionShowUndoStack;
    QActionGroup *coord, *floorstyle, *grids, *plotstyle, *dataTools;
	QAction *actionMagnify;

    QList<QAction *> d_user_actions;
    QList<QMenu* > d_user_menus; //Mantid

    QUndoView *d_undo_view;
    /// list of mantidmatrix windows opened from project file.
    QList<MantidMatrix*> m_mantidmatrixWindows;

    friend class MantidUI;
    QString m_nexusInputWSName;

    // Store initialized script environments
    QHash<QString, ScriptingEnv*> m_script_envs;
  /// Store a list of environments that cannot be used
  QSet<QString> m_bad_script_envs;

public:
    MantidUI *mantidUI;
};
#endif
