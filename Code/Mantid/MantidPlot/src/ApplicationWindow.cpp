/***************************************************************************
	File                 : ApplicationWindow.cpp
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
#include "globals.h"
#include "ApplicationWindow.h"
#include "pixmaps.h"
#include "CurvesDialog.h"
#include "PlotDialog.h"
#include "AxesDialog.h"
#include "LineDialog.h"
#include "TextDialog.h"
#include "ExportDialog.h"
#include "TableDialog.h"
#include "SetColValuesDialog.h"
#include "ErrDialog.h"
#include "LegendWidget.h"
#include "ArrowMarker.h"
#include "ImageMarker.h"
#include "Graph.h"
#include "Plot.h"
#include "Grid.h"
#include "PlotWizard.h"
#include "PolynomFitDialog.h"
#include "ExpDecayDialog.h"
#include "FunctionDialog.h"
#include "FitDialog.h"
#include "SurfaceDialog.h"
#include "Graph3D.h"
#include "Plot3DDialog.h"
#include "ImageDialog.h"
#include "MultiLayer.h"
#include "LayerDialog.h"
#include "DataSetDialog.h"
#include "IntDialog.h"
#include "ConfigDialog.h"
#include "MatrixDialog.h"
#include "MatrixSizeDialog.h"
#include "MatrixValuesDialog.h"
#include "MatrixModel.h"
#include "MatrixCommand.h"
#include "importOPJ.h"
#include "AssociationsDialog.h"
#include "RenameWindowDialog.h"
#include "QwtErrorPlotCurve.h"
#include "InterpolationDialog.h"
#include "ImportASCIIDialog.h"
#include "ImageExportDialog.h"
#include "SmoothCurveDialog.h"
#include "FilterDialog.h"
#include "FFTDialog.h"
#include "Note.h"
#include "Folder.h"
#include "FindDialog.h"
#include "ScaleDraw.h"
#include "plot2D/ScaleEngine.h"
#include "ScriptingLangDialog.h"
#include "ScriptingWindow.h"
#include "ScriptManagerWidget.h"
#include "TableStatistics.h"
#include "Fit.h"
#include "MultiPeakFit.h"
#include "PolynomialFit.h"
#include "SigmoidalFit.h"
#include "LogisticFit.h"
#include "NonLinearFit.h"
#include "FunctionCurve.h"
#include "QwtPieCurve.h"
#include "Spectrogram.h"
#include "Integration.h"
#include "Differentiation.h"
#include "SmoothFilter.h"
#include "FFTFilter.h"
#include "Convolution.h"
#include "Correlation.h"
#include "CurveRangeDialog.h"
#include "ColorBox.h"
#include "QwtHistogram.h"
#include "OpenProjectDialog.h"
#include "ColorMapDialog.h"
#include "TextEditor.h"
#include "SymbolDialog.h"
#include "CustomActionDialog.h"
#include "MdiSubWindow.h"

// TODO: move tool-specific code to an extension manager
#include "ScreenPickerTool.h"
#include "DataPickerTool.h"
#include "TranslateCurveTool.h"
#include "MultiPeakFitTool.h"
#include "LineProfileTool.h"
#include "RangeSelectorTool.h"
#include "PlotToolInterface.h"
#include "Mantid/MantidMatrix.h"
#include "Mantid/MantidCurve.h"
#include "ContourLinesEditor.h"
#include "Mantid/InstrumentWidget/InstrumentWindow.h"
#include "Mantid/RemoveErrorsDialog.h"

#include <stdio.h>
#include <stdlib.h>

#include <qwt_scale_engine.h>
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QPrintDialog>
#include <QPixmapCache>
#include <QMenuBar>
#include <QClipboard>
#include <QTranslator>
#include <QSplitter>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QPrinter>
#include <QPrinterInfo>
#include <QActionGroup>
#include <QAction>
#include <QToolBar>
#include <QKeySequence>
#include <QImageReader>
#include <QImageWriter>
#include <QDateTime>
#include <QShortcut>
#include <QDockWidget>
#include <QTextStream>
#include <QVarLengthArray>
#include <QList>
#include <QUrl>
//#include <QAssistantClient>
#include <QFontComboBox>
#include <QSpinBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QUndoStack>
#include <QUndoView>

#include <zlib.h>

//Mantid
#include "ScriptingWindow.h"

#include "Mantid/MantidUI.h"
#include "MantidPlotReleaseDate.h"
#include "Mantid/MantidAbout.h"
#include "Mantid/PeakPickerTool.h"
#include "Mantid/ManageCustomMenus.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtMantidWidgets/ICatSearch.h"
#include "MantidQtMantidWidgets/ICatMyDataSearch.h"
#include "MantidQtMantidWidgets/ICatAdvancedSearch.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"


using namespace Qwt3D;
using namespace MantidQt::API;

extern "C"
{
void file_compress(char  *file, char  *mode);
void file_uncompress(char  *file);
}


ApplicationWindow::ApplicationWindow(bool factorySettings)
: QMainWindow(), Scripted(ScriptingLangManager::newEnv(this))
{
  QCoreApplication::setOrganizationName("ISIS");
  QCoreApplication::setApplicationName("MantidPlot");
  mantidUI = new MantidUI(this);
  setAttribute(Qt::WA_DeleteOnClose);
  init(factorySettings);
}

void ApplicationWindow::init(bool factorySettings)
{
  setWindowTitle(tr("MantidPlot - untitled"));//Mantid
  setObjectName("main application");
  initGlobalConstants();
  QPixmapCache::setCacheLimit(20*QPixmapCache::cacheLimit ());

  tablesDepend = new QMenu(this);

  explorerWindow = new QDockWidget( this );
  explorerWindow->setWindowTitle(tr("Project Explorer"));
  explorerWindow->setObjectName("explorerWindow"); // this is needed for QMainWindow::restoreState()
  explorerWindow->setMinimumHeight(150);
  addDockWidget( Qt::BottomDockWidgetArea, explorerWindow );

  actionSaveFile=NULL;
  actionSaveProject=NULL;
  folders = new FolderListView(this);
  folders->header()->setClickEnabled( false );
  folders->addColumn( tr("Folder") );
  folders->setRootIsDecorated( true );
  folders->setResizeMode(Q3ListView::LastColumn);
  folders->header()->hide();
  folders->setSelectionMode(Q3ListView::Single);

  connect(folders, SIGNAL(currentChanged(Q3ListViewItem *)),
      this, SLOT(folderItemChanged(Q3ListViewItem *)));
  connect(folders, SIGNAL(itemRenamed(Q3ListViewItem *, int, const QString &)),
      this, SLOT(renameFolder(Q3ListViewItem *, int, const QString &)));
  connect(folders, SIGNAL(contextMenuRequested(Q3ListViewItem *, const QPoint &, int)),
      this, SLOT(showFolderPopupMenu(Q3ListViewItem *, const QPoint &, int)));
  connect(folders, SIGNAL(dragItems(QList<Q3ListViewItem *>)),
      this, SLOT(dragFolderItems(QList<Q3ListViewItem *>)));
  connect(folders, SIGNAL(dropItems(Q3ListViewItem *)),
      this, SLOT(dropFolderItems(Q3ListViewItem *)));
  connect(folders, SIGNAL(renameItem(Q3ListViewItem *)),
      this, SLOT(startRenameFolder(Q3ListViewItem *)));
  connect(folders, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
  connect(folders, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));

  current_folder = new Folder( 0, tr("UNTITLED"));
  FolderListItem *fli = new FolderListItem(folders, current_folder);
  current_folder->setFolderListItem(fli);
  fli->setOpen( true );

  lv = new FolderListView();
  lv->addColumn (tr("Name"),-1 );
  lv->addColumn (tr("Type"),-1 );
  lv->addColumn (tr("View"),-1 );
  lv->addColumn (tr("Size"),-1 );
  lv->addColumn (tr("Created"),-1);
  lv->addColumn (tr("Label"),-1);
  lv->setResizeMode(Q3ListView::LastColumn);
  lv->setMinimumHeight(80);
  lv->setSelectionMode(Q3ListView::Extended);
  lv->setDefaultRenameAction (Q3ListView::Accept);

  explorerSplitter = new QSplitter(Qt::Horizontal, explorerWindow);
  explorerSplitter->addWidget(folders);
  explorerSplitter->addWidget(lv);
  explorerWindow->setWidget(explorerSplitter);

  QList<int> splitterSizes;
  explorerSplitter->setSizes( splitterSizes << 45 << 45);
  explorerWindow->hide();

  logWindow = new QDockWidget(this);
  logWindow->setObjectName("logWindow"); // this is needed for QMainWindow::restoreState()
  logWindow->setWindowTitle(tr("Results Log"));
  addDockWidget( Qt::TopDockWidgetArea, logWindow );

  results=new QTextEdit(logWindow);
  results->setReadOnly (true);
  results->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(results, SIGNAL(customContextMenuRequested(const QPoint &)), this, 
	  SLOT(showLogWindowContextMenu(const QPoint &)));
  logWindow->setWidget(results);
  logWindow->hide();

  //#ifdef SCRIPTING_CONSOLE
  consoleWindow = new QDockWidget(this);
  consoleWindow->setObjectName("consoleWindow"); // this is needed for QMainWindow::restoreState()
  consoleWindow->setWindowTitle(tr("Scripting Console"));
  addDockWidget( Qt::TopDockWidgetArea, consoleWindow );
  console = new QTextEdit(consoleWindow);
  console->setReadOnly(true);
  console->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(console, SIGNAL(customContextMenuRequested(const QPoint &)), this,
	  SLOT(showScriptConsoleContextMenu(const QPoint &)));
  consoleWindow->setWidget(console);
  consoleWindow->hide();
  //#endif
  m_interpreterDock = new QDockWidget(this);
  m_interpreterDock->setObjectName("interpreterDock"); // this is needed for QMainWindow::restoreState()
  m_interpreterDock->setWindowTitle("Script Interpreter");
  addDockWidget( Qt::BottomDockWidgetArea, m_interpreterDock );
  // This is a temporary widget so that when the settings are read the dock's visible state is correct.
  // It gets replaced with the script widget after the scripting language has been read and set
  m_interpreterDock->setWidget(new QTextEdit);
  m_interpreterDock->hide();

  undoStackWindow = new QDockWidget(this);
  undoStackWindow->setObjectName("undoStackWindow"); // this is needed for QMainWindow::restoreState()
  undoStackWindow->setWindowTitle(tr("Undo Stack"));
  addDockWidget(Qt::RightDockWidgetArea, undoStackWindow);

  d_undo_view = new QUndoView(undoStackWindow);
  d_undo_view->setCleanIcon(QIcon(getQPixmap("filesave_xpm")));
  undoStackWindow->setWidget(d_undo_view);
  undoStackWindow->hide();

  //Initialize Mantid
  // MG: 01/02/2009 - Moved this to before scripting so that the logging is connected when
  // we register Python algorithms
  mantidUI->init();

  // Needs to be done after initialization of dock windows,
  // because we now use QDockWidget::toggleViewAction()
  createActions();
  initToolBars();
  initMainMenu();

  d_workspace = new QMdiArea();
  d_workspace->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
  d_workspace->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  d_workspace->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCentralWidget(d_workspace);

  setAcceptDrops(true);

  hiddenWindows = new QList<QWidget*>();

  scriptingWindow = NULL;
  d_text_editor = NULL;

  // List of registered PyQt interfaces
  QString pyqt_interfaces_as_str = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("mantidqt.python_interfaces"));
  pyqt_interfaces = QStringList::split(" ", pyqt_interfaces_as_str);

  renamedTables = QStringList();
  if (!factorySettings)
    readSettings();
  createLanguagesList();
  insertTranslatedStrings();
  disableToolbars();

//  assistant = new QAssistantClient( QString(), this );

  actionNextWindow = new QAction(QIcon(getQPixmap("next_xpm")), tr("&Next","next window"), this);
  actionNextWindow->setShortcut( tr("F5","next window shortcut") );
  connect(actionNextWindow, SIGNAL(activated()), d_workspace, SLOT(activateNextSubWindow()));

  actionPrevWindow = new QAction(QIcon(getQPixmap("prev_xpm")), tr("&Previous","previous window"), this);
  actionPrevWindow->setShortcut( tr("F6","previous window shortcut") );
  connect(actionPrevWindow, SIGNAL(activated()), d_workspace, SLOT(activatePreviousSubWindow()));

  connect(tablesDepend, SIGNAL(activated(int)), this, SLOT(showTable(int)));

  connect(this, SIGNAL(modified()),this, SLOT(modifiedProject()));
  connect(d_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)),
      this, SLOT(windowActivated(QMdiSubWindow*)));
  connect(lv, SIGNAL(doubleClicked(Q3ListViewItem *)),
      this, SLOT(maximizeWindow(Q3ListViewItem *)));
  connect(lv, SIGNAL(doubleClicked(Q3ListViewItem *)),
      this, SLOT(folderItemDoubleClicked(Q3ListViewItem *)));
  connect(lv, SIGNAL(contextMenuRequested(Q3ListViewItem *, const QPoint &, int)),
      this, SLOT(showWindowPopupMenu(Q3ListViewItem *, const QPoint &, int)));
  connect(lv, SIGNAL(dragItems(QList<Q3ListViewItem *>)),
      this, SLOT(dragFolderItems(QList<Q3ListViewItem *>)));
  connect(lv, SIGNAL(dropItems(Q3ListViewItem *)),
      this, SLOT(dropFolderItems(Q3ListViewItem *)));
  connect(lv, SIGNAL(renameItem(Q3ListViewItem *)),
      this, SLOT(startRenameFolder(Q3ListViewItem *)));
  connect(lv, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
  connect(lv, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));
  connect(lv, SIGNAL(itemRenamed(Q3ListViewItem *, int, const QString &)),
      this, SLOT(renameWindow(Q3ListViewItem *, int, const QString &)));

  connect(recent, SIGNAL(activated(int)), this, SLOT(openRecentProject(int)));
  //connect(&http, SIGNAL(done(bool)), this, SLOT(receivedVersionFile(bool)));

  //apply user settings
  updateAppFonts();
  setAppColors(workspaceColor, panelsColor, panelsTextColor, true);

  //Scripting
  m_script_envs = QHash<QString, ScriptingEnv*>();
  setScriptingLanguage(defaultScriptingLang);
  m_scriptInterpreter = new ScriptManagerWidget(scriptingEnv(), m_interpreterDock,true);
  delete m_interpreterDock->widget();
  m_interpreterDock->setWidget(m_scriptInterpreter);
  m_iface_script = NULL;
  loadCustomActions();

  // Print a warning message if the scripting language is set to muParser
  if (defaultScriptingLang == "muParser")
  {
    logWindow->show();
    results->setTextColor(Qt::blue);
    results->insertPlainText("The scripting language is set to muParser. This is probably not what you want! Change the default in View->Preferences.");
    results->setTextColor(Qt::black);
  }
}

void ApplicationWindow::showLogWindowContextMenu(const QPoint & p)
{
  (void)p; //Avoid compiler warning
  QMenu *menu = results->createStandardContextMenu();
  if(!menu) return;
  if(results->text().isEmpty())
  {
    actionClearLogInfo->setEnabled(false);
  }
  else
  {
    actionClearLogInfo->setEnabled(true);
  }

  menu->addAction(actionClearLogInfo);
  menu->popup(QCursor::pos());
}

void ApplicationWindow::showScriptConsoleContextMenu(const QPoint &p)
{
  (void)p;
  QMenu *menu = results->createStandardContextMenu();
  menu->addAction(actionClearConsole);
  menu->popup(QCursor::pos());
}

void ApplicationWindow::initWindow()
{
  switch(d_init_window_type){
  case TableWindow:
    newTable();
    break;
  case MatrixWindow:
    newMatrix();
    break;
  case MultiLayerWindow:
    newGraph();
    break;
  case NoteWindow:
    newNote();
    break;
  default:
    break;
  }
}

void ApplicationWindow::initGlobalConstants()
{
  d_auto_update_table_values = true;
  d_active_window = NULL;
  d_matrix_undo_stack_size = 10;

  d_opening_file = false;
  d_in_place_editing = true;

  d_matrix_tool_bar = true;
  d_file_tool_bar = true;
  d_table_tool_bar = true;
  d_column_tool_bar = true;
  d_edit_tool_bar = true;
  d_plot_tool_bar = true;
  d_plot3D_tool_bar = true;
  d_display_tool_bar = false;
  d_format_tool_bar = true;

  appStyle = qApp->style()->objectName();
  d_app_rect = QRect();
  projectname = "untitled";
  lastCopiedLayer = 0;
  d_text_copy = NULL;
  d_arrow_copy = NULL;
  d_image_copy = NULL;

  savingTimerId = 0;

#ifdef QTIPLOT_DEMO
  QTimer::singleShot(600000, this, SLOT(close()));
#endif

  autoSearchUpdatesRequest = false;

  show_windows_policy = ActiveFolder;
  d_script_win_on_top = false;  //M. Gigg, Mantid
  d_script_win_rect = QRect(0, 0, 600, 660);
  d_init_window_type = NoWindow;

  QString aux = qApp->applicationDirPath();
  workingDir = aux;

#ifdef TRANSLATIONS_PATH
  d_translations_folder = TRANSLATIONS_PATH;
#else
  d_translations_folder = aux + "/translations";
#endif

#ifdef MANUAL_PATH
  helpFilePath = MANUAL_PATH;
  helpFilePath += "/html/index.html";
#else
  helpFilePath = aux + "/manual/index.html";
#endif

#ifdef PYTHON_CONFIG_PATH
  d_python_config_folder = PYTHON_CONFIG_PATH;
#else
  d_python_config_folder = aux;
#endif

  fitPluginsPath = aux + "fitPlugins";
  fitModelsPath = QString::null;
  templatesDir = aux;
  asciiDirPath = aux;
  imagesDirPath = aux;
  scriptsDirPath = aux;
  customActionsDirPath = QString::null;

  appFont = QFont();
  QString family = appFont.family();
  int pointSize = appFont.pointSize();
  tableTextFont = appFont;
  tableHeaderFont = appFont;
  plotAxesFont = QFont(family, pointSize, QFont::Bold, false);
  plotNumbersFont = QFont(family, pointSize );
  plotLegendFont = appFont;
  plotTitleFont = QFont(family, pointSize + 2, QFont::Bold,false);

  plot3DAxesFont = QFont(family, pointSize, QFont::Bold, false );
  plot3DNumbersFont = QFont(family, pointSize);
  plot3DTitleFont = QFont(family, pointSize + 2, QFont::Bold,false);

  autoSearchUpdates = false;
  appLanguage = QLocale::system().name().section('_',0,0);
  show_windows_policy = ApplicationWindow::ActiveFolder;

  workspaceColor = QColor("darkGray");
  panelsColor = QColor("#ffffff");
  panelsTextColor = QColor("#000000");
  tableBkgdColor = QColor("#ffffff");
  tableTextColor = QColor("#000000");
  tableHeaderColor = QColor("#000000");

  plot3DColors = QStringList();
  plot3DColors << "blue";
  plot3DColors << "#000000";
  plot3DColors << "#000000";
  plot3DColors << "#000000";
  plot3DColors << "red";
  plot3DColors << "#000000";
  plot3DColors << "#000000";
  plot3DColors << "#ffffff";

  autoSave = true;
  autoSaveTime = 15;
  d_backup_files = true;
  defaultScriptingLang = "Python";  //Mantid M. Gigg
  d_thousands_sep = true;
  d_locale = QLocale::system().name();
  if (!d_thousands_sep)
    d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

  d_decimal_digits = 13;

  d_extended_open_dialog = true;
  d_extended_export_dialog = true;
  d_extended_import_ASCII_dialog = true;
  d_extended_plot_dialog = true;

  d_add_curves_dialog_size = QSize(700, 400);
  d_show_current_folder = false;

  confirmCloseFolder = false;
  confirmCloseTable = false;
  confirmCloseMatrix = false;
  confirmClosePlot2D = false;
  confirmClosePlot3D = false;
  confirmCloseNotes = false;
  d_inform_rename_table = false;
  confirmCloseInstrWindow=false;

  d_show_table_comments = false;

  titleOn = true;
  allAxesOn = false;
  canvasFrameWidth = 0;
  defaultPlotMargin = 0;
  drawBackbones = true;

  //these settings are overridden, but the default axes scales are linear
  xaxisScale = "linear";
  yaxisScale = "linear";
  zaxisScale = "linear";

  axesLineWidth = 1;
  autoscale2DPlots = true;
  autoScaleFonts = true;
  autoResizeLayers = true;
  antialiasing2DPlots = false; 	//Mantid
  d_scale_plots_on_print = false;
  d_print_cropmarks = false;

  defaultCurveStyle = int(Graph::LineSymbols);
  defaultCurveLineWidth = 1;
  defaultSymbolSize = 7;

  majTicksStyle = int(ScaleDraw::Out);
  minTicksStyle = int(ScaleDraw::Out);
  minTicksLength = 5;
  majTicksLength = 9;

  legendFrameStyle = int(LegendWidget::Line);
  legendTextColor = Qt::black;
  legendBackground = Qt::white;
  legendBackground.setAlpha(0); // transparent by default;

  defaultArrowLineWidth = 1;
  defaultArrowColor = Qt::black;
  defaultArrowHeadLength = 4;
  defaultArrowHeadAngle = 45;
  defaultArrowHeadFill = true;
  defaultArrowLineStyle = Graph::getPenStyle("SolidLine");

  showPlot3DLegend = true;
  showPlot3DProjection = false;
  smooth3DMesh = false;
  plot3DResolution = 1;
  orthogonal3DPlots = false;
  autoscale3DPlots = true;

  fit_output_precision = 13;
  pasteFitResultsToPlot = false;
  writeFitResultsToLog = true;
  generateUniformFitPoints = true;
  fitPoints = 100;
  generatePeakCurves = true;
  peakCurvesColor = 2;
  fit_scale_errors = true;
  d_2_linear_fit_points = true;

  columnSeparator = "\t";
  ignoredLines = 0;
  renameColumns = true;
  strip_spaces = false;
  simplify_spaces = false;
  d_ASCII_file_filter = "*";
  d_ASCII_import_locale = QLocale::system().name();
  d_import_dec_separators = true;
  d_ASCII_import_mode = int(ImportASCIIDialog::NewTables);
  d_ASCII_comment_string = "#";
  d_ASCII_import_comments = false;
  d_ASCII_import_read_only = false;
  d_ASCII_import_preview = true;
  d_preview_lines = 100;
  d_ASCII_end_line = LF;
  d_eol = LF;
#ifdef Q_OS_MAC
  d_ASCII_end_line = CR;
  d_eol = CR;
#endif

  d_export_col_separator = "\t";
  d_export_col_names = false;
  d_export_col_comment = false;
  d_export_table_selection = false;

  d_image_export_filter = ".png";
  d_export_transparency = false;
  d_export_quality = 100;

  // MG: On Linux, if cups defines a printer queue that cannot be contact, the
  // QPrinter constructor hangs and doesn't timeout.

  //	QPrinterInfo::availablePrinters();


  //	d_export_resolution = QPrinter().resolution();
  d_export_color = true;
  d_export_vector_size = int(QPrinter::Custom);
  d_keep_plot_aspect = true;
}

void ApplicationWindow::initToolBars()
{
  initPlot3DToolBar();

  //	setWindowIcon(QIcon(getQPixmap("logo_xpm")));
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  QPixmap openIcon, saveIcon;

  fileTools = new QToolBar(tr( "File" ), this);
  fileTools->setObjectName("fileTools"); // this is needed for QMainWindow::restoreState()
  fileTools->setIconSize( QSize(18,20) );
  addToolBar( Qt::TopToolBarArea, fileTools );

  fileTools->addAction(actionNewProject);
  fileTools->addAction(actionNewFolder);
  fileTools->addAction(actionNewTable);
  fileTools->addAction(actionNewMatrix);
  fileTools->addAction(actionNewNote);
  fileTools->addAction(actionNewGraph);
  fileTools->addAction(actionNewFunctionPlot);
  fileTools->addAction(actionNewSurfacePlot);
  fileTools->addSeparator ();
  fileTools->addAction(actionManageDirs);
  fileTools->addSeparator ();
  
  fileTools->addAction(actionOpenProj);
  fileTools->addAction(actionOpenRaw);
  fileTools->addAction(actionOpenNexus);
  fileTools->addAction(actionLoadFile);

  fileTools->addAction(actionSaveProject);
  fileTools->addSeparator ();
  fileTools->addAction(actionLoad);
  fileTools->addSeparator ();
  fileTools->addAction(actionCopyWindow);
  fileTools->addAction(actionPrint);
  fileTools->addAction(actionExportPDF);
  fileTools->addSeparator();
  fileTools->addAction(actionShowExplorer);
  fileTools->addAction(actionShowLog);
#ifdef SCRIPTING_PYTHON
  fileTools->addAction(actionShowScriptWindow);
#endif

  editTools = new QToolBar(tr("Edit"), this);
  editTools->setObjectName("editTools"); // this is needed for QMainWindow::restoreState()
  editTools->setIconSize( QSize(18,20) );
  addToolBar( editTools );

  editTools->addAction(actionUndo);
  editTools->addAction(actionRedo);
  editTools->addAction(actionCutSelection);
  editTools->addAction(actionCopySelection);
  editTools->addAction(actionPasteSelection);
  editTools->addAction(actionClearSelection);

  plotTools = new QToolBar(tr("Plot"), this);
  plotTools->setObjectName("plotTools"); // this is needed for QMainWindow::restoreState()
  plotTools->setIconSize( QSize(16,20) );
  addToolBar( plotTools );

  plotTools->addAction(actionAddLayer);
  plotTools->addAction(actionShowLayerDialog);
  plotTools->addAction(actionAutomaticLayout);
  plotTools->addSeparator();
  plotTools->addAction(actionAddErrorBars);
  plotTools->addAction(actionRemoveErrorBars);
  plotTools->addAction(actionShowCurvesDialog);
  plotTools->addAction(actionAddFunctionCurve);
  plotTools->addAction(actionNewLegend);
  plotTools->addSeparator ();
  plotTools->addAction(actionUnzoom);

  dataTools = new QActionGroup( this );
  dataTools->setExclusive( true );

  btnPointer = new QAction(tr("Disable &Tools"), this);
  btnPointer->setActionGroup(dataTools);
  btnPointer->setCheckable( true );
  btnPointer->setIcon(QIcon(getQPixmap("pointer_xpm")) );
  btnPointer->setChecked(true);
  plotTools->addAction(btnPointer);


  actionMagnify->setActionGroup(dataTools);
  actionMagnify->setCheckable( true );
  plotTools->addAction(actionMagnify);

  btnZoomIn = new QAction(tr("&Zoom In"), this);
  btnZoomIn->setShortcut( tr("Ctrl++") );
  btnZoomIn->setActionGroup(dataTools);
  btnZoomIn->setCheckable( true );
  btnZoomIn->setIcon(QIcon(getQPixmap("zoom_xpm")) );
  plotTools->addAction(btnZoomIn);

  btnZoomOut = new QAction(tr("&Zoom Out"), this);
  btnZoomOut->setShortcut( tr("Ctrl+-") );
  btnZoomOut->setActionGroup(dataTools);
  btnZoomOut->setCheckable( true );
  btnZoomOut->setIcon(QIcon(getQPixmap("zoomOut_xpm")) );
  plotTools->addAction(btnZoomOut);


  btnCursor = new QAction(tr("&Data Reader"), this);
  btnCursor->setShortcut( tr("CTRL+D") );
  btnCursor->setActionGroup(dataTools);
  btnCursor->setCheckable( true );
  btnCursor->setIcon(QIcon(getQPixmap("select_xpm")) );
  plotTools->addAction(btnCursor);

  btnSelect = new QAction(tr("&Select Data Range"), this);
  btnSelect->setShortcut( tr("ALT+S") );
  btnSelect->setActionGroup(dataTools);
  btnSelect->setCheckable( true );
  btnSelect->setIcon(QIcon(getQPixmap("cursors_xpm")) );
  plotTools->addAction(btnSelect);

  btnPicker = new QAction(tr("S&creen Reader"), this);
  btnPicker->setActionGroup(dataTools);
  btnPicker->setCheckable( true );
  btnPicker->setIcon(QIcon(getQPixmap("cursor_16_xpm")) );
  plotTools->addAction(btnPicker);

  actionDrawPoints = new QAction(tr("&Draw Data Points"), this);
  actionDrawPoints->setActionGroup(dataTools);
  actionDrawPoints->setCheckable( true );
  actionDrawPoints->setIcon(QIcon(getQPixmap("draw_points_xpm")) );
  plotTools->addAction(actionDrawPoints);

  btnMovePoints = new QAction(tr("&Move Data Points..."), this);
  btnMovePoints->setShortcut( tr("Ctrl+ALT+M") );
  btnMovePoints->setActionGroup(dataTools);
  btnMovePoints->setCheckable( true );
  btnMovePoints->setIcon(QIcon(getQPixmap("hand_xpm")) );
  plotTools->addAction(btnMovePoints);

  btnRemovePoints = new QAction(tr("Remove &Bad Data Points..."), this);
  btnRemovePoints->setShortcut( tr("Alt+B") );
  btnRemovePoints->setActionGroup(dataTools);
  btnRemovePoints->setCheckable( true );
  btnRemovePoints->setIcon(QIcon(getQPixmap("gomme_xpm")));
  plotTools->addAction(btnRemovePoints);

  if (mantidUI->fitFunctionBrowser())
  {
    btnMultiPeakPick = new QAction(tr("Select Multiple Peaks..."), this);
    btnMultiPeakPick->setActionGroup(dataTools);
    btnMultiPeakPick->setCheckable( true );
    btnMultiPeakPick->setIcon(QIcon(getQPixmap("Fit_xpm")));
    plotTools->addAction(btnMultiPeakPick);
  }
  else
  {
    btnMultiPeakPick = NULL;
  }

  connect( dataTools, SIGNAL( triggered( QAction* ) ), this, SLOT( pickDataTool( QAction* ) ) );
  plotTools->addSeparator ();

  actionAddText = new QAction(tr("Add &Text"), this);
  actionAddText->setShortcut( tr("ALT+T") );
  actionAddText->setIcon(QIcon(getQPixmap("text_xpm")));
  actionAddText->setCheckable(true);
  connect(actionAddText, SIGNAL(activated()), this, SLOT(addText()));
  plotTools->addAction(actionAddText);

  btnArrow = new QAction(tr("Draw &Arrow"), this);
  btnArrow->setShortcut( tr("CTRL+ALT+A") );
  btnArrow->setActionGroup(dataTools);
  btnArrow->setCheckable( true );
  btnArrow->setIcon(QIcon(getQPixmap("arrow_xpm")) );
  plotTools->addAction(btnArrow);

  btnLine = new QAction(tr("Draw &Line"), this);
  btnLine->setShortcut( tr("CTRL+ALT+L") );
  btnLine->setActionGroup(dataTools);
  btnLine->setCheckable( true );
  btnLine->setIcon(QIcon(getQPixmap("lPlot_xpm")) );
  plotTools->addAction(btnLine);

  plotTools->addAction(actionTimeStamp);
  plotTools->addAction(actionAddImage);
  plotTools->hide();

  tableTools = new QToolBar(tr("Table"), this);
  tableTools->setObjectName("tableTools"); // this is needed for QMainWindow::restoreState()
  tableTools->setIconSize( QSize(16, 20));
  addToolBar(Qt::TopToolBarArea, tableTools);

  tableTools->addAction(actionPlotL);
  tableTools->addAction(actionPlotP);
  tableTools->addAction(actionPlotLP);
  tableTools->addAction(actionPlotVerticalBars);
  tableTools->addAction(actionPlotHorizontalBars);
  tableTools->addAction(actionPlotArea);
  tableTools->addAction(actionPlotPie);
  tableTools->addAction(actionPlotHistogram);
  tableTools->addAction(actionBoxPlot);
  tableTools->addAction(actionPlotVectXYXY);
  tableTools->addAction(actionPlotVectXYAM);
  tableTools->addSeparator ();
  tableTools->addAction(actionPlot3DRibbon);
  tableTools->addAction(actionPlot3DBars);
  tableTools->addAction(actionPlot3DScatter);
  tableTools->addAction(actionPlot3DTrajectory);
  tableTools->addSeparator();
  tableTools->addAction(actionAddColToTable);
  tableTools->addAction(actionShowColStatistics);
  tableTools->addAction(actionShowRowStatistics);
  tableTools->setEnabled(false);
  tableTools->hide();

  columnTools = new QToolBar(tr( "Column"), this);
  columnTools->setObjectName("columnTools"); // this is needed for QMainWindow::restoreState()
  columnTools->setIconSize(QSize(16, 20));
  addToolBar(Qt::TopToolBarArea, columnTools);

  columnTools->addAction(actionShowColumnValuesDialog);
  columnTools->addAction(actionSetAscValues);
  columnTools->addAction(actionSetRandomValues);
  columnTools->addSeparator();
  columnTools->addAction(actionSetXCol);
  columnTools->addAction(actionSetYCol);
  columnTools->addAction(actionSetZCol);
  columnTools->addAction(actionSetYErrCol);
  columnTools->addAction(actionSetLabelCol);
  columnTools->addAction(actionDisregardCol);
  columnTools->addSeparator();
  columnTools->addAction(actionMoveColFirst);
  columnTools->addAction(actionMoveColLeft);
  columnTools->addAction(actionMoveColRight);
  columnTools->addAction(actionMoveColLast);
  columnTools->addAction(actionSwapColumns);
  columnTools->setEnabled(false);
  columnTools->hide();

  displayBar = new QToolBar( tr( "Data Display" ), this );
  displayBar->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
  displayBar->setObjectName("displayBar"); // this is needed for QMainWindow::restoreState()
  info = new QLineEdit( this );
  displayBar->addWidget( info );
  info->setReadOnly(true);
  QPalette palette;
  palette.setColor(QPalette::Text, QColor(Qt::green));
  palette.setColor(QPalette::HighlightedText, QColor(Qt::darkGreen));
  palette.setColor(QPalette::Base, QColor(Qt::black));
  info->setPalette(palette);

  addToolBar( Qt::TopToolBarArea, displayBar );
  displayBar->hide();

  insertToolBarBreak (displayBar);

  plotMatrixBar = new QToolBar( tr( "Matrix Plot" ), this);
  plotMatrixBar->setObjectName("plotMatrixBar");
  addToolBar(Qt::BottomToolBarArea, plotMatrixBar);

  actionPlot3DWireFrame->addTo(plotMatrixBar);
  actionPlot3DHiddenLine->addTo(plotMatrixBar);

  actionPlot3DPolygons->addTo(plotMatrixBar);
  actionPlot3DWireSurface->addTo(plotMatrixBar);

  plotMatrixBar->addSeparator();

  actionPlot3DBars->addTo(plotMatrixBar);
  actionPlot3DScatter->addTo(plotMatrixBar);

  plotMatrixBar->addSeparator();
  actionColorMap->addTo(plotMatrixBar);
  actionContourMap->addTo(plotMatrixBar);
  actionGrayMap->addTo(plotMatrixBar);
  actionImagePlot->addTo(plotMatrixBar);
  //actionPlotHistogram->addTo(plotMatrixBar);
  plotMatrixBar->addSeparator();
  actionSetMatrixValues->addTo(plotMatrixBar);
  actionFlipMatrixHorizontally->addTo(plotMatrixBar);
  actionFlipMatrixVertically->addTo(plotMatrixBar);
  actionRotateMatrix->addTo(plotMatrixBar);
  actionRotateMatrixMinus->addTo(plotMatrixBar);
  plotMatrixBar->hide();

  formatToolBar = new QToolBar(tr( "Format" ), this);
  formatToolBar->setObjectName("formatToolBar");
  addToolBar(Qt::TopToolBarArea, formatToolBar);

  QFontComboBox *fb = new QFontComboBox();
  connect(fb, SIGNAL(currentFontChanged(const QFont &)), this, SLOT(setFontFamily(const QFont &)));
  actionFontBox = formatToolBar->addWidget(fb);

  QSpinBox *sb = new QSpinBox();
  connect(sb, SIGNAL(valueChanged(int)), this, SLOT(setFontSize(int)));
  actionFontSize = formatToolBar->addWidget(sb);

  actionFontBold->addTo(formatToolBar);
  actionFontItalic->addTo(formatToolBar);

  actionUnderline->addTo(formatToolBar);
  actionSuperscript->addTo(formatToolBar);
  actionSubscript->addTo(formatToolBar);
  actionGreekSymbol->addTo(formatToolBar);
  actionGreekMajSymbol->addTo(formatToolBar);
  actionMathSymbol->addTo(formatToolBar);

  formatToolBar->setEnabled(false);
  formatToolBar->hide();



}

void ApplicationWindow::insertTranslatedStrings()
{
  if (projectname == "untitled")
    setWindowTitle(tr("MantidPlot - untitled"));//Mantid

  lv->setColumnText (0, tr("Name"));
  lv->setColumnText (1, tr("Type"));
  lv->setColumnText (2, tr("View"));
  lv->setColumnText (3, tr("Size"));
  lv->setColumnText (4, tr("Created"));
  lv->setColumnText (5, tr("Label"));

  explorerWindow->setWindowTitle(tr("Project Explorer"));
  logWindow->setWindowTitle(tr("Results Log"));
  undoStackWindow->setWindowTitle(tr("Undo Stack"));
  //#ifdef SCRIPTING_CONSOLE
  consoleWindow->setWindowTitle(tr("Scripting Console"));
  //#endif
  displayBar->setWindowTitle(tr("Data Display"));
  tableTools->setWindowTitle(tr("Table"));
  columnTools->setWindowTitle(tr("Column"));
  plotTools->setWindowTitle(tr("Plot"));
  fileTools->setWindowTitle(tr("File"));
  editTools->setWindowTitle(tr("Edit"));
  plotMatrixBar->setWindowTitle(tr("Matrix Plot"));
  plot3DTools->setWindowTitle(tr("3D Surface"));
  formatToolBar->setWindowTitle(tr("Format"));

  fileMenu->changeItem(recentMenuID, tr("&Recent Projects"));

  translateActionsStrings();
  customMenu(activeWindow());
}

void ApplicationWindow::initMainMenu()
{
  fileMenu = new QMenu(this);
  fileMenu->setObjectName("fileMenu");
  connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));

  recent = new QMenu(this);
  newMenu = new QMenu(this);
  newMenu->setObjectName("newMenu");
  exportPlotMenu = new QMenu(this);
  exportPlotMenu->setObjectName("exportPlotMenu");

  edit = new QMenu(this);
  edit->setObjectName("editMenu");

  edit->addAction(actionUndo);
  edit->addAction(actionRedo);
  edit->insertSeparator();
  edit->addAction(actionCopySelection);
  edit->addAction(actionPasteSelection);
  edit->addAction(actionClearSelection);
  edit->insertSeparator();
  edit->addAction(actionDeleteFitTables);

  connect(edit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

  view = new QMenu(this);
  view->setObjectName("viewMenu");

  view->setCheckable(true);

  //view->addAction(actionShowPlotWizard);
  view->addAction(actionShowExplorer);
  view->addAction(actionShowLog);
  //view->addAction(actionShowUndoStack);
  //#ifdef SCRIPTING_CONSOLE
  view->addAction(actionShowConsole);
  //#endif

  graph = new QMenu(this);
  graph->setObjectName("graphMenu");
  graph->setCheckable(true);
  graph->addAction(actionAddErrorBars);
  graph->addAction(actionRemoveErrorBars);
  graph->addAction(actionShowCurvesDialog);
  graph->addAction(actionAddFunctionCurve);
  graph->addAction(actionNewLegend);
  graph->insertSeparator();
  graph->addAction(actionAddText);
  graph->addAction(btnArrow);
  graph->addAction(btnLine);
  graph->addAction(actionTimeStamp);
  graph->addAction(actionAddImage);
  graph->insertSeparator();//layers section
  graph->addAction(actionAddLayer);
  graph->addAction(actionDeleteLayer);
  graph->addAction(actionShowLayerDialog);

  plot3DMenu = new QMenu(this);
  plot3DMenu->setObjectName("plot3DMenu");
  plot3DMenu->addAction(actionPlot3DWireFrame);
  plot3DMenu->addAction(actionPlot3DHiddenLine);
  plot3DMenu->addAction(actionPlot3DPolygons);
  plot3DMenu->addAction(actionPlot3DWireSurface);
  plot3DMenu->insertSeparator();
  plot3DMenu->addAction(actionPlot3DBars);
  plot3DMenu->addAction(actionPlot3DScatter);
  plot3DMenu->insertSeparator();
  plot3DMenu->addAction(actionImagePlot);
  plot3DMenu->addAction(actionColorMap);
  plot3DMenu->addAction(actionNoContourColorMap);
  plot3DMenu->addAction(actionContourMap);
  plot3DMenu->addAction(actionGrayMap);
  plot3DMenu->insertSeparator();
  //plot3DMenu->addAction(actionPlotHistogram);

  matrixMenu = new QMenu(this);
  matrixMenu->setObjectName("matrixMenu");
  connect(matrixMenu, SIGNAL(aboutToShow()), this, SLOT(matrixMenuAboutToShow()));

  plot2DMenu = new QMenu(this);
  plot2DMenu->setObjectName("plot2DMenu");
  connect(plot2DMenu, SIGNAL(aboutToShow()), this, SLOT(plotMenuAboutToShow()));

  plotDataMenu = new QMenu(this);
  plotDataMenu->setObjectName("plotDataMenu");
  plotDataMenu->setCheckable(true);
  connect(plotDataMenu, SIGNAL(aboutToShow()), this, SLOT(plotDataMenuAboutToShow()));

  normMenu = new QMenu(this);
  normMenu->setObjectName("normMenu");

  fillMenu = new QMenu(this);
  fillMenu->setObjectName("fillMenu");

  tableMenu = new QMenu(this);
  tableMenu->setObjectName("tableMenu");
  connect(tableMenu, SIGNAL(aboutToShow()), this, SLOT(tableMenuAboutToShow()));

  smoothMenu = new QMenu(this);
  smoothMenu->setObjectName("smoothMenu");

  filterMenu = new QMenu(this);
  filterMenu->setObjectName("filterMenu");

  decayMenu = new QMenu(this);
  decayMenu->setObjectName("decayMenu");

  multiPeakMenu = new QMenu(this);
  multiPeakMenu->setObjectName("multiPeakMenu");

  analysisMenu = new QMenu(this);
  analysisMenu->setObjectName("analysisMenu");
  connect(analysisMenu, SIGNAL(aboutToShow()), this, SLOT(analysisMenuAboutToShow()));

  format = new QMenu(this);
  format->setObjectName("formatMenu");

  windowsMenu = new QMenu(this);
  windowsMenu->setObjectName("windowsMenu");
  windowsMenu->setCheckable(true);
  connect(windowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));

  foldersMenu = new QMenu(this);
  foldersMenu->setCheckable(true);

  help = new QMenu(this);
  help->setObjectName("helpMenu");

  help->addAction(actionHomePage);
  help->addAction(actionMantidConcepts);
  help->addAction(actionMantidAlgorithms);
  help->addAction(actionmantidplotHelp);
  help->insertSeparator();
  help->addAction(actionHelpBugReports);
  help->insertSeparator();
  help->addAction(actionAbout);

  icat = new QMenu(this);
  icat->setObjectName("CatalogMenu");
  icat->addAction(actionICatLogin);//Login menu item
  icat->addAction(actionMydataSearch);// my data search menu item
  icat->addAction(actionICatSearch);//search menu item
  icat->addAction(actionAdvancedSearch); //advanced search menu item
  icat->addAction(actionICatLogout);//logout menu item
  disableActions();
}

void ApplicationWindow::tableMenuAboutToShow()
{
  tableMenu->clear();
  fillMenu->clear();

  QMenu *setAsMenu = tableMenu->addMenu(tr("Set Columns &As"));
  setAsMenu->addAction(actionSetXCol);
  setAsMenu->addAction(actionSetYCol);
  setAsMenu->addAction(actionSetZCol);
  setAsMenu->insertSeparator();
  setAsMenu->addAction(actionSetLabelCol);
  setAsMenu->addAction(actionDisregardCol);
  setAsMenu->insertSeparator();
  setAsMenu->addAction(actionSetXErrCol);
  setAsMenu->addAction(actionSetYErrCol);
  setAsMenu->insertSeparator();
  setAsMenu->addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
  setAsMenu->addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));

  tableMenu->addAction(actionShowColumnOptionsDialog);
  tableMenu->insertSeparator();

  tableMenu->addAction(actionShowColumnValuesDialog);
  tableMenu->addAction(actionTableRecalculate);

  fillMenu = tableMenu->addMenu (tr("&Fill Columns With"));
  fillMenu->addAction(actionSetAscValues);
  fillMenu->addAction(actionSetRandomValues);

  tableMenu->addAction(actionClearTable);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionAddColToTable);
  tableMenu->addAction(actionShowColsDialog);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionHideSelectedColumns);
  tableMenu->addAction(actionShowAllColumns);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionMoveColFirst);
  tableMenu->addAction(actionMoveColLeft);
  tableMenu->addAction(actionMoveColRight);
  tableMenu->addAction(actionMoveColLast);
  tableMenu->addAction(actionSwapColumns);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionShowRowsDialog);
  tableMenu->addAction(actionDeleteRows);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionGoToRow);
  tableMenu->addAction(actionGoToColumn);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionConvertTable);

  tableMenu->insertSeparator();
  tableMenu->addAction(actionShowPlotWizard);

  reloadCustomActions();
}

void ApplicationWindow::plotDataMenuAboutToShow()
{
  plotDataMenu->clear();
  plotDataMenu->addAction(btnPointer);
  plotDataMenu->addAction(btnZoomIn);
  plotDataMenu->addAction(btnZoomOut);
  plotDataMenu->addAction(actionMagnify);
  plotDataMenu->addAction(actionUnzoom);
  plotDataMenu->insertSeparator();
  plotDataMenu->addAction(btnCursor);
  plotDataMenu->addAction(btnSelect);
  plotDataMenu->addAction(btnPicker);
  plotDataMenu->insertSeparator();
  plotDataMenu->addAction(actionDrawPoints);
  plotDataMenu->addAction(btnMovePoints);
  plotDataMenu->addAction(btnRemovePoints);


  reloadCustomActions();
}

void ApplicationWindow::plotMenuAboutToShow()
{
  plot2DMenu->clear();

  plot2DMenu->addAction(actionPlotL);
  plot2DMenu->addAction(actionPlotP);
  plot2DMenu->addAction(actionPlotLP);

  QMenu *specialPlotMenu = plot2DMenu->addMenu (tr("Special Line/Symb&ol"));
  specialPlotMenu->addAction(actionPlotVerticalDropLines);
  specialPlotMenu->addAction(actionPlotSpline);
  specialPlotMenu->addAction(actionPlotVertSteps);
  specialPlotMenu->addAction(actionPlotHorSteps);
  plot2DMenu->insertSeparator();
  plot2DMenu->addAction(actionPlotVerticalBars);
  plot2DMenu->addAction(actionPlotHorizontalBars);
  plot2DMenu->addAction(actionPlotArea);
  plot2DMenu->addAction(actionPlotPie);
  plot2DMenu->addAction(actionPlotVectXYXY);
  plot2DMenu->addAction(actionPlotVectXYAM);
  plot2DMenu->insertSeparator();

  QMenu *statMenu = plot2DMenu->addMenu (tr("Statistical &Graphs"));
  statMenu->addAction(actionBoxPlot);
  statMenu->addAction(actionPlotHistogram);
  statMenu->addAction(actionPlotStackedHistograms);

  QMenu *panelsMenu = plot2DMenu->addMenu (tr("Pa&nel"));
  panelsMenu->addAction(actionPlot2VerticalLayers);
  panelsMenu->addAction(actionPlot2HorizontalLayers);
  panelsMenu->addAction(actionPlot4Layers);
  panelsMenu->addAction(actionPlotStackedLayers);

  QMenu *plot3D = plot2DMenu->addMenu (tr("3&D Plot"));
  plot3D->addAction(actionPlot3DRibbon);
  plot3D->addAction(actionPlot3DBars);
  plot3D->addAction(actionPlot3DScatter);
  plot3D->addAction(actionPlot3DTrajectory);

  reloadCustomActions();
}

void ApplicationWindow::customMenu(QMdiSubWindow* w)
{
  menuBar()->clear();
  menuBar()->insertItem(tr("&File"), fileMenu);
  fileMenuAboutToShow();
  menuBar()->insertItem(tr("&Edit"), edit);
  editMenuAboutToShow();
  menuBar()->insertItem(tr("&View"), view);


  view->insertSeparator();
  view->addAction(actionShowScriptWindow);//Mantid
  view->addAction(actionShowScriptInterpreter);
  view->insertSeparator();

  mantidUI->addMenuItems(view);

  view->insertSeparator();
  view->addAction(actionToolBars);
  view->addAction(actionShowConfigureDialog);
  view->insertSeparator();
  view->addAction(actionCustomActionDialog);

  //#ifdef SCRIPTING_DIALOG
  //	scriptingMenu->addAction(actionScriptingLang);
  //#endif
  // these use the same keyboard shortcut (Ctrl+Return) and should not be enabled at the same time
  actionNoteEvaluate->setEnabled(false);
  actionTableRecalculate->setEnabled(false);

  // clear undo stack view (in case window is not a matrix)
  d_undo_view->setStack(0);
  actionUndo->setEnabled(false);
  actionRedo->setEnabled(false);

  if(w){
    actionPrintAllPlots->setEnabled(projectHas2DPlots());
    actionPrint->setEnabled(true);
    actionCutSelection->setEnabled(true);
    actionCopySelection->setEnabled(true);
    actionPasteSelection->setEnabled(true);
    actionClearSelection->setEnabled(true);
    actionSaveTemplate->setEnabled(true);
    QStringList tables = tableNames() + matrixNames();
    if (!tables.isEmpty())
      actionShowExportASCIIDialog->setEnabled(true);
    else
      actionShowExportASCIIDialog->setEnabled(false);

    if (w->isA("MultiLayer")) {
      menuBar()->insertItem(tr("&Graph"), graph);
      menuBar()->insertItem(tr("&Data"), plotDataMenu);
      plotDataMenuAboutToShow();
      //menuBar()->insertItem(tr("&Analysis"), analysisMenu);
      //analysisMenuAboutToShow();
      menuBar()->insertItem(tr("For&mat"), format);

      format->clear();
      format->addAction(actionShowPlotDialog);
      format->insertSeparator();
      format->addAction(actionShowScaleDialog);
      format->addAction(actionShowAxisDialog);
      actionShowAxisDialog->setEnabled(true);
      format->insertSeparator();
      format->addAction(actionShowGridDialog);
      format->addAction(actionShowTitleDialog);
    } else if (w->isA("Graph3D")) {
      disableActions();

      menuBar()->insertItem(tr("For&mat"), format);

      actionPrint->setEnabled(true);
      actionSaveTemplate->setEnabled(true);

      format->clear();
      format->addAction(actionShowPlotDialog);
      format->addAction(actionShowScaleDialog);
      format->addAction(actionShowAxisDialog);
      format->addAction(actionShowTitleDialog);
      if (((Graph3D*)w)->coordStyle() == Qwt3D::NOCOORD)
        actionShowAxisDialog->setEnabled(false);
    } else if (w->inherits("Table")) {
      menuBar()->insertItem(tr("&Plot"), plot2DMenu);
      menuBar()->insertItem(tr("&Analysis"), analysisMenu);
      analysisMenuAboutToShow();
      menuBar()->insertItem(tr("&Table"), tableMenu);
      tableMenuAboutToShow();
      actionTableRecalculate->setEnabled(true);

    } else if (w->isA("Matrix")){
      actionTableRecalculate->setEnabled(true);
      menuBar()->insertItem(tr("3D &Plot"), plot3DMenu);
      menuBar()->insertItem(tr("&Matrix"), matrixMenu);
      matrixMenuAboutToShow();
      menuBar()->insertItem(tr("&Analysis"), analysisMenu);
      analysisMenuAboutToShow();
      d_undo_view->setEmptyLabel(w->objectName() + ": " + tr("Empty Stack"));
      QUndoStack *stack = ((Matrix *)w)->undoStack();
      d_undo_view->setStack(stack);

    } else if (w->isA("Note")) {
      actionSaveTemplate->setEnabled(false);
      actionNoteEvaluate->setEnabled(true);

      actionNoteExecute->disconnect(SIGNAL(activated()));
      actionNoteExecuteAll->disconnect(SIGNAL(activated()));
      actionNoteEvaluate->disconnect(SIGNAL(activated()));
      connect(actionNoteExecute, SIGNAL(activated()), w, SLOT(execute()));
      connect(actionNoteExecuteAll, SIGNAL(activated()), w, SLOT(executeAll()));
      connect(actionNoteEvaluate, SIGNAL(activated()), w, SLOT(evaluate()));
    } else if (!mantidUI->menuAboutToShow(w))
      disableActions();
  } else
    disableActions();

  menuBar()->insertItem(tr("&Windows"), windowsMenu);
  windowsMenuAboutToShow();
  // -- Mantid: add script actions, if any exist --
  QListIterator<QMenu*> mIter(d_user_menus);
  while( mIter.hasNext() )
  {
    QMenu* item = mIter.next();
    menuBar()->insertItem(tr(item->title()), item);

  }

  menuBar()->insertItem(tr("&Catalog"),icat);

  // Interface menu. Build the interface from the user sub windows list.
  // Modifications will be done through the ManageCustomMenus dialog and
  // remembered through QSettings.
  QStringList user_windows = MantidQt::API::InterfaceManager::Instance().getUserSubWindowKeys();
  QStringListIterator itr(user_windows);
  QString menuName = "&Interfaces";
  addUserMenu(menuName);
  while( itr.hasNext() )
  {
      QString itemName = itr.next();
      // Check whether the menu item was flagged in the QSettings.
      if (getMenuSettingsFlag(itemName))
          addUserMenuAction( menuName, itemName, itemName);
  }

  // Go through PyQt interfaces
  QString scriptsDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("mantidqt.python_interfaces_directory"));
  QStringListIterator pyqt_itr(pyqt_interfaces);
  while( pyqt_itr.hasNext() )
  {
    QString itemName = pyqt_itr.next();
    QString scriptPath = scriptsDir + '/' + itemName;

    if( QFileInfo(scriptPath).exists() ) {
      QString baseName = QFileInfo(scriptPath).baseName();
      if (getMenuSettingsFlag(itemName))
        addUserMenuAction(menuName, baseName, scriptPath);
    }
    else
    {
      Mantid::Kernel::Logger& g_log = Mantid::Kernel::Logger::get("ConfigService");
      g_log.warning() << "Could not find interface script: " << scriptPath.ascii() << "\n";
    }
  }

  menuBar()->insertItem(tr("&Help"), help );

  reloadCustomActions();
}

/**
 * Returns whether a custom interface should be added to the Interfaces menu.
 * @param menu_item: name of the custom interface
 */
bool ApplicationWindow::getMenuSettingsFlag(const QString & menu_item)
{
  // Look for the interface in the user menu list
  // If we found the item in the user menu list, return true
  QMenu *menu = NULL;
  foreach(menu, d_user_menus)
  {
    if( menu->title() == menu_item ) return true;
  }

  // If we didn't find it, check whether is was manually removed
  if( removed_interfaces.contains(menu_item) ) return false;
  return true;
}

void ApplicationWindow::disableActions()
{
  actionSaveTemplate->setEnabled(false);
  actionPrintAllPlots->setEnabled(false);
  actionPrint->setEnabled(false);

  actionCutSelection->setEnabled(false);
  actionCopySelection->setEnabled(false);
  actionPasteSelection->setEnabled(false);
  actionClearSelection->setEnabled(false);
}

void ApplicationWindow::customColumnActions()
{
  actionMoveColFirst->setEnabled(false);
  actionMoveColLeft->setEnabled(false);
  actionMoveColRight->setEnabled(false);
  actionMoveColLast->setEnabled(false);
  actionSetXCol->setEnabled(false);
  actionSetYCol->setEnabled(false);
  actionSetZCol->setEnabled(false);
  actionSetLabelCol->setEnabled(false);
  actionSetYErrCol->setEnabled(false);
  actionDisregardCol->setEnabled(false);
  actionSwapColumns->setEnabled(false);
  actionSetAscValues->setEnabled(false);
  actionSetRandomValues->setEnabled(false);

  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  int selectedCols = t->selectedColsNumber();
  if (selectedCols == 1){
    int col = t->selectedColumn();
    if (col > 0){
      actionMoveColFirst->setEnabled(true);
      actionMoveColLeft->setEnabled(true);
    }

    if (col < t->numCols() - 1){
      actionMoveColRight->setEnabled(true);
      actionMoveColLast->setEnabled(true);
    }
  }

  if (selectedCols >= 1){
    actionSetAscValues->setEnabled(true);
    actionSetRandomValues->setEnabled(true);
    actionSetXCol->setEnabled(true);
    actionSetYCol->setEnabled(true);
    actionSetZCol->setEnabled(true);
    actionSetYErrCol->setEnabled(true);
    actionSetLabelCol->setEnabled(true);
    actionDisregardCol->setEnabled(true);
  }

  if (selectedCols == 2)
    actionSwapColumns->setEnabled(true);
}

void ApplicationWindow::customToolBars(QMdiSubWindow* w)
{
  disableToolbars();
  if (!w)
    return;

  if (w->isA("MultiLayer") && d_plot_tool_bar){
    if(!plotTools->isVisible())
      plotTools->show();
    plotTools->setEnabled (true);
    customMultilayerToolButtons((MultiLayer*)w);
    if(d_format_tool_bar && !formatToolBar->isVisible()){
      formatToolBar->setEnabled (true);
      formatToolBar->show();
    }
  } else if (w->inherits("Table")){
    if(d_table_tool_bar){
      if(!tableTools->isVisible())
        tableTools->show();
      tableTools->setEnabled (true);
    }
    if (d_column_tool_bar){
      if(!columnTools->isVisible())
        columnTools->show();
      columnTools->setEnabled (true);
      customColumnActions();
    }
  } else if (w->isA("Matrix") && d_matrix_tool_bar){
    if(!plotMatrixBar->isVisible())
      plotMatrixBar->show();
    plotMatrixBar->setEnabled (true);
  } else if (w->isA("Graph3D") && d_plot3D_tool_bar){
    if(!plot3DTools->isVisible())
      plot3DTools->show();

    if (((Graph3D*)w)->plotStyle() == Qwt3D::NOPLOT)
      plot3DTools->setEnabled(false);
    else
      plot3DTools->setEnabled(true);
    custom3DActions(w);
  }
}

void ApplicationWindow::disableToolbars()
{
  plotTools->setEnabled(false);
  tableTools->setEnabled(false);
  columnTools->setEnabled(false);
  plot3DTools->setEnabled(false);
  plotMatrixBar->setEnabled(false);
}

void ApplicationWindow::plot3DRibbon()
{
  MdiSubWindow *w = activeWindow(TableWindow);
  if (!w)
    return;

  Table *table = static_cast<Table*>(w);
  if(table->selectedColumns().count() == 1){
    if (!validFor3DPlot(table))
      return;
    plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Ribbon);
  } else
    QMessageBox::warning(this, tr("MantidPLot - Plot error"), tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plot3DWireframe()
{
  plot3DMatrix (0, Qwt3D::WIREFRAME);
}

void ApplicationWindow::plot3DHiddenLine()
{
  plot3DMatrix (0, Qwt3D::HIDDENLINE);
}

void ApplicationWindow::plot3DPolygons()
{
  plot3DMatrix (0, Qwt3D::FILLED);
}

void ApplicationWindow::plot3DWireSurface()
{
  plot3DMatrix (0, Qwt3D::FILLEDMESH);
}

void ApplicationWindow::plot3DBars()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table")){
    Table *table = static_cast<Table *>(w);
    if (!validFor3DPlot(table))
      return;

    if(table->selectedColumns().count() == 1)
      plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Bars);
    else
      QMessageBox::warning(this, tr("MantidPlot - Plot error"),tr("You must select exactly one column for plotting!"));//Mantid
  }
  else if(w->inherits("Matrix"))
    plot3DMatrix(0, Qwt3D::USER);
}

void ApplicationWindow::plot3DScatter()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table"))
  {
    Table *table = static_cast<Table *>(w);
    if (!validFor3DPlot(table))
      return;

    if(table->selectedColumns().count() == 1)
      plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Scatter);
    else
      QMessageBox::warning(this, tr("MantidPlot - Plot error"),tr("You must select exactly one column for plotting!"));//Mantid
  }
  else if(w->inherits("Matrix"))
    plot3DMatrix (0, Qwt3D::POINTS);
}

void ApplicationWindow::plot3DTrajectory()
{
  Table *table = (Table *)activeWindow(TableWindow);
  if (!table)
    return;
  if (!validFor3DPlot(table))
    return;

  if(table->selectedColumns().count() == 1)
    plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Trajectory);
  else
    QMessageBox::warning(this, tr("MantidPlot - Plot error"), tr("You must select exactly one column for plotting!"));//Mantid
}

void ApplicationWindow::plotBoxDiagram()
{
  generate2DGraph(Graph::Box);
}

void ApplicationWindow::plotVerticalBars()
{
  generate2DGraph(Graph::VerticalBars);
}

void ApplicationWindow::plotHorizontalBars()
{
  generate2DGraph(Graph::HorizontalBars);
}

MultiLayer* ApplicationWindow::plotHistogram()
{
  return generate2DGraph(Graph::Histogram);
}

MultiLayer* ApplicationWindow::plotHistogram(Matrix *m)
{
  if (!m) {
    m = (Matrix*)activeWindow(MatrixWindow);
    if (!m)
      return 0;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer* g = new MultiLayer(this);
  initMultilayerPlot(g, generateUniqueName(tr("Graph")));

  Graph* plot = g->activeGraph();
  setPreferences(plot);
  plot->addHistogram(m);

  QApplication::restoreOverrideCursor();
  return g;
}

void ApplicationWindow::plotArea()
{
  generate2DGraph(Graph::Area);
}

void ApplicationWindow::plotPie()
{
  Table *table = (Table *)activeWindow(TableWindow);
  if (!table)
    return;

  if(table->selectedColumns().count() != 1){
    QMessageBox::warning(this, tr("MantidPlot - Plot error"),//Mantid
        tr("You must select exactly one column for plotting!"));
    return;
  }

  QStringList s = table->selectedColumns();
  if (s.count()>0){
    Q3TableSelection sel = table->getSelection();
    multilayerPlot(table, s, Graph::Pie, sel.topRow(), sel.bottomRow());
  } else
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select a column to plot!"));//Mantid
}

void ApplicationWindow::plotL()
{
  generate2DGraph(Graph::Line);
}

void ApplicationWindow::plotP()
{
  generate2DGraph(Graph::Scatter);
}

void ApplicationWindow::plotLP()
{
  generate2DGraph(Graph::LineSymbols);
}

void ApplicationWindow::plotVerticalDropLines()
{
  generate2DGraph(Graph::VerticalDropLines);
}

void ApplicationWindow::plotSpline()
{
  generate2DGraph(Graph::Spline);
}

void ApplicationWindow::plotVertSteps()
{
  generate2DGraph(Graph::VerticalSteps);
}

void ApplicationWindow::plotHorSteps()
{
  generate2DGraph(Graph::HorizontalSteps);
}

void ApplicationWindow::plotVectXYXY()
{
  Table *table = (Table *)activeWindow(TableWindow);
  if (!table)
    return;
  if (!validFor2DPlot(table))
    return;

  QStringList s = table->selectedColumns();
  if (s.count() == 4) {
    Q3TableSelection sel = table->getSelection();
    multilayerPlot(table, s, Graph::VectXYXY, sel.topRow(), sel.bottomRow());
  } else
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select four columns for this operation!"));//Mantid
}

void ApplicationWindow::plotVectXYAM()
{
  Table *table = (Table *)activeWindow(TableWindow);
  if (!table)
    return;
  if (!validFor2DPlot(table))
    return;

  QStringList s = table->selectedColumns();
  if (s.count() == 4){
    Q3TableSelection sel = table->getSelection();
    multilayerPlot(table, s, Graph::VectXYAM, sel.topRow(), sel.bottomRow());
  } else
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select four columns for this operation!"));//Mantid
}

void ApplicationWindow::renameListViewItem(const QString& oldName,const QString& newName)
{
  Q3ListViewItem *it=lv->findItem (oldName,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(0,newName);
}

void ApplicationWindow::setListViewLabel(const QString& caption,const QString& label)
{
  Q3ListViewItem *it=lv->findItem ( caption, 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(5,label);
}

void ApplicationWindow::setListViewDate(const QString& caption,const QString& date)
{
  Q3ListViewItem *it=lv->findItem ( caption, 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(4,date);
}

void ApplicationWindow::setListView(const QString& caption,const QString& view)
{
  Q3ListViewItem *it=lv->findItem ( caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(2,view);
}

void ApplicationWindow::setListViewSize(const QString& caption,const QString& size)
{
  Q3ListViewItem *it=lv->findItem ( caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(3,size);
}

QString ApplicationWindow::listViewDate(const QString& caption)
{
  Q3ListViewItem *it=lv->findItem (caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    return it->text(4);
  else
    return "";
}

void ApplicationWindow::updateTableNames(const QString& oldName, const QString& newName)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (w->isA("MultiLayer")) {
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurveNames(oldName, newName);
    } else if (w->isA("Graph3D")) {
      QString name = ((Graph3D*)w)->formula();
      if (name.contains(oldName, true)) {
        name.replace(oldName,newName);
        ((Graph3D*)w)->setPlotAssociation(name);
      }
    }
  }
}

void ApplicationWindow::updateColNames(const QString& oldName, const QString& newName)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurveNames(oldName, newName, false);
    }
    else if (w->isA("Graph3D")){
      QString name = ((Graph3D*)w)->formula();
      if (name.contains(oldName)){
        name.replace(oldName,newName);
        ((Graph3D*)w)->setPlotAssociation(name);
      }
    }
  }
}

void ApplicationWindow::changeMatrixName(const QString& oldName, const QString& newName)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D"))
    {
      QString s = ((Graph3D*)w)->formula();
      if (s.contains(oldName))
      {
        s.replace(oldName, newName);
        ((Graph3D*)w)->setPlotAssociation(s);
      }
    }
    else if (w->isA("MultiLayer"))
    {
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          QwtPlotItem *sp = (QwtPlotItem *)g->plotItem(i);
          if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->title().text() == oldName)
            sp->setTitle(newName);
        }
      }
    }
  }
}

void ApplicationWindow::remove3DMatrixPlots(Matrix *m)
{
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D") && ((Graph3D*)w)->matrix() == m)
      ((Graph3D*)w)->clearData();
    else if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          if (g->curveType(i) == Graph::Histogram){
            QwtHistogram *h = (QwtHistogram *)g->plotItem(i);
            if (h && h->matrix() == m)
              g->removeCurve(i);
          } else {
            Spectrogram *sp = (Spectrogram *)g->plotItem(i);
            if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->matrix() == m)
              g->removeCurve(i);
          }
        }
      }
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateMatrixPlots(MdiSubWindow *window)
{
  Matrix *m = (Matrix *)window;
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D") && ((Graph3D*)w)->matrix() == m)
      ((Graph3D*)w)->updateMatrixData(m);
    else if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          if (g->curveType(i) == Graph::Histogram){
            QwtHistogram *h = (QwtHistogram *)g->plotItem(i);
            if (h && h->matrix() == m)
              h->loadData();
          } else {
            Spectrogram *sp = (Spectrogram *)g->plotItem(i);
            if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->matrix() == m)
              sp->updateData(m);
          }
        }
        g->updatePlot();
      }
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::add3DData()
{
  if (!hasTable()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no tables available in this project.</h4>"
            "<p><h4>Please create a table and try again!</h4>"));
    return;
  }

  QStringList zColumns = columnsList(Table::Z);
  if ((int)zColumns.count() <= 0)
  {
    QMessageBox::critical(this,tr("MantidPlot - Warning"),//Mantid
        tr("There are no available columns with plot designation set to Z!"));
    return;
  }

  DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect (ad,SIGNAL(options(const QString&)), this, SLOT(insertNew3DData(const QString&)));
  ad->setWindowTitle(tr("MantidPlot - Choose data set"));//Mantid
  ad->setCurveNames(zColumns);
  ad->exec();
}

void ApplicationWindow::change3DData()
{
  DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect (ad,SIGNAL(options(const QString&)), this, SLOT(change3DData(const QString&)));

  ad->setWindowTitle(tr("MantidPlot - Choose data set"));//Mantid
  ad->setCurveNames(columnsList(Table::Z));
  ad->exec();
}

void ApplicationWindow::change3DMatrix()
{
  DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect (ad, SIGNAL(options(const QString&)), this, SLOT(change3DMatrix(const QString&)));

  ad->setWindowTitle(tr("MantidPlot - Choose matrix to plot"));//Mantid
  ad->setCurveNames(matrixNames());

  Graph3D* g = (Graph3D*)activeWindow(Plot3DWindow);
  if (g && g->matrix())
    ad->setCurentDataSet(g->matrix()->objectName());
  ad->exec();
}

void ApplicationWindow::change3DMatrix(const QString& matrix_name)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D* g = (Graph3D*)w;
  Matrix *m = matrix(matrix_name);
  if (m && g)
    g->addMatrixData(m);

  emit modified();
}

void ApplicationWindow::add3DMatrixPlot()
{
  QStringList matrices = matrixNames();
  if ((int)matrices.count() <= 0)
  {
    QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no matrices available in this project.</h4>"
            "<p><h4>Please create a matrix and try again!</h4>"));
    return;
  }

  DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " :", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect (ad,SIGNAL(options(const QString&)), this, SLOT(insert3DMatrixPlot(const QString&)));

  ad->setWindowTitle(tr("MantidPlot - Choose matrix to plot"));//Mantid
  ad->setCurveNames(matrices);
  ad->exec();
}

void ApplicationWindow::insert3DMatrixPlot(const QString& matrix_name)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  ((Graph3D*)w)->addMatrixData(matrix(matrix_name));
  emit modified();
}

void ApplicationWindow::insertNew3DData(const QString& colName)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  ((Graph3D*)w)->insertNewData(table(colName),colName);
  emit modified();
}

void ApplicationWindow::change3DData(const QString& colName)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  ((Graph3D*)w)->changeDataColumn(table(colName), colName);
  emit modified();
}

void ApplicationWindow::editSurfacePlot()
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D* g = (Graph3D*)w;
  SurfaceDialog* sd = new SurfaceDialog(this);
  sd->setAttribute(Qt::WA_DeleteOnClose);

  if (g->hasData() && g->userFunction())
    sd->setFunction(g);
  else if (g->hasData() && g->parametricSurface())
    sd->setParametricSurface(g);
  sd->exec();
}

void ApplicationWindow::newSurfacePlot()
{
  SurfaceDialog* sd = new SurfaceDialog(this);
  sd->setAttribute(Qt::WA_DeleteOnClose);
  sd->exec();
}

Graph3D* ApplicationWindow::plotSurface(const QString& formula, double xl, double xr,
    double yl, double yr, double zl, double zr, int columns, int rows)
{
  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this);
  plot->resize(500,400);
  plot->setWindowTitle(label);
  plot->setName(label);
  customPlot3D(plot);
  plot->addFunction(formula, xl, xr, yl, yr, zl, zr, columns, rows);

  initPlot3D(plot);

  emit modified();
  return plot;
}

Graph3D* ApplicationWindow::plotParametricSurface(const QString& xFormula, const QString& yFormula,
    const QString& zFormula, double ul, double ur, double vl, double vr,
    int columns, int rows, bool uPeriodic, bool vPeriodic)
{
  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this);
  plot->resize(500, 400);
  plot->setWindowTitle(label);
  plot->setName(label);
  customPlot3D(plot);
  plot->addParametricSurface(xFormula, yFormula, zFormula, ul, ur, vl, vr,
      columns, rows, uPeriodic, vPeriodic);
  initPlot3D(plot);
  emit modified();
  return plot;
}

void ApplicationWindow::updateSurfaceFuncList(const QString& s)
{
  surfaceFunc.remove(s);
  surfaceFunc.push_front(s);
  while ((int)surfaceFunc.size() > 10)
    surfaceFunc.pop_back();
}

Graph3D* ApplicationWindow::dataPlot3D(const QString& caption,const QString& formula,
    double xl, double xr, double yl, double yr, double zl, double zr)
{
  int pos=formula.find("_",0);
  QString wCaption=formula.left(pos);

  Table* w=table(wCaption);
  if (!w)
    return 0;

  int posX=formula.find("(",pos);
  QString xCol=formula.mid(pos+1,posX-pos-1);

  pos=formula.find(",",posX);
  posX=formula.find("(",pos);
  QString yCol=formula.mid(pos+1,posX-pos-1);

  Graph3D *plot = new Graph3D("", this, 0);
  plot->addData(w, xCol, yCol, xl, xr, yl, yr, zl, zr);
  plot->update();

  QString label=caption;
  while(alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  plot->setWindowTitle(label);
  plot->setName(label);
  initPlot3D(plot);

  return plot;
}

Graph3D* ApplicationWindow::newPlot3D()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this, 0);
  plot->setWindowTitle(label);
  plot->setName(label);

  customPlot3D(plot);
  initPlot3D(plot);

  emit modified();
  QApplication::restoreOverrideCursor();
  return plot;
}

Graph3D* ApplicationWindow::plotXYZ(Table* table, const QString& zColName, int type)
{
  int zCol = table->colIndex(zColName);
  if (zCol < 0)
    return 0;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  Graph3D *plot = new Graph3D("", this, 0);
  QString label = generateUniqueName(tr("Graph"));
  plot->setWindowTitle(label);
  plot->setName(label);

  customPlot3D(plot);
  if (type == Graph3D::Ribbon) {
    int ycol = table->colIndex(zColName);
    plot->addData(table, table->colName(table->colX(ycol)), zColName);
  } else
    plot->addData(table, table->colX(zCol), table->colY(zCol), zCol, type);
  initPlot3D(plot);

  emit modified();
  QApplication::restoreOverrideCursor();
  return plot;
}

Graph3D* ApplicationWindow::openPlotXYZ(const QString& caption,const QString& formula,
    double xl, double xr, double yl, double yr, double zl, double zr)
{
  int pos=formula.find("_",0);
  QString wCaption=formula.left(pos);

  Table* w=table(wCaption);
  if (!w)
    return 0;

  int posX=formula.find("(X)",pos);
  QString xColName=formula.mid(pos+1,posX-pos-1);

  pos=formula.find(",",posX);

  posX=formula.find("(Y)",pos);
  QString yColName=formula.mid(pos+1,posX-pos-1);

  pos=formula.find(",",posX);
  posX=formula.find("(Z)",pos);
  QString zColName=formula.mid(pos+1,posX-pos-1);

  int xCol=w->colIndex(xColName);
  int yCol=w->colIndex(yColName);
  int zCol=w->colIndex(zColName);

  Graph3D *plot=new Graph3D("", this, 0);
  plot->loadData(w, xCol, yCol, zCol, xl, xr, yl, yr, zl, zr);

  QString label = caption;
  if (alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  plot->setWindowTitle(label);
  plot->setName(label);
  initPlot3D(plot);
  return plot;
}

void ApplicationWindow::customPlot3D(Graph3D *plot)
{
  plot->setDataColors(QColor(plot3DColors[4]), QColor(plot3DColors[0]));
  plot->setMeshColor(QColor(plot3DColors[2]));
  plot->setAxesColor(QColor(plot3DColors[6]));
  plot->setNumbersColor(QColor(plot3DColors[5]));
  plot->setLabelsColor(QColor(plot3DColors[1]));
  plot->setBackgroundColor(QColor(plot3DColors[7]));
  plot->setGridColor(QColor(plot3DColors[3]));
  plot->setResolution(plot3DResolution);
  plot->showColorLegend(showPlot3DLegend);
  plot->setAntialiasing(smooth3DMesh);
  plot->setOrthogonal(orthogonal3DPlots);
  if (showPlot3DProjection)
    plot->setFloorData();
  plot->setNumbersFont(plot3DNumbersFont);
  plot->setXAxisLabelFont(plot3DAxesFont);
  plot->setYAxisLabelFont(plot3DAxesFont);
  plot->setZAxisLabelFont(plot3DAxesFont);
  plot->setTitleFont(plot3DTitleFont);
}

void ApplicationWindow::initPlot3D(Graph3D *plot)
{
  d_workspace->addSubWindow(plot);
  connectSurfacePlot(plot);

  plot->setIcon(getQPixmap("trajectory_xpm"));
  plot->show();
  plot->setFocus();

  addListViewItem(plot);

  if (!plot3DTools->isVisible())
    plot3DTools->show();

  if (!plot3DTools->isEnabled())
    plot3DTools->setEnabled(true);

  customMenu(plot);
  customToolBars(plot);
}

void ApplicationWindow::exportMatrix()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  ImageExportDialog *ied = new ImageExportDialog(this, m!=NULL, d_extended_export_dialog);
  ied->setDir(workingDir);
  ied->selectFilter(d_image_export_filter);
  if ( ied->exec() != QDialog::Accepted )
    return;
  workingDir = ied->directory().path();
  if (ied->selectedFiles().isEmpty())
    return;

  QString selected_filter = ied->selectedFilter();
  QString file_name = ied->selectedFiles()[0];
  QFileInfo file_info(file_name);
  if (!file_info.fileName().contains("."))
    file_name.append(selected_filter.remove("*"));

  QFile file(file_name);
  if (!file.open( QIODevice::WriteOnly )){
    QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
    return;
  }

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
    m->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
  else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i=0; i<(int)list.count(); i++){
      if (selected_filter.contains("." + (list[i]).lower()))
        m->image().save(file_name, list[i], ied->quality());
    }
  }
}

Matrix* ApplicationWindow::importImage(const QString& fileName)
{
  QString fn = fileName;
  if (fn.isEmpty()){
    QList<QByteArray> list = QImageReader::supportedImageFormats();
    QString filter = tr("Images") + " (", aux1, aux2;
    for (int i=0; i<(int)list.count(); i++){
      aux1 = " *."+list[i]+" ";
      aux2 += " *."+list[i]+";;";
      filter += aux1;
    }
    filter+=");;" + aux2;

    fn = QFileDialog::getOpenFileName(this, tr("MantidPlot - Import image from file"), imagesDirPath, filter);//Mantid
    if ( !fn.isEmpty() ){
      QFileInfo fi(fn);
      imagesDirPath = fi.dirPath(true);
    }
  }

  QImage image(fn);
  if (image.isNull())
    return 0;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MdiSubWindow *w = activeWindow(MatrixWindow);
  Matrix* m = NULL;
  if (w){
    m = (Matrix *)w;
    m->importImage(fn);
  } else {
    m = new Matrix(scriptingEnv(), image, "", this);
    initMatrix(m, generateUniqueName(tr("Matrix")));
    m->show();
    m->setWindowLabel(fn);
    m->setCaptionPolicy(MdiSubWindow::Both);
  }

  QApplication::restoreOverrideCursor();
  return m;
}

void ApplicationWindow::loadImage()
{
  QList<QByteArray> list = QImageReader::supportedImageFormats();
  QString filter = tr("Images") + " (", aux1, aux2;
  for (int i=0; i<(int)list.count(); i++){
    aux1 = " *."+list[i]+" ";
    aux2 += " *."+list[i]+";;";
    filter += aux1;
  }
  filter+=");;" + aux2;

  QString fn = QFileDialog::getOpenFileName(this, tr("MantidPlot - Load image from file"), imagesDirPath, filter);//Mantid
  if ( !fn.isEmpty() ){
    loadImage(fn);
    QFileInfo fi(fn);
    imagesDirPath = fi.dirPath(true);
  }
}

void ApplicationWindow::loadImage(const QString& fn)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer *plot = multilayerPlot(generateUniqueName(tr("Graph")));
  plot->setWindowLabel(fn);
  plot->setCaptionPolicy(MdiSubWindow::Both);

  Graph *g = plot->activeGraph();
  g->setTitle("");
  for (int i=0; i<4; i++)
    g->enableAxis(i, false);
  g->removeLegend();
  g->addImage(fn);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::polishGraph(Graph *g, int style)
{
  if (style == Graph::VerticalBars || style == Graph::HorizontalBars ||style == Graph::Histogram)
  {
    QList<int> ticksList;
    int ticksStyle = ScaleDraw::Out;
    ticksList<<ticksStyle<<ticksStyle<<ticksStyle<<ticksStyle;
    g->setMajorTicksType(ticksList);
    g->setMinorTicksType(ticksList);
  }
  if (style == Graph::HorizontalBars){
    g->setAxisTitle(QwtPlot::xBottom, tr("X Axis Title"));
    g->setAxisTitle(QwtPlot::yLeft, tr("Y Axis Title"));
  }
}

MultiLayer* ApplicationWindow::multilayerPlot(const QString& caption, int layers, int rows, int cols)
{
  MultiLayer* ml = new MultiLayer(this, layers, rows, cols);
  QString label = caption;
  initMultilayerPlot(ml, label.replace(QRegExp("_"), "-"));
  return ml;
}

MultiLayer* ApplicationWindow::newGraph(const QString& caption)
{
  MultiLayer *ml = multilayerPlot(generateUniqueName(caption));
  if (ml){
    Graph *g = ml->activeGraph();
    setPreferences(g);
    g->newLegend();
  }
  return ml;
}

MultiLayer* ApplicationWindow::multilayerPlot(Table* w, const QStringList& colList, int style, int startRow, int endRow)
{//used when plotting selected columns
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph *ag = g->activeGraph();
  if (!ag)
    return 0;

  setPreferences(ag);
  ag->addCurves(w, colList, style, defaultCurveLineWidth, defaultSymbolSize, startRow, endRow);

  polishGraph(ag, style);
  ag->newLegend();

  ag->setAutoScale();//Mantid

  QApplication::restoreOverrideCursor();
  return g;
}

MultiLayer* ApplicationWindow::multilayerPlot(int c, int r, int style)
{//used when plotting from the panel menu
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return 0;

  if (!validFor2DPlot(t))
    return 0;

  QStringList list = t->selectedYColumns();
  if((int)list.count() < 1) {
    QMessageBox::warning(this, tr("MantidPlot - Plot error"), tr("Please select a Y column to plot!"));//Mantid
    return 0;
  }

  int curves = list.count();
  if (r < 0)
    r = curves;

  int layers = c*r;
  MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")), layers, r, c);
  QList<Graph *> layersList = g->layersList();
  int i = 0;
  foreach(Graph *ag, layersList){
    setPreferences(ag);
    if (i < curves)
      ag->addCurves(t, QStringList(list[i]), style, defaultCurveLineWidth, defaultSymbolSize);
    ag->newLegend();
    polishGraph(ag, style);
    i++;
  }
  g->arrangeLayers(false, false);
  return g;
}

MultiLayer* ApplicationWindow::multilayerPlot(const QStringList& colList)
{//used when plotting from wizard
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph *ag = g->activeGraph();
  setPreferences(ag);
  polishGraph(ag, defaultCurveStyle);
  int curves = (int)colList.count();
  int errorBars = 0;
  for (int i=0; i<curves; i++) {
    if (colList[i].contains("(yErr)") || colList[i].contains("(xErr)"))
      errorBars++;
  }

  for (int i=0; i<curves; i++){
    QString s = colList[i];
    int pos = s.find(":", 0);
    QString caption = s.left(pos) + "_";
    Table *w = (Table *)table(caption);

    int posX = s.find("(X)", pos);
    QString xColName = caption+s.mid(pos+2, posX-pos-2);
    int xCol=w->colIndex(xColName);

    posX = s.find(",", posX);
    int posY = s.find("(Y)", posX);
    QString yColName = caption+s.mid(posX+2, posY-posX-2);

    PlotCurve *c = NULL;
    if (s.contains("(yErr)") || s.contains("(xErr)")){
      posY = s.find(",", posY);
      int posErr, errType;
      if (s.contains("(yErr)")){
        errType = QwtErrorPlotCurve::Vertical;
        posErr = s.find("(yErr)", posY);
      } else {
        errType = QwtErrorPlotCurve::Horizontal;
        posErr = s.find("(xErr)",posY);
      }

      QString errColName = caption+s.mid(posY+2, posErr-posY-2);
      c = (PlotCurve *)ag->addErrorBars(xColName, yColName, w, errColName, errType);
    } else
      c = (PlotCurve *)ag->insertCurve(w, xCol, yColName, defaultCurveStyle);

    CurveLayout cl = ag->initCurveLayout(defaultCurveStyle, curves - errorBars);
    cl.lWidth = defaultCurveLineWidth;
    cl.sSize = defaultSymbolSize;
    ag->updateCurveLayout(c, &cl);
  }
  ag->newLegend();
  ag->initScaleLimits();
  QApplication::restoreOverrideCursor();
  return g;
}

void ApplicationWindow::initMultilayerPlot(MultiLayer* g, const QString& name)
{
  QString label = name;
  while(alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  g->setWindowTitle(label);
  g->setName(label);
  g->setIcon(getQPixmap("graph_xpm"));
  g->setScaleLayersOnPrint(d_scale_plots_on_print);
  g->printCropmarks(d_print_cropmarks);

  d_workspace->addSubWindow(g);
  connectMultilayerPlot(g);
  g->showNormal();

  addListViewItem(g);
}

void ApplicationWindow::customizeTables(const QColor& bgColor,const QColor& textColor,
    const QColor& headerColor,const QFont& textFont, const QFont& headerFont, bool showComments)
{
  tableBkgdColor = bgColor;
  tableTextColor = textColor;
  tableHeaderColor = headerColor;
  tableTextFont = textFont;
  tableHeaderFont = headerFont;
  d_show_table_comments = showComments;

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->inherits("Table"))
      customTable((Table*)w);
  }
}

void ApplicationWindow::setAutoUpdateTableValues(bool on)
{
  if (d_auto_update_table_values == on)
    return;

  d_auto_update_table_values = on;

  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->inherits("Table"))
        ((Table *)w)->setAutoUpdateValues(d_auto_update_table_values);
    }
    f = f->folderBelow();
  }
}

void ApplicationWindow::customTable(Table* w)
{
  QColorGroup cg;
  cg.setColor(QColorGroup::Base, QColor(tableBkgdColor));
  cg.setColor(QColorGroup::Text, QColor(tableTextColor));
  w->setPalette(QPalette(cg, cg, cg));

  w->setHeaderColor(tableHeaderColor);
  w->setTextFont(tableTextFont);
  w->setHeaderFont(tableHeaderFont);
  w->showComments(d_show_table_comments);
  w->setNumericPrecision(d_decimal_digits);
}

void ApplicationWindow::setPreferences(Graph* g)
{
  if (!g->isPiePlot()){
    if (allAxesOn){
      for (int i=0; i<4; i++)
        g->enableAxis(i);
      g->updateSecondaryAxis(QwtPlot::xTop);
      g->updateSecondaryAxis(QwtPlot::yRight);
    }

    //set the scale type i.e. log or linear
    g->setScale(QwtPlot::xBottom, xaxisScale);
    g->setScale(QwtPlot::yLeft, yaxisScale);
    // yRight is the color bar, representing a third (z) dimension
    g->setScale(QwtPlot::yRight, zaxisScale);

    QList<int> ticksList;
    ticksList<<majTicksStyle<<majTicksStyle<<majTicksStyle<<majTicksStyle;
    g->setMajorTicksType(ticksList);
    ticksList.clear();
    ticksList<<minTicksStyle<<minTicksStyle<<minTicksStyle<<minTicksStyle;
    g->setMinorTicksType(ticksList);

    g->setTicksLength (minTicksLength, majTicksLength);
    g->setAxesLinewidth(axesLineWidth);
    g->drawAxesBackbones(drawBackbones);
    //    need to call the plot functions for log/linear, errorbars and distribution stuff
  }

  g->initFonts(plotAxesFont, plotNumbersFont);
  g->initTitle(titleOn, plotTitleFont);
  g->setCanvasFrame(canvasFrameWidth);
  g->plotWidget()->setMargin(defaultPlotMargin);


  g->enableAutoscaling(autoscale2DPlots);
  g->setAutoscaleFonts(autoScaleFonts);
  g->setIgnoreResizeEvents(!autoResizeLayers);
  g->setAntialiasing(antialiasing2DPlots);

}

/*
 *creates a new empty table
 */
Table* ApplicationWindow::newTable()
{
  Table* w = new Table(scriptingEnv(), 30, 2, "", this, 0);
  initTable(w, generateUniqueName(tr("Table")));
  w->showNormal();
  return w;
}

/*
 *used when opening a project file
 */
Table* ApplicationWindow::newTable(const QString& caption, int r, int c)
{
  Table* w = new Table(scriptingEnv(), r, c, "", this, 0);
  initTable(w, caption);
  if (w->objectName() != caption){//the table was renamed
    renamedTables << caption << w->objectName();
    if (d_inform_rename_table){
      QMessageBox:: warning(this, tr("MantidPlot - Renamed Window"),//Mantid
          tr("The table '%1' already exists. It has been renamed '%2'.").arg(caption).arg(w->objectName()));
    }
  }
  w->showNormal();
  return w;
}

Table* ApplicationWindow::newTable(int r, int c, const QString& name, const QString& legend)
{
  Table* w = new Table(scriptingEnv(), r, c, legend, this, 0);
  initTable(w, name);
  return w;
}

Table* ApplicationWindow::newTable(const QString& caption, int r, int c, const QString& text)
{
  QStringList lst = caption.split("\t", QString::SkipEmptyParts);
  QString legend = QString();
  if (lst.count() == 2)
    legend = lst[1];

  Table* w = new Table(scriptingEnv(), r, c, legend, this, 0);

  QStringList rows = text.split("\n", QString::SkipEmptyParts);
  QString rlist = rows[0];
  QStringList list = rlist.split("\t");
  w->setHeader(list);

  for (int i=0; i<r; i++)
  {
    rlist=rows[i+1];
    list=rlist.split("\t");
    for (int j=0; j<c; j++)
      w->setText(i, j, list[j]);
  }

  initTable(w, lst[0]);
  w->showNormal();
  return w;
}

Table* ApplicationWindow::newHiddenTable(const QString& name, const QString& label, int r, int c, const QString& text)
{
  Table* w = new Table(scriptingEnv(), r, c, label, this, 0);

  if (!text.isEmpty()) {
    QStringList rows = text.split("\n", QString::SkipEmptyParts);
    QStringList list = rows[0].split("\t");
    w->setHeader(list);

    QString rlist;
    for (int i=0; i<r; i++){
      rlist=rows[i+1];
      list = rlist.split("\t");
      for (int j=0; j<c; j++)
        w->setText(i, j, list[j]);
    }
  }

  initTable(w, name);
  hideWindow(w);
  return w;
}

void ApplicationWindow::initTable(Table* w, const QString& caption)
{
  QString name = caption;
  name = name.replace ("_","-");

  while(name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Table"));

  d_workspace->addSubWindow(w);

  connectTable(w);
  customTable(w);

  w->setName(name);
  w->setIcon( getQPixmap("worksheet_xpm") );
  w->setSpecifications(w->saveToString(windowGeometryInfo(w)));

  addListViewItem(w);
}

/*
 * !creates a new table with type statistics on target columns/rows of table base
 */
TableStatistics *ApplicationWindow::newTableStatistics(Table *base, int type, QList<int> target, const QString &caption)
{
  TableStatistics* s = new TableStatistics(scriptingEnv(), this, base, (TableStatistics::Type) type, target);
  if (caption.isEmpty())
    initTable(s, s->objectName());
  else
    initTable(s, caption);
  s->showNormal();
  return s;
}

/*
 *creates a new empty note window
 */
Note* ApplicationWindow::newNote(const QString& caption)
{
  Note* m = new Note(scriptingEnv(), "", this);

  QString name = caption;
  while(name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Notes"));

  m->setName(name);
  m->setIcon(getQPixmap("note_xpm"));
  m->askOnCloseEvent(confirmCloseNotes);
  m->setDirPath(scriptsDirPath);

  d_workspace->addSubWindow(m);
  addListViewItem(m);

  connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
  connect(m, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect(m, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
  connect(m, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
  connect(m, SIGNAL(statusChanged(MdiSubWindow*)), this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect(m, SIGNAL(dirPathChanged(const QString&)), this, SLOT(scriptsDirPathChanged(const QString&)));

  m->showNormal();
  return m;
}

Matrix* ApplicationWindow::newMatrix(int rows, int columns)
{
  Matrix* m = new Matrix(scriptingEnv(), rows, columns, "", this, 0);
  initMatrix(m, generateUniqueName(tr("Matrix")));
  m->showNormal();
  return m;
}

Matrix* ApplicationWindow::newMatrix(const QString& caption, int r, int c)
{
  Matrix* w = new Matrix(scriptingEnv(), r, c, "", this, 0);
  initMatrix(w, caption);
  if (w->objectName() != caption)//the matrix was renamed
    renamedTables << caption << w->objectName();

  w->showNormal();
  return w;
}

void ApplicationWindow::viewMatrixImage()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetViewCommand(m, m->viewType(), Matrix::ImageView, tr("Set Image Mode")));
  m->setViewType(Matrix::ImageView);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixTable()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetViewCommand(m, m->viewType(), Matrix::TableView, tr("Set Data Mode")));
  m->setViewType(Matrix::TableView);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixXY()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetHeaderViewCommand(m, m->headerViewType(), Matrix::XY, tr("Show X/Y")));
  m->setHeaderViewType(Matrix::XY);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixColumnRow()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetHeaderViewCommand(m, m->headerViewType(), Matrix::ColumnRow, tr("Show Column/Row")));
  m->setHeaderViewType(Matrix::ColumnRow);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixGrayScale()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetColorMapCommand(m, m->colorMapType(), m->colorMap(),
      Matrix::GrayScale, QwtLinearColorMap(), tr("Set Gray Scale Palette")));
  m->setGrayScale();
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixRainbowScale()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetColorMapCommand(m, m->colorMapType(), m->colorMap(),
      Matrix::Rainbow, QwtLinearColorMap(), tr("Set Rainbow Palette")));
  m->setRainbowColorMap();
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::showColorMapDialog()
{
  Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  ColorMapDialog *cmd = new ColorMapDialog(this);
  cmd->setAttribute(Qt::WA_DeleteOnClose);
  cmd->setMatrix(m);
  cmd->exec();
}

void ApplicationWindow::transposeMatrix()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->transpose();
}

void ApplicationWindow::flipMatrixVertically()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->flipVertically();
}

void ApplicationWindow::flipMatrixHorizontally()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->flipHorizontally();
}

void ApplicationWindow::rotateMatrix90()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->rotate90();
}

void ApplicationWindow::rotateMatrixMinus90()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->rotate90(false);
}

void ApplicationWindow::matrixDeterminant()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  QDateTime dt = QDateTime::currentDateTime ();
  QString info=dt.toString(Qt::LocalDate);
  info+= "\n" + tr("Determinant of ") + QString(m->objectName()) + ":\t";
  info+= "det = " + QString::number(m->determinant()) + "\n";
  info+="-------------------------------------------------------------\n";

  current_folder->appendLogInfo(info);

  showResults(true);
}

void ApplicationWindow::invertMatrix()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->invert();
}

Table* ApplicationWindow::convertMatrixToTableDirect()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return 0;

  return matrixToTable(m, Direct);
}

Table* ApplicationWindow::convertMatrixToTableXYZ()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return 0;

  return matrixToTable(m, XYZ);
}

Table* ApplicationWindow::convertMatrixToTableYXZ()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return 0;

  return matrixToTable(m, YXZ);
}

Table* ApplicationWindow::matrixToTable(Matrix* m, MatrixToTableConversion conversionType)
{
  if (!m)
    return 0;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int rows = m->numRows();
  int cols = m->numCols();
  MatrixModel *mModel = m->matrixModel();

  Table* w = NULL;
  if (conversionType == Direct){
    w = new Table(scriptingEnv(), rows, cols, "", this, 0);
    for (int i = 0; i<rows; i++){
      for (int j = 0; j<cols; j++)
        w->setCell(i, j, m->cell(i,j));
    }
  } else if (conversionType == XYZ){
    int tableRows = rows*cols;
    w = new Table(scriptingEnv(), tableRows, 3, "", this, 0);
    for (int i = 0; i<rows; i++){
      for (int j = 0; j<cols; j++){
        int cell = i*cols + j;
        w->setCell(cell, 0, mModel->x(j));
        w->setCell(cell, 1, mModel->y(i));
        w->setCell(cell, 2, mModel->cell(i, j));
      }
    }
  } else if (conversionType == YXZ){
    int tableRows = rows*cols;
    w = new Table(scriptingEnv(), tableRows, 3, "", this, 0);
    for (int i = 0; i<cols; i++){
      for (int j = 0; j<rows; j++){
        int cell = i*rows + j;
        w->setCell(cell, 0, mModel->x(i));
        w->setCell(cell, 1, mModel->y(j));
        w->setCell(cell, 2, mModel->cell(i, j));
      }
    }
  }


  initTable(w, generateUniqueName(tr("Table")));
  w->setWindowLabel(m->windowLabel());
  w->setCaptionPolicy(m->captionPolicy());
  w->resize(m->size());
  w->showNormal();

  QApplication::restoreOverrideCursor();
  return w;
}

void ApplicationWindow::initMatrix(Matrix* m, const QString& caption)
{
  QString name = caption;
  while(alreadyUsedName(name)){name = generateUniqueName(tr("Matrix"));}

  m->setWindowTitle(name);
  m->setName(name);
  m->setIcon( m->matrixIcon() );//Mantid
  m->askOnCloseEvent(confirmCloseMatrix);
  m->setNumericPrecision(d_decimal_digits);

  d_workspace->addSubWindow(m);
  addListViewItem(m);

  QUndoStack *stack = m->undoStack();
  connect(stack, SIGNAL(canUndoChanged(bool)), actionUndo, SLOT(setEnabled(bool)));
  connect(stack, SIGNAL(canRedoChanged(bool)), actionRedo, SLOT(setEnabled(bool)));

  connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
  connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(updateMatrixPlots(MdiSubWindow *)));
  connect(m, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect(m, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
  connect(m, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
  connect(m, SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect(m, SIGNAL(showContextMenu()), this, SLOT(showWindowContextMenu()));

  emit modified();
}

Matrix* ApplicationWindow::convertTableToMatrix()
{
  Table* t = (Table*)activeWindow(TableWindow);
  if (!t)
    return 0;

  return tableToMatrix (t);
}

Matrix* ApplicationWindow::tableToMatrix(Table* t)
{
  if (!t)
    return 0;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int rows = t->numRows();
  int cols = t->numCols();

  QString caption = generateUniqueName(tr("Matrix"));
  Matrix* m = new Matrix(scriptingEnv(), rows, cols, "", this, 0);
  initMatrix(m, caption);

  for (int i = 0; i<rows; i++){
    for (int j = 0; j<cols; j++)
      m->setCell(i, j, t->cell(i, j));
  }

  m->setWindowLabel(m->windowLabel());
  m->setCaptionPolicy(m->captionPolicy());
  m->resize(m->size());
  m->showNormal();

  QApplication::restoreOverrideCursor();
  return m;
}

MdiSubWindow* ApplicationWindow::window(const QString& name)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->objectName() == name)
      return w;
  }
  return  NULL;
}

Table* ApplicationWindow::table(const QString& name)
{
  int pos = name.find("_", 0);
  QString caption = name.left(pos);

  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->inherits("Table") && w->objectName() == caption)
        return (Table*)w;
    }
    f = f->folderBelow();
  }

  return  0;
}

Matrix* ApplicationWindow::matrix(const QString& name)
{
  QString caption = name;
  if (!renamedTables.isEmpty() && renamedTables.contains(caption)){
    int index = renamedTables.findIndex(caption);
    caption = renamedTables[index+1];
  }

  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->isA("Matrix") && w->objectName() == caption)
        return (Matrix*)w;
    }
    f = f->folderBelow();
  }
  return  0;
}

MdiSubWindow *ApplicationWindow::activeWindow(WindowType type)
{
  if (!d_active_window)
    return NULL;

  switch(type){
  case NoWindow:
    break;

  case TableWindow:
    if (d_active_window->inherits("Table"))
      return d_active_window;
    else
      return NULL;
    break;

  case MatrixWindow:
    if (d_active_window->inherits("Matrix"))//Mantid
      return d_active_window;
    else
      return NULL;
    break;

  case MultiLayerWindow:
    if (d_active_window->isA("MultiLayer"))
      return d_active_window;
    else
      return NULL;
    break;

  case NoteWindow:
    if (d_active_window->isA("Note"))
      return d_active_window;
    else
      return NULL;
    break;

  case Plot3DWindow:
    if (d_active_window->isA("Graph3D"))
      return d_active_window;
    else
      return NULL;
    break;
  }
  return d_active_window;
}

void ApplicationWindow::windowActivated(QMdiSubWindow *w)
{	
  MdiSubWindow *qti_subwin = qobject_cast<MdiSubWindow*>(w);
  if( !qti_subwin ) return;

  if( d_active_window && d_active_window == qti_subwin ) return;

  d_active_window = qti_subwin;
  customToolBars(w);
  customMenu(w);

  if (d_opening_file) return;

  QList<MdiSubWindow *> windows = current_folder->windowsList();
  foreach(MdiSubWindow *ow, windows){
    if (ow != w && ow->status() == MdiSubWindow::Maximized){
      ow->setNormal();
      break;
    }
  }

  Folder *f = qti_subwin->folder();
  if (f)
    f->setActiveWindow(qti_subwin);

  emit modified();
}

void ApplicationWindow::addErrorBars()
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  MultiLayer* plot = (MultiLayer*)w;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  if (!g->curves()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no curves available on this plot!"));//Mantid
    return;
  }

  if (g->isPiePlot()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("This functionality is not available for pie plots!"));//Mantid
    return;
  }

  ErrDialog* ed = new ErrDialog(this);
  ed->setAttribute(Qt::WA_DeleteOnClose);
  connect (ed,SIGNAL(options(const QString&,int,const QString&,int,bool)),this,SLOT(defineErrorBars(const QString&,int,const QString&,int,bool))); 
  connect (ed,SIGNAL(options(const QString&,const QString&,int)),this,SLOT(defineErrorBars(const QString&,const QString&,int)));

  ed->setCurveNames(g->analysableCurvesList());
  ed->setSrcTables(tableList());
  ed->exec();
}

void ApplicationWindow::removeErrorBars()
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  MultiLayer* plot = (MultiLayer*)w;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  if (!g->curves()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no curves available on this plot!"));//Mantid
    return;
  }

  if (g->isPiePlot()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("This functionality is not available for pie plots!"));//Mantid
    return;
  }

  RemoveErrorsDialog* ed = new RemoveErrorsDialog(this);
  connect (ed,SIGNAL(curveName(const QString&)),this,SLOT(removeErrorBars(const QString&)));

  ed->setCurveNames(g->analysableCurvesList());
  ed->exec();
}

void ApplicationWindow::removeErrorBars(const QString& name)
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return;

  g->removeMantidErrorBars(name);

}

void ApplicationWindow::defineErrorBars(const QString& name, int type, const QString& percent, int direction, bool drawAll)
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return;

  if (type == 2) // A MantidCurve - do all the work in the Graph method
  {
    g->addMantidErrorBars(name, drawAll);
    return;
  }

  Table *t = table(name);
  if (!t){//user defined function
    QMessageBox::critical(this,tr("MantidPlot - Error bars error"),//Mantid
        tr("This feature is not available for user defined function curves!"));
    return;
  }

  DataCurve *master_curve = (DataCurve *)g->curve(name);
  QString xColName = master_curve->xColumnName();
  if (xColName.isEmpty())
    return;

  if (direction == QwtErrorPlotCurve::Horizontal)
    t->addCol(Table::xErr);
  else
    t->addCol(Table::yErr);

  int r=t->numRows();
  int c=t->numCols()-1;
  int ycol=t->colIndex(name);
  if (!direction)
    ycol=t->colIndex(xColName);

  QVarLengthArray<double> Y(r);
  Y=t->col(ycol);
  QString errColName=t->colName(c);

  double prc=percent.toDouble();
  double moyenne=0.0;
  if (type==0){
    for (int i=0;i<r;i++){
      if (!t->text(i,ycol).isEmpty())
        t->setText(i, c, QString::number(Y[i]*prc/100.0,'g',15));
    }
  } else if (type==1) {
    int i;
    double dev=0.0;
    for (i=0;i<r;i++)
      moyenne+=Y[i];
    moyenne/=r;
    for (i=0;i<r;i++)
      dev+=(Y[i]-moyenne)*(Y[i]-moyenne);
    dev=sqrt(dev/(r-1));
    for (i=0;i<r;i++){
      if (!t->table()->item(i,ycol)->text().isEmpty())
        t->setText(i, c, QString::number(dev, 'g', 15));
    }
  }
  g->addErrorBars(xColName, name, t, errColName, direction);
}

void ApplicationWindow::defineErrorBars(const QString& curveName, const QString& errColumnName, int direction)
{
  Table *w=table(curveName);
  if (!w){//user defined function --> no worksheet available
    QMessageBox::critical(this,tr("MantidPlot - Error"),//Mantid
        tr("This feature is not available for user defined function curves!"));
    return;
  }

  Table *errTable=table(errColumnName);
  if (w->numRows() != errTable->numRows()){
    QMessageBox::critical(this,tr("MantidPlot - Error"),//Mantid
        tr("The selected columns have different numbers of rows!"));

    addErrorBars();
    return;
  }

  int errCol=errTable->colIndex(errColumnName);
  if (errTable->isEmptyColumn(errCol)){
    QMessageBox::critical(this, tr("MantidPlot - Error"),//Mantid
        tr("The selected error column is empty!"));
    addErrorBars();
    return;
  }

  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  g->addErrorBars(curveName, errTable, errColumnName, direction);
  emit modified();
}

void ApplicationWindow::removeCurves(const QString& name)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer"))
    {
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers)
      g->removeCurves(name);
    }
    else if (w->isA("Graph3D"))
    {
      if ( (((Graph3D*)w)->formula()).contains(name) )
        ((Graph3D*)w)->clearData();
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateCurves(Table *t, const QString& name)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurvesData(t, name);
    } else if (w->isA("Graph3D")){
      Graph3D* g = (Graph3D*)w;
      if ((g->formula()).contains(name))
        g->updateData(t);
    }
  }
}

void ApplicationWindow::showPreferencesDialog()
{
  ConfigDialog* cd= new ConfigDialog(this);
  cd->setAttribute(Qt::WA_DeleteOnClose);
  cd->setColumnSeparator(columnSeparator);
  cd->exec();
}

void ApplicationWindow::setSaveSettings(bool autoSaving, int min)
{
  if (autoSave==autoSaving && autoSaveTime==min)
    return;

  autoSave=autoSaving;
  autoSaveTime=min;

  killTimer(savingTimerId);

  if (autoSave)
    savingTimerId=startTimer(autoSaveTime*60000);
  else
    savingTimerId=0;
}

void ApplicationWindow::changeAppStyle(const QString& s)
{
  // style keys are case insensitive
  if (appStyle.toLower() == s.toLower())
    return;

  qApp->setStyle(s);
  appStyle = qApp->style()->objectName();

  QPalette pal = qApp->palette();
  pal.setColor (QPalette::Active, QPalette::Base, QColor(panelsColor));
  qApp->setPalette(pal);

}

void ApplicationWindow::changeAppFont(const QFont& f)
{
  if (appFont == f)
    return;

  appFont = f;
  updateAppFonts();
}

void ApplicationWindow::updateAppFonts()
{
  qApp->setFont(appFont);
  this->setFont(appFont);
  info->setFont(QFont(appFont.family(), 2 + appFont.pointSize(), QFont::Bold,false));
}

void ApplicationWindow::updateConfirmOptions(bool askTables, bool askMatrices, bool askPlots2D,
    bool askPlots3D, bool askNotes,bool askInstrWindow)
{
  QList<MdiSubWindow *> windows = windowsList();


  if (confirmCloseTable != askTables){
    confirmCloseTable=askTables;
    foreach(MdiSubWindow *w, windows){

      if (w->inherits("Table"))
      {w->askOnCloseEvent(confirmCloseTable);
      }
    }
  }

  if (confirmCloseMatrix != askMatrices){
    confirmCloseMatrix = askMatrices;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Matrix"))
      {w->askOnCloseEvent(confirmCloseMatrix);
      }
    }
  }

  if (confirmClosePlot2D != askPlots2D){
    confirmClosePlot2D=askPlots2D;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("MultiLayer"))
      {w->askOnCloseEvent(confirmClosePlot2D);
      }
    }
  }

  if (confirmClosePlot3D != askPlots3D){
    confirmClosePlot3D=askPlots3D;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Graph3D"))
        w->askOnCloseEvent(confirmClosePlot3D);
    }
  }

  if (confirmCloseNotes != askNotes){
    confirmCloseNotes = askNotes;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Note"))
        w->askOnCloseEvent(confirmCloseNotes);
    }
  }

  if (confirmCloseInstrWindow != askInstrWindow){
    confirmCloseInstrWindow = askInstrWindow;

    foreach(MdiSubWindow *w, windows){
      if (w->isA("InstrumentWindow"))
      {w->askOnCloseEvent(confirmCloseInstrWindow);
      }
    }
  }
}

void ApplicationWindow::setGraphDefaultSettings(bool autoscale, bool scaleFonts,
    bool resizeLayers, bool antialiasing)
{
  if (autoscale2DPlots == autoscale &&
      autoScaleFonts == scaleFonts &&
      autoResizeLayers != resizeLayers &&
      antialiasing2DPlots == antialiasing)
    return;

  autoscale2DPlots = autoscale;
  autoScaleFonts = scaleFonts;
  autoResizeLayers = !resizeLayers;
  antialiasing2DPlots = antialiasing;

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers){
        g->enableAutoscaling(autoscale2DPlots);
        g->updateScale();
        g->setIgnoreResizeEvents(!autoResizeLayers);
        g->setAutoscaleFonts(autoScaleFonts);
        g->setAntialiasing(antialiasing2DPlots);
      }
    }
  }
}

void ApplicationWindow::setLegendDefaultSettings(int frame, const QFont& font,
    const QColor& textCol, const QColor& backgroundCol)
{
  if (legendFrameStyle == frame &&
      legendTextColor == textCol &&
      legendBackground == backgroundCol &&
      plotLegendFont == font)
    return;

  legendFrameStyle = frame;
  legendTextColor = textCol;
  legendBackground = backgroundCol;
  plotLegendFont = font;
  saveSettings();
}

void ApplicationWindow::setArrowDefaultSettings(double lineWidth,  const QColor& c, Qt::PenStyle style,
    int headLength, int headAngle, bool fillHead)
{
  if (defaultArrowLineWidth == lineWidth &&
      defaultArrowColor == c &&
      defaultArrowLineStyle == style &&
      defaultArrowHeadLength == headLength &&
      defaultArrowHeadAngle == headAngle &&
      defaultArrowHeadFill == fillHead)
    return;

  defaultArrowLineWidth = lineWidth;
  defaultArrowColor = c;
  defaultArrowLineStyle = style;
  defaultArrowHeadLength = headLength;
  defaultArrowHeadAngle = headAngle;
  defaultArrowHeadFill = fillHead;
  saveSettings();
}

ApplicationWindow * ApplicationWindow::plotFile(const QString& fn)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  ApplicationWindow *app = new ApplicationWindow();
  app->restoreApplicationGeometry();

  Table* t = app->newTable();
  if (!t)
    return NULL;

  t->importASCII(fn, app->columnSeparator, 0, app->renameColumns, app->strip_spaces, app->simplify_spaces,
      app->d_ASCII_import_comments, app->d_ASCII_comment_string,
      app->d_ASCII_import_read_only, Table::Overwrite, app->d_ASCII_end_line);
  t->setCaptionPolicy(MdiSubWindow::Both);
  app->multilayerPlot(t, t->YColumns(),Graph::LineSymbols);
  QApplication::restoreOverrideCursor();
  return 0;
}

void ApplicationWindow::importASCII()
{
  ImportASCIIDialog *import_dialog = new ImportASCIIDialog(!activeWindow(TableWindow) && !activeWindow(MatrixWindow), this, d_extended_import_ASCII_dialog);
  import_dialog->setDir(asciiDirPath);
  import_dialog->selectFilter(d_ASCII_file_filter);
  if (import_dialog->exec() != QDialog::Accepted)
    return;
  asciiDirPath = import_dialog->directory().path();
  d_ASCII_import_mode = import_dialog->importMode();
  columnSeparator = import_dialog->columnSeparator();
  ignoredLines = import_dialog->ignoredLines();
  renameColumns = import_dialog->renameColumns();
  strip_spaces = import_dialog->stripSpaces();
  simplify_spaces = import_dialog->simplifySpaces();
  d_ASCII_import_locale = import_dialog->decimalSeparators();
  d_import_dec_separators = import_dialog->updateDecimalSeparators();
  d_ASCII_comment_string = import_dialog->commentString();
  d_ASCII_import_comments = import_dialog->importComments();
  d_ASCII_import_read_only = import_dialog->readOnly();
  d_ASCII_end_line = (EndLineChar)import_dialog->endLineChar();
  saveSettings();

  importASCII(import_dialog->selectedFiles(),
      import_dialog->importMode(),
      import_dialog->columnSeparator(),
      import_dialog->ignoredLines(),
      import_dialog->renameColumns(),
      import_dialog->stripSpaces(),
      import_dialog->simplifySpaces(),
      import_dialog->importComments(),
      import_dialog->updateDecimalSeparators(),
      import_dialog->decimalSeparators(),
      import_dialog->commentString(),
      import_dialog->readOnly(),
      import_dialog->endLineChar(),import_dialog->getselectedColumnSeparator());
}

void ApplicationWindow::importASCII(const QStringList& files, int import_mode, const QString& local_column_separator,
    int local_ignored_lines, bool local_rename_columns, bool local_strip_spaces, bool local_simplify_spaces,
    bool local_import_comments, bool update_dec_separators, QLocale local_separators, const QString& local_comment_string,
    bool import_read_only, int endLineChar,const QString& sepforloadAscii)
{
  if (files.isEmpty())
    return;
  switch(import_mode) {
  case ImportASCIIDialog::NewTables:
  {
    int dx = 0, dy = 0;
    QStringList sorted_files = files;
    sorted_files.sort();
    int filesCount = sorted_files.size();
    for (int i=0; i<filesCount; i++){
      Table *w = newTable();
      if (!w)
        continue;

      w->importASCII(sorted_files[i], local_column_separator, local_ignored_lines,
          local_rename_columns, local_strip_spaces, local_simplify_spaces,
          local_import_comments, local_comment_string, import_read_only,
          Table::Overwrite, endLineChar);
      if (!w) continue;
      w->setWindowLabel(sorted_files[i]);
      w->setCaptionPolicy(MdiSubWindow::Both);
      if (i == 0){
        dx = w->verticalHeaderWidth();
        dy = w->frameGeometry().height() - w->widget()->height();
      }
      if (filesCount > 1)
        w->move(QPoint(i*dx, i*dy));

      if (update_dec_separators)
        w->updateDecimalSeparators(local_separators);
    }
    modifiedProject();
    break;
  }
  case ImportASCIIDialog::NewMatrices:
  {
    int dx = 0, dy = 0;
    QStringList sorted_files = files;
    sorted_files.sort();
    int filesCount = sorted_files.size();
    for (int i=0; i<filesCount; i++){
      Matrix *w = newMatrix();
      if (!w)
        continue;
      w->importASCII(sorted_files[i], local_column_separator, local_ignored_lines,
          local_strip_spaces, local_simplify_spaces, local_comment_string,
          Matrix::Overwrite, local_separators, endLineChar);
      w->setWindowLabel(sorted_files[i]);
      w->setCaptionPolicy(MdiSubWindow::Both);
      if (i == 0){
        dx = w->verticalHeaderWidth();
        dy = w->frameGeometry().height() - w->widget()->height();
      }
      if (filesCount > 1)
        w->move(QPoint(i*dx,i*dy));
    }
    modifiedProject();
    break;
  }

  case ImportASCIIDialog::NewColumns:
  case ImportASCIIDialog::NewRows:
  {
    MdiSubWindow *w = activeWindow();
    if (!w)
      return;

    if (w->inherits("Table")){
      Table *t = (Table*)w;
      for (int i=0; i<files.size(); i++)
        t->importASCII(files[i], local_column_separator, local_ignored_lines, local_rename_columns,
            local_strip_spaces, local_simplify_spaces, local_import_comments,
            local_comment_string, import_read_only, (Table::ImportMode)(import_mode - 2), endLineChar);

      if (update_dec_separators)
        t->updateDecimalSeparators(local_separators);
      t->notifyChanges();
      emit modifiedProject(t);
    } else if (w->isA("Matrix")){
      Matrix *m = (Matrix *)w;
      for (int i=0; i<files.size(); i++){
        m->importASCII(files[i], local_column_separator, local_ignored_lines,
            local_strip_spaces, local_simplify_spaces, local_comment_string,
            (Matrix::ImportMode)(import_mode - 2), local_separators, endLineChar);
      }
    }
    w->setWindowLabel(files.join("; "));
    w->setCaptionPolicy(MdiSubWindow::Name);
    break;
  }
  case ImportASCIIDialog::Overwrite:
  {
    MdiSubWindow *w = activeWindow();
    if (!w)
      return;

    if (w->inherits("Table")){
      Table *t = (Table *)w;
      t->importASCII(files[0], local_column_separator, local_ignored_lines, local_rename_columns,
          local_strip_spaces, local_simplify_spaces, local_import_comments,
          local_comment_string, import_read_only, Table::Overwrite, endLineChar);
      if (update_dec_separators)
        t->updateDecimalSeparators(local_separators);
      t->notifyChanges();
    } else if (w->isA("Matrix")){
      Matrix *m = (Matrix *)w;
      m->importASCII(files[0], local_column_separator, local_ignored_lines,
          local_strip_spaces, local_simplify_spaces, local_comment_string,
          Matrix::Overwrite, local_separators, endLineChar);
    }

    w->setWindowLabel(files[0]);
    w->setCaptionPolicy(MdiSubWindow::Both);
    modifiedProject();
    break;
  }
  case ImportASCIIDialog::NewWorkspace:
  {
    try
    {
      Mantid::API::IAlgorithm_sptr alg =mantidUI->CreateAlgorithm("LoadAscii");
      QStringList sorted_files = files;
      sorted_files.sort();
      for (int i=0; i<sorted_files.size(); i++){
        QStringList ws=sorted_files[i].split(".",QString::SkipEmptyParts);
        QString temp=ws[0];
        int index=temp.lastIndexOf("\\");
        if(index==-1) return;
        QString wsName=temp.right(temp.size()-(index+1));
        alg->setPropertyValue("Filename",sorted_files[i].toStdString());
        alg->setPropertyValue("OutputWorkspace",wsName.toStdString());
        std::string sep;
        alg->setPropertyValue("Separator",sepforloadAscii.toStdString());
        alg->execute();
      }

    }
    catch(...)
    {
      throw std::runtime_error("LoadAscii failed when importing the file as workspace");
    }
    break;
  }
  }
}

void ApplicationWindow::open()
{
  OpenProjectDialog *open_dialog = new OpenProjectDialog(this, d_extended_open_dialog);
  open_dialog->setDirectory(workingDir);
  if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
    return;
  workingDir = open_dialog->directory().path();

  switch(open_dialog->openMode()) {
  case OpenProjectDialog::NewProject:
  {
    QString fn = open_dialog->selectedFiles()[0];
    QFileInfo fi(fn);

    if (projectname != "untitled"){
      QFileInfo fi(projectname);
      QString pn = fi.absFilePath();
      if (fn == pn){
        QMessageBox::warning(this, tr("MantidPlot - File openning error"),//Mantid
            tr("The file: <b>%1</b> is the current file!").arg(fn));
        return;
      }
    }

    if (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
        fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogm",Qt::CaseInsensitive) ||
        fn.endsWith(".ogw",Qt::CaseInsensitive) || fn.endsWith(".ogg",Qt::CaseInsensitive) ||
        fn.endsWith(".qti.gz",Qt::CaseInsensitive)||fn.endsWith(".mantid",Qt::CaseInsensitive)||
        fn.endsWith(".mantid~",Qt::CaseInsensitive))
    {
      if (!fi.exists ()){
        QMessageBox::critical(this, tr("MantidPlot - File openning error"),//Mantid
            tr("The file: <b>%1</b> doesn't exist!").arg(fn));
        return;
      }

      saveSettings();//the recent projects must be saved

      ApplicationWindow *a = open (fn,false,false);
      if (a){
        a->workingDir = workingDir;
        if (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
            fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive) ||
            fn.endsWith(".qti.gz",Qt::CaseInsensitive))
        {// this->close();
        }
      }
    } else {
      QMessageBox::critical(this,tr("MantidPlot - File openning error"),//Mantid
          tr("The file: <b>%1</b> is not a MantidPlot or Origin project file!").arg(fn));
      return;
    }
    break;
  }
  case OpenProjectDialog::NewFolder:
    appendProject(open_dialog->selectedFiles()[0]);
    break;
  }
}

ApplicationWindow* ApplicationWindow::open(const QString& fn, bool factorySettings, bool newProject)
{
  if (fn.endsWith(".opj", Qt::CaseInsensitive) || fn.endsWith(".ogm", Qt::CaseInsensitive) ||
      fn.endsWith(".ogw", Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive))
    return importOPJ(fn, factorySettings, newProject);
  else if (fn.endsWith(".py", Qt::CaseInsensitive))
    return loadScript(fn);
  else if (!( fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti.gz",Qt::CaseInsensitive) ||
      fn.endsWith(".qti~",Qt::CaseInsensitive)||fn.endsWith(".mantid",Qt::CaseInsensitive)||fn.endsWith(".mantid~",Qt::CaseInsensitive)  ))
  {return plotFile(fn);
  }

  QString fname = fn;
  if (fn.endsWith(".qti.gz", Qt::CaseInsensitive)||fn.endsWith(".mantid.gz",Qt::CaseInsensitive)){//decompress using zlib
    file_uncompress((char *)fname.ascii());
    fname = fname.left(fname.size() - 3);
  }

  QFile f(fname);
  QTextStream t( &f );
  f.open(QIODevice::ReadOnly);
  QString s = t.readLine();
  QStringList list = s.split(QRegExp("\\s"), QString::SkipEmptyParts);
  if (list.count() < 2 || list[0] != "MantidPlot"){
    f.close();
    if (QFile::exists(fname + "~")){
      int choice = QMessageBox::question(this, tr("MantidPlot - File opening error"),//Mantid
          tr("The file <b>%1</b> is corrupted, but there exists a backup copy.<br>Do you want to open the backup instead?").arg(fn),
          QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape);
      if (choice == QMessageBox::Yes)
        return open(fname + "~");
      else
        QMessageBox::critical(this, tr("MantidPlot - File opening error"),  tr("The file: <b> %1 </b> was not created using MantidPlot!").arg(fn));//Mantid
      return 0;
    }
  }

  QStringList vl = list[1].split(".", QString::SkipEmptyParts);
  d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();
  ApplicationWindow* app = openProject(fname, factorySettings, newProject);
  f.close();
  return app;
}

void ApplicationWindow::openRecentProject(int index)
{
  QString fn = recent->text(index);
  int pos = fn.find(" ",0);
  fn = fn.right(fn.length()-pos-1);

  QFile f(fn);
  if (!f.exists()){
    QMessageBox::critical(this, tr("MantidPlot - File Open Error"),//Mantid
        tr("The file: <b> %1 </b> <p>does not exist anymore!"
            "<p>It will be removed from the list.").arg(fn));

    recentProjects.remove(fn);
    updateRecentProjectsList();
    return;
  }

  if (projectname != "untitled"){
    QFileInfo fi(projectname);
    QString pn = fi.absFilePath();
    if (fn == pn){
      QMessageBox::warning(this, tr("MantidPlot - File openning error"),//Mantid
          tr("The file: <p><b> %1 </b><p> is the current file!").arg(fn));
      return;
    }
  }

  if (!fn.isEmpty()){
    saveSettings();//the recent projects must be saved
    bool isSaved = saved;
    ApplicationWindow * a = open (fn,false,false);
    if (a && (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
        fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive)))
      if (isSaved)
        savedProject();//force saved state
    //close();
  }
}

ApplicationWindow* ApplicationWindow::openProject(const QString& fn, bool factorySettings, bool newProject)
{	
  ApplicationWindow *app = this;

  //if the current project is not saved prompt to save and close all windows opened
  mantidUI->saveProject(saved);
  if (newProject)
  { 	app = new ApplicationWindow(factorySettings);
  }
  // the matrix window list
  m_mantidmatrixWindows.clear();
  app->projectname = fn;
  app->d_file_version = d_file_version;
  app->setWindowTitle(tr("MantidPlot") + " - " + fn);
  app->d_opening_file = true;
  app->d_workspace->blockSignals(true);
  QFile f(fn);
  QTextStream t( &f );
  t.setEncoding(QTextStream::UnicodeUTF8);
  f.open(QIODevice::ReadOnly);

  QFileInfo fi(fn);
  QString baseName = fi.fileName();

  t.readLine();
  if (d_file_version < 73)
    t.readLine();
  QString s = t.readLine();
  QStringList list=s.split("\t", QString::SkipEmptyParts);
  if (list[0] == "<scripting-lang>")
  {
    if (!app->setScriptingLanguage(list[1]))
      QMessageBox::warning(app, tr("MantidPlot - File opening error"),//Mantid
          tr("The file \"%1\" was created using \"%2\" as scripting language.\n\n"\
              "Initializing support for this language FAILED; I'm using \"%3\" instead.\n"\
              "Various parts of this file may not be displayed as expected.")\
              .arg(fn).arg(list[1]).arg(scriptingEnv()->name()));

    s = t.readLine();
    list=s.split("\t", QString::SkipEmptyParts);
  }
  int aux=0,widgets=list[1].toInt();

  QString titleBase = tr("Window") + ": ";
  QString title = titleBase + "1/"+QString::number(widgets)+"  ";

  QProgressDialog progress(this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setRange(0, widgets);
  progress.setMinimumWidth(app->width()/2);
  progress.setWindowTitle(tr("MantidPlot - Opening file") + ": " + baseName);//Mantid
  progress.setLabelText(title);

  Folder *cf = app->projectFolder();
  app->folders->blockSignals (true);
  app->blockSignals (true);

  //rename project folder item
  FolderListItem *item = (FolderListItem *)app->folders->firstChild();
  item->setText(0, fi.baseName());
  item->folder()->setObjectName(fi.baseName());

  //process tables and matrix information
  while ( !t.atEnd() && !progress.wasCanceled()){
    s = t.readLine();
    list.clear();
    if  (s.left(8) == "<folder>"){
      list = s.split("\t");
      Folder *f = new Folder(app->current_folder, list[1]);
      f->setBirthDate(list[2]);
      f->setModificationDate(list[3]);
      if(list.count() > 4)
        if (list[4] == "current")
          cf = f;

      FolderListItem *fli = new FolderListItem(app->current_folder->folderListItem(), f);
      f->setFolderListItem(fli);

      app->current_folder = f;
    } else if  (s.contains("<open>")) {
      app->current_folder->folderListItem()->setOpen(s.remove("<open>").remove("</open>").toInt());
    } else if  (s == "<table>") {
      title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s!="</table>" ){
        s=t.readLine();
        lst<<s;
      }
      lst.pop_back();
      openTable(app,lst);
      progress.setValue(aux);
    } else if (s.left(17)=="<TableStatistics>") {
      QStringList lst;
      while ( s!="</TableStatistics>" ){
        s=t.readLine();
        lst<<s;
      }
      lst.pop_back();
      app->openTableStatistics(lst);
    } else if  (s == "<matrix>") {
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s != "</matrix>" ) {
        s=t.readLine();
        lst<<s;
      }
      lst.pop_back();
      openMatrix(app, lst);
      progress.setValue(aux);
    } else if  (s == "<note>") {
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      for (int i=0; i<3; i++){
        s = t.readLine();
        list << s;
      }
      Note* m = openNote(app,list);
      QStringList cont;
      while ( s != "</note>" ){
        s=t.readLine();
        cont << s;
      }
      cont.pop_back();
      m->restore(cont);
      progress.setValue(aux);
    } else if  (s == "</folder>") {
      Folder *parent = (Folder *)app->current_folder->parent();
      if (!parent)
        app->current_folder = app->projectFolder();
      else
        app->current_folder = parent;
    }else if(s=="<mantidmatrix>"){
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s != "</mantidmatrix>" ){
        s=t.readLine();
        lst<<s;
      }
      lst.pop_back();
      openMantidMatrix(lst);
      progress.setValue(aux);
    }
    else if(s=="<mantidworkspaces>"){
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s != "</mantidworkspaces>" ) {
        s=t.readLine();
        lst<<s;
      }
      lst.pop_back();
      s=lst[0];
      populateMantidTreeWdiget(s);
      progress.setValue(aux);
    }
    else if (s=="<scriptwindow>"){
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s != "</scriptwindow>" ) {
        s=t.readLine();
        lst<<s;
      }
      openScriptWindow(lst);
      progress.setValue(aux);
    }
    else if (s=="<instrumentwindow>"){
      title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      QStringList lst;
      while ( s != "</instrumentwindow>" ) {
        s=t.readLine();
        lst<<s;
      }
      openInstrumentWindow(lst);
      progress.setValue(aux);
    }
  }
  f.close();

  if (progress.wasCanceled()){
    app->saved = true;
    app->close();
    return 0;
  }

  //process the rest
  f.open(QIODevice::ReadOnly);
  MultiLayer *plot=0;
  while ( !t.atEnd() && !progress.wasCanceled()){
    s=t.readLine();
    if  (s.left(8) == "<folder>"){
      list = s.split("\t");
      app->current_folder = app->current_folder->findSubfolder(list[1]);
    } else if  (s == "<multiLayer>"){//process multilayers information
      title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);

      s=t.readLine();
      QStringList graph=s.split("\t");
      QString caption=graph[0];
      plot =multilayerPlot(caption, 0,  graph[2].toInt(), graph[1].toInt());
      app->setListViewDate(caption, graph[3]);
      plot->setBirthDate(graph[3]);

      restoreWindowGeometry(app, plot, t.readLine());
      plot->blockSignals(true);

      if (d_file_version > 71)
      {
        QStringList lst=t.readLine().split("\t");
        plot->setWindowLabel(lst[1]);
        plot->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
      }
      if (d_file_version > 83)
      {
        QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
        plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
        lst=t.readLine().split("\t", QString::SkipEmptyParts);
        plot->setSpacing(lst[1].toInt(),lst[2].toInt());
        lst=t.readLine().split("\t", QString::SkipEmptyParts);
        plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
        lst=t.readLine().split("\t", QString::SkipEmptyParts);
        plot->setAlignement(lst[1].toInt(),lst[2].toInt());
      }

      while ( s!="</multiLayer>" )
      {//open layers
        s = t.readLine();
        if (s.left(7)=="<graph>")
        {	list.clear();
        while ( s!="</graph>" )
        {
          s=t.readLine();
          list<<s;
        }
        openGraph(app, plot, list);
        }
      }
      if(plot) plot->blockSignals(false);
      progress.setValue(aux);
    }
    else if  (s == "<SurfacePlot>")
    {//process 3D plots information
      list.clear();
      title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
      progress.setLabelText(title);
      while ( s!="</SurfacePlot>" )
      {
        s=t.readLine();
        list<<s;
      }
      openSurfacePlot(app,list);
      progress.setValue(aux);
    }
    else if (s == "</folder>")
    {
      Folder *parent = (Folder *)app->current_folder->parent();
      if (!parent)
        app->current_folder = projectFolder();
      else
        app->current_folder = parent;
    }
    else if (s == "<log>")
    {//process analysis information
      s = t.readLine();
      QString log = s + "\n";
      while(s != "</log>"){
        s = t.readLine();
        log += s + "\n";
      }
      app->current_folder->appendLogInfo(log.remove("</log>"));
    }
  }
  f.close();

  if (progress.wasCanceled())
  {
    app->saved = true;
    app->close();
    return 0;
  }

  QFileInfo fi2(f);
  QString fileName = fi2.absFilePath();
  app->recentProjects.remove(fileName);
  app->recentProjects.push_front(fileName);
  app->updateRecentProjectsList();

  app->folders->setCurrentItem(cf->folderListItem());
  app->folders->blockSignals (false);
  //change folder to user defined current folder
  app->changeFolder(cf, true);

  app->blockSignals (false);
  app->renamedTables.clear();

  app->restoreApplicationGeometry();

  app->executeNotes();
  app->savedProject();
  app->d_opening_file = false;
  app->d_workspace->blockSignals(false);
  return app;
}

void ApplicationWindow::executeNotes()
{
  QList<MdiSubWindow *> lst = projectFolder()->windowsList();
  foreach(MdiSubWindow *widget, lst)
  if (widget->isA("Note") && ((Note*)widget)->autoexec())
    ((Note*)widget)->executeAll();
}

void ApplicationWindow::scriptPrint(const QString &msg, bool error, bool timestamp)
{
  if( error || msg.contains("error",Qt::CaseInsensitive) )
  {
    console->setTextColor(Qt::red);
    consoleWindow->show();
  }
  else
  {
    console->setTextColor(Qt::black);
  }
  QString msg_to_print = msg;

  if( error || timestamp )
  {
    if( timestamp )
    {
      QString separator(100, '-'); 
      msg_to_print  = separator + "\n" + QDateTime::currentDateTime().toString() 
	    + ": " + msg.trimmed() + "\n" + separator + '\n';
    }

    // Check for last character being a new line character unless we are at the start of the 
    // scroll area
    if( !console->text().endsWith('\n') && console->textCursor().position() != 0 )
    {
      console->textCursor().insertText("\n");    
    }
  }

  console->textCursor().insertText(msg_to_print);
  console->moveCursor(QTextCursor::End);
}

bool ApplicationWindow::setScriptingLanguage(const QString &lang)
{
  if ( lang.isEmpty() ) return false;
  if( scriptingEnv() && lang == scriptingEnv()->name() ) return true;

  if( m_bad_script_envs.contains(lang) ) 
  {
    QMessageBox::information(this, "MantidPlot", QString("Previous initialization of ") + lang + QString(" failed, cannot retry."));
    return false;
  }

  ScriptingEnv* newEnv(NULL);
  if( m_script_envs.contains(lang) )
  {
    newEnv = m_script_envs.value(lang);
  }
  else
  {
    newEnv = ScriptingLangManager::newEnv(lang, this);
    connect(newEnv, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));

    connect(mantidUI, SIGNAL(algorithmAboutToBeCreated()), newEnv, SLOT(refreshAlgorithms()));

    if( newEnv->initialize() )
    {   
      m_script_envs.insert(lang, newEnv);
    }
    else
    {
      delete newEnv;
      m_bad_script_envs.insert(lang);
      QMessageBox::information(this, "MantidPlot", QString("Failed to initialize ") + lang);
      return false;
    }
  }

  // notify everyone who might be interested
  ScriptingChangeEvent *sce = new ScriptingChangeEvent(newEnv);
  QApplication::sendEvent(this, sce);
  delete sce;

  foreach(QObject *i, findChildren<QObject*>())
  QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));

  if (scriptingWindow)
  {
    //Mantid - This is so that the title of the script window reflects the current scripting language
    QApplication::postEvent(scriptingWindow, new ScriptingChangeEvent(newEnv));

    foreach(QObject *i, scriptingWindow->findChildren<QObject*>())
    QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));
  }

  return true;
}

void ApplicationWindow::showScriptingLangDialog()
{
  // If a script is currently active, don't let a new one be selected
  if( scriptingEnv()->isRunning() )
  {
    QMessageBox msg_box;
    msg_box.setText("Cannot change scripting language, a script is still running.");
    msg_box.exec();
    return;
  }
  ScriptingLangDialog* d = new ScriptingLangDialog(scriptingEnv(), this);
  d->exec();
}

void ApplicationWindow::openTemplate()
{
  QString filter = "MantidPlot 2D Graph Template (*.qpt);;";
  filter += "MantidPlot 3D Surface Template (*.qst);;";
  filter += "MantidPlot Table Template (*.qtt);;";
  filter += "MantidPlot Matrix Template (*.qmt);;";

  QString fn = QFileDialog::getOpenFileName(this, tr("MantidPlot - Open Template File"), templatesDir, filter);//Mantid
  if (!fn.isEmpty()){
    QFileInfo fi(fn);
    templatesDir = fi.dirPath(true);
    if (fn.contains(".qmt") || fn.contains(".qpt") || fn.contains(".qtt") || fn.contains(".qst"))
      openTemplate(fn);
    else {
      QMessageBox::critical(this,tr("MantidPlot - File opening error"),//Mantid
          tr("The file: <b>%1</b> is not a MantidPlot template file!").arg(fn));
      return;
    }
  }
}

MdiSubWindow* ApplicationWindow::openTemplate(const QString& fn)
{
  if (fn.isEmpty() || !QFile::exists(fn)){
    QMessageBox::critical(this, tr("MantidPlot - File opening error"),//Mantid
        tr("The file: <b>%1</b> doesn't exist!").arg(fn));
    return 0;
  }

  QFile f(fn);
  QTextStream t(&f);
  t.setEncoding(QTextStream::UnicodeUTF8);
  f.open(QIODevice::ReadOnly);
  QStringList l=t.readLine().split(QRegExp("\\s"), QString::SkipEmptyParts);
  QString fileType=l[0];
  if (fileType != "MantidPlot"){
    QMessageBox::critical(this,tr("MantidPlot - File opening error"),//Mantid
        tr("The file: <b> %1 </b> was not created using MantidPlot!").arg(fn));
    return 0;
  }

  QStringList vl = l[1].split(".", QString::SkipEmptyParts);
  d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  MdiSubWindow *w = 0;
  QString templateType;
  t>>templateType;

  if (templateType == "<SurfacePlot>") {
    t.skipWhiteSpace();
    QStringList lst;
    while (!t.atEnd())
      lst << t.readLine();
    w = openSurfacePlot(this,lst);
    if (w)
      ((Graph3D *)w)->clearData();
  } else {
    int rows, cols;
    t>>rows; t>>cols;
    t.skipWhiteSpace();
    QString geometry = t.readLine();

    if (templateType == "<multiLayer>"){
      w = multilayerPlot(generateUniqueName(tr("Graph")));
      if (w){
        ((MultiLayer*)w)->setCols(cols);
        ((MultiLayer*)w)->setRows(rows);
        restoreWindowGeometry(this, w, geometry);
        if (d_file_version > 83){
          QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
          ((MultiLayer*)w)->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          ((MultiLayer*)w)->setSpacing(lst[1].toInt(),lst[2].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          ((MultiLayer*)w)->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          ((MultiLayer*)w)->setAlignement(lst[1].toInt(),lst[2].toInt());
        }
        while (!t.atEnd()){//open layers
          QString s=t.readLine();
          if (s.left(7)=="<graph>"){
            QStringList lst;
            while ( s!="</graph>" ){
              s = t.readLine();
              lst << s;
            }
            openGraph(this, (MultiLayer*)w, lst);
          }
        }
      }
    } else {
      if (templateType == "<table>")
        w = newTable(tr("Table1"), rows, cols);
      else if (templateType == "<matrix>")
        w = newMatrix(rows, cols);
      if (w){
        QStringList lst;
        while (!t.atEnd())
          lst << t.readLine();
        w->restore(lst);
        restoreWindowGeometry(this, w, geometry);
      }
    }
  }

  f.close();
  if (w){
    customMenu(w);
    customToolBars(w);
  }

  QApplication::restoreOverrideCursor();
  return w;
}

void ApplicationWindow::readSettings()
{
#ifdef Q_OS_MAC // Mac
  QSettings settings(QSettings::IniFormat,QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
  QSettings settings;
#endif

  /* ---------------- group General --------------- */
  settings.beginGroup("/General");
  settings.beginGroup("/ApplicationGeometry");//main window geometry
  d_app_rect = QRect(settings.value("/x", 0).toInt(), settings.value("/y", 0).toInt(),
      settings.value("/width", 0).toInt(), settings.value("/height", 0).toInt());
  settings.endGroup();

  autoSearchUpdates = settings.value("/AutoSearchUpdates", false).toBool();
  appLanguage = settings.value("/Language", QLocale::system().name().section('_',0,0)).toString();
  show_windows_policy = (ShowWindowsPolicy)settings.value("/ShowWindowsPolicy", ApplicationWindow::ActiveFolder).toInt();

  recentProjects = settings.value("/RecentProjects").toStringList();
  //Follows an ugly hack added by Ion in order to fix Qt4 porting issues
  //(only needed on Windows due to a Qt bug?)
#ifdef Q_OS_WIN
  if (!recentProjects.isEmpty() && recentProjects[0].contains("^e"))
    recentProjects = recentProjects[0].split("^e", QString::SkipEmptyParts);
  else if (recentProjects.count() == 1){
    QString s = recentProjects[0];
    if (s.remove(QRegExp("\\s")).isEmpty())
      recentProjects = QStringList();
  }
#endif

  updateRecentProjectsList();

  changeAppStyle(settings.value("/Style", appStyle).toString());
  autoSave = settings.value("/AutoSave",true).toBool();
  autoSaveTime = settings.value("/AutoSaveTime",15).toInt();
  d_backup_files = settings.value("/BackupProjects", true).toBool();
  d_init_window_type = (WindowType)settings.value("/InitWindow", NoWindow).toInt();
  defaultScriptingLang = settings.value("/ScriptingLang","Python").toString();    //Mantid M. Gigg
  d_thousands_sep = settings.value("/ThousandsSeparator", true).toBool();
  d_locale = QLocale(settings.value("/Locale", QLocale::system().name()).toString());
  if (!d_thousands_sep)
    d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

  d_decimal_digits = settings.value("/DecimalDigits", 13).toInt();
  d_matrix_undo_stack_size = settings.value("/MatrixUndoStackSize", 10).toInt();
  d_eol = (EndLineChar)settings.value("/EndOfLine", d_eol).toInt();

  //restore dock windows and tool bars
  restoreState(settings.value("/DockWindows").toByteArray());
  explorerSplitter->restoreState(settings.value("/ExplorerSplitter").toByteArray());
  QList<int> lst = explorerSplitter->sizes();
  for (int i=0; i< lst.count(); i++){
    if (lst[i] == 0){
      lst[i] = 45;
      explorerSplitter->setSizes(lst);
    }
  }

  QStringList applicationFont = settings.value("/Font").toStringList();
  if (applicationFont.size() == 4)
    appFont=QFont (applicationFont[0],applicationFont[1].toInt(),applicationFont[2].toInt(),applicationFont[3].toInt());

  settings.beginGroup("/Dialogs");
  d_extended_open_dialog = settings.value("/ExtendedOpenDialog", true).toBool();
  d_extended_export_dialog = settings.value("/ExtendedExportDialog", true).toBool();
  d_extended_import_ASCII_dialog = settings.value("/ExtendedImportAsciiDialog", true).toBool();
  d_extended_plot_dialog = settings.value("/ExtendedPlotDialog", true).toBool();//used by PlotDialog

  settings.beginGroup("/AddRemoveCurves");
  d_add_curves_dialog_size = QSize(settings.value("/Width", 700).toInt(),
      settings.value("/Height", 400).toInt());
  d_show_current_folder = settings.value("/ShowCurrentFolder", false).toBool();
  settings.endGroup(); // AddRemoveCurves Dialog
  settings.endGroup(); // Dialogs

  settings.beginGroup("/Colors");
  workspaceColor = QColor(settings.value("/Workspace","darkGray").value<QColor>());
  // see http://doc.trolltech.com/4.2/qvariant.html for instructions on qcolor <-> qvariant conversion
  panelsColor = QColor(settings.value("/Panels","#ffffff").value<QColor>());
  panelsTextColor = QColor(settings.value("/PanelsText","#000000").value<QColor>());
  settings.endGroup(); // Colors

  settings.beginGroup("/Paths");
  QString appPath = qApp->applicationDirPath();
  workingDir = settings.value("/WorkingDir", appPath).toString();
#ifdef Q_OS_WIN
  fitPluginsPath = settings.value("/FitPlugins", "fitPlugins").toString();
  templatesDir = settings.value("/TemplatesDir", appPath).toString();
  asciiDirPath = settings.value("/ASCII", appPath).toString();
  imagesDirPath = settings.value("/Images", appPath).toString();
#else
  fitPluginsPath = settings.value("/FitPlugins", "/usr/lib/MantidPlot/plugins").toString();
  templatesDir = settings.value("/TemplatesDir", QDir::homePath()).toString();
  asciiDirPath = settings.value("/ASCII", QDir::homePath()).toString();
  imagesDirPath = settings.value("/Images", QDir::homePath()).toString();
  workingDir = settings.value("/WorkingDir", QDir::homePath()).toString();
#endif
  scriptsDirPath = settings.value("/ScriptsDir", appPath).toString();
  fitModelsPath = settings.value("/FitModelsDir", "").toString();
  customActionsDirPath = settings.value("/CustomActionsDir", "").toString();
  helpFilePath = settings.value("/HelpFile", helpFilePath).toString();
  d_translations_folder = settings.value("/Translations", d_translations_folder).toString();
  d_python_config_folder = settings.value("/PythonConfigDir", d_python_config_folder).toString();
  settings.endGroup(); // Paths
  settings.endGroup();
  /* ------------- end group General ------------------- */

  settings.beginGroup("/UserFunctions");
  if (100*maj_version + 10*min_version + patch_version == 91 &&
      settings.contains("/FitFunctions")){
    saveFitFunctions(settings.value("/FitFunctions").toStringList());
    settings.remove("/FitFunctions");
  }
  surfaceFunc = settings.value("/SurfaceFunctions").toStringList();
  xFunctions = settings.value("/xFunctions").toStringList();
  yFunctions = settings.value("/yFunctions").toStringList();
  rFunctions = settings.value("/rFunctions").toStringList();
  thetaFunctions = settings.value("/thetaFunctions").toStringList();
  d_param_surface_func = settings.value("/ParametricSurfaces").toStringList();
  settings.endGroup(); // UserFunctions

  settings.beginGroup("/Confirmations");
  confirmCloseFolder = settings.value("/Folder", true).toBool();
  confirmCloseTable = settings.value("/Table", true).toBool();
  confirmCloseMatrix = settings.value("/Matrix", true).toBool();
  confirmClosePlot2D = settings.value("/Plot2D", true).toBool();
  confirmClosePlot3D = settings.value("/Plot3D", true).toBool();
  confirmCloseNotes = settings.value("/Note", true).toBool();
  d_inform_rename_table = settings.value("/RenameTable", true).toBool();
  confirmCloseInstrWindow=settings.value("/InstrumentWindow", false).toBool();
  settings.endGroup(); // Confirmations


  /* ---------------- group Tables --------------- */
  settings.beginGroup("/Tables");
  d_show_table_comments = settings.value("/DisplayComments", false).toBool();
  d_auto_update_table_values = settings.value("/AutoUpdateValues", true).toBool();

  QStringList tableFonts = settings.value("/Fonts").toStringList();
  if (tableFonts.size() == 8)
  {
    tableTextFont=QFont (tableFonts[0],tableFonts[1].toInt(),tableFonts[2].toInt(),tableFonts[3].toInt());
    tableHeaderFont=QFont (tableFonts[4],tableFonts[5].toInt(),tableFonts[6].toInt(),tableFonts[7].toInt());
  }

  settings.beginGroup("/Colors");
  tableBkgdColor = QColor(settings.value("/Background","#ffffff").value<QColor>());
  tableTextColor = QColor(settings.value("/Text","#000000").value<QColor>());
  tableHeaderColor = QColor(settings.value("/Header","#000000").value<QColor>());
  settings.endGroup(); // Colors
  settings.endGroup();
  /* --------------- end group Tables ------------------------ */

  /* --------------- group 2D Plots ----------------------------- */
  settings.beginGroup("/2DPlots");
  settings.beginGroup("/General");
  titleOn = settings.value("/Title", true).toBool();
  allAxesOn = settings.value("/AllAxes", false).toBool();
  canvasFrameWidth = settings.value("/CanvasFrameWidth", 0).toInt();
  defaultPlotMargin = settings.value("/Margin", 0).toInt();
  drawBackbones = settings.value("/AxesBackbones", true).toBool();
  xaxisScale = settings.value("/AxisXScale", "linear").toString();
  yaxisScale = settings.value("/AxisYScale", "linear").toString();
  zaxisScale = settings.value("/AxisZScale", "linear").toString();
  axesLineWidth = settings.value("/AxesLineWidth", 1).toInt();
  autoscale2DPlots = settings.value("/Autoscale", true).toBool();
  autoScaleFonts = settings.value("/AutoScaleFonts", true).toBool();
  autoResizeLayers = settings.value("/AutoResizeLayers", true).toBool();
  antialiasing2DPlots = settings.value("/Antialiasing", false).toBool(); //Mantid
  d_scale_plots_on_print = settings.value("/ScaleLayersOnPrint", false).toBool();
  d_print_cropmarks = settings.value("/PrintCropmarks", false).toBool();

  QStringList graphFonts = settings.value("/Fonts").toStringList();
  if (graphFonts.size() == 16) {
    plotAxesFont=QFont (graphFonts[0],graphFonts[1].toInt(),graphFonts[2].toInt(),graphFonts[3].toInt());
    plotNumbersFont=QFont (graphFonts[4],graphFonts[5].toInt(),graphFonts[6].toInt(),graphFonts[7].toInt());
    plotLegendFont=QFont (graphFonts[8],graphFonts[9].toInt(),graphFonts[10].toInt(),graphFonts[11].toInt());
    plotTitleFont=QFont (graphFonts[12],graphFonts[13].toInt(),graphFonts[14].toInt(),graphFonts[15].toInt());
  }
  d_in_place_editing = settings.value("/InPlaceEditing", true).toBool();
  settings.endGroup(); // General

  settings.beginGroup("/Curves");
  defaultCurveStyle = settings.value("/Style", Graph::LineSymbols).toInt();
  defaultCurveLineWidth = settings.value("/LineWidth", 1).toDouble();
  defaultSymbolSize = settings.value("/SymbolSize", 7).toInt();
  applyCurveStyleToMantid = settings.value("/ApplyMantid", true).toBool();
  settings.endGroup(); // Curves

  settings.beginGroup("/Ticks");
  majTicksStyle = settings.value("/MajTicksStyle", ScaleDraw::Out).toInt();
  minTicksStyle = settings.value("/MinTicksStyle", ScaleDraw::Out).toInt();
  minTicksLength = settings.value("/MinTicksLength", 5).toInt();
  majTicksLength = settings.value("/MajTicksLength", 9).toInt();
  settings.endGroup(); // Ticks

  settings.beginGroup("/Legend");
  legendFrameStyle = settings.value("/FrameStyle", LegendWidget::Line).toInt();
  legendTextColor = QColor(settings.value("/TextColor", "#000000").value<QColor>()); //default color Qt::black
  legendBackground = QColor(settings.value("/BackgroundColor", "#ffffff").value<QColor>()); //default color Qt::white
  legendBackground.setAlpha(settings.value("/Transparency", 0).toInt()); // transparent by default;
  settings.endGroup(); // Legend

  settings.beginGroup("/Arrows");
  defaultArrowLineWidth = settings.value("/Width", 1).toDouble();
  defaultArrowColor = QColor(settings.value("/Color", "#000000").value<QColor>());//default color Qt::black
  defaultArrowHeadLength = settings.value("/HeadLength", 4).toInt();
  defaultArrowHeadAngle = settings.value("/HeadAngle", 45).toInt();
  defaultArrowHeadFill = settings.value("/HeadFill", true).toBool();
  defaultArrowLineStyle = Graph::getPenStyle(settings.value("/LineStyle", "SolidLine").toString());
  settings.endGroup(); // Arrows
  settings.endGroup();
  /* ----------------- end group 2D Plots --------------------------- */

  /* ----------------- group 3D Plots --------------------------- */
  settings.beginGroup("/3DPlots");
  showPlot3DLegend = settings.value("/Legend",true).toBool();
  showPlot3DProjection = settings.value("/Projection", false).toBool();
  smooth3DMesh = settings.value("/Antialiasing", false).toBool(); //Mantid
  plot3DResolution = settings.value ("/Resolution", 1).toInt();
  orthogonal3DPlots = settings.value("/Orthogonal", false).toBool();
  autoscale3DPlots = settings.value ("/Autoscale", true).toBool();

  QStringList plot3DFonts = settings.value("/Fonts").toStringList();
  if (plot3DFonts.size() == 12){
    plot3DTitleFont=QFont (plot3DFonts[0],plot3DFonts[1].toInt(),plot3DFonts[2].toInt(),plot3DFonts[3].toInt());
    plot3DNumbersFont=QFont (plot3DFonts[4],plot3DFonts[5].toInt(),plot3DFonts[6].toInt(),plot3DFonts[7].toInt());
    plot3DAxesFont=QFont (plot3DFonts[8],plot3DFonts[9].toInt(),plot3DFonts[10].toInt(),plot3DFonts[11].toInt());
  }

  settings.beginGroup("/Colors");
  plot3DColors = QStringList();
  plot3DColors << QColor(settings.value("/MaxData", "blue").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Labels", "#000000").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Mesh", "#000000").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Grid", "#000000").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/MinData", "red").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Numbers", "#000000").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Axes", "#000000").value<QColor>()).name();
  plot3DColors << QColor(settings.value("/Background", "#ffffff").value<QColor>()).name();
  settings.endGroup(); // Colors
  settings.endGroup();
  /* ----------------- end group 3D Plots --------------------------- */

  settings.beginGroup("/Fitting");
  fit_output_precision = settings.value("/OutputPrecision", 15).toInt();
  pasteFitResultsToPlot = settings.value("/PasteResultsToPlot", false).toBool();
  writeFitResultsToLog = settings.value("/WriteResultsToLog", true).toBool();
  generateUniformFitPoints = settings.value("/GenerateFunction", true).toBool();
  fitPoints = settings.value("/Points", 100).toInt();
  generatePeakCurves = settings.value("/GeneratePeakCurves", true).toBool();
  peakCurvesColor = settings.value("/PeaksColor", 2).toInt();//green color
  fit_scale_errors = settings.value("/ScaleErrors", true).toBool();
  d_2_linear_fit_points = settings.value("/TwoPointsLinearFit", true).toBool();
  settings.endGroup(); // Fitting

  settings.beginGroup("/ImportASCII");
  columnSeparator = settings.value("/ColumnSeparator", "\\t").toString();
  columnSeparator.replace("\\t", "\t").replace("\\s", " ");
  ignoredLines = settings.value("/IgnoreLines", 0).toInt();
  renameColumns = settings.value("/RenameColumns", true).toBool();
  strip_spaces = settings.value("/StripSpaces", false).toBool();
  simplify_spaces = settings.value("/SimplifySpaces", false).toBool();
  d_ASCII_file_filter = settings.value("/AsciiFileTypeFilter", "*").toString();
  d_ASCII_import_locale = settings.value("/AsciiImportLocale", QLocale::system().name()).toString();
  d_import_dec_separators = settings.value("/UpdateDecSeparators", true).toBool();
  d_ASCII_import_mode = settings.value("/ImportMode", ImportASCIIDialog::NewTables).toInt();
  d_ASCII_comment_string = settings.value("/CommentString", "#").toString();
  d_ASCII_import_comments = settings.value("/ImportComments", false).toBool();
  d_ASCII_import_read_only = settings.value("/ImportReadOnly", false).toBool();
  d_ASCII_import_preview = settings.value("/Preview", true).toBool();
  d_preview_lines = settings.value("/PreviewLines", 100).toInt();
  d_ASCII_end_line = (EndLineChar)settings.value("/EndLineCharacter", d_ASCII_end_line).toInt();
  settings.endGroup(); // Import ASCII

  settings.beginGroup("/ExportASCII");
  d_export_col_separator = settings.value("/ColumnSeparator", "\\t").toString();
  d_export_col_separator.replace("\\t", "\t").replace("\\s", " ");
  d_export_col_names = settings.value("/ExportLabels", false).toBool();
  d_export_col_comment = settings.value("/ExportComments", false).toBool();

  d_export_table_selection = settings.value("/ExportSelection", false).toBool();
  settings.endGroup(); // ExportASCII

  settings.beginGroup("/ExportImage");
  d_image_export_filter = settings.value("/ImageFileTypeFilter", ".png").toString();
  d_export_transparency = settings.value("/ExportTransparency", false).toBool();
  d_export_quality = settings.value("/ImageQuality", 100).toInt();
  //	d_export_resolution = settings.value("/Resolution", QPrinter().resolution()).toInt();
  d_export_color = settings.value("/ExportColor", true).toBool();
  d_export_vector_size = settings.value("/ExportPageSize", QPrinter::Custom).toInt();
  d_keep_plot_aspect = settings.value("/KeepAspect", true).toBool();
  settings.endGroup(); // ExportImage

  settings.beginGroup("/ScriptWindow");
  d_script_win_on_top = settings.value("/AlwaysOnTop", false).toBool();  //M. Gigg, Mantid
  d_script_win_rect = QRect(settings.value("/x", 100).toInt(), settings.value("/y", 50).toInt(),
      settings.value("/width", 600).toInt(), settings.value("/height", 660).toInt());
  d_script_win_arrow = settings.value("/ProgressArrow", true).toBool();  // Mantid - restore progress arrow state
  settings.endGroup();

  settings.beginGroup("/ToolBars");
  d_file_tool_bar = settings.value("/FileToolBar", true).toBool();
  d_edit_tool_bar = settings.value("/EditToolBar", true).toBool();
  d_table_tool_bar = settings.value("/TableToolBar", true).toBool();
  d_column_tool_bar = settings.value("/ColumnToolBar", true).toBool();
  d_matrix_tool_bar = settings.value("/MatrixToolBar", true).toBool();
  d_plot_tool_bar = settings.value("/PlotToolBar", true).toBool();
  d_plot3D_tool_bar = settings.value("/Plot3DToolBar", true).toBool();
  d_display_tool_bar = settings.value("/DisplayToolBar", false).toBool();
  d_format_tool_bar = settings.value("/FormatToolBar", true).toBool();
  settings.endGroup();

  //---------------------------------
  // Mantid

  bool warning_shown = settings.value("/DuplicationDialogShown", false).toBool();

  //Check for user defined scripts in settings and create menus for them
  //Top level scripts group
  settings.beginGroup("CustomScripts");

  // Reference list of custom Interfaces that will be added to the Interfaces menu
  QStringList user_windows = MantidQt::API::InterfaceManager::Instance().getUserSubWindowKeys();
  // List it user items that will be moved to the Interfaces menu
  QStringList duplicated_custom_menu = QStringList();

  foreach(QString menu, settings.childGroups())
  {
    addUserMenu(menu);
    settings.beginGroup(menu);
    foreach(QString keyName, settings.childKeys())
    {
      if ( menu.contains("Interfaces")==0 &&
          (user_windows.grep(keyName).size() > 0 || pyqt_interfaces.grep(keyName).size() > 0) )
      {
        duplicated_custom_menu.append(menu+"/"+keyName);
      }
      if ( QFileInfo(settings.value(keyName).toString()).exists() )
        addUserMenuAction(menu, keyName, settings.value(keyName).toString());
    }
    settings.endGroup();
  }

  // Mantid - Remember which interfaces the user explicitely removed
  // from the Interfaces menu
  removed_interfaces = settings.value("RemovedInterfaces").toStringList();

  settings.endGroup();

  if (duplicated_custom_menu.size() > 0 && !warning_shown)
  {
    QString mess = "The following menus are now part of the Interfaces menu:\n\n";
    mess += duplicated_custom_menu.join("\n");
    mess += "\n\nYou may consider removing them from your custom menus.";
    //FIXME: A nice alternative to showing a message in the log window would
    // be to pop up a message box. This should be done AFTER MantidPlot has started.
    //QMessageBox::warning(this, tr("MantidPlot - Menu Warning"), tr(mess.ascii()));
    Mantid::Kernel::Logger& g_log = Mantid::Kernel::Logger::get("ConfigService");
    g_log.warning() << mess.ascii() << "\n";
    settings.setValue("/DuplicationDialogShown", true);
  }

}

void ApplicationWindow::saveSettings()
{

#ifdef Q_OS_MAC // Mac
  QSettings settings(QSettings::IniFormat,QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
  QSettings settings;//(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "MantidPlot");
#endif

  /* ---------------- group General --------------- */
  settings.beginGroup("/General");

  settings.beginGroup("/ApplicationGeometry");
  d_app_rect = QRect(this->pos(), this->size());
  if (this->isMaximized())
    d_app_rect = QRect();

  settings.setValue("/x", d_app_rect.x());
  settings.setValue("/y", d_app_rect.y());
  settings.setValue("/width", d_app_rect.width());
  settings.setValue("/height", d_app_rect.height());
  settings.endGroup();

  settings.setValue("/AutoSearchUpdates", autoSearchUpdates);
  settings.setValue("/Language", appLanguage);
  settings.setValue("/ShowWindowsPolicy", show_windows_policy);
  settings.setValue("/RecentProjects", recentProjects);
  settings.setValue("/Style", appStyle);
  settings.setValue("/AutoSave", autoSave);
  settings.setValue("/AutoSaveTime", autoSaveTime);
  settings.setValue("/BackupProjects", d_backup_files);
  settings.setValue("/InitWindow", int(d_init_window_type));

  settings.setValue("/ScriptingLang", defaultScriptingLang);
  settings.setValue("/ThousandsSeparator", d_thousands_sep);
  settings.setValue("/Locale", d_locale.name());
  settings.setValue("/DecimalDigits", d_decimal_digits);
  settings.setValue("/MatrixUndoStackSize", d_matrix_undo_stack_size);
  settings.setValue("/EndOfLine", (int)d_eol);
  settings.setValue("/DockWindows", saveState());
  settings.setValue("/ExplorerSplitter", explorerSplitter->saveState());

  QStringList applicationFont;
  applicationFont<<appFont.family();
  applicationFont<<QString::number(appFont.pointSize());
  applicationFont<<QString::number(appFont.weight());
  applicationFont<<QString::number(appFont.italic());
  settings.setValue("/Font", applicationFont);

  settings.beginGroup("/Dialogs");
  settings.setValue("/ExtendedOpenDialog", d_extended_open_dialog);
  settings.setValue("/ExtendedExportDialog", d_extended_export_dialog);
  settings.setValue("/ExtendedImportAsciiDialog", d_extended_import_ASCII_dialog);
  settings.setValue("/ExtendedPlotDialog", d_extended_plot_dialog);
  settings.beginGroup("/AddRemoveCurves");
  settings.setValue("/Width", d_add_curves_dialog_size.width());
  settings.setValue("/Height", d_add_curves_dialog_size.height());
  settings.setValue("/ShowCurrentFolder", d_show_current_folder);
  settings.endGroup(); // AddRemoveCurves Dialog
  settings.endGroup(); // Dialogs

  settings.beginGroup("/Colors");
  settings.setValue("/Workspace", workspaceColor.name());
  settings.setValue("/Panels", panelsColor.name());
  settings.setValue("/PanelsText", panelsTextColor.name());
  settings.endGroup(); // Colors

  settings.beginGroup("/Paths");
  settings.setValue("/WorkingDir", workingDir);
  settings.setValue("/TemplatesDir", templatesDir);
  settings.setValue("/HelpFile", helpFilePath);
  settings.setValue("/FitPlugins", fitPluginsPath);
  settings.setValue("/ASCII", asciiDirPath);
  settings.setValue("/Images", imagesDirPath);
  settings.setValue("/ScriptsDir", scriptsDirPath);
  settings.setValue("/FitModelsDir", fitModelsPath);
  settings.setValue("/CustomActionsDir", customActionsDirPath);
  settings.setValue("/Translations", d_translations_folder);
  settings.setValue("/PythonConfigDir", d_python_config_folder);
  settings.endGroup(); // Paths
  settings.endGroup();
  /* ---------------- end group General --------------- */

  settings.beginGroup("/UserFunctions");
  settings.setValue("/SurfaceFunctions", surfaceFunc);
  settings.setValue("/xFunctions", xFunctions);
  settings.setValue("/yFunctions", yFunctions);
  settings.setValue("/rFunctions", rFunctions);
  settings.setValue("/thetaFunctions", thetaFunctions);
  settings.setValue("/ParametricSurfaces", d_param_surface_func);
  settings.endGroup(); // UserFunctions

  settings.beginGroup("/Confirmations");
  settings.setValue("/Folder", confirmCloseFolder);
  settings.setValue("/Table", confirmCloseTable);
  settings.setValue("/Matrix", confirmCloseMatrix);
  settings.setValue("/Plot2D", confirmClosePlot2D);
  settings.setValue("/Plot3D", confirmClosePlot3D);
  settings.setValue("/Note", confirmCloseNotes);
  settings.setValue("/RenameTable", d_inform_rename_table);
  settings.value("/InstrumentWindow", confirmCloseInstrWindow).toBool();
  settings.endGroup(); // Confirmations

  /* ----------------- group Tables -------------- */
  settings.beginGroup("/Tables");
  settings.setValue("/DisplayComments", d_show_table_comments);
  settings.setValue("/AutoUpdateValues", d_auto_update_table_values);
  QStringList tableFonts;
  tableFonts<<tableTextFont.family();
  tableFonts<<QString::number(tableTextFont.pointSize());
  tableFonts<<QString::number(tableTextFont.weight());
  tableFonts<<QString::number(tableTextFont.italic());
  tableFonts<<tableHeaderFont.family();
  tableFonts<<QString::number(tableHeaderFont.pointSize());
  tableFonts<<QString::number(tableHeaderFont.weight());
  tableFonts<<QString::number(tableHeaderFont.italic());
  settings.setValue("/Fonts", tableFonts);

  settings.beginGroup("/Colors");
  settings.setValue("/Background", tableBkgdColor.name());
  settings.setValue("/Text", tableTextColor.name());
  settings.setValue("/Header", tableHeaderColor.name());
  settings.endGroup(); // Colors
  settings.endGroup();
  /* ----------------- end group Tables ---------- */

  /* ----------------- group 2D Plots ------------ */
  settings.beginGroup("/2DPlots");
  settings.beginGroup("/General");
  settings.setValue("/Title", titleOn);
  settings.setValue("/AllAxes", allAxesOn);
  settings.setValue("/CanvasFrameWidth", canvasFrameWidth);
  settings.setValue("/Margin", defaultPlotMargin);
  settings.setValue("/AxesBackbones", drawBackbones);
  settings.setValue("/AxisXScale", xaxisScale);
  settings.setValue("/AxisYScale", yaxisScale);
  settings.setValue("/AxisZScale", zaxisScale);
  settings.setValue("/AxesLineWidth", axesLineWidth);
  settings.setValue("/Autoscale", autoscale2DPlots);
  settings.setValue("/AutoScaleFonts", autoScaleFonts);
  settings.setValue("/AutoResizeLayers", autoResizeLayers);
  settings.setValue("/Antialiasing", antialiasing2DPlots);
  settings.setValue("/ScaleLayersOnPrint", d_scale_plots_on_print);
  settings.setValue("/PrintCropmarks", d_print_cropmarks);

  QStringList graphFonts;
  graphFonts<<plotAxesFont.family();
  graphFonts<<QString::number(plotAxesFont.pointSize());
  graphFonts<<QString::number(plotAxesFont.weight());
  graphFonts<<QString::number(plotAxesFont.italic());
  graphFonts<<plotNumbersFont.family();
  graphFonts<<QString::number(plotNumbersFont.pointSize());
  graphFonts<<QString::number(plotNumbersFont.weight());
  graphFonts<<QString::number(plotNumbersFont.italic());
  graphFonts<<plotLegendFont.family();
  graphFonts<<QString::number(plotLegendFont.pointSize());
  graphFonts<<QString::number(plotLegendFont.weight());
  graphFonts<<QString::number(plotLegendFont.italic());
  graphFonts<<plotTitleFont.family();
  graphFonts<<QString::number(plotTitleFont.pointSize());
  graphFonts<<QString::number(plotTitleFont.weight());
  graphFonts<<QString::number(plotTitleFont.italic());
  settings.setValue("/Fonts", graphFonts);


  settings.setValue("/InPlaceEditing", d_in_place_editing);
  settings.endGroup(); // General

  settings.beginGroup("/Curves");
  settings.setValue("/Style", defaultCurveStyle);
  settings.setValue("/LineWidth", defaultCurveLineWidth);
  settings.setValue("/SymbolSize", defaultSymbolSize);
  settings.setValue("/ApplyMantid", applyCurveStyleToMantid);
  settings.endGroup(); // Curves

  settings.beginGroup("/Ticks");
  settings.setValue ("/MajTicksStyle", majTicksStyle);
  settings.setValue ("/MinTicksStyle", minTicksStyle);
  settings.setValue("/MinTicksLength", minTicksLength);
  settings.setValue("/MajTicksLength", majTicksLength);
  settings.endGroup(); // Ticks

  settings.beginGroup("/Legend");
  settings.setValue("/FrameStyle", legendFrameStyle);
  settings.setValue("/TextColor", legendTextColor.name());
  settings.setValue("/BackgroundColor", legendBackground.name());
  settings.setValue("/Transparency", legendBackground.alpha());
  settings.endGroup(); // Legend

  settings.beginGroup("/Arrows");
  settings.setValue("/Width", defaultArrowLineWidth);
  settings.setValue("/Color", defaultArrowColor.name());
  settings.setValue("/HeadLength", defaultArrowHeadLength);
  settings.setValue("/HeadAngle", defaultArrowHeadAngle);
  settings.setValue("/HeadFill", defaultArrowHeadFill);
  settings.setValue("/LineStyle", Graph::penStyleName(defaultArrowLineStyle));
  settings.endGroup(); // Arrows
  settings.endGroup();
  /* ----------------- end group 2D Plots -------- */

  /* ----------------- group 3D Plots ------------ */
  settings.beginGroup("/3DPlots");
  settings.setValue("/Legend", showPlot3DLegend);
  settings.setValue("/Projection", showPlot3DProjection);
  settings.setValue("/Antialiasing", smooth3DMesh);
  settings.setValue("/Resolution", plot3DResolution);
  settings.setValue("/Orthogonal", orthogonal3DPlots);
  settings.setValue("/Autoscale", autoscale3DPlots);

  QStringList plot3DFonts;
  plot3DFonts<<plot3DTitleFont.family();
  plot3DFonts<<QString::number(plot3DTitleFont.pointSize());
  plot3DFonts<<QString::number(plot3DTitleFont.weight());
  plot3DFonts<<QString::number(plot3DTitleFont.italic());
  plot3DFonts<<plot3DNumbersFont.family();
  plot3DFonts<<QString::number(plot3DNumbersFont.pointSize());
  plot3DFonts<<QString::number(plot3DNumbersFont.weight());
  plot3DFonts<<QString::number(plot3DNumbersFont.italic());
  plot3DFonts<<plot3DAxesFont.family();
  plot3DFonts<<QString::number(plot3DAxesFont.pointSize());
  plot3DFonts<<QString::number(plot3DAxesFont.weight());
  plot3DFonts<<QString::number(plot3DAxesFont.italic());
  settings.setValue("/Fonts", plot3DFonts);

  settings.beginGroup("/Colors");
  settings.setValue("/MaxData", plot3DColors[0]);
  settings.setValue("/Labels", plot3DColors[1]);
  settings.setValue("/Mesh", plot3DColors[2]);
  settings.setValue("/Grid", plot3DColors[3]);
  settings.setValue("/MinData", plot3DColors[4]);
  settings.setValue("/Numbers", plot3DColors[5]);
  settings.setValue("/Axes", plot3DColors[6]);
  settings.setValue("/Background", plot3DColors[7]);
  settings.endGroup(); // Colors
  settings.endGroup();
  /* ----------------- end group 2D Plots -------- */

  settings.beginGroup("/Fitting");
  settings.setValue("/OutputPrecision", fit_output_precision);
  settings.setValue("/PasteResultsToPlot", pasteFitResultsToPlot);
  settings.setValue("/WriteResultsToLog", writeFitResultsToLog);
  settings.setValue("/GenerateFunction", generateUniformFitPoints);
  settings.setValue("/Points", fitPoints);
  settings.setValue("/GeneratePeakCurves", generatePeakCurves);
  settings.setValue("/PeaksColor", peakCurvesColor);
  settings.setValue("/ScaleErrors", fit_scale_errors);
  settings.setValue("/TwoPointsLinearFit", d_2_linear_fit_points);
  settings.endGroup(); // Fitting

  settings.beginGroup("/ImportASCII");
  QString sep = columnSeparator;
  settings.setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
  settings.setValue("/IgnoreLines", ignoredLines);
  settings.setValue("/RenameColumns", renameColumns);
  settings.setValue("/StripSpaces", strip_spaces);
  settings.setValue("/SimplifySpaces", simplify_spaces);
  settings.setValue("/AsciiFileTypeFilter", d_ASCII_file_filter);
  settings.setValue("/AsciiImportLocale", d_ASCII_import_locale.name());
  settings.setValue("/UpdateDecSeparators", d_import_dec_separators);
  settings.setValue("/ImportMode", d_ASCII_import_mode);
  settings.setValue("/CommentString", d_ASCII_comment_string);
  settings.setValue("/ImportComments", d_ASCII_import_comments);
  settings.setValue("/ImportReadOnly", d_ASCII_import_read_only);
  settings.setValue("/Preview", d_ASCII_import_preview);
  settings.setValue("/PreviewLines", d_preview_lines);
  settings.setValue("/EndLineCharacter", (int)d_ASCII_end_line);
  settings.endGroup(); // ImportASCII

  settings.beginGroup("/ExportASCII");
  sep = d_export_col_separator;
  settings.setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
  settings.setValue("/ExportLabels", d_export_col_names);
  settings.setValue("/ExportComments", d_export_col_comment);
  settings.setValue("/ExportSelection", d_export_table_selection);
  settings.endGroup(); // ExportASCII

  settings.beginGroup("/ExportImage");
  settings.setValue("/ImageFileTypeFilter", d_image_export_filter);
  settings.setValue("/ExportTransparency", d_export_transparency);
  settings.setValue("/ImageQuality", d_export_quality);
  settings.setValue("/Resolution", d_export_resolution);
  settings.setValue("/ExportColor", d_export_color);
  settings.setValue("/ExportPageSize", d_export_vector_size);
  settings.setValue("/KeepAspect", d_keep_plot_aspect);
  settings.endGroup(); // ExportImage

  if(m_scriptInterpreter)
  {
    m_scriptInterpreter->saveSettings();
    m_interpreterDock->hide();
  }

  if( scriptingWindow )
  {
    scriptingWindow->raise();//Mantid
    scriptingWindow->show();//Mantid
    scriptingWindow->saveSettings(); //Mantid
    scriptingWindow->hide();
  }


  settings.beginGroup("/ToolBars");
  settings.setValue("/FileToolBar", d_file_tool_bar);
  settings.setValue("/EditToolBar", d_edit_tool_bar);
  settings.setValue("/TableToolBar", d_table_tool_bar);
  settings.setValue("/ColumnToolBar", d_column_tool_bar);
  settings.setValue("/MatrixToolBar", d_matrix_tool_bar);
  settings.setValue("/PlotToolBar", d_plot_tool_bar);
  settings.setValue("/Plot3DToolBar", d_plot3D_tool_bar);
  settings.setValue("/DisplayToolBar", d_display_tool_bar);
  settings.setValue("/FormatToolBar", d_format_tool_bar);
  settings.endGroup();

  //Save mantid settings
  mantidUI->saveSettings();

  //--------------------------------------
  // Mantid - Save custom scripts
  settings.beginGroup("CustomScripts");
  settings.remove("");
  foreach( QMenu* menu, d_user_menus )
  {
    settings.beginGroup(menu->title());
    foreach( QAction* action, menu->actions() )
    {
      settings.setValue(action->text(), action->data().toString());
    }
    settings.endGroup();
  }

  // Mantid - Remember which interfaces the user explicitely removed
  // from the Interfaces menu
  settings.setValue("RemovedInterfaces", removed_interfaces);

  settings.endGroup();
  //-----------------------------------
}

void ApplicationWindow::exportGraph()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  MultiLayer *plot2D = 0;
  Graph3D *plot3D = 0;
  if(w->isA("MultiLayer")){
    plot2D = (MultiLayer*)w;
    if (plot2D->isEmpty()){
      QMessageBox::critical(this, tr("MantidPlot - Export Error"),//Mantid
          tr("<h4>There are no plot layers available in this window!</h4>"));
      return;
    }
  } else if (w->isA("Graph3D"))
    plot3D = (Graph3D*)w;
  else
    return;

  ImageExportDialog *ied = new ImageExportDialog(this, plot2D!=NULL, d_extended_export_dialog);
  ied->setDir(workingDir);
  ied->selectFilter(d_image_export_filter);
  if ( ied->exec() != QDialog::Accepted )
    return;
  workingDir = ied->directory().path();
  if (ied->selectedFiles().isEmpty())
    return;

  QString selected_filter = ied->selectedFilter();
  QString file_name = ied->selectedFiles()[0];
  QFileInfo file_info(file_name);
  if (!file_info.fileName().contains("."))
    file_name.append(selected_filter.remove("*"));

  QFile file(file_name);
  if (!file.open( QIODevice::WriteOnly )){
    QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
    return;
  }
  file.close();

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") ||
      selected_filter.contains(".ps") || selected_filter.contains(".svg")) {
    if (plot3D)
      plot3D->exportVector(file_name);
    else if (plot2D){
      if (selected_filter.contains(".svg"))
        plot2D->exportSVG(file_name);
      else
        plot2D->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
    }
  } else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i=0; i<(int)list.count(); i++){
      if (selected_filter.contains("." + (list[i]).lower())) {
        if (plot2D)
          plot2D->exportImage(file_name, ied->quality(), ied->transparency());
        else if (plot3D)
          plot3D->exportImage(file_name, ied->quality(), ied->transparency());
      }
    }
  }
}

void ApplicationWindow::exportLayer()
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return;

  ImageExportDialog *ied = new ImageExportDialog(this, g!=NULL, d_extended_export_dialog);
  ied->setDir(workingDir);
  ied->selectFilter(d_image_export_filter);
  if ( ied->exec() != QDialog::Accepted )
    return;
  workingDir = ied->directory().path();
  if (ied->selectedFiles().isEmpty())
    return;

  QString selected_filter = ied->selectedFilter();
  QString file_name = ied->selectedFiles()[0];
  QFileInfo file_info(file_name);
  if (!file_info.fileName().contains("."))
    file_name.append(selected_filter.remove("*"));

  QFile file(file_name);
  if ( !file.open( QIODevice::WriteOnly ) ){
    QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
    return;
  }
  file.close();

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
    g->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
  else if (selected_filter.contains(".svg"))
    g->exportSVG(file_name);
  /*else if (selected_filter.contains(".emf"))
		g->exportEMF(file_name);*/
  else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i=0; i<(int)list.count(); i++)
      if (selected_filter.contains("."+(list[i]).lower()))
        g->exportImage(file_name, ied->quality(), ied->transparency());
  }
}

void ApplicationWindow::exportAllGraphs()
{
  ImageExportDialog *ied = new ImageExportDialog(this, true, d_extended_export_dialog);
  ied->setWindowTitle(tr("Choose a directory to export the graphs to"));
  QStringList tmp = ied->filters();
  ied->setFileMode(QFileDialog::Directory);
  ied->setFilters(tmp);
  ied->setLabelText(QFileDialog::FileType, tr("Output format:"));
  ied->setLabelText(QFileDialog::FileName, tr("Directory:"));

  ied->setDir(workingDir);
  ied->selectFilter(d_image_export_filter);

  if ( ied->exec() != QDialog::Accepted )
    return;
  workingDir = ied->directory().path();
  if (ied->selectedFiles().isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString output_dir = ied->selectedFiles()[0];
  QString file_suffix = ied->selectedFilter();
  file_suffix.lower();
  file_suffix.remove("*");

  bool confirm_overwrite = true;
  MultiLayer *plot2D;
  Graph3D *plot3D;

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")) {
      plot3D = 0;
      plot2D = (MultiLayer *)w;
      if (plot2D->isEmpty()) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
            tr("There are no plot layers available in window <b>%1</b>.<br>"
                "Graph window not exported!").arg(plot2D->objectName()));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        continue;
      }
    } else if (w->isA("Graph3D")) {
      plot2D = 0;
      plot3D = (Graph3D *)w;
    } else
      continue;

    QString file_name = output_dir + "/" + w->objectName() + file_suffix;
    QFile f(file_name);
    if (f.exists() && confirm_overwrite) {
      QApplication::restoreOverrideCursor();

      QString msg = tr("A file called: <p><b>%1</b><p>already exists. ""Do you want to overwrite it?").arg(file_name);
      QMessageBox msgBox(QMessageBox::Question, tr("MantidPlot - Overwrite file?"), msg,//Mantid
          QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel,
          (ApplicationWindow *)this);
      msgBox.exec();
      switch(msgBox.standardButton(msgBox.clickedButton())){
      case QMessageBox::Yes:
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        break;
      case QMessageBox::YesToAll:
        confirm_overwrite = false;
        break;
      case QMessageBox::No:
        confirm_overwrite = true;
        continue;
        break;
      case QMessageBox::Cancel:
        return;
        break;
      default:
        break;
      }
    }
    if ( !f.open( QIODevice::WriteOnly ) ) {
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
          tr("Could not write to file: <br><h4>%1</h4><p>"
              "Please verify that you have the right to write to this location!").arg(file_name));
      return;
    }
    f.close();

    if (file_suffix.contains(".eps") || file_suffix.contains(".pdf") ||
        file_suffix.contains(".ps") || file_suffix.contains(".svg")) {
      if (plot3D)
        plot3D->exportVector(file_name);
      else if (plot2D){
        if (file_suffix.contains(".svg"))
          plot2D->exportSVG(file_name);
        else
          plot2D->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
      }
    } else {
      QList<QByteArray> list = QImageWriter::supportedImageFormats();
      for (int i=0; i<(int)list.count(); i++){
        if (file_suffix.contains("." + (list[i]).lower())) {
          if (plot2D)
            plot2D->exportImage(file_name, ied->quality(), ied->transparency());
          else if (plot3D)
            plot3D->exportImage(file_name, ied->quality(), ied->transparency());
        }
      }
    }
  }
  QApplication::restoreOverrideCursor();
}

QString ApplicationWindow::windowGeometryInfo(MdiSubWindow *w)
{
  QString s = "geometry\t";
  if (w->status() == MdiSubWindow::Maximized){
    if (w == w->folder()->activeWindow())
      return s + "maximized\tactive\n";
    else
      return s + "maximized\n";
  }

  s += QString::number(w->x()) + "\t";
  s += QString::number(w->y()) + "\t";
  if (w->status() != MdiSubWindow::Minimized){
    s += QString::number(w->width()) + "\t";
    s += QString::number(w->height()) + "\t";
  } else {
    s += QString::number(w->minRestoreSize().width()) + "\t";
    s += QString::number(w->minRestoreSize().height()) + "\t";
    s += "minimized\t";
  }

  bool hide = hidden(w);
  if (w == w->folder()->activeWindow() && !hide)
    s+="active\n";
  else if(hide)
    s+="hidden\n";
  else
    s+="\n";
  return s;
}

void ApplicationWindow::restoreWindowGeometry(ApplicationWindow *app, MdiSubWindow *w, const QString s)
{
  if(!w) return ;
  w->hide();

  QString caption = w->objectName();
  if (s.contains ("minimized")) {
    QStringList lst = s.split("\t");
    if (lst.count() > 4){
      int width = lst[3].toInt();
      int height = lst[4].toInt();
      if(width > 0 && height > 0)
        w->resize(width, height);
    }
    w->setStatus(MdiSubWindow::Minimized);
    app->setListView(caption, tr("Minimized"));
  } else if (s.contains ("maximized")){
    w->setStatus(MdiSubWindow::Maximized);
    app->setListView(caption, tr("Maximized"));
  } else {
    QStringList lst = s.split("\t");
    if (lst.count() > 4){
      w->resize(lst[3].toInt(), lst[4].toInt());
      w->move(lst[1].toInt(), lst[2].toInt());
    }
    w->setStatus(MdiSubWindow::Normal);
    if (lst.count() > 5) {
      if (lst[5] == "hidden")
        app->hideWindow(w);
    }
  }

  if (s.contains ("active")) {
    Folder *f = w->folder();
    if (f)
      f->setActiveWindow(w);
  }
}

Folder* ApplicationWindow::projectFolder()
{
  return ((FolderListItem *)folders->firstChild())->folder();
}

bool ApplicationWindow::saveProject(bool compress)
{
  if (projectname == "untitled" || projectname.endsWith(".opj", Qt::CaseInsensitive) ||
      projectname.endsWith(".ogm", Qt::CaseInsensitive) || projectname.endsWith(".ogw", Qt::CaseInsensitive)
      || projectname.endsWith(".ogg", Qt::CaseInsensitive))
  {
    saveProjectAs();
    return true;;
  }

#ifdef QTIPLOT_DEMO
  showDemoVersionMessage();
  return false;
#endif

  saveFolder(projectFolder(), projectname, compress);

  setWindowTitle("MantidPlot - " + projectname);
  savedProject();

  if (autoSave){
    if (savingTimerId)
      killTimer(savingTimerId);
    savingTimerId=startTimer(autoSaveTime*60000);
  } else
    savingTimerId=0;

  QApplication::restoreOverrideCursor();
  return true;
}
void ApplicationWindow::savetoNexusFile()
{
  QString filter = tr("Mantid Files")+" (*.nxs *.nx5 *.xml);;";
  QString selectedFilter;
  QString fileDir=MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  if(fileDir.isEmpty())
  {fileDir="C\\Mantid\\Test\\Nexus";
  }
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File As"), fileDir, filter, &selectedFilter);
  if ( !fileName.isEmpty() ){
    std::string wsName;
    MdiSubWindow *w = activeWindow();
    if(w)
    {
      if(w->isA("MantidMatrix"))
      {
        wsName=((MantidMatrix*)w)->getWorkspaceName();
      }
    }
    else
    {
      wsName=m_nexusInputWSName.toStdString();
    }
    if(!Mantid::API::AnalysisDataService::Instance().doesExist(wsName))
    {
      throw std::runtime_error("Invalid input workspace for SaveNexus");
    }

    savedatainNexusFormat(wsName,fileName.toStdString());

    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(fileName).absoluteDir().path());

  }
}
void ApplicationWindow::loadNexus()
{
  QString filter = tr("Mantid Files")+" (*.nxs *.nx5  *.s *.xml);;";
  QString selectedFilter;
  std::string wsName;
  QString fileDir=MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  if(fileDir.isEmpty())
  {fileDir="C\\Mantid\\Test\\Nexus";//default
  }

  QString fileName= QFileDialog::getOpenFileName(this, tr("Open File"), fileDir, filter, &selectedFilter);
  if(fileName.isEmpty()) return;

  //look for the last "/ "in the file and . ,extract the string between these charcters to get the workspace name.
  std::string name(fileName.toStdString());
  //std::basic_string<char>::size_type index1,index2;
  int index1,index2;
  index1=static_cast<int>( name.find_last_of("//") );
  index2=static_cast<int>( name.find(".") );
  int count= index2-index1-1;
  if(index1!=-1 && index2!=-1)
    wsName= name.substr(index1+1,count);
  if(mantidUI) mantidUI->loaddataFromNexusFile(wsName,fileName.toStdString());
  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(fileName).absoluteDir().path());
}


void ApplicationWindow::loadRaw()
{
  QString filter = tr("Mantid Files")+" (*.raw *.s);;";
  QString selectedFilter;
  std::string wsName;
  QString fileDir=MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  if(fileDir.isEmpty())
  {fileDir="C\\Mantid\\Test\\Data";//default
  }
  QString fileName = QFileDialog::getOpenFileName (this, tr("Open File"), fileDir, filter, &selectedFilter);
  if(fileName.isEmpty())return;

  //look for the last / in the file and . ,extract the string between these charcters to get the workspace name.
  std::string name(fileName.toStdString());
  //std::basic_string<char>::size_type index1,index2;
  int index1,index2;
  index1=static_cast<int>( name.find_last_of("//") );
  index2=static_cast<int>( name.find(".") );
  int count=index2-index1-1;
  if(index1!=-1 && index2!=-1)
    wsName= name.substr(index1+1,count);
  if(mantidUI) mantidUI->loadadataFromRawFile(wsName,fileName.toStdString());
  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(fileName).absoluteDir().path());

}

void ApplicationWindow::loadDataFile()
{
  if(mantidUI)
  {
    mantidUI->executeAlgorithm("Load",-1);
  }
}
void ApplicationWindow::saveProjectAs(const QString& fileName, bool compress)
{
#ifdef QTIPLOT_DEMO
  showDemoVersionMessage();
  return;
#endif

  QString fn = fileName;
  if (fileName.isEmpty()){
    QString filter = tr("MantidPlot project")+" (*.mantid);;"; //tr("QtiPlot project")+" (*.qti);;";
    filter += tr("Compressed MantidPlot project")+" (*.mantid.gz)";

    QString selectedFilter;
    fn = QFileDialog::getSaveFileName(this, tr("Save Project As"), workingDir, filter, &selectedFilter);
    if (selectedFilter.contains(".gz"))
      compress = true;
  }

  if ( !fn.isEmpty() ){
    QFileInfo fi(fn);
    workingDir = fi.dirPath(true);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      //fn.append(".qti");
      fn.append(".mantid");

    projectname = fn;
    if (saveProject(compress)){
      recentProjects.remove(projectname);
      recentProjects.push_front(projectname);
      updateRecentProjectsList();

      QFileInfo fi(fn);
      QString baseName = fi.baseName();
      FolderListItem *item = (FolderListItem *)folders->firstChild();
      item->setText(0, baseName);
      item->folder()->setObjectName(baseName);
    }
  }
}

void ApplicationWindow::saveNoteAs()
{
  Note* w = (Note*)activeWindow(NoteWindow);
  if (!w)
    return;
  w->exportASCII();
}

void ApplicationWindow::saveAsTemplate(MdiSubWindow* w, const QString& fileName)
{
  if (!w) {
    w = activeWindow();
    if (!w)
      return;
  }

  QString fn = fileName;
  if (fn.isEmpty()){
    QString filter;
    if (w->isA("Matrix"))
      filter = tr("MantidPlot Matrix Template")+" (*.qmt)";
    else if (w->isA("MultiLayer"))
      filter = tr("MantidPlot 2D Graph Template")+" (*.qpt)";
    else if (w->inherits("Table"))
      filter = tr("MantidPlot Table Template")+" (*.qtt)";
    else if (w->isA("Graph3D"))
      filter = tr("MantidPlot 3D Surface Template")+" (*.qst)";

    QString selectedFilter;
    fn = QFileDialog::getSaveFileName(this, tr("Save Window As Template"), templatesDir + "/" + w->objectName(), filter, &selectedFilter);

    if (!fn.isEmpty()){
      QFileInfo fi(fn);
      workingDir = fi.dirPath(true);
      QString baseName = fi.fileName();
      if (!baseName.contains(".")){
        selectedFilter = selectedFilter.right(5).left(4);
        fn.append(selectedFilter);
      }
    } else
      return;
  }

  QFile f(fn);
  if ( !f.open( QIODevice::WriteOnly ) ){
    QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QString text = "MantidPlot " + QString::number(maj_version)+"."+ QString::number(min_version)+"."+
      QString::number(patch_version) + " template file\n";
  text += w->saveAsTemplate(windowGeometryInfo(w));
  QTextStream t( &f );
  t.setEncoding(QTextStream::UnicodeUTF8);
  t << text;
  f.close();
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::rename()
{
  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  RenameWindowDialog *rwd = new RenameWindowDialog(this);
  rwd->setAttribute(Qt::WA_DeleteOnClose);
  rwd->setWidget(m);
  rwd->exec();
}

void ApplicationWindow::renameWindow()
{
  WindowListItem *it = (WindowListItem *)lv->currentItem();
  MdiSubWindow *w = it->window();
  if (!w)
    return;

  RenameWindowDialog *rwd = new RenameWindowDialog(this);
  rwd->setAttribute(Qt::WA_DeleteOnClose);
  rwd->setWidget(w);
  rwd->exec();
}

void ApplicationWindow::renameWindow(Q3ListViewItem *item, int, const QString &text)
{
  if (!item)
    return;

  MdiSubWindow *w = ((WindowListItem *)item)->window();
  if (!w || text == w->objectName())
    return;

  if(!setWindowName(w, text))
    item->setText(0, w->objectName());
}

bool ApplicationWindow::setWindowName(MdiSubWindow *w, const QString &text)
{
  if (!w)
    return false;

  QString name = w->objectName();
  if (name == text)
    return true;

  QString newName = text;
  newName.replace("-", "_");
  if (newName.isEmpty()){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please enter a valid name!"));//Mantid
    return false;
  } else if (newName.contains(QRegExp("\\W"))){
    QMessageBox::critical(this, tr("MantidPlot - Error"),//Mantid
        tr("The name you chose is not valid: only letters and digits are allowed!")+
        "<p>" + tr("Please choose another name!"));
    return false;
  }

  newName.replace("_", "-");

  while(alreadyUsedName(newName)){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Name <b>%1</b> already exists!").arg(newName)+//Mantid
        "<p>"+tr("Please choose another name!")+
        "<p>"+tr("Warning: for internal consistency reasons the underscore character is replaced with a minus sign."));
    return false;
  }

  if (w->inherits("Table"))
    updateTableNames(name, newName);
  else if (w->isA("Matrix"))
    changeMatrixName(name, newName);

  w->setCaptionPolicy(w->captionPolicy());
  w->setName(newName);
  renameListViewItem(name, newName);
  return true;
}

QStringList ApplicationWindow::columnsList(Table::PlotDesignation plotType)
{
  QStringList list;
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (!w->inherits("Table"))
      continue;

    Table *t = (Table *)w;
    for (int i=0; i < t->numCols(); i++)
    {
      if (t->colPlotDesignation(i) == plotType || plotType == Table::All)
        list << QString(t->objectName()) + "_" + t->colLabel(i);
    }
  }
  return list;
}

void ApplicationWindow::showCurvesDialog()
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  if (((MultiLayer*)w)->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Error"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return;

  if (g->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Error"),//Mantid
        tr("This functionality is not available for pie plots!"));
  } else {
    CurvesDialog* crvDialog = new CurvesDialog(this);
    crvDialog->setAttribute(Qt::WA_DeleteOnClose);
    crvDialog->setGraph(g);
    crvDialog->resize(d_add_curves_dialog_size);
    crvDialog->setModal(true);
    crvDialog->show();
  }
}

bool ApplicationWindow::hasTable()
{
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->inherits("Table"))
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

QStringList ApplicationWindow::tableNames()
{
  QStringList lst = QStringList();
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->inherits("Table"))
        lst << w->objectName();
    }
    f = f->folderBelow();
  }
  return lst;
}

QList<MdiSubWindow*> ApplicationWindow::tableList()
{
  QList<MdiSubWindow*> lst;
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->inherits("Table"))
        lst << w;
    }
    f = f->folderBelow();
  }
  return lst;
}

AssociationsDialog* ApplicationWindow::showPlotAssociations(int curve)
{
  MdiSubWindow* w = activeWindow(MultiLayerWindow);
  if (!w)
    return 0;

  Graph *g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return 0;

  AssociationsDialog* ad = new AssociationsDialog(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->setGraph(g);
  ad->initTablesList(tableList(), curve);
  ad->show();
  return ad;
}

void ApplicationWindow::showTitleDialog()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->isA("MultiLayer")){
    Graph* g = ((MultiLayer*)w)->activeGraph();
    if (g){
      TextDialog* td= new TextDialog(TextDialog::LayerTitle, this,0);
      td->setGraph(g);
      td->exec();
    }
  } else if (w->isA("Graph3D")) {
    Plot3DDialog* pd = (Plot3DDialog*)showPlot3dDialog();
    if (pd)
      pd->showTitleTab();
  }
}

void ApplicationWindow::showAxisTitleDialog()
{
  MdiSubWindow* w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = ((MultiLayer*)w)->activeGraph();
  if (!g)
    return;

  TextDialog* td = new TextDialog(TextDialog::AxisTitle, this, 0);
  td->setGraph(g);
  td->exec();
}

void ApplicationWindow::showExportASCIIDialog()
{
  QString tableName = QString::null;
  MdiSubWindow* t = activeWindow();
  if (t && (t->isA("Matrix") || t->inherits("Table") || t->isA("MantidMatrix")))
  {
    tableName = t->objectName();

    ExportDialog* ed = new ExportDialog(tableName, this, Qt::WindowContextHelpButtonHint);
    ed->setAttribute(Qt::WA_DeleteOnClose);
    ed->exec();
  }
}

void ApplicationWindow::exportAllTables(const QString& sep, bool colNames, bool colComments, bool expSelection)
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to export the tables to"), workingDir, QFileDialog::ShowDirsOnly);
  if (!dir.isEmpty()){
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    workingDir = dir;

    bool confirmOverwrite = true;
    bool success = true;
    QList<MdiSubWindow *> windows = windowsList();
    foreach(MdiSubWindow *w, windows){
      if (w->inherits("Table") || w->isA("Matrix")){
        QString fileName = dir + "/" + w->objectName() + ".txt";
        QFile f(fileName);
        if (f.exists(fileName) && confirmOverwrite){
          QApplication::restoreOverrideCursor();
          switch(QMessageBox::question(this, tr("MantidPlot - Overwrite file?"),//Mantid
              tr("A file called: <p><b>%1</b><p>already exists. "
                  "Do you want to overwrite it?").arg(fileName), tr("&Yes"), tr("&All"), tr("&Cancel"), 0, 1))
          {
          case 0:
            if (w->inherits("Table"))
              success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
            else if (w->isA("Matrix"))
              success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);
            break;

          case 1:
            confirmOverwrite = false;
            if (w->inherits("Table"))
              success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
            else if (w->isA("Matrix"))
              success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);
            break;

          case 2:
            return;
            break;
          }
        } else if (w->inherits("Table"))
          success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
        else if (w->isA("Matrix"))
          success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);

        if (!success)
          break;
      }
    }
    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::exportASCII(const QString& tableName, const QString& sep,
    bool colNames, bool colComments, bool expSelection)
{
  MdiSubWindow* w = window(tableName);
  if (!w || !(w->isA("Matrix") || w->inherits("Table") || w->isA("MantidMatrix")))
    return;

  QString selectedFilter;
  QString fname = QFileDialog::getSaveFileName(this, tr("Choose a filename to save under"),
      asciiDirPath + "/" + w->objectName(), "*.txt;;*.dat;;*.DAT", &selectedFilter);
  if (!fname.isEmpty() ){
    QFileInfo fi(fname);
    QString baseName = fi.fileName();
    if (baseName.contains(".")==0)
      fname.append(selectedFilter.remove("*"));

    asciiDirPath = fi.dirPath(true);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if (w->inherits("Table"))
      ((Table *)w)->exportASCII(fname, sep, colNames, colComments, expSelection);
    else if (w->isA("Matrix"))
      ((Matrix *)w)->exportASCII(fname, sep, expSelection);
    else if (w->isA("MantidMatrix"))
    {
      //call save ascii
      try
      {
        Mantid::API::IAlgorithm_sptr alg =mantidUI->CreateAlgorithm("SaveAscii");
        alg->setPropertyValue("Filename",fname.toStdString());
        alg->setPropertyValue("Workspace",tableName.toStdString());
        alg->execute();
      }
      catch(...)
      {
      }
    }

    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::showRowsDialog()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  bool ok;
  int rows = QInputDialog::getInteger(this, tr("MantidPlot - Enter rows number"), tr("Rows"),//Mantid
      t->numRows(), 0, 1000000, 1, &ok);
  if ( ok )
    t->resizeRows(rows);
}

void ApplicationWindow::showDeleteRowsDialog()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  bool ok;
  int start_row = QInputDialog::getInteger(this, tr("MantidPlot - Delete rows"), tr("Start row"),//Mantid
      1, 1, t->numRows(), 1, &ok);
  if (ok){
    int end_row = QInputDialog::getInteger(this, tr("MantidPlot - Delete rows"), tr("End row"),//Mantid
        t->numRows(), 1, t->numRows(), 1, &ok);
    if (ok)
      t->deleteRows(start_row, end_row);
  }
}

void ApplicationWindow::showColsDialog()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  bool ok;
  int cols = QInputDialog::getInteger(this, tr("MantidPlot - Enter columns number"), tr("Columns"),//Mantid
      t->numCols(), 0, 1000000, 1, &ok);
  if ( ok )
    t->resizeCols(cols);
}

void ApplicationWindow::showColumnValuesDialog()
{
  Table *w = (Table*)activeWindow(TableWindow);
  if (!w)
    return;

  if (w->selectedColumns().count()>0 || w->table()->currentSelection() >= 0){
    SetColValuesDialog* vd = new SetColValuesDialog(scriptingEnv(), this);
    vd->setAttribute(Qt::WA_DeleteOnClose);
    vd->setTable(w);
    vd->exec();
  } else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::recalculateTable()
{
  MdiSubWindow* w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table"))
    ((Table*)w)->calculate();
  else if (w->isA("Matrix"))
    ((Matrix*)w)->calculate();
}

void ApplicationWindow::sortActiveTable()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if ((int)t->selectedColumns().count()>0)
    t->sortTableDialog();
  else
    QMessageBox::warning(this, "MantidPlot - Column selection error","Please select a column first!");//Mantid
}

void ApplicationWindow::sortSelection()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  t->sortColumnsDialog();
}

void ApplicationWindow::normalizeActiveTable()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if (int(t->selectedColumns().count())>0)
    t->normalize();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::normalizeSelection()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if (int(t->selectedColumns().count())>0)
    t->normalizeSelection();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::correlate()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2){
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select two columns for this operation!"));//Mantid
    return;
  }

  Correlation *cor = new Correlation(this, t, s[0], s[1]);
  cor->run();
  delete cor;
}

void ApplicationWindow::autoCorrelate()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 1)
  {
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select exactly one columns for this operation!"));//Mantid
    return;
  }

  Correlation *cor = new Correlation(this, t, s[0], s[0]);
  cor->run();
  delete cor;
}

void ApplicationWindow::convolute()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2)
  {
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));//Mantid
    return;
  }

  Convolution *cv = new Convolution(this, t, s[0], s[1]);
  cv->run();
  delete cv;
}

void ApplicationWindow::deconvolute()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2)
  {
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));//Mantid
    return;
  }

  Deconvolution *dcv = new Deconvolution(this, t, s[0], s[1]);
  dcv->run();
  delete dcv;
}

void ApplicationWindow::showColStatistics()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if (int(t->selectedColumns().count()) > 0)
  {
    QList<int> targets;
    for (int i=0; i < t->numCols(); i++)
      if (t->isColumnSelected(i, true))
        targets << i;
    newTableStatistics(t, TableStatistics::column, targets)->showNormal();
  }
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"),//Mantid
        tr("Please select a column first!"));
}

void ApplicationWindow::showRowStatistics()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if (t->numSelectedRows() > 0){
    QList<int> targets;
    for (int i=0; i < t->numRows(); i++)
      if (t->isRowSelected(i, true))
        targets << i;
    newTableStatistics(t, TableStatistics::row, targets)->showNormal();
  } else
    QMessageBox::warning(this, tr("MantidPlot - Row selection error"),//Mantid
        tr("Please select a row first!"));
}

void ApplicationWindow::showColMenu(int c)
{
  Table *w = (Table*)activeWindow(TableWindow);
  if (!w)
    return;

  QMenu contextMenu(this);
  QMenu plot(this);
  QMenu specialPlot(this);
  QMenu fill(this);
  QMenu sorting(this);
  QMenu colType(this);
  colType.setCheckable(true);
  QMenu panels(this);
  QMenu stat(this);
  QMenu norm(this);

  if ((int)w->selectedColumns().count()==1)
  {
    w->setSelectedCol(c);
    plot.addAction(QIcon(getQPixmap("lPlot_xpm")),tr("&Line"), this, SLOT(plotL()));
    plot.addAction(QIcon(getQPixmap("pPlot_xpm")),tr("&Scatter"), this, SLOT(plotP()));
    plot.addAction(QIcon(getQPixmap("lpPlot_xpm")),tr("Line + S&ymbol"), this, SLOT(plotLP()));

    specialPlot.addAction(QIcon(getQPixmap("dropLines_xpm")),tr("Vertical &Drop Lines"), this, SLOT(plotVerticalDropLines()));
    specialPlot.addAction(QIcon(getQPixmap("spline_xpm")),tr("&Spline"), this,SLOT(plotSpline()));
    specialPlot.addAction(QIcon(getQPixmap("vert_steps_xpm")),tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
    specialPlot.addAction(QIcon(getQPixmap("hor_steps_xpm")),tr("&Horizontal Steps"), this, SLOT(plotHorSteps()));
    specialPlot.setTitle(tr("Special Line/Symb&ol"));
    plot.addMenu(&specialPlot);
    plot.insertSeparator();

    plot.addAction(QIcon(getQPixmap("vertBars_xpm")),tr("&Columns"), this, SLOT(plotVerticalBars()));
    plot.addAction(QIcon(getQPixmap("hBars_xpm")),tr("&Rows"), this, SLOT(plotHorizontalBars()));
    plot.addAction(QIcon(getQPixmap("area_xpm")),tr("&Area"), this, SLOT(plotArea()));

    plot.addAction(QIcon(getQPixmap("pie_xpm")),tr("&Pie"), this, SLOT(plotPie()));
    plot.insertSeparator();

    plot.addAction(QIcon(getQPixmap("ribbon_xpm")),tr("3D Ribbo&n"), this, SLOT(plot3DRibbon()));
    plot.addAction(QIcon(getQPixmap("bars_xpm")),tr("3D &Bars"), this, SLOT(plot3DBars()));
    plot.addAction(QIcon(getQPixmap("scatter_xpm")),tr("3&D Scatter"), this, SLOT(plot3DScatter()));
    plot.addAction(QIcon(getQPixmap("trajectory_xpm")),tr("3D &Trajectory"), this, SLOT(plot3DTrajectory()));

    plot.insertSeparator();

    stat.addAction(actionBoxPlot);
    stat.addAction(QIcon(getQPixmap("histogram_xpm")),tr("&Histogram"), this, SLOT(plotHistogram()));
    stat.addAction(QIcon(getQPixmap("stacked_hist_xpm")),tr("&Stacked Histograms"), this, SLOT(plotStackedHistograms()));
    stat.setTitle(tr("Statistical &Graphs"));
    plot.addMenu(&stat);

    plot.setTitle(tr("&Plot"));
    contextMenu.addMenu(&plot);
    contextMenu.insertSeparator();

    contextMenu.addAction(QIcon(getQPixmap("cut_xpm")),tr("Cu&t"), w, SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")),tr("&Copy"), w, SLOT(copySelection()));
    contextMenu.addAction(QIcon(getQPixmap("paste_xpm")),tr("Past&e"), w, SLOT(pasteSelection()));
    contextMenu.insertSeparator();

    QAction * xColID=colType.addAction(QIcon(getQPixmap("x_col_xpm")), tr("&X"), this, SLOT(setXCol()));
    xColID->setCheckable(true);
    QAction * yColID=colType.addAction(QIcon(getQPixmap("y_col_xpm")), tr("&Y"), this, SLOT(setYCol()));
    yColID->setCheckable(true);
    QAction * zColID=colType.addAction(QIcon(getQPixmap("z_col_xpm")), tr("&Z"), this, SLOT(setZCol()));
    zColID->setCheckable(true);
    colType.insertSeparator();
    QAction * labelID = colType.addAction(QIcon(getQPixmap("set_label_col_xpm")), tr("&Label"), this, SLOT(setLabelCol()));
    labelID->setCheckable(true);
    QAction * noneID=colType.addAction(QIcon(getQPixmap("disregard_col_xpm")), tr("&None"), this, SLOT(disregardCol()));
    noneID->setCheckable(true);
    colType.insertSeparator();
    QAction * xErrColID =colType.addAction(tr("X E&rror"), this, SLOT(setXErrCol()));
    xErrColID->setCheckable(true);
    QAction * yErrColID = colType.addAction(QIcon(getQPixmap("errors_xpm")), tr("Y &Error"), this, SLOT(setYErrCol()));
    yErrColID->setCheckable(true);
    colType.insertSeparator();


    if (w->colPlotDesignation(c) == Table::X)
      xColID->setChecked(true);
    else if (w->colPlotDesignation(c) == Table::Y)
      yColID->setChecked(true);
    else if (w->colPlotDesignation(c) == Table::Z)
      zColID->setChecked(true);
    else if (w->colPlotDesignation(c) == Table::xErr)
      xErrColID->setChecked(true);
    else if (w->colPlotDesignation(c) == Table::yErr)
      yErrColID->setChecked(true);
    else if (w->colPlotDesignation(c) == Table::Label)
      labelID->setChecked(true);
    else
      noneID->setChecked(true);

    actionReadOnlyCol->addTo(&colType);
    actionReadOnlyCol->setCheckable(true);
    actionReadOnlyCol->setChecked(w->isReadOnlyColumn(c));

    colType.setTitle(tr("Set As"));
    contextMenu.addMenu(&colType);

    if (w){
      contextMenu.insertSeparator();

      contextMenu.addAction(actionShowColumnValuesDialog);
      contextMenu.addAction(actionTableRecalculate);
      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Column With"));
      contextMenu.addMenu(&fill);

      norm.addAction(tr("&Column"), w, SLOT(normalizeSelection()));
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      contextMenu.addMenu(& norm);

      contextMenu.insertSeparator();
      contextMenu.addAction(actionShowColStatistics);

      contextMenu.insertSeparator();

      contextMenu.addAction(QIcon(getQPixmap("erase_xpm")), tr("Clea&r"), w, SLOT(clearSelection()));
      contextMenu.addAction(QIcon(getQPixmap("delete_column_xpm")), tr("&Delete"), w, SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.insertSeparator();
      contextMenu.addAction(getQPixmap("insert_column_xpm"), tr("&Insert"), w, SLOT(insertCol()));
      contextMenu.addAction(actionAddColToTable);
      contextMenu.insertSeparator();

      sorting.addAction(QIcon(getQPixmap("sort_ascending_xpm")), tr("&Ascending"), w, SLOT(sortColAsc()));
      sorting.addAction(QIcon(getQPixmap("sort_descending_xpm")), tr("&Descending"), w, SLOT(sortColDesc()));

      sorting.setTitle(tr("Sort Colu&mn"));
      contextMenu.addMenu(&sorting);

      contextMenu.addAction(actionSortTable);
    }

    contextMenu.insertSeparator();
    contextMenu.addAction(actionShowColumnOptionsDialog);
  }
  else if ((int)w->selectedColumns().count()>1)
  {
    plot.addAction(QIcon(getQPixmap("lPlot_xpm")),tr("&Line"), this, SLOT(plotL()));
    plot.addAction(QIcon(getQPixmap("pPlot_xpm")),tr("&Scatter"), this, SLOT(plotP()));
    plot.addAction(QIcon(getQPixmap("lpPlot_xpm")),tr("Line + S&ymbol"), this,SLOT(plotLP()));

    specialPlot.addAction(QIcon(getQPixmap("dropLines_xpm")),tr("Vertical &Drop Lines"), this, SLOT(plotVerticalDropLines()));
    specialPlot.addAction(QIcon(getQPixmap("spline_xpm")),tr("&Spline"), this, SLOT(plotSpline()));
    specialPlot.addAction(QIcon(getQPixmap("vert_steps_xpm")),tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
    specialPlot.addAction(QIcon(getQPixmap("hor_steps_xpm")),tr("&Vertical Steps"), this, SLOT(plotHorSteps()));
    specialPlot.setTitle(tr("Special Line/Symb&ol"));
    plot.addMenu(&specialPlot);
    plot.insertSeparator();

    plot.addAction(QIcon(getQPixmap("vertBars_xpm")),tr("&Columns"), this, SLOT(plotVerticalBars()));
    plot.addAction(QIcon(getQPixmap("hBars_xpm")),tr("&Rows"), this, SLOT(plotHorizontalBars()));
    plot.addAction(QIcon(getQPixmap("area_xpm")),tr("&Area"), this, SLOT(plotArea()));
    plot.addAction(QIcon(getQPixmap("vectXYXY_xpm")),tr("Vectors &XYXY"), this, SLOT(plotVectXYXY()));
    plot.insertSeparator();

    stat.addAction(actionBoxPlot);
    stat.addAction(QIcon(getQPixmap("histogram_xpm")),tr("&Histogram"), this, SLOT(plotHistogram()));
    stat.addAction(QIcon(getQPixmap("stacked_hist_xpm")),tr("&Stacked Histograms"), this, SLOT(plotStackedHistograms()));
    stat.setTitle(tr("Statistical &Graphs"));
    plot.addMenu(&stat);

    panels.addAction(QIcon(getQPixmap("panel_v2_xpm")),tr("&Vertical 2 Layers"), this, SLOT(plot2VerticalLayers()));
    panels.addAction(QIcon(getQPixmap("panel_h2_xpm")),tr("&Horizontal 2 Layers"), this, SLOT(plot2HorizontalLayers()));
    panels.addAction(QIcon(getQPixmap("panel_4_xpm")),tr("&4 Layers"), this, SLOT(plot4Layers()));
    panels.addAction(QIcon(getQPixmap("stacked_xpm")),tr("&Stacked Layers"), this, SLOT(plotStackedLayers()));
    panels.setTitle(tr("Pa&nel"));
    plot.addMenu(&panels);

    plot.setTitle(tr("&Plot"));
    contextMenu.addMenu(&plot);
    contextMenu.insertSeparator();
    contextMenu.addAction(QIcon(getQPixmap("cut_xpm")),tr("Cu&t"), w, SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")),tr("&Copy"), w, SLOT(copySelection()));
    contextMenu.addAction(QIcon(getQPixmap("paste_xpm")),tr("Past&e"), w, SLOT(pasteSelection()));
    contextMenu.insertSeparator();

    if (w){
      contextMenu.addAction(QIcon(getQPixmap("erase_xpm")),tr("Clea&r"), w, SLOT(clearSelection()));
      contextMenu.addAction(QIcon(getQPixmap("close_xpm")),tr("&Delete"), w, SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.insertSeparator();
      contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
      contextMenu.addAction(actionAddColToTable);
      contextMenu.insertSeparator();
    }

    colType.addAction(actionSetXCol);
    colType.addAction(actionSetYCol);
    colType.addAction(actionSetZCol);
    colType.insertSeparator();
    colType.addAction(actionSetLabelCol);
    colType.addAction(actionDisregardCol);
    colType.insertSeparator();
    colType.addAction(actionSetXErrCol);
    colType.addAction(actionSetYErrCol);
    colType.insertSeparator();
    colType.addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
    colType.addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));
    colType.setTitle(tr("Set As"));
    contextMenu.addMenu(&colType);

    if (w)
    {
      contextMenu.insertSeparator();

      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Columns With"));
      contextMenu.addMenu(&fill);

      norm.addAction(actionNormalizeSelection);
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      contextMenu.addMenu(&norm);

      contextMenu.insertSeparator();
      contextMenu.addAction(actionSortSelection);
      contextMenu.addAction(actionSortTable);
      contextMenu.insertSeparator();
      contextMenu.addAction(actionShowColStatistics);
    }
  }

  QPoint posMouse=QCursor::pos();
  contextMenu.exec(posMouse);
}

void ApplicationWindow::plot2VerticalLayers()
{
  multilayerPlot(1, 2, defaultCurveStyle);
}

void ApplicationWindow::plot2HorizontalLayers()
{
  multilayerPlot(2, 1, defaultCurveStyle);
}

void ApplicationWindow::plot4Layers()
{
  multilayerPlot(2, 2, defaultCurveStyle);
}

void ApplicationWindow::plotStackedLayers()
{
  multilayerPlot(1, -1, defaultCurveStyle);
}

void ApplicationWindow::plotStackedHistograms()
{
  multilayerPlot(1, -1, Graph::Histogram);
}

void ApplicationWindow::showMatrixDialog()
{
  Matrix *m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  MatrixDialog* md = new MatrixDialog(this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix (m);
  md->exec();
}

void ApplicationWindow::showMatrixSizeDialog()
{
  Matrix *m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  MatrixSizeDialog* md = new MatrixSizeDialog(m, this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->exec();
}

void ApplicationWindow::showMatrixValuesDialog()
{
  Matrix *m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  MatrixValuesDialog* md = new MatrixValuesDialog(scriptingEnv(), this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix(m);
  md->exec();
}

void ApplicationWindow::showColumnOptionsDialog()
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  if(t->selectedColumns().count()>0) {
    TableDialog* td = new TableDialog(t, this);
    td->setAttribute(Qt::WA_DeleteOnClose);
    td->exec();
  } else
    QMessageBox::warning(this, tr("MantidPlot"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::showGeneralPlotDialog()
{
  MdiSubWindow* plot = activeWindow();
  if (!plot)
    return;

  if (plot->isA("MultiLayer") && ((MultiLayer*)plot)->layers())
    showPlotDialog();
  else if (plot->isA("Graph3D")){
    QDialog* gd = showScaleDialog();
    ((Plot3DDialog*)gd)->showGeneralTab();
  }
}

void ApplicationWindow::showAxisDialog()
{
  MdiSubWindow* plot = activeWindow();
  if (!plot)
    return;

  QDialog* gd = showScaleDialog();
  if (gd && plot->isA("MultiLayer") && ((MultiLayer*)plot)->layers())
    ((AxesDialog*)gd)->showAxesPage();
  else if (gd && plot->isA("Graph3D"))
    ((Plot3DDialog*)gd)->showAxisTab();
}

void ApplicationWindow::showGridDialog()
{
  AxesDialog* gd = (AxesDialog*)showScaleDialog();
  if (gd)
    gd->showGridPage();
}

QDialog* ApplicationWindow::showScaleDialog()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return 0;

  if (w->isA("MultiLayer")){
    if (((MultiLayer*)w)->isEmpty())
      return 0;

    Graph* g = ((MultiLayer*)w)->activeGraph();
    if (g->isPiePlot()){
      QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("This functionality is not available for pie plots!"));//Mantid
      return 0;
    }

    AxesDialog* ad = new AxesDialog(this);
    ad->setGraph(g);
    ad->exec();
    return ad;
  } else if (w->isA("Graph3D"))
    return showPlot3dDialog();

  return 0;
}

AxesDialog* ApplicationWindow::showScalePageFromAxisDialog(int axisPos)
{
  AxesDialog* gd = (AxesDialog*)showScaleDialog();
  if (gd)
    gd->setCurrentScale(axisPos);

  return gd;
}

AxesDialog* ApplicationWindow::showAxisPageFromAxisDialog(int axisPos)
{
  AxesDialog* gd = (AxesDialog*)showScaleDialog();
  if (gd){
    gd->showAxesPage();
    gd->setCurrentScale(axisPos);
  }
  return gd;
}

QDialog* ApplicationWindow::showPlot3dDialog()
{
  Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
  if (!g)
    return 0;

  if (!g->hasData()){
    QApplication::restoreOverrideCursor();
    QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
        tr("Not available for empty 3D surface plots!"));
    return 0;
  }

  Plot3DDialog* pd = new Plot3DDialog(this);
  pd->setPlot(g);
  pd->show();
  return pd;
}

void ApplicationWindow::showPlotDialog(int curveKey)
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  PlotDialog* pd = new PlotDialog(d_extended_plot_dialog, this);
  pd->setAttribute(Qt::WA_DeleteOnClose);
  pd->insertColumnsList(columnsList(Table::All));
  pd->setMultiLayer(w);
  if (curveKey >= 0){
    Graph *g = w->activeGraph();
    if (g)
      pd->selectCurve(g->curveIndex(curveKey));
  }
  pd->initFonts(plotTitleFont, plotAxesFont, plotNumbersFont, plotLegendFont);
  pd->showAll(d_extended_plot_dialog);
  pd->show();
}

void ApplicationWindow::showCurvePlotDialog()
{
  showPlotDialog(actionShowCurvePlotDialog->data().toInt());
}

void ApplicationWindow::showCurveContextMenu(int curveKey)
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph *g = w->activeGraph();
  DataCurve *c = (DataCurve *)g->curve(g->curveIndex(curveKey));
  if (!c || !c->isVisible())
    return;

  QMenu curveMenu(this);
  curveMenu.addAction(c->title().text(), this, SLOT(showCurvePlotDialog()));
  curveMenu.insertSeparator();

  curveMenu.addAction(actionHideCurve);
  actionHideCurve->setData(curveKey);

  if (g->visibleCurves() > 1 && c->type() == Graph::Function){
    curveMenu.addAction(actionHideOtherCurves);
    actionHideOtherCurves->setData(curveKey);
  } else if (c->type() != Graph::Function) {
    if ((g->visibleCurves() - c->errorBarsList().count()) > 1) {
      curveMenu.addAction(actionHideOtherCurves);
      actionHideOtherCurves->setData(curveKey);
    }
  }

  if (g->visibleCurves() != g->curves())
    curveMenu.addAction(actionShowAllCurves);
  curveMenu.insertSeparator();

  if (g->activeTool()){
    if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
        g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
      curveMenu.addAction(actionCopySelection);
  }

  if (c->type() == Graph::Function){
    curveMenu.insertSeparator();
    curveMenu.addAction(actionEditFunction);
    actionEditFunction->setData(curveKey);
  } else if (c->type() != Graph::ErrorBars){
    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
          g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker){
        curveMenu.addAction(actionCutSelection);
        curveMenu.addAction(actionPasteSelection);
        curveMenu.addAction(actionClearSelection);
        curveMenu.insertSeparator();
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector){
          QAction *act = new QAction(tr("Set Display Range"), this);
          connect(act, SIGNAL(activated()), (RangeSelectorTool *)g->activeTool(), SLOT(setCurveRange()));
          curveMenu.addAction(act);
        }
      }
    }

    curveMenu.addAction(actionEditCurveRange);
    actionEditCurveRange->setData(curveKey);

    curveMenu.addAction(actionCurveFullRange);
    if (c->isFullRange())
      actionCurveFullRange->setDisabled(true);
    else
      actionCurveFullRange->setEnabled(true);
    actionCurveFullRange->setData(curveKey);

    curveMenu.insertSeparator();
  }

  curveMenu.addAction(actionShowCurveWorksheet);
  actionShowCurveWorksheet->setData(curveKey);

  curveMenu.addAction(actionShowCurvePlotDialog);
  actionShowCurvePlotDialog->setData(curveKey);

  curveMenu.insertSeparator();

  curveMenu.addAction(actionRemoveCurve);
  actionRemoveCurve->setData(curveKey);
  curveMenu.exec(QCursor::pos());
}

void ApplicationWindow::showAllCurves()
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = w->activeGraph();
  if (!g)
    return;

  for(int i=0; i< g->curves(); i++)
    g->showCurve(i);
  g->replot();
}

void ApplicationWindow::hideOtherCurves()
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionHideOtherCurves->data().toInt();
  for(int i=0; i< g->curves(); i++)
    g->showCurve(i, false);

  g->showCurve(g->curveIndex(curveKey));
  g->replot();
}

void ApplicationWindow::hideCurve()
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionHideCurve->data().toInt();
  g->showCurve(g->curveIndex(curveKey), false);
}

void ApplicationWindow::removeCurve()
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionRemoveCurve->data().toInt();
  g->removeCurve(g->curveIndex(curveKey));
  g->updatePlot();
}

void ApplicationWindow::showCurveWorksheet(Graph *g, int curveIndex)
{
  if (!g)
    return;

  const QwtPlotItem *it = g->plotItem(curveIndex);
  if (!it)
    return;

  if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
    Spectrogram *sp = (Spectrogram *)it;
    if (sp->matrix())
      sp->matrix()->showMaximized();
  } else if (((PlotCurve *)it)->type() == Graph::Function)
    g->createTable((PlotCurve *)it);
  else {
    showTable(it->title().text());
    if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
      ((DataPickerTool *)g->activeTool())->selectTableRow();
  }
}

void ApplicationWindow::showCurveWorksheet()
{
  MultiLayer *w = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionShowCurveWorksheet->data().toInt();
  showCurveWorksheet(g, g->curveIndex(curveKey));
}

void ApplicationWindow::zoomIn()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  if (plot->isEmpty())
  {
    QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setOn(true);
    return;
  }

  if ((Graph*)plot->activeGraph()->isPiePlot())
  {
    if (btnZoomIn->isOn())
      QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
          tr("This functionality is not available for pie plots!"));
    btnPointer->setOn(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers){
    if (!g->isPiePlot())
      g->zoom(true);
  }
}

void ApplicationWindow::zoomOut()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  if (plot->isEmpty() || (Graph*)plot->activeGraph()->isPiePlot())
    return;

  ((Graph*)plot->activeGraph())->zoomOut();
  btnPointer->setOn(true);
}

void ApplicationWindow::setAutoScale()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  Graph *g = (Graph*)plot->activeGraph();
  if (g)
    g->setAutoScale();
}

void ApplicationWindow::removePoints()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
  {
    btnPointer->setChecked(true);
    return;
  }

  if (g->isPiePlot())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }
  else
  {
    switch(QMessageBox::warning (this,tr("MantidPlot"),//Mantid
        tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"),
        tr("Continue"),tr("Cancel"),0,1))
    {
    case 0:
      g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Remove, info, SLOT(setText(const QString&))));
      displayBar->show();
      break;

    case 1:
      btnPointer->setChecked(true);
      break;
    }
  }
}

void ApplicationWindow::movePoints()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g || !g->validCurvesDataSize()){
    btnPointer->setChecked(true);
    return;
  }

  if (g->isPiePlot()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));

    btnPointer->setChecked(true);
    return;
  } else {
    switch(QMessageBox::warning (this, tr("MantidPlot"),//Mantid
        tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"),
        tr("Continue"), tr("Cancel"), 0, 1))
    {
    case 0:
      if (g){
        g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Move, info, SLOT(setText(const QString&))));
        displayBar->show();
      }
      break;

    case 1:
      btnPointer->setChecked(true);
      break;
    }
  }
}

void ApplicationWindow::exportPDF()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->isA("MultiLayer") && ((MultiLayer *)w)->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  QString fname = QFileDialog::getSaveFileName(this, tr("Choose a filename to save under"), workingDir, "*.pdf");
  if (!fname.isEmpty() ){
    QFileInfo fi(fname);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fname.append(".pdf");

    workingDir = fi.dirPath(true);

    QFile f(fname);
    if (!f.open(QIODevice::WriteOnly)){
      QMessageBox::critical(this, tr("MantidPlot - Export error"),//Mantid
          tr("Could not write to file: <h4>%1</h4><p>Please verify that you have the right to write to this location or that the file is not being used by another application!").arg(fname));
      return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    w->exportPDF(fname);
    QApplication::restoreOverrideCursor();
  }
}

//print active window
void ApplicationWindow::print()
{
  MdiSubWindow* w = activeWindow();
  if (!w)
    return;

  if (w->isA("MultiLayer") && ((MultiLayer *)w)->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }
  w->print();
}

void ApplicationWindow::printAllPlots()
{
  QPrinter printer;
  printer.setOrientation(QPrinter::Landscape);
  printer.setColorMode (QPrinter::Color);
  printer.setFullPage(true);

  if (printer.setup())
  {
    QPainter *paint = new QPainter (&printer);

    int plots = 0;
    QList<MdiSubWindow *> windows = windowsList();
    foreach(MdiSubWindow *w, windows){
      if (w->isA("MultiLayer"))
        plots++;
    }

    printer.setMinMax (0, plots);
    printer.setFromTo (0, plots);

    foreach(MdiSubWindow *w, windows){
      if (w->isA("MultiLayer") && printer.newPage())
        ((MultiLayer*)w)->printAllLayers(paint);
    }
    paint->end();
    delete paint;
  }
}

void ApplicationWindow::showExpGrowthDialog()
{
  showExpDecayDialog(-1);
}

void ApplicationWindow::showExpDecayDialog()
{
  showExpDecayDialog(1);
}

void ApplicationWindow::showExpDecayDialog(int type)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  ExpDecayDialog *edd = new ExpDecayDialog(type, this);
  edd->setAttribute(Qt::WA_DeleteOnClose);
  connect (g, SIGNAL(destroyed()), edd, SLOT(close()));

  edd->setGraph(g);
  edd->show();
}

void ApplicationWindow::showTwoExpDecayDialog()
{
  showExpDecayDialog(2);
}

void ApplicationWindow::showExpDecay3Dialog()
{
  showExpDecayDialog(3);
}

void ApplicationWindow::showFitDialog()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  MultiLayer* plot = 0;
  if(w->isA("MultiLayer"))
    plot = (MultiLayer*)w;
  else if(w->inherits("Table"))
    plot = multilayerPlot((Table *)w, ((Table *)w)->drawableColumnSelection(), Graph::LineSymbols);

  if (!plot)
    return;

  Graph* g = (Graph*)plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  FitDialog *fd = new FitDialog(g, this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  connect (plot, SIGNAL(destroyed()), fd, SLOT(close()));

  fd->setSrcTables(tableList());
  fd->show();
  fd->resize(fd->minimumSize());
}

void ApplicationWindow::showFilterDialog(int filter)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if ( g && g->validCurvesDataSize())
  {
    FilterDialog *fd = new FilterDialog(filter, this);
    fd->setAttribute(Qt::WA_DeleteOnClose);
    fd->setGraph(g);
    fd->exec();
  }
}

void ApplicationWindow::lowPassFilterDialog()
{
  showFilterDialog(FFTFilter::LowPass);
}

void ApplicationWindow::highPassFilterDialog()
{
  showFilterDialog(FFTFilter::HighPass);
}

void ApplicationWindow::bandPassFilterDialog()
{
  showFilterDialog(FFTFilter::BandPass);
}

void ApplicationWindow::bandBlockFilterDialog()
{
  showFilterDialog(FFTFilter::BandBlock);
}

void ApplicationWindow::showFFTDialog()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  FFTDialog *sd = 0;
  if (w->isA("MultiLayer")) {
    Graph* g = ((MultiLayer*)w)->activeGraph();
    if ( g && g->validCurvesDataSize() ){
      sd = new FFTDialog(FFTDialog::onGraph, this);
      sd->setAttribute(Qt::WA_DeleteOnClose);
      sd->setGraph(g);
    }
  } else if (w->inherits("Table")) {
    sd = new FFTDialog(FFTDialog::onTable, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setTable((Table*)w);
  } else if (w->inherits("Matrix")) {
    sd = new FFTDialog(FFTDialog::onMatrix, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setMatrix((Matrix*)w);
  }

  if (sd)
    sd->exec();
}

void ApplicationWindow::showSmoothDialog(int m)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  SmoothCurveDialog *sd = new SmoothCurveDialog(m, this);
  sd->setAttribute(Qt::WA_DeleteOnClose);
  sd->setGraph(g);
  sd->exec();
}

void ApplicationWindow::showSmoothSavGolDialog()
{
  showSmoothDialog(SmoothFilter::SavitzkyGolay);
}

void ApplicationWindow::showSmoothFFTDialog()
{
  showSmoothDialog(SmoothFilter::FFT);
}

void ApplicationWindow::showSmoothAverageDialog()
{
  showSmoothDialog(SmoothFilter::Average);
}

void ApplicationWindow::showInterpolationDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  InterpolationDialog *id = new InterpolationDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect (g, SIGNAL(destroyed()), id, SLOT(close()));
  id->setGraph(g);
  id->show();
}

void ApplicationWindow::showFitPolynomDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  PolynomFitDialog *pfd = new PolynomFitDialog(this);
  pfd->setAttribute(Qt::WA_DeleteOnClose);
  connect(g, SIGNAL(destroyed()), pfd, SLOT(close()));
  pfd->setGraph(g);
  pfd->show();
}

void ApplicationWindow::updateLog(const QString& result)
{	
  if ( !result.isEmpty() ){
    current_folder->appendLogInfo(result);
    showResults(true);
    emit modified();
  }
}

void ApplicationWindow::showIntegrationDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  IntDialog *id = new IntDialog(this, g);
  id->exec();
}

/**
 * Sets the visibilty of the log window. If it is shown then
 * the results are scrolled to the bottom
 */
void ApplicationWindow::showLogWindow(bool show)
{
  logWindow->setVisible(show);
  if( show )
  {
    QTextCursor cur = results->textCursor();
    cur.movePosition(QTextCursor::End);
    results->setTextCursor(cur);
  }
}

void ApplicationWindow::showResults(bool ok)
{
  if (ok)
  {
    if (!current_folder->logInfo().isEmpty())
      results->setText(current_folder->logInfo());
    else
      results->setText(tr("Sorry, there are no results to display!"));
  }
  showLogWindow(ok);
}

void ApplicationWindow::showResults(const QString& s, bool ok)
{
  current_folder->appendLogInfo(s);

  QString logInfo = current_folder->logInfo();
  if (!logInfo.isEmpty())
    results->setText(logInfo);
  showResults(ok);
}

void ApplicationWindow::showScreenReader()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers)
  g->setActiveTool(new ScreenPickerTool(g, info, SLOT(setText(const QString&))));

  displayBar->show();
}

void ApplicationWindow::drawPoints()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers)
  g->setActiveTool(new DrawPointTool(this, g, info, SLOT(setText(const QString&))));

  displayBar->show();
}

void ApplicationWindow::showRangeSelectors()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no plot layers available in this window!"));//Mantid
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  if (!g->curves()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no curves available on this plot!"));//Mantid
    btnPointer->setChecked(true);
    return;
  } else if (g->isPiePlot()) {
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("This functionality is not available for pie plots!"));//Mantid
    btnPointer->setChecked(true);
    return;
  }

  displayBar->show();
  g->enableRangeSelectors(info, SLOT(setText(const QString&)));
}

void ApplicationWindow::showCursor()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if ((Graph*)plot->activeGraph()->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers){
    if (g->isPiePlot() || !g->curves())
      continue;
    if (g->validCurvesDataSize())
      g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Display, info, SLOT(setText(const QString&))));
  }
  displayBar->show();
}

/**  Switch on the multi-peak selecting tool for fitting
 * with the Fit algorithm of multiple peaks on a single background
 */
void ApplicationWindow::selectMultiPeak()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if ((Graph*)plot->activeGraph()->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers){
    if (g->isPiePlot() || !g->curves())
    {
      continue;
    }
    if (g->validCurvesDataSize())
    {
      PeakPickerTool* ppicker = new PeakPickerTool(g, mantidUI);
      g->setActiveTool(ppicker);
      connect(plot,SIGNAL(windowStateChanged(Qt::WindowStates, Qt::WindowStates)),ppicker,SLOT(windowStateChanged(Qt::WindowStates, Qt::WindowStates)));
    }
  }

}

void ApplicationWindow::newLegend()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if ( g )
    g->newLegend();
}

void ApplicationWindow::addTimeStamp()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if ( g )
    g->addTimeStamp();
}

void ApplicationWindow::disableAddText()
{
  actionAddText->setChecked(false);
  showTextDialog();
}

void ApplicationWindow::addText()
{
  if (!btnPointer->isOn())
    btnPointer->setOn(TRUE);

  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  Graph *g = (Graph*)plot->activeGraph();
  if (g)
    g->drawText(true);
}

void ApplicationWindow::addImage()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  QList<QByteArray> list = QImageReader::supportedImageFormats();
  QString filter = tr("Images") + " (", aux1, aux2;
  for (int i=0; i<(int)list.count(); i++){
    aux1 = " *."+list[i]+" ";
    aux2 += " *."+list[i]+";;";
    filter += aux1;
  }
  filter+=");;" + aux2;

  QString fn = QFileDialog::getOpenFileName(this, tr("MantidPlot - Insert image from file"), imagesDirPath, filter);//Mantid
  if ( !fn.isEmpty() ){
    QFileInfo fi(fn);
    imagesDirPath = fi.dirPath(true);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    g->addImage(fn);
    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::drawLine()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));

    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (g)
  {
    g->drawLine(true);
    emit modified();
  }
}

void ApplicationWindow::drawArrow()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));

    btnPointer->setOn(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (g)
  {
    g->drawLine(true, 1);
    emit modified();
  }
}

void ApplicationWindow::showImageDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g)
  {
    ImageMarker *im = (ImageMarker *) g->selectedMarkerPtr();
    if (!im)
      return;

    ImageDialog *id = new ImageDialog(this);
    id->setAttribute(Qt::WA_DeleteOnClose);
    connect (id, SIGNAL(setGeometry(int, int, int, int)),
        g, SLOT(updateImageMarker(int, int, int, int)));
    //		id->setIcon(getQPixmap("logo_xpm"));
    id->setOrigin(im->origin());
    id->setSize(im->size());
    id->exec();
  }
}

void ApplicationWindow::showLayerDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if(plot->isEmpty())
  {
    QMessageBox::warning(this, tr("MantidPlot - Warning"),//Mantid
        tr("There are no plot layers available in this window."));
    return;
  }

  LayerDialog *id=new LayerDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  id->setMultiLayer(plot);
  id->exec();
}

void ApplicationWindow::showTextDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if ( g ){
    LegendWidget *l = (LegendWidget *) g->selectedText();
    if (!l)
      return;

    TextDialog *td = new TextDialog(TextDialog::TextMarker, this, 0);
    td->setLegendWidget(l);
    td->exec();
  }
}

void ApplicationWindow::showLineDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g){
    ArrowMarker *lm = (ArrowMarker *) g->selectedMarkerPtr();
    if (!lm)
      return;

    LineDialog *ld = new LineDialog(lm, this);
    ld->exec();
  }
}

void ApplicationWindow::addColToTable()
{
  Table* m = (Table*)activeWindow(TableWindow);
  if ( m )
    m->addCol();
}

void ApplicationWindow::clearSelection()
{
  if(lv->hasFocus()){
    deleteSelectedItems();
    return;
  }

  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table"))
    ((Table*)m)->clearSelection();
  else if (m->isA("Matrix"))
    ((Matrix*)m)->clearSelection();
  else if (m->isA("MultiLayer")){
    Graph* g = ((MultiLayer*)m)->activeGraph();
    if (!g)
      return;

    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
        ((RangeSelectorTool *)g->activeTool())->clearSelection();
      else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
        ((DataPickerTool *)g->activeTool())->removePoint();
    } else if (g->titleSelected())
      g->removeTitle();
    else if (g->markerSelected())
      g->removeMarker();
  }
  else if (m->isA("Note"))
    ((Note*)m)->editor()->clear();
  emit modified();
}

void ApplicationWindow::copySelection()
{
  if(results->hasFocus()){
    results->copy();
    return;
  } else if(info->hasFocus()) {
    info->copy();
    return;
  }
  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table"))
    ((Table*)m)->copySelection();
  else if (m->isA("Matrix"))
    ((Matrix*)m)->copySelection();
  else if (m->isA("MultiLayer")){
    MultiLayer* plot = (MultiLayer*)m;
    if (!plot || plot->layers() == 0)
      return;

    Graph* g = (Graph*)plot->activeGraph();
    if (!g)
      return;

    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
      {
        ((RangeSelectorTool *)g->activeTool())->copySelection();
      }
      else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
      {
        ((DataPickerTool *)g->activeTool())->copySelection();
      }
    } else if (g->markerSelected()){
      copyMarker();
    } else
    {
      copyActiveLayer();
    }

    plot->copyAllLayers();
  }
  else if (m->isA("Note"))
    ((Note*)m)->editor()->copy();
  else
    mantidUI->copyValues();//Mantid
}

void ApplicationWindow::cutSelection()
{
  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table"))
    ((Table*)m)->cutSelection();
  else if (m->isA("Matrix"))
    ((Matrix*)m)->cutSelection();
  else if(m->isA("MultiLayer")){
    MultiLayer* plot = (MultiLayer*)m;
    if (!plot || plot->layers() == 0)
      return;

    Graph* g = (Graph*)plot->activeGraph();
    if (!g)
      return;

    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
        ((RangeSelectorTool *)g->activeTool())->cutSelection();
      else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
        ((DataPickerTool *)g->activeTool())->cutSelection();
    } else {
      copyMarker();
      g->removeMarker();
    }
  }
  else if (m->isA("Note"))
    ((Note*)m)->editor()->cut();

  emit modified();
}

void ApplicationWindow::copyMarker()
{
  lastCopiedLayer = NULL;

  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g && g->markerSelected()){
    if (g->selectedText()){
      d_text_copy = g->selectedText();
      d_image_copy = NULL;
      d_arrow_copy = NULL;
    } else if (g->arrowMarkerSelected()){
      d_arrow_copy = (ArrowMarker *) g->selectedMarkerPtr();
      d_image_copy = NULL;
      d_text_copy = NULL;
    } else if (g->imageMarkerSelected()){
      d_image_copy = (ImageMarker *) g->selectedMarkerPtr();
      d_text_copy = NULL;
      d_arrow_copy = NULL;
    }
  }
}

void ApplicationWindow::pasteSelection()
{
  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table"))
    ((Table*)m)->pasteSelection();
  else if (m->isA("Matrix"))
    ((Matrix*)m)->pasteSelection();
  else if (m->isA("Note"))
    ((Note*)m)->editor()->paste();
  else if (m->isA("MultiLayer")){
    MultiLayer* plot = (MultiLayer*)m;
    if (!plot)
      return;

    if (lastCopiedLayer){
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      Graph* g = plot->addLayer();
      g->copy(lastCopiedLayer);
      QPoint pos = plot->mapFromGlobal(QCursor::pos());
      plot->setGraphGeometry(pos.x(), pos.y()-20, lastCopiedLayer->width(), lastCopiedLayer->height());

      QApplication::restoreOverrideCursor();
    } else {
      if (plot->layers() == 0)
        return;

      Graph* g = (Graph*)plot->activeGraph();
      if (!g)
        return;

      if (g->activeTool()){
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
          ((RangeSelectorTool *)g->activeTool())->pasteSelection();
        else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
          ((DataPickerTool *)g->activeTool())->pasteSelection();
      } else if (d_text_copy){
        LegendWidget *t = g->insertText(d_text_copy);
        t->move(g->mapFromGlobal(QCursor::pos()));
      } else if (d_arrow_copy){
        ArrowMarker *a = g->addArrow(d_arrow_copy);
        a->setStartPoint(QPoint(d_arrow_copy->startPoint().x() + 10,
            d_arrow_copy->startPoint().y() + 10));
        a->setEndPoint(QPoint(d_arrow_copy->endPoint().x() + 10,
            d_arrow_copy->endPoint().y() + 10));
        g->replot();
        g->deselectMarker();
      } else if (d_image_copy){
        ImageMarker *i = g->addImage(d_image_copy);
        QPoint pos = g->plotWidget()->canvas()->mapFromGlobal(QCursor::pos());
        QSize size = d_image_copy->size();
        i->setRect(pos.x(), pos.y(), size.width(), size.height());
        g->replot();
        g->deselectMarker();
      }
    }
  }
  emit modified();
}

MdiSubWindow* ApplicationWindow::clone(MdiSubWindow* w)
{
  if (!w) {
    w = activeWindow();
    if (!w){
      QMessageBox::critical(this,tr("MantidPlot - Duplicate window error"),//Mantid
          tr("There are no windows available in this project!"));
      return 0;
    }
  }

  MdiSubWindow* nw = 0;
  MdiSubWindow::Status status = w->status();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if (w->isA("MultiLayer")){
    MultiLayer *g = (MultiLayer *)w;
    nw = multilayerPlot(generateUniqueName(tr("Graph")), 0, g->getRows(), g->getCols());
    ((MultiLayer *)nw)->copy(g);
  } else if (w->inherits("Table")){
    Table *t = (Table *)w;
    QString caption = generateUniqueName(tr("Table"));
    nw = newTable(caption, t->numRows(), t->numCols());
    ((Table *)nw)->copy(t);
    QString spec = t->saveToString("geometry\n");
    ((Table *)nw)->setSpecifications(spec.replace(t->objectName(), caption));
  } else if (w->isA("Graph3D")){
    Graph3D *g = (Graph3D *)w;
    if (!g->hasData()){
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, tr("MantidPlot - Duplicate error"), tr("Empty 3D surface plots cannot be duplicated!"));//Mantid
      return 0;
    }

    QString caption = generateUniqueName(tr("Graph"));
    QString s = g->formula();
    if (g->userFunction()){
      UserFunction *f = g->userFunction();
      nw = plotSurface(f->function(), g->xStart(), g->xStop(), g->yStart(), g->yStop(),
          g->zStart(), g->zStop(), f->columns(), f->rows());
    } else if (g->parametricSurface()){
      UserParametricSurface *s = g->parametricSurface();
      nw = plotParametricSurface(s->xFormula(), s->yFormula(), s->zFormula(), s->uStart(), s->uEnd(),
          s->vStart(), s->vEnd(), s->columns(), s->rows(), s->uPeriodic(), s->vPeriodic());
    } else if (s.endsWith("(Z)"))
      nw = openPlotXYZ(caption, s, g->xStart(),g->xStop(), g->yStart(),g->yStop(),g->zStart(),g->zStop());
    else if (s.endsWith("(Y)"))//Ribbon plot
      nw = dataPlot3D(caption, s, g->xStart(),g->xStop(), g->yStart(),g->yStop(),g->zStart(),g->zStop());
    else
      nw = openMatrixPlot3D(caption, s, g->xStart(), g->xStop(), g->yStart(), g->yStop(),g->zStart(),g->zStop());

    if (!nw)
      return 0;

    if (status == MdiSubWindow::Maximized)
      nw->hide();
    ((Graph3D *)nw)->copy(g);
    customToolBars(nw);
  } else if (w->isA("Matrix")){
    nw = newMatrix(((Matrix *)w)->numRows(), ((Matrix *)w)->numCols());
    ((Matrix *)nw)->copy((Matrix *)w);
  } else if (w->isA("Note")){
    nw = newNote();
    if (nw)
      ((Note*)nw)->setText(((Note*)w)->text());
  }

  if (nw){
    if (w->isA("MultiLayer")){
      if (status == MdiSubWindow::Maximized)
        nw->showMaximized();
    } else if (w->isA("Graph3D")){
      ((Graph3D*)nw)->setIgnoreFonts(true);
      if (status != MdiSubWindow::Maximized){
        nw->resize(w->size());
        nw->showNormal();
      } else
        nw->showMaximized();
      ((Graph3D*)nw)->setIgnoreFonts(false);
    } else {
      nw->resize(w->size());
      nw->showNormal();
    }

    nw->setWindowLabel(w->windowLabel());
    nw->setCaptionPolicy(w->captionPolicy());
    setListViewSize(nw->objectName(), w->sizeToString());
  }
  QApplication::restoreOverrideCursor();
  customMenu(nw);
  return nw;
}

void ApplicationWindow::undo()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if (qobject_cast<Note*>(w))
    ((Note*)w)->editor()->undo();
  else if (qobject_cast<Matrix*>(w)){
    QUndoStack *stack = ((Matrix *)w)->undoStack();
    if (stack && stack->canUndo())
      stack->undo();
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::redo()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if (qobject_cast<Note*>(w))
    ((Note*)w)->editor()->redo();
  else if (qobject_cast<Matrix*>(w)){
    QUndoStack *stack = ((Matrix *)w)->undoStack();
    if (stack && stack->canRedo())
      stack->redo();
  }

  QApplication::restoreOverrideCursor();
}

bool ApplicationWindow::hidden(QWidget* window)
{
  if (hiddenWindows->contains(window))
    return true;

  return false;
}

void ApplicationWindow::updateWindowStatus(MdiSubWindow* w)
{
  setListView(w->objectName(), w->aspect());
  if (w->status() == MdiSubWindow::Maximized){
    QList<MdiSubWindow *> windows = current_folder->windowsList();
    foreach(MdiSubWindow *oldMaxWindow, windows){
      if (oldMaxWindow != w && oldMaxWindow->status() == MdiSubWindow::Maximized)
        oldMaxWindow->setStatus(MdiSubWindow::Normal);
    }
  }
  modifiedProject();
}

void ApplicationWindow::hideActiveWindow()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  hideWindow(w);
}

void ApplicationWindow::hideWindow(MdiSubWindow* w)
{
  hiddenWindows->append(w);
  w->setHidden();
  emit modified();
}

void ApplicationWindow::hideWindow()
{
  WindowListItem *it = (WindowListItem *)lv->currentItem();
  MdiSubWindow *w = it->window();
  if (!w)
    return;

  hideWindow(w);
}

void ApplicationWindow::resizeActiveWindow()
{
  MdiSubWindow* w = activeWindow();
  if (!w)
    return;

  ImageDialog *id = new ImageDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect (id, SIGNAL(setGeometry(int,int,int,int)), this, SLOT(setWindowGeometry(int,int,int,int)));

  id->setWindowTitle(tr("MantidPlot - Window Geometry"));//Mantid
  id->setOrigin(w->pos());
  id->setSize(w->size());
  id->exec();
}

void ApplicationWindow::resizeWindow()
{
  WindowListItem *it = (WindowListItem *)lv->currentItem();
  MdiSubWindow *w = it->window();
  if (!w)
    return;

  d_workspace->setActiveSubWindow(w);

  ImageDialog *id = new ImageDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect (id, SIGNAL(setGeometry(int,int,int,int)), this, SLOT(setWindowGeometry(int,int,int,int)));

  id->setWindowTitle(tr("MantidPlot - Window Geometry"));//Mantid
  id->setOrigin(w->pos());
  id->setSize(w->size());
  id->exec();
}

void ApplicationWindow::setWindowGeometry(int x, int y, int w, int h)
{
  activeWindow()->setGeometry(x, y, w, h);
}

void ApplicationWindow::activateWindow()
{
  WindowListItem *it = (WindowListItem *)lv->currentItem();
  activateWindow(it->window());
}

void ApplicationWindow::activateWindow(MdiSubWindow *w)
{
  if (!w)
    return;

  w->setNormal();
  d_workspace->setActiveSubWindow(w);

  updateWindowLists(w);
  emit modified();
}

void ApplicationWindow::maximizeWindow(Q3ListViewItem * lbi)
{
  if (!lbi)
    lbi = lv->currentItem();

  if (!lbi || lbi->rtti() == FolderListItem::RTTI)
    return;

  maximizeWindow(((WindowListItem*)lbi)->window());
}

void ApplicationWindow::maximizeWindow(MdiSubWindow *w)
{
  if (!w || w->status() == MdiSubWindow::Maximized)
    return;

  QList<MdiSubWindow *> windows = current_folder->windowsList();
  foreach(MdiSubWindow *ow, windows){
    if (ow != w && ow->status() == MdiSubWindow::Maximized){
      ow->setNormal();
      break;
    }
  }

  w->setMaximized();
  updateWindowLists(w);
  emit modified();
}

void ApplicationWindow::minimizeWindow(MdiSubWindow *w)
{
  if (!w)
    w = ((WindowListItem *)lv->currentItem())->window();

  if (!w)
    return;

  updateWindowLists(w);
  w->setMinimized();
  emit modified();
}

void ApplicationWindow::updateWindowLists(MdiSubWindow *w)
{
  if (!w)
    return;

  if (hiddenWindows->contains(w))
    hiddenWindows->takeAt(hiddenWindows->indexOf(w));
}

void ApplicationWindow::closeActiveWindow()
{
  MdiSubWindow *w = activeWindow();
  if (w)
    w->close();
}

void ApplicationWindow::removeWindowFromLists(MdiSubWindow* w)
{
  if (!w)
    return;

  QString caption = w->objectName();
  if (w->inherits("Table")){
    Table* m=(Table*)w;
    for (int i=0; i<m->numCols(); i++){
      QString name=m->colName(i);
      removeCurves(name);
    }
  } else if (w->isA("MultiLayer")){
    MultiLayer *ml =  (MultiLayer*)w;
    Graph *g = ml->activeGraph();
    if (g)
      btnPointer->setChecked(true);
  } else if (w->isA("Matrix"))
  {
    remove3DMatrixPlots((Matrix*)w);
  }

  else
  {   mantidUI->removeWindowFromLists(w);
  }

  if (hiddenWindows->contains(w))
  {
    hiddenWindows->takeAt(hiddenWindows->indexOf(w));
  }
}

void ApplicationWindow::closeWindow(MdiSubWindow* window)
{
  if (!window)
    return;

  if (d_active_window == window)
    d_active_window = NULL;
  removeWindowFromLists(window);

  Folder *f = window->folder();
  f->removeWindow(window);

  //update list view in project explorer
  Q3ListViewItem *it=lv->findItem (window->objectName(), 0, Q3ListView::ExactMatch|Q3ListView::CaseSensitive);
  if (it)
    lv->takeItem(it);

  if (show_windows_policy == ActiveFolder && !f->windowsList().count()){
    customMenu(0);
    customToolBars(0);
  } else if (show_windows_policy == SubFolders && !(current_folder->children()).isEmpty()){
    FolderListItem *fi = current_folder->folderListItem();
    FolderListItem *item = (FolderListItem *)fi->firstChild();
    int initial_depth = item->depth();
    bool emptyFolder = true;
    while (item && item->depth() >= initial_depth){
      QList<MdiSubWindow *> lst = item->folder()->windowsList();
      if (lst.count() > 0){
        emptyFolder = false;
        break;
      }
      item = (FolderListItem *)item->itemBelow();
    }
    if (emptyFolder){
      customMenu(0);
      customToolBars(0);
    }
  }
  emit modified();
}

void ApplicationWindow::about()
{
  /*
QString text = "<h2>"+ versionString() + "</h2>";
text +=	"<h3>" + QString(copyright_string).replace("\n", "<br>") + "</h3>";
text += "<h3>" + tr("Released") + ": " + QString(release_date) + "</h3>";

text += "<h3> MantidPlot released: " + QString(MANTIDPLOT_RELEASE_DATE) + QString("</h3>");//Mantid

QMessageBox *mb = new QMessageBox();
mb->setWindowTitle (tr("About QtiPlot"));
mb->setWindowIcon (getQPixmap("logo_xpm"));
mb->setIconPixmap(getQPixmap("logo_xpm"));
mb->setText(text);
mb->exec();
   */
  //Mantid
  MantidAbout *ma = new MantidAbout();
  ma->exec();
  delete ma;
}

void ApplicationWindow::analysisMenuAboutToShow()
{
  analysisMenu->clear();
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->isA("MultiLayer")){
    QMenu *translateMenu = analysisMenu->addMenu (tr("&Translate"));
    translateMenu->addAction(actionTranslateVert);
    translateMenu->addAction(actionTranslateHor);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionDifferentiate);
    analysisMenu->addAction(actionIntegrate);
    analysisMenu->addAction(actionShowIntDialog);
    analysisMenu->insertSeparator();

    smoothMenu->clear();
    smoothMenu = analysisMenu->addMenu (tr("&Smooth"));
    smoothMenu->addAction(actionSmoothSavGol);
    smoothMenu->addAction(actionSmoothAverage);
    smoothMenu->addAction(actionSmoothFFT);

    filterMenu->clear();
    filterMenu = analysisMenu->addMenu (tr("&FFT filter"));
    filterMenu->addAction(actionLowPassFilter);
    filterMenu->addAction(actionHighPassFilter);
    filterMenu->addAction(actionBandPassFilter);
    filterMenu->addAction(actionBandBlockFilter);

    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionInterpolate);
    analysisMenu->addAction(actionFFT);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionFitLinear);
    analysisMenu->addAction(actionShowFitPolynomDialog);
    analysisMenu->insertSeparator();

    decayMenu->clear();
    decayMenu = analysisMenu->addMenu (tr("Fit E&xponential Decay"));
    decayMenu->addAction(actionShowExpDecayDialog);
    decayMenu->addAction(actionShowTwoExpDecayDialog);
    decayMenu->addAction(actionShowExpDecay3Dialog);

    analysisMenu->addAction(actionFitExpGrowth);
    analysisMenu->addAction(actionFitSigmoidal);
    analysisMenu->addAction(actionFitGauss);
    analysisMenu->addAction(actionFitLorentz);

    multiPeakMenu->clear();
    multiPeakMenu = analysisMenu->addMenu (tr("Fit &Multi-peak"));
    multiPeakMenu->addAction(actionMultiPeakGauss);
    multiPeakMenu->addAction(actionMultiPeakLorentz);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionShowFitDialog);
  } else if (w->isA("Matrix")){
    analysisMenu->addAction(actionIntegrate);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionFFT);
    analysisMenu->addAction(actionMatrixFFTDirect);
    analysisMenu->addAction(actionMatrixFFTInverse);
  } else if (w->inherits("Table")){
    analysisMenu->addAction(actionShowColStatistics);
    analysisMenu->addAction(actionShowRowStatistics);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionSortSelection);
    analysisMenu->addAction(actionSortTable);

    normMenu->clear();
    normMenu = analysisMenu->addMenu (tr("&Normalize"));
    normMenu->addAction(actionNormalizeSelection);
    normMenu->addAction(actionNormalizeTable);

    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionFFT);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionCorrelate);
    analysisMenu->addAction(actionAutoCorrelate);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionConvolute);
    analysisMenu->addAction(actionDeconvolute);
    analysisMenu->insertSeparator();
    analysisMenu->addAction(actionShowFitDialog);
  }
  reloadCustomActions();
}

void ApplicationWindow::matrixMenuAboutToShow()
{
  matrixMenu->clear();
  matrixMenu->addAction(actionSetMatrixProperties);
  matrixMenu->addAction(actionSetMatrixDimensions);
  matrixMenu->insertSeparator();
  matrixMenu->addAction(actionSetMatrixValues);
  matrixMenu->addAction(actionTableRecalculate);
  matrixMenu->insertSeparator();
  matrixMenu->addAction(actionRotateMatrix);
  matrixMenu->addAction(actionRotateMatrixMinus);
  matrixMenu->addAction(actionFlipMatrixVertically);
  matrixMenu->addAction(actionFlipMatrixHorizontally);
  matrixMenu->insertSeparator();
  matrixMenu->addAction(actionTransposeMatrix);
  matrixMenu->addAction(actionInvertMatrix);
  matrixMenu->addAction(actionMatrixDeterminant);
  matrixMenu->insertSeparator();
  matrixMenu->addAction(actionGoToRow);
  matrixMenu->addAction(actionGoToColumn);
  matrixMenu->insertSeparator();
  QMenu *matrixViewMenu = matrixMenu->addMenu (tr("Vie&w"));
  matrixViewMenu->addAction(actionViewMatrixImage);
  matrixViewMenu->addAction(actionViewMatrix);
  QMenu *matrixPaletteMenu = matrixMenu->addMenu (tr("&Palette"));
  matrixPaletteMenu->addAction(actionMatrixGrayScale);
  matrixPaletteMenu->addAction(actionMatrixRainbowScale);
  matrixPaletteMenu->addAction(actionMatrixCustomScale);
  matrixMenu->insertSeparator();
  matrixMenu->addAction(actionMatrixColumnRow);
  matrixMenu->addAction(actionMatrixXY);
  matrixMenu->insertSeparator();
  QMenu *convertToTableMenu = matrixMenu->addMenu (tr("&Convert to Spreadsheet"));
  convertToTableMenu->addAction(actionConvertMatrixDirect);
  convertToTableMenu->addAction(actionConvertMatrixXYZ);
  convertToTableMenu->addAction(actionConvertMatrixYXZ);

  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  actionViewMatrixImage->setChecked(m->viewType() == Matrix::ImageView);
  actionViewMatrix->setChecked(m->viewType() == Matrix::TableView);
  actionMatrixColumnRow->setChecked(m->headerViewType() == Matrix::ColumnRow);
  actionMatrixColumnRow->setEnabled(m->viewType() == Matrix::TableView);
  actionMatrixXY->setChecked(m->headerViewType() == Matrix::XY);
  actionMatrixXY->setEnabled(m->viewType() == Matrix::TableView);

  actionMatrixGrayScale->setChecked(m->colorMapType() == Matrix::GrayScale);
  actionMatrixRainbowScale->setChecked(m->colorMapType() == Matrix::Rainbow);
  actionMatrixCustomScale->setChecked(m->colorMapType() == Matrix::Custom);

  reloadCustomActions();
}

void ApplicationWindow::fileMenuAboutToShow()
{
  fileMenu->clear();
  newMenu->clear();
  exportPlotMenu->clear();

  newMenu = fileMenu->addMenu(tr("&New"));
  newMenu->addAction(actionNewProject);
  newMenu->addAction(actionNewFolder);
  newMenu->addAction(actionNewTable);
  newMenu->addAction(actionNewMatrix);
  newMenu->addAction(actionNewNote);
  newMenu->addAction(actionNewGraph);
  newMenu->addAction(actionNewFunctionPlot);
  newMenu->addAction(actionNewSurfacePlot);


  openMenu=fileMenu->addMenu(tr("&Load"));
  openMenu->addAction(actionOpenProj);
  openMenu->addAction(actionOpenRaw);
  openMenu->addAction(actionOpenNexus);
  openMenu->addAction(actionLoadFile);

  recentMenuID = fileMenu->insertItem(tr("&Recent Projects"), recent);

  fileMenu->insertSeparator();
  fileMenu->addAction(actionManageDirs);
  fileMenu->insertSeparator();
  fileMenu->addAction(actionLoadImage);

  MdiSubWindow *w = activeWindow();
  if (w && w->isA("Matrix"))
    fileMenu->addAction(actionExportMatrix);

  fileMenu->insertSeparator();
  fileMenu->addAction(actionSaveProjectAs);
  //fileMenu->insertSeparator();
  saveMenu=fileMenu->addMenu(tr("&Save"));
  saveMenu->addAction(actionSaveFile);
  saveMenu->addAction(actionSaveProject);

  fileMenu->insertSeparator();

  fileMenu->addAction(actionPrint);
  fileMenu->addAction(actionPrintAllPlots);
  fileMenu->insertSeparator();
  MdiSubWindow* t = activeWindow();
  if (t && (t->isA("Matrix") || t->inherits("Table") || t->isA("MantidMatrix")))
  {
    actionShowExportASCIIDialog->setEnabled(true);
  }
  else
  {
    actionShowExportASCIIDialog->setEnabled(false);
  }

  fileMenu->addAction(actionShowExportASCIIDialog);
  fileMenu->addAction(actionLoad);
  fileMenu->insertSeparator();
  fileMenu->addAction(actionclearAllMemory);
#ifdef USE_TCMALLOC
  fileMenu->addAction(actionreleaseFreeMemory);
#endif

  fileMenu->insertSeparator();
  fileMenu->addAction(actionCloseAllWindows);

  reloadCustomActions();
}

void ApplicationWindow::editMenuAboutToShow()
{
  MdiSubWindow *w = activeWindow();
  if (!w){
    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);
    return;
  }

  if (qobject_cast<Note *>(w)){
    ScriptEdit* doc = ((Note *)w)->editor();
    actionUndo->setEnabled(doc->isUndoAvailable());
    actionRedo->setEnabled(doc->isRedoAvailable());
  } else if (qobject_cast<Matrix *>(w)){
    QUndoStack *stack = ((Matrix *)w)->undoStack();
    actionUndo->setEnabled(stack->canUndo());
    actionRedo->setEnabled(stack->canRedo());
  } else {
    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);
  }

  reloadCustomActions();
}

void ApplicationWindow::windowsMenuAboutToShow()
{
  windowsMenu->clear();
  foldersMenu->clear();

  int folder_param = 0;
  Folder *f = projectFolder();
  while (f){
    int id;
    if (folder_param < 9)
      id = foldersMenu->insertItem("&" + QString::number(folder_param+1) + " " + f->path(), this, SLOT(foldersMenuActivated(int)));
    else
      id = foldersMenu->insertItem(f->path(), this, SLOT(foldersMenuActivated(int)));

    foldersMenu->setItemParameter(id, folder_param);
    folder_param++;
    foldersMenu->setItemChecked(id, f == current_folder);

    f = f->folderBelow();
  }

  windowsMenu->insertItem(tr("&Folders"), foldersMenu);
  windowsMenu->insertSeparator();

  QList<MdiSubWindow *> windows = current_folder->windowsList();
  int n = int(windows.count());
  if (!n ){
    /*#ifdef SCRIPTING_PYTHON
			windowsMenu->addAction(actionShowScriptWindow);
		#endif*/
    return;
  }

  windowsMenu->insertItem(tr("&Cascade"), this, SLOT(cascade()));
  windowsMenu->insertItem(tr("&Tile"), d_workspace, SLOT(tileSubWindows()));
  windowsMenu->insertSeparator();
  windowsMenu->addAction(actionNextWindow);
  windowsMenu->addAction(actionPrevWindow);
  windowsMenu->insertSeparator();
  windowsMenu->addAction(actionRename);

  windowsMenu->addAction(actionCopyWindow);
  MdiSubWindow* activeWin=activeWindow();
  if(!activeWin) return;
  if(activeWin->isA("MantidMatrix") || activeWin->isA("InstrumentWindow") )
  {
    actionCopyWindow->setEnabled(false);
  }
  else
  {
    actionCopyWindow->setEnabled(true);
  }

  windowsMenu->insertSeparator();


  windowsMenu->addAction(actionResizeActiveWindow);
  windowsMenu->insertItem(tr("&Hide Window"),
      this, SLOT(hideActiveWindow()));

  // Having the shorcut set here is neccessary on Windows, but
  // leads to an error message elsewhere. Don't know why and don't 
  // have a better solution than this right now.
#ifdef _WIN32
  windowsMenu->insertItem(getQPixmap("close_xpm"), tr("Close &Window"),  
                   this, SLOT(closeActiveWindow()), Qt::CTRL+Qt::Key_W );  
#else
  windowsMenu->insertItem(getQPixmap("close_xpm"), tr("Close &Window"),
			  this, SLOT(closeActiveWindow()) );
#endif

  if (n>0 && n<10){
    windowsMenu->insertSeparator();
    for (int i = 0; i<n; ++i ){
      int id = windowsMenu->insertItem(windows.at(i)->objectName(),
          this, SLOT( windowsMenuActivated( int ) ) );
      windowsMenu->setItemParameter( id, i );
      windowsMenu->setItemChecked( id, current_folder->activeWindow() == windows.at(i));
    }
  } else if (n>=10) {
    windowsMenu->insertSeparator();
    for ( int i = 0; i<9; ++i ){
      int id = windowsMenu->insertItem(windows.at(i)->objectName(),
          this, SLOT( windowsMenuActivated( int ) ) );
      windowsMenu->setItemParameter( id, i );
      windowsMenu->setItemChecked( id, activeWindow() == windows.at(i) );
    }
    windowsMenu->insertSeparator();
    windowsMenu->insertItem(tr("More windows..."),this, SLOT(showMoreWindows()));
  }
  reloadCustomActions();
}

void ApplicationWindow::showMarkerPopupMenu()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  QMenu markerMenu(this);

  if (g->imageMarkerSelected()){
    markerMenu.insertItem(getQPixmap("pixelProfile_xpm"),tr("&View Pixel Line profile"),this, SLOT(pixelLineProfile()));
    markerMenu.insertItem(tr("&Intensity Matrix"),this, SLOT(intensityTable()));
    markerMenu.insertSeparator();
  }

  markerMenu.insertItem(getQPixmap("cut_xpm"),tr("&Cut"),this, SLOT(cutSelection()));
  markerMenu.insertItem(getQPixmap("copy_xpm"), tr("&Copy"),this, SLOT(copySelection()));
  markerMenu.insertItem(getQPixmap("erase_xpm"), tr("&Delete"),this, SLOT(clearSelection()));
  markerMenu.insertSeparator();
  if (g->arrowMarkerSelected())
    markerMenu.insertItem(tr("&Properties..."),this, SLOT(showLineDialog()));
  else if (g->imageMarkerSelected())
    markerMenu.insertItem(tr("&Properties..."),this, SLOT(showImageDialog()));
  else
    markerMenu.insertItem(tr("&Properties..."),this, SLOT(showTextDialog()));

  markerMenu.exec(QCursor::pos());
}

void ApplicationWindow::showMoreWindows()
{
  if (explorerWindow->isVisible())
    QMessageBox::information(this, "MantidPlot",tr("Please use the project explorer to select a window!"));//Mantid
  else
    explorerWindow->show();
}

void ApplicationWindow::windowsMenuActivated( int id )
{
  QList<MdiSubWindow *> windows = current_folder->windowsList();
  MdiSubWindow* w = windows.at( id );
  if ( w ){
    w->showNormal();
    w->setFocus();
    if(hidden(w)){
      hiddenWindows->takeAt(hiddenWindows->indexOf(w));
      setListView(w->objectName(), tr("Normal"));
    }
    d_workspace->setActiveSubWindow(w);
  }
}

void ApplicationWindow::foldersMenuActivated( int id )
{
  int folder_param = 0;
  Folder *f = projectFolder();
  while (f){
    if (folder_param == id){
      changeFolder (f);
      return;
    }

    folder_param++;
    f = f->folderBelow();
  }
}

void ApplicationWindow::newProject()
{
  saveSettings();//the recent projects must be saved
  mantidUI->saveProject(saved);
  clearLogInfo();
  setWindowTitle(tr("MantidPlot - untitled"));//Mantid
}

void ApplicationWindow::savedProject()
{	QCoreApplication::processEvents();
if(actionSaveFile) actionSaveFile->setEnabled(false);
if(actionSaveProject)actionSaveProject->setEnabled(false);
saved = true;

Folder *f = projectFolder();
while (f){
  QList<MdiSubWindow *> folderWindows = f->windowsList();
  foreach(MdiSubWindow *w, folderWindows){
    if (w->isA("Matrix"))
      ((Matrix *)w)->undoStack()->setClean();
  }
  f = f->folderBelow();
}
}

void ApplicationWindow::modifiedProject()
{
  if (saved == false)
    return;
  if(actionSaveFile) actionSaveFile->setEnabled(true);
  if(actionSaveProject)actionSaveProject->setEnabled(true);
  saved = false;
}

void ApplicationWindow::modifiedProject(MdiSubWindow *)
{
  modifiedProject();
}

void ApplicationWindow::timerEvent ( QTimerEvent *e)
{
  if (e->timerId() == savingTimerId)
    saveProject();
  else
    QWidget::timerEvent(e);
}

void ApplicationWindow::dropEvent( QDropEvent* e )
{
  if (mantidUI->drop(e)) return;//Mantid

  QStringList fileNames;
  if (Q3UriDrag::decodeLocalFiles(e, fileNames)){
    QList<QByteArray> lst = QImageReader::supportedImageFormats() << "JPG";
    QStringList asciiFiles;

    for(int i = 0; i<(int)fileNames.count(); i++){
      QString fn = fileNames[i];
      QFileInfo fi (fn);
      QString ext = fi.extension().lower();
      QStringList tempList;
      QByteArray temp;
      // convert QList<QByteArray> to QStringList to be able to 'filter'
      foreach(temp,lst)
      tempList.append(QString(temp));
      QStringList l = tempList.filter(ext, Qt::CaseInsensitive);
      if (l.count()>0)
        loadImage(fn);
      else if ( ext == "opj" || ext == "qti")
        open(fn);
      else
        asciiFiles << fn;
    }

    importASCII(asciiFiles, ImportASCIIDialog::NewTables, columnSeparator, ignoredLines,
        renameColumns, strip_spaces, simplify_spaces, d_ASCII_import_comments,
        d_import_dec_separators, d_ASCII_import_locale, d_ASCII_comment_string,
        d_ASCII_import_read_only, d_ASCII_end_line,"");
  }
}

void ApplicationWindow::dragEnterEvent( QDragEnterEvent* e )
{
  if (e->source()){
    //e->ignore();//Mantid
    e->accept();//Mantid
    return;
  }
  else//Mantid
    e->accept(Q3UriDrag::canDecode(e));
}

//Mantid
void ApplicationWindow::dragMoveEvent( QDragMoveEvent* e )
{
  if (centralWidget()->geometry().contains(e->pos())) e->accept();
  else
    e->ignore();
}

void ApplicationWindow::closeEvent( QCloseEvent* ce )
{
#ifdef QTIPLOT_DEMO
  showDemoVersionMessage();
#endif

  // Mantid changes here

  if( scriptingEnv()->isRunning() )
  {
    if( QMessageBox::question(this, tr("MantidPlot"), "A script is still running, abort and quit application?", tr("Yes"), tr("No")) == 0 )
    {
      mantidUI->cancelAllRunningAlgorithms();
    }
    else
    {
      ce->ignore();
      return;
    }	  
  }

  if( !saved )
  {
    QString savemsg = tr("Save changes to project: <p><b> %1 </b> ?").arg(projectname);
    int result = QMessageBox::information(this, tr("MantidPlot"), savemsg, tr("Yes"), tr("No"), tr("Cancel"), 0, 2);
    if( result == 2 || (result == 0 && !saveProject()) )
    {
      ce->ignore();
      return;
    }
  }
  
  //Save the settings and exit
  saveSettings();
  mantidUI->shutdown();
  ce->accept();

}

void ApplicationWindow::customEvent(QEvent *e)
{
  if (e->type() == SCRIPTING_CHANGE_EVENT)
    scriptingChangeEvent((ScriptingChangeEvent*)e);
}

void ApplicationWindow::deleteSelectedItems()
{
  if (folders->hasFocus() && folders->currentItem() != folders->firstChild())
  {//we never allow the user to delete the project folder item
    deleteFolder();
    return;
  }

  Q3ListViewItem *item;
  QList<Q3ListViewItem *> lst;
  for (item = lv->firstChild(); item; item = item->nextSibling()){
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach(item, lst){
    if (item->rtti() == FolderListItem::RTTI){
      Folder *f = ((FolderListItem *)item)->folder();
      if (deleteFolder(f))
        delete item;
    } else
      ((WindowListItem *)item)->window()->close();
  }
  folders->blockSignals(false);
}

void ApplicationWindow::showListViewSelectionMenu(const QPoint &p)
{
  QMenu cm(this);
  cm.insertItem(tr("&Show All Windows"), this, SLOT(showSelectedWindows()));
  cm.insertItem(tr("&Hide All Windows"), this, SLOT(hideSelectedWindows()));
  cm.insertSeparator();
  cm.insertItem(tr("&Delete Selection"), this, SLOT(deleteSelectedItems()), Qt::Key_F8);
  cm.exec(p);
}

void ApplicationWindow::showListViewPopupMenu(const QPoint &p)
{
  QMenu cm(this);
  QMenu window(this);

  window.addAction(actionNewTable);
  window.addAction(actionNewMatrix);
  window.addAction(actionNewNote);
  window.addAction(actionNewGraph);
  window.addAction(actionNewFunctionPlot);
  window.addAction(actionNewSurfacePlot);
  cm.insertItem(tr("New &Window"), &window);

  cm.insertItem(getQPixmap("newfolder_xpm"), tr("New F&older"), this, SLOT(addFolder()), Qt::Key_F7);
  cm.insertSeparator();
  cm.insertItem(tr("Auto &Column Width"), lv, SLOT(adjustColumns()));
  cm.exec(p);
}

void ApplicationWindow::showWindowPopupMenu(Q3ListViewItem *it, const QPoint &p, int)
{
  if (folders->isRenaming())
    return;

  if (!it){
    showListViewPopupMenu(p);
    return;
  }

  Q3ListViewItem *item;
  int selected = 0;
  for (item = lv->firstChild(); item; item = item->nextSibling()){
    if (item->isSelected())
      selected++;

    if (selected>1){
      showListViewSelectionMenu(p);
      return;
    }
  }

  if (it->rtti() == FolderListItem::RTTI){
    current_folder = ((FolderListItem *)it)->folder();
    showFolderPopupMenu(it, p, false);
    return;
  }

  MdiSubWindow *w= ((WindowListItem *)it)->window();
  if (w){
    QMenu cm(this);
    QMenu plots(this);

    cm.addAction(actionActivateWindow);
    cm.addAction(actionMinimizeWindow);
    cm.addAction(actionMaximizeWindow);
    cm.insertSeparator();
    if (!hidden(w))
      cm.addAction(actionHideWindow);
    cm.insertItem(getQPixmap("close_xpm"), tr("&Delete Window"), w, SLOT(close()), Qt::Key_F8);
    cm.insertSeparator();
    cm.insertItem(tr("&Rename Window"), this, SLOT(renameWindow()), Qt::Key_F2);
    cm.addAction(actionResizeWindow);
    cm.insertSeparator();
    cm.insertItem(getQPixmap("fileprint_xpm"), tr("&Print Window"), w, SLOT(print()));
    cm.insertSeparator();
    cm.insertItem(tr("&Properties..."), this, SLOT(windowProperties()));

    if (w->inherits("Table")){
      QStringList graphs = dependingPlots(w->objectName());
      if (int(graphs.count())>0){
        cm.insertSeparator();
        for (int i=0;i<int(graphs.count());i++)
          plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

        cm.insertItem(tr("D&epending Graphs"),&plots);
      }
    } else if (w->isA("Matrix")){
      QStringList graphs = depending3DPlots((Matrix*)w);
      if (int(graphs.count())>0){
        cm.insertSeparator();
        for (int i=0;i<int(graphs.count());i++)
          plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

        cm.insertItem(tr("D&epending 3D Graphs"),&plots);
      }
    } else if (w->isA("MultiLayer")) {
      tablesDepend->clear();
      QStringList tbls=multilayerDependencies(w);
      int n = int(tbls.count());
      if (n > 0){
        cm.insertSeparator();
        for (int i=0; i<n; i++)
          tablesDepend->insertItem(tbls[i], i, -1);

        cm.insertItem(tr("D&epends on"), tablesDepend);
      }
    } else if (w->isA("Graph3D")) {
      Graph3D *sp=(Graph3D*)w;
      Matrix *m = sp->matrix();
      QString formula = sp->formula();
      if (!formula.isEmpty()){
        cm.insertSeparator();
        if (formula.contains("_")){
          QStringList tl = formula.split("_", QString::SkipEmptyParts);
          tablesDepend->clear();
          tablesDepend->insertItem(tl[0], 0, -1);
          cm.insertItem(tr("D&epends on"), tablesDepend);
        } else if (m) {
          plots.insertItem(m->objectName(), m, SLOT(showNormal()));
          cm.insertItem(tr("D&epends on"),&plots);
        } else {
          plots.insertItem(formula, w, SLOT(showNormal()));
          cm.insertItem(tr("Function"), &plots);
        }
      }
    }
    cm.exec(p);
  }
}

void ApplicationWindow::showTable(int i)
{
  Table *t = table(tablesDepend->text(i));
  if (!t)
    return;

  updateWindowLists(t);

  t->showMaximized();
  Q3ListViewItem *it=lv->findItem (t->objectName(), 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(2,tr("Maximized"));
}

void ApplicationWindow::showTable(const QString& curve)
{
  Table* w=table(curve);
  if (!w)
    return;

  updateWindowLists(w);
  int colIndex = w->colIndex(curve);
  w->setSelectedCol(colIndex);
  w->table()->clearSelection();
  w->table()->selectColumn(colIndex);
  w->showMaximized();
  Q3ListViewItem *it=lv->findItem (w->objectName(), 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
  if (it)
    it->setText(2,tr("Maximized"));
  emit modified();
}

QStringList ApplicationWindow::depending3DPlots(Matrix *m)
{
  QStringList plots;
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D") && ((Graph3D *)w)->matrix() == m)
      plots << w->objectName();
  }
  return plots;
}

QStringList ApplicationWindow::dependingPlots(const QString& name)
{
  QStringList onPlot, plots;

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers){
        onPlot = g->curvesList();
        onPlot = onPlot.grep (name,TRUE);
        if (int(onPlot.count()) && plots.contains(w->objectName())<=0)
          plots << w->objectName();
      }
    }else if (w->isA("Graph3D")){
      if ((((Graph3D*)w)->formula()).contains(name,TRUE) && plots.contains(w->objectName())<=0)
        plots << w->objectName();
    }
  }
  return plots;
}

QStringList ApplicationWindow::multilayerDependencies(QWidget *w)
{
  QStringList tables;
  MultiLayer *g=(MultiLayer*)w;
  QList<Graph *> layers = g->layersList();
  foreach(Graph *ag, layers){
    QStringList onPlot=ag->curvesList();
    for (int j=0; j<onPlot.count(); j++)
    {
      QStringList tl = onPlot[j].split("_", QString::SkipEmptyParts);
      if (tables.contains(tl[0])<=0)
        tables << tl[0];
    }
  }
  return tables;
}

void ApplicationWindow::showGraphContextMenu()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  QMenu cm(this);
  Graph* ag = (Graph*)plot->activeGraph();
  PlotToolInterface* tool = ag->activeTool();
  if (dynamic_cast<PeakPickerTool*>(tool))
  {
    dynamic_cast<PeakPickerTool*>(tool)->prepareContextMenu(cm);
    cm.exec(QCursor::pos());
    return;
  }

  QMenu axes(this);
  QMenu colour(this);
  QMenu exports(this);
  QMenu copy(this);
  QMenu prints(this);

  if (ag->isPiePlot())
    cm.insertItem(tr("Re&move Pie Curve"),ag, SLOT(removePie()));
  else {
    if (ag->visibleCurves() != ag->curves()){
      cm.addAction(actionShowAllCurves);
      cm.insertSeparator();
    }
    cm.addAction(actionShowCurvesDialog);
    cm.addAction(actionAddFunctionCurve);
    //cm.insertItem(tr("Anal&yze"), analysisMenu);
  }

  if (lastCopiedLayer){
    cm.insertSeparator();
    cm.insertItem(getQPixmap("paste_xpm"), tr("&Paste Layer"), this, SLOT(pasteSelection()));
  } else if (d_text_copy){
    cm.insertSeparator();
    cm.insertItem(getQPixmap("paste_xpm"), tr("&Paste Text"), plot, SIGNAL(pasteMarker()));
  } else if (d_arrow_copy){
    cm.insertSeparator();
    cm.insertItem(getQPixmap("paste_xpm"), tr("&Paste Line/Arrow"), plot, SIGNAL(pasteMarker()));
  } else if (d_image_copy){
    cm.insertSeparator();
    cm.insertItem(getQPixmap("paste_xpm"), tr("&Paste Image"), plot, SIGNAL(pasteMarker()));
  }
  cm.insertSeparator();
  axes.insertItem(tr("Lo&g(x),Log(y)"), ag, SLOT(logLogAxes()));
  axes.insertItem(tr("Log(&x),Linear(y)"), ag, SLOT(logXLinY()));
  axes.insertItem(tr("Linear(x),Log(&y)"), ag, SLOT(logYlinX()));
  axes.insertItem(tr("&Linear(x),Linear(y)"), ag, SLOT(linearAxes()));
  cm.insertItem(tr("&Axes"), &axes);

  colour.insertItem(tr("Lo&g Scale"), ag, SLOT(logColor()));
  colour.insertItem(tr("&Linear"), ag, SLOT(linColor()));
  cm.insertItem(tr("&Color Bar"), &colour);

  cm.insertSeparator();
  copy.insertItem(tr("&Layer"), this, SLOT(copyActiveLayer()));
  copy.insertItem(tr("&Window"), plot, SLOT(copyAllLayers()));
  cm.insertItem(getQPixmap("copy_xpm"), tr("&Copy"), &copy);

  exports.insertItem(tr("&Layer"), this, SLOT(exportLayer()));
  exports.insertItem(tr("&Window"), this, SLOT(exportGraph()));
  cm.insertItem(tr("E&xport"),&exports);

  prints.insertItem(tr("&Layer"), plot, SLOT(printActiveLayer()));
  prints.insertItem(tr("&Window"), plot, SLOT(print()));
  cm.insertItem(getQPixmap("fileprint_xpm"), tr("&Print"),&prints);
  cm.insertSeparator();
  cm.insertItem(tr("P&roperties..."), this, SLOT(showGeneralPlotDialog()));
  cm.insertSeparator();
  cm.insertItem(getQPixmap("close_xpm"), tr("&Delete Layer"), plot, SLOT(confirmRemoveLayer()));
  cm.exec(QCursor::pos());
}

void ApplicationWindow::showWindowContextMenu()
{
  MdiSubWindow* w = activeWindow();
  if (!w)
    return;


  QMenu cm(this);
  QMenu plot3D(this);
  if (w->isA("MultiLayer")){
    MultiLayer *g = (MultiLayer*)w;
    if (lastCopiedLayer){
      cm.insertItem(getQPixmap("paste_xpm"), tr("&Paste Layer"), this, SLOT(pasteSelection()));
      cm.insertSeparator();
    }

    cm.addAction(actionAddLayer);
    if (g->layers() != 0)
      cm.addAction(actionDeleteLayer);

    cm.addAction(actionShowLayerDialog);
    cm.insertSeparator();
    cm.addAction(actionRename);
    cm.addAction(actionCopyWindow);
    cm.insertSeparator();
    cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy Page"), g, SLOT(copyAllLayers()));
    cm.insertItem(tr("E&xport Page"), this, SLOT(exportGraph()));
    cm.addAction(actionPrint);
    cm.insertSeparator();
    cm.addAction(actionCloseWindow);
  } else if (w->isA("Graph3D")){
    Graph3D *g=(Graph3D*)w;
    if (!g->hasData()){
      cm.insertItem(tr("3D &Plot"), &plot3D);
      plot3D.addAction(actionAdd3DData);
      plot3D.insertItem(tr("&Matrix..."), this, SLOT(add3DMatrixPlot()));
      plot3D.addAction(actionEditSurfacePlot);
    } else {
      if (g->table())
        cm.insertItem(tr("Choose &Data Set..."), this, SLOT(change3DData()));
      else if (g->matrix())
        cm.insertItem(tr("Choose &Matrix..."), this, SLOT(change3DMatrix()));
      else if (g->userFunction() || g->parametricSurface())
        cm.addAction(actionEditSurfacePlot);
      cm.insertItem(getQPixmap("erase_xpm"), tr("C&lear"), g, SLOT(clearData()));
    }

    cm.insertSeparator();
    cm.addAction(actionRename);
    cm.addAction(actionCopyWindow);
    cm.insertSeparator();
    cm.insertItem(tr("&Copy Graph"), g, SLOT(copyImage()));
    cm.insertItem(tr("&Export"), this, SLOT(exportGraph()));
    cm.addAction(actionPrint);
    cm.insertSeparator();
    cm.addAction(actionCloseWindow);
  } else if (w->isA("Matrix")) {
    Matrix *t = (Matrix *)w;
    if (t->viewType() == Matrix::TableView){
      cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      cm.insertItem(getQPixmap("insert_row_xpm"), tr("&Insert Row"), t, SLOT(insertRow()));
      cm.insertItem(getQPixmap("insert_column_xpm"), tr("&Insert Column"), t, SLOT(insertColumn()));
      if (t->numSelectedRows() > 0)
        cm.insertItem(getQPixmap("delete_row_xpm"), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
      else if (t->numSelectedColumns() > 0)
        cm.insertItem(getQPixmap("delete_column_xpm"), tr("&Delete Columns"), t, SLOT(deleteSelectedColumns()));

      cm.insertItem(getQPixmap("erase_xpm"),tr("Clea&r"), t, SLOT(clearSelection()));
    } else if (t->viewType() == Matrix::ImageView){
      cm.addAction(actionImportImage);
      cm.addAction(actionExportMatrix);
      cm.insertSeparator();
      cm.addAction(actionSetMatrixProperties);
      cm.addAction(actionSetMatrixDimensions);
      cm.insertSeparator();
      cm.addAction(actionSetMatrixValues);
      cm.addAction(actionTableRecalculate);
      cm.insertSeparator();
      cm.addAction(actionRotateMatrix);
      cm.addAction(actionRotateMatrixMinus);
      cm.insertSeparator();
      cm.addAction(actionFlipMatrixVertically);
      cm.addAction(actionFlipMatrixHorizontally);
      cm.insertSeparator();
      cm.addAction(actionTransposeMatrix);
      cm.addAction(actionInvertMatrix);
    }
  } else
    mantidUI->showContextMenu(cm,w);//Mantid
  cm.exec(QCursor::pos());
}

void ApplicationWindow::customWindowTitleBarMenu(MdiSubWindow *w, QMenu *menu)
{
  menu->addAction(actionHideActiveWindow);
  menu->addSeparator();
  if (w->inherits("Table")){
    menu->addAction(actionShowExportASCIIDialog);
    menu->addSeparator();
  }

  if (w->isA("Note"))
    menu->addAction(actionSaveNote);
  else
    menu->addAction(actionSaveTemplate);
  menu->addAction(actionPrint);
  menu->addSeparator();
  menu->addAction(actionRename);
  menu->addAction(actionCopyWindow);
  menu->addSeparator();
}

void ApplicationWindow::showTableContextMenu(bool selection)
{
  Table *t = (Table*)activeWindow(TableWindow);
  if (!t)
    return;

  QMenu cm(this);
  if (selection){
    if ((int)t->selectedColumns().count() > 0){
      showColMenu(t->firstSelectedColumn());
      return;
    } else if (t->numSelectedRows() == 1) {
      cm.addAction(actionShowColumnValuesDialog);
      cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      cm.addAction(actionTableRecalculate);
      cm.insertItem(getQPixmap("insert_row_xpm"), tr("&Insert Row"), t, SLOT(insertRow()));
      cm.insertItem(getQPixmap("delete_row_xpm"), tr("&Delete Row"), t, SLOT(deleteSelectedRows()));
      cm.insertItem(getQPixmap("erase_xpm"), tr("Clea&r Row"), t, SLOT(clearSelection()));
      cm.insertSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numSelectedRows() > 1) {
      cm.addAction(actionShowColumnValuesDialog);
      cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      cm.addAction(actionTableRecalculate);
      cm.insertItem(getQPixmap("delete_row_xpm"), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
      cm.insertItem(getQPixmap("erase_xpm"),tr("Clea&r Rows"), t, SLOT(clearSelection()));
      cm.insertSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numRows() > 0 && t->numCols() > 0){
      cm.addAction(actionShowColumnValuesDialog);
      cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      cm.addAction(actionTableRecalculate);
      cm.insertItem(getQPixmap("erase_xpm"),tr("Clea&r"), t, SLOT(clearSelection()));
    }
  } else {
    cm.addAction(actionShowExportASCIIDialog);
    cm.insertSeparator();
    cm.addAction(actionAddColToTable);
    cm.addAction(actionClearTable);
    cm.insertSeparator();
    cm.addAction(actionGoToRow);
    cm.addAction(actionGoToColumn);
  }
  cm.exec(QCursor::pos());
}

void ApplicationWindow::chooseHelpFolder()
{
  QFileInfo hfi(helpFilePath);
  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the location of the MantidPlot help folder!"),
      hfi.dir().absolutePath(), 0 /**QFileDialog::ShowDirsOnly*/);

  if (!dir.isEmpty()){
    helpFilePath = dir + "index.html";

    QFile helpFile(helpFilePath);
    if (!helpFile.exists()){
      QMessageBox::critical(this, tr("MantidPlot - index.html File Not Found!"),//Mantid
          tr("There is no file called <b>index.html</b> in this folder.<br>Please choose another folder!"));
    }
  }
}

void ApplicationWindow::showStandAloneHelp()
{
#ifdef Q_OS_MAC // Mac
  QSettings settings(QSettings::IniFormat,QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
  QSettings settings;//(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif

  settings.beginGroup("/General");
  settings.beginGroup("/Paths");
  QString helpPath = settings.value("/HelpFile", qApp->applicationDirPath()+"/manual/index.html").toString();
  settings.endGroup();
  settings.endGroup();

  QFile helpFile(helpPath);
  if (!helpPath.isEmpty() && !helpFile.exists())
  {
    QMessageBox::critical(0, tr("MantidPlot - Help Files Not Found!"),//Mantid
        tr("The manual can be downloaded from the following internet address:")+
        "<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
    exit(0);
  }

  QFileInfo fi(helpPath);
  QString profilePath = QString(fi.dirPath(true)+"/qtiplot.adp");
  if (!QFile(profilePath).exists())
  {
    QMessageBox::critical(0, tr("MantidPlot - Help Profile Not Found!"),//Mantid
        tr("The assistant could not start because the file <b>%1</b> was not found in the help file directory!").arg("qtiplot.adp")+"<br>"+
        tr("This file is provided with the MantidPlot manual which can be downloaded from the following internet address:")+
        "<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
    exit(0);
  }

  QStringList cmdLst = QStringList() << "-profile" << profilePath;
//  QAssistantClient *assist = new QAssistantClient( QString(), 0);
//  assist->setArguments( cmdLst );
//  assist->showPage(helpPath);
//  connect(assist, SIGNAL(assistantClosed()), qApp, SLOT(quit()) );
}

void ApplicationWindow::showHelp()
{
  QFile helpFile(helpFilePath);
  if (!helpFile.exists())
  {
    QMessageBox::critical(this,tr("MantidPlot - Help Files Not Found!"),//Mantid
        tr("Please indicate the location of the help file!")+"<br>"+
        tr("The manual can be downloaded from the following internet address:")+
        "<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
    QString fn = QFileDialog::getOpenFileName(QDir::currentDirPath(), "*.html", this );
    if (!fn.isEmpty())
    {
      QFileInfo fi(fn);
      helpFilePath=fi.absFilePath();
      saveSettings();
    }
  }

  QFileInfo fi(helpFilePath);
  QString profilePath = QString(fi.dirPath(true)+"/qtiplot.adp");
  if (!QFile(profilePath).exists())
  {
    QMessageBox::critical(this,tr("MantidPlot - Help Profile Not Found!"),//Mantid
        tr("The assistant could not start because the file <b>%1</b> was not found in the help file directory!").arg("qtiplot.adp")+"<br>"+
        tr("This file is provided with the MantidPlot manual which can be downloaded from the following internet address:")+
        "<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
    return;
  }

  QStringList cmdLst = QStringList() << "-profile" << profilePath;
//  assistant->setArguments( cmdLst );
//  assistant->showPage(helpFilePath);
}

void ApplicationWindow::showPlotWizard()
{
  QStringList lst = tableNames();
  if (lst.count() > 0){
    PlotWizard* pw = new PlotWizard(this, 0);
    pw->setAttribute(Qt::WA_DeleteOnClose);
    connect (pw,SIGNAL(plot(const QStringList&)),this,SLOT(multilayerPlot(const QStringList&)));

    pw->insertTablesList(lst);
    pw->setColumnsList(columnsList(Table::All));
    pw->changeColumnsList(lst[0]);
    pw->exec();
  } else
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no tables available in this project.</h4>"
            "<p><h4>Please create a table and try again!</h4>"));
}

void ApplicationWindow::setCurveFullRange()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  int curveKey = actionCurveFullRange->data().toInt();
  g->setCurveFullRange(g->curveIndex(curveKey));
}

void ApplicationWindow::showCurveRangeDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  int curveKey = actionEditCurveRange->data().toInt();
  showCurveRangeDialog(g, g->curveIndex(curveKey));
}

CurveRangeDialog* ApplicationWindow::showCurveRangeDialog(Graph *g, int curve)
{
  if (!g)
    return 0;

  CurveRangeDialog* crd = new CurveRangeDialog(this);
  crd->setAttribute(Qt::WA_DeleteOnClose);
  crd->setCurveToModify(g, curve);
  crd->exec();
  return crd;
}

FunctionDialog* ApplicationWindow::showFunctionDialog()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return 0;

  Graph* g = plot->activeGraph();
  if (!g)
    return 0;

  int curveKey = actionEditFunction->data().toInt();
  return showFunctionDialog(g, g->curveIndex(curveKey));
}

FunctionDialog* ApplicationWindow::showFunctionDialog(Graph *g, int curve)
{
  if ( !g )
    return 0;

  FunctionDialog* fd = functionDialog();
  fd->setWindowTitle(tr("MantidPlot - Edit function"));//Mantid
  fd->setCurveToModify(g, curve);
  return fd;
}

FunctionDialog* ApplicationWindow::functionDialog()
{
  FunctionDialog* fd = new FunctionDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  connect (fd,SIGNAL(clearParamFunctionsList()),this,SLOT(clearParamFunctionsList()));
  connect (fd,SIGNAL(clearPolarFunctionsList()),this,SLOT(clearPolarFunctionsList()));

  fd->insertParamFunctionsList(xFunctions, yFunctions);
  fd->insertPolarFunctionsList(rFunctions, thetaFunctions);
  fd->show();
  fd->setActiveWindow();
  return fd;
}

void ApplicationWindow::addFunctionCurve()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = plot->activeGraph();
  if ( g ) {
    FunctionDialog* fd = functionDialog();
    if (fd)
      fd->setGraph(g);
  }
}

void ApplicationWindow::updateFunctionLists(int type, QStringList &formulas)
{
  int maxListSize = 10;
  if (type == 2)
  {
    rFunctions.remove(formulas[0]);
    rFunctions.push_front(formulas[0]);

    thetaFunctions.remove(formulas[1]);
    thetaFunctions.push_front(formulas[1]);

    while ((int)rFunctions.size() > maxListSize)
      rFunctions.pop_back();
    while ((int)thetaFunctions.size() > maxListSize)
      thetaFunctions.pop_back();
  }
  else if (type == 1)
  {
    xFunctions.remove(formulas[0]);
    xFunctions.push_front(formulas[0]);

    yFunctions.remove(formulas[1]);
    yFunctions.push_front(formulas[1]);

    while ((int)xFunctions.size() > maxListSize)
      xFunctions.pop_back();
    while ((int)yFunctions.size() > maxListSize)
      yFunctions.pop_back();
  }
}

MultiLayer * ApplicationWindow::newFunctionPlot(QStringList &formulas, double start, double end, int points, const QString& var, int type)
{
  MultiLayer *ml = newGraph();
  if (ml)
    ml->activeGraph()->addFunction(formulas, start, end, points, var, type);

  updateFunctionLists(type, formulas);
  return ml;
}

void ApplicationWindow::clearLogInfo()
{
  //if (!current_folder->logInfo().isEmpty()){
  current_folder->clearLogInfo();
  results->setText("");
  emit modified();
  //}
}

void ApplicationWindow::clearParamFunctionsList()
{
  xFunctions.clear();
  yFunctions.clear();
}

void ApplicationWindow::clearPolarFunctionsList()
{
  rFunctions.clear();
  thetaFunctions.clear();
}

void ApplicationWindow::clearSurfaceFunctionsList()
{
  surfaceFunc.clear();
}

void ApplicationWindow::setFramed3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFramed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::setBoxed3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setBoxed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::removeAxes3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setNoAxes();
  actionShowAxisDialog->setEnabled(false);
}

void ApplicationWindow::removeGrid3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setPolygonStyle();
}

void ApplicationWindow::setHiddenLineGrid3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setHiddenLineStyle();
}

void ApplicationWindow::setPoints3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setDotStyle();
}

void ApplicationWindow::setCones3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setConeStyle();
}

void ApplicationWindow::setCrosses3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setCrossStyle();
}

void ApplicationWindow::setBars3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setBarStyle();
}

void ApplicationWindow::setLineGrid3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setWireframeStyle();
}

void ApplicationWindow::setFilledMesh3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFilledMeshStyle();
}

void ApplicationWindow::setFloorData3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFloorData();
}

void ApplicationWindow::setFloorIso3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFloorIsolines();
}

void ApplicationWindow::setEmptyFloor3DPlot()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setEmptyFloor();
}

void ApplicationWindow::setFrontGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFrontGrid(on);
}

void ApplicationWindow::setBackGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setBackGrid(on);
}

void ApplicationWindow::setFloorGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setFloorGrid(on);
}

void ApplicationWindow::setCeilGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setCeilGrid(on);
}

void ApplicationWindow::setRightGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setRightGrid(on);
}

void ApplicationWindow::setLeftGrid3DPlot(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setLeftGrid(on);
}

void ApplicationWindow::pickPlotStyle( QAction* action )
{
  if (!action )
    return;

  if (action == polygon)
    removeGrid3DPlot();
  else if (action == filledmesh)
    setFilledMesh3DPlot();
  else if (action == wireframe)
    setLineGrid3DPlot();
  else if (action == hiddenline)
    setHiddenLineGrid3DPlot();
  else if (action == pointstyle)
    setPoints3DPlot();
  else if (action == conestyle)
    setCones3DPlot();
  else if (action == crossHairStyle)
    setCrosses3DPlot();
  else if (action == barstyle)
    setBars3DPlot();

  emit modified();
}


void ApplicationWindow::pickCoordSystem( QAction* action)
{
  if (!action)
    return;

  if (action == Box || action == Frame)
  {
    if (action == Box)
      setBoxed3DPlot();
    if (action == Frame)
      setFramed3DPlot();
    grids->setEnabled(true);
  }
  else if (action == None)
  {
    removeAxes3DPlot();
    grids->setEnabled(false);
  }

  emit modified();
}

void ApplicationWindow::pickFloorStyle( QAction* action )
{
  if (!action)
    return;

  if (action == floordata)
    setFloorData3DPlot();
  else if (action == flooriso)
    setFloorIso3DPlot();
  else
    setEmptyFloor3DPlot();

  emit modified();
}

void ApplicationWindow::custom3DActions(QMdiSubWindow *w)
{
  if (w && w->isA("Graph3D"))
  {
    Graph3D* plot = (Graph3D*)w;
    actionAnimate->setOn(plot->isAnimated());
    actionPerspective->setOn(!plot->isOrthogonal());
    switch(plot->plotStyle())
    {
    case FILLEDMESH:
      wireframe->setChecked( false );
      hiddenline->setChecked( false );
      polygon->setChecked( false );
      filledmesh->setChecked( true );
      pointstyle->setChecked( false );
      barstyle->setChecked( false );
      conestyle->setChecked( false );
      crossHairStyle->setChecked( false );
      break;

    case FILLED:
      wireframe->setChecked( false );
      hiddenline->setChecked( false );
      polygon->setChecked( true );
      filledmesh->setChecked( false );
      pointstyle->setChecked( false );
      barstyle->setChecked( false );
      conestyle->setChecked( false );
      crossHairStyle->setChecked( false );
      break;

    case Qwt3D::USER:
      wireframe->setChecked( false );
      hiddenline->setChecked( false );
      polygon->setChecked( false );
      filledmesh->setChecked( false );

      if (plot->pointType() == Graph3D::VerticalBars)
      {
        pointstyle->setChecked( false );
        conestyle->setChecked( false );
        crossHairStyle->setChecked( false );
        barstyle->setChecked( true );
      }
      else if (plot->pointType() == Graph3D::Dots)
      {
        pointstyle->setChecked( true );
        barstyle->setChecked( false );
        conestyle->setChecked( false );
        crossHairStyle->setChecked( false );
      }
      else if (plot->pointType() == Graph3D::HairCross)
      {
        pointstyle->setChecked( false );
        barstyle->setChecked( false );
        conestyle->setChecked( false );
        crossHairStyle->setChecked( true );
      }
      else if (plot->pointType() == Graph3D::Cones)
      {
        pointstyle->setChecked( false );
        barstyle->setChecked( false );
        conestyle->setChecked( true );
        crossHairStyle->setChecked( false );
      }
      break;

    case WIREFRAME:
      wireframe->setChecked( true );
      hiddenline->setChecked( false );
      polygon->setChecked( false );
      filledmesh->setChecked( false );
      pointstyle->setChecked( false );
      barstyle->setChecked( false );
      conestyle->setChecked( false );
      crossHairStyle->setChecked( false );
      break;

    case HIDDENLINE:
      wireframe->setChecked( false );
      hiddenline->setChecked( true );
      polygon->setChecked( false );
      filledmesh->setChecked( false );
      pointstyle->setChecked( false );
      barstyle->setChecked( false );
      conestyle->setChecked( false );
      crossHairStyle->setChecked( false );
      break;

    default:
      break;
    }

    switch(plot->coordStyle())
    {
    case Qwt3D::NOCOORD:
      None->setChecked( true );
      Box->setChecked( false );
      Frame->setChecked( false );
      break;

    case Qwt3D::BOX:
      None->setChecked( false );
      Box->setChecked( true );
      Frame->setChecked( false );
      break;

    case Qwt3D::FRAME:
      None->setChecked(false );
      Box->setChecked( false );
      Frame->setChecked(true );
      break;
    }

    switch(plot->floorStyle())
    {
    case NOFLOOR:
      floornone->setChecked( true );
      flooriso->setChecked( false );
      floordata->setChecked( false );
      break;

    case FLOORISO:
      floornone->setChecked( false );
      flooriso->setChecked( true );
      floordata->setChecked( false );
      break;

    case FLOORDATA:
      floornone->setChecked(false );
      flooriso->setChecked( false );
      floordata->setChecked(true );
      break;
    }
    custom3DGrids(plot->grids());
  }
}

void ApplicationWindow::custom3DGrids(int grids)
{
  if (Qwt3D::BACK & grids)
    back->setChecked(true);
  else
    back->setChecked(false);

  if (Qwt3D::FRONT & grids)
    front->setChecked(true);
  else
    front->setChecked(false);

  if (Qwt3D::CEIL & grids)
    ceil->setChecked(true);
  else
    ceil->setChecked(false);

  if (Qwt3D::FLOOR & grids)
    floor->setChecked(true);
  else
    floor->setChecked(false);

  if (Qwt3D::RIGHT & grids)
    right->setChecked(true);
  else
    right->setChecked(false);

  if (Qwt3D::LEFT & grids)
    left->setChecked(true);
  else
    left->setChecked(false);
}

void ApplicationWindow::initPlot3DToolBar()
{
  plot3DTools = new QToolBar( tr( "3D Surface" ), this );
  plot3DTools->setObjectName("plot3DTools"); // this is needed for QMainWindow::restoreState()
  plot3DTools->setIconSize( QSize(20,20) );
  addToolBarBreak( Qt::TopToolBarArea );
  addToolBar( Qt::TopToolBarArea, plot3DTools );

  coord = new QActionGroup( this );
  Box = new QAction( coord );
  Box->setIcon(QIcon(getQPixmap("box_xpm")));
  Box->setCheckable(true);

  Frame = new QAction( coord );
  Frame->setIcon(QIcon(getQPixmap("free_axes_xpm")) );
  Frame->setCheckable(true);

  None = new QAction( coord );
  None->setIcon(QIcon(getQPixmap("no_axes_xpm")) );
  None->setCheckable(true);

  plot3DTools->addAction(Frame);
  plot3DTools->addAction(Box);
  plot3DTools->addAction(None);
  Box->setChecked( true );

  plot3DTools->addSeparator();

  // grid actions
  grids = new QActionGroup( this );
  grids->setEnabled( true );
  grids->setExclusive( false );
  front = new QAction( grids );
  front->setCheckable( true );
  front->setIcon(QIcon(getQPixmap("frontGrid_xpm")) );
  back = new QAction( grids );
  back->setCheckable( true );
  back->setIcon(QIcon(getQPixmap("backGrid_xpm")));
  right = new QAction( grids );
  right->setCheckable( true );
  right->setIcon(QIcon(getQPixmap("leftGrid_xpm")) );
  left = new QAction( grids );
  left->setCheckable( true );
  left->setIcon(QIcon(getQPixmap("rightGrid_xpm")));
  ceil = new QAction( grids );
  ceil->setCheckable( true );
  ceil->setIcon(QIcon(getQPixmap("ceilGrid_xpm")) );
  floor = new QAction( grids );
  floor->setCheckable( true );
  floor->setIcon(QIcon(getQPixmap("floorGrid_xpm")) );

  plot3DTools->addAction(front);
  plot3DTools->addAction(back);
  plot3DTools->addAction(right);
  plot3DTools->addAction(left);
  plot3DTools->addAction(ceil);
  plot3DTools->addAction(floor);

  plot3DTools->addSeparator();

  actionPerspective = new QAction( this );
  actionPerspective->setToggleAction( TRUE );
  actionPerspective->setIconSet(getQPixmap("perspective_xpm") );
  actionPerspective->addTo( plot3DTools );
  actionPerspective->setOn(!orthogonal3DPlots);
  connect(actionPerspective, SIGNAL(toggled(bool)), this, SLOT(togglePerspective(bool)));

  actionResetRotation = new QAction( this );
  actionResetRotation->setToggleAction( false );
  actionResetRotation->setIconSet(getQPixmap("reset_rotation_xpm"));
  actionResetRotation->addTo( plot3DTools );
  connect(actionResetRotation, SIGNAL(activated()), this, SLOT(resetRotation()));

  actionFitFrame = new QAction( this );
  actionFitFrame->setToggleAction( false );
  actionFitFrame->setIconSet(getQPixmap("fit_frame_xpm"));
  actionFitFrame->addTo( plot3DTools );
  connect(actionFitFrame, SIGNAL(activated()), this, SLOT(fitFrameToLayer()));

  plot3DTools->addSeparator();

  //plot style actions
  plotstyle = new QActionGroup( this );
  wireframe = new QAction( plotstyle );
  wireframe->setCheckable( true );
  wireframe->setEnabled( true );
  wireframe->setIcon(QIcon(getQPixmap("lineMesh_xpm")) );
  hiddenline = new QAction( plotstyle );
  hiddenline->setCheckable( true );
  hiddenline->setEnabled( true );
  hiddenline->setIcon(QIcon(getQPixmap("grid_only_xpm")) );
  polygon = new QAction( plotstyle );
  polygon->setCheckable( true );
  polygon->setEnabled( true );
  polygon->setIcon(QIcon(getQPixmap("no_grid_xpm")));
  filledmesh = new QAction( plotstyle );
  filledmesh->setCheckable( true );
  filledmesh->setIcon(QIcon(getQPixmap("grid_poly_xpm")) );
  pointstyle = new QAction( plotstyle );
  pointstyle->setCheckable( true );
  pointstyle->setIcon(QIcon(getQPixmap("pointsMesh_xpm")) );

  conestyle = new QAction( plotstyle );
  conestyle->setCheckable( true );
  conestyle->setIcon(QIcon(getQPixmap("cones_xpm")) );

  crossHairStyle = new QAction( plotstyle );
  crossHairStyle->setCheckable( true );
  crossHairStyle->setIcon(QIcon(getQPixmap("crosses_xpm")) );

  barstyle = new QAction( plotstyle );
  barstyle->setCheckable( true );
  barstyle->setIcon(QIcon(getQPixmap("plot_bars_xpm")) );

  plot3DTools->addAction(barstyle);
  plot3DTools->addAction(pointstyle);

  plot3DTools->addAction(conestyle);
  plot3DTools->addAction(crossHairStyle);
  plot3DTools->addSeparator();

  plot3DTools->addAction(wireframe);
  plot3DTools->addAction(hiddenline);
  plot3DTools->addAction(polygon);
  plot3DTools->addAction(filledmesh);
  filledmesh->setChecked( true );

  plot3DTools->addSeparator();

  //floor actions
  floorstyle = new QActionGroup( this );
  floordata = new QAction( floorstyle );
  floordata->setCheckable( true );
  floordata->setIcon(QIcon(getQPixmap("floor_xpm")) );
  flooriso = new QAction( floorstyle );
  flooriso->setCheckable( true );
  flooriso->setIcon(QIcon(getQPixmap("isolines_xpm")) );
  floornone = new QAction( floorstyle );
  floornone->setCheckable( true );
  floornone->setIcon(QIcon(getQPixmap("no_floor_xpm")));

  plot3DTools->addAction(floordata);
  plot3DTools->addAction(flooriso);
  plot3DTools->addAction(floornone);
  floornone->setChecked( true );

  plot3DTools->addSeparator();

  actionAnimate = new QAction( this );
  actionAnimate->setToggleAction( true );
  actionAnimate->setIconSet(getQPixmap("movie_xpm"));
  plot3DTools->addAction(actionAnimate);

  plot3DTools->hide();

  connect(actionAnimate, SIGNAL(toggled(bool)), this, SLOT(toggle3DAnimation(bool)));
  connect( coord, SIGNAL( triggered( QAction* ) ), this, SLOT( pickCoordSystem( QAction* ) ) );
  connect( floorstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickFloorStyle( QAction* ) ) );
  connect( plotstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickPlotStyle( QAction* ) ) );

  connect( left, SIGNAL( triggered( bool ) ), this, SLOT( setLeftGrid3DPlot(bool) ));
  connect( right, SIGNAL( triggered( bool ) ), this, SLOT( setRightGrid3DPlot( bool ) ) );
  connect( ceil, SIGNAL( triggered( bool ) ), this, SLOT( setCeilGrid3DPlot( bool ) ) );
  connect( floor, SIGNAL( triggered( bool ) ), this, SLOT(setFloorGrid3DPlot( bool ) ) );
  connect( back, SIGNAL( triggered( bool ) ), this, SLOT(setBackGrid3DPlot( bool ) ) );
  connect( front, SIGNAL( triggered( bool ) ), this, SLOT( setFrontGrid3DPlot( bool ) ) );
}

void ApplicationWindow::pixelLineProfile()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  bool ok;
  int res = QInputDialog::getInteger(
      tr("MantidPlot - Set the number of pixels to average"), tr("Number of averaged pixels"),1, 1, 2000, 2,//Mantid
      &ok, this );
  if ( !ok )
    return;

  LineProfileTool *lpt = new LineProfileTool(g, this, res);
  g->setActiveTool(lpt);
}

void ApplicationWindow::intensityTable()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g){
    ImageMarker *im = (ImageMarker *) g->selectedMarkerPtr();
    if (im){
      QString fn = im->fileName();
      if (!fn.isEmpty())
        importImage(fn);
    }
  }
}

void ApplicationWindow::autoArrangeLayers()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  plot->setMargins(5, 5, 5, 5);
  plot->setSpacing(5, 5);
  plot->arrangeLayers(true, false);
}

void ApplicationWindow::addLayer()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  switch(QMessageBox::information(this,
      tr("MantidPlot - Guess best origin for the new layer?"),//Mantid
      tr("Do you want MantidPlot to guess the best position for the new layer?\n Warning: this will rearrange existing layers!"),//Mantid
      tr("&Guess"), tr("&Top-left corner"), tr("&Cancel"), 0, 2 ) )
  {
  case 0:
  {
    setPreferences(plot->addLayer());
    plot->arrangeLayers(true, false);
  }
  break;

  case 1:
    setPreferences(plot->addLayer(0, 0, plot->size().width(), plot->size().height()));
    break;

  case 2:
    return;
    break;
  }
}

void ApplicationWindow::deleteLayer()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  plot->confirmRemoveLayer();
}

Note* ApplicationWindow::openNote(ApplicationWindow* app, const QStringList &flist)
{
  QStringList lst = flist[0].split("\t", QString::SkipEmptyParts);
  QString caption = lst[0];
  Note* w = app->newNote(caption);
  if (lst.count() == 2){
    app->setListViewDate(caption, lst[1]);
    w->setBirthDate(lst[1]);
  }
  restoreWindowGeometry(app, w, flist[1]);

  lst=flist[2].split("\t");
  w->setWindowLabel(lst[1]);
  w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
  return w;
}

Matrix* ApplicationWindow::openMatrix(ApplicationWindow* app, const QStringList &flist)
{
  QStringList::const_iterator line = flist.begin();

  QStringList list=(*line).split("\t");
  QString caption=list[0];
  int rows = list[1].toInt();
  int cols = list[2].toInt();

  Matrix* w = app->newMatrix(caption, rows, cols);
  app->setListViewDate(caption,list[3]);
  w->setBirthDate(list[3]);

  for (line++; line!=flist.end(); line++)
  {
    QStringList fields = (*line).split("\t");
    if (fields[0] == "geometry") {
      restoreWindowGeometry(app, w, *line);
    } else if (fields[0] == "ColWidth") {
      w->setColumnsWidth(fields[1].toInt());
    } else if (fields[0] == "Formula") {
      w->setFormula(fields[1]);
    } else if (fields[0] == "<formula>") {
      QString formula;
      for (line++; line!=flist.end() && *line != "</formula>"; line++)
        formula += *line + "\n";
      formula.truncate(formula.length()-1);
      w->setFormula(formula);
    } else if (fields[0] == "TextFormat") {
      if (fields[1] == "f")
        w->setTextFormat('f', fields[2].toInt());
      else
        w->setTextFormat('e', fields[2].toInt());
    } else if (fields[0] == "WindowLabel") { // d_file_version > 71
      w->setWindowLabel(fields[1]);
      w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
    } else if (fields[0] == "Coordinates") { // d_file_version > 81
      w->setCoordinates(fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble(), fields[4].toDouble());
    } else if (fields[0] == "ViewType") { // d_file_version > 90
      w->setViewType((Matrix::ViewType)fields[1].toInt());
    } else if (fields[0] == "HeaderViewType") { // d_file_version > 90
      w->setHeaderViewType((Matrix::HeaderViewType)fields[1].toInt());
    } else if (fields[0] == "ColorPolicy"){// d_file_version > 90
      w->setColorMapType((Matrix::ColorMapType)fields[1].toInt());
    } else if (fields[0] == "<ColorMap>"){// d_file_version > 90
      QStringList lst;
      while ( *line != "</ColorMap>" ){
        line++;
        lst << *line;
      }
      lst.pop_back();
      w->setColorMap(lst);
    } else // <data> or values
      break;
  }
  if (*line == "<data>") line++;

  //read and set table values
  for (; line!=flist.end() && *line != "</data>"; line++){
    QStringList fields = (*line).split("\t");
    int row = fields[0].toInt();
    for (int col=0; col<cols; col++){
      QString cell = fields[col+1];
      if (cell.isEmpty())
        continue;

      if (d_file_version < 90)
        w->setCell(row, col, QLocale::c().toDouble(cell));
      else if (d_file_version == 90)
        w->setText(row, col, cell);
      else
        w->setCell(row, col, cell.toDouble());
    }
    qApp->processEvents(QEventLoop::ExcludeUserInput);
  }
  w->resetView();
  return w;
}
void ApplicationWindow::openMantidMatrix(const QStringList &list)
{ 	
  QString s=list[0];
  QStringList qlist=s.split("\t");
  QString wsName=qlist[1];
  MantidMatrix *m=newMantidMatrix(wsName,-1,-1);//mantidUI->importMatrixWorkspace(wsName,-1,-1,false,false);
  //if(!m)throw std::runtime_error("Error on opening matrixworkspace ");
  if(!m) return;
  //adding the mantid matrix windows opened to a list.
  //this list is used for find the MantidMatrix window pointer to open a 3D/2DGraph
  m_mantidmatrixWindows<<m;
  QStringList::const_iterator line = list.begin();
  for (line++; line!=list.end(); line++)
  {	QStringList fields = (*line).split("\t");
  if (fields[0] == "geometry" || fields[0] == "tgeometry") {
    restoreWindowGeometry(this, m, *line);
  }
  }
}
void ApplicationWindow::openInstrumentWindow(const QStringList &list)
{
  QString s=list[0];
  QStringList qlist=s.split("\t");
  QString wsName=qlist[1];
  InstrumentWindow *insWin = mantidUI->getInstrumentView(wsName);
  if(!insWin) return;
  insWin->show();
  QStringList::const_iterator line = list.begin();
  for (line++; line!=list.end(); line++)
  {	QStringList fields = (*line).split("\t");
  if (fields[0] == "geometry" || fields[0] == "tgeometry") {
    restoreWindowGeometry(this, insWin, *line);
  }
  }
}
/** This method opens script window when  project file is loaded
 */
void ApplicationWindow::openScriptWindow(const QStringList &list)
{	showScriptWindow();
if(!scriptingWindow) return;
scriptingWindow->setWindowTitle("MantidPlot: " + scriptingEnv()->scriptingLanguage() + " Window");
QString s=list[0];
QStringList scriptnames=s.split("\t");
int count=scriptnames.size();
if(count==0) return;
// don't create a new tab when the first script file from theproject file  opened
if(!scriptnames[1].isEmpty()) scriptingWindow->open(scriptnames[1],false);
// create a new tab  and open the script for all otehr filenames
for(int i=2;i<count;++i)
{   if(!scriptnames[i].isEmpty())scriptingWindow->open(scriptnames[i],true);
}
}
/** This method populates the mantid workspace tree when  project file is loaded 
 */
void ApplicationWindow::populateMantidTreeWdiget(const QString &s)
{	
  QStringList list = s.split("\t");
  QStringList::const_iterator line = list.begin();
  for (++line; line!=list.end(); ++line)
  {	std::string wsName=(*line).toStdString();
  if(wsName.empty())throw std::runtime_error("Workspace Name not found in project file ");
  std::string fileName(workingDir.toStdString()+"/"+wsName);
  fileName.append(".nxs");
  try
  {
    mantidUI->loaddataFromNexusFile(wsName,fileName,true);
  }
  catch(...)
  {
  }
  }
}
/** This method opens mantid matrix window when  project file is loaded 
 */
MantidMatrix* ApplicationWindow::newMantidMatrix(const QString& wsName,int lower,int upper)
{	
  MantidMatrix* m=mantidUI->openMatrixWorkspace(this,wsName,lower,upper);
  return m;
}
Table* ApplicationWindow::openTable(ApplicationWindow* app, const QStringList &flist)
{
  QStringList::const_iterator line = flist.begin();

  QStringList list=(*line).split("\t");
  QString caption=list[0];
  int rows = list[1].toInt();
  int cols = list[2].toInt();

  Table* w = app->newTable(caption, rows, cols);
  app->setListViewDate(caption, list[3]);
  w->setBirthDate(list[3]);

  for (line++; line!=flist.end(); line++)
  {
    QStringList fields = (*line).split("\t");
    if (fields[0] == "geometry" || fields[0] == "tgeometry") {
      restoreWindowGeometry(app, w, *line);
    } else if (fields[0] == "header") {
      fields.pop_front();
      if (d_file_version >= 78)
        w->loadHeader(fields);
      else
      {
        w->setColPlotDesignation(list[4].toInt(), Table::X);
        w->setColPlotDesignation(list[6].toInt(), Table::Y);
        w->setHeader(fields);
      }
    } else if (fields[0] == "ColWidth") {
      fields.pop_front();
      w->setColWidths(fields);
    } else if (fields[0] == "com") { // legacy code
      w->setCommands(*line);
    } else if (fields[0] == "<com>") {
      for (line++; line!=flist.end() && *line != "</com>"; line++)
      {
        int col = (*line).mid(9,(*line).length()-11).toInt();
        QString formula;
        for (line++; line!=flist.end() && *line != "</col>"; line++)
          formula += *line + "\n";
        formula.truncate(formula.length()-1);
        w->setCommand(col,formula);
      }
    } else if (fields[0] == "ColType") { // d_file_version > 65
      fields.pop_front();
      w->setColumnTypes(fields);
    } else if (fields[0] == "Comments") { // d_file_version > 71
      fields.pop_front();
      w->setColComments(fields);
      w->setHeaderColType();
    } else if (fields[0] == "WindowLabel") { // d_file_version > 71
      w->setWindowLabel(fields[1]);
      w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
    } else if (fields[0] == "ReadOnlyColumn") { // d_file_version > 91
      fields.pop_front();
      for (int i=0; i < w->numCols(); i++)
        w->setReadOnlyColumn(i, fields[i] == "1");
    } else if (fields[0] == "HiddenColumn") { // d_file_version >= 93
      fields.pop_front();
      for (int i=0; i < w->numCols(); i++)
        w->hideColumn(i, fields[i] == "1");
    } else // <data> or values
      break;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  w->table()->blockSignals(true);
  for (line++; line!=flist.end() && *line != "</data>"; line++)
  {//read and set table values
    QStringList fields = (*line).split("\t");
    int row = fields[0].toInt();
    for (int col=0; col<cols; col++){
      if (fields.count() >= col+2){
        QString cell = fields[col+1];
        if (cell.isEmpty())
          continue;

        if (w->columnType(col) == Table::Numeric){
          if (d_file_version < 90)
            w->setCell(row, col, QLocale::c().toDouble(cell.replace(",", ".")));
          else if (d_file_version == 90)
            w->setText(row, col, cell);
          else if (d_file_version >= 91)
            w->setCell(row, col, cell.toDouble());
        } else
          w->setText(row, col, cell);
      }
    }
    QApplication::processEvents(QEventLoop::ExcludeUserInput);
  }
  QApplication::restoreOverrideCursor();

  w->setSpecifications(w->saveToString("geometry\n"));
  w->table()->blockSignals(false);
  return w;
}

TableStatistics* ApplicationWindow::openTableStatistics(const QStringList &flist)
{
  QStringList::const_iterator line = flist.begin();

  QStringList list=(*line++).split("\t");
  QString caption=list[0];

  QList<int> targets;
  for (int i=1; i <= (*line).count('\t'); i++)
    targets << (*line).section('\t',i,i).toInt();

  TableStatistics* w = newTableStatistics(table(list[1]),
      list[2]=="row" ? TableStatistics::row : TableStatistics::column, targets, caption);

  setListViewDate(caption,list[3]);
  w->setBirthDate(list[3]);

  for (line++; line!=flist.end(); line++)
  {
    QStringList fields = (*line).split("\t");
    if (fields[0] == "geometry"){
      restoreWindowGeometry(this, w, *line);}
    else if (fields[0] == "header") {
      fields.pop_front();
      if (d_file_version >= 78)
        w->loadHeader(fields);
      else
      {
        w->setColPlotDesignation(list[4].toInt(), Table::X);
        w->setColPlotDesignation(list[6].toInt(), Table::Y);
        w->setHeader(fields);
      }
    } else if (fields[0] == "ColWidth") {
      fields.pop_front();
      w->setColWidths(fields);
    } else if (fields[0] == "com") { // legacy code
      w->setCommands(*line);
    } else if (fields[0] == "<com>") {
      for (line++; line!=flist.end() && *line != "</com>"; line++)
      {
        int col = (*line).mid(9,(*line).length()-11).toInt();
        QString formula;
        for (line++; line!=flist.end() && *line != "</col>"; line++)
          formula += *line + "\n";
        formula.truncate(formula.length()-1);
        w->setCommand(col,formula);
      }
    } else if (fields[0] == "ColType") { // d_file_version > 65
      fields.pop_front();
      w->setColumnTypes(fields);
    } else if (fields[0] == "Comments") { // d_file_version > 71
      fields.pop_front();
      w->setColComments(fields);
    } else if (fields[0] == "WindowLabel") { // d_file_version > 71
      w->setWindowLabel(fields[1]);
      w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
    }
  }
  return w;
}

Graph* ApplicationWindow::openGraph(ApplicationWindow* app, MultiLayer *plot,
    const QStringList &list)
{
  Graph* ag = 0;
  int curveID = 0;
  QString wsName;
  for (int j=0;j<(int)list.count()-1;j++){
    QString s=list[j];
    if (s.contains ("ggeometry")){
      QStringList fList=s.split("\t");
      ag =(Graph*)plot->addLayer(fList[1].toInt(), fList[2].toInt(),
          fList[3].toInt(), fList[4].toInt());

      ag->blockSignals(true);
      ag->enableAutoscaling(autoscale2DPlots);

    }
    else if( s.contains("MantidCurve")) //1D plot curves
    {
      QStringList curvelst=s.split("\t");
      if( !curvelst[1].isEmpty()&& !curvelst[2].isEmpty())
      {
        try {
          new MantidCurve(curvelst[1],ag,curvelst[2],curvelst[3].toInt(),curvelst[4].toInt());
        } catch (Mantid::Kernel::Exception::NotFoundError &) {
          // Get here if workspace name is invalid - shouldn't be possible, but just in case
          closeWindow(plot);
          return 0;

        } catch (std::invalid_argument&) {
          // Get here if invalid spectrum number given - shouldn't be possible, but just in case
          // plot->askOnCloseEvent(false);
          //plot->close();
          closeWindow(plot);
          return 0;
        }
      }
    }
    else if (s.left(10) == "Background"){
      QStringList fList = s.split("\t");
      QColor c = QColor(fList[1]);
      if (fList.count() == 3)
        c.setAlpha(fList[2].toInt());
      ag->setBackgroundColor(c);
    }
    else if (s.contains ("Margin")){
      QStringList fList=s.split("\t");
      ag->plotWidget()->setMargin(fList[1].toInt());
    }
    else if (s.contains ("Border")){
      QStringList fList=s.split("\t");
      ag->setFrame(fList[1].toInt(), QColor(fList[2]));
    }
    else if (s.contains ("EnabledAxes")){
      QStringList fList=s.split("\t");
      fList.pop_front();
      for (int i=0; i<(int)fList.count(); i++)
        ag->enableAxis(i, fList[i].toInt());
    }
    else if (s.contains ("AxesBaseline")){
      QStringList fList = s.split("\t", QString::SkipEmptyParts);
      fList.pop_front();
      for (int i=0; i<(int)fList.count(); i++)
        ag->setAxisMargin(i, fList[i].toInt());
    }
    else if (s.contains ("EnabledTicks"))
    {//version < 0.8.6
      QStringList fList=s.split("\t");
      fList.pop_front();
      fList.replaceInStrings("-1", "3");
      ag->setMajorTicksType(fList);
      ag->setMinorTicksType(fList);
    }
    else if (s.contains ("MajorTicks"))
    {//version >= 0.8.6
      QStringList fList=s.split("\t");
      fList.pop_front();
      ag->setMajorTicksType(fList);
    }
    else if (s.contains ("MinorTicks"))
    {//version >= 0.8.6
      QStringList fList=s.split("\t");
      fList.pop_front();
      ag->setMinorTicksType(fList);
    }
    else if (s.contains ("TicksLength")){
      QStringList fList=s.split("\t");
      ag->setTicksLength(fList[1].toInt(), fList[2].toInt());
    }
    else if (s.contains ("EnabledTickLabels")){
      QStringList fList=s.split("\t");
      fList.pop_front();
      for (int i=0; i<int(fList.count()); i++)
        ag->enableAxisLabels(i, fList[i].toInt());
    }
    else if (s.contains ("AxesColors")){
      QStringList fList = s.split("\t");
      fList.pop_front();
      for (int i=0; i<int(fList.count()); i++)
        ag->setAxisColor(i, QColor(fList[i]));
    }
    else if (s.contains ("AxesNumberColors")){
      QStringList fList=QStringList::split ("\t",s,TRUE);
      fList.pop_front();
      for (int i=0; i<int(fList.count()); i++)
        ag->setAxisLabelsColor(i, QColor(fList[i]));
    }
    else if (s.left(5)=="grid\t"){
      ag->plotWidget()->grid()->load(s.split("\t"));
    }
    else if (s.startsWith ("<Antialiasing>") && s.endsWith ("</Antialiasing>")){
      bool antialiasing = s.remove("<Antialiasing>").remove("</Antialiasing>").toInt();
      ag->setAntialiasing(antialiasing);
    }
    else if (s.contains ("PieCurve")){
      QStringList curve=s.split("\t");
      if (!app->renamedTables.isEmpty()){
        QString caption = (curve[1]).left((curve[1]).find("_",0));
        if (app->renamedTables.contains(caption))
        {//modify the name of the curve according to the new table name
          int index = app->renamedTables.findIndex(caption);
          QString newCaption = app->renamedTables[++index];
          curve.replaceInStrings(caption+"_", newCaption+"_");
        }
      }
      QPen pen = QPen(QColor(curve[3]), curve[2].toDouble(),Graph::getPenStyle(curve[4]));

      Table *table = app->table(curve[1]);
      if (table){
        int startRow = 0;
        int endRow = table->numRows() - 1;
        int first_color = curve[7].toInt();
        bool visible = true;
        if (d_file_version >= 90)
        {
          startRow = curve[8].toInt();
          endRow = curve[9].toInt();
          visible = curve[10].toInt();
        }

        if (d_file_version <= 89)
          first_color = convertOldToNewColorIndex(first_color);

        if (curve.size() >= 22){//version 0.9.3-rc3
          ag->plotPie(table, curve[1], pen, curve[5].toInt(),
              curve[6].toInt(), first_color, startRow, endRow, visible,
              curve[11].toDouble(), curve[12].toDouble(), curve[13].toDouble(),
              curve[14].toDouble(), curve[15].toDouble(), curve[16].toInt(),
              curve[17].toInt(), curve[18].toInt(), curve[19].toInt(),
              curve[20].toInt(), curve[21].toInt());
        } else
          ag->plotPie(table, curve[1], pen, curve[5].toInt(),
              curve[6].toInt(), first_color, startRow, endRow, visible);
      }
    }else if (s.left(6)=="curve\t"){
      QStringList curve = s.split("\t", QString::SkipEmptyParts);
      if (!app->renamedTables.isEmpty()){
        QString caption = (curve[2]).left((curve[2]).find("_",0));
        if (app->renamedTables.contains(caption))
        {//modify the name of the curve according to the new table name
          int index = app->renamedTables.findIndex (caption);
          QString newCaption = app->renamedTables[++index];
          curve.replaceInStrings(caption+"_", newCaption+"_");
        }
      }

      CurveLayout cl;
      cl.connectType=curve[4].toInt();
      cl.lCol=curve[5].toInt();
      if (d_file_version <= 89)
        cl.lCol = convertOldToNewColorIndex(cl.lCol);
      cl.lStyle=curve[6].toInt();
      cl.lWidth=curve[7].toDouble();
      cl.sSize=curve[8].toInt();
      if (d_file_version <= 78)
        cl.sType=Graph::obsoleteSymbolStyle(curve[9].toInt());
      else
        cl.sType=curve[9].toInt();

      cl.symCol=curve[10].toInt();
      if (d_file_version <= 89)
        cl.symCol = convertOldToNewColorIndex(cl.symCol);
      cl.fillCol=curve[11].toInt();
      if (d_file_version <= 89)
        cl.fillCol = convertOldToNewColorIndex(cl.fillCol);
      cl.filledArea=curve[12].toInt();
      cl.aCol=curve[13].toInt();
      if (d_file_version <= 89)
        cl.aCol = convertOldToNewColorIndex(cl.aCol);
      cl.aStyle=curve[14].toInt();
      if(curve.count() < 16)
        cl.penWidth = cl.lWidth;
      else if ((d_file_version >= 79) && (curve[3].toInt() == Graph::Box))
        cl.penWidth = curve[15].toDouble();
      else if ((d_file_version >= 78) && (curve[3].toInt() <= Graph::LineSymbols))
        cl.penWidth = curve[15].toDouble();
      else
        cl.penWidth = cl.lWidth;

      int plotType = curve[3].toInt();
      Table *w = app->table(curve[2]);
      PlotCurve *c = NULL;
      if (w){
        if(plotType == Graph::VectXYXY || plotType == Graph::VectXYAM){
          QStringList colsList;
          colsList<<curve[2]; colsList<<curve[20]; colsList<<curve[21];
          if (d_file_version < 72)
            colsList.prepend(w->colName(curve[1].toInt()));
          else
            colsList.prepend(curve[1]);

          int startRow = 0;
          int endRow = -1;
          if (d_file_version >= 90){
            startRow = curve[curve.count()-3].toInt();
            endRow = curve[curve.count()-2].toInt();
          }

          c = (PlotCurve *)ag->plotVectorCurve(w, colsList, plotType, startRow, endRow);

          if (d_file_version <= 77){
            int temp_index = convertOldToNewColorIndex(curve[15].toInt());
            ag->updateVectorsLayout(curveID, ColorBox::color(temp_index), curve[16].toDouble(), curve[17].toInt(),
                curve[18].toInt(), curve[19].toInt(), 0, curve[20], curve[21]);
          } else {
            if(plotType == Graph::VectXYXY)
              ag->updateVectorsLayout(curveID, curve[15], curve[16].toDouble(),
                  curve[17].toInt(), curve[18].toInt(), curve[19].toInt(), 0);
            else
              ag->updateVectorsLayout(curveID, curve[15], curve[16].toDouble(), curve[17].toInt(),
                  curve[18].toInt(), curve[19].toInt(), curve[22].toInt());
          }
        } else if (plotType == Graph::Box)
          c = (PlotCurve *)ag->openBoxDiagram(w, curve, d_file_version);
        else {
          if (d_file_version < 72)
            c = (PlotCurve *)ag->insertCurve(w, curve[1].toInt(), curve[2], plotType);
          else if (d_file_version < 90)
            c = (PlotCurve *)ag->insertCurve(w, curve[1], curve[2], plotType);
          else{
            int startRow = curve[curve.count()-3].toInt();
            int endRow = curve[curve.count()-2].toInt();
            c = (PlotCurve *)ag->insertCurve(w, curve[1], curve[2], plotType, startRow, endRow);
          }
        }

        if(plotType == Graph::Histogram){
          QwtHistogram *h = (QwtHistogram *)ag->curve(curveID);
          if (d_file_version <= 76)
            h->setBinning(curve[16].toInt(),curve[17].toDouble(),curve[18].toDouble(),curve[19].toDouble());
          else
            h->setBinning(curve[17].toInt(),curve[18].toDouble(),curve[19].toDouble(),curve[20].toDouble());
          h->loadData();
        }

        if(plotType == Graph::VerticalBars || plotType == Graph::HorizontalBars ||
            plotType == Graph::Histogram){
          if (d_file_version <= 76)
            ag->setBarsGap(curveID, curve[15].toInt(), 0);
          else
            ag->setBarsGap(curveID, curve[15].toInt(), curve[16].toInt());
        }
        ag->updateCurveLayout(c, &cl);
        if (d_file_version >= 88){
          if (c && c->rtti() == QwtPlotItem::Rtti_PlotCurve){
            if (d_file_version < 90)
              c->setAxis(curve[curve.count()-2].toInt(), curve[curve.count()-1].toInt());
            else {
              c->setAxis(curve[curve.count()-5].toInt(), curve[curve.count()-4].toInt());
              c->setVisible(curve.last().toInt());
            }
          }
        }
      } else if(plotType == Graph::Histogram){//histograms from matrices
        Matrix *m = app->matrix(curve[2]);
        QwtHistogram *h = ag->restoreHistogram(m, curve);
        ag->updateCurveLayout(h, &cl);
      }
      curveID++;
    } else if (s == "<CurveLabels>"){
      QStringList lst;
      while ( s!="</CurveLabels>" ){
        s = list[++j];
        lst << s;
      }
      lst.pop_back();
      ag->restoreCurveLabels(curveID - 1, lst);
    } else if (s == "<Function>"){//version 0.9.5
      curveID++;
      QStringList lst;
      while ( s != "</Function>" ){
        s = list[++j];
        lst << s;
      }
      lst.pop_back();
      ag->restoreFunction(lst);
    } else if (s.contains ("FunctionCurve")){
      QStringList curve = s.split("\t");
      CurveLayout cl;
      cl.connectType=curve[6].toInt();
      cl.lCol=curve[7].toInt();
      cl.lStyle=curve[8].toInt();
      cl.lWidth=curve[9].toDouble();
      cl.sSize=curve[10].toInt();
      cl.sType=curve[11].toInt();
      cl.symCol=curve[12].toInt();
      cl.fillCol=curve[13].toInt();
      cl.filledArea=curve[14].toInt();
      cl.aCol=curve[15].toInt();
      cl.aStyle=curve[16].toInt();
      int current_index = 17;
      if(curve.count() < 16)
        cl.penWidth = cl.lWidth;
      else if ((d_file_version >= 79) && (curve[5].toInt() == Graph::Box))
      {
        cl.penWidth = curve[17].toDouble();
        current_index++;
      }
      else if ((d_file_version >= 78) && (curve[5].toInt() <= Graph::LineSymbols))
      {
        cl.penWidth = curve[17].toDouble();
        current_index++;
      }
      else
        cl.penWidth = cl.lWidth;

      PlotCurve *c = (PlotCurve *)ag->insertFunctionCurve(curve[1], curve[2].toInt(), d_file_version);
      ag->setCurveType(curveID, curve[5].toInt());
      ag->updateCurveLayout(c, &cl);
      if (d_file_version >= 88){
        QwtPlotCurve *c = ag->curve(curveID);
        if (c){
          if(current_index + 1 < curve.size())
            c->setAxis(curve[current_index].toInt(), curve[current_index+1].toInt());
          if (d_file_version >= 90 && current_index+2 < curve.size())
            c->setVisible(curve.last().toInt());
          else
            c->setVisible(true);
        }

      }
      curveID++;
    }
    else if (s.contains ("ErrorBars")){
      QStringList curve = s.split("\t", QString::SkipEmptyParts);
      if (!app->renamedTables.isEmpty()){
        QString caption = (curve[4]).left((curve[4]).find("_",0));
        if (app->renamedTables.contains(caption))
        {//modify the name of the curve according to the new table name
          int index = app->renamedTables.findIndex (caption);
          QString newCaption = app->renamedTables[++index];
          curve.replaceInStrings(caption+"_", newCaption+"_");
        }
      }
      Table *w = app->table(curve[3]);
      Table *errTable = app->table(curve[4]);
      if (w && errTable){
        ag->addErrorBars(curve[2], curve[3], errTable, curve[4], curve[1].toInt(),
            curve[5].toDouble(), curve[6].toInt(), QColor(curve[7]),
            curve[8].toInt(), curve[10].toInt(), curve[9].toInt());
      }
      curveID++;
    }
    else if (s == "<spectrogram>"){
      QStringList lst;
      lst<<list[0];
      lst<<list[1];
      QString lineone=lst[1];
      QStringList lineonelst=lineone.split("\t");
      QString name=lineonelst[1];
      QStringList qlist=name.split(" ");
      std::string specgramwsName =qlist[1].toStdString();

      lst.clear();
      while ( s!="</spectrogram>" ){
        s = list[++j];
        lst << s;
      }
      lst.pop_back();
      Spectrogram* sp=openSpectrogram(ag,specgramwsName,lst);
      if(!sp)
      {	  closeWindow(plot);
      return 0;
      }
      curveID++;
    }
    else if (s.left(6) == "scale\t"){
      QStringList scl = s.split("\t");
      scl.pop_front();
      int size = scl.count();
      if (d_file_version < 88){
        double step = scl[2].toDouble();
        if (scl[5] == "0")
          step = 0.0;
        ag->setScale(QwtPlot::xBottom, scl[0].toDouble(), scl[1].toDouble(), step,
            scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));
        ag->setScale(QwtPlot::xTop, scl[0].toDouble(), scl[1].toDouble(), step,
            scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));

        step = scl[10].toDouble();
        if (scl[13] == "0")
          step = 0.0;
        ag->setScale(QwtPlot::yLeft, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
            scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
        ag->setScale(QwtPlot::yRight, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
            scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
      }
      else if (size == 8){
        ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
            scl[4].toInt(), scl[5].toInt(),  scl[6].toInt(), bool(scl[7].toInt()));
      }
      else if (size == 9){

        if(scl[8].toInt()==1)
        {	//if axis details like scale,majortick,minor tick changed
          ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
              scl[4].toInt(), scl[5].toInt(),  scl[6].toInt(), bool(scl[7].toInt()));
        }
      }
      else if (size == 18){
        ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
            scl[4].toInt(), scl[5].toInt(), scl[6].toInt(), bool(scl[7].toInt()), scl[8].toDouble(),
            scl[9].toDouble(), scl[10].toInt(), scl[11].toDouble(), scl[12].toDouble(), scl[13].toInt(),
            scl[14].toInt(), bool(scl[15].toInt()), scl[16].toInt(), bool(scl[17].toInt()));
      }
      else if (size == 19){
        //if axis details scale,majortick,minor tick changed
        if(scl[8].toInt()==1)
          ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
              scl[4].toInt(), scl[5].toInt(), scl[6].toInt(), bool(scl[7].toInt()), scl[8].toDouble(),
              scl[9].toDouble(), scl[10].toInt(), scl[11].toDouble(), scl[12].toDouble(), scl[13].toInt(),
              scl[14].toInt(), bool(scl[15].toInt()), scl[16].toInt(), bool(scl[17].toInt()));
      }
    }
    else if (s.contains ("PlotTitle")){
      QStringList fList=s.split("\t");
      wsName=fList[1].split(" ")[1];
      ag->setTitle(fList[1]);
      ag->setTitleColor(QColor(fList[2]));
      ag->setTitleAlignment(fList[3].toInt());
    }
    else if (s.contains ("TitleFont")){
      QStringList fList=s.split("\t");
      QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
      fnt.setUnderline(fList[5].toInt());
      fnt.setStrikeOut(fList[6].toInt());
      ag->setTitleFont(fnt);
    }
    else if (s.contains ("AxesTitles")){
      QStringList lst=s.split("\t");
      lst.pop_front();
      for (int i=0; i<4; i++){
        if (lst.count() > i)
          ag->setScaleTitle(i, lst[i]);
      }
    }
    else if (s.contains ("AxesTitleColors")){
      QStringList colors = s.split("\t", QString::SkipEmptyParts);
      colors.pop_front();
      for (int i=0; i<int(colors.count()); i++)
        ag->setAxisTitleColor(i, colors[i]);
    }else if (s.contains ("AxesTitleAlignment")){
      QStringList align=s.split("\t", QString::SkipEmptyParts);
      align.pop_front();
      for (int i=0; i<(int)align.count(); i++)
        ag->setAxisTitleAlignment(i, align[i].toInt());
    }else if (s.contains ("ScaleFont")){
      QStringList fList=s.split("\t");
      QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
      fnt.setUnderline(fList[5].toInt());
      fnt.setStrikeOut(fList[6].toInt());

      int axis=(fList[0].right(1)).toInt();
      ag->setAxisTitleFont(axis,fnt);
    }else if (s.contains ("AxisFont")){
      QStringList fList=s.split("\t");
      QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
      fnt.setUnderline(fList[5].toInt());
      fnt.setStrikeOut(fList[6].toInt());

      int axis=(fList[0].right(1)).toInt();
      ag->setAxisFont(axis,fnt);
    }
    else if (s.contains ("AxesFormulas"))
    {	QStringList fList=s.split("\t");
    fList.remove(fList.first());
    for (int i=0; i<(int)fList.count(); i++)
      ag->setAxisFormula(i, fList[i]);
    }
    else if (s.startsWith("<AxisFormula "))
    {
      int axis = s.mid(18,s.length()-20).toInt();
      QString formula;
      for (j++; j<(int)list.count() && list[j] != "</AxisFormula>"; j++)
        formula += list[j] + "\n";
      formula.truncate(formula.length()-1);
      ag->setAxisFormula(axis, formula);
    }
    else if (s.contains ("LabelsFormat"))
    {
      QStringList fList=s.split("\t");
      fList.pop_front();
      ag->setLabelsNumericFormat(fList);
    }
    else if (s.contains ("LabelsRotation"))
    {
      QStringList fList=s.split("\t");
      ag->setAxisLabelRotation(QwtPlot::xBottom, fList[1].toInt());
      ag->setAxisLabelRotation(QwtPlot::xTop, fList[2].toInt());
    }
    else if (s.contains ("DrawAxesBackbone"))
    {
      QStringList fList=s.split("\t");
      ag->loadAxesOptions(fList[1]);
    }
    else if (s.contains ("AxesLineWidth"))
    {
      QStringList fList=s.split("\t");
      ag->loadAxesLinewidth(fList[1].toInt());
    }
    else if (s.contains ("CanvasFrame")){
      QStringList lst = s.split("\t");
      ag->setCanvasFrame(lst[1].toInt(), QColor(lst[2]));
    }
    else if (s.contains ("CanvasBackground"))
    {
      QStringList list = s.split("\t");
      QColor c = QColor(list[1]);
      if (list.count() == 3)
        c.setAlpha(list[2].toInt());
      ag->setCanvasBackground(c);
    }
    else if (s.contains ("Legend"))
    {// version <= 0.8.9
      QStringList fList = QStringList::split ("\t",s, true);
      ag->insertLegend(fList, d_file_version);
    }
    else if (s.startsWith ("<legend>") && s.endsWith ("</legend>"))
    {
      QStringList fList = QStringList::split ("\t", s.remove("</legend>"), true);
      ag->insertLegend(fList, d_file_version);
    }
    else if (s.contains ("textMarker"))
    {// version <= 0.8.9
      QStringList fList = QStringList::split ("\t",s, true);
      ag->insertText(fList, d_file_version);
    }
    else if (s.startsWith ("<text>") && s.endsWith ("</text>"))
    {
      QStringList fList = QStringList::split ("\t", s.remove("</text>"), true);
      ag->insertText(fList, d_file_version);
    }
    else if (s.startsWith ("<PieLabel>") && s.endsWith ("</PieLabel>"))
    {
      QStringList fList = QStringList::split ("\t", s.remove("</PieLabel>"), true);
      ag->insertText(fList, d_file_version);
    }
    else if (s.contains ("lineMarker"))
    {// version <= 0.8.9
      QStringList fList=s.split("\t");
      ag->addArrow(fList, d_file_version);
    }
    else if (s.startsWith ("<line>") && s.endsWith ("</line>"))
    {
      QStringList fList=s.remove("</line>").split("\t");
      ag->addArrow(fList, d_file_version);
    }
    else if (s.contains ("ImageMarker") || (s.startsWith ("<image>") && s.endsWith ("</image>")))
    {
      QStringList fList=s.remove("</image>").split("\t");
      ag->insertImageMarker(fList, d_file_version);
    }
    else if (s.contains("AxisType"))
    {
      QStringList fList = s.split("\t");
      for (int i=0; i<4; i++){
        QStringList lst = fList[i+1].split(";", QString::SkipEmptyParts);
        int format = lst[0].toInt();
        if (format == ScaleDraw::Numeric)
          continue;
        if (format == ScaleDraw::Day)
          ag->setLabelsDayFormat(i, lst[1].toInt());
        else if (format == ScaleDraw::Month)
          ag->setLabelsMonthFormat(i, lst[1].toInt());
        else if (format == ScaleDraw::Time || format == ScaleDraw::Date)
          ag->setLabelsDateTimeFormat(i, format, lst[1]+";"+lst[2]);
        else if (lst.size() > 1)
          ag->setLabelsTextFormat(i, format, lst[1], app->table(lst[1]));
      }
    }
    else if (d_file_version < 69 && s.contains ("AxesTickLabelsCol"))
    {
      QStringList fList = s.split("\t");
      for (int i=0; i<4; i++){
        QString colName = fList[i+1];
        Table *nw = app->table(colName);
        ag->setLabelsTextFormat(i, ag->axisType(i), colName, nw);
      }
    }
  }
  ag->replot();

  ag->blockSignals(false);
  ag->setIgnoreResizeEvents(!app->autoResizeLayers);
  ag->setAutoscaleFonts(app->autoScaleFonts);
  return ag;
}

Graph3D* ApplicationWindow::openSurfacePlot(ApplicationWindow* app, const QStringList &lst)
{
  QStringList fList=lst[0].split("\t");
  QString caption=fList[0];
  QString date=fList[1];
  if (date.isEmpty())
    date = QDateTime::currentDateTime().toString(Qt::LocalDate);

  fList=lst[2].split("\t", QString::SkipEmptyParts);
  Graph3D *plot=0;

  if (fList[1].endsWith("(Y)",true))
    plot=app->dataPlot3D(caption, fList[1],fList[2].toDouble(),fList[3].toDouble(),
        fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());
  else if (fList[1].contains("(Z)",true) > 0)
    plot=app->openPlotXYZ(caption, fList[1], fList[2].toDouble(),fList[3].toDouble(),
        fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());
  else if (fList[1].startsWith("matrix<",true) && fList[1].endsWith(">",false))
    plot=app->openMatrixPlot3D(caption, fList[1], fList[2].toDouble(),fList[3].toDouble(),
        fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());

  else if (fList[1].contains("mantidMatrix3D"))
  {
    QString linefive=lst[5];
    QStringList linefivelst=linefive.split("\t");
    QString name=linefivelst[1];
    QStringList qlist=name.split(" ");
    std::string graph3DwsName=qlist[1].toStdString();
    MantidMatrix *m=0;
    QList<MantidMatrix*>::const_iterator matrixItr;;
    for( matrixItr=m_mantidmatrixWindows.begin();matrixItr!=m_mantidmatrixWindows.end();++matrixItr)
    {
      if(graph3DwsName==(*matrixItr)->getWorkspaceName()) m=*matrixItr;
    }
    QString linethree=lst[3];
    qlist.clear();
    qlist=linethree.split("\t");
    int style=qlist[1].toInt();
    if(m)plot=m->plotGraph3D(style);
    if(!plot)
    {
      closeWindow(plot);
      return 0;
    }
  }
  else if (fList[1].contains(",")){
    QStringList l = fList[1].split(",", QString::SkipEmptyParts);
    plot = app->plotParametricSurface(l[0], l[1], l[2], l[3].toDouble(), l[4].toDouble(),
        l[5].toDouble(), l[6].toDouble(), l[7].toInt(), l[8].toInt(), l[9].toInt(), l[10].toInt());
    app->setWindowName(plot, caption);
  } else {
    QStringList l = fList[1].split(";", QString::SkipEmptyParts);
    if (l.count() == 1)
      plot = app->plotSurface(fList[1], fList[2].toDouble(), fList[3].toDouble(),
          fList[4].toDouble(), fList[5].toDouble(), fList[6].toDouble(), fList[7].toDouble());
    else if (l.count() == 3)
      plot = app->plotSurface(l[0], fList[2].toDouble(), fList[3].toDouble(), fList[4].toDouble(),
          fList[5].toDouble(), fList[6].toDouble(), fList[7].toDouble(), l[1].toInt(), l[2].toInt());
    app->setWindowName(plot, caption);
  }

  if (!plot)
    return 0;

  app->setListViewDate(caption, date);
  plot->setBirthDate(date);
  plot->setIgnoreFonts(true);
  restoreWindowGeometry(app, plot, lst[1]);

  fList=lst[4].split("\t", QString::SkipEmptyParts);
  plot->setGrid(fList[1].toInt());

  plot->setTitle(lst[5].split("\t"));
  plot->setColors(lst[6].split("\t", QString::SkipEmptyParts));

  fList=lst[7].split("\t", QString::SkipEmptyParts);
  fList.pop_front();
  plot->setAxesLabels(fList);

  plot->setTicks(lst[8].split("\t", QString::SkipEmptyParts));
  plot->setTickLengths(lst[9].split("\t", QString::SkipEmptyParts));
  plot->setOptions(lst[10].split("\t", QString::SkipEmptyParts));
  plot->setNumbersFont(lst[11].split("\t", QString::SkipEmptyParts));
  plot->setXAxisLabelFont(lst[12].split("\t", QString::SkipEmptyParts));
  plot->setYAxisLabelFont(lst[13].split("\t", QString::SkipEmptyParts));
  plot->setZAxisLabelFont(lst[14].split("\t", QString::SkipEmptyParts));

  fList=lst[15].split("\t", QString::SkipEmptyParts);
  plot->setRotation(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

  fList=lst[16].split("\t", QString::SkipEmptyParts);
  plot->setZoom(fList[1].toDouble());

  fList=lst[17].split("\t", QString::SkipEmptyParts);
  plot->setScale(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

  fList=lst[18].split("\t", QString::SkipEmptyParts);
  plot->setShift(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

  fList=lst[19].split("\t", QString::SkipEmptyParts);
  plot->setMeshLineWidth(fList[1].toDouble());

  if (d_file_version > 71){
    fList=lst[20].split("\t"); // using QString::SkipEmptyParts here causes a crash for empty window labels
    plot->setWindowLabel(fList[1]);
    plot->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fList[2].toInt());
  }

  if (d_file_version >= 88){
    fList=lst[21].split("\t", QString::SkipEmptyParts);
    plot->setOrthogonal(fList[1].toInt());
  }

  plot->setStyle(lst[3].split("\t", QString::SkipEmptyParts));
  plot->setIgnoreFonts(true);
  plot->update();
  return plot;
}
Spectrogram*  ApplicationWindow::openSpectrogram(Graph*ag,const std::string &specgramwsName,const QStringList &lst)
{
  ProjectData *prjData=new ProjectData;
  if(!prjData)return 0;

  foreach (QString str, lst) {
    if(str.contains("<ColorMap>"))
    {	int index=lst.indexOf(str);
    //read the colormap file name from project file
    QString colormapLine=lst[index+1];
    QStringList list=colormapLine.split("\t");
    QString colormapFile=list[2];
    prjData->setColormapFile(colormapFile);
    }
    if(str.contains("<ColorPolicy>"))
    { 	//read the colormap policy to set gray scale
      int index=lst.indexOf(str);
      QString colormapPolicy=lst[index];
      int index1=colormapPolicy.indexOf(">");
      int index2=colormapPolicy.lastIndexOf("<");
      bool gray=colormapPolicy.mid(index1+1,index2-index1-1).toInt();
      prjData->setGrayScale(gray);

    }
    if (str.contains("\t<ContourLines>"))
    { //setting contour mode
      int index=lst.indexOf(str);
      QString contourlines=lst[index];
      int index1=contourlines.indexOf(">");
      int index2=contourlines.lastIndexOf("<");
      int bcontour=contourlines.mid(index1+1,index2-index1-1).toInt();
      if(bcontour)prjData->setContourMode(true);

      //setting contour levels
      QString contourlevels=lst[index+1];
      index1=contourlevels.indexOf(">");
      index2=contourlevels.lastIndexOf("<");
      int levels=contourlevels.mid(index1+1,index2-index1-1).toInt();
      prjData->setContourLevels(levels);

      //setting contour default pen
      QString pen=lst[index+2];
      if(pen.contains("<DefaultPen>"))
      {
        QString colorstring=lst[index+3];
        int index1=colorstring.indexOf(">");
        int index2=colorstring.lastIndexOf("<");
        QString pencolor=colorstring.mid(index1+1,index2-index1-1);

        QString widthstring=lst[index+4];
        index1=widthstring.indexOf(">");
        index2=widthstring.lastIndexOf("<");
        QString penwidth=widthstring.mid(index1+1,index2-index1-1);

        QString stylestring=lst[index+4];
        index1=stylestring.indexOf(">");
        index2=stylestring.lastIndexOf("<");
        QString penstyle=stylestring.mid(index1+1,index2-index1-1);
        QColor qcolor(pencolor);
        QPen pen = QPen(qcolor, penwidth.toDouble(),Graph::getPenStyle(penstyle.toInt()));
        prjData->setDefaultContourPen(pen);
        prjData->setColorMapPen(false);
      }
      else if (pen.contains("<CustomPen>"))
      {	ContourLinesEditor* contourLinesEditor = new ContourLinesEditor(this->locale());
      prjData->setCotntourLinesEditor(contourLinesEditor);
      prjData->setCustomPen(true);
      }
      else prjData->setColorMapPen(true);
    }
    if(str.contains("<IntensityChanged>"))
    {	 //read the intensity changed line from file and setting the spectrogram flag for intenisity

      int index=lst.indexOf(str);
      QString intensity=lst[index];
      int index1=intensity.indexOf(">");
      int index2=intensity.lastIndexOf("<");
      bool bIntensity=intensity.mid(index1+1,index2-index1-1).toInt();
      prjData->setIntensity(bIntensity);
    }

  }
  MantidMatrix *m=0;
  //getting the mantidmatrix object  for the saved spectrogram  inthe project file
  QList<MantidMatrix*>::const_iterator matrixItr;;
  for( matrixItr=m_mantidmatrixWindows.begin();matrixItr!=m_mantidmatrixWindows.end();++matrixItr)
  {
    if(specgramwsName==(*matrixItr)->getWorkspaceName())
      m=*matrixItr;
  }
  if(!m) return 0 ;
  Spectrogram* sp=m->plotSpectrogram(ag,this,Graph::ColorMap,true,prjData);
  return sp;
}

void ApplicationWindow::copyActiveLayer()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph *g = plot->activeGraph();

  lastCopiedLayer = g;
  connect (g, SIGNAL(destroyed()), this, SLOT(closedLastCopiedLayer()));
  g->copyImage();
}

void ApplicationWindow::showDataSetDialog(Analysis operation)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  DataSetDialog *ad = new DataSetDialog(tr("Curve") + ": ", this);
  ad->setGraph(g);
  ad->setOperationType(operation);
  ad->exec();
}

void ApplicationWindow::analyzeCurve(Graph *g, Analysis operation, const QString& curveTitle)
{
  if (!g)
    return;

  Fit *fitter = 0;
  switch(operation){
  case NoAnalysis:
    break;
  case Integrate:
  {
    Integration *i = new Integration(this, g, curveTitle);
    i->run();
    delete i;
  }
  break;
  case Diff:
  {
    Differentiation *diff = new Differentiation(this, g, curveTitle);
    diff->enableGraphicsDisplay(true);
    diff->run();
    delete diff;
  }
  break;
  case FitLinear:
    fitter = new LinearFit (this, g);
    break;
  case FitLorentz:
    fitter = new LorentzFit(this, g);
    break;
  case FitGauss:
    fitter = new GaussFit(this, g);
    break;
  case FitSigmoidal:
  {
    QwtPlotCurve* c = g->curve(curveTitle);
    if (c){
      ScaleEngine *se = (ScaleEngine *)g->plotWidget()->axisScaleEngine(c->xAxis());
      if(se->type() == QwtScaleTransformation::Log10)
        fitter = new LogisticFit (this, g);
      else
        fitter = new SigmoidalFit (this, g);
    }
  }
  break;
  }

  if (!fitter)
    return;

  if (fitter->setDataFromCurve(curveTitle)){
    if (operation != FitLinear){
      fitter->guessInitialValues();
      fitter->scaleErrors(fit_scale_errors);
      fitter->generateFunction(generateUniformFitPoints, fitPoints);
    } else if (d_2_linear_fit_points)
      fitter->generateFunction(generateUniformFitPoints, 2);
    fitter->setOutputPrecision(fit_output_precision);
    fitter->fit();
    if (pasteFitResultsToPlot)
      fitter->showLegend();
    delete fitter;
  }
}

void ApplicationWindow::analysis(Analysis operation)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  QString curve_title = g->selectedCurveTitle();
  if (!curve_title.isNull()) {
    analyzeCurve(g, operation, curve_title);
    return;
  }

  QStringList lst = g->analysableCurvesList();
  if (lst.count() == 1){
    const QwtPlotCurve *c = g->curve(lst[0]);
    if (c)
      analyzeCurve(g, operation, lst[0]);
  } else
    showDataSetDialog(operation);
}

void ApplicationWindow::integrate()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->isA("MultiLayer"))
    analysis(Integrate);
  else if (w->isA("Matrix")){
    QDateTime dt = QDateTime::currentDateTime ();
    QString info = dt.toString(Qt::LocalDate);
    info += "\n" + tr("Integration of %1 from zero is").arg(QString(w->objectName())) + ":\t";
    info += QString::number(((Matrix *)w)->integrate()) + "\n";
    info += "-------------------------------------------------------------\n";
    current_folder->appendLogInfo(info);
    showResults(true);
  }
}

void ApplicationWindow::differentiate()
{
  analysis(Diff);
}

void ApplicationWindow::fitLinear()
{
  analysis(FitLinear);
}

void ApplicationWindow::fitSigmoidal()
{
  analysis(FitSigmoidal);
}

void ApplicationWindow::fitGauss()
{
  analysis(FitGauss);
}

void ApplicationWindow::fitLorentz()

{
  analysis(FitLorentz);
}

void ApplicationWindow::pickPointerCursor()
{
  btnPointer->setChecked(true);
}

void ApplicationWindow::disableTools()
{
  if (displayBar->isVisible())
    displayBar->hide();

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)w)->layersList();
      foreach(Graph *g, layers)
      g->disableTools();
    }
  }
}

void ApplicationWindow::pickDataTool( QAction* action )
{
  if (!action)
    return;

  disableTools();

  if (action == btnCursor)
    showCursor();
  else if (action == btnSelect)
    showRangeSelectors();
  else if (action == btnPicker)
    showScreenReader();
  else if (action == btnMovePoints)
    movePoints();
  else if (action == btnRemovePoints)
    removePoints();
  else if (action == actionDrawPoints)
    drawPoints();
  else if (action == btnZoomIn)
    zoomIn();
  else if (action == btnZoomOut)
    zoomOut();
  else if (action == btnArrow)
    drawArrow();
  else if (action == btnLine)
    drawLine();
  else if (action == btnMultiPeakPick)
    selectMultiPeak();
  else if (action == actionMagnify)
    magnify();
}

void ApplicationWindow::connectSurfacePlot(Graph3D *plot)
{
  connect (plot, SIGNAL(showContextMenu()), this,SLOT(showWindowContextMenu()));
  connect (plot, SIGNAL(showOptionsDialog()), this,SLOT(showPlot3dDialog()));
  connect (plot, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
  connect (plot, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
  connect (plot, SIGNAL(statusChanged(MdiSubWindow*)), this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect (plot, SIGNAL(modified()), this, SIGNAL(modified()));

  plot->askOnCloseEvent(confirmClosePlot3D);
}

void ApplicationWindow::connectMultilayerPlot(MultiLayer *g)
{
  connect (g,SIGNAL(showTextDialog()),this,SLOT(showTextDialog()));
  connect (g,SIGNAL(showPlotDialog(int)),this,SLOT(showPlotDialog(int)));
  connect (g,SIGNAL(showScaleDialog(int)), this, SLOT(showScalePageFromAxisDialog(int)));
  connect (g,SIGNAL(showAxisDialog(int)), this, SLOT(showAxisPageFromAxisDialog(int)));
  connect (g,SIGNAL(showCurveContextMenu(int)), this, SLOT(showCurveContextMenu(int)));
  connect (g,SIGNAL(showContextMenu()),this,SLOT(showWindowContextMenu()));
  connect (g,SIGNAL(showCurvesDialog()),this,SLOT(showCurvesDialog()));
  connect (g,SIGNAL(drawLineEnded(bool)), btnPointer, SLOT(setOn(bool)));
  connect (g,SIGNAL(drawTextOff()),this, SLOT(disableAddText()));
  connect (g, SIGNAL(showAxisTitleDialog()), this, SLOT(showAxisTitleDialog()));

  connect (g,SIGNAL(showMarkerPopupMenu()),this,SLOT(showMarkerPopupMenu()));
  connect (g,SIGNAL(closedWindow(MdiSubWindow*)),this, SLOT(closeWindow(MdiSubWindow*)));
  connect (g,SIGNAL(hiddenWindow(MdiSubWindow*)),this, SLOT(hideWindow(MdiSubWindow*)));
  connect (g,SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect (g,SIGNAL(cursorInfo(const QString&)),info,SLOT(setText(const QString&)));
  connect (g,SIGNAL(showImageDialog()),this,SLOT(showImageDialog()));
  connect (g,SIGNAL(createTable(const QString&,int,int,const QString&)),
      this,SLOT(newTable(const QString&,int,int,const QString&)));
  connect (g,SIGNAL(viewTitleDialog()),this,SLOT(showTitleDialog()));
  connect (g,SIGNAL(modifiedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect (g,SIGNAL(resizedWindow(MdiSubWindow*)), this, SLOT(repaintWindows()));
  connect (g,SIGNAL(modifiedPlot()), this, SLOT(modifiedProject()));
  connect (g,SIGNAL(showLineDialog()),this, SLOT(showLineDialog()));
  connect (g,SIGNAL(pasteMarker()),this,SLOT(pasteSelection()));
  connect (g,SIGNAL(showGraphContextMenu()),this,SLOT(showGraphContextMenu()));
  connect (g,SIGNAL(setPointerCursor()),this, SLOT(pickPointerCursor()));
  connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SLOT(setFormatBarFont(const QFont&)));
  connect (g,SIGNAL(enableTextEditor(Graph *)), this, SLOT(enableTextEditor(Graph *)));

  g->askOnCloseEvent(confirmClosePlot2D);
}

void ApplicationWindow::connectTable(Table* w)
{
  connect (w->table(), SIGNAL(selectionChanged()), this, SLOT(customColumnActions()));
  connect (w,SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect (w,SIGNAL(hiddenWindow(MdiSubWindow*)),this, SLOT(hideWindow(MdiSubWindow*)));
  connect (w,SIGNAL(closedWindow(MdiSubWindow*)),this, SLOT(closeWindow(MdiSubWindow*)));
  connect (w,SIGNAL(removedCol(const QString&)),this,SLOT(removeCurves(const QString&)));
  connect (w,SIGNAL(modifiedData(Table *, const QString&)),
      this, SLOT(updateCurves(Table *, const QString&)));
  connect (w,SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect (w,SIGNAL(modifiedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect (w,SIGNAL(optionsDialog()),this,SLOT(showColumnOptionsDialog()));
  connect (w,SIGNAL(colValuesDialog()),this,SLOT(showColumnValuesDialog()));
  connect (w,SIGNAL(showContextMenu(bool)),this,SLOT(showTableContextMenu(bool)));
  connect (w,SIGNAL(changedColHeader(const QString&,const QString&)),this,SLOT(updateColNames(const QString&,const QString&)));
  connect (w,SIGNAL(createTable(const QString&,int,int,const QString&)),this,SLOT(newTable(const QString&,int,int,const QString&)));

  w->askOnCloseEvent(confirmCloseTable);
}

void ApplicationWindow::setAppColors(const QColor& wc, const QColor& pc, const QColor& tpc, bool force)
{
  if (force || workspaceColor != wc){
    workspaceColor = wc;
    d_workspace->setBackground(wc);
  }

  if (!force && panelsColor == pc && panelsTextColor == tpc)
    return;

  panelsColor = pc;
  panelsTextColor = tpc;

  QPalette palette;
  palette.setColor(QPalette::Base, QColor(panelsColor));
  qApp->setPalette(palette);

  palette.setColor(QPalette::Text, QColor(panelsTextColor));
  palette.setColor(QPalette::WindowText, QColor(panelsTextColor));

  lv->setPalette(palette);
  results->setPalette(palette);
  folders->setPalette(palette);
}

void ApplicationWindow::setPlot3DOptions()
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D")){
      Graph3D *g = (Graph3D*)w;
      g->setOrthogonal(orthogonal3DPlots);
      g->setAutoscale(autoscale3DPlots);
      g->setAntialiasing(smooth3DMesh);
    }
  }
}

void ApplicationWindow::createActions()
{
  actionCustomActionDialog = new QAction(tr("Manage Custom Menus..."), this);
  connect(actionCustomActionDialog, SIGNAL(activated()), this, SLOT(showCustomActionDialog()));
  
  actionManageDirs = new QAction(QIcon(getQPixmap("managefolders_xpm")), tr("Manage User Directories"), this);
  connect(actionManageDirs, SIGNAL(activated()), this, SLOT(showUserDirectoryDialog()));

  actionNewProject = new QAction(QIcon(getQPixmap("new_xpm")), tr("New &Project"), this);
  actionNewProject->setShortcut( tr("Ctrl+N") );
  connect(actionNewProject, SIGNAL(activated()), this, SLOT(newProject()));

  actionSaveProject=new QAction(QIcon(getQPixmap("filesave_xpm")), tr("Save &Project"), this);
  actionSaveProject->setShortcut( tr("Ctrl+Shift+S") );
  connect(actionSaveProject, SIGNAL(activated()), this, SLOT(saveProject()));

  actionSaveFile=new QAction(QIcon(getQPixmap("filesave_nexus_xpm")), tr("Save Nexus &File"), this);
  actionSaveFile->setShortcut( tr("Ctrl+S") );
  connect(actionSaveFile, SIGNAL(activated()), this, SLOT(savetoNexusFile()));

  actionNewFolder = new QAction(QIcon(getQPixmap("newFolder_xpm")), tr("New &Project"), this);
  actionNewProject->setShortcut(Qt::Key_F7);
  connect(actionNewFolder, SIGNAL(activated()), this, SLOT(addFolder()));

  actionNewGraph = new QAction(QIcon(getQPixmap("new_graph_xpm")), tr("New &Graph"), this);
  actionNewGraph->setShortcut( tr("Ctrl+G") );
  connect(actionNewGraph, SIGNAL(activated()), this, SLOT(newGraph()));

  actionNewNote = new QAction(QIcon(getQPixmap("new_note_xpm")), tr("New &Note"), this);
  connect(actionNewNote, SIGNAL(activated()), this, SLOT(newNote()));

  actionNewTable = new QAction(QIcon(getQPixmap("table_xpm")), tr("New &Table"), this);
  actionNewTable->setShortcut( tr("Ctrl+T") );
  connect(actionNewTable, SIGNAL(activated()), this, SLOT(newTable()));

  actionNewMatrix = new QAction(QIcon(getQPixmap("new_matrix_xpm")), tr("New &Matrix"), this);
  actionNewMatrix->setShortcut( tr("Ctrl+M") );
  connect(actionNewMatrix, SIGNAL(activated()), this, SLOT(newMatrix()));

  actionNewFunctionPlot = new QAction(QIcon(getQPixmap("newF_xpm")), tr("New &Function Plot"), this);
  actionNewFunctionPlot->setShortcut( tr("Ctrl+F") );
  connect(actionNewFunctionPlot, SIGNAL(activated()), this, SLOT(functionDialog()));

  actionNewSurfacePlot = new QAction(QIcon(getQPixmap("newFxy_xpm")), tr("New 3D &Surface Plot"), this);
  actionNewSurfacePlot->setShortcut( tr("Ctrl+ALT+Z") );
  connect(actionNewSurfacePlot, SIGNAL(activated()), this, SLOT(newSurfacePlot()));

  actionOpenNexus=new QAction(QIcon(getQPixmap("fileopen_nexus_xpm")), tr("&Nexus"), this);
  actionOpenNexus->setShortcut( tr("Ctrl+Shift+N") );
  connect(actionOpenNexus, SIGNAL(activated()), this, SLOT(loadNexus()));

  actionOpenProj=new QAction(QIcon(getQPixmap("folder_open_xpm")), tr("&Project"), this);
  actionOpenProj->setShortcut( tr("Ctrl+Shift+O") );
  connect(actionOpenProj, SIGNAL(activated()), this, SLOT(open()));

  actionOpenRaw=new QAction(QIcon(getQPixmap("fileopen_raw_xpm")), tr("&Raw"), this);
  actionOpenRaw->setShortcut( tr("Ctrl+Shift+R") );
  connect(actionOpenRaw, SIGNAL(activated()), this, SLOT(loadRaw()));

  actionLoadFile=new QAction(QIcon(getQPixmap("fileopen_raw_xpm")), tr("File"), this);
  actionLoadFile->setShortcut( tr("Ctrl+Shift+F") );
  connect(actionLoadFile, SIGNAL(activated()), this, SLOT(loadDataFile()));

  actionLoadImage = new QAction(tr("Open Image &File"), this);
  actionLoadImage->setShortcut( tr("Ctrl+I") );
  connect(actionLoadImage, SIGNAL(activated()), this, SLOT(loadImage()));

  actionImportImage = new QAction(tr("Import I&mage..."), this);
  connect(actionImportImage, SIGNAL(activated()), this, SLOT(importImage()));

  actionSaveProjectAs = new QAction(tr("Save Project &As..."), this);
  connect(actionSaveProjectAs, SIGNAL(activated()), this, SLOT(saveProjectAs()));

  actionOpenTemplate = new QAction(QIcon(getQPixmap("open_template_xpm")),tr("Open Temp&late..."), this);
  connect(actionOpenTemplate, SIGNAL(activated()), this, SLOT(openTemplate()));

  actionSaveTemplate = new QAction(QIcon(getQPixmap("save_template_xpm")), tr("Save As &Template..."), this);
  connect(actionSaveTemplate, SIGNAL(activated()), this, SLOT(saveAsTemplate()));

  actionSaveNote = new QAction(tr("Save Note As..."), this);
  connect(actionSaveNote, SIGNAL(activated()), this, SLOT(saveNoteAs()));

  actionLoad = new QAction(QIcon(getQPixmap("import_xpm")), tr("&Import ASCII..."), this);
  connect(actionLoad, SIGNAL(activated()), this, SLOT(importASCII()));

  actionUndo = new QAction(QIcon(getQPixmap("undo_xpm")), tr("&Undo"), this);
  actionUndo->setShortcut( tr("Ctrl+Z") );
  connect(actionUndo, SIGNAL(activated()), this, SLOT(undo()));

  actionRedo = new QAction(QIcon(getQPixmap("redo_xpm")), tr("&Redo"), this);
  actionRedo->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Z));
  connect(actionRedo, SIGNAL(activated()), this, SLOT(redo()));

  actionCopyWindow = new QAction(QIcon(getQPixmap("duplicate_xpm")), tr("&Duplicate"), this);
  connect(actionCopyWindow, SIGNAL(activated()), this, SLOT(clone()));

  actionCutSelection = new QAction(QIcon(getQPixmap("cut_xpm")), tr("Cu&t Selection"), this);
  actionCutSelection->setShortcut( tr("Ctrl+X") );
  connect(actionCutSelection, SIGNAL(activated()), this, SLOT(cutSelection()));

  actionCopySelection = new QAction(QIcon(getQPixmap("copy_xpm")), tr("&Copy Selection"), this);
  actionCopySelection->setShortcut( tr("Ctrl+C") );
  connect(actionCopySelection, SIGNAL(activated()), this, SLOT(copySelection()));

  actionPasteSelection = new QAction(QIcon(getQPixmap("paste_xpm")), tr("&Paste Selection"), this);
  actionPasteSelection->setShortcut( tr("Ctrl+V") );
  connect(actionPasteSelection, SIGNAL(activated()), this, SLOT(pasteSelection()));

  actionClearSelection = new QAction(QIcon(getQPixmap("erase_xpm")), tr("&Delete Selection"), this);
  actionClearSelection->setShortcut( tr("Del","delete key") );
  connect(actionClearSelection, SIGNAL(activated()), this, SLOT(clearSelection()));

  actionShowExplorer = explorerWindow->toggleViewAction();
  actionShowExplorer->setIcon(getQPixmap("folder_xpm"));
  actionShowExplorer->setShortcut( tr("Ctrl+E") );

  actionShowLog = logWindow->toggleViewAction();
  actionShowLog->setIcon(getQPixmap("log_xpm"));

  actionShowUndoStack = undoStackWindow->toggleViewAction();

    //#ifdef SCRIPTING_CONSOLE
  actionShowConsole = consoleWindow->toggleViewAction();
    //#endif

  actionAddLayer = new QAction(QIcon(getQPixmap("newLayer_xpm")), tr("Add La&yer"), this);
  actionAddLayer->setShortcut( tr("ALT+L") );
  connect(actionAddLayer, SIGNAL(activated()), this, SLOT(addLayer()));

  actionShowLayerDialog = new QAction(QIcon(getQPixmap("arrangeLayers_xpm")), tr("Arran&ge Layers"), this);
  actionShowLayerDialog->setShortcut( tr("Shift+A") );
  connect(actionShowLayerDialog, SIGNAL(activated()), this, SLOT(showLayerDialog()));

  actionAutomaticLayout = new QAction(QIcon(getQPixmap("auto_layout_xpm")), tr("Automatic Layout"), this);
  connect(actionAutomaticLayout, SIGNAL(activated()), this, SLOT(autoArrangeLayers()));

  actionExportGraph = new QAction(tr("&Current"), this);
  actionExportGraph->setShortcut( tr("Alt+G") );
  connect(actionExportGraph, SIGNAL(activated()), this, SLOT(exportGraph()));

  actionExportAllGraphs = new QAction(tr("&All"), this);
  actionExportAllGraphs->setShortcut( tr("Alt+X") );
  connect(actionExportAllGraphs, SIGNAL(activated()), this, SLOT(exportAllGraphs()));

  actionExportPDF = new QAction(QIcon(getQPixmap("pdf_xpm")), tr("&Export PDF"), this);
  actionExportPDF->setShortcut( tr("Ctrl+Alt+P") );
  connect(actionExportPDF, SIGNAL(activated()), this, SLOT(exportPDF()));

  actionPrint = new QAction(QIcon(getQPixmap("fileprint_xpm")), tr("&Print"), this);
  actionPrint->setShortcut( tr("Ctrl+P") );
  connect(actionPrint, SIGNAL(activated()), this, SLOT(print()));

  actionPrintAllPlots = new QAction(tr("Print All Plo&ts"), this);
  connect(actionPrintAllPlots, SIGNAL(activated()), this, SLOT(printAllPlots()));

  actionShowExportASCIIDialog = new QAction(tr("E&xport ASCII"), this);
  connect(actionShowExportASCIIDialog, SIGNAL(activated()), this, SLOT(showExportASCIIDialog()));

  actionCloseAllWindows = new QAction(QIcon(getQPixmap("quit_xpm")), tr("&Quit"), this);
  actionCloseAllWindows->setShortcut( tr("Ctrl+Q") );
  connect(actionCloseAllWindows, SIGNAL(activated()), qApp, SLOT(closeAllWindows()));

  actionClearLogInfo = new QAction(tr("Clear &Log Information"), this);
  connect(actionClearLogInfo, SIGNAL(activated()), this, SLOT(clearLogInfo()));

  actionClearConsole = new QAction(tr("Clear &Console"), this);
  connect(actionClearConsole, SIGNAL(activated()), console, SLOT(clear()));

  actionDeleteFitTables = new QAction(QIcon(getQPixmap("close_xpm")), tr("Delete &Fit Tables"), this);
  connect(actionDeleteFitTables, SIGNAL(activated()), this, SLOT(deleteFitTables()));

  actionShowPlotWizard = new QAction(QIcon(getQPixmap("wizard_xpm")), tr("Plot &Wizard"), this);
  actionShowPlotWizard->setShortcut( tr("Ctrl+Alt+W") );
  connect(actionShowPlotWizard, SIGNAL(activated()), this, SLOT(showPlotWizard()));

  actionShowConfigureDialog = new QAction(tr("&Preferences..."), this);
  connect(actionShowConfigureDialog, SIGNAL(activated()), this, SLOT(showPreferencesDialog()));

  actionShowCurvesDialog = new QAction(QIcon(getQPixmap("curves_xpm")), tr("Add/Remove &Curve..."), this);
  actionShowCurvesDialog->setShortcut( tr("ALT+C") );
  connect(actionShowCurvesDialog, SIGNAL(activated()), this, SLOT(showCurvesDialog()));

  actionAddErrorBars = new QAction(QIcon(getQPixmap("errors_xpm")), tr("Add &Error Bars..."), this);
  actionAddErrorBars->setShortcut( tr("Ctrl+B") );
  connect(actionAddErrorBars, SIGNAL(activated()), this, SLOT(addErrorBars()));

  actionRemoveErrorBars = new QAction(QIcon(getQPixmap("errors_xpm")), tr("Remove Error Bars..."), this);
  //actionRemoveErrorBars->setShortcut( tr("Ctrl+B") );
  connect(actionRemoveErrorBars, SIGNAL(activated()), this, SLOT(removeErrorBars()));

  actionAddFunctionCurve = new QAction(QIcon(getQPixmap("fx_xpm")), tr("Add &Function..."), this);
  actionAddFunctionCurve->setShortcut( tr("Ctrl+Alt+F") );
  connect(actionAddFunctionCurve, SIGNAL(activated()), this, SLOT(addFunctionCurve()));

  actionUnzoom = new QAction(QIcon(getQPixmap("unzoom_xpm")), tr("&Rescale to Show All"), this);
  actionUnzoom->setShortcut( tr("Ctrl+Shift+R") );
  connect(actionUnzoom, SIGNAL(activated()), this, SLOT(setAutoScale()));

  actionNewLegend = new QAction(QIcon(getQPixmap("legend_xpm")), tr("New &Legend"), this);
  actionNewLegend->setShortcut( tr("Ctrl+L") );
  connect(actionNewLegend, SIGNAL(activated()), this, SLOT(newLegend()));

  actionTimeStamp = new QAction(QIcon(getQPixmap("clock_xpm")), tr("Add Time Stamp"), this);
  actionTimeStamp->setShortcut( tr("Ctrl+ALT+T") );
  connect(actionTimeStamp, SIGNAL(activated()), this, SLOT(addTimeStamp()));

  actionAddImage = new QAction(QIcon(getQPixmap("monalisa_xpm")), tr("Add &Image"), this);
  actionAddImage->setShortcut( tr("ALT+I") );
  connect(actionAddImage, SIGNAL(activated()), this, SLOT(addImage()));

  actionPlotL = new QAction(QIcon(getQPixmap("lPlot_xpm")), tr("&Line"), this);
  connect(actionPlotL, SIGNAL(activated()), this, SLOT(plotL()));

  actionPlotP = new QAction(QIcon(getQPixmap("pPlot_xpm")), tr("&Scatter"), this);
  connect(actionPlotP, SIGNAL(activated()), this, SLOT(plotP()));

  actionPlotLP = new QAction(QIcon(getQPixmap("lpPlot_xpm")), tr("Line + S&ymbol"), this);
  connect(actionPlotLP, SIGNAL(activated()), this, SLOT(plotLP()));

  actionPlotVerticalDropLines = new QAction(QIcon(getQPixmap("dropLines_xpm")), tr("Vertical &Drop Lines"), this);
  connect(actionPlotVerticalDropLines, SIGNAL(activated()), this, SLOT(plotVerticalDropLines()));

  actionPlotSpline = new QAction(QIcon(getQPixmap("spline_xpm")), tr("&Spline"), this);
  connect(actionPlotSpline, SIGNAL(activated()), this, SLOT(plotSpline()));

  actionPlotHorSteps = new QAction(getQPixmap("hor_steps_xpm"), tr("&Horizontal Steps"), this);
  connect(actionPlotHorSteps, SIGNAL(activated()), this, SLOT(plotHorSteps()));

  actionPlotVertSteps = new QAction(QIcon(getQPixmap("vert_steps_xpm")), tr("&Vertical Steps"), this);
  connect(actionPlotVertSteps, SIGNAL(activated()), this, SLOT(plotVertSteps()));

  actionPlotVerticalBars = new QAction(QIcon(getQPixmap("vertBars_xpm")), tr("&Columns"), this);
  connect(actionPlotVerticalBars, SIGNAL(activated()), this, SLOT(plotVerticalBars()));

  actionPlotHorizontalBars = new QAction(QIcon(getQPixmap("hBars_xpm")), tr("&Rows"), this);
  connect(actionPlotHorizontalBars, SIGNAL(activated()), this, SLOT(plotHorizontalBars()));

  actionPlotArea = new QAction(QIcon(getQPixmap("area_xpm")), tr("&Area"), this);
  connect(actionPlotArea, SIGNAL(activated()), this, SLOT(plotArea()));

  actionPlotPie = new QAction(QIcon(getQPixmap("pie_xpm")), tr("&Pie"), this);
  connect(actionPlotPie, SIGNAL(activated()), this, SLOT(plotPie()));

  actionPlotVectXYAM = new QAction(QIcon(getQPixmap("vectXYAM_xpm")), tr("Vectors XY&AM"), this);
  connect(actionPlotVectXYAM, SIGNAL(activated()), this, SLOT(plotVectXYAM()));

  actionPlotVectXYXY = new QAction(QIcon(getQPixmap("vectXYXY_xpm")), tr("&Vectors &XYXY"), this);
  connect(actionPlotVectXYXY, SIGNAL(activated()), this, SLOT(plotVectXYXY()));

  actionPlotHistogram = new QAction(QIcon(getQPixmap("histogram_xpm")), tr("&Histogram"), this);
  connect(actionPlotHistogram, SIGNAL(activated()), this, SLOT(plotHistogram()));

  actionPlotStackedHistograms = new QAction(QIcon(getQPixmap("stacked_hist_xpm")), tr("&Stacked Histogram"), this);
  connect(actionPlotStackedHistograms, SIGNAL(activated()), this, SLOT(plotStackedHistograms()));

  actionPlot2VerticalLayers = new QAction(QIcon(getQPixmap("panel_v2_xpm")), tr("&Vertical 2 Layers"), this);
  connect(actionPlot2VerticalLayers, SIGNAL(activated()), this, SLOT(plot2VerticalLayers()));

  actionPlot2HorizontalLayers = new QAction(QIcon(getQPixmap("panel_h2_xpm")), tr("&Horizontal 2 Layers"), this);
  connect(actionPlot2HorizontalLayers, SIGNAL(activated()), this, SLOT(plot2HorizontalLayers()));

  actionPlot4Layers = new QAction(QIcon(getQPixmap("panel_4_xpm")), tr("&4 Layers"), this);
  connect(actionPlot4Layers, SIGNAL(activated()), this, SLOT(plot4Layers()));

  actionPlotStackedLayers = new QAction(QIcon(getQPixmap("stacked_xpm")), tr("&Stacked Layers"), this);
  connect(actionPlotStackedLayers, SIGNAL(activated()), this, SLOT(plotStackedLayers()));

  actionPlot3DRibbon = new QAction(QIcon(getQPixmap("ribbon_xpm")), tr("&Ribbon"), this);
  connect(actionPlot3DRibbon, SIGNAL(activated()), this, SLOT(plot3DRibbon()));

  actionPlot3DBars = new QAction(QIcon(getQPixmap("bars_xpm")), tr("&Bars"), this);
  connect(actionPlot3DBars, SIGNAL(activated()), this, SLOT(plot3DBars()));

  actionPlot3DScatter = new QAction(QIcon(getQPixmap("scatter_xpm")), tr("&Scatter"), this);
  connect(actionPlot3DScatter, SIGNAL(activated()), this, SLOT(plot3DScatter()));

  actionPlot3DTrajectory = new QAction(QIcon(getQPixmap("trajectory_xpm")), tr("&Trajectory"), this);
  connect(actionPlot3DTrajectory, SIGNAL(activated()), this, SLOT(plot3DTrajectory()));

  actionShowColStatistics = new QAction(QIcon(getQPixmap("col_stat_xpm")), tr("Statistics on &Columns"), this);
  connect(actionShowColStatistics, SIGNAL(activated()), this, SLOT(showColStatistics()));

  actionShowRowStatistics = new QAction(QIcon(getQPixmap("stat_rows_xpm")), tr("Statistics on &Rows"), this);
  connect(actionShowRowStatistics, SIGNAL(activated()), this, SLOT(showRowStatistics()));

  actionIntegrate = new QAction(tr("&Integrate"), this);
  connect(actionIntegrate, SIGNAL(activated()), this, SLOT(integrate()));

  actionShowIntDialog = new QAction(tr("Integr&ate Function..."), this);
  connect(actionShowIntDialog, SIGNAL(activated()), this, SLOT(showIntegrationDialog()));

  actionInterpolate = new QAction(tr("Inte&rpolate ..."), this);
  connect(actionInterpolate, SIGNAL(activated()), this, SLOT(showInterpolationDialog()));

  actionLowPassFilter = new QAction(tr("&Low Pass..."), this);
  connect(actionLowPassFilter, SIGNAL(activated()), this, SLOT(lowPassFilterDialog()));

  actionHighPassFilter = new QAction(tr("&High Pass..."), this);
  connect(actionHighPassFilter, SIGNAL(activated()), this, SLOT(highPassFilterDialog()));

  actionBandPassFilter = new QAction(tr("&Band Pass..."), this);
  connect(actionBandPassFilter, SIGNAL(activated()), this, SLOT(bandPassFilterDialog()));

  actionBandBlockFilter = new QAction(tr("&Band Block..."), this);
  connect(actionBandBlockFilter, SIGNAL(activated()), this, SLOT(bandBlockFilterDialog()));

  actionFFT = new QAction(tr("&FFT..."), this);
  connect(actionFFT, SIGNAL(activated()), this, SLOT(showFFTDialog()));

  actionSmoothSavGol = new QAction(tr("&Savitzky-Golay..."), this);
  connect(actionSmoothSavGol, SIGNAL(activated()), this, SLOT(showSmoothSavGolDialog()));

  actionSmoothFFT = new QAction(tr("&FFT Filter..."), this);
  connect(actionSmoothFFT, SIGNAL(activated()), this, SLOT(showSmoothFFTDialog()));

  actionSmoothAverage = new QAction(tr("Moving Window &Average..."), this);
  connect(actionSmoothAverage, SIGNAL(activated()), this, SLOT(showSmoothAverageDialog()));

  actionDifferentiate = new QAction(tr("&Differentiate"), this);
  connect(actionDifferentiate, SIGNAL(activated()), this, SLOT(differentiate()));

  actionFitLinear = new QAction(tr("Fit &Linear"), this);
  connect(actionFitLinear, SIGNAL(activated()), this, SLOT(fitLinear()));

  actionShowFitPolynomDialog = new QAction(tr("Fit &Polynomial ..."), this);
  connect(actionShowFitPolynomDialog, SIGNAL(activated()), this, SLOT(showFitPolynomDialog()));

  actionShowExpDecayDialog = new QAction(tr("&First Order ..."), this);
  connect(actionShowExpDecayDialog, SIGNAL(activated()), this, SLOT(showExpDecayDialog()));

  actionShowTwoExpDecayDialog = new QAction(tr("&Second Order ..."), this);
  connect(actionShowTwoExpDecayDialog, SIGNAL(activated()), this, SLOT(showTwoExpDecayDialog()));

  actionShowExpDecay3Dialog = new QAction(tr("&Third Order ..."), this);
  connect(actionShowExpDecay3Dialog, SIGNAL(activated()), this, SLOT(showExpDecay3Dialog()));

  actionFitExpGrowth = new QAction(tr("Fit Exponential Gro&wth ..."), this);
  connect(actionFitExpGrowth, SIGNAL(activated()), this, SLOT(showExpGrowthDialog()));

  actionFitSigmoidal = new QAction(tr("Fit &Boltzmann (Sigmoidal)"), this);
  connect(actionFitSigmoidal, SIGNAL(activated()), this, SLOT(fitSigmoidal()));

  actionFitGauss = new QAction(tr("Fit &Gaussian"), this);
  connect(actionFitGauss, SIGNAL(activated()), this, SLOT(fitGauss()));

  actionFitLorentz = new QAction(tr("Fit Lorent&zian"), this);
  connect(actionFitLorentz, SIGNAL(activated()), this, SLOT(fitLorentz()));

  actionShowFitDialog = new QAction(tr("Fit &Wizard..."), this);
  actionShowFitDialog->setShortcut( tr("Ctrl+Y") );
  connect(actionShowFitDialog, SIGNAL(activated()), this, SLOT(showFitDialog()));

  actionShowPlotDialog = new QAction(tr("&Plot ..."), this);
  connect(actionShowPlotDialog, SIGNAL(activated()), this, SLOT(showGeneralPlotDialog()));

  actionShowScaleDialog = new QAction(tr("&Scales..."), this);
  connect(actionShowScaleDialog, SIGNAL(activated()), this, SLOT(showScaleDialog()));

  actionShowAxisDialog = new QAction(tr("&Axes..."), this);
  connect(actionShowAxisDialog, SIGNAL(activated()), this, SLOT(showAxisDialog()));

  actionShowGridDialog = new QAction(tr("&Grid ..."), this);
  connect(actionShowGridDialog, SIGNAL(activated()), this, SLOT(showGridDialog()));

  actionShowTitleDialog = new QAction(tr("&Title ..."), this);
  connect(actionShowTitleDialog, SIGNAL(activated()), this, SLOT(showTitleDialog()));

  actionShowColumnOptionsDialog = new QAction(tr("Column &Options ..."), this);
  actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
  connect(actionShowColumnOptionsDialog, SIGNAL(activated()), this, SLOT(showColumnOptionsDialog()));

  actionShowColumnValuesDialog = new QAction(QIcon(getQPixmap("formula_xpm")), tr("Set Column &Values ..."), this);
  connect(actionShowColumnValuesDialog, SIGNAL(activated()), this, SLOT(showColumnValuesDialog()));
  actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));

  actionTableRecalculate = new QAction(tr("Recalculate"), this);
  actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
  connect(actionTableRecalculate, SIGNAL(activated()), this, SLOT(recalculateTable()));

  actionHideSelectedColumns = new QAction(tr("&Hide Selected"), this);
  connect(actionHideSelectedColumns, SIGNAL(activated()), this, SLOT(hideSelectedColumns()));

  actionShowAllColumns = new QAction(tr("Sho&w All Columns"), this);
  connect(actionShowAllColumns, SIGNAL(activated()), this, SLOT(showAllColumns()));

  actionSwapColumns = new QAction(QIcon(getQPixmap("swap_columns_xpm")), tr("&Swap columns"), this);
  connect(actionSwapColumns, SIGNAL(activated()), this, SLOT(swapColumns()));

  actionMoveColRight = new QAction(QIcon(getQPixmap("move_col_right_xpm")), tr("Move &Right"), this);
  connect(actionMoveColRight, SIGNAL(activated()), this, SLOT(moveColumnRight()));

  actionMoveColLeft = new QAction(QIcon(getQPixmap("move_col_left_xpm")), tr("Move &Left"), this);
  connect(actionMoveColLeft, SIGNAL(activated()), this, SLOT(moveColumnLeft()));

  actionMoveColFirst = new QAction(QIcon(getQPixmap("move_col_first_xpm")), tr("Move to F&irst"), this);
  connect(actionMoveColFirst, SIGNAL(activated()), this, SLOT(moveColumnFirst()));

  actionMoveColLast = new QAction(QIcon(getQPixmap("move_col_last_xpm")), tr("Move to Las&t"), this);
  connect(actionMoveColLast, SIGNAL(activated()), this, SLOT(moveColumnLast()));

  actionShowColsDialog = new QAction(tr("&Columns..."), this);
  connect(actionShowColsDialog, SIGNAL(activated()), this, SLOT(showColsDialog()));

  actionShowRowsDialog = new QAction(tr("&Rows..."), this);
  connect(actionShowRowsDialog, SIGNAL(activated()), this, SLOT(showRowsDialog()));

  actionDeleteRows = new QAction(tr("&Delete Rows Interval..."), this);
  connect(actionDeleteRows, SIGNAL(activated()), this, SLOT(showDeleteRowsDialog()));

  actionAbout = new QAction(tr("&About MantidPlot"), this);//Mantid
  actionAbout->setShortcut( tr("F1") );
  connect(actionAbout, SIGNAL(activated()), this, SLOT(about()));

  actionShowHelp = new QAction(tr("&Help"), this);
  actionShowHelp->setShortcut( tr("Ctrl+H") );
  connect(actionShowHelp, SIGNAL(activated()), this, SLOT(showHelp()));

  actionMantidConcepts=new QAction(tr("&Mantid Concepts"), this);
  connect(actionMantidConcepts, SIGNAL(activated()), this, SLOT(showMantidConcepts()));

  actionMantidAlgorithms=new QAction(tr("&Algorithm Descriptions"), this);
  connect(actionMantidAlgorithms, SIGNAL(activated()), this, SLOT(showalgorithmDescriptions()));

  actionmantidplotHelp=new QAction(tr("&MantidPlot Help"), this);
  connect(actionmantidplotHelp, SIGNAL(activated()), this, SLOT(showmantidplotHelp()));


  actionChooseHelpFolder = new QAction(tr("&Choose Help Folder..."), this);
  connect(actionChooseHelpFolder, SIGNAL(activated()), this, SLOT(chooseHelpFolder()));

  actionRename = new QAction(tr("&Rename Window"), this);
  connect(actionRename, SIGNAL(activated()), this, SLOT(rename()));

  actionCloseWindow = new QAction(QIcon(getQPixmap("close_xpm")), tr("Close &Window"), this);
  actionCloseWindow->setShortcut( tr("Ctrl+W") );
  connect(actionCloseWindow, SIGNAL(activated()), this, SLOT(closeActiveWindow()));

  actionAddColToTable = new QAction(QIcon(getQPixmap("addCol_xpm")), tr("Add Column"), this);
  connect(actionAddColToTable, SIGNAL(activated()), this, SLOT(addColToTable()));

  actionGoToRow = new QAction(tr("&Go to Row..."), this);
  actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));
  connect(actionGoToRow, SIGNAL(activated()), this, SLOT(goToRow()));

  actionGoToColumn = new QAction(tr("Go to Colum&n..."), this);
  actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));
  connect(actionGoToColumn, SIGNAL(activated()), this, SLOT(goToColumn()));

  actionClearTable = new QAction(getQPixmap("erase_xpm"), tr("Clear"), this);
  connect(actionClearTable, SIGNAL(activated()), this, SLOT(clearTable()));

  actionDeleteLayer = new QAction(QIcon(getQPixmap("erase_xpm")), tr("&Remove Layer"), this);
  actionDeleteLayer->setShortcut( tr("Alt+R") );
  connect(actionDeleteLayer, SIGNAL(activated()), this, SLOT(deleteLayer()));

  actionResizeActiveWindow = new QAction(QIcon(getQPixmap("resize_xpm")), tr("Window &Geometry..."), this);
  connect(actionResizeActiveWindow, SIGNAL(activated()), this, SLOT(resizeActiveWindow()));

  actionHideActiveWindow = new QAction(tr("&Hide Window"), this);
  connect(actionHideActiveWindow, SIGNAL(activated()), this, SLOT(hideActiveWindow()));

  actionShowMoreWindows = new QAction(tr("More windows..."), this);
  connect(actionShowMoreWindows, SIGNAL(activated()), this, SLOT(showMoreWindows()));

  actionPixelLineProfile = new QAction(QIcon(getQPixmap("pixelProfile_xpm")), tr("&View Pixel Line Profile"), this);
  connect(actionPixelLineProfile, SIGNAL(activated()), this, SLOT(pixelLineProfile()));

  actionIntensityTable = new QAction(tr("&Intensity Table"), this);
  connect(actionIntensityTable, SIGNAL(activated()), this, SLOT(intensityTable()));

  actionShowLineDialog = new QAction(tr("&Properties"), this);
  connect(actionShowLineDialog, SIGNAL(activated()), this, SLOT(showLineDialog()));

  actionShowImageDialog = new QAction(tr("&Properties"), this);
  connect(actionShowImageDialog, SIGNAL(activated()), this, SLOT(showImageDialog()));

  actionShowTextDialog = new QAction(tr("&Properties"), this);
  connect(actionShowTextDialog, SIGNAL(activated()), this, SLOT(showTextDialog()));

  actionActivateWindow = new QAction(tr("&Activate Window"), this);
  connect(actionActivateWindow, SIGNAL(activated()), this, SLOT(activateWindow()));

  actionMinimizeWindow = new QAction(tr("Mi&nimize Window"), this);
  connect(actionMinimizeWindow, SIGNAL(activated()), this, SLOT(minimizeWindow()));

  actionMaximizeWindow = new QAction(tr("Ma&ximize Window"), this);
  connect(actionMaximizeWindow, SIGNAL(activated()), this, SLOT(maximizeWindow()));

  actionHideWindow = new QAction(tr("&Hide Window"), this);
  connect(actionHideWindow, SIGNAL(activated()), this, SLOT(hideWindow()));

  actionResizeWindow = new QAction(QIcon(getQPixmap("resize_xpm")), tr("Re&size Window..."), this);
  connect(actionResizeWindow, SIGNAL(activated()), this, SLOT(resizeWindow()));

  actionEditSurfacePlot = new QAction(tr("&Surface..."), this);
  connect(actionEditSurfacePlot, SIGNAL(activated()), this, SLOT(editSurfacePlot()));

  actionAdd3DData = new QAction(tr("&Data Set..."), this);
  connect(actionAdd3DData, SIGNAL(activated()), this, SLOT(add3DData()));

  actionSetMatrixProperties = new QAction(tr("Set &Properties..."), this);
  connect(actionSetMatrixProperties, SIGNAL(activated()), this, SLOT(showMatrixDialog()));

  actionSetMatrixDimensions = new QAction(tr("Set &Dimensions..."), this);
  connect(actionSetMatrixDimensions, SIGNAL(activated()), this, SLOT(showMatrixSizeDialog()));
  actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));

  actionSetMatrixValues = new QAction(QIcon(getQPixmap("formula_xpm")), tr("Set &Values..."), this);
  connect(actionSetMatrixValues, SIGNAL(activated()), this, SLOT(showMatrixValuesDialog()));
  actionSetMatrixValues->setShortcut(tr("Alt+Q"));

  actionImagePlot =  new QAction(QIcon(getQPixmap("image_plot_xpm")), tr("&Image Plot"), this);
  connect(actionImagePlot, SIGNAL(activated()), this, SLOT(plotImage()));

  actionTransposeMatrix = new QAction(tr("&Transpose"), this);
  connect(actionTransposeMatrix, SIGNAL(activated()), this, SLOT(transposeMatrix()));

  actionFlipMatrixVertically = new QAction(QIcon(getQPixmap("flip_vertical_xpm")), tr("Flip &V"), this);
  actionFlipMatrixVertically->setShortcut(tr("Ctrl+Shift+V"));
  connect(actionFlipMatrixVertically, SIGNAL(activated()), this, SLOT(flipMatrixVertically()));

  actionFlipMatrixHorizontally = new QAction(QIcon(getQPixmap("flip_horizontal_xpm")), tr("Flip &H"), this);
  actionFlipMatrixHorizontally->setShortcut(tr("Ctrl+Shift+H"));
  connect(actionFlipMatrixHorizontally, SIGNAL(activated()), this, SLOT(flipMatrixHorizontally()));

  actionRotateMatrix = new QAction(QIcon(getQPixmap("rotate_clockwise_xpm")), tr("R&otate 90"), this);
  actionRotateMatrix->setShortcut(tr("Ctrl+Shift+R"));
  connect(actionRotateMatrix, SIGNAL(activated()), this, SLOT(rotateMatrix90()));

  actionRotateMatrixMinus = new QAction(QIcon(getQPixmap("rotate_counterclockwise_xpm")), tr("Rotate &-90"), this);
  actionRotateMatrixMinus->setShortcut(tr("Ctrl+Alt+R"));
  connect(actionRotateMatrixMinus, SIGNAL(activated()), this, SLOT(rotateMatrixMinus90()));

  actionInvertMatrix = new QAction(tr("&Invert"), this);
  connect(actionInvertMatrix, SIGNAL(activated()), this, SLOT(invertMatrix()));

  actionMatrixDeterminant = new QAction(tr("&Determinant"), this);
  connect(actionMatrixDeterminant, SIGNAL(activated()), this, SLOT(matrixDeterminant()));

  actionViewMatrixImage = new QAction(tr("&Image mode"), this);
  actionViewMatrixImage->setShortcut(tr("Ctrl+Shift+I"));
  connect(actionViewMatrixImage, SIGNAL(activated()), this, SLOT(viewMatrixImage()));
  actionViewMatrixImage->setCheckable(true);

  actionViewMatrix = new QAction(tr("&Data mode"), this);
  actionViewMatrix->setShortcut(tr("Ctrl+Shift+D"));
  connect(actionViewMatrix, SIGNAL(activated()), this, SLOT(viewMatrixTable()));
  actionViewMatrix->setCheckable(true);

  actionMatrixXY = new QAction(tr("Show &X/Y"), this);
  actionMatrixXY->setShortcut(tr("Ctrl+Shift+X"));
  connect(actionMatrixXY, SIGNAL(activated()), this, SLOT(viewMatrixXY()));
  actionMatrixXY->setCheckable(true);

  actionMatrixColumnRow = new QAction(tr("Show &Column/Row"), this);
  actionMatrixColumnRow->setShortcut(tr("Ctrl+Shift+C"));
  connect(actionMatrixColumnRow, SIGNAL(activated()), this, SLOT(viewMatrixColumnRow()));
  actionMatrixColumnRow->setCheckable(true);

  actionMatrixGrayScale = new QAction(tr("&Gray Scale"), this);
  connect(actionMatrixGrayScale, SIGNAL(activated()), this, SLOT(setMatrixGrayScale()));
  actionMatrixGrayScale->setCheckable(true);

  actionMatrixRainbowScale = new QAction(tr("&Rainbow"), this);
  connect(actionMatrixRainbowScale, SIGNAL(activated()), this, SLOT(setMatrixRainbowScale()));
  actionMatrixRainbowScale->setCheckable(true);

  actionMatrixCustomScale = new QAction(tr("&Custom"), this);
  connect(actionMatrixCustomScale, SIGNAL(activated()), this, SLOT(showColorMapDialog()));
  actionMatrixCustomScale->setCheckable(true);

  actionExportMatrix = new QAction(tr("&Export Image ..."), this);
  connect(actionExportMatrix, SIGNAL(activated()), this, SLOT(exportMatrix()));

  actionConvertMatrixDirect = new QAction(tr("&Direct"), this);
  connect(actionConvertMatrixDirect, SIGNAL(activated()), this, SLOT(convertMatrixToTableDirect()));

  actionConvertMatrixXYZ = new QAction(tr("&XYZ Columns"), this);
  connect(actionConvertMatrixXYZ, SIGNAL(activated()), this, SLOT(convertMatrixToTableXYZ()));

  actionConvertMatrixYXZ = new QAction(tr("&YXZ Columns"), this);
  connect(actionConvertMatrixYXZ, SIGNAL(activated()), this, SLOT(convertMatrixToTableYXZ()));

  actionMatrixFFTDirect = new QAction(tr("&Forward FFT"), this);
  connect(actionMatrixFFTDirect, SIGNAL(activated()), this, SLOT(matrixDirectFFT()));

  actionMatrixFFTInverse = new QAction(tr("&Inverse FFT"), this);
  connect(actionMatrixFFTInverse, SIGNAL(activated()), this, SLOT(matrixInverseFFT()));

  actionConvertTable= new QAction(tr("Convert to &Matrix"), this);
  connect(actionConvertTable, SIGNAL(activated()), this, SLOT(convertTableToMatrix()));

  actionPlot3DWireFrame = new QAction(QIcon(getQPixmap("lineMesh_xpm")), tr("3D &Wire Frame"), this);
  connect(actionPlot3DWireFrame, SIGNAL(activated()), this, SLOT(plot3DWireframe()));

  actionPlot3DHiddenLine = new QAction(QIcon(getQPixmap("grid_only_xpm")), tr("3D &Hidden Line"), this);
  connect(actionPlot3DHiddenLine, SIGNAL(activated()), this, SLOT(plot3DHiddenLine()));

  actionPlot3DPolygons = new QAction(QIcon(getQPixmap("no_grid_xpm")), tr("3D &Polygons"), this);
  connect(actionPlot3DPolygons, SIGNAL(activated()), this, SLOT(plot3DPolygons()));

  actionPlot3DWireSurface = new QAction(QIcon(getQPixmap("grid_poly_xpm")), tr("3D Wire &Surface"), this);
  connect(actionPlot3DWireSurface, SIGNAL(activated()), this, SLOT(plot3DWireSurface()));

  actionColorMap = new QAction(QIcon(getQPixmap("color_map_xpm")), tr("Contour - &Color Fill"), this);
  connect(actionColorMap, SIGNAL(activated()), this, SLOT(plotColorMap()));

  actionContourMap = new QAction(QIcon(getQPixmap("contour_map_xpm")), tr("Contour &Lines"), this);
  connect(actionContourMap, SIGNAL(activated()), this, SLOT(plotContour()));

  actionGrayMap = new QAction(QIcon(getQPixmap("gray_map_xpm")), tr("&Gray Scale Map"), this);
  connect(actionGrayMap, SIGNAL(activated()), this, SLOT(plotGrayScale()));

  actionNoContourColorMap = new QAction(QIcon(getQPixmap("color_map_xpm")), tr("Color &Fill"), this);
  connect(actionNoContourColorMap, SIGNAL(activated()), this, SLOT(plotNoContourColorMap()));

  actionSortTable = new QAction(tr("Sort Ta&ble"), this);
  connect(actionSortTable, SIGNAL(activated()), this, SLOT(sortActiveTable()));

  actionSortSelection = new QAction(tr("Sort Columns"), this);
  connect(actionSortSelection, SIGNAL(activated()), this, SLOT(sortSelection()));

  actionNormalizeTable = new QAction(tr("&Table"), this);
  connect(actionNormalizeTable, SIGNAL(activated()), this, SLOT(normalizeActiveTable()));

  actionNormalizeSelection = new QAction(tr("&Columns"), this);
  connect(actionNormalizeSelection, SIGNAL(activated()), this, SLOT(normalizeSelection()));

  actionCorrelate = new QAction(tr("Co&rrelate"), this);
  connect(actionCorrelate, SIGNAL(activated()), this, SLOT(correlate()));

  actionAutoCorrelate = new QAction(tr("&Autocorrelate"), this);
  connect(actionAutoCorrelate, SIGNAL(activated()), this, SLOT(autoCorrelate()));

  actionConvolute = new QAction(tr("&Convolute"), this);
  connect(actionConvolute, SIGNAL(activated()), this, SLOT(convolute()));

  actionDeconvolute = new QAction(tr("&Deconvolute"), this);
  connect(actionDeconvolute, SIGNAL(activated()), this, SLOT(deconvolute()));

  actionTranslateHor = new QAction(tr("&Horizontal"), this);
  connect(actionTranslateHor, SIGNAL(activated()), this, SLOT(translateCurveHor()));

  actionTranslateVert = new QAction(tr("&Vertical"), this);
  connect(actionTranslateVert, SIGNAL(activated()), this, SLOT(translateCurveVert()));

  actionSetAscValues = new QAction(QIcon(getQPixmap("rowNumbers_xpm")),tr("Ro&w Numbers"), this);
  connect(actionSetAscValues, SIGNAL(activated()), this, SLOT(setAscValues()));

  actionSetRandomValues = new QAction(QIcon(getQPixmap("randomNumbers_xpm")),tr("&Random Values"), this);
  connect(actionSetRandomValues, SIGNAL(activated()), this, SLOT(setRandomValues()));

  actionReadOnlyCol = new QAction(tr("&Read Only"), this);
  connect(actionReadOnlyCol, SIGNAL(activated()), this, SLOT(setReadOnlyCol()));

  actionSetXCol = new QAction(QIcon(getQPixmap("x_col_xpm")), tr("&X"), this);
  connect(actionSetXCol, SIGNAL(activated()), this, SLOT(setXCol()));

  actionSetYCol = new QAction(QIcon(getQPixmap("y_col_xpm")), tr("&Y"), this);
  connect(actionSetYCol, SIGNAL(activated()), this, SLOT(setYCol()));

  actionSetZCol = new QAction(QIcon(getQPixmap("z_col_xpm")), tr("&Z"), this);
  connect(actionSetZCol, SIGNAL(activated()), this, SLOT(setZCol()));

  actionSetXErrCol = new QAction(tr("X E&rror"), this);
  connect(actionSetXErrCol, SIGNAL(activated()), this, SLOT(setXErrCol()));

  actionSetYErrCol = new QAction(QIcon(getQPixmap("errors_xpm")), tr("Y &Error"), this);
  connect(actionSetYErrCol, SIGNAL(activated()), this, SLOT(setYErrCol()));

  actionDisregardCol = new QAction(QIcon(getQPixmap("disregard_col_xpm")), tr("&Disregard"), this);
  connect(actionDisregardCol, SIGNAL(activated()), this, SLOT(disregardCol()));

  actionSetLabelCol = new QAction(QIcon(getQPixmap("set_label_col_xpm")), tr("&Label"), this);
  connect(actionSetLabelCol, SIGNAL(activated()), this, SLOT(setLabelCol()));

  actionBoxPlot = new QAction(QIcon(getQPixmap("boxPlot_xpm")),tr("&Box Plot"), this);
  connect(actionBoxPlot, SIGNAL(activated()), this, SLOT(plotBoxDiagram()));

  actionMultiPeakGauss = new QAction(tr("&Gaussian..."), this);
  connect(actionMultiPeakGauss, SIGNAL(activated()), this, SLOT(fitMultiPeakGauss()));

  actionMultiPeakLorentz = new QAction(tr("&Lorentzian..."), this);
  connect(actionMultiPeakLorentz, SIGNAL(activated()), this, SLOT(fitMultiPeakLorentz()));

  //actionCheckUpdates = new QAction(tr("Search for &Updates"), this);
  //connect(actionCheckUpdates, SIGNAL(activated()), this, SLOT(searchForUpdates()));

  actionHomePage = new QAction(tr("&Mantid Homepage"), this); //Mantid change
  connect(actionHomePage, SIGNAL(activated()), this, SLOT(showHomePage()));

  //actionHelpForums = new QAction(tr("QtiPlot &Forums"), this); // Mantid change
  //	connect(actionHelpForums, SIGNAL(triggered()), this, SLOT(showForums())); // Mantid change

  actionHelpBugReports = new QAction(tr("Report a &Bug"), this);
  connect(actionHelpBugReports, SIGNAL(triggered()), this, SLOT(showBugTracker()));

  //actionDownloadManual = new QAction(tr("Download &Manual"), this); // Mantid change
  //connect(actionDownloadManual, SIGNAL(activated()), this, SLOT(downloadManual())); // Mantid change

  //actionTranslations = new QAction(tr("&Translations"), this); // Mantid change
  //connect(actionTranslations, SIGNAL(activated()), this, SLOT(downloadTranslation())); // Mantid change

  //actionDonate = new QAction(tr("Make a &Donation"), this); // Mantid change
  //connect(actionDonate, SIGNAL(activated()), this, SLOT(showDonationsPage())); // Mantid change

  // 	actionTechnicalSupport = new QAction(tr("Technical &Support"), this); // Mantid change
  // 	connect(actionTechnicalSupport, SIGNAL(activated()), this, SLOT(showSupportPage())); // Mantid change

  //#ifdef SCRIPTING_DIALOG
  //	actionScriptingLang = new QAction(tr("Scripting &language"), this);
  //	connect(actionScriptingLang, SIGNAL(activated()), this, SLOT(showScriptingLangDialog()));
  //#endif

  actionNoteExecute = new QAction(tr("E&xecute"), this);
  actionNoteExecute->setShortcut(tr("Ctrl+J"));

  actionNoteExecuteAll = new QAction(tr("Execute &All"), this);
  actionNoteExecuteAll->setShortcut(tr("Ctrl+Shift+J"));

  actionNoteEvaluate = new QAction(tr("&Evaluate Expression"), this);
  actionNoteEvaluate->setShortcut(tr("Ctrl+Return"));

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow = new QAction(getQPixmap("python_xpm"), tr("&Script Window"), this);
  actionShowScriptWindow->setShortcut(tr("F3"));
  actionShowScriptWindow->setToggleAction( true );
  connect(actionShowScriptWindow, SIGNAL(activated()), this, SLOT(showScriptWindow()));
  actionShowScriptInterpreter = new QAction(getQPixmap("python_xpm"), tr("Script &Interpreter"), this);
  actionShowScriptInterpreter->setShortcut(tr("F4"));
  actionShowScriptInterpreter->setToggleAction(true);
  connect(actionShowScriptInterpreter, SIGNAL(activated()), this, SLOT(showScriptInterpreter()));
#endif

  actionShowCurvePlotDialog = new QAction(tr("&Plot details..."), this);
  connect(actionShowCurvePlotDialog, SIGNAL(activated()), this, SLOT(showCurvePlotDialog()));

  actionShowCurveWorksheet = new QAction(tr("&Worksheet"), this);
  connect(actionShowCurveWorksheet, SIGNAL(activated()), this, SLOT(showCurveWorksheet()));

  actionCurveFullRange = new QAction(tr("&Reset to Full Range"), this);
  connect(actionCurveFullRange, SIGNAL(activated()), this, SLOT(setCurveFullRange()));

  actionEditCurveRange = new QAction(tr("Edit &Range..."), this);
  connect(actionEditCurveRange, SIGNAL(activated()), this, SLOT(showCurveRangeDialog()));

  actionRemoveCurve = new QAction(getQPixmap("close_xpm"), tr("&Delete"), this);
  connect(actionRemoveCurve, SIGNAL(activated()), this, SLOT(removeCurve()));

  actionHideCurve = new QAction(tr("&Hide"), this);
  connect(actionHideCurve, SIGNAL(activated()), this, SLOT(hideCurve()));

  actionHideOtherCurves = new QAction(tr("Hide &Other Curves"), this);
  connect(actionHideOtherCurves, SIGNAL(activated()), this, SLOT(hideOtherCurves()));

  actionShowAllCurves = new QAction(tr("&Show All Curves"), this);
  connect(actionShowAllCurves, SIGNAL(activated()), this, SLOT(showAllCurves()));

  actionEditFunction = new QAction(tr("&Edit Function..."), this);
  connect(actionEditFunction, SIGNAL(activated()), this, SLOT(showFunctionDialog()));

  actionToolBars = new QAction(tr("&Toolbars..."), this);
  actionToolBars->setShortcut(tr("Ctrl+Shift+T"));
  connect(actionToolBars, SIGNAL(activated()), this, SLOT(showToolBarsMenu()));

  actionFontBold = new QAction("B", this);
  actionFontBold->setToolTip(tr("Bold"));
  QFont font = appFont;
  font.setBold(true);
  actionFontBold->setFont(font);
  actionFontBold->setCheckable(true);
  connect(actionFontBold, SIGNAL(toggled(bool)), this, SLOT(setBoldFont(bool)));

  actionFontItalic = new QAction("It", this);
  actionFontItalic->setToolTip(tr("Italic"));
  font = appFont;
  font.setItalic(true);
  actionFontItalic->setFont(font);
  actionFontItalic->setCheckable(true);
  connect(actionFontItalic, SIGNAL(toggled(bool)), this, SLOT(setItalicFont(bool)));

  actionSuperscript = new QAction(getQPixmap("exp_xpm"), tr("Superscript"), this);
  connect(actionSuperscript, SIGNAL(activated()), this, SLOT(insertSuperscript()));
  actionSuperscript->setEnabled(false);

  actionSubscript = new QAction(getQPixmap("index_xpm"), tr("Subscript"), this);
  connect(actionSubscript, SIGNAL(activated()), this, SLOT(insertSubscript()));
  actionSubscript->setEnabled(false);

  actionUnderline = new QAction("U", this);
  actionUnderline->setToolTip(tr("Underline (Ctrl+U)"));
  actionUnderline->setShortcut(tr("Ctrl+U"));
  font = appFont;
  font.setUnderline(true);
  actionUnderline->setFont(font);
  connect(actionUnderline, SIGNAL(activated()), this, SLOT(underline()));
  actionUnderline->setEnabled(false);

  actionGreekSymbol = new QAction(QString(QChar(0x3B1)) + QString(QChar(0x3B2)), this);
  actionGreekSymbol->setToolTip(tr("Greek"));
  connect(actionGreekSymbol, SIGNAL(activated()), this, SLOT(insertGreekSymbol()));

  actionGreekMajSymbol = new QAction(QString(QChar(0x393)), this);
  actionGreekMajSymbol->setToolTip(tr("Greek"));
  connect(actionGreekMajSymbol, SIGNAL(activated()), this, SLOT(insertGreekMajSymbol()));

  actionMathSymbol = new QAction(QString(QChar(0x222B)), this);
  actionMathSymbol->setToolTip(tr("Mathematical Symbols"));
  connect(actionMathSymbol, SIGNAL(activated()), this, SLOT(insertMathSymbol()));

  actionclearAllMemory = new QAction("&Clear All Memory",this);
  actionclearAllMemory->setShortcut(QKeySequence::fromString("Ctrl+Shift+L"));
  connect(actionclearAllMemory,SIGNAL(triggered()), mantidUI, SLOT(clearAllMemory() ));

#ifdef USE_TCMALLOC
  actionreleaseFreeMemory = new QAction("&Release Free Memory",this);
  connect(actionreleaseFreeMemory,SIGNAL(triggered()), mantidUI, SLOT(releaseFreeMemory() ));
#endif

  actionMagnify = new QAction(QIcon(getQPixmap("magnifier_xpm")), tr("Zoom &In/Out and Drag Canvas"), this);
  connect(actionMagnify, SIGNAL(activated()), this, SLOT(magnify()));

  actionICatLogin  = new QAction("Login",this);
  actionICatLogin->setToolTip(tr("Catalog Login"));
  connect(actionICatLogin, SIGNAL(activated()), this, SLOT(ICatLogin()));

  actionICatSearch=new QAction("Basic Search",this);
  actionICatSearch->setToolTip(tr("Catalog Basic Search"));
  connect(actionICatSearch, SIGNAL(activated()), this, SLOT(ICatIsisSearch()));

  actionMydataSearch=new QAction("My Data Search",this);
  actionMydataSearch->setToolTip(tr("Catalog MyData Search"));
  connect(actionMydataSearch, SIGNAL(activated()), this, SLOT(ICatMyDataSearch()));

  actionICatLogout=new QAction("Logout",this);
  actionICatLogout->setToolTip(tr("Catalog Logout"));
  connect(actionICatLogout, SIGNAL(activated()), this, SLOT(ICatLogout()));

  actionAdvancedSearch = new QAction("Advanced Search",this);
  actionAdvancedSearch->setToolTip(tr("Catalog Advanced Search"));
  connect(actionAdvancedSearch, SIGNAL(activated()), this, SLOT(ICatAdvancedSearch()));
}

void ApplicationWindow::translateActionsStrings()
{
  actionFontBold->setToolTip(tr("Bold"));
  actionFontItalic->setToolTip(tr("Italic"));
  actionUnderline->setStatusTip(tr("Underline (Ctrl+U)"));
  actionUnderline->setShortcut(tr("Ctrl+U"));
  actionGreekSymbol->setToolTip(tr("Greek"));
  actionGreekMajSymbol->setToolTip(tr("Greek"));
  actionMathSymbol->setToolTip(tr("Mathematical Symbols"));

  actionShowCurvePlotDialog->setMenuText(tr("&Plot details..."));
  actionShowCurveWorksheet->setMenuText(tr("&Worksheet"));
  actionRemoveCurve->setMenuText(tr("&Delete"));
  actionEditFunction->setMenuText(tr("&Edit Function..."));

  actionCurveFullRange->setMenuText(tr("&Reset to Full Range"));
  actionEditCurveRange->setMenuText(tr("Edit &Range..."));
  actionHideCurve->setMenuText(tr("&Hide"));
  actionHideOtherCurves->setMenuText(tr("Hide &Other Curves"));
  actionShowAllCurves->setMenuText(tr("&Show All Curves"));

  actionNewProject->setMenuText(tr("New &Project"));
  actionNewProject->setToolTip(tr("Open a new project"));
  actionNewProject->setShortcut(tr("Ctrl+N"));

  actionNewFolder->setMenuText(tr("New Fol&der"));
  actionNewFolder->setToolTip(tr("Create a new folder"));
  actionNewFolder->setShortcut(Qt::Key_F7);

  actionNewGraph->setMenuText(tr("New &Graph"));
  actionNewGraph->setToolTip(tr("Create an empty 2D plot"));
  actionNewGraph->setShortcut(tr("Ctrl+G"));

  actionNewNote->setMenuText(tr("New &Note"));
  actionNewNote->setToolTip(tr("Create an empty note window"));

  actionNewTable->setMenuText(tr("New &Table"));
  actionNewTable->setShortcut(tr("Ctrl+T"));
  actionNewTable->setToolTip(tr("New table"));

  actionNewMatrix->setMenuText(tr("New &Matrix"));
  actionNewMatrix->setShortcut(tr("Ctrl+M"));
  actionNewMatrix->setToolTip(tr("New matrix"));

  actionNewFunctionPlot->setMenuText(tr("New &Function Plot"));
  actionNewFunctionPlot->setToolTip(tr("Create a new 2D function plot"));
  actionNewFunctionPlot->setShortcut(tr("Ctrl+F"));

  actionNewSurfacePlot->setMenuText(tr("New 3D &Surface Plot"));
  actionNewSurfacePlot->setToolTip(tr("Create a new 3D surface plot"));
  actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));

  actionOpenProj->setMenuText(tr("&Project"));
  actionOpenProj->setShortcut(tr("Ctrl+Shift+O"));
  actionOpenProj->setToolTip(tr("Load Mantid project"));

  actionOpenRaw->setMenuText(tr("&Raw"));
  actionOpenRaw->setShortcut(tr("Ctrl+Shift+R"));
  actionOpenRaw->setToolTip(tr("Load Raw File"));

  actionOpenNexus->setMenuText(tr("&Nexus"));
  actionOpenNexus->setShortcut(tr("Ctrl+Shift+N"));
  actionOpenNexus->setToolTip(tr("Load Nexus File"));

  actionLoadFile->setMenuText(tr("&File"));
  actionLoadFile->setShortcut(tr("Ctrl+Shift+F"));
  actionLoadFile->setToolTip(tr("Load Data File"));
 

  actionLoadImage->setMenuText(tr("Open Image &File"));
  actionLoadImage->setShortcut(tr("Ctrl+I"));

  actionImportImage->setMenuText(tr("Import I&mage..."));

  actionSaveFile->setMenuText(tr("&Nexus"));
  actionSaveFile->setToolTip(tr("Save nexus file"));
  actionSaveFile->setShortcut(tr("Ctrl+S"));

  actionSaveProject->setMenuText(tr("&Project"));
  actionSaveProject->setToolTip(tr("Save Mantid Project"));
  actionSaveProject->setShortcut(tr("Ctrl+Shift+S"));


  actionSaveProjectAs->setMenuText(tr("Save Project &As..."));

  actionOpenTemplate->setMenuText(tr("Open Te&mplate..."));
  actionOpenTemplate->setToolTip(tr("Open template"));

  actionSaveTemplate->setMenuText(tr("Save As &Template..."));
  actionSaveTemplate->setToolTip(tr("Save window as template"));

  actionLoad->setMenuText(tr("&Import ASCII..."));
  actionLoad->setToolTip(tr("Import data file(s)"));
  actionLoad->setShortcut(tr("Ctrl+K"));

  actionUndo->setMenuText(tr("&Undo"));
  actionUndo->setToolTip(tr("Undo changes"));
  actionUndo->setShortcut(tr("Ctrl+Z"));

  actionRedo->setMenuText(tr("&Redo"));
  actionRedo->setToolTip(tr("Redo changes"));
  actionRedo->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Z));

  actionCopyWindow->setMenuText(tr("&Duplicate"));
  actionCopyWindow->setToolTip(tr("Duplicate window"));

  actionCutSelection->setMenuText(tr("Cu&t Selection"));
  actionCutSelection->setToolTip(tr("Cut selection"));
  actionCutSelection->setShortcut(tr("Ctrl+X"));
  
  actionCopySelection->setMenuText(tr("&Copy Selection"));
  actionCopySelection->setToolTip(tr("Copy selection"));
  actionCopySelection->setShortcut(tr("Ctrl+C"));
  

  actionPasteSelection->setMenuText(tr("&Paste Selection"));
  actionPasteSelection->setToolTip(tr("Paste selection"));
  actionPasteSelection->setShortcut(tr("Ctrl+V"));
  

  actionClearSelection->setMenuText(tr("&Delete Selection"));
  actionClearSelection->setToolTip(tr("Delete selection"));
  actionClearSelection->setShortcut(tr("Del","delete key"));

  actionShowExplorer->setMenuText(tr("Project &Explorer"));
  actionShowExplorer->setShortcut(tr("Ctrl+E"));
  actionShowExplorer->setToolTip(tr("Show project explorer"));

  actionShowLog->setMenuText(tr("Results &Log"));
  actionShowLog->setToolTip(tr("Show analysis results"));

  actionShowUndoStack->setMenuText(tr("&Undo/Redo Stack"));
  actionShowUndoStack->setToolTip(tr("Show available undo/redo commands"));

    //#ifdef SCRIPTING_CONSOLE
  actionShowConsole->setMenuText(tr("&Console"));
  actionShowConsole->setToolTip(tr("Show Scripting console"));
    //#endif

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow->setMenuText(tr("&Script Window"));
  actionShowScriptWindow->setToolTip(tr("Script Window"));
  actionShowScriptWindow->setShortcut(tr("F3"));
#endif

  actionCustomActionDialog->setMenuText(tr("Manage Custom Menus..."));

  actionAddLayer->setMenuText(tr("Add La&yer"));
  actionAddLayer->setToolTip(tr("Add Layer"));
  actionAddLayer->setShortcut(tr("ALT+L"));

  actionShowLayerDialog->setMenuText(tr("Arran&ge Layers"));
  actionShowLayerDialog->setToolTip(tr("Arrange Layers"));
  actionShowLayerDialog->setShortcut(tr("Shift+A"));

  actionAutomaticLayout->setMenuText(tr("Automatic Layout"));
  actionAutomaticLayout->setToolTip(tr("Automatic Layout"));

  actionExportGraph->setMenuText(tr("&Current"));
  actionExportGraph->setShortcut(tr("Alt+G"));
  actionExportGraph->setToolTip(tr("Export current graph"));

  actionExportAllGraphs->setMenuText(tr("&All"));
  actionExportAllGraphs->setShortcut(tr("Alt+X"));
  actionExportAllGraphs->setToolTip(tr("Export all graphs"));

  actionExportPDF->setMenuText(tr("&Export PDF"));
  actionExportPDF->setShortcut(tr("Ctrl+Alt+P"));
  actionExportPDF->setToolTip(tr("Export to PDF"));

  actionPrint->setMenuText(tr("&Print"));
  actionPrint->setShortcut(tr("Ctrl+P"));
  actionPrint->setToolTip(tr("Print window"));

  actionPrintAllPlots->setMenuText(tr("Print All Plo&ts"));
  actionShowExportASCIIDialog->setMenuText(tr("E&xport ASCII"));

  actionCloseAllWindows->setMenuText(tr("&Quit"));
  actionCloseAllWindows->setShortcut(tr("Ctrl+Q"));

  actionClearLogInfo->setMenuText(tr("Clear &Log Information"));
  actionClearConsole->setMenuText(tr("Clear &Console"));
  actionDeleteFitTables->setMenuText(tr("Delete &Fit Tables"));

  actionToolBars->setMenuText(tr("&Toolbars..."));
  actionToolBars->setShortcut(tr("Ctrl+Shift+T"));

  actionShowPlotWizard->setMenuText(tr("Plot &Wizard"));
  actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));

  actionShowConfigureDialog->setMenuText(tr("&Preferences..."));

  actionShowCurvesDialog->setMenuText(tr("Add/Remove &Curve..."));
  actionShowCurvesDialog->setShortcut(tr("ALT+C"));
  actionShowCurvesDialog->setToolTip(tr("Add curve to graph"));

  actionAddErrorBars->setMenuText(tr("Add &Error Bars..."));
  actionAddErrorBars->setToolTip(tr("Add Error Bars..."));
  actionAddErrorBars->setShortcut(tr("Ctrl+B"));

  actionAddFunctionCurve->setMenuText(tr("Add &Function..."));
  actionAddFunctionCurve->setToolTip(tr("Add Function..."));
  actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));

  actionUnzoom->setMenuText(tr("&Rescale to Show All"));
  actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
  actionUnzoom->setToolTip(tr("Best fit"));

  actionNewLegend->setMenuText( tr("New &Legend"));
  actionNewLegend->setShortcut(tr("Ctrl+L"));
  actionNewLegend->setToolTip(tr("Add new legend"));

  actionTimeStamp->setMenuText(tr("Add Time Stamp"));
  actionTimeStamp->setShortcut(tr("Ctrl+ALT+T"));
  actionTimeStamp->setToolTip(tr("Date & time "));

  actionAddImage->setMenuText(tr("Add &Image"));
  actionAddImage->setToolTip(tr("Add Image"));
  actionAddImage->setShortcut(tr("ALT+I"));

  actionPlotL->setMenuText(tr("&Line"));
  actionPlotL->setToolTip(tr("Plot as line"));

  actionPlotP->setMenuText(tr("&Scatter"));
  actionPlotP->setToolTip(tr("Plot as symbols"));

  actionPlotLP->setMenuText(tr("Line + S&ymbol"));
  actionPlotLP->setToolTip(tr("Plot as line + symbols"));

  actionPlotVerticalDropLines->setMenuText(tr("Vertical &Drop Lines"));

  actionPlotSpline->setMenuText(tr("&Spline"));
  actionPlotVertSteps->setMenuText(tr("&Vertical Steps"));
  actionPlotHorSteps->setMenuText(tr("&Horizontal Steps"));

  actionPlotVerticalBars->setMenuText(tr("&Columns"));
  actionPlotVerticalBars->setToolTip(tr("Plot with vertical bars"));

  actionPlotHorizontalBars->setMenuText(tr("&Rows"));
  actionPlotHorizontalBars->setToolTip(tr("Plot with horizontal bars"));

  actionPlotArea->setMenuText(tr("&Area"));
  actionPlotArea->setToolTip(tr("Plot area"));

  actionPlotPie->setMenuText(tr("&Pie"));
  actionPlotPie->setToolTip(tr("Plot pie"));

  actionPlotVectXYXY->setMenuText(tr("&Vectors XYXY"));
  actionPlotVectXYXY->setToolTip(tr("Vectors XYXY"));

  actionPlotVectXYAM->setMenuText(tr("Vectors XY&AM"));
  actionPlotVectXYAM->setToolTip(tr("Vectors XYAM"));

  actionPlotHistogram->setMenuText( tr("&Histogram"));
  actionPlotStackedHistograms->setMenuText(tr("&Stacked Histogram"));
  actionPlot2VerticalLayers->setMenuText(tr("&Vertical 2 Layers"));
  actionPlot2HorizontalLayers->setMenuText(tr("&Horizontal 2 Layers"));
  actionPlot4Layers->setMenuText(tr("&4 Layers"));
  actionPlotStackedLayers->setMenuText(tr("&Stacked Layers"));

  actionPlot3DRibbon->setMenuText(tr("&Ribbon"));
  actionPlot3DRibbon->setToolTip(tr("Plot 3D ribbon"));

  actionPlot3DBars->setMenuText(tr("&Bars"));
  actionPlot3DBars->setToolTip(tr("Plot 3D bars"));

  actionPlot3DScatter->setMenuText(tr("&Scatter"));
  actionPlot3DScatter->setToolTip(tr("Plot 3D scatter"));

  actionPlot3DTrajectory->setMenuText(tr("&Trajectory"));
  actionPlot3DTrajectory->setToolTip(tr("Plot 3D trajectory"));

  actionColorMap->setMenuText(tr("Contour + &Color Fill"));
  actionColorMap->setToolTip(tr("Contour Lines + Color Fill"));

  actionNoContourColorMap->setMenuText(tr("Color &Fill"));
  actionNoContourColorMap->setToolTip(tr("Color Fill (No contours)"));

  actionContourMap->setMenuText(tr("Contour &Lines"));
  actionContourMap->setToolTip(tr("Contour Lines"));

  actionGrayMap->setMenuText(tr("&Gray Scale Map"));
  actionGrayMap->setToolTip(tr("Gray Scale Map"));

  actionShowColStatistics->setMenuText(tr("Statistics on &Columns"));
  actionShowColStatistics->setToolTip(tr("Selected columns statistics"));

  actionShowRowStatistics->setMenuText(tr("Statistics on &Rows"));
  actionShowRowStatistics->setToolTip(tr("Selected rows statistics"));
  actionShowIntDialog->setMenuText(tr("Integr&ate Function..."));
  actionIntegrate->setMenuText(tr("&Integrate"));
  actionInterpolate->setMenuText(tr("Inte&rpolate ..."));
  actionLowPassFilter->setMenuText(tr("&Low Pass..."));
  actionHighPassFilter->setMenuText(tr("&High Pass..."));
  actionBandPassFilter->setMenuText(tr("&Band Pass..."));
  actionBandBlockFilter->setMenuText(tr("&Band Block..."));
  actionFFT->setMenuText(tr("&FFT..."));
  actionSmoothSavGol->setMenuText(tr("&Savitzky-Golay..."));
  actionSmoothFFT->setMenuText(tr("&FFT Filter..."));
  actionSmoothAverage->setMenuText(tr("Moving Window &Average..."));
  actionDifferentiate->setMenuText(tr("&Differentiate"));
  actionFitLinear->setMenuText(tr("Fit &Linear"));
  actionShowFitPolynomDialog->setMenuText(tr("Fit &Polynomial ..."));
  actionShowExpDecayDialog->setMenuText(tr("&First Order ..."));
  actionShowTwoExpDecayDialog->setMenuText(tr("&Second Order ..."));
  actionShowExpDecay3Dialog->setMenuText(tr("&Third Order ..."));
  actionFitExpGrowth->setMenuText(tr("Fit Exponential Gro&wth ..."));
  actionFitSigmoidal->setMenuText(tr("Fit &Boltzmann (Sigmoidal)"));
  actionFitGauss->setMenuText(tr("Fit &Gaussian"));
  actionFitLorentz->setMenuText(tr("Fit Lorent&zian"));

  actionShowFitDialog->setMenuText(tr("Fit &Wizard..."));
  actionShowFitDialog->setShortcut(tr("Ctrl+Y"));

  actionShowPlotDialog->setMenuText(tr("&Plot ..."));
  actionShowScaleDialog->setMenuText(tr("&Scales..."));
  actionShowAxisDialog->setMenuText(tr("&Axes..."));
  actionShowGridDialog->setMenuText(tr("&Grid ..."));
  actionShowTitleDialog->setMenuText(tr("&Title ..."));
  actionShowColumnOptionsDialog->setMenuText(tr("Column &Options ..."));
  actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
  actionShowColumnValuesDialog->setMenuText(tr("Set Column &Values ..."));
  actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));
  actionTableRecalculate->setMenuText(tr("Recalculate"));
  actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
  actionHideSelectedColumns->setMenuText(tr("&Hide Selected"));
  actionHideSelectedColumns->setToolTip(tr("Hide selected columns"));
  actionShowAllColumns->setMenuText(tr("Sho&w All Columns"));
  actionHideSelectedColumns->setToolTip(tr("Show all table columns"));
  actionSwapColumns->setMenuText(tr("&Swap columns"));
  actionSwapColumns->setToolTip(tr("Swap selected columns"));
  actionMoveColRight->setMenuText(tr("Move &Right"));
  actionMoveColRight->setToolTip(tr("Move Right"));
  actionMoveColLeft->setMenuText(tr("Move &Left"));
  actionMoveColLeft->setToolTip(tr("Move Left"));
  actionMoveColFirst->setMenuText(tr("Move to F&irst"));
  actionMoveColFirst->setToolTip(tr("Move to First"));
  actionMoveColLast->setMenuText(tr("Move to Las&t"));
  actionMoveColLast->setToolTip(tr("Move to Last"));
  actionShowColsDialog->setMenuText(tr("&Columns..."));
  actionShowRowsDialog->setMenuText(tr("&Rows..."));
  actionDeleteRows->setMenuText(tr("&Delete Rows Interval..."));

  actionAbout->setMenuText(tr("&About MantidPlot"));//Mantid
  actionAbout->setShortcut(tr("F1"));

  //actionShowHelp->setMenuText(tr("&Help"));
  //actionShowHelp->setShortcut(tr("Ctrl+H"));

  actionMantidConcepts->setMenuText(tr("&Mantid Concepts"));

  actionMantidAlgorithms->setMenuText("&Algorithm Descriptions");

  actionmantidplotHelp->setMenuText("&MantidPlot Help");

  //actionChooseHelpFolder->setMenuText(tr("&Choose Help Folder..."));
  //actionRename->setMenuText(tr("&Rename Window"));

  actionCloseWindow->setMenuText(tr("Close &Window"));
  actionCloseWindow->setShortcut(tr("Ctrl+W"));

  actionAddColToTable->setMenuText(tr("Add Column"));
  actionAddColToTable->setToolTip(tr("Add Column"));

  actionClearTable->setMenuText(tr("Clear"));
  actionGoToRow->setMenuText(tr("&Go to Row..."));
  actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));

  actionGoToColumn->setMenuText(tr("Go to Colum&n..."));
  actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));

  actionDeleteLayer->setMenuText(tr("&Remove Layer"));
  actionDeleteLayer->setShortcut(tr("Alt+R"));

  actionResizeActiveWindow->setMenuText(tr("Window &Geometry..."));
  actionHideActiveWindow->setMenuText(tr("&Hide Window"));
  actionShowMoreWindows->setMenuText(tr("More Windows..."));
  actionPixelLineProfile->setMenuText(tr("&View Pixel Line Profile"));
  actionIntensityTable->setMenuText(tr("&Intensity Table"));
  actionShowLineDialog->setMenuText(tr("&Properties"));
  actionShowImageDialog->setMenuText(tr("&Properties"));
  actionShowTextDialog->setMenuText(tr("&Properties"));
  actionActivateWindow->setMenuText(tr("&Activate Window"));
  actionMinimizeWindow->setMenuText(tr("Mi&nimize Window"));
  actionMaximizeWindow->setMenuText(tr("Ma&ximize Window"));
  actionHideWindow->setMenuText(tr("&Hide Window"));
  actionResizeWindow->setMenuText(tr("Re&size Window..."));
  actionEditSurfacePlot->setMenuText(tr("&Surface..."));
  actionAdd3DData->setMenuText(tr("&Data Set..."));
  actionSetMatrixProperties->setMenuText(tr("Set &Properties..."));
  actionSetMatrixDimensions->setMenuText(tr("Set &Dimensions..."));
  actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));
  actionSetMatrixValues->setMenuText(tr("Set &Values..."));
  actionSetMatrixValues->setToolTip(tr("Set Matrix Values"));
  actionSetMatrixValues->setShortcut(tr("Alt+Q"));
  actionImagePlot->setMenuText(tr("&Image Plot"));
  actionImagePlot->setToolTip(tr("Image Plot"));
  actionTransposeMatrix->setMenuText(tr("&Transpose"));
  actionRotateMatrix->setMenuText(tr("R&otate 90"));
  actionRotateMatrix->setToolTip(tr("Rotate 90° Clockwise"));
  actionRotateMatrixMinus->setMenuText(tr("Rotate &-90"));
  actionRotateMatrixMinus->setToolTip(tr("Rotate 90° Counterclockwise"));
  actionFlipMatrixVertically->setMenuText(tr("Flip &V"));
  actionFlipMatrixVertically->setToolTip(tr("Flip Vertically"));
  actionFlipMatrixHorizontally->setMenuText(tr("Flip &H"));
  actionFlipMatrixHorizontally->setToolTip(tr("Flip Horizontally"));

  actionMatrixXY->setMenuText(tr("Show &X/Y"));
  actionMatrixColumnRow->setMenuText(tr("Show &Column/Row"));
  actionViewMatrix->setMenuText(tr("&Data mode"));
  actionViewMatrixImage->setMenuText(tr("&Image mode"));
  actionMatrixGrayScale->setMenuText(tr("&Gray Scale"));
  actionMatrixRainbowScale->setMenuText(tr("&Rainbow"));
  actionMatrixCustomScale->setMenuText(tr("&Custom"));
  actionInvertMatrix->setMenuText(tr("&Invert"));
  actionMatrixDeterminant->setMenuText(tr("&Determinant"));
  actionConvertMatrixDirect->setMenuText(tr("&Direct"));
  actionConvertMatrixXYZ->setMenuText(tr("&XYZ Columns"));
  actionConvertMatrixYXZ->setMenuText(tr("&YXZ Columns"));
  actionExportMatrix->setMenuText(tr("&Export Image ..."));

  actionConvertTable->setMenuText(tr("Convert to &Matrix"));
  actionPlot3DWireFrame->setMenuText(tr("3D &Wire Frame"));
  actionPlot3DHiddenLine->setMenuText(tr("3D &Hidden Line"));
  actionPlot3DPolygons->setMenuText(tr("3D &Polygons"));
  actionPlot3DWireSurface->setMenuText(tr("3D Wire &Surface"));
  actionSortTable->setMenuText(tr("Sort Ta&ble"));
  actionSortSelection->setMenuText(tr("Sort Columns"));
  actionNormalizeTable->setMenuText(tr("&Table"));
  actionNormalizeSelection->setMenuText(tr("&Columns"));
  actionCorrelate->setMenuText(tr("Co&rrelate"));
  actionAutoCorrelate->setMenuText(tr("&Autocorrelate"));
  actionConvolute->setMenuText(tr("&Convolute"));
  actionDeconvolute->setMenuText(tr("&Deconvolute"));
  actionTranslateHor->setMenuText(tr("&Horizontal"));
  actionTranslateVert->setMenuText(tr("&Vertical"));
  actionSetAscValues->setMenuText(tr("Ro&w Numbers"));
  actionSetAscValues->setToolTip(tr("Fill selected columns with row numbers"));
  actionSetRandomValues->setMenuText(tr("&Random Values"));
  actionSetRandomValues->setToolTip(tr("Fill selected columns with random numbers"));
  actionSetXCol->setMenuText(tr("&X"));
  actionSetXCol->setToolTip(tr("Set column as X"));
  actionSetYCol->setMenuText(tr("&Y"));
  actionSetYCol->setToolTip(tr("Set column as Y"));
  actionSetZCol->setMenuText(tr("&Z"));
  actionSetZCol->setToolTip(tr("Set column as Z"));
  actionSetXErrCol->setMenuText(tr("X E&rror"));
  actionSetYErrCol->setMenuText(tr("Y &Error"));
  actionSetYErrCol->setToolTip(tr("Set as Y Error Bars"));
  actionSetLabelCol->setMenuText(tr("&Label"));
  actionSetLabelCol->setToolTip(tr("Set as Labels"));
  actionDisregardCol->setMenuText(tr("&Disregard"));
  actionDisregardCol->setToolTip(tr("Disregard Columns"));
  actionReadOnlyCol->setMenuText(tr("&Read Only"));

  actionBoxPlot->setMenuText(tr("&Box Plot"));
  actionBoxPlot->setToolTip(tr("Box and whiskers plot"));

  actionMultiPeakGauss->setMenuText(tr("&Gaussian..."));
  actionMultiPeakLorentz->setMenuText(tr("&Lorentzian..."));
  actionHomePage->setMenuText(tr("&Mantid Homepage")); // Mantid change
  //actionCheckUpdates->setMenuText(tr("Search for &Updates")); //Mantid change - commented out
  //actionHelpForums->setText(tr("Visit QtiPlot &Forums"));
  actionHelpBugReports->setText(tr("Report a &Bug"));
  //actionDownloadManual->setMenuText(tr("Download &Manual"));//Mantid change - commented out
  //actionTranslations->setMenuText(tr("&Translations"));//Mantid change - commented out
  //actionDonate->setMenuText(tr("Make a &Donation"));
  //actionTechnicalSupport->setMenuText(tr("Technical &Support"));

  //#ifdef SCRIPTING_DIALOG
  //	actionScriptingLang->setMenuText(tr("Scripting &language"));
  //#endif

  actionNoteExecute->setMenuText(tr("E&xecute"));
  actionNoteExecute->setShortcut(tr("Ctrl+J"));

  actionNoteExecuteAll->setMenuText(tr("Execute &All"));
  actionNoteExecuteAll->setShortcut(tr("Ctrl+Shift+J"));

  actionNoteEvaluate->setMenuText(tr("&Evaluate Expression"));
  actionNoteEvaluate->setShortcut(tr("Ctrl+Return"));

  btnPointer->setMenuText(tr("Disable &tools"));
  btnPointer->setToolTip( tr( "Pointer" ) );

  btnZoomIn->setMenuText(tr("&Zoom In"));
  btnZoomIn->setShortcut(tr("Ctrl++"));
  btnZoomIn->setToolTip(tr("Zoom In"));

  btnZoomOut->setMenuText(tr("Zoom &Out"));
  btnZoomOut->setShortcut(tr("Ctrl+-"));
  btnZoomOut->setToolTip(tr("Zoom Out"));

  actionMagnify->setMenuText(tr("Zoom &In/Out and Drag Canvas"));
  actionMagnify->setToolTip(tr("Zoom In (Shift++) or Out (-) and Drag Canvas"));

  btnCursor->setMenuText(tr("&Data Reader"));
  btnCursor->setShortcut(tr("CTRL+D"));
  btnCursor->setToolTip(tr("Data reader"));

  btnSelect->setMenuText(tr("&Select Data Range"));
  btnSelect->setShortcut(tr("ALT+S"));
  btnSelect->setToolTip(tr("Select data range"));

  btnPicker->setMenuText(tr("S&creen Reader"));
  btnPicker->setToolTip(tr("Screen reader"));

  actionDrawPoints->setMenuText(tr("&Draw Data Points"));
  actionDrawPoints->setToolTip(tr("Draw Data Points"));

  btnMovePoints->setMenuText(tr("&Move Data Points..."));
  btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
  btnMovePoints->setToolTip(tr("Move data points"));

  btnRemovePoints->setMenuText(tr("Remove &Bad Data Points..."));
  btnRemovePoints->setShortcut(tr("Alt+B"));
  btnRemovePoints->setToolTip(tr("Remove data points"));

  actionAddText->setMenuText(tr("Add &Text"));
  actionAddText->setToolTip(tr("Add Text"));
  actionAddText->setShortcut(tr("ALT+T"));

  btnArrow->setMenuText(tr("Draw &Arrow"));
  btnArrow->setShortcut(tr("CTRL+ALT+A"));
  btnArrow->setToolTip(tr("Draw arrow"));

  btnLine->setMenuText(tr("Draw &Line"));
  btnLine->setShortcut(tr("CTRL+ALT+L"));
  btnLine->setToolTip(tr("Draw line"));

  // FIXME: is setText necessary for action groups?
  //	coord->setText( tr( "Coordinates" ) );
  //	coord->setMenuText( tr( "&Coord" ) );
  //  coord->setStatusTip( tr( "Coordinates" ) );
  Box->setText( tr( "Box" ) );
  Box->setMenuText( tr( "Box" ) );
  Box->setToolTip( tr( "Box" ) );
  Box->setStatusTip( tr( "Box" ) );
  Frame->setText( tr( "Frame" ) );
  Frame->setMenuText( tr( "&Frame" ) );
  Frame->setToolTip( tr( "Frame" ) );
  Frame->setStatusTip( tr( "Frame" ) );
  None->setText( tr( "No Axes" ) );
  None->setMenuText( tr( "No Axes" ) );
  None->setToolTip( tr( "No axes" ) );
  None->setStatusTip( tr( "No axes" ) );

  front->setToolTip( tr( "Front grid" ) );
  back->setToolTip( tr( "Back grid" ) );
  right->setToolTip( tr( "Right grid" ) );
  left->setToolTip( tr( "Left grid" ) );
  ceil->setToolTip( tr( "Ceiling grid" ) );
  floor->setToolTip( tr( "Floor grid" ) );

  wireframe->setText( tr( "Wireframe" ) );
  wireframe->setMenuText( tr( "Wireframe" ) );
  wireframe->setToolTip( tr( "Wireframe" ) );
  wireframe->setStatusTip( tr( "Wireframe" ) );
  hiddenline->setText( tr( "Hidden Line" ) );
  hiddenline->setMenuText( tr( "Hidden Line" ) );
  hiddenline->setToolTip( tr( "Hidden line" ) );
  hiddenline->setStatusTip( tr( "Hidden line" ) );
  polygon->setText( tr( "Polygon Only" ) );
  polygon->setMenuText( tr( "Polygon Only" ) );
  polygon->setToolTip( tr( "Polygon only" ) );
  polygon->setStatusTip( tr( "Polygon only" ) );
  filledmesh->setText( tr( "Mesh & Filled Polygons" ) );
  filledmesh->setMenuText( tr( "Mesh & Filled Polygons" ) );
  filledmesh->setToolTip( tr( "Mesh & filled Polygons" ) );
  filledmesh->setStatusTip( tr( "Mesh & filled Polygons" ) );
  pointstyle->setText( tr( "Dots" ) );
  pointstyle->setMenuText( tr( "Dots" ) );
  pointstyle->setToolTip( tr( "Dots" ) );
  pointstyle->setStatusTip( tr( "Dots" ) );
  barstyle->setText( tr( "Bars" ) );
  barstyle->setMenuText( tr( "Bars" ) );
  barstyle->setToolTip( tr( "Bars" ) );
  barstyle->setStatusTip( tr( "Bars" ) );
  conestyle->setText( tr( "Cones" ) );
  conestyle->setMenuText( tr( "Cones" ) );
  conestyle->setToolTip( tr( "Cones" ) );
  conestyle->setStatusTip( tr( "Cones" ) );
  crossHairStyle->setText( tr( "Crosshairs" ) );
  crossHairStyle->setMenuText( tr( "Crosshairs" ) );
  crossHairStyle->setToolTip( tr( "Crosshairs" ) );
  crossHairStyle->setStatusTip( tr( "Crosshairs" ) );

  //floorstyle->setText( tr( "Floor Style" ) );
  //floorstyle->setMenuText( tr( "Floor Style" ) );
  //floorstyle->setStatusTip( tr( "Floor Style" ) );
  floordata->setText( tr( "Floor Data Projection" ) );
  floordata->setMenuText( tr( "Floor Data Projection" ) );
  floordata->setToolTip( tr( "Floor data projection" ) );
  floordata->setStatusTip( tr( "Floor data projection" ) );
  flooriso->setText( tr( "Floor Isolines" ) );
  flooriso->setMenuText( tr( "Floor Isolines" ) );
  flooriso->setToolTip( tr( "Floor isolines" ) );
  flooriso->setStatusTip( tr( "Floor isolines" ) );
  floornone->setText( tr( "Empty Floor" ) );
  floornone->setMenuText( tr( "Empty Floor" ) );
  floornone->setToolTip( tr( "Empty floor" ) );
  floornone->setStatusTip( tr( "Empty floor" ) );

  actionAnimate->setText( tr( "Animation" ) );
  actionAnimate->setMenuText( tr( "Animation" ) );
  actionAnimate->setToolTip( tr( "Animation" ) );
  actionAnimate->setStatusTip( tr( "Animation" ) );

  actionPerspective->setText( tr( "Enable perspective" ) );
  actionPerspective->setMenuText( tr( "Enable perspective" ) );
  actionPerspective->setToolTip( tr( "Enable perspective" ) );
  actionPerspective->setStatusTip( tr( "Enable perspective" ) );

  actionResetRotation->setText( tr( "Reset rotation" ) );
  actionResetRotation->setMenuText( tr( "Reset rotation" ) );
  actionResetRotation->setToolTip( tr( "Reset rotation" ) );
  actionResetRotation->setStatusTip( tr( "Reset rotation" ) );

  actionFitFrame->setText( tr( "Fit frame to window" ) );
  actionFitFrame->setMenuText( tr( "Fit frame to window" ) );
  actionFitFrame->setToolTip( tr( "Fit frame to window" ) );
  actionFitFrame->setStatusTip( tr( "Fit frame to window" ) );


}

Graph3D * ApplicationWindow::openMatrixPlot3D(const QString& caption, const QString& matrix_name,
    double xl,double xr,double yl,double yr,double zl,double zr)
{
  QString name = matrix_name;
  name.remove("matrix<", true);
  name.remove(">");
  Matrix* m = matrix(name);
  if (!m)
    return 0;

  Graph3D *plot = new Graph3D("", this, 0, 0);
  plot->setWindowTitle(caption);
  plot->setName(caption);
  plot->addMatrixData(m, xl, xr, yl, yr, zl, zr);
  plot->update();

  initPlot3D(plot);
  return plot;
}

Graph3D * ApplicationWindow::plot3DMatrix(Matrix *m, int style)
{
  if (!m) {
    //Mantid
    Graph3D *plot = mantidUI->plot3DMatrix(style);
    if (plot) return plot;
    m = (Matrix*)activeWindow(MatrixWindow);
    if (!m)
      return 0;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this, 0);
  plot->addMatrixData(m);
  plot->customPlotStyle(style);
  customPlot3D(plot);
  plot->update();

  plot->resize(500, 400);
  plot->setWindowTitle(label);
  plot->setName(label);
  initPlot3D(plot);

  emit modified();
  QApplication::restoreOverrideCursor();
  return plot;
}

MultiLayer* ApplicationWindow::plotGrayScale(Matrix *m)
{
  if (!m) {
    //Mantid
    MultiLayer* plot = mantidUI->plotSpectrogram(Graph::GrayScale);
    if (plot) return plot;
    m = (Matrix*)activeWindow(MatrixWindow);
    if (!m)
      return 0;
  }

  return plotSpectrogram(m, Graph::GrayScale);
}

MultiLayer* ApplicationWindow::plotContour(Matrix *m)
{
  if (!m) {
    //Mantid
    MultiLayer* plot = mantidUI->plotSpectrogram(Graph::Contour);
    if (plot) return plot;
    m = (Matrix*)activeWindow(MatrixWindow);
    if (!m)
      return 0;
  }

  return plotSpectrogram(m, Graph::Contour);
}

MultiLayer* ApplicationWindow::plotColorMap(Matrix *m)
{
  if (!m) {
    //Mantid
    MultiLayer* plot = mantidUI->plotSpectrogram(Graph::ColorMap);
    if (plot) return plot;
    m = (Matrix*)activeWindow(MatrixWindow);
    if (!m)
      return 0;
  }

  return plotSpectrogram(m, Graph::ColorMap);
}

MultiLayer* ApplicationWindow::plotNoContourColorMap(Matrix *m)
{
  MultiLayer* ml = NULL;
  if( !m )
  {
    m = qobject_cast<Matrix*>(activeWindow(MatrixWindow));
  }
  if( m )
  {
    ml = plotSpectrogram(m, Graph::ColorMap);
  }
  else
  {
    ml =  mantidUI->plotSpectrogram(Graph::ColorMap);
  }
  if( !ml ) 
  {
    QApplication::restoreOverrideCursor();
    return 0;
  }

  Spectrogram *spgrm = dynamic_cast<Spectrogram*>(ml->activeGraph()->plotItem(0));
  if( spgrm )
  {
    //1 = ImageMode
    spgrm->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    spgrm->setDisplayMode(QwtPlotSpectrogram::ContourMode, false);
  }
  return ml;
}

MultiLayer* ApplicationWindow::plotImage(Matrix *m)
{
  MultiLayer *g = NULL;
  Graph *plot = NULL;
  if( !m )
  {
    m = qobject_cast<Matrix*>(activeWindow(MatrixWindow));
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if( m )
  {
    g = multilayerPlot(generateUniqueName(tr("Graph")));
    plot = g->activeGraph();
    setPreferences(plot);

    Spectrogram *s = plot->plotSpectrogram(m, Graph::GrayScale);
    if( !s ) 
    {
      QApplication::restoreOverrideCursor();
      return 0;
    }
    s->setAxis(QwtPlot::xTop, QwtPlot::yLeft);
    plot->setScale(QwtPlot::xTop, QMIN(m->xStart(), m->xEnd()), QMAX(m->xStart(), m->xEnd()));
    plot->setScale(QwtPlot::yLeft, QMIN(m->yStart(), m->yEnd()), QMAX(m->yStart(), m->yEnd()),
        0.0, 5, 5, GraphOptions::Linear, true);
  }
  else
  {
    g =  mantidUI->plotSpectrogram(Graph::GrayScale);
    if( !g ) 
    {
      QApplication::restoreOverrideCursor();
      return 0;
    }
    plot = g->activeGraph();
    setPreferences(plot);
    if( plot->plotItem(0) )plot->plotItem(0)->setAxis(QwtPlot::xTop, QwtPlot::yLeft);
  }

  plot->enableAxis(QwtPlot::xTop, true);

  plot->enableAxis(QwtPlot::xBottom, false);
  plot->enableAxis(QwtPlot::yRight, false);
  plot->setAxisTitle(QwtPlot::yLeft, QString::null);
  plot->setAxisTitle(QwtPlot::xTop, QString::null);
  plot->setTitle(QString::null);

  emit modified();
  QApplication::restoreOverrideCursor();
  return g;
}

MultiLayer* ApplicationWindow::plotSpectrogram(Matrix *m, Graph::CurveType type)
{
  if (type == Graph::ImagePlot)
    return plotImage(m);
  else if (type == Graph::Histogram)
    return plotHistogram(m);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph* plot = g->activeGraph();
  setPreferences(plot);

  plot->plotSpectrogram(m, type);

  plot->setAutoScale();//Mantid

  QApplication::restoreOverrideCursor();
  return g;
}

ApplicationWindow* ApplicationWindow::importOPJ(const QString& filename, bool factorySettings, bool newProject)
{
  if (filename.endsWith(".opj", Qt::CaseInsensitive) || filename.endsWith(".ogg", Qt::CaseInsensitive))
  {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    ApplicationWindow *app = this;
    if (newProject)
      app = new ApplicationWindow(factorySettings);

    app->setWindowTitle("MantidPlot - " + filename);//Mantid
    app->restoreApplicationGeometry();
    app->projectname = filename;
    app->recentProjects.remove(filename);
    app->recentProjects.push_front(filename);
    app->updateRecentProjectsList();

    ImportOPJ(app, filename);

    QApplication::restoreOverrideCursor();
    return app;
  }
  else if (filename.endsWith(".ogm", Qt::CaseInsensitive) || filename.endsWith(".ogw", Qt::CaseInsensitive))
  {
    ImportOPJ(this, filename);
    recentProjects.remove(filename);
    recentProjects.push_front(filename);
    updateRecentProjectsList();
    return this;
  }
  return 0;
}

void ApplicationWindow::deleteFitTables()
{
  QList<QWidget*>* mLst = new QList<QWidget*>();
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer"))
      mLst->append(w);
  }

  foreach(QWidget *ml, *mLst){
    if (ml->isA("MultiLayer")){
      QList<Graph *> layers = ((MultiLayer*)ml)->layersList();
      foreach(Graph *g, layers){
        QList<QwtPlotCurve *> curves = g->fitCurvesList();
        foreach(QwtPlotCurve *c, curves){
          if (((PlotCurve *)c)->type() != Graph::Function){
            Table *t = ((DataCurve *)c)->table();
            if (!t)
              continue;

            t->askOnCloseEvent(false);
            t->close();
          }
        }
      }
    }
  }
  delete mLst;
}

QList<MdiSubWindow *> ApplicationWindow::windowsList()
{
  QList<MdiSubWindow *> lst;

  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows)
    lst << w;
    f = f->folderBelow();
  }
  return lst;
}

void ApplicationWindow::updateRecentProjectsList()
{
  if (recentProjects.isEmpty())
    return;

  while ((int)recentProjects.size() > MaxRecentProjects)
    recentProjects.pop_back();

  recent->clear();

  for (int i = 0; i<(int)recentProjects.size(); i++ )
    recent->insertItem("&" + QString::number(i+1) + " " + recentProjects[i]);
}

void ApplicationWindow::translateCurveHor()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  if (g->isPiePlot())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));

    btnPointer->setChecked(true);
    return;
  }
  else if (g->validCurvesDataSize())
  {
    btnPointer->setChecked(true);
    g->setActiveTool(new TranslateCurveTool(g, this, TranslateCurveTool::Horizontal, info, SLOT(setText(const QString&))));
    displayBar->show();
  }
}

void ApplicationWindow::translateCurveVert()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g)
    return;

  if (g->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));

    btnPointer->setChecked(true);
    return;
  } else if (g->validCurvesDataSize()) {
    btnPointer->setChecked(true);
    g->setActiveTool(new TranslateCurveTool(g, this, TranslateCurveTool::Vertical, info, SLOT(setText(const QString&))));
    displayBar->show();
  }
}

void ApplicationWindow::setReadOnlyCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), actionReadOnlyCol->isChecked());
}

void ApplicationWindow::setReadOnlyColumns()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]));
}

void ApplicationWindow::setReadWriteColumns()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), false);
}

void ApplicationWindow::setAscValues()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setAscValues();
}

void ApplicationWindow::setRandomValues()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setRandomValues();
}

void ApplicationWindow::setXErrCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::xErr);
}

void ApplicationWindow::setYErrCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::yErr);
}

void ApplicationWindow::setXCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::X);
}

void ApplicationWindow::setYCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::Y);
}

void ApplicationWindow::setZCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::Z);
}

void ApplicationWindow::setLabelCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::Label);
}

void ApplicationWindow::disregardCol()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  t->setPlotDesignation(Table::None);
}

void ApplicationWindow::fitMultiPeakGauss()
{
  fitMultiPeak((int)MultiPeakFit::Gauss);
}

void ApplicationWindow::fitMultiPeakLorentz()
{
  fitMultiPeak((int)MultiPeakFit::Lorentz);
}

void ApplicationWindow::fitMultiPeak(int profile)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = (Graph*)plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  if (g->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("This functionality is not available for pie plots!"));
    return;
  } else {
    bool ok;
    int peaks = QInputDialog::getInteger(tr("MantidPlot - Enter the number of peaks"),//Mantid
        tr("Peaks"), 2, 2, 1000000, 1, &ok, this);
    if (ok && peaks){
      g->setActiveTool(new MultiPeakFitTool(g, this, (MultiPeakFit::PeakProfile)profile, peaks, info, SLOT(setText(const QString&))));
      displayBar->show();
    }
  }
}

//void ApplicationWindow::showSupportPage()
//{
//	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/contracts.html"));
//}


//void ApplicationWindow::showDonationsPage()
//{
//	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/why_donate.html"));
//}

//void ApplicationWindow::downloadManual()
//{
//	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/manuals.html"));
//}
//
//void ApplicationWindow::downloadTranslation()
//{
//	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/translations.html"));
//}

void ApplicationWindow::showHomePage()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org"));
}
void ApplicationWindow::showMantidConcepts()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Category:Concepts"));
}
void ApplicationWindow::showalgorithmDescriptions()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Category:Algorithms"));
}
/*
 Show mantidplot help page
 */
void ApplicationWindow::showmantidplotHelp()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/MantidPlot:_Help"));
}

//
//void ApplicationWindow::showForums()
//{
//	QDesktopServices::openUrl(QUrl("https://developer.berlios.de/forum/?group_id=6626"));
//}

void ApplicationWindow::showBugTracker()
{
  QDesktopServices::openUrl(QUrl("mailto:mantid-help@mantidproject.org"));
}

void ApplicationWindow::parseCommandLineArguments(const QStringList& args)
{
  int num_args = args.count();
  if(num_args == 0){
    initWindow();
    savedProject();
    return;
  }

  QString str;
  bool exec = false;
  bool quit = false;
  bool default_settings = false;
  foreach(str, args){
    if( (str == "-a" || str == "--about") ||
        (str == "-m" || str == "--manual") )
    {
      QMessageBox::critical(this, tr("MantidPlot - Error"),//Mantid
          tr("<b> %1 </b>: This command line option must be used without other arguments!").arg(str));
    }
    else if( (str == "-d" || str == "--default-settings"))
    {
      default_settings = true;
    }
    else if (str == "-v" || str == "--version")
    {
#ifdef Q_OS_WIN
      hide();
      about();
#else
      std::wcout << versionString().toStdWString();
#endif
      exit(0);
    }
    else if (str == "-r" || str == "--revision") // Print and return subversion revision number
    {
      hide();
      QString version(MANTIDPLOT_RELEASE_VERSION);
      version.remove(0,4);
      std::cout << version.toStdString() << std::endl;
      exit(version.toInt());
    }
    else if (str == "-h" || str == "--help")
    {
      QString s = "\n" + tr("Usage") + ": ";
      s += "qtiplot [" + tr("options") + "] [" + tr("file") + "_" + tr("name") + "]\n\n";
      s += tr("Valid options are") + ":\n";
      s += "-a " + tr("or") + " --about: " + tr("show about dialog and exit") + "\n";
      s += "-d " + tr("or") + " --default-settings: " + tr("start MantidPlot with the default settings") + "\n";//Mantid
      s += "-h " + tr("or") + " --help: " + tr("show command line options") + "\n";
      s += "-l=XX " + tr("or") + " --lang=XX: " + tr("start MantidPlot in language") + " XX ('en', 'fr', 'de', ...)\n";//Mantid
      s += "-m " + tr("or") + " --manual: " + tr("show MantidPlot manual in a standalone window") + "\n";
      s += "-v " + tr("or") + " --version: " + tr("print MantidPlot version and release date") + "\n";
      s += "-x " + tr("or") + " --execute: " + tr("execute the script file given as argument") + "\n\n";
      s += "'" + tr("file") + "_" + tr("name") + "' " + tr("can be any .qti, qti.gz, .opj, .ogm, .ogw, .ogg, .py or ASCII file") + "\n";
#ifdef Q_OS_WIN
      hide();
      QMessageBox::information(this, tr("MantidPlot") + " - " + tr("Help"), s);//Mantid
#else
      std::wcout << s.toStdWString();
#endif
      exit(0);
    }
    else if (str.startsWith("--lang=") || str.startsWith("-l="))
    {
      QString locale = str.mid(str.find('=')+1);
      if (locales.contains(locale))
        switchToLanguage(locale);

      if (!locales.contains(locale))
        QMessageBox::critical(this, tr("MantidPlot - Error"),//Mantid
            tr("<b> %1 </b>: Wrong locale option or no translation available!").arg(locale));
    }
    else if (str.endsWith("--execute") || str.endsWith("-x"))
    {
      exec = true;
      quit = false;
    }
    else if (str.endsWith("--execandquit") || str.endsWith("-xq"))
    {
      exec = true;
      quit = true;
    }
    else if (str.startsWith("-") || str.startsWith("--"))
    {
      QMessageBox::critical(this, tr("MantidPlot - Error"),//Mantid
          tr("<b> %1 </b> unknown command line option!").arg(str) + "\n" + tr("Type %1 to see the list of the valid options.").arg("'MantidPlot -h'"));
    }
  }

  QString file_name = args[num_args-1]; // last argument
  if(file_name.startsWith("-")){// no file name given
    initWindow();
    savedProject();
    return;
  }

  if (!file_name.isEmpty()){
    QFileInfo fi(file_name);
    if (fi.isDir()){
      QMessageBox::critical(this, tr("MantidPlot - Error opening file"),//Mantid
          tr("<b>%1</b> is a directory, please specify a file name!").arg(file_name));
      return;
    } else if (!fi.exists()) {
      QMessageBox::critical(this, tr("MantidPlot - Error opening file"),//Mantid
          tr("The file: <b>%1</b> doesn't exist!").arg(file_name));
      return;
    } else if (!fi.isReadable()) {
      QMessageBox::critical(this, tr("MantidPlot - Error opening file"),//Mantid
          tr("You don't have the permission to open this file: <b>%1</b>").arg(file_name));
      return;
    }

    workingDir = fi.dirPath(true);
    saveSettings();//the recent projects must be saved

    if (exec)
      loadScript(file_name, exec, quit);
    else
    {
      saved=true;
      open(file_name, default_settings, false);
    }
  }
}

void ApplicationWindow::createLanguagesList()
{
  locales.clear();

  appTranslator = new QTranslator(this);
  qtTranslator = new QTranslator(this);
  qApp->installTranslator(appTranslator);
  qApp->installTranslator(qtTranslator);

  QString qmPath = d_translations_folder;
  QDir dir(qmPath);
  QStringList fileNames = dir.entryList("qtiplot_*.qm");
  for (int i=0; i < (int)fileNames.size(); i++)
  {
    QString locale = fileNames[i];
    locale = locale.mid(locale.find('_')+1);
    locale.truncate(locale.find('.'));
    locales.push_back(locale);
  }
  locales.push_back("en");
  locales.sort();

  if (appLanguage != "en")
  {
    appTranslator->load("qtiplot_" + appLanguage, qmPath);
    qtTranslator->load("qt_" + appLanguage, qmPath+"/qt");
  }
}

void ApplicationWindow::switchToLanguage(int param)
{
  if (param < (int)locales.size())
    switchToLanguage(locales[param]);
}

void ApplicationWindow::switchToLanguage(const QString& locale)
{
  if (!locales.contains(locale) || appLanguage == locale)
    return;

  appLanguage = locale;
  if (locale == "en")
  {
    qApp->removeTranslator(appTranslator);
    qApp->removeTranslator(qtTranslator);
    delete appTranslator;
    delete qtTranslator;
    appTranslator = new QTranslator(this);
    qtTranslator = new QTranslator(this);
    qApp->installTranslator(appTranslator);
    qApp->installTranslator(qtTranslator);
  }
  else
  {
    QString qmPath = d_translations_folder;
    appTranslator->load("qtiplot_" + locale, qmPath);
    qtTranslator->load("qt_" + locale, qmPath+"/qt");
  }
  insertTranslatedStrings();
}

QStringList ApplicationWindow::matrixNames()
{
  QStringList names;
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->isA("Matrix"))
        names << w->objectName();
    }
    f = f->folderBelow();
  }
  return names;
}
QStringList ApplicationWindow::mantidmatrixNames()
{
  QStringList names;
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->isA("MantidMatrix"))
        names << w->objectName();
    }
    f = f->folderBelow();
  }
  return names;
}

bool ApplicationWindow::alreadyUsedName(const QString& label)
{
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->objectName() == label)
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

bool ApplicationWindow::projectHas2DPlots()
{
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->isA("MultiLayer"))
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

void ApplicationWindow::appendProject()
{
  OpenProjectDialog *open_dialog = new OpenProjectDialog(this, false);
  open_dialog->setDirectory(workingDir);
  open_dialog->setExtensionWidget(0);
  if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
    return;
  workingDir = open_dialog->directory().path();
  appendProject(open_dialog->selectedFiles()[0]);
}

Folder* ApplicationWindow::appendProject(const QString& fn, Folder* parentFolder)
{
  if (fn.isEmpty())
    return 0;

  QFileInfo fi(fn);
  workingDir = fi.dirPath(true);

  if (fn.contains(".qti") || fn.contains(".opj", Qt::CaseInsensitive) ||
      fn.contains(".ogm", Qt::CaseInsensitive) || fn.contains(".ogw", Qt::CaseInsensitive) ||
      fn.contains(".ogg", Qt::CaseInsensitive)){
    QFileInfo f(fn);
    if (!f.exists ()){
      QMessageBox::critical(this, tr("MantidPlot - File opening error"),//Mantid
          tr("The file: <b>%1</b> doesn't exist!").arg(fn));
      return 0;
    }
  }else{
    QMessageBox::critical(this,tr("MantidPlot - File opening error"),//Mantid
        tr("The file: <b>%1</b> is not a MantidPlot or Origin project file!").arg(fn));
    return 0;
  }

  recentProjects.remove(fn);
  recentProjects.push_front(fn);
  updateRecentProjectsList();

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString fname = fn;
  if (fn.contains(".qti.gz")){//decompress using zlib
    file_uncompress((char *)fname.ascii());
    fname.remove(".gz");
  }

  Folder *cf = current_folder;
  if (parentFolder)
    changeFolder(parentFolder, true);

  FolderListItem *item = (FolderListItem *)current_folder->folderListItem();
  folders->blockSignals (true);
  blockSignals (true);

  QString baseName = fi.baseName();
  QStringList lst = current_folder->subfolders();
  int n = lst.count(baseName);
  if (n){//avoid identical subfolder names
    while (lst.count(baseName + QString::number(n)))
      n++;
    baseName += QString::number(n);
  }

  Folder *new_folder;
  if (parentFolder)
    new_folder = new Folder(parentFolder, baseName);
  else
    new_folder = new Folder(current_folder, baseName);

  current_folder = new_folder;
  FolderListItem *fli = new FolderListItem(item, current_folder);
  current_folder->setFolderListItem(fli);

  if (fn.contains(".opj", Qt::CaseInsensitive) || fn.contains(".ogm", Qt::CaseInsensitive) ||
      fn.contains(".ogw", Qt::CaseInsensitive) || fn.contains(".ogg", Qt::CaseInsensitive))
    ImportOPJ(this, fn);
  else{
    QFile f(fname);
    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    f.open(QIODevice::ReadOnly);

    QString s = t.readLine();
    lst = s.split(QRegExp("\\s"), QString::SkipEmptyParts);
    QString version = lst[1];
    lst = version.split(".", QString::SkipEmptyParts);
    d_file_version =100*(lst[0]).toInt()+10*(lst[1]).toInt()+(lst[2]).toInt();

    t.readLine();
    if (d_file_version < 73)
      t.readLine();

    //process tables and matrix information
    while ( !t.atEnd()){
      s = t.readLine();
      lst.clear();
      if  (s.left(8) == "<folder>"){
        lst = s.split("\t");
        Folder *f = new Folder(current_folder, lst[1]);
        f->setBirthDate(lst[2]);
        f->setModificationDate(lst[3]);
        if(lst.count() > 4)
          if (lst[4] == "current")
            cf = f;

        FolderListItem *fli = new FolderListItem(current_folder->folderListItem(), f);
        fli->setText(0, lst[1]);
        f->setFolderListItem(fli);

        current_folder = f;
      }else if  (s == "<table>"){
        while ( s!="</table>" ){
          s=t.readLine();
          lst<<s;
        }
        lst.pop_back();
        openTable(this,lst);
      }else if  (s == "<matrix>"){
        while ( s != "</matrix>" ){
          s=t.readLine();
          lst<<s;
        }
        lst.pop_back();
        openMatrix(this, lst);
      }else if  (s == "<note>"){
        for (int i=0; i<3; i++){
          s = t.readLine();
          lst << s;
        }
        Note* m = openNote(this, lst);
        QStringList cont;
        while ( s != "</note>" ){
          s=t.readLine();
          cont << s;
        }
        cont.pop_back();
        m->restore(cont);
      }else if  (s == "</folder>"){
        Folder *parent = (Folder *)current_folder->parent();
        if (!parent)
          current_folder = projectFolder();
        else
          current_folder = parent;
      }
    }
    f.close();

    //process the rest
    f.open(QIODevice::ReadOnly);

    MultiLayer *plot=0;
    while ( !t.atEnd()){
      s=t.readLine();
      if  (s.left(8) == "<folder>"){
        lst = s.split("\t");
        current_folder = current_folder->findSubfolder(lst[1]);
      }else if  (s == "<multiLayer>"){//process multilayers information
        s=t.readLine();
        QStringList graph=s.split("\t");
        QString caption=graph[0];
        plot = multilayerPlot(caption, 0, graph[2].toInt(), graph[1].toInt());
        setListViewDate(caption, graph[3]);
        plot->setBirthDate(graph[3]);
        plot->blockSignals(true);

        restoreWindowGeometry(this, plot, t.readLine());

        if (d_file_version > 71){
          QStringList lst=t.readLine().split("\t");
          plot->setWindowLabel(lst[1]);
          plot->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
        }

        if (d_file_version > 83){
          QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
          plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          plot->setSpacing(lst[1].toInt(),lst[2].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
          lst=t.readLine().split("\t", QString::SkipEmptyParts);
          plot->setAlignement(lst[1].toInt(),lst[2].toInt());
        }

        while ( s!="</multiLayer>" ){//open layers
          s=t.readLine();
          if (s.left(7)=="<graph>"){
            lst.clear();
            while ( s!="</graph>" ){
              s=t.readLine();
              lst<<s;
            }
            openGraph(this, plot, lst);
          }
        }
        if(plot)
        { plot->blockSignals(false);
        }
      }else if  (s == "<SurfacePlot>"){//process 3D plots information
        lst.clear();
        while ( s!="</SurfacePlot>" ){
          s=t.readLine();
          lst<<s;
        }
        openSurfacePlot(this,lst);
      }else if  (s == "</folder>"){
        Folder *parent = (Folder *)current_folder->parent();
        if (!parent)
          current_folder = projectFolder();
        else
          current_folder = parent;
      }
    }
    f.close();
  }

  folders->blockSignals (false);
  //change folder to user defined current folder
  changeFolder(cf);
  blockSignals (false);
  renamedTables = QStringList();
  QApplication::restoreOverrideCursor();
  return new_folder;
}

#ifdef QTIPLOT_DEMO
void ApplicationWindow::showDemoVersionMessage()
{
  saved = true;
  /**
	QMessageBox::critical(this, tr("MantidPlot - Demo Version"),//Mantid
			tr("You are using the demonstration version of Qtiplot.\
				It is identical with the full version, except that you can't save your work to project files and you can't use it for more than 10 minutes per session.\
				<br><br>\
				If you want to have ready-to-use, fully functional binaries, please subscribe for a\
				<a href=\"http://soft.proindependent.com/individual_contract.html\">single-user binaries maintenance contract</a>.\
				<br><br>\
				QtiPlot is free software in the sense of free speech.\
				If you know how to use it, you can get\
				<a href=\"http://developer.berlios.de/project/showfiles.php?group_id=6626\">the source code</a>\
				free of charge.\
				Nevertheless, you are welcome to\
				<a href=\"http://soft.proindependent.com/why_donate.html\">make a donation</a>\
				in order to support the further development of QtiPlot."));
   */
}
#endif

void ApplicationWindow::saveFolder(Folder *folder, const QString& fn, bool compress)
{
  QFile f( fn );
  if (d_backup_files && f.exists())
  {// make byte-copy of current file so that there's always a copy of the data on disk
    while (!f.open(QIODevice::ReadOnly)){
      if (f.isOpen())
        f.close();
      int choice = QMessageBox::warning(this, tr("MantidPlot - File backup error"),//Mantid
          tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If you ignore this, you run the risk of <b>data loss</b>.").arg(projectname).arg(projectname+"~"),
          QMessageBox::Retry|QMessageBox::Default, QMessageBox::Abort|QMessageBox::Escape, QMessageBox::Ignore);
      if (choice == QMessageBox::Abort)
        return;
      if (choice == QMessageBox::Ignore)
        break;
    }

    if (f.isOpen()){
      QFile::copy (fn, fn + "~");
      f.close();
    }
  }

  if ( !f.open( QIODevice::WriteOnly ) ){
    QMessageBox::about(this, tr("MantidPlot - File save error"), tr("The file: <br><b>%1</b> is opened in read-only mode").arg(fn));//Mantid
    return;
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> lst = folder->windowsList();
  int windows = 0;
  QString text;
  //save all loaded mantid workspace names to project file
  // call save nexus on each workspace
  QString aux=mantidUI->saveToString(workingDir.toStdString());
  text+=aux;
  //if script window is open save the currently opened script file names to project file
  if (scriptingWindow)
  {	//returns the files names of the all the opened script files.
    QString aux=scriptingWindow->saveToString();
    text+=aux;
  }
  foreach(MdiSubWindow *w, lst){
    QString aux = w->saveToString(windowGeometryInfo(w));
    if (w->inherits("Table"))
      ((Table *)w)->setSpecifications(aux);
    text += aux;
    windows++;
  }
  int initial_depth = folder->depth();
  Folder *dir = folder->folderBelow();
  while (dir && dir->depth() > initial_depth){
    text += "<folder>\t" + QString(dir->objectName()) + "\t" + dir->birthDate() + "\t" + dir->modificationDate();
    if (dir == current_folder)
      text += "\tcurrent\n";
    else
      text += "\n";  // FIXME: Having no 5th string here is not a good idea
    text += "<open>" + QString::number(dir->folderListItem()->isOpen()) + "</open>\n";

    lst = dir->windowsList();
    foreach(MdiSubWindow *w, lst){
      QString aux = w->saveToString(windowGeometryInfo(w));
      if (w->inherits("Table"))
        ((Table *)w)->setSpecifications(aux);
      text += aux;
      windows++;
    }

    if (!dir->logInfo().isEmpty() )
      text += "<log>\n" + dir->logInfo() + "</log>\n" ;

    if ( (dir->children()).isEmpty() )
      text += "</folder>\n";

    int depth = dir->depth();
    dir = dir->folderBelow();
    if (dir){
      int next_dir_depth = dir->depth();
      if (next_dir_depth < depth){
        int diff = depth - next_dir_depth;
        for (int i = 0; i < diff; i++)
          text += "</folder>\n";
      }
    } else {
      int diff = depth - initial_depth - 1;
      for (int i = 0; i < diff; i++)
        text += "</folder>\n";
    }
  }

  text += "<open>" + QString::number(folder->folderListItem()->isOpen()) + "</open>\n";
  if (!folder->logInfo().isEmpty())
    text += "<log>\n" + folder->logInfo() + "</log>" ;

  text.prepend("<windows>\t"+QString::number(windows)+"\n");
  text.prepend("<scripting-lang>\t"+QString(scriptingEnv()->name())+"\n");
  text.prepend("MantidPlot " + QString::number(maj_version)+"."+ QString::number(min_version)+"."+
      QString::number(patch_version)+" project file\n");

  QTextStream t( &f );
  t.setEncoding(QTextStream::UnicodeUTF8);
  t << text;
  f.close();

  if (compress)
  {
    char w9[]="w9";
    file_compress((char *)fn.ascii(), "w9");
  }

  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::saveAsProject()
{
  saveFolderAsProject(current_folder);
}

void ApplicationWindow::saveFolderAsProject(Folder *f)
{
#ifdef QTIPLOT_DEMO
  showDemoVersionMessage();
  return;
#endif
  QString filter = tr("MantidPlot project")+" (*.qti);;";//Mantid
  filter += tr("Compressed MantidPlot project")+" (*.qti.gz)";

  QString selectedFilter;
  QString fn = QFileDialog::getSaveFileName(this, tr("Save project as"), workingDir, filter, &selectedFilter);
  if ( !fn.isEmpty() ){
    QFileInfo fi(fn);
    workingDir = fi.dirPath(true);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fn.append(".qti");

    saveFolder(f, fn, selectedFilter.contains(".gz"));
  }
}

void ApplicationWindow::showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, int)
{
  showFolderPopupMenu(it, p, true);
}

void ApplicationWindow::showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, bool fromFolders)
{
  if (!it || folders->isRenaming())
    return;

  QMenu cm(this);
  QMenu window(this);
  QMenu viewWindowsMenu(this);
  viewWindowsMenu.setCheckable ( true );

  cm.insertItem(tr("&Find..."), this, SLOT(showFindDialogue()));
  cm.insertSeparator();
  cm.insertItem(tr("App&end Project..."), this, SLOT(appendProject()));
  if (((FolderListItem *)it)->folder()->parent())
    cm.insertItem(tr("Save &As Project..."), this, SLOT(saveAsProject()));
  else
    cm.insertItem(tr("Save Project &As..."), this, SLOT(saveProjectAs()));
  cm.insertSeparator();

  if (fromFolders && show_windows_policy != HideAll)
  {
    cm.insertItem(tr("&Show All Windows"), this, SLOT(showAllFolderWindows()));
    cm.insertItem(tr("&Hide All Windows"), this, SLOT(hideAllFolderWindows()));
    cm.insertSeparator();
  }

  if (((FolderListItem *)it)->folder()->parent())
  {
    cm.insertItem(getQPixmap("close_xpm"), tr("&Delete Folder"), this, SLOT(deleteFolder()), Qt::Key_F8);
    cm.insertItem(tr("&Rename"), this, SLOT(startRenameFolder()), Qt::Key_F2);
    cm.insertSeparator();
  }

  if (fromFolders)
  {
    window.addAction(actionNewTable);
    window.addAction(actionNewMatrix);
    window.addAction(actionNewNote);
    window.addAction(actionNewGraph);
    window.addAction(actionNewFunctionPlot);
    window.addAction(actionNewSurfacePlot);
    cm.insertItem(tr("New &Window"), &window);
  }

  cm.insertItem(getQPixmap("newfolder_xpm"), tr("New F&older"), this, SLOT(addFolder()), Qt::Key_F7);
  cm.insertSeparator();

  QStringList lst;
  lst << tr("&None") << tr("&Windows in Active Folder") << tr("Windows in &Active Folder && Subfolders");
  for (int i = 0; i < 3; ++i)
  {
    int id = viewWindowsMenu.insertItem(lst[i],this, SLOT( setShowWindowsPolicy( int ) ) );
    viewWindowsMenu.setItemParameter( id, i );
    viewWindowsMenu.setItemChecked( id, show_windows_policy == i );
  }
  cm.insertItem(tr("&View Windows"), &viewWindowsMenu);
  cm.insertSeparator();
  cm.insertItem(tr("&Properties..."), this, SLOT(folderProperties()));
  cm.exec(p);
}

void ApplicationWindow::setShowWindowsPolicy(int p)
{
  if (show_windows_policy == (ShowWindowsPolicy)p)
    return;

  show_windows_policy = (ShowWindowsPolicy)p;
  if (show_windows_policy == HideAll){
    QList<MdiSubWindow *> windows = windowsList();
    foreach(MdiSubWindow *w, windows){
      hiddenWindows->append(w);
      w->hide();
      setListView(w->objectName(), tr("Hidden"));
    }
  } else
    showAllFolderWindows();
}

void ApplicationWindow::showFindDialogue()
{
  FindDialog *fd = new FindDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  fd->exec();
}

void ApplicationWindow::startRenameFolder()
{
  FolderListItem *fi = current_folder->folderListItem();
  if (!fi)
    return;

  disconnect(folders, SIGNAL(currentChanged(Q3ListViewItem *)), this, SLOT(folderItemChanged(Q3ListViewItem *)));
  fi->setRenameEnabled (0, true);
  fi->startRename (0);
}

void ApplicationWindow::startRenameFolder(Q3ListViewItem *item)
{
  if (!item || item == folders->firstChild())
    return;

  if (item->listView() == lv && item->rtti() == FolderListItem::RTTI) {
    disconnect(folders, SIGNAL(currentChanged(Q3ListViewItem *)), this, SLOT(folderItemChanged(Q3ListViewItem *)));
    current_folder = ((FolderListItem *)item)->folder();
    FolderListItem *it = current_folder->folderListItem();
    it->setRenameEnabled (0, true);
    it->startRename (0);
  } else {
    item->setRenameEnabled (0, true);
    item->startRename (0);
  }
}

void ApplicationWindow::renameFolder(Q3ListViewItem *it, int col, const QString &text)
{
  Q_UNUSED(col)

		    if (!it)
		      return;

  Folder *parent = (Folder *)current_folder->parent();
  if (!parent)//the parent folder is the project folder (it always exists)
    parent = projectFolder();

  while(text.isEmpty())
  {
    QMessageBox::critical(this,tr("MantidPlot - Error"), tr("Please enter a valid name!"));//Mantid
    it->setRenameEnabled (0, true);
    it->startRename (0);
    return;
  }

  QStringList lst = parent->subfolders();
  lst.remove(current_folder->objectName());
  while(lst.contains(text)){
    QMessageBox::critical(this,tr("MantidPlot - Error"),//Mantid
        tr("Name already exists!")+"\n"+tr("Please choose another name!"));

    it->setRenameEnabled (0, true);
    it->startRename (0);
    return;
  }

  current_folder->setObjectName(text);
  it->setRenameEnabled (0, false);
  connect(folders, SIGNAL(currentChanged(Q3ListViewItem *)),
      this, SLOT(folderItemChanged(Q3ListViewItem *)));
  folders->setCurrentItem(parent->folderListItem());//update the list views
}

void ApplicationWindow::showAllFolderWindows()
{
  QList<MdiSubWindow *> lst = current_folder->windowsList();
  foreach(MdiSubWindow *w, lst)
  {//force show all windows in current folder
    if (w)
    {
      updateWindowLists(w);
      switch (w->status())
      {
      case MdiSubWindow::Hidden:
        w->showNormal();
        break;

      case MdiSubWindow::Normal:
        w->showNormal();
        break;

      case MdiSubWindow::Minimized:
        w->showMinimized();
        break;

      case MdiSubWindow::Maximized:
        w->showMaximized();
        break;
      }
    }
  }

  if ( (current_folder->children()).isEmpty() )
    return;

  FolderListItem *fi = current_folder->folderListItem();
  FolderListItem *item = (FolderListItem *)fi->firstChild();
  int initial_depth = item->depth();
  while (item && item->depth() >= initial_depth)
  {// show/hide windows in all subfolders
    lst = ((Folder *)item->folder())->windowsList();
    foreach(MdiSubWindow *w, lst){
      if (w && show_windows_policy == SubFolders){
        updateWindowLists(w);
        switch (w->status())
        {
        case MdiSubWindow::Hidden:
          w->showNormal();
          break;

        case MdiSubWindow::Normal:
          w->showNormal();
          break;

        case MdiSubWindow::Minimized:
          w->showMinimized();
          break;

        case MdiSubWindow::Maximized:
          w->showMaximized();
          break;
        }
      }
      else
        w->hide();
    }

    item = (FolderListItem *)item->itemBelow();
  }
}

void ApplicationWindow::hideAllFolderWindows()
{
  QList<MdiSubWindow *> lst = current_folder->windowsList();
  foreach(MdiSubWindow *w, lst)
  hideWindow(w);

  if ( (current_folder->children()).isEmpty() )
    return;

  if (show_windows_policy == SubFolders)
  {
    FolderListItem *fi = current_folder->folderListItem();
    FolderListItem *item = (FolderListItem *)fi->firstChild();
    int initial_depth = item->depth();
    while (item && item->depth() >= initial_depth)
    {
      lst = item->folder()->windowsList();
      foreach(MdiSubWindow *w, lst)
      hideWindow(w);

      item = (FolderListItem *)item->itemBelow();
    }
  }
}

void ApplicationWindow::projectProperties()
{
  QString s = QString(current_folder->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Project")+"\n\n";
  if (projectname != "untitled")
  {
    s += tr("Path") + ": " + projectname + "\n\n";

    QFileInfo fi(projectname);
    s += tr("Size") + ": " + QString::number(fi.size()) + " " + tr("bytes")+ "\n\n";
  }

  s += tr("Contents") + ": " + QString::number(windowsList().size()) + " " + tr("windows");
  s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
  s += "\n\n\n";

  if (projectname != "untitled")
  {
    QFileInfo fi(projectname);
    s += tr("Created") + ": " + fi.created().toString(Qt::LocalDate) + "\n\n";
    s += tr("Modified") + ": " + fi.lastModified().toString(Qt::LocalDate) + "\n\n";
  }
  else
    s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  //mbox->setIconPixmap(QPixmap( qtiplot_logo_xpm ));
  mbox->show();
}

void ApplicationWindow::folderProperties()
{
  if (!current_folder->parent())
  {
    projectProperties();
    return;
  }

  QString s = QString(current_folder->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Folder")+"\n\n";
  s += tr("Path") + ": " + current_folder->path() + "\n\n";
  s += tr("Size") + ": " + current_folder->sizeToString() + "\n\n";
  s += tr("Contents") + ": " + QString::number(current_folder->windowsList().count()) + " " + tr("windows");
  s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
  //s += "\n\n\n";
  s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";
  //s += tr("Modified") + ": " + current_folder->modificationDate() + "\n\n";

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  mbox->setIconPixmap(getQPixmap("folder_open_xpm"));
  mbox->show();
}

void ApplicationWindow::addFolder()
{
  if (!explorerWindow->isVisible())
    explorerWindow->show();

  QStringList lst = current_folder->subfolders();
  QString name =  tr("New Folder");
  lst = lst.grep( name );
  if (!lst.isEmpty())
    name += " ("+ QString::number(lst.size()+1)+")";

  Folder *f = new Folder(current_folder, name);
  addFolderListViewItem(f);

  FolderListItem *fi = new FolderListItem(current_folder->folderListItem(), f);
  if (fi){
    f->setFolderListItem(fi);
    fi->setRenameEnabled (0, true);
    fi->startRename(0);
  }
}

Folder* ApplicationWindow::addFolder(QString name, Folder* parent)
{
  if(!parent){
    if (current_folder)
      parent = current_folder;
    else
      parent = projectFolder();
  }

  QStringList lst = parent->subfolders();
  lst = lst.grep( name );
  if (!lst.isEmpty())
    name += " ("+ QString::number(lst.size()+1)+")";

  Folder *f = new Folder(parent, name);
  addFolderListViewItem(f);

  FolderListItem *fi = new FolderListItem(parent->folderListItem(), f);
  if (fi)
    f->setFolderListItem(fi);

  return f;
}

bool ApplicationWindow::deleteFolder(Folder *f)
{
  if (!f)
    return false;

  if (confirmCloseFolder && QMessageBox::information(this, tr("MantidPlot - Delete folder?"),//Mantid
      tr("Delete folder '%1' and all the windows it contains?").arg(f->objectName()),
      tr("Yes"), tr("No"), 0, 0))
    return false;
  else {
    Folder *parent = projectFolder();
    if (current_folder){
      if (current_folder->parent())
        parent = (Folder *)current_folder->parent();
    }

    folders->blockSignals(true);

    FolderListItem *fi = f->folderListItem();
    foreach(MdiSubWindow *w, f->windowsList())
    closeWindow(w);

    if (!(f->children()).isEmpty()){
      Folder *subFolder = f->folderBelow();
      int initial_depth = f->depth();
      while (subFolder && subFolder->depth() > initial_depth){
        foreach(MdiSubWindow *w, subFolder->windowsList()){
          removeWindowFromLists(w);
          subFolder->removeWindow(w);
          delete w;
        }
        delete subFolder->folderListItem();
        delete subFolder;

        subFolder = f->folderBelow();
      }
    }

    delete f;
    delete fi;

    current_folder = parent;
    folders->setCurrentItem(parent->folderListItem());
    changeFolder(parent, true);
    folders->blockSignals(false);
    folders->setFocus();
    return true;
  }
}

void ApplicationWindow::deleteFolder()
{
  Folder *parent = (Folder *)current_folder->parent();
  if (!parent)
    parent = projectFolder();

  folders->blockSignals(true);

  if (deleteFolder(current_folder)){
    current_folder = parent;
    folders->setCurrentItem(parent->folderListItem());
    changeFolder(parent, true);
  }

  folders->blockSignals(false);
  folders->setFocus();
}

void ApplicationWindow::folderItemDoubleClicked(Q3ListViewItem *it)
{
  if (!it || it->rtti() != FolderListItem::RTTI)
    return;

  FolderListItem *item = ((FolderListItem *)it)->folder()->folderListItem();
  folders->setCurrentItem(item);
}

void ApplicationWindow::folderItemChanged(Q3ListViewItem *it)
{
  if (!it)
    return;

  it->setOpen(true);
  changeFolder (((FolderListItem *)it)->folder());
  folders->setFocus();
}

void ApplicationWindow::hideFolderWindows(Folder *f)
{
  QList<MdiSubWindow *> lst = f->windowsList();
  foreach(MdiSubWindow *w, lst)
  w->hide();

  if ((f->children()).isEmpty())
    return;

  Folder *dir = f->folderBelow();
  int initial_depth = f->depth();
  while (dir && dir->depth() > initial_depth){
    lst = dir->windowsList();
    foreach(MdiSubWindow *w, lst)
    w->hide();

    dir = dir->folderBelow();
  }
}

bool ApplicationWindow::changeFolder(Folder *newFolder, bool force)
{
  if (!newFolder)
    return false;

  if (current_folder == newFolder && !force)
    return false;

  desactivateFolders();
  newFolder->folderListItem()->setActive(true);

  Folder *oldFolder = current_folder;
  MdiSubWindow::Status old_active_window_state = MdiSubWindow::Normal;
  MdiSubWindow *old_active_window = oldFolder->activeWindow();
  if (old_active_window)
    old_active_window_state = old_active_window->status();

  MdiSubWindow::Status active_window_state = MdiSubWindow::Normal;
  MdiSubWindow *active_window = newFolder->activeWindow();

  if (active_window)
    active_window_state = active_window->status();

  hideFolderWindows(oldFolder);
  current_folder = newFolder;

  results->setText(current_folder->logInfo());

  lv->clear();

  QObjectList folderLst = newFolder->children();
  if(!folderLst.isEmpty()){
    foreach(QObject *f, folderLst)
			    addFolderListViewItem(static_cast<Folder *>(f));
  }

  QList<MdiSubWindow *> lst = newFolder->windowsList();
  foreach(MdiSubWindow *w, lst){
    if (!hiddenWindows->contains(w) && show_windows_policy != HideAll){
      //show only windows in the current folder which are not hidden by the user
      if(w->status() == MdiSubWindow::Normal)
        w->showNormal();
      else if(w->status() == MdiSubWindow::Minimized)
        w->showMinimized();
      else if(w->status() == MdiSubWindow::Maximized)
        w->showMaximized();
    } else
      w->setStatus(MdiSubWindow::Hidden);

    addListViewItem(w);
  }

  if (!(newFolder->children()).isEmpty()){
    Folder *f = newFolder->folderBelow();
    int initial_depth = newFolder->depth();
    while (f && f->depth() > initial_depth){//show/hide windows in subfolders
      lst = f->windowsList();
      foreach(MdiSubWindow *w, lst){
        if (!hiddenWindows->contains(w)){
          if (show_windows_policy == SubFolders){
            if (w->status() == MdiSubWindow::Normal || w->status() == MdiSubWindow::Maximized)
              w->showNormal();
            else if (w->status() == MdiSubWindow::Minimized)
              w->showMinimized();
          } else
            w->hide();
        }
      }
      f = f->folderBelow();
    }
  }

  if (active_window){
    d_active_window = active_window;
    d_workspace->setActiveSubWindow(active_window);
    customMenu(active_window);
    customToolBars(active_window);
    if (active_window_state == MdiSubWindow::Minimized)
      active_window->showMinimized();//ws->setActiveWindow() makes minimized windows to be shown normally
    else if (active_window_state == MdiSubWindow::Maximized){
      if (active_window->isA("Graph3D"))
        ((Graph3D *)active_window)->setIgnoreFonts(true);
      active_window->showMaximized();
      if (active_window->isA("Graph3D"))
        ((Graph3D *)active_window)->setIgnoreFonts(false);
    }
  }

  if (old_active_window){
    old_active_window->setStatus(old_active_window_state);
    oldFolder->setActiveWindow(old_active_window);
  }

  if (d_opening_file)
    modifiedProject();
  return true;
}

void ApplicationWindow::desactivateFolders()
{
  FolderListItem *item = (FolderListItem *)folders->firstChild();
  while (item){
    item->setActive(false);
    item = (FolderListItem *)item->itemBelow();
  }
}

void ApplicationWindow::addListViewItem(MdiSubWindow *w)
{
  if (!w)
    return;

  WindowListItem* it = new WindowListItem(lv, w);
  if (w->isA("Matrix")){
    it->setPixmap(0, getQPixmap("matrix_xpm"));
    it->setText(1, tr("Matrix"));
  }
  else if (w->inherits("Table")){
    it->setPixmap(0, getQPixmap("worksheet_xpm"));
    it->setText(1, tr("Table"));
  }
  else if (w->isA("Note")){
    it->setPixmap(0, getQPixmap("note_xpm"));
    it->setText(1, tr("Note"));
  }
  else if (w->isA("MultiLayer")){
    it->setPixmap(0, getQPixmap("graph_xpm"));
    it->setText(1, tr("Graph"));
  }
  else if (w->isA("Graph3D")){
    it->setPixmap(0, getQPixmap("trajectory_xpm"));
    it->setText(1, tr("3D Graph"));
  }
  else if (w->isA("MantidMatrix")){
    it->setPixmap(0, getQPixmap("mantid_matrix_xpm"));
    it->setText(1, tr("Workspace"));
  }
  else if (w->isA("InstrumentWindow")){
    it->setText(1, tr("Instrument"));
  }


  it->setText(0, w->objectName());
  it->setText(2, w->aspect());
  it->setText(3, w->sizeToString());
  it->setText(4, w->birthDate());
  it->setText(5, w->windowLabel());
}

void ApplicationWindow::windowProperties()
{
  WindowListItem *it = (WindowListItem *)lv->currentItem();
  MdiSubWindow *w = it->window();
  if (!w)
    return;

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), QString(), QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  QString s = QString(w->objectName()) + "\n\n";
  s += "\n\n\n";

  s += tr("Label") + ": " + ((MdiSubWindow *)w)->windowLabel() + "\n\n";

  if (w->isA("Matrix")){
    mbox->setIconPixmap(getQPixmap("matrix_xpm"));
    s +=  tr("Type") + ": " + tr("Matrix") + "\n\n";
  }else if (w->inherits("Table")){
    mbox->setIconPixmap(getQPixmap("worksheet_xpm"));
    s +=  tr("Type") + ": " + tr("Table") + "\n\n";
  }else if (w->isA("Note")){
    mbox->setIconPixmap(getQPixmap("note_xpm"));
    s +=  tr("Type") + ": " + tr("Note") + "\n\n";
  }else if (w->isA("MultiLayer")){
    mbox->setIconPixmap(getQPixmap("graph_xpm"));
    s +=  tr("Type") + ": " + tr("Graph") + "\n\n";
  }else if (w->isA("Graph3D")){
    mbox->setIconPixmap(getQPixmap("trajectory_xpm"));
    s +=  tr("Type") + ": " + tr("3D Graph") + "\n\n";
  }
  s += tr("Path") + ": " + current_folder->path() + "\n\n";
  s += tr("Size") + ": " + w->sizeToString() + "\n\n";
  s += tr("Created") + ": " + w->birthDate() + "\n\n";
  s += tr("Status") + ": " + it->text(2) + "\n\n";
  mbox->setText(s);
  mbox->show();
}

void ApplicationWindow::addFolderListViewItem(Folder *f)
{
  if (!f)
    return;

  FolderListItem* it = new FolderListItem(lv, f);
  it->setActive(false);
  it->setText(0, f->objectName());
  it->setText(1, tr("Folder"));
  it->setText(3, f->sizeToString());
  it->setText(4, f->birthDate());
}

void ApplicationWindow::find(const QString& s, bool windowNames, bool labels,
    bool folderNames, bool caseSensitive, bool partialMatch, bool subfolders)
{
  if (windowNames || labels){
    MdiSubWindow *w = current_folder->findWindow(s, windowNames, labels, caseSensitive, partialMatch);
    if (w){
      activateWindow(w);
      return;
    }

    if (subfolders){
      FolderListItem *item = (FolderListItem *)folders->currentItem()->firstChild();
      while (item){
        Folder *f = item->folder();
        MdiSubWindow *w = f->findWindow(s,windowNames,labels,caseSensitive,partialMatch);
        if (w){
          folders->setCurrentItem(f->folderListItem());
          activateWindow(w);
          return;
        }
        item = (FolderListItem *)item->itemBelow();
      }
    }
  }

  if (folderNames){
    Folder *f = current_folder->findSubfolder(s, caseSensitive, partialMatch);
    if (f){
      folders->setCurrentItem(f->folderListItem());
      return;
    }

    if (subfolders){
      FolderListItem *item = (FolderListItem *)folders->currentItem()->firstChild();
      while (item){
        Folder *f = item->folder()->findSubfolder(s, caseSensitive, partialMatch);
        if (f){
          folders->setCurrentItem(f->folderListItem());
          return;
        }

        item = (FolderListItem *)item->itemBelow();
      }
    }
  }

  QMessageBox::warning(this, tr("MantidPlot - No match found"),//Mantid
      tr("Sorry, no match found for string: '%1'").arg(s));
}

void ApplicationWindow::dropFolderItems(Q3ListViewItem *dest)
{
  if (!dest || draggedItems.isEmpty ())
    return;

  Folder *dest_f = ((FolderListItem *)dest)->folder();

  Q3ListViewItem *it;
  QStringList subfolders = dest_f->subfolders();

  foreach(it, draggedItems){
    if (it->rtti() == FolderListItem::RTTI){
      Folder *f = ((FolderListItem *)it)->folder();
      FolderListItem *src = f->folderListItem();
      if (dest_f == f){
        QMessageBox::critical(this, "MantidPlot - Error", tr("Cannot move an object to itself!"));//Mantid
        return;
      }

      if (((FolderListItem *)dest)->isChildOf(src)){
        QMessageBox::critical(this,"MantidPlot - Error",tr("Cannot move a parent folder into a child folder!"));//Mantid
        draggedItems.clear();
        folders->setCurrentItem(current_folder->folderListItem());
        return;
      }

      Folder *parent = (Folder *)f->parent();
      if (!parent)
        parent = projectFolder();
      if (dest_f == parent)
        return;

      if (subfolders.contains(f->objectName())){
        QMessageBox::critical(this, tr("MantidPlot") +" - " + tr("Skipped moving folder"),//Mantid
            tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(f->objectName()));
      } else
        moveFolder(src, (FolderListItem *)dest);
    } else {
      if (dest_f == current_folder)
        return;

      MdiSubWindow *w = ((WindowListItem *)it)->window();
      if (w){
        current_folder->removeWindow(w);
        w->hide();
        dest_f->addWindow(w);
        delete it;
      }
    }
  }

  draggedItems.clear();
  current_folder = dest_f;
  folders->setCurrentItem(dest_f->folderListItem());
  changeFolder(dest_f, true);
  folders->setFocus();
}

void ApplicationWindow::moveFolder(FolderListItem *src, FolderListItem *dest)
{
  folders->blockSignals(true);
  if (copyFolder(src->folder(), dest->folder())){
    delete src->folder();
    delete src;
  }
  folders->blockSignals(false);
}

bool ApplicationWindow::copyFolder(Folder *src, Folder *dest)
{
  if (!src || !dest)
    return false;

  if (dest->subfolders().contains(src->objectName())){
    QMessageBox::critical(this, tr("MantidPlot") + " - " + tr("Error"),//Mantid
        tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(src->objectName()));
    return false;
  }

  Folder *dest_f = new Folder(dest, src->objectName());
  dest_f->setBirthDate(src->birthDate());
  dest_f->setModificationDate(src->modificationDate());

  FolderListItem *copy_item = new FolderListItem(dest->folderListItem(), dest_f);
  copy_item->setText(0, src->objectName());
  copy_item->setOpen(src->folderListItem()->isOpen());
  dest_f->setFolderListItem(copy_item);

  QList<MdiSubWindow *> lst = QList<MdiSubWindow *>(src->windowsList());
  foreach(MdiSubWindow *w, lst)
  dest_f->addWindow(w);

  if (!(src->children()).isEmpty()){
    int initial_depth = src->depth();
    Folder *parentFolder = dest_f;
    src = src->folderBelow();
    while (src && parentFolder && src->depth() > initial_depth){
      dest_f = new Folder(parentFolder, src->objectName());
      dest_f->setBirthDate(src->birthDate());
      dest_f->setModificationDate(src->modificationDate());

      copy_item = new FolderListItem(parentFolder->folderListItem(), dest_f);
      copy_item->setText(0, src->objectName());
      copy_item->setOpen(src->folderListItem()->isOpen());
      dest_f->setFolderListItem(copy_item);

      lst = QList<MdiSubWindow *>(src->windowsList());
      foreach(MdiSubWindow *w, lst)
      dest_f->addWindow(w);

      int depth = src->depth();
      src = src->folderBelow();
      if (src){
        int next_folder_depth = src->depth();
        if (next_folder_depth > depth)
          parentFolder = dest_f;
        else if (next_folder_depth < depth && next_folder_depth > initial_depth)
          parentFolder = (Folder*)parentFolder->parent();
      }
    }
  }
  return true;
}
//Mantid commented out
//void ApplicationWindow::searchForUpdates()
//{
//    int choice = QMessageBox::question(this, tr("QtiPlot"),
//					tr("QtiPlot will try to download necessary information about the last available updates. Please modify your firewall settings in order to allow QtiPlot to connect to the internet!") + "\n" +
//					tr("Do you wish to continue?"),
//					QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape);
//
//    if (choice == QMessageBox::Yes){
//        version_buffer.open(IO_WriteOnly);
//        http.setHost("soft.proindependent.com");
//        http.get("/version.txt", &version_buffer);
//        http.closeConnection();
//    }
//}
//
//void ApplicationWindow::receivedVersionFile(bool error)
//{
//	if (error){
//		QMessageBox::warning(this, tr("QtiPlot - HTTP get version file"),
//				tr("Error while fetching version file with HTTP: %1.").arg(http.errorString()));
//		return;
//	}
//
//	version_buffer.close();
//
//	if (version_buffer.open(IO_ReadOnly))
//	{
//		QTextStream t( &version_buffer );
//		t.setEncoding(QTextStream::UnicodeUTF8);
//		QString version = t.readLine();
//		version_buffer.close();
//
//		QString currentVersion = QString::number(maj_version) + "." + QString::number(min_version) +
//			"." + QString::number(patch_version) + QString(extra_version);
//
//		if (currentVersion != version)
//		{
//			if(QMessageBox::question(this, tr("QtiPlot - Updates Available"),
//						tr("There is a newer version of QtiPlot (%1) available for download. Would you like to download it?").arg(version),
//						QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape) == QMessageBox::Yes)
//				QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/download.html"));
//		}
//		else if (!autoSearchUpdatesRequest)
//		{
//			QMessageBox::information(this, tr("QtiPlot - No Updates Available"),
//					tr("No updates available. Your current version %1 is the last version available!").arg(version));
//		}
//		autoSearchUpdatesRequest = false;
//	}
//}

/**
  Turns 3D animation on or off
 */
void ApplicationWindow::toggle3DAnimation(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->animate(on);
}

QString ApplicationWindow::generateUniqueName(const QString& name, bool increment)
{
  int index = 0;
  QStringList lst;
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      lst << QString(w->objectName());
      if (QString(w->objectName()).startsWith(name))
        index++;
    }
    f = f->folderBelow();
  }

  QString newName = name;
  if (increment)//force return of a different name
    newName += QString::number(++index);
  else if (index>0)
    newName += QString::number(index);

  while(lst.contains(newName))
    newName = name + QString::number(++index);

  return newName;
}

void ApplicationWindow::clearTable()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;

  if (QMessageBox::question(this, tr("MantidPlot - Warning"),//Mantid
      tr("This will clear the contents of all the data associated with the table. Are you sure?"),
      tr("&Yes"), tr("&No"), QString(), 0, 1 ) )
    return;
  else
    t->clear();
}

void ApplicationWindow::goToRow()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table") || w->isA("Matrix")){
    bool ok;
    int row = QInputDialog::getInteger(this, tr("MantidPlot - Enter row number"), tr("Row"),//Mantid
        1, 0, 1000000, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint );
    if ( !ok )
      return;

    if (w->inherits("Table"))
      ((Table *)w)->goToRow(row);
    else if (w->isA("Matrix"))
      ((Matrix *)w)->goToRow(row);
  }
}

void ApplicationWindow::goToColumn()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table") || w->isA("Matrix")){
    bool ok;
    int col = QInputDialog::getInteger(this, tr("MantidPlot - Enter column number"), tr("Column"),//Mantid
        1, 0, 1000000, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint );
    if ( !ok )
      return;

    if (w->inherits("Table"))
      ((Table *)w)->goToColumn(col);
    else if (w->isA("Matrix"))
      ((Matrix *)w)->goToColumn(col);
  }
}

void ApplicationWindow::showScriptWindow()
{
  if (!scriptingWindow)
  { 
    // MG 09/02/2010 : Removed parent from scripting window. If it has one then it doesn't respect the always on top 
    // flag, it is treated as a sub window of its parent
    scriptingWindow = new ScriptingWindow(scriptingEnv(),NULL);
    connect(scriptingWindow, SIGNAL(chooseScriptingLanguage()), this, SLOT(showScriptingLangDialog()));
    scriptingWindow->resize(d_script_win_rect.size());
    scriptingWindow->move(d_script_win_rect.topLeft());
  }

  if (!scriptingWindow->isVisible())
  {
    Qt::WindowFlags flags = Qt::Window;
    if (d_script_win_on_top)
    {
      flags |= Qt::WindowStaysOnTopHint;
    }
    scriptingWindow->setWindowFlags(flags);
    scriptingWindow->show();
    scriptingWindow->setFocus();
  } 
  else
  {
    scriptingWindow->hide();
  }
}

void ApplicationWindow::showScriptInterpreter()
{
  if( !m_interpreterDock )
  {
    m_interpreterDock = new QDockWidget(this);
    m_interpreterDock->setObjectName("interpreterDock");
    m_interpreterDock->setWindowTitle("Script Interpreter");
    addDockWidget( Qt::BottomDockWidgetArea, m_interpreterDock );

    m_scriptInterpreter = new ScriptManagerWidget(scriptingEnv(), m_interpreterDock,true);
    m_interpreterDock->setWidget(m_scriptInterpreter);
    m_interpreterDock->setFocusPolicy(Qt::StrongFocus);
    m_interpreterDock->setFocusProxy(m_scriptInterpreter);
  }
  if( m_interpreterDock->isVisible() )
  {
    m_interpreterDock->hide();
  }
  else
  { 
    m_interpreterDock->show();
    m_scriptInterpreter->setFocus();
  }

}

/**
  Turns perspective mode on or off
 */
void ApplicationWindow::togglePerspective(bool on)
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setOrthogonal(!on);
}

/**
  Resets rotation of 3D plots to default values
 */
void ApplicationWindow::resetRotation()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->setRotation(30, 0, 15);
}

/**
  Finds best layout for the 3D plot
 */
void ApplicationWindow::fitFrameToLayer()
{
  Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
  if (!g)
    return;

  g->findBestLayout();
}

ApplicationWindow::~ApplicationWindow()
{
  if (lastCopiedLayer)
    delete lastCopiedLayer;

  delete hiddenWindows;

  if (scriptingWindow)
  {
    delete scriptingWindow;
  }

  if (d_text_editor)
    delete d_text_editor;

  QApplication::clipboard()->clear(QClipboard::Clipboard);

  btnPointer->setChecked(true);

  //Mantid
  if (mantidUI) delete mantidUI;
}

QString ApplicationWindow::versionString()
{
  QString version(MANTIDPLOT_RELEASE_VERSION);
  QString date(MANTIDPLOT_RELEASE_DATE);
  return "This is MantidPlot version " + version + " of " + date;
}

int ApplicationWindow::convertOldToNewColorIndex(int cindex)
{
  if( (cindex == 13) || (cindex == 14) ) // white and light gray
    return cindex + 4;

  if(cindex == 15) // dark gray
    return cindex + 8;

  return cindex;
}

void ApplicationWindow::cascade()
{
  const int xoffset = 13;
  const int yoffset = 20;
  int x = 0;
  int y = 0;
  QList<QMdiSubWindow*> windows = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach (QMdiSubWindow *w, windows){
    w->setActiveWindow();
    ((MdiSubWindow *)w)->setNormal();
    w->setGeometry(x, y, w->geometry().width(), w->geometry().height());
    w->raise();
    x += xoffset;
    y += yoffset;
  }
  modifiedProject();
}

ApplicationWindow * ApplicationWindow::loadScript(const QString& fn, bool execute, bool quit)
{
#ifdef SCRIPTING_PYTHON
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setScriptingLanguage("Python");
  restoreApplicationGeometry();
  showScriptWindow();
  scriptingWindow->open(fn, false);
  QApplication::restoreOverrideCursor();
  if (execute)
    scriptingWindow->executeAll();
  if( quit )
  {
    saved = true;
    this->close();
  }
  return this;
#else
  QMessageBox::critical(this, tr("MantidPlot") + " - " + tr("Error"),//Mantid
      tr("MantidPlot was not built with Python scripting support included!"));//Mantid
#endif
  return 0;
}

bool ApplicationWindow::validFor2DPlot(Table *table)
{
  if (!table->selectedYColumns().count()){
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select a Y column to plot!"));//Mantid
    return false;
  } else if (table->numCols()<2) {
    QMessageBox::critical(this, tr("MantidPlot - Error"),tr("You need at least two columns for this operation!"));//Mantid
    return false;
  } else if (table->noXColumn()) {
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please set a default X column for this table, first!"));//Mantid
    return false;
  }
  return true;
}

MultiLayer* ApplicationWindow::generate2DGraph(Graph::CurveType type)
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return 0;

  if (w->inherits("Table")){
    Table *table = static_cast<Table *>(w);
    if (!validFor2DPlot(table))
      return 0;

    Q3TableSelection sel = table->getSelection();
    return multilayerPlot(table, table->drawableColumnSelection(), type, sel.topRow(), sel.bottomRow());
  } else if (w->isA("Matrix")){
    Matrix *m = static_cast<Matrix *>(w);
    return plotHistogram(m);
  }
  return 0;
}

bool ApplicationWindow::validFor3DPlot(Table *table)
{
  if (table->numCols()<2){
    QMessageBox::critical(0,tr("MantidPlot - Error"),tr("You need at least two columns for this operation!"));//Mantid
    return false;
  }
  if (table->selectedColumn() < 0 || table->colPlotDesignation(table->selectedColumn()) != Table::Z){
    QMessageBox::critical(0,tr("MantidPlot - Error"),tr("Please select a Z column for this operation!"));//Mantid
    return false;
  }
  if (table->noXColumn()){
    QMessageBox::critical(0,tr("MantidPlot - Error"),tr("You need to define a X column first!"));//Mantid
    return false;
  }
  if (table->noYColumn()){
    QMessageBox::critical(0,tr("MantidPlot - Error"),tr("You need to define a Y column first!"));//Mantid
    return false;
  }
  return true;
}

void ApplicationWindow::hideSelectedWindows()
{
  Q3ListViewItem *item;
  QList<Q3ListViewItem *> lst;
  for (item = lv->firstChild(); item; item = item->nextSibling()){
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach(item, lst){
    if (item->rtti() != FolderListItem::RTTI)
      hideWindow(((WindowListItem *)item)->window());
  }
  folders->blockSignals(false);
}

void ApplicationWindow::showSelectedWindows()
{
  Q3ListViewItem *item;
  QList<Q3ListViewItem *> lst;
  for (item = lv->firstChild(); item; item = item->nextSibling()){
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach(item, lst){
    if (item->rtti() != FolderListItem::RTTI)
      activateWindow(((WindowListItem *)item)->window());
  }
  folders->blockSignals(false);
}

void ApplicationWindow::swapColumns()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (!t)
    return;
  QStringList lst = t->selectedColumns();
  if(lst.count() != 2)
    return;

  t->swapColumns(t->colIndex(lst[0]), t->colIndex(lst[1]));
}

void ApplicationWindow::moveColumnRight()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->moveColumnBy(1);
}

void ApplicationWindow::moveColumnLeft()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->moveColumnBy(-1);
}

void ApplicationWindow::moveColumnFirst()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->moveColumnBy(0 - t->selectedColumn());
}

void ApplicationWindow::moveColumnLast()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->moveColumnBy(t->numCols() - t->selectedColumn() - 1);
}

void ApplicationWindow::restoreApplicationGeometry()
{
  if (d_app_rect.isNull())
    showMaximized();
  else {
    resize(d_app_rect.size());
    move(d_app_rect.topLeft());
    show();
  }
}

void ApplicationWindow::scriptsDirPathChanged(const QString& path)
{
  scriptsDirPath = path;

  QList<MdiSubWindow*> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Note"))
      ((Note*)w)->setDirPath(path);
  }
}

void ApplicationWindow::showToolBarsMenu()
{
  QMenu toolBarsMenu;

  QAction *actionFileTools = new QAction(fileTools->windowTitle(), this);
  actionFileTools->setCheckable(true);
  actionFileTools->setChecked(fileTools->isVisible());
  connect(actionFileTools, SIGNAL(toggled(bool)), fileTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionFileTools);

  QAction *actionEditTools = new QAction(editTools->windowTitle(), this);
  actionEditTools->setCheckable(true);
  actionEditTools->setChecked(editTools->isVisible());
  connect(actionEditTools, SIGNAL(toggled(bool)), editTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionEditTools);

  QAction *actionTableTools = new QAction(tableTools->windowTitle(), this);
  actionTableTools->setCheckable(true);
  actionTableTools->setChecked(tableTools->isVisible());
  connect(actionTableTools, SIGNAL(toggled(bool)), tableTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionTableTools);

  QAction *actionColumnTools = new QAction(columnTools->windowTitle(), this);
  actionColumnTools->setCheckable(true);
  actionColumnTools->setChecked(columnTools->isVisible());
  connect(actionColumnTools, SIGNAL(toggled(bool)), columnTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionColumnTools);

  QAction *actionPlotTools = new QAction(plotTools->windowTitle(), this);
  actionPlotTools->setCheckable(true);
  actionPlotTools->setChecked(plotTools->isVisible());
  connect(actionPlotTools, SIGNAL(toggled(bool)), plotTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionPlotTools);

  QAction *actionMatrixTools = new QAction(plotMatrixBar->windowTitle(), this);
  actionMatrixTools->setCheckable(true);
  actionMatrixTools->setChecked(plotMatrixBar->isVisible());
  connect(actionMatrixTools, SIGNAL(toggled(bool)), plotMatrixBar, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionMatrixTools);

  QAction *actionPlot3DTools = new QAction(plot3DTools->windowTitle(), this);
  actionPlot3DTools->setCheckable(true);
  actionPlot3DTools->setChecked(plot3DTools->isVisible());
  connect(actionPlot3DTools, SIGNAL(toggled(bool)), plot3DTools, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionPlot3DTools);

  QAction *actionDisplayBar = new QAction(displayBar->windowTitle(), this);
  actionDisplayBar->setCheckable(true);
  actionDisplayBar->setChecked(displayBar->isVisible());
  connect(actionDisplayBar, SIGNAL(toggled(bool)), displayBar, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionDisplayBar);

  QAction *actionFormatToolBar = new QAction(formatToolBar->windowTitle(), this);
  actionFormatToolBar->setCheckable(true);
  actionFormatToolBar->setChecked(formatToolBar->isVisible());
  connect(actionFormatToolBar, SIGNAL(toggled(bool)), formatToolBar, SLOT(setVisible(bool)));
  toolBarsMenu.addAction(actionFormatToolBar);

  QAction *action = toolBarsMenu.exec(QCursor::pos());
  if (!action)
    return;

  MdiSubWindow *w = activeWindow();

  if (action->text() == plotMatrixBar->windowTitle()){
    d_matrix_tool_bar = action->isChecked();
    plotMatrixBar->setEnabled(w && w->isA("Matrix"));
  } else if (action->text() == tableTools->windowTitle()){
    d_table_tool_bar = action->isChecked();
    tableTools->setEnabled(w && w->inherits("Table"));
  } else if (action->text() == columnTools->windowTitle()){
    d_column_tool_bar = action->isChecked();
    columnTools->setEnabled(w && w->inherits("Table"));
  } else if (action->text() == plotTools->windowTitle()){
    d_plot_tool_bar = action->isChecked();
    plotTools->setEnabled(w && w->isA("MultiLayer"));
  } else if (action->text() == plot3DTools->windowTitle()){
    d_plot3D_tool_bar = action->isChecked();
    plot3DTools->setEnabled(w && w->isA("Graph3D"));
  } else if (action->text() == fileTools->windowTitle()){
    d_file_tool_bar = action->isChecked();
  } else if (action->text() == editTools->windowTitle()){
    d_edit_tool_bar = action->isChecked();
  } else if (action->text() == displayBar->windowTitle()){
    d_display_tool_bar = action->isChecked();
  } else if (action->text() == formatToolBar->windowTitle()){
    d_format_tool_bar = action->isChecked();
  }
}

void ApplicationWindow::saveFitFunctions(const QStringList& lst)
{
  if (!lst.count())
    return;

  QString explain = tr("Starting with version 0.9.1 MantidPlot stores the user defined fit models to a different location.");
  explain += " " + tr("If you want to save your already defined models, please choose a destination folder.");
  if (QMessageBox::Ok != QMessageBox::information(this, tr("MantidPlot") + " - " + tr("Import fit models"), explain,//Mantid
      QMessageBox::Ok, QMessageBox::Cancel)) return;

  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to export the fit models to"), fitModelsPath, QFileDialog::ShowDirsOnly);
  if (!dir.isEmpty()){
    fitModelsPath = dir;

    for (int i = 0; i<lst.count(); i++){
      QString s = lst[i].simplified();
      if (!s.isEmpty()){
        NonLinearFit *fit = new NonLinearFit(this, 0);

        int pos1 = s.find("(", 0);
        fit->setObjectName(s.left(pos1));

        int pos2 = s.find(")", pos1);
        QString par = s.mid(pos1+4, pos2-pos1-4);
        QStringList paramList = par.split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts);
        fit->setParametersList(paramList);

        QStringList l = s.split("=");
        if (l.count() == 2)
          fit->setFormula(l[1]);

        fit->save(fitModelsPath + "/" + fit->objectName() + ".fit");
      }
    }
  }
}

void ApplicationWindow::matrixDirectFFT()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->fft();
}

void ApplicationWindow::matrixInverseFFT()
{
  Matrix* m = (Matrix*)activeWindow(MatrixWindow);
  if (!m)
    return;

  m->fft(true);
}

void ApplicationWindow::setFormatBarFont(const QFont& font)
{
  formatToolBar->setEnabled(true);

  QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  fb->blockSignals(true);
  fb->setCurrentFont(font);
  fb->blockSignals(false);

  QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
  sb->blockSignals(true);
  sb->setValue(font.pointSize());
  sb->blockSignals(false);

  actionFontBold->blockSignals(true);
  actionFontBold->setChecked(font.bold());
  actionFontBold->blockSignals(false);

  actionFontItalic->blockSignals(true);
  actionFontItalic->setChecked(font.italic());
  actionFontItalic->blockSignals(false);

  actionSubscript->setEnabled(false);
  actionSuperscript->setEnabled(false);
  actionUnderline->setEnabled(false);
  actionGreekSymbol->setEnabled(false);
  actionGreekMajSymbol->setEnabled(false);
  actionMathSymbol->setEnabled(false);
}

void ApplicationWindow::setFontSize(int size)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  QFont f(fb->currentFont().family(), size);
  f.setBold(actionFontBold->isChecked());
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::setFontFamily(const QFont& font)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
  QFont f(font.family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::setItalicFont(bool italic)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
  QFont f(fb->currentFont().family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(italic);
  g->setCurrentFont(f);
}

void ApplicationWindow::setBoldFont(bool bold)
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
  QFont f(fb->currentFont().family(), sb->value());
  f.setBold(bold);
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::enableTextEditor(Graph *g)
{
  if (!g){
    formatToolBar->setEnabled(false);
    if (d_text_editor){
      d_text_editor->close();
      d_text_editor = NULL;
    }
  } else if (g) {
    d_text_editor = new TextEditor(g);

    formatToolBar->setEnabled(true);
    actionSubscript->setEnabled(true);
    actionSuperscript->setEnabled(true);
    actionUnderline->setEnabled(true);
    actionGreekSymbol->setEnabled(true);
    actionGreekMajSymbol->setEnabled(true);
    actionMathSymbol->setEnabled(true);
  }
}

void ApplicationWindow::insertSuperscript()
{
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<sup>","</sup>");
}

void ApplicationWindow::insertSubscript()
{
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<sub>","</sub>");
}

void ApplicationWindow::underline()
{
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<u>","</u>");
}

void ApplicationWindow::insertGreekSymbol()
{
  if (!d_text_editor)
    return;

  SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::lowerGreek, this);
  connect(greekLetters, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
  greekLetters->exec();
}

void ApplicationWindow::insertGreekMajSymbol()
{
  if (!d_text_editor)
    return;

  SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::upperGreek, this);
  connect(greekLetters, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
  greekLetters->exec();
}

void ApplicationWindow::insertMathSymbol()
{
  if (!d_text_editor)
    return;

  SymbolDialog *ms = new SymbolDialog(SymbolDialog::mathSymbols, this);
  connect(ms, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
  ms->exec();
}

void ApplicationWindow::showCustomActionDialog()
{
  ManageCustomMenus *ad = new ManageCustomMenus(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void ApplicationWindow::showUserDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

void ApplicationWindow::addCustomAction(QAction *action, const QString& parentName, int index)
{
  if (!action)
    return;

  QList<QToolBar *> toolBars = toolBarsList();
  foreach (QToolBar *t, toolBars){
    if (t->objectName() == parentName){
      t->addAction(action);
      if (index < 0)
        d_user_actions << action;
      else if (index >= 0 && index < d_user_actions.size())
        d_user_actions.replace(index, action);
      return;
    }
  }

  QList<QMenu *> menus = customizableMenusList();
  foreach (QMenu *m, menus){
    if (m->objectName() == parentName){
      m->addAction(action);
      if (index < 0)
        d_user_actions << action;
      else if (index >= 0 && index < d_user_actions.size())
        d_user_actions.replace(index, action);
      return;
    }
  }
}

void ApplicationWindow::reloadCustomActions()
{
  QList<QMenu *> menus = customizableMenusList();
  foreach(QAction *a, d_user_actions){
    if (!a->statusTip().isEmpty()){
      foreach (QMenu *m, menus){
        if (m->objectName() == a->statusTip()){
          QList<QAction *> lst = m->actions();
          if (!lst.contains(a))
            m->addAction(a);
        }
      }
    }
  }
}

void ApplicationWindow::removeCustomAction(QAction *action)
{
  int index = d_user_actions.indexOf(action);
  if (index >= 0 && index < d_user_actions.count()){
    d_user_actions.removeAt(index);
    delete action;
  }
}

void ApplicationWindow::performCustomAction(QAction *action)
{
  if (!action || !d_user_actions.contains(action))
    return;
#ifdef SCRIPTING_PYTHON
QString action_data = action->data().toString();
if( QFileInfo(action_data).exists() )
{
  QFile script_file(action_data);
  if ( !script_file.open(IO_ReadOnly) )
  {
    QMessageBox::information(this, "MantidPlot", "Error: There was a problem reading\n" + action_data);
    return;
  }
  
  QTextStream stream(&script_file);
  QString scriptPath = QString("r'%1'").arg(QFileInfo(action_data).absolutePath());
  QString code = QString("sys.path.append(%1)\n").arg(scriptPath);
    runPythonScript(code, true);
  code = "";
  while( !stream.atEnd() )
  {
    code.append(stream.readLine() + "\n");
  }
  runPythonScript(code);
  code = "";
  code.append(QString("\nsys.path.remove(%1)").arg(scriptPath));
    runPythonScript(code, true);
}
else
{
  //First search for an existing window
  foreach( QMdiSubWindow* sub_win, d_workspace->subWindowList() )
  {
    if( sub_win->widget()->objectName() == action_data )
    {
      sub_win->widget()->show();
      return;
    }
  }

  QMdiSubWindow* usr_win = new QMdiSubWindow;
  usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
  MantidQt::API::UserSubWindow *user_interface = MantidQt::API::InterfaceManager::Instance().createSubWindow(action_data, usr_win);
  if(user_interface)
  {
    setGeometry(usr_win,user_interface);
    connect(user_interface, SIGNAL(runAsPythonScript(const QString&)), this,
        SLOT(runPythonScript(const QString&)));
    user_interface->initializeLocalPython();
  }
  else
  {
    delete usr_win;
  }

}
#else
QMessageBox::critical(this, tr("MantidPlot") + " - " + tr("Error"),//Mantid
    tr("MantidPlot was not built with Python scripting support included!"));
#endif
}

void ApplicationWindow::runPythonScript(const QString & code, bool quiet)
{
  if( code.isEmpty() || scriptingEnv()->isRunning() ) return;

  if( m_iface_script == NULL )
  {
    setScriptingLanguage("Python");
    m_iface_script = scriptingEnv()->newScript("",this,false, "");
    m_iface_script->setLineOffset(0);
    connect(m_iface_script, SIGNAL(print(const QString &)), this, SLOT(scriptPrint(const QString&)));
    connect(m_iface_script, SIGNAL(error(const QString &, const QString&, int)), this, 
        SLOT(scriptPrint(const QString &)));

  }

  m_iface_script->setCode(code);
  if( !quiet )
  {
    // Output a message to say we've started
    scriptPrint("Script execution started.", false, true);
  }
  bool success = m_iface_script->exec();
  if(success && !quiet)
  {
    scriptPrint("Script execution completed successfully.", false, true);
  }
    
}

void ApplicationWindow::loadCustomActions()
{
  QString path = customActionsDirPath + "/";
  QDir dir(path);
  QStringList lst = dir.entryList(QDir::Files|QDir::NoSymLinks, QDir::Name);
  for (int i=0; i<lst.count(); i++){
    QString fileName = path + lst[i];
    QFile file(fileName);
    QFileInfo fi(file);
    if (!file.open(QFile::ReadOnly | QFile::Text))
      continue;

    QAction *action = new QAction(this);
    CustomActionHandler handler(action);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    QXmlInputSource xmlInputSource(&file);
    if (reader.parse(xmlInputSource))
      addCustomAction(action, handler.parentName());
  }
}

QList<QMenu *> ApplicationWindow::customizableMenusList()
{
  QList<QMenu *> lst;
  lst << 	windowsMenu << view << graph << fileMenu << format << edit;
  lst << help << plot2DMenu << analysisMenu << multiPeakMenu;
  lst << matrixMenu << plot3DMenu << plotDataMenu ;// scriptingMenu;
  lst << tableMenu << fillMenu << normMenu << newMenu << exportPlotMenu << smoothMenu;
  lst << filterMenu << decayMenu;
  return lst;
}

//-------------------------------
// Mantid
void ApplicationWindow::addUserMenu(const QString & topMenu)
{
  if( topMenu.isEmpty() ) return;

  foreach(QMenu* menu, d_user_menus)
  {
    if( menu->title() == topMenu ) return;
  }

  QMenu* customMenu = new QMenu(topMenu);
  customMenu->setTitle(topMenu);
  customMenu->setName(topMenu);
  connect(customMenu, SIGNAL(triggered(QAction*)), this, SLOT(performCustomAction(QAction*)));
  d_user_menus.append(customMenu);
  menuBar()->insertItem(tr(topMenu), customMenu);
}

void ApplicationWindow::addUserMenuAction(const QString & parentMenu, const QString & itemName, const QString & itemData)
{
  QMenu* topMenu(NULL);
  foreach(topMenu, d_user_menus)
  {
    if( topMenu->title() == parentMenu ) break;
  } 

  if( !topMenu ) return;
  foreach(QAction* userAction, topMenu->actions())
  {
    if( userAction->text() == itemName ) return;
  }

  QAction* scriptAction = new QAction(tr(itemName), topMenu);
  scriptAction->setData(itemData); 
  topMenu->addAction(scriptAction);
  d_user_actions.append(scriptAction);

  // Remove name from the list of removed interfaces if applicable
  removed_interfaces.remove(itemName);
}

void ApplicationWindow::removeUserMenu(const QString & parentMenu)
{
  int i(0);
  QMenu *menu = NULL;
  foreach(menu, d_user_menus)
  {
    if( menu->title() == parentMenu ) break;
    ++i;
  }
  if( !menu ) return; 

  d_user_menus.removeAt(i);
  menuBar()->removeAction(menu->menuAction());
}

void ApplicationWindow::removeUserMenuAction(const QString & parentMenu, const QString & userAction)
{
  QMenu *menu = NULL;
  foreach(menu, d_user_menus)
  {
    if( menu->title() == parentMenu ) break;
  }
  if( !menu ) return;

  QAction *action = NULL;
  int menu_count(0);
  foreach(action, d_user_actions)
  {
    if( action->text() == userAction ) break;
    ++menu_count;
  }
  if( !action ) return;

  d_user_actions.removeAt(menu_count);
  menu->removeAction(action);

  // Add interface name to the list of removed interfaces
  removed_interfaces.append(userAction);
}

const QList<QMenu*> & ApplicationWindow::getCustomMenus() const
{
  return d_user_menus;
} 

QList<QMenu *> ApplicationWindow::menusList()
{
  QList<QMenu *> lst;
  QObjectList children = this->children();
  foreach (QObject *w, children){
    if (w->isA("QMenu"))
      lst << (QMenu *)w;
  }
  return lst;
}

// End of a section of Mantid custom functions
//-------------------------------------------

QList<QToolBar *> ApplicationWindow::toolBarsList()
{
  QList<QToolBar *> lst;
  QObjectList children = this->children();
  foreach (QObject *w, children){
    if (w->isA("QToolBar"))
      lst << (QToolBar *)w;
  }
  return lst;
}

void ApplicationWindow::hideSelectedColumns()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->hideSelectedColumns();
}

void ApplicationWindow::showAllColumns()
{
  Table *t = (Table *)activeWindow(TableWindow);
  if (t)
    t->showAllColumns();
}

void ApplicationWindow::setMatrixUndoStackSize(int size)
{
  if (d_matrix_undo_stack_size == size)
    return;

  d_matrix_undo_stack_size = size;
  Folder *f = projectFolder();
  while (f){
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows){
      if (w->isA("Matrix")){
        QUndoStack *stack = ((Matrix *)w)->undoStack();
        if (!stack->count())// undo limit can only be changed for empty stacks
          stack->setUndoLimit(size);
      }
    }
    f = f->folderBelow();
  }
}

//! This is a dirty hack: sometimes the workspace area and the windows are not redrawn properly
// after a MultiLayer plot window is resized by the user: Qt bug?
void ApplicationWindow::repaintWindows()
{
  if (d_opening_file || (d_active_window && d_active_window->status() == MdiSubWindow::Maximized))
    return;

  QWidget *viewPort = d_workspace->viewport();
  QSize size = viewPort->size();
  viewPort->resize(QSize(size.width() + 1, size.height() + 1));
  viewPort->resize(size);
}

QString ApplicationWindow::endOfLine()
{
  switch(d_eol){
  case LF:
    return "\n";
    break;
  case CRLF:
    return "\r\n";
    break;
  case CR:
    return "\r";
    break;
  }
  return "\n";
}

/**  Switch on the right tool buttons associated with a MultiLayer window
 *   @param w :: The active MultiLayer window.
 */
void ApplicationWindow::customMultilayerToolButtons(MultiLayer* w)
{
  if (!w)
  {
    btnPointer->setOn(true);
    return;
  }

  Graph* g = w->activeGraph();
  if (g)
  {
    PlotToolInterface* tool = g->activeTool();
    if (g->zoomOn())
      btnZoomIn->setOn(true);

    else if (g->areRangeSelectorsOn())
      btnSelect->setOn(true);
    //else if (dynamic_cast<PeakPickerTool1D*>(tool))
    //  btnPeakPick->setOn(true);
    else if (dynamic_cast<PeakPickerTool*>(tool))
      btnMultiPeakPick->setOn(true);
    else if (dynamic_cast<DataPickerTool*>(tool))
    {
      switch(((DataPickerTool*)tool)->getMode())
      {
      case DataPickerTool::Move:
        btnMovePoints->setOn(true);
        break;
      case DataPickerTool::Remove:
        btnRemovePoints->setOn(true);
        break;
      case DataPickerTool::Display:
        btnCursor->setOn(true);
        break;
      default:
        btnPointer->setOn(true);
      }
    }
    else if (dynamic_cast<DrawPointTool*>(tool))
      actionDrawPoints->setOn(true);
    else if (dynamic_cast<ScreenPickerTool*>(tool))
      btnPicker->setOn(true);
    else
      btnPointer->setOn(true);
  }
  else
    btnPointer->setOn(true);
}
/**  save workspace data in nexus format
 *   @param wsName :: name of the ouput file.
 *   @param fileName :: name of the ouput file.
 */
void ApplicationWindow::savedatainNexusFormat(const std::string& wsName,const std::string& fileName)
{		
  //std::string s=workingDir.toStdString()+wsName+".nxs";
  // std::string fileName(s);
  if(fileName.empty()) return ;
  try
  {
    if(mantidUI)mantidUI->savedatainNexusFormat(fileName,wsName);
  }
  catch(...)
  {
  }
}
void ApplicationWindow::enablesaveNexus(const QString &wsName)
{
  if(actionSaveFile) actionSaveFile->setEnabled(true);
  m_nexusInputWSName=wsName;
}
/* For zooming the selected graph using the drag canvas tool and mouse drag.
 */
void ApplicationWindow::magnify()
{
  MultiLayer *plot = (MultiLayer *)activeWindow(MultiLayerWindow);
  if (!plot)
    return;

  if (plot->isEmpty()){
    QMessageBox::warning(this, tr("QtiPlot - Warning"),
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setOn(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach(Graph *g, layers)
  g->enablePanningMagnifier();
}
/// Handler for ICat Login Menu
void ApplicationWindow::ICatLogin()
{
  mantidUI->executeAlgorithm("CatalogLogin",1);
}

void ApplicationWindow::ICatIsisSearch()
{	
  QMdiSubWindow* usr_win = new QMdiSubWindow(this);
  usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
  QWidget* icatsearch_interface = new MantidQt::MantidWidgets::ICatSearch(usr_win);
  if(icatsearch_interface)
  {
    setGeometry(usr_win,icatsearch_interface);
  }
  else
  {
    delete usr_win;
  }
}
void ApplicationWindow::ICatMyDataSearch()
{	
  QMdiSubWindow* usr_win = new QMdiSubWindow(this);
  usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
  QWidget* mydatsearch = new MantidQt::MantidWidgets::ICatMyDataSearch(usr_win);
  if(mydatsearch)
  {
    setGeometry(usr_win,mydatsearch);
  }
  else
  {
    delete usr_win;
  }
}
void ApplicationWindow ::ICatAdvancedSearch()
{
  QMdiSubWindow* usr_win = new QMdiSubWindow(this);
  usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
  QWidget* advanced_search = new MantidQt::MantidWidgets::ICatAdvancedSearch(usr_win);
  if(advanced_search)
  {
    setGeometry(usr_win,advanced_search);
  }
  else
  {
    delete usr_win;
  }
}
void ApplicationWindow::setGeometry(QMdiSubWindow* usr_win,QWidget* user_interface)
{   
  QRect frame = QRect(usr_win->frameGeometry().topLeft() - usr_win->geometry().topLeft(),
      usr_win->geometry().bottomRight() - usr_win->geometry().bottomRight());
  usr_win->setWidget(user_interface);
  QRect iface_geom = QRect(frame.topLeft() + user_interface->geometry().topLeft(),
      frame.bottomRight() + user_interface->geometry().bottomRight());
  usr_win->setGeometry(iface_geom);
  d_workspace->addSubWindow(usr_win);
  usr_win->show();



}
void ApplicationWindow::ICatLogout()
{
  mantidUI->executeICatLogout(-1);
}

/**
 * Write a message to the log window
 * @param message :: A string containing the message
 * @param error :: A boolean indicating if this is an error
 */
void ApplicationWindow::writeToLogWindow(const QString& message,bool error)
{		
  if(error)
  {
    results->setTextColor(Qt::red);
  }
  else
  {
    results->setTextColor(Qt::black);
  }
  QTextCursor cursor = results->textCursor();
  cursor.movePosition(QTextCursor::End);
  results->setTextCursor(cursor);
  results->insertPlainText(message + "\n");
  cursor = results->textCursor();
  cursor.movePosition(QTextCursor::End);
}

  /**
  * Write an error message to the log window (convenience slot)
  * @param message :: The string to send the log window
  */
  void ApplicationWindow::writeErrorToLogWindow(const QString& message)
  {
    writeToLogWindow(message, true);
  }

/* This method executes loadraw asynchrnously
 * @param  fileName - name of the file to load
 * @param wsName :: -name of the workspace to store data
 */
void ApplicationWindow::executeLoadRawAsynch(const QString& fileName,const QString& wsName )
{
  mantidUI->loadrawfromICatInterface(fileName,wsName);
}

/* This method executes loadnexus asynchrnously
 * @param  fileName - name of the file to load
 * @param wsName :: -name of the workspace to store data
 */
void ApplicationWindow::executeLoadNexusAsynch(const QString& fileName,const QString& wsName)
{
  mantidUI->loadnexusfromICatInterface(fileName,wsName);
}

/* This method executes Download data files algorithm
 * @param  filenames - list of the file names to download
 */
void ApplicationWindow::executeDownloadDataFiles(const std::vector<std::string>& filenNames,const std::vector<long long>& fileIds)
{
  //getting the sender of the signal(it's ICatInvestigation object)
  QObject* qsender= sender();
  if(!qsender) return;

  // connecting  filelocations signal to ICatInvestigation slot setfileLocations
  // This is to send the filelocations vector  after  algorithm execution to ICatInvestigation object(which is MnatidQt) for further processing
  connect(mantidUI,SIGNAL(fileLocations(const std::vector<std::string>&)),qsender,SLOT(setfileLocations(const std::vector<std::string>&)));
  /// execute the algorithm
  mantidUI->executeDownloadDataFiles(filenNames,fileIds);
}

void  ApplicationWindow::executeloadAlgorithm(const QString& algName,const QString& fileName, const QString& wsName)
{
  mantidUI->executeloadAlgorithm(algName,fileName,wsName);
}

