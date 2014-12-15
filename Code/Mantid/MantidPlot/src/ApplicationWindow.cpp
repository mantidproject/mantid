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
#include "ScriptFileInterpreter.h"
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
#include "FloatingWindow.h"
#include "DataPickerTool.h"
#include "TiledWindow.h"
#include "DockedWindow.h"
#include "TSVSerialiser.h"

// TODO: move tool-specific code to an extension manager
#include "ScreenPickerTool.h"
#include "LabelTool.h"
#include "TranslateCurveTool.h"
#include "MultiPeakFitTool.h"
#include "LineProfileTool.h"
#include "RangeSelectorTool.h"
#include "PlotToolInterface.h"
#include "Mantid/IProjectSerialisable.h"
#include "Mantid/MantidMatrix.h"
#include "Mantid/MantidTable.h"
#include "Mantid/MantidMatrixCurve.h"
#include "ContourLinesEditor.h"
#include "Mantid/InstrumentWidget/InstrumentWindow.h"
#include "Mantid/RemoveErrorsDialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <cassert>

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
#include <QFontComboBox>
#include <QSpinBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSignalMapper>
#include <QDesktopWidget>
#include <QPair>
#include <QtAlgorithms>
#include <zlib.h>

#include <gsl/gsl_sort.h>

#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>

//Mantid
#include "ScriptingWindow.h"

#include "Mantid/MantidUI.h"
#include "Mantid/MantidAbout.h"
#include "Mantid/PeakPickerTool.h"
#include "Mantid/ManageCustomMenus.h"
#include "Mantid/ManageInterfaceCategories.h"
#include "Mantid/FirstTimeSetup.h"
#include "Mantid/SetUpParaview.h"

#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtAPI/Message.h"

#include "MantidQtMantidWidgets/CatalogHelper.h"
#include "MantidQtMantidWidgets/CatalogSearch.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MessageDisplay.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtAPI/ScriptRepositoryView.h"

using namespace Qwt3D;
using namespace MantidQt::API;

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("ApplicationWindow");
  /// ParaView plugins key
  const char * PVPLUGINS_DIR_KEY = "pvplugins.directory";
}

extern "C"
{
void file_compress(const char  *file, const char  *mode);
void file_uncompress(const char  *file);
}

ApplicationWindow::ApplicationWindow(bool factorySettings)
: QMainWindow(),
Scripted(ScriptingLangManager::newEnv(this)),
blockWindowActivation(false),
m_enableQtiPlotFitting(false),
m_exitCode(0),
#ifdef Q_OS_MAC // Mac
  settings(QSettings::IniFormat, QSettings::UserScope, "Mantid", "MantidPlot")
#else
  settings("Mantid", "MantidPlot")
#endif
{
  QStringList empty;
  init(factorySettings, empty);
}

ApplicationWindow::ApplicationWindow(bool factorySettings, const QStringList& args)
: QMainWindow(),
Scripted(ScriptingLangManager::newEnv(this)),
blockWindowActivation(false),
m_enableQtiPlotFitting(false),
m_exitCode(0),
#ifdef Q_OS_MAC // Mac
  settings(QSettings::IniFormat, QSettings::UserScope, "Mantid", "MantidPlot")
#else
  settings("Mantid", "MantidPlot")
#endif
{
  init(factorySettings, args);
}

/**
 * This function is responsible for copying the old configuration
 * information from the ISIS\MantidPlot area to the new Mantid\MantidPlot
 * area. The old area is deleted once the trnasfer is complete. On subsequent
 * runs, if the old configuration area is missing or empty, the copying
 * is ignored.
 */
void ApplicationWindow::handleConfigDir()
{
#ifdef Q_OS_WIN
  // We use the registry for settings on Windows
  QSettings oldSettings("ISIS", "MantidPlot");
  QStringList keys = oldSettings.allKeys();
  // If the keys are empty, we removed the MantidPlot entries
  if (!keys.empty())
  {
    foreach (QString key, keys)
    {
	    settings.setValue(key, oldSettings.value(key));
    }
    // This unfortunately cannot remove the top-level entry
    oldSettings.remove("");
  }
#else
  QFileInfo curConfig(settings.fileName());
  QString oldPath = settings.fileName();
  oldPath.replace("Mantid", "ISIS");
  QFileInfo oldConfig(oldPath);

  // If the old config directory exists, copy it's contents and
  // then delete it
  QDir oldConfigDir = oldConfig.dir();
  if (oldConfigDir.exists())
  {
    QStringList entries = oldConfigDir.entryList();
    foreach ( QString entry, entries )
    {
      if (!entry.startsWith("."))
      {
        QFileInfo oldFile(oldConfig.dir(), entry);
        QFileInfo newFile(curConfig.dir(), entry);
        // Qt will not overwrite files, so remove new one first
        QFile::remove(newFile.filePath());
        QFile::copy(oldFile.filePath(), newFile.filePath());
        QFile::remove(oldFile.filePath());
      }
    }
    oldConfigDir.rmdir(oldConfig.path());
  }
#endif
}

/**
 * Calls QCoreApplication::exit(m_exitCode)
 */
void ApplicationWindow::exitWithPresetCode()
{
  QCoreApplication::exit(m_exitCode);
  handleConfigDir();
}

void ApplicationWindow::init(bool factorySettings, const QStringList& args)
{
  QCoreApplication::setOrganizationName("Mantid");
  QCoreApplication::setApplicationName("MantidPlot");
  setAttribute(Qt::WA_DeleteOnClose);

  #ifdef SHARED_MENUBAR
  m_sharedMenuBar = new QMenuBar(NULL);
  m_sharedMenuBar->setNativeMenuBar(true);
#endif
  setWindowTitle(tr("MantidPlot - untitled"));//Mantid
  setObjectName("main application");
  initGlobalConstants();
  QPixmapCache::setCacheLimit(20*QPixmapCache::cacheLimit ());

  // Logging as early as possible
  logWindow = new QDockWidget(this);
  logWindow->hide();
  logWindow->setObjectName("logWindow"); // this is needed for QMainWindow::restoreState()
  logWindow->setWindowTitle(tr("Results Log"));
  addDockWidget( Qt::TopDockWidgetArea, logWindow );

  using MantidQt::MantidWidgets::MessageDisplay;
  using MantidQt::API::Message;
  qRegisterMetaType<Message>("Message"); // Required to use it in signals-slots
  resultsLog = new MessageDisplay(MessageDisplay::EnableLogLevelControl, logWindow);
  logWindow->setWidget(resultsLog);
  connect(resultsLog, SIGNAL(errorReceived(const QString &)), logWindow, SLOT(show()));


  // Start Mantid
  // Set the Paraview path BEFORE libaries are loaded. Doing it here prevents
  // the logs being poluted with library loading errors.
  trySetParaviewPath(args);

  using Mantid::Kernel::ConfigService;
  auto & config = ConfigService::Instance(); // Starts logging
  resultsLog->attachLoggingChannel(); // Must be done after logging starts
  using Mantid::API::FrameworkManager;
  auto & framework = FrameworkManager::Instance(); // Loads framework libraries
  // Load Paraview plugin libraries if possible
  if(config.quickParaViewCheck())
  {
    // load paraview plugins
    framework.loadPluginsUsingKey(PVPLUGINS_DIR_KEY);
  }

  // Create UI object
  mantidUI = new MantidUI(this);

  // Everything else...
  tablesDepend = new QMenu(this);
  explorerWindow = new QDockWidget( this );
  explorerWindow->setWindowTitle(tr("Project Explorer"));
  explorerWindow->setObjectName("explorerWindow"); // this is needed for QMainWindow::restoreState()
  explorerWindow->setMinimumHeight(150);
  addDockWidget( Qt::BottomDockWidgetArea, explorerWindow );

  actionSaveFile=NULL;
  actionSaveProject=NULL;
  actionSaveProjectAs=NULL;
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

  d_current_folder = new Folder( 0, tr("untitled"));
  FolderListItem *fli = new FolderListItem(folders, d_current_folder);
  d_current_folder->setFolderListItem(fli);
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

  // Needs to be done after initialization of dock windows,
  // because we now use QDockWidget::toggleViewAction()
  createActions();
  initToolBars();
  initMainMenu();
  makeToolbarsMenu();

  d_workspace = new QMdiArea();
  d_workspace->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
  d_workspace->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  d_workspace->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCentralWidget(d_workspace);

  setAcceptDrops(true);

  hiddenWindows = new QList<QWidget*>();

  scriptingWindow = NULL;
  d_text_editor = NULL;

  const QString scriptsDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("mantidqt.python_interfaces_directory"));

  // Parse the list of registered PyQt interfaces and their respective categories.
  QString pyQtInterfacesProperty = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("mantidqt.python_interfaces"));
  foreach(const QString pyQtInterfaceInfo, QStringList::split(" ", pyQtInterfacesProperty))
  {
    QString pyQtInterfaceFile;
    QSet<QString> pyQtInterfaceCategories;
    const QStringList tokens = QStringList::split("/", pyQtInterfaceInfo);

    if( tokens.size() == 0 ) // Empty token - ignore.
    {
      continue;
    }
    else if( tokens.size() == 1 ) // Assume missing category.
    {
      pyQtInterfaceCategories += "Uncatagorised";
      pyQtInterfaceFile = tokens[0];
    }
    else if( tokens.size() == 2 ) // Assume correct interface name and categories.
    {
      pyQtInterfaceCategories += QStringList::split(";", tokens[0]).toSet();
      pyQtInterfaceFile = tokens[1];
    }
    else // Too many forward slashes, or no space between two interfaces.  Warn user and move on.
    {
      g_log.warning() << "The mantidqt.python_interfaces property contains an unparsable value: "
                      << pyQtInterfaceInfo.toStdString();
      continue;
    }

    const QString scriptPath = scriptsDir + '/' + pyQtInterfaceFile;

    if( QFileInfo(scriptPath).exists() )
    {
      const QString pyQtInterfaceName = QFileInfo(scriptPath).baseName().replace("_", " ");
      m_interfaceNameDataPairs.append(qMakePair(pyQtInterfaceName, scriptPath));

      // Keep track of the interface's categories as we go.
      m_interfaceCategories[pyQtInterfaceName] = pyQtInterfaceCategories;
      m_allCategories += pyQtInterfaceCategories;
    }
    else
    {
      g_log.warning() << "Could not find interface script: " << scriptPath.ascii() << "\n";
    }
  }

  MantidQt::API::InterfaceManager interfaceManager;
  // Add all interfaces inherited from UserSubWindow.
  foreach(const QString userSubWindowName, interfaceManager.getUserSubWindowKeys())
  {
    m_interfaceNameDataPairs.append(qMakePair(userSubWindowName, userSubWindowName));

    const QSet<QString> categories = UserSubWindowFactory::Instance().getInterfaceCategories(userSubWindowName);

    m_interfaceCategories[userSubWindowName] = categories;
    m_allCategories += categories;
  }

  renamedTables = QStringList();
  if (!factorySettings)
    readSettings();

  createLanguagesList();
  insertTranslatedStrings();
  disableToolbars();
  displayToolbars();
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
      this, SLOT(activateWindow(Q3ListViewItem *)));
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

  connect(recentProjectsMenu, SIGNAL(activated(int)), this, SLOT(openRecentProject(int)));
  connect(recentFilesMenu, SIGNAL(activated(int)), this, SLOT(openRecentFile(int)));

  //apply user settings
  updateAppFonts();
  setAppColors(workspaceColor, panelsColor, panelsTextColor, true);

  //Scripting
  m_script_envs = QHash<QString, ScriptingEnv*>();
  setScriptingLanguage(defaultScriptingLang);
  m_iface_script = NULL;

  m_interpreterDock = new QDockWidget(this);
  m_interpreterDock->setObjectName("interpreterDock"); // this is needed for QMainWindow::restoreState()
  m_interpreterDock->setWindowTitle("Script Interpreter");
  runPythonScript("from ipython_widget import *\nw = _qti.app._getInterpreterDock()\nw.setWidget(MantidIPythonWidget())",false,true,true);
  if ( ! restoreDockWidget(m_interpreterDock))
  {
    // Restoring the widget fails if the settings aren't found or read. Therefore, add it manually.
    addDockWidget( Qt::BottomDockWidgetArea, m_interpreterDock );
  }

  loadCustomActions();

  // Nullify catalogSearch
  catalogSearch = NULL;

  // Print a warning message if the scripting language is set to muParser
  if (defaultScriptingLang == "muParser")
  {
    logWindow->show();
    g_log.warning("The scripting language is set to muParser. This is probably not what you want! Change the default in View->Preferences.");
  }

  // Need to show first time setup dialog?
  if (shouldWeShowFirstTimeSetup(args))
  {
    showFirstTimeSetup();
  }
 
  using namespace Mantid::API;
  // Do this as late as possible to avoid unnecessary updates
  AlgorithmFactory::Instance().enableNotifications();
  AlgorithmFactory::Instance().notificationCenter.postNotification(new AlgorithmFactoryUpdateNotification);

  /*
  The scripting enironment call setScriptingLanguage is trampling over the PATH, so we have to set it again.
  Here we do not off the setup dialog.
  */
  const bool skipDialog = true;
  trySetParaviewPath(args, skipDialog);
}

/** Determines if the first time dialog should be shown
* @param commandArguments : all command line arguments.
* @returns true if the dialog should be shown
*/
bool ApplicationWindow::shouldWeShowFirstTimeSetup(const QStringList& commandArguments)
{
	//Early check of execute and quit command arguments used by system tests.
	QString str;
	foreach(str, commandArguments)
	{
		if((this->shouldExecuteAndQuit(str)) || 
      (this->isSilentStartup(str)))
		{
		return false;
		}
	}

  //first check the facility and instrument
  using Mantid::Kernel::ConfigService;
  auto & config = ConfigService::Instance(); 
  std::string facility = config.getString("default.facility");
  std::string instrument = config.getString("default.instrument");
  if ( facility.empty() || instrument.empty() )
  {
    return true;
  }
  else
  {
    //check we can get the facility and instrument
    try
    {
      const Mantid::Kernel::FacilityInfo& facilityInfo = config.getFacility(facility);
      const Mantid::Kernel::InstrumentInfo& instrumentInfo = config.getInstrument(instrument);
      g_log.information()<<"Default facility '" << facilityInfo.name() 
        << "', instrument '" << instrumentInfo.name() << "'" << std::endl;
    }
    catch (Mantid::Kernel::Exception::NotFoundError&)
    {
      //failed to find the facility or instrument
      g_log.error()<<"Could not find your default facility '" << facility 
        <<"' or instrument '" << instrument << "' in facilities.xml, showing please select again." << std::endl;
      return true;
    }
  }

  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  const bool doNotShowUntilNextRelease = settings.value("DoNotShowUntilNextRelease", 0).toInt();
  const QString lastVersion = settings.value("LastVersion", "").toString();
  settings.endGroup();

  if (!doNotShowUntilNextRelease)
  {
    return true;
  }

  //Now check if the version has changed since last time
  const QString version = QString::fromStdString(Mantid::Kernel::MantidVersion::releaseNotes());
  if (version != lastVersion)
  {
    return true;
  }

  return false;
}

/*
Function tries to set the paraview path.

This is a windows only feature. the PATH enviromental variable can be set at runtime on windows.

- Abort if Vates libraries do not seem to be present.
- Othwerise, if the paraview.path is already in the properties file, use it.
- Otherwise, if the user is not using executeandquit command arguments launch the Setup gui.

@param commandArguments : all command line arguments.
@param noDialog : set to true to skip over the a dialog launch.
*/
void ApplicationWindow::trySetParaviewPath(const QStringList& commandArguments, bool noDialog)
{
#ifdef _WIN32

    Mantid::Kernel::ConfigServiceImpl& confService = Mantid::Kernel::ConfigService::Instance();
    //Early check of execute and quit command arguments used by system tests.
    QString str;
    bool b_skipDialog = noDialog;
    foreach(str, commandArguments)
    {
      if ((this->shouldExecuteAndQuit(str)) ||
        (this->isSilentStartup(str)))
      {
        b_skipDialog = true;
        break;
      }
    }

    //ONLY If skipping is not already selected
    if(!b_skipDialog)
    {
      //If the ignore property exists and is set to true, then skip the dialog.
      const std::string paraviewIgnoreProperty = "paraview.ignore";
      b_skipDialog = confService.hasProperty(paraviewIgnoreProperty) && QString::fromStdString(confService.getString(paraviewIgnoreProperty)).toInt() > 0;
    }

    if(this->hasParaviewPath())
    {
      //Already have a path in the properties file, just apply it.
      std::string path = confService.getString("paraview.path");
      confService.setParaviewLibraryPath(path);
    }
    else
    {
      //Only run the following if skipping is not implied.
      if(!b_skipDialog)
      {
        //Launch the dialog to set the PV path.
        SetUpParaview pv(SetUpParaview::FirstLaunch);
        pv.exec();
      }
    }

#else
  UNUSED_ARG(commandArguments)
  UNUSED_ARG(noDialog)
#endif
}


/*
Getter to determine if the paraview path has been set.
*/
bool ApplicationWindow::hasParaviewPath() const
{
  const std::string propertyname = "paraview.path";
  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
  return config.hasProperty(propertyname) && !config.getString(propertyname).empty() ;
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
  d_standard_tool_bar = true;
  d_column_tool_bar = true;
  d_edit_tool_bar = true;
  d_plot_tool_bar = true;
  d_display_tool_bar = false;
  d_format_tool_bar = true;

  appStyle = qApp->style()->objectName();
  d_app_rect = QRect();
  projectname = "untitled";
  lastCopiedLayer = NULL;
  d_text_copy = NULL;
  d_arrow_copy = NULL;
  d_image_copy = NULL;

  savingTimerId = 0;

  autoSearchUpdatesRequest = false;

  show_windows_policy = ActiveFolder;
  d_init_window_type = NoWindow;

  QString aux = qApp->applicationDirPath();
  workingDir = aux;

  d_translations_folder = aux + "/translations";
  helpFilePath = aux + "/manual/index.html";
  d_python_config_folder = aux;

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

  d_graph_tick_labels_dist = 4;
  d_graph_axes_labels_dist = 2;

  autoSave = false;
  autoSaveTime = 15;
  d_backup_files = true;
  defaultScriptingLang = "Python";  //Mantid M. Gigg
  // Scripting window geometry
  d_script_win_pos = QPoint(250,200);
  d_script_win_size = QSize(600,660);
  d_thousands_sep = true;
  d_locale = QLocale::system().name();
  if (!d_thousands_sep)
    d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

  d_decimal_digits = 13;
  d_graphing_digits = 3;

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
  d_inform_delete_workspace = true;
  d_inform_rename_table = false;
  confirmCloseInstrWindow=false;

  d_show_table_comments = false;

  titleOn = true;
  // 'Factory' default is to show top & right axes but without labels
  d_show_axes = QVector<bool> (QwtPlot::axisCnt, true);
  d_show_axes_labels = QVector<bool> (QwtPlot::axisCnt, true);
  d_show_axes_labels[1] = false;
  d_show_axes_labels[3] = false;
  autoDistribution1D = true;
  canvasFrameWidth = 0;
  defaultPlotMargin = 0;
  drawBackbones = true;

  //these settings are overridden, but the default axes scales are linear
  d_axes_scales = QVector<QString> (QwtPlot::axisCnt, "linear");

  axesLineWidth = 1;
  autoscale2DPlots = true;
  autoScaleFonts = true;
  autoResizeLayers = true;
  antialiasing2DPlots = true;
  fixedAspectRatio2DPlots = false; //Mantid
  d_scale_plots_on_print = false;
  d_print_cropmarks = false;
  d_synchronize_graph_scales = true;

  defaultCurveStyle = static_cast<int>(Graph::Line);
  defaultCurveLineWidth = 1;
  defaultSymbolSize = 7;

  majTicksStyle = static_cast<int>(ScaleDraw::In);
  minTicksStyle = static_cast<int>(ScaleDraw::In);
  minTicksLength = 5;
  majTicksLength = 9;

  legendFrameStyle = static_cast<int>(LegendWidget::Line);
  legendTextColor = Qt::black;
  legendBackground = Qt::white;
  legendBackground.setAlpha(255); // opaque by default;

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
  d_ASCII_import_mode = static_cast<int>(ImportASCIIDialog::NewTables);
  d_ASCII_comment_string = "#";
  d_ASCII_import_comments = false;
  d_ASCII_import_read_only = false;
  d_ASCII_import_preview = true;
  d_preview_lines = 100;

#ifdef Q_OS_MAC
  d_eol = CR;
#else
#ifdef _WIN32
  d_eol = CRLF;
#else
  d_eol = LF;
#endif
#endif

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
  d_export_vector_size = static_cast<int>(QPrinter::Custom);
  d_keep_plot_aspect = true;
}

QMenuBar* ApplicationWindow::myMenuBar()
{
#ifdef SHARED_MENUBAR
  return m_sharedMenuBar == NULL ? menuBar() : m_sharedMenuBar;
#else
  return menuBar();
#endif
}

void ApplicationWindow::initToolBars()
{
  initPlot3DToolBar();

  //	setWindowIcon(QIcon(getQPixmap("logo_xpm")));
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  QPixmap openIcon, saveIcon;

  standardTools = new QToolBar(tr( "Standard Tools" ), this);
  standardTools->setObjectName("standardTools"); // this is needed for QMainWindow::restoreState()
  standardTools->setIconSize( QSize(18,20) );
  addToolBar( Qt::TopToolBarArea, standardTools );

  standardTools->addAction(actionLoadFile);
  standardTools->addSeparator ();
  standardTools->addAction(actionNewProject);
  standardTools->addAction(actionOpenProj);
  standardTools->addAction(actionSaveProject);
  standardTools->addSeparator ();

  standardTools->addAction(actionShowLog);
#ifdef SCRIPTING_PYTHON
  standardTools->addAction(actionShowScriptWindow);
#endif

  standardTools->addSeparator ();
  standardTools->addAction(actionManageDirs);
  standardTools->addSeparator ();

  standardTools->addAction(actionCopySelection);
  standardTools->addAction(actionPasteSelection);

  plotTools = new QToolBar(tr("Plot"), this);
  plotTools->setObjectName("plotTools"); // this is needed for QMainWindow::restoreState()
  plotTools->setIconSize( QSize(16,20) );
  addToolBar( plotTools );

  dataTools = new QActionGroup( this );
  dataTools->setExclusive( true );

  btnPointer = new QAction(tr("Disable &Tools"), this);
  btnPointer->setActionGroup(dataTools);
  btnPointer->setCheckable( true );
  btnPointer->setIcon(QIcon(getQPixmap("pointer_xpm")) );
  btnPointer->setChecked(true);
  plotTools->addAction(btnPointer);

  actionPanPlot->setActionGroup(dataTools);
  actionPanPlot->setCheckable( true );
  plotTools->addAction(actionPanPlot);

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
  plotTools->addAction(actionUnzoom);

  btnCursor = new QAction(tr("&Data Reader"), this);
  btnCursor->setShortcut( tr("CTRL+D") );
  btnCursor->setActionGroup(dataTools);
  btnCursor->setCheckable( true );
  btnCursor->setIcon(QIcon(getQPixmap("select_xpm")) );
  //plotTools->addAction(btnCursor); disabled until fixed (#2783)
  btnPicker = new QAction(tr("S&creen Reader"), this);
  btnPicker->setActionGroup(dataTools);
  btnPicker->setCheckable( true );
  btnPicker->setIcon(QIcon(getQPixmap("cursor_16_xpm")) );
  plotTools->addAction(btnPicker); //disabled until fixed (#2783)

  actionDrawPoints = new QAction(tr("&Draw Data Points"), this);
  actionDrawPoints->setActionGroup(dataTools);
  actionDrawPoints->setCheckable( true );
  actionDrawPoints->setIcon(QIcon(getQPixmap("draw_points_xpm")) );
  //plotTools->addAction(actionDrawPoints); disabled until fixed (#2783)

  btnMovePoints = new QAction(tr("&Move Data Points..."), this);
  btnMovePoints->setShortcut( tr("Ctrl+ALT+M") );
  btnMovePoints->setActionGroup(dataTools);
  btnMovePoints->setCheckable( true );
  btnMovePoints->setIcon(QIcon(getQPixmap("hand_xpm")) );
  //plotTools->addAction(btnMovePoints); disabled until fixed (#2783)

  btnRemovePoints = new QAction(tr("Remove &Bad Data Points..."), this);
  btnRemovePoints->setShortcut( tr("Alt+B") );
  btnRemovePoints->setActionGroup(dataTools);
  btnRemovePoints->setCheckable( true );
  btnRemovePoints->setIcon(QIcon(getQPixmap("gomme_xpm")));
  //plotTools->addAction(btnRemovePoints); disabled until fixed (#2783)

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

  btnLabel = new QAction(tr("Label &Tool"), this);
  btnLabel->setShortcut(tr("Ctrl+Alt+T"));
  btnLabel->setActionGroup(dataTools);
  btnLabel->setIcon(QIcon(getQPixmap("text_xpm")));
  btnLabel->setCheckable(true);
  plotTools->addAction(btnLabel);

  btnArrow = new QAction(tr("Draw &Arrow"), this);
  btnArrow->setShortcut( tr("Ctrl+Alt+A") );
  btnArrow->setActionGroup(dataTools);
  btnArrow->setCheckable( true );
  btnArrow->setIcon(QIcon(getQPixmap("arrow_xpm")) );
  plotTools->addAction(btnArrow);

  btnLine = new QAction(tr("Draw Li&ne"), this);
  btnLine->setShortcut( tr("Ctrl+Alt+N") );
  btnLine->setActionGroup(dataTools);
  btnLine->setCheckable( true );
  btnLine->setIcon(QIcon(getQPixmap("lPlot_xpm")) );
  plotTools->addAction(btnLine);

  plotTools->addSeparator();
  plotTools->addAction(actionAddFunctionCurve);
  plotTools->addAction(actionNewLegend);
  plotTools->addSeparator();

  plotTools->hide();

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
  displayBar->setWindowTitle(tr("Data Display"));
  plotTools->setWindowTitle(tr("Plot"));
  standardTools->setWindowTitle(tr("Standard Tools"));
  formatToolBar->setWindowTitle(tr("Format"));

  fileMenu->changeItem(recentMenuID, tr("&Recent Projects"));
  fileMenu->changeItem(recentFilesMenuID, tr("R&ecent Files"));

  translateActionsStrings();
  customMenu(activeWindow());
}

void ApplicationWindow::initMainMenu()
{
  fileMenu = new QMenu(this);
  fileMenu->setObjectName("fileMenu");
  connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));

  newMenu = new QMenu(this);
  recentProjectsMenu = new QMenu(this);
  recentFilesMenu = new QMenu(this);
  newMenu->setObjectName("newMenu");
  exportPlotMenu = new QMenu(this);
  exportPlotMenu->setObjectName("exportPlotMenu");

  edit = new QMenu(this);
  edit->setObjectName("editMenu");

  edit->insertSeparator();
  edit->addAction(actionCopySelection);
  edit->addAction(actionPasteSelection);
  edit->insertSeparator();
  edit->addAction(actionDeleteFitTables);

  connect(edit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

  view = new QMenu(this);
  view->setObjectName("viewMenu");

  view->setCheckable(true);

  view->addAction(actionShowExplorer);
  view->addAction(actionShowLog);

  view->insertSeparator();
  view->addAction(actionShowScriptWindow);//Mantid
  view->addAction(actionShowScriptInterpreter);
  view->insertSeparator();

  mantidUI->addMenuItems(view);

  view->insertSeparator();
  toolbarsMenu = view->addMenu(tr("&Toolbars"));
  view->addAction(actionShowConfigureDialog);
  view->insertSeparator();
  view->addAction(actionCustomActionDialog);

  graph = new QMenu(this);
  graph->setObjectName("graphMenu");
  graph->setCheckable(true);
  graph->addAction(actionAddErrorBars);
  graph->addAction(actionRemoveErrorBars);
  graph->addAction(actionShowCurvesDialog);
  graph->addAction(actionAddFunctionCurve);
  graph->addAction(actionNewLegend);
  graph->insertSeparator();
  graph->addAction(btnLabel);
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

  interfaceMenu = new QMenu(this);
  interfaceMenu->setObjectName("interfaceMenu");
  connect(interfaceMenu, SIGNAL(aboutToShow()), this, SLOT(interfaceMenuAboutToShow()));

  foldersMenu = new QMenu(this);
  foldersMenu->setCheckable(true);

  tiledWindowMenu = new QMenu(this);
  tiledWindowMenu->setObjectName("tiledWindowMenu");
  connect(tiledWindowMenu, SIGNAL(aboutToShow()), this, SLOT(tiledWindowMenuAboutToShow()));

  help = new QMenu(this);
  help->setObjectName("helpMenu");

  help->addAction(actionHomePage);
  help->addAction(actionMantidConcepts);
  help->addAction(actionMantidAlgorithms);
  help->addAction(actionmantidplotHelp);
  help->insertSeparator();
  help->addAction(actionHelpBugReports);
  help->addAction(actionAskHelp);
  help->insertSeparator();
  help->addAction(actionFirstTimeSetup);
  help->insertSeparator();

  ///The paraview action should only be available on windows
#ifdef _WIN32
  help->addAction(actionSetupParaview);
  help->insertSeparator();
#endif

  help->addAction(actionAbout);

  icat = new QMenu(this);
  icat->setObjectName("CatalogMenu");
  connect(icat, SIGNAL(aboutToShow()), this, SLOT(populateCatalogLoginMenu()));

  disableActions();
}

void ApplicationWindow::tableMenuAboutToShow()
{
  tableMenu->clear();
  fillMenu->clear();

  MdiSubWindow* t = activeWindow();
  if ( t == NULL ) return;

  Table *table = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!table)
    return;

  bool isFixedColumns = table->isFixedColumns();
  bool isEditable = table->isEditable();

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
  if (isEditable) tableMenu->insertSeparator();

  if (isEditable) tableMenu->addAction(actionShowColumnValuesDialog);
  if (isEditable) tableMenu->addAction(actionTableRecalculate);

  if (isEditable)
  {
    fillMenu = tableMenu->addMenu (tr("&Fill Columns With"));
    fillMenu->addAction(actionSetAscValues);
    fillMenu->addAction(actionSetRandomValues);
  }

  if (isEditable) tableMenu->addAction(actionClearTable);
  tableMenu->insertSeparator();
  if (!isFixedColumns) tableMenu->addAction(actionAddColToTable);
  tableMenu->addAction(actionShowColsDialog);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionHideSelectedColumns);
  tableMenu->addAction(actionShowAllColumns);
  if (!isFixedColumns) tableMenu->insertSeparator();
  if (!isFixedColumns) tableMenu->addAction(actionMoveColFirst);
  if (!isFixedColumns) tableMenu->addAction(actionMoveColLeft);
  if (!isFixedColumns) tableMenu->addAction(actionMoveColRight);
  if (!isFixedColumns) tableMenu->addAction(actionMoveColLast);
  if (!isFixedColumns) tableMenu->addAction(actionSwapColumns);
  tableMenu->insertSeparator();
  if (t->isA("Table")) tableMenu->addAction(actionShowRowsDialog);
  tableMenu->addAction(actionDeleteRows);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionGoToRow);
  tableMenu->addAction(actionGoToColumn);
  tableMenu->insertSeparator();
  tableMenu->addAction(actionConvertTable);
  if (t->isA("Table")) // but not MantidTable
  {
    tableMenu->addAction(actionConvertTableToWorkspace);
  }
  tableMenu->addAction(actionConvertTableToMatrixWorkspace);
  tableMenu->addAction(actionSortTable);

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
  plotDataMenu->addAction(actionPanPlot);
  plotDataMenu->addAction(actionUnzoom);
  plotDataMenu->insertSeparator();
  plotDataMenu->addAction(btnCursor);
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
  specialPlotMenu->addAction(actionWaterfallPlot);
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
  statMenu->insertSeparator();
  statMenu->addAction(actionStemPlot);

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

void ApplicationWindow::customMenu(MdiSubWindow* w)
{
  myMenuBar()->clear();
  myMenuBar()->insertItem(tr("&File"), fileMenu);
  fileMenuAboutToShow();
  myMenuBar()->insertItem(tr("&Edit"), edit);
  editMenuAboutToShow();
  myMenuBar()->insertItem(tr("&View"), view);

  // these use the same keyboard shortcut (Ctrl+Return) and should not be enabled at the same time
  actionTableRecalculate->setEnabled(false);

  if(w){
    actionPrintAllPlots->setEnabled(projectHas2DPlots());
    actionPrint->setEnabled(true);
    actionCutSelection->setEnabled(true);
    actionCopySelection->setEnabled(true);
    actionPasteSelection->setEnabled(true);
    actionClearSelection->setEnabled(true);
    QStringList tables = tableNames() + matrixNames();
    if (!tables.isEmpty())
      actionShowExportASCIIDialog->setEnabled(true);
    else
      actionShowExportASCIIDialog->setEnabled(false);

    if (w->isA("MultiLayer")) {
      myMenuBar()->insertItem(tr("&Graph"), graph);
      myMenuBar()->insertItem(tr("&Data"), plotDataMenu);
      plotDataMenuAboutToShow();
      if (m_enableQtiPlotFitting)
      {
        myMenuBar()->insertItem(tr("&Analysis"), analysisMenu);
        analysisMenuAboutToShow();
      }
      myMenuBar()->insertItem(tr("For&mat"), format);
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

      myMenuBar()->insertItem(tr("For&mat"), format);

      actionPrint->setEnabled(true);

      format->clear();
      format->addAction(actionShowPlotDialog);
      format->addAction(actionShowScaleDialog);
      format->addAction(actionShowAxisDialog);
      format->addAction(actionShowTitleDialog);

      if (dynamic_cast<Graph3D*>(w)->coordStyle() == Qwt3D::NOCOORD)
        actionShowAxisDialog->setEnabled(false);

      format->addSeparator();
      QMenu *gridLines = format->addMenu("Grid Lines");
      gridLines->addAction(front);
      gridLines->addAction(back);
      gridLines->addAction(left);
      gridLines->addAction(right);
      gridLines->addAction(ceil);
      gridLines->addAction(floor);

      QMenu *frameMenu = format->addMenu("Frame");
      frameMenu->addAction(Frame);
      frameMenu->addAction(Box);
      frameMenu->addAction(None);

      QMenu *internalView = format->addMenu("View");
      internalView->addAction(actionPerspective);
      internalView->addAction(actionResetRotation);
      internalView->addAction(actionFitFrame);

      QMenu *style = format->addMenu("Style");
      style->addAction(barstyle);
      style->addAction(pointstyle);
      style->addAction(conestyle);
      style->addAction(crossHairStyle);
      style->addSeparator();
      style->addAction(wireframe);
      style->addAction(hiddenline);
      style->addAction(polygon);
      style->addAction(filledmesh);
      style->addSeparator();
      style->addAction(floordata);
      style->addAction(flooriso);
      style->addAction(floornone);

      format->addAction(actionAnimate);

    } else if (w->inherits("Table")) {
      myMenuBar()->insertItem(tr("&Plot"), plot2DMenu);
      myMenuBar()->insertItem(tr("&Analysis"), analysisMenu);
      analysisMenuAboutToShow();
      myMenuBar()->insertItem(tr("&Table"), tableMenu);
      tableMenuAboutToShow();
      actionTableRecalculate->setEnabled(true);

    } else if (w->isA("Matrix")){
      actionTableRecalculate->setEnabled(true);
      myMenuBar()->insertItem(tr("3D &Plot"), plot3DMenu);
      myMenuBar()->insertItem(tr("&Matrix"), matrixMenu);
      matrixMenuAboutToShow();
      myMenuBar()->insertItem(tr("&Analysis"), analysisMenu);
      analysisMenuAboutToShow();

    } else if (w->isA("TiledWindow")) {
      myMenuBar()->insertItem(tr("Tiled Window"),tiledWindowMenu);

    } else if (!mantidUI->menuAboutToShow(w)) // Note that this call has a side-effect (it enables menus)
        disableActions();

  } else
    disableActions();

  myMenuBar()->insertItem(tr("&Windows"), windowsMenu);
  windowsMenuAboutToShow();
  // -- Mantid: add script actions, if any exist --
  QListIterator<QMenu*> mIter(d_user_menus);
  while( mIter.hasNext() )
  {
    QMenu* item = mIter.next();
    myMenuBar()->insertItem(tr(item->title()), item);

  }

  myMenuBar()->insertItem(tr("&Catalog"),icat);

  // -- INTERFACE MENU --
  myMenuBar()->insertItem(tr("&Interfaces"), interfaceMenu);
  interfaceMenuAboutToShow();

  myMenuBar()->insertItem(tr("&Help"), help );

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

  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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


/** Set the exit code to be returned by the application at
 * exit. Used by MantidPlot unit tests to signal failure.
 *
 * @param code :: int code, non-zero for failure
 */
void ApplicationWindow::setExitCode(int code)
{
  m_exitCode = code;
}

/** Get the exit code to be returned by the application at
 * exit. Used by MantidPlot unit tests to signal failure.
 *
 * @return code :: int code, non-zero for failure
 */
int ApplicationWindow::getExitCode()
{
  return m_exitCode;
}


void ApplicationWindow::customToolBars(MdiSubWindow* w)
{
  disableToolbars();
  if (!w)
    return;

  if (w->isA("MultiLayer") && d_plot_tool_bar){
    if(!plotTools->isVisible())
      plotTools->show();
    plotTools->setEnabled (true);
    customMultilayerToolButtons(dynamic_cast<MultiLayer*>(w));
    if(d_format_tool_bar && !formatToolBar->isVisible()){
      formatToolBar->setEnabled (true);
      formatToolBar->show();
    }
  }
  else if (w->isA("Graph3D"))
  {
    custom3DActions(w);
  }
}

void ApplicationWindow::disableToolbars()
{
  plotTools->setEnabled(false);
}

/**
 * Show/hide MantidPlot toolbars.
 * @param visible If true, make toolbar visible, if false - hidden
 */
void ApplicationWindow::setToolbarsVisible(bool visible)
{
  standardTools->setVisible(visible);
  displayBar->setVisible(visible);
  plotTools->setVisible(visible);
  formatToolBar->setVisible(visible);
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
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
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
    m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
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
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
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
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
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

QString ApplicationWindow::stemPlot(Table *t, const QString& colName, int power, int startRow, int endRow)
{
  if (!t)
    return QString();

  int col = t->colIndex(colName);
  if (col < 0){
    QMessageBox::critical(this, tr("MantidPlot - Error"),
    tr("Data set: %1 doesn't exist!").arg(colName));
    return QString();
  }

  startRow--;
  endRow--;
  if (startRow < 0 || startRow >= t->numRows())
    startRow = 0;
  if (endRow < 0 || endRow >= t->numRows())
    endRow = t->numRows() - 1;

  QString result = tr("Stem and leaf plot of dataset") + ": " + colName + " ";
  result += tr("from row") + ": " + QString::number(startRow + 1) + " ";
  result += tr("to row") + ": " + QString::number(endRow + 1) + "\n";

  int rows = 0;
  for (int j = startRow; j <= endRow; j++){
    if (!t->text(j, col).isEmpty())
       rows++;
  }

  if (rows >= 1){
    double *data = (double *)malloc(rows * sizeof (double));
    if (!data){
      result += tr("Not enough memory for this dataset!") + "\n";
      return result;
    }

    result += "\n" + tr("Stem") + " | " + tr("Leaf");
    result += "\n---------------------\n";

    int row = 0;
    for (int j = startRow; j <= endRow; j++){
      if (!t->text(j, col).isEmpty()){
        data[row] = t->cell(j, col);
        row++;
      }
    }
    gsl_sort (data, 1, rows);

    if (power > 1e3){
      power = static_cast<int>(std::ceil(log10(data[rows - 1] - data[0]) - log10(rows - 1.0)));
      bool ok;
      int input = QInputDialog::getInteger(this, tr("Please confirm the stem unit!"),
                                      tr("Data set") + ": " + colName + ", " + tr("stem unit") + " = 10<sup>n</sup>, n = ",
                                      power, -1000, 1000, 1, &ok);
      if (ok)
        power = input;
    }

    double stem_unit = pow(10.0, power);
    double leaf_unit = stem_unit/10.0;

    int prev_stem = int(data[0]/stem_unit);
    result += "      " + QString::number(prev_stem) + " | ";

    for (int j = 0; j <rows; j++){
      double val = data[j];
      int stem = int(val/stem_unit);
      int leaf = int(qRound((val - stem*stem_unit)/leaf_unit));
      for (int k = prev_stem + 1; k < stem + 1; k++)
        result += "\n      " + QString::number(k) + " | ";
      result += QString::number(leaf);
      prev_stem = stem;
    }

    result += "\n---------------------\n";
    result += tr("Stem unit") + ": " + locale().toString(stem_unit) + "\n";
    result += tr("Leaf unit") + ": " + locale().toString(leaf_unit) + "\n";

    QString legend = tr("Key") + ": " + QString::number(prev_stem) + " | ";
    int leaf = int(qRound((data[rows - 1] - prev_stem*stem_unit)/leaf_unit));
    legend += QString::number(leaf);
    legend += " " + tr("means") + ": " + locale().toString(prev_stem*stem_unit + leaf*leaf_unit) + "\n";

    result += legend + "---------------------\n";
    free(data);
  } else
    result += "\t" + tr("Input error: empty data set!") + "\n";
  return result;
}

Note * ApplicationWindow::newStemPlot()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return NULL;

  int ts = t->table()->currentSelection();
  if (ts < 0)
    return NULL;

  Note *n = newNote();
  if (!n)
    return NULL;
  n->hide();

  QStringList lst = t->selectedColumns();
  if (lst.isEmpty()){
    Q3TableSelection sel = t->table()->selection(ts);
    for (int i = sel.leftCol(); i <= sel.rightCol(); i++)
      n->setText(n->text() + stemPlot(t, t->colName(i), 1001, sel.topRow() + 1, sel.bottomRow() + 1) + "\n");
  } else {
    for (int i = 0; i < lst.count(); i++)
      n->setText(n->text() + stemPlot(t, lst[i], 1001) + "\n");
  }

  n->show();
  return n;
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
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurveNames(oldName, newName);
    } else if (w->isA("Graph3D")) {
      QString name = dynamic_cast<Graph3D*>(w)->formula();
      if (name.contains(oldName, true)) {
        name.replace(oldName,newName);
        dynamic_cast<Graph3D*>(w)->setPlotAssociation(name);
      }
    }
  }
}

void ApplicationWindow::updateColNames(const QString& oldName, const QString& newName)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurveNames(oldName, newName, false);
    }
    else if (w->isA("Graph3D")){
      QString name = dynamic_cast<Graph3D*>(w)->formula();
      if (name.contains(oldName)){
        name.replace(oldName,newName);
        dynamic_cast<Graph3D*>(w)->setPlotAssociation(name);
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
      QString s = dynamic_cast<Graph3D*>(w)->formula();
      if (s.contains(oldName))
      {
        s.replace(oldName, newName);
        dynamic_cast<Graph3D*>(w)->setPlotAssociation(s);
      }
    }
    else if (w->isA("MultiLayer"))
    {
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          QwtPlotItem *sp = dynamic_cast<QwtPlotItem*>(g->plotItem(i));
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
    if (w->isA("Graph3D") && dynamic_cast<Graph3D*>(w)->matrix() == m)
      dynamic_cast<Graph3D*>(w)->clearData();
    else if (w->isA("MultiLayer")){
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          if (g->curveType(i) == Graph::Histogram){
            QwtHistogram *h = dynamic_cast<QwtHistogram*>(g->plotItem(i));
            if (h && h->matrix() == m)
              g->removeCurve(i);
          } else {
            Spectrogram *sp = dynamic_cast<Spectrogram*>(g->plotItem(i));
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
  Matrix *m = dynamic_cast<Matrix *>(window);
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D") && dynamic_cast<Graph3D*>(w)->matrix() == m)
      dynamic_cast<Graph3D*>(w)->updateMatrixData(m);
    else if (w->isA("MultiLayer")){
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers){
        for (int i=0; i<g->curves(); i++){
          if (g->curveType(i) == Graph::Histogram){
            QwtHistogram *h = dynamic_cast<QwtHistogram*>(g->plotItem(i));
            if (h && h->matrix() == m)
              h->loadData();
          } else {
            Spectrogram *sp = dynamic_cast<Spectrogram*>(g->plotItem(i));
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

  Graph3D* g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (g && g->matrix())
    ad->setCurentDataSet(g->matrix()->objectName());
  ad->exec();
}

void ApplicationWindow::change3DMatrix(const QString& matrix_name)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D* g = dynamic_cast<Graph3D*>(w);
  Matrix *m = matrix(matrix_name);
  if (m && g)
    g->addMatrixData(m);

  emit modified();
}

void ApplicationWindow::add3DMatrixPlot()
{
  QStringList matrices = matrixNames();
  if (static_cast<int>(matrices.count()) <= 0)
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

  dynamic_cast<Graph3D*>(w)->addMatrixData(matrix(matrix_name));
  emit modified();
}

void ApplicationWindow::insertNew3DData(const QString& colName)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  dynamic_cast<Graph3D*>(w)->insertNewData(table(colName),colName);
  emit modified();
}

void ApplicationWindow::change3DData(const QString& colName)
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  dynamic_cast<Graph3D*>(w)->changeDataColumn(table(colName), colName);
  emit modified();
}

void ApplicationWindow::editSurfacePlot()
{
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D* g = dynamic_cast<Graph3D*>(w);
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
    double yl, double yr, double zl, double zr, size_t columns, size_t rows)
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
  addMdiSubWindow(plot);
  connectSurfacePlot(plot);

  plot->setIcon(getQPixmap("trajectory_xpm"));
  plot->show();
  plot->setFocus();

  customMenu(plot);
  customToolBars(plot);
}

void ApplicationWindow::exportMatrix()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
    m = dynamic_cast<Matrix*>(w);
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

void ApplicationWindow::loadScriptRepo(){
   MantidQt::API::ScriptRepositoryView *ad = new MantidQt::API::ScriptRepositoryView(this);
   connect(ad, SIGNAL(loadScript(const QString)),this,SLOT(loadScript(const QString& )));
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
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

/**
 * Prepares MultiLayer window for plotting - creates it if necessary, clears it, applies initial
 * settings etc.
 * @param isNew         :: Whether the Graph used for plotting was created, or the old one was used
 * @param window        :: Existing MultiLayer window. If NULL - a new one will be created
 * @param newWindowName :: Name of the new window if one is created
 * @param clearWindow   :: Whether to clear existing window before plotting. Ignored if window is NULL
 * @return Pointer to created window if window == NULL, otherwise - window.
 */
MultiLayer* ApplicationWindow::prepareMultiLayer(bool& isNew, MultiLayer* window, const QString& newWindowName, bool clearWindow)
{
  isNew = false;

  if(window == NULL)
  { // If plot window is not specified, create a new one
    window = multilayerPlot(generateUniqueName( newWindowName + "-"));
    window->setCloseOnEmpty(true);
    isNew = true;
  }
  else if(clearWindow)
  {
    window->setLayersNumber(0); // Clear by removing all the layers
  }

  if (window->isEmpty())
  { // This will add a new layer in two situations: when we've cleared the window manually,
    // or when the window specified didn't actually have any layers
    window->addLayer();
    isNew = true;
  }

  if(isNew)
  { // If new graph was created, need to set some initial stuff

    Graph *g = window->activeGraph(); // We use active graph only. No support for proper _multi_ layers yet.

    connect(g,SIGNAL(curveRemoved()),window,SLOT(maybeNeedToClose()), Qt::QueuedConnection);
    setPreferences(g);
    g->newLegend();
    g->setTitle( newWindowName );
  }

  return window;
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
  /* The 'setAutoScale' above is needed to make sure that the plot initially encompasses all the
   * data points. However, this has the side-effect suggested by its name: all the axes become
   * auto-scaling if the data changes. If, in the plot preferences, autoscaling has been disabled
   * the the next line re-fixes the axes
   */
  if ( ! autoscale2DPlots ) ag->enableAutoscaling(false);

  QApplication::restoreOverrideCursor();
  return g;
}

MultiLayer* ApplicationWindow::multilayerPlot(int c, int r, int style)
{//used when plotting from the panel menu
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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
    Table *w = dynamic_cast<Table *>(table(caption));

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
      c = dynamic_cast<PlotCurve *>(ag->addErrorBars(xColName, yColName, w, errColName, errType));
    } else
      c = dynamic_cast<PlotCurve *>(ag->insertCurve(w, xCol, yColName, defaultCurveStyle));

    CurveLayout cl = ag->initCurveLayout(defaultCurveStyle, curves - errorBars);
    cl.lWidth = float(defaultCurveLineWidth);
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
  g->setScaleLayersOnPrint(d_scale_plots_on_print);
  g->printCropmarks(d_print_cropmarks);

  connectMultilayerPlot(g);

  addMdiSubWindow(g);
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
      customTable(dynamic_cast<Table*>(w));
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
        dynamic_cast<Table*>(w)->setAutoUpdateValues(d_auto_update_table_values);
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
  if (!g)
    return;

  if (!g->isPiePlot())
  {
    for (int i = 0; i < QwtPlot::axisCnt; i++)
    {
      bool show = d_show_axes[i];
      g->enableAxis(i, show);
      if (show)
      {
        ScaleDraw *sd = static_cast<ScaleDraw *>(g->plotWidget()->axisScaleDraw (i));
        sd->enableComponent(QwtAbstractScaleDraw::Labels, d_show_axes_labels[i]);
        sd->setSpacing(d_graph_tick_labels_dist);
        if (i == QwtPlot::yRight && !d_show_axes_labels[i])
          g->setAxisTitle(i, tr(" "));
      }
    }



    //set the scale type i.e. log or linear
    g->setScale(QwtPlot::yLeft, d_axes_scales[0]);
    g->setScale(QwtPlot::yRight, d_axes_scales[1]);
    g->setScale(QwtPlot::xBottom, d_axes_scales[2]);
    g->setScale(QwtPlot::xTop, d_axes_scales[3]);

    // QtiPlot makes these calls here (as of 26/6/12), but they spoil color fill plots for us.
    //   Losing them seems to have no detrimental effect. Perhaps we need to update our updateSecondaryAxis code to match QtiPlot's.
    //g->updateSecondaryAxis(QwtPlot::xTop);
    //g->updateSecondaryAxis(QwtPlot::yRight);

    QList<int> ticksList;
    ticksList<<majTicksStyle<<majTicksStyle<<majTicksStyle<<majTicksStyle;
    g->setMajorTicksType(ticksList);
    ticksList.clear();
    ticksList<<minTicksStyle<<minTicksStyle<<minTicksStyle<<minTicksStyle;
    g->setMinorTicksType(ticksList);

    g->setTicksLength (minTicksLength, majTicksLength);
    g->setAxesLinewidth(axesLineWidth);
    g->drawAxesBackbones(drawBackbones);
    for (int i = 0; i < QwtPlot::axisCnt; i++)
      g->setAxisTitleDistance(i, d_graph_axes_labels_dist);
    //    need to call the plot functions for log/linear, errorbars and distribution stuff

  }

  g->setSynchronizedScaleDivisions(d_synchronize_graph_scales);
  g->initFonts(plotAxesFont, plotNumbersFont);
  g->initTitle(titleOn, plotTitleFont);
  g->setCanvasFrame(canvasFrameWidth);
  g->plotWidget()->setMargin(defaultPlotMargin);


  g->enableAutoscaling(autoscale2DPlots);
  g->setAutoscaleFonts(autoScaleFonts);
  g->setIgnoreResizeEvents(!autoResizeLayers);
  g->setAntialiasing(antialiasing2DPlots);
  g->enableFixedAspectRatio(fixedAspectRatio2DPlots);

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

bool ApplicationWindow::isDeleteWorkspacePromptEnabled()
{
  return d_inform_delete_workspace;
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

/* Perfom initialization on a Table?
 * @param w :: table that was created
 * @param caption :: title to set
 */
void ApplicationWindow::initTable(Table* w, const QString& caption)
{
  QString name = caption;

  while(name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Table"));

  connectTable(w);
  customTable(w);

  w->setName(name);
  if(!w->isA("MantidTable"))
    w->setIcon( getQPixmap("worksheet_xpm") );

  addMdiSubWindow(w);
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
  Note* m = new Note("", this);

  QString name = caption;
  while(name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Notes"));

  m->setName(name);
  m->confirmClose(confirmCloseNotes);

  addMdiSubWindow(m);
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
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->transpose();
}

void ApplicationWindow::flipMatrixVertically()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->flipVertically();
}

void ApplicationWindow::flipMatrixHorizontally()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->flipHorizontally();
}

void ApplicationWindow::rotateMatrix90()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->rotate90();
}

void ApplicationWindow::rotateMatrixMinus90()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->rotate90(false);
}

void ApplicationWindow::matrixDeterminant()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QDateTime dt = QDateTime::currentDateTime ();
  QString info=dt.toString(Qt::LocalDate);
  info+= "\n" + tr("Determinant of ") + QString(m->objectName()) + ":\t";
  info+= "det = " + QString::number(m->determinant()) + "\n";
  info+="-------------------------------------------------------------\n";

  currentFolder()->appendLogInfo(info);

  showResults(true);
}

void ApplicationWindow::invertMatrix()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->invert();
}

Table* ApplicationWindow::convertMatrixToTableDirect()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return 0;

  return matrixToTable(m, Direct);
}

Table* ApplicationWindow::convertMatrixToTableXYZ()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return 0;

  return matrixToTable(m, XYZ);
}

Table* ApplicationWindow::convertMatrixToTableYXZ()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
  m->confirmClose(confirmCloseMatrix);
  m->setNumericPrecision(d_decimal_digits);

  addMdiSubWindow(m);

  connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(updateMatrixPlots(MdiSubWindow *)));

  emit modified();
}

Matrix* ApplicationWindow::convertTableToMatrix()
{
  Table* t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return 0;

  return tableToMatrix (t);
}

/**
 * Convert Table in the active window to a TableWorkspace
 */
void ApplicationWindow::convertTableToWorkspace()
{
  Table* t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t) return;
  convertTableToTableWorkspace(t);
}

/**
 * Convert Table in the active window to a MatrixWorkspace
 */
void ApplicationWindow::convertTableToMatrixWorkspace()
{
  Table* t = dynamic_cast<Table*>(activeWindow(TableWindow));
	if (!t) return;

	// dynamic_cast is successful when converting MantidTable to MatrixWorkspace
	auto *mt = dynamic_cast<MantidTable*>(t);

	if (!mt){
	// if dynamic_cast is unsuccessful, create MantidTable from which to create MatrixWorkspace
		mt = convertTableToTableWorkspace(t);
	}

	if (mt){
		QHash<QString,QString> params;
		params["InputWorkspace"] = QString::fromStdString(mt->getWorkspaceName());
		mantidUI->showAlgorithmDialog(QString("ConvertTableToMatrixWorkspace"),params);
	}

}

/**
 * Convert a Table to a TableWorkspace. Columns with plot designations X,Y,Z,xErr,yErr
 * are transformed to doubles, others - to strings.
 * @param t :: The Table to convert.
 */
MantidTable* ApplicationWindow::convertTableToTableWorkspace(Table* t)
{
  if (!t) return NULL;
  std::vector<int> format(t->numCols(),-1);
  std::vector<int> precision(t->numCols(),-1);
  Mantid::API::ITableWorkspace_sptr tws = Mantid::API::WorkspaceFactory::Instance().createTable();
  for(int col = 0; col < t->numCols(); ++col)
  {
    Table::PlotDesignation des = (Table::PlotDesignation)t->colPlotDesignation(col);
    QString name = t->colLabel(col);
    std::string type;
    int plotType = 6; // Label
    switch(des)
    {
    case Table::X:     { plotType = 1; type = "double"; break; }
    case Table::Y:     { plotType = 2; type = "double"; break; }
    case Table::Z:     { plotType = 3; type = "double"; break; }
    case Table::xErr:  { plotType = 4; type = "double"; break; }
    case Table::yErr:  { plotType = 5; type = "double"; break; }
    default:
      type = "string"; plotType = 6;
    }

    if ( plotType < 6 )
    {
        // temporarily convert numeric columns to format that doesn't use commas in numbers
        t->columnNumericFormat(col,&format[col],&precision[col]);
        t->setColNumericFormat(2,precision[col],col);
    }
    std::string columnName = name.toStdString();
    tws->addColumn(type,columnName);
    Mantid::API::Column_sptr column = tws->getColumn(columnName);
    column->setPlotType(plotType);
  }
  // copy data from table to workspace
  tws->setRowCount(t->numRows());
  for(int col = 0; col < t->numCols(); ++col)
  {
    Mantid::API::Column_sptr column = tws->getColumn(col);
    for(int row = 0; row < t->numRows(); ++row)
    {
      column->read(row, t->text(row,col).toStdString());
    }
  }
  // restore original format of numeric columns
  for(int col = 0; col < t->numCols(); ++col)
  {
      if ( format[col] >= 0 )
      {
          t->setColNumericFormat(format[col],precision[col],col);
      }
  }
  std::string wsName = t->objectName().toStdString();
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName))
  {
    if ( QMessageBox::question(this, "MantidPlot","Workspace with name " + t->objectName() + " already exists\n"
      "Do you want to overwrite it?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes )
    {
      Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,tws);
    }
    else
    {
      return NULL;
    }
  }
  else
  {
    Mantid::API::AnalysisDataService::Instance().add(wsName,tws);
  }
  return new MantidTable(scriptingEnv(), tws, t->objectName(), this);
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
        return dynamic_cast<Table*>(w);
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
        return dynamic_cast<Matrix*>(w);
    }
    f = f->folderBelow();
  }
  return  0;
}

MdiSubWindow *ApplicationWindow::activeWindow(WindowType type)
{
  MdiSubWindow* active = getActiveWindow();
  if (!active)
    return NULL;

  switch(type){
  case NoWindow:
    break;

  case TableWindow:
    if (active->inherits("Table"))
      return active;
    else
      return NULL;
    break;

  case MatrixWindow:
    if (active->inherits("Matrix"))//Mantid
      return active;
    else
      return NULL;
    break;

  case MultiLayerWindow:
    if (active->isA("MultiLayer"))
      return active;
    else
      return NULL;
    break;

  case NoteWindow:
    if (active->isA("Note"))
      return active;
    else
      return NULL;
    break;

  case Plot3DWindow:
    if (active->isA("Graph3D"))
      return active;
    else
      return NULL;
    break;
  }
  return active;
}

void ApplicationWindow::windowActivated(QMdiSubWindow *w)
{
  if ( !w ) return;

  MdiSubWindow *qti_subwin = qobject_cast<MdiSubWindow*>(w->widget());
  if( !qti_subwin ) return;

  activateWindow(qti_subwin);

}

void ApplicationWindow::addErrorBars()
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  MultiLayer* plot = dynamic_cast<MultiLayer*>(w);
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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

  MultiLayer* plot = dynamic_cast<MultiLayer*>(w);
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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

  Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
  if (!g)
    return;

  g->removeMantidErrorBars(name);

}

void ApplicationWindow::defineErrorBars(const QString& name, int type, const QString& percent, int direction, bool drawAll)
{
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
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

  DataCurve *master_curve = dynamic_cast<DataCurve *>(g->curve(name));
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

  QVarLengthArray<double> Y(t->col(ycol));
 // Y=t->col(ycol);
  QString errColName=t->colName(c);

  double prc=percent.toDouble();
  if (type==0){
    for (int i=0;i<r;i++){
      if (!t->text(i,ycol).isEmpty())
        t->setText(i, c, QString::number(Y[i]*prc/100.0,'g',15));
    }
  } else if (type==1) {
    int i;
    double dev=0.0;
    double moyenne=0.0;
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
  QwtErrorPlotCurve * errs = g->addErrorBars(xColName, name, t, errColName, direction);
  if ( errs )
  {
    // Error bars should be the same color as the curve line
    errs->setColor(master_curve->pen().color());
    g->updatePlot();
  }
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

  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QwtErrorPlotCurve * errs = g->addErrorBars(curveName, errTable, errColumnName, direction);
  if ( errs )
  {
    QwtPlotCurve * master_curve = g->curve(curveName);
    if (master_curve) errs->setColor(master_curve->pen().color());
    g->updatePlot();
  }
  emit modified();
}

void ApplicationWindow::removeCurves(const QString& name)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer"))
    {
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers)
      g->removeCurves(name);
    }
    else if (w->isA("Graph3D"))
    {
      if ( (dynamic_cast<Graph3D*>(w)->formula()).contains(name) )
        dynamic_cast<Graph3D*>(w)->clearData();
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateCurves(Table *t, const QString& name)
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers)
      g->updateCurvesData(t, name);
    } else if (w->isA("Graph3D")){
      Graph3D* g = dynamic_cast<Graph3D*>(w);
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
      {w->confirmClose(confirmCloseTable);
      }
    }
  }

  if (confirmCloseMatrix != askMatrices){
    confirmCloseMatrix = askMatrices;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Matrix"))
      {w->confirmClose(confirmCloseMatrix);
      }
    }
  }

  if (confirmClosePlot2D != askPlots2D){
    confirmClosePlot2D=askPlots2D;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("MultiLayer"))
      {w->confirmClose(confirmClosePlot2D);
      }
    }
  }

  if (confirmClosePlot3D != askPlots3D){
    confirmClosePlot3D=askPlots3D;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Graph3D"))
        w->confirmClose(confirmClosePlot3D);
    }
  }

  if (confirmCloseNotes != askNotes){
    confirmCloseNotes = askNotes;
    foreach(MdiSubWindow *w, windows){
      if (w->isA("Note"))
        w->confirmClose(confirmCloseNotes);
    }
  }

  if (confirmCloseInstrWindow != askInstrWindow){
    confirmCloseInstrWindow = askInstrWindow;

    foreach(MdiSubWindow *w, windows){
      if (w->isA("InstrumentWindow"))
      {w->confirmClose(confirmCloseInstrWindow);
      }
    }
  }
}

void ApplicationWindow::setGraphDefaultSettings(bool autoscale, bool scaleFonts,
    bool resizeLayers, bool antialiasing, bool fixedAspectRatio)
{
  if (autoscale2DPlots == autoscale &&
      autoScaleFonts == scaleFonts &&
      autoResizeLayers != resizeLayers &&
      antialiasing2DPlots == antialiasing &&
      fixedAspectRatio2DPlots == fixedAspectRatio)
    return;

  autoscale2DPlots = autoscale;
  autoScaleFonts = scaleFonts;
  autoResizeLayers = !resizeLayers;
  antialiasing2DPlots = antialiasing;
  fixedAspectRatio2DPlots = fixedAspectRatio;

  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer"))
    {
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers)
      {
        g->enableAutoscaling(autoscale2DPlots);
        g->updateScale();
        g->setIgnoreResizeEvents(!autoResizeLayers);
        g->setAutoscaleFonts(autoScaleFonts);
        g->setAntialiasing(antialiasing2DPlots);
        g->enableFixedAspectRatio(fixedAspectRatio2DPlots);
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
      app->d_ASCII_import_read_only, Table::Overwrite, app->d_eol);
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
  d_eol = (EndLineChar)import_dialog->endLineChar();
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
      Table *t = dynamic_cast<Table*>(w);
      for (int i=0; i<files.size(); i++)
        t->importASCII(files[i], local_column_separator, local_ignored_lines, local_rename_columns,
            local_strip_spaces, local_simplify_spaces, local_import_comments,
            local_comment_string, import_read_only, (Table::ImportMode)(import_mode - 2), endLineChar);

      if (update_dec_separators)
        t->updateDecimalSeparators(local_separators);
      t->notifyChanges();
      emit modifiedProject(t);
    } else if (w->isA("Matrix")){
      Matrix *m = dynamic_cast<Matrix*>(w);
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
      Table *t = dynamic_cast<Table*>(w);
      t->importASCII(files[0], local_column_separator, local_ignored_lines, local_rename_columns,
          local_strip_spaces, local_simplify_spaces, local_import_comments,
          local_comment_string, import_read_only, Table::Overwrite, endLineChar);
      if (update_dec_separators)
        t->updateDecimalSeparators(local_separators);
      t->notifyChanges();
    } else if (w->isA("Matrix")){
      Matrix *m = dynamic_cast<Matrix*>(w);
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
      Mantid::API::IAlgorithm_sptr alg =mantidUI->createAlgorithm("LoadAscii");
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
    file_uncompress(fname.ascii());
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
  const int fileVersion = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();
  ApplicationWindow* app = openProject(fname, fileVersion);
  f.close();
  return app;
}

void ApplicationWindow::openRecentFile(int index)
{
  QString fn = recentFilesMenu->text(index);
  int pos = fn.find(" ",0);
  fn = fn.right(fn.length()-pos-1);

  QFile f(fn);
  if (!f.exists()){
    QMessageBox::critical(this, tr("MantidPlot - File Open Error"),//Mantid
                          tr("The file: <b> %1 </b> <p>is not there anymore!"
                             "<p>It will be removed from the list of recent files.").arg(fn));

    recentFiles.remove(fn);
    updateRecentFilesList();
    return;
  }

  loadDataFileByName(fn);
  saveSettings();   // save new list of recent files
}

void ApplicationWindow::openRecentProject(int index)
{
  QString fn = recentProjectsMenu->text(index);
  int pos = fn.find(" ",0);
  fn = fn.right(fn.length()-pos-1);

  QFile f(fn);
  if (!f.exists()){
    QMessageBox::critical(this, tr("MantidPlot - File Open Error"),//Mantid
        tr("The file: <b> %1 </b> <p>does not exist anymore!"
            "<p>It will be removed from the list of recent projects.").arg(fn));

    recentProjects.remove(fn);
    updateRecentProjectsList();
    return;
  }

  if (projectname != "untitled"){
    QFileInfo fi(projectname);
    QString pn = fi.absFilePath();
    if (fn == pn){
      QMessageBox::warning(this, tr("MantidPlot - File open error"),//Mantid
          tr("The file: <p><b> %1 </b><p> is the current file!").arg(fn));
      return;
    }
  }

  if (!fn.isEmpty()){
    saveSettings();//the recent projects must be saved
    bool isSaved = saved;
    // Have to change the working directory here because that is used when finding the nexus files to load
    workingDir = QFileInfo(f).absolutePath();
    ApplicationWindow * a = open (fn,false,false);
    if (a && (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
        fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive)))
      if (isSaved)
        savedProject();//force saved state
    //close();
  }
}

ApplicationWindow* ApplicationWindow::openProject(const QString& filename, const int fileVersion)
{
  newProject();
  m_mantidmatrixWindows.clear();

  projectname = filename;
  setWindowTitle("MantidPlot - " + filename);

  d_opening_file = true;

  QFile file(filename);
  QFileInfo fileInfo(filename);

  file.open(QIODevice::ReadOnly);
  QTextStream fileTS(&file);
  fileTS.setEncoding(QTextStream::UnicodeUTF8);

  QString baseName = fileInfo.fileName();

  //Skip mantid version line
  fileTS.readLine();

  //Skip the <scripting-lang> line. We only really use python now anyway.
  fileTS.readLine();
  setScriptingLanguage("Python");

  //Skip the <windows> line.
  fileTS.readLine();

  folders->blockSignals(true);
  blockSignals(true);

  Folder* curFolder = projectFolder();

  //rename project folder item
  FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
  item->setText(0, fileInfo.baseName());
  item->folder()->setObjectName(fileInfo.baseName());

  //Read the rest of the project file in for parsing
  std::string lines = fileTS.readAll().toUtf8().constData();

  d_loaded_current = 0;

  //Open as a top level folder
  openProjectFolder(lines, fileVersion, true);

  if(d_loaded_current)
    curFolder = d_loaded_current;

  {
    //WHY use another fileinfo?
    QFileInfo fi2(file);
    QString fileName = fi2.absFilePath();
    recentProjects.remove(filename);
    recentProjects.push_front(filename);
    updateRecentProjectsList();
  }

  folders->setCurrentItem(curFolder->folderListItem());
  folders->blockSignals(false);

  //change folder to user defined current folder
  changeFolder(curFolder, true);

  blockSignals(false);

  renamedTables.clear();

  restoreApplicationGeometry();

  savedProject();
  d_opening_file = false;
  d_workspace->blockSignals(false);

  return this;
}

void ApplicationWindow::openProjectFolder(std::string lines, const int fileVersion, const bool isTopLevel)
{
  //If we're not the top level folder, read the folder settings and create the folder
  //This is a legacy edgecase because folders are written <folder>\tsettings\tgo\there
  if(!isTopLevel && lines.size() > 0)
  {
    std::vector<std::string> lineVec;
    boost::split(lineVec, lines, boost::is_any_of("\n"));

    std::string firstLine = lineVec.front();

    std::vector<std::string> values;
    boost::split(values, firstLine, boost::is_any_of("\t"));

    Folder* newFolder = new Folder(currentFolder(), QString::fromStdString(values[1]));
    newFolder->setBirthDate(QString::fromStdString(values[2]));
    newFolder->setModificationDate(QString::fromStdString(values[3]));

    if(values.size() > 4 && values[4] == "current")
      d_loaded_current = newFolder;


    FolderListItem* fli = new FolderListItem(currentFolder()->folderListItem(), newFolder);
    newFolder->setFolderListItem(fli);

    d_current_folder = newFolder;

    //Remove the first line (i.e. the folder's settings line)
    lineVec.erase(lineVec.begin());
    lines = boost::algorithm::join(lineVec, "\n");
  }

  //This now ought to be the regular contents of a folder. Parse as normal.
  TSVSerialiser tsv(lines);

  //If this is the top level folder of the project, we'll need to load the workspaces before anything else.
  if(isTopLevel && tsv.hasSection("mantidworkspaces"))
  {
    //There should only be one of these, so we only read the first.
    std::string workspaces = tsv.sections("mantidworkspaces").front();
    populateMantidTreeWidget(QString::fromStdString(workspaces));
  }

  if(tsv.hasSection("open"))
  {
    std::string openStr = tsv.sections("open").front();
    int openValue = 0;
    std::stringstream(openStr) >> openValue;
    currentFolder()->folderListItem()->setOpen(openValue);
  }

  if(tsv.hasSection("mantidmatrix"))
  {
    std::vector<std::string> matrices = tsv.sections("mantidmatrix");
    for(auto it = matrices.begin(); it != matrices.end(); ++it)
    {
      openMantidMatrix(*it);
    }
  }

  if(tsv.hasSection("table"))
  {
    std::vector<std::string> tableSections = tsv.sections("table");
    for(auto it = tableSections.begin(); it != tableSections.end(); ++it)
    {
      openTable(*it, fileVersion);
    }
  }

  if(tsv.hasSection("TableStatistics"))
  {
    std::vector<std::string> tableStatsSections = tsv.sections("TableStatistics");
    for(auto it = tableStatsSections.begin(); it != tableStatsSections.end(); ++it)
    {
      openTableStatistics(*it, fileVersion);
    }
  }

  if(tsv.hasSection("matrix"))
  {
    std::vector<std::string> matrixSections = tsv.sections("matrix");
    for(auto it = matrixSections.begin(); it != matrixSections.end(); ++it)
    {
      openMatrix(*it, fileVersion);
    }
  }

  if(tsv.hasSection("multiLayer"))
  {
    std::vector<std::string> multiLayer = tsv.sections("multiLayer");
    for(auto it = multiLayer.begin(); it != multiLayer.end(); ++it)
    {
      openMultiLayer(*it, fileVersion);
    }
  }

  if(tsv.hasSection("SurfacePlot"))
  {
    std::vector<std::string> plotSections = tsv.sections("SurfacePlot");
    for(auto it = plotSections.begin(); it != plotSections.end(); ++it)
    {
      openSurfacePlot(*it, fileVersion);
    }
  }

  if(tsv.hasSection("log"))
  {
    std::vector<std::string> logSections = tsv.sections("log");
    for(auto it = logSections.begin(); it != logSections.end(); ++it)
    {
      currentFolder()->appendLogInfo(QString::fromStdString(*it));
    }
  }

  if(tsv.hasSection("note"))
  {
    std::vector<std::string> noteSections = tsv.sections("note");
    for(auto it = noteSections.begin(); it != noteSections.end(); ++it)
    {
      Note* n = newNote("");
      n->loadFromProject(*it, this, fileVersion);
    }
  }

  if(tsv.hasSection("scriptwindow"))
  {
    std::vector<std::string> scriptSections = tsv.sections("scriptwindow");
    for(auto it = scriptSections.begin(); it != scriptSections.end(); ++it)
    {
      TSVSerialiser sTSV(*it);
      QStringList files;

      auto scriptNames = sTSV.values("ScriptNames");
      //Iterate, ignoring scriptNames[0] which is just "ScriptNames"
      for(size_t i = 1; i < scriptNames.size(); ++i)
        files.append(QString::fromStdString(scriptNames[i]));
      openScriptWindow(files);
    }
  }

  if(tsv.hasSection("instrumentwindow"))
  {
    std::vector<std::string> instrumentSections = tsv.sections("instrumentwindow");
    for(auto it = instrumentSections.begin(); it != instrumentSections.end(); ++it)
    {
      TSVSerialiser iws(*it);
      if(iws.selectLine("WorkspaceName"))
      {
        std::string wsName = iws.asString(1);
        InstrumentWindow* iw = mantidUI->getInstrumentView(QString::fromStdString(wsName));
        if(iw)
          iw->loadFromProject(*it, this, fileVersion);
      }
    }
  }

  //Deal with subfolders last.
  if(tsv.hasSection("folder"))
  {
    std::vector<std::string> folders = tsv.sections("folder");
    for(auto it = folders.begin(); it != folders.end(); ++it)
    {
      openProjectFolder(*it, fileVersion);
    }
  }


  //We're returning to our parent folder, so set d_current_folder to our parent
  Folder *parent = dynamic_cast<Folder*>(currentFolder()->parent());
  if (!parent)
    d_current_folder = projectFolder();
  else
    d_current_folder = parent;
}

bool ApplicationWindow::setScriptingLanguage(const QString &lang)
{
  if ( lang.isEmpty() ) return false;
  if( scriptingEnv() && lang == scriptingEnv()->name() ) return true;

  if( m_bad_script_envs.contains(lang) )
  {
    using MantidQt::API::Message;
    writeToLogWindow(Message("Previous initialization of " + lang + " failed, cannot retry.",Message::Priority::PRIO_ERROR));
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
    connect(newEnv, SIGNAL(print(const QString&)), resultsLog, SLOT(appendNotice(const QString&)));

    if( newEnv->initialize() )
    {
      m_script_envs.insert(lang, newEnv);
    }
    else
    {
      delete newEnv;
      m_bad_script_envs.insert(lang);
      QMessageBox::information(this, "MantidPlot", QString("Failed to initialize ") + lang + ". Please contact support.");
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
  if( scriptingWindow->isExecuting() )
  {
    QMessageBox msg_box;
    msg_box.setText("Cannot change scripting language, a script is still running.");
    msg_box.exec();
    return;
  }
  ScriptingLangDialog* d = new ScriptingLangDialog(scriptingEnv(), this);
  d->exec();
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
  recentFiles = settings.value("/RecentFiles").toStringList();
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

  if (!recentFiles.isEmpty() && recentFiles[0].contains("^e"))
    recentFiles = recentFiles[0].split("^e", QString::SkipEmptyParts);
  else if (recentFiles.count() == 1){
    QString s = recentFiles[0];
    if (s.remove(QRegExp("\\s")).isEmpty())
      recentFiles = QStringList();
  }
#endif

  updateRecentProjectsList();
  updateRecentFilesList();

  changeAppStyle(settings.value("/Style", appStyle).toString());
  autoSave = settings.value("/AutoSave", false).toBool();
  autoSaveTime = settings.value("/AutoSaveTime",15).toInt();
  //set logging level to the last saved level
  int lastLoggingLevel = settings.value("/LastLoggingLevel", Mantid::Kernel::Logger::Priority::PRIO_NOTICE).toInt();
  Mantid::Kernel::Logger::setLevelForAll(lastLoggingLevel);

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
  //Once only for each Qsettings instance set all of the confirmations to false - they are annoying
  //however if people consciously turn them back on then leave them alone.
  //leaving renameTable out of this as it is bit different
  bool setConfirmationDefaultsToFalseOnce= settings.value("/DefaultsSetToFalseOnce", false).toBool();
  if (!setConfirmationDefaultsToFalseOnce)
  {
    settings.setValue("/Folder", false);
    settings.setValue("/Table", false);
    settings.setValue("/Matrix", false);
    settings.setValue("/Plot2D", false);
    settings.setValue("/Plot3D", false);
    settings.setValue("/Note", false);
    settings.setValue("/InstrumentWindow", false);
    settings.setValue("/DefaultsSetToFalseOnce", true);
  }
  confirmCloseFolder = settings.value("/Folder", false).toBool();
  confirmCloseTable = settings.value("/Table", false).toBool();
  confirmCloseMatrix = settings.value("/Matrix", false).toBool();
  confirmClosePlot2D = settings.value("/Plot2D", false).toBool();
  confirmClosePlot3D = settings.value("/Plot3D", false).toBool();
  confirmCloseNotes = settings.value("/Note", false).toBool();
  d_inform_delete_workspace = settings.value("/DeleteWorkspace", true).toBool();
  d_inform_rename_table = settings.value("/RenameTable", false).toBool();
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

  // Transform from the old setting for plot defaults, will only happen once.
  if ( !settings.contains("/UpdateForPlotImprovements1") )
  {
    settings.writeEntry("/UpdateForPlotImprovements1","true");
    settings.beginGroup("/General");

    settings.writeEntry("/Antialiasing","true");

    //enable right and top axes without labels
    settings.beginWriteArray("EnabledAxes");
    int i=1;
    settings.setArrayIndex(i);
    settings.writeEntry("enabled", "true");
    settings.writeEntry("labels", "false");
    i=3;
    settings.setArrayIndex(i);
    settings.writeEntry("enabled", "true");
    settings.writeEntry("labels", "false");
    settings.endArray();
    settings.endGroup();

    //ticks should be in
    settings.beginGroup("/Ticks");
    settings.writeEntry("/MajTicksStyle", ScaleDraw::In);
    settings.writeEntry("/MinTicksStyle", ScaleDraw::In);
    settings.endGroup();

    //legend to opaque
    settings.beginGroup("/Legend");
    settings.writeEntry("/Transparency", 255);
    settings.endGroup(); // Legend
  }
    // Transform from the old setting for plot defaults, will only happen once.
  if ( !settings.contains("/UpdateForPlotImprovements2") )
  {
    settings.writeEntry("/UpdateForPlotImprovements2","true");
    settings.beginGroup("/General");

    //turn axes backbones off as these rarely join at the corners
    settings.writeEntry("/AxesBackbones","false");

    settings.writeEntry("/CanvasFrameWidth","1");
    settings.endGroup();
  }

  settings.beginGroup("/General");
  titleOn = settings.value("/Title", true).toBool();
  autoDistribution1D = settings.value("/AutoDistribution1D", true).toBool();
  canvasFrameWidth = settings.value("/CanvasFrameWidth", 0).toInt();
  defaultPlotMargin = settings.value("/Margin", 0).toInt();
  drawBackbones = settings.value("/AxesBackbones", true).toBool();
  d_axes_scales[0] = settings.value("/AxisYScale", "linear").toString();
  d_axes_scales[1] = settings.value("/AxisZScale", "linear").toString();
  d_axes_scales[2] = settings.value("/AxisXScale", "linear").toString();
  d_axes_scales[3] = settings.value("/AxisTScale", "linear").toString();
  axesLineWidth = settings.value("/AxesLineWidth", 1).toInt();
  autoscale2DPlots = settings.value("/Autoscale", true).toBool();
  autoScaleFonts = settings.value("/AutoScaleFonts", true).toBool();
  autoResizeLayers = settings.value("/AutoResizeLayers", true).toBool();

  antialiasing2DPlots = settings.value("/Antialiasing", false).toBool(); //Mantid
  fixedAspectRatio2DPlots = settings.value("/FixedAspectRatio2DPlots", false).toBool(); //Mantid
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
  d_graph_axes_labels_dist = settings.value("/LabelsAxesDist", d_graph_axes_labels_dist).toInt();
  d_graph_tick_labels_dist = settings.value("/TickLabelsDist", d_graph_tick_labels_dist).toInt();
  // Transform from the old setting for controlling visible axes. Will only happen once, after which it's deleted.
  if ( settings.contains("/AllAxes") )
  {
    if ( settings.value("/AllAxes").toBool() )
    {
      d_show_axes = QVector<bool> (QwtPlot::axisCnt, true);
    }
    settings.remove("/AllAxes");
  }
  else
  {
    int size = settings.beginReadArray("EnabledAxes");
    for (int i = 0; i < size; ++i) {
      settings.setArrayIndex(i);
      d_show_axes[i] = settings.value("enabled", true).toBool();
      d_show_axes_labels[i] = settings.value("labels", true).toBool();
    }
    settings.endArray();
  }
  d_synchronize_graph_scales = settings.value("/SynchronizeScales", d_synchronize_graph_scales).toBool();
  settings.endGroup(); // General

  settings.beginGroup("/Curves");
  defaultCurveStyle = settings.value("/Style", Graph::LineSymbols).toInt();
  defaultCurveLineWidth = settings.value("/LineWidth", 1).toDouble();
  defaultSymbolSize = settings.value("/SymbolSize", 3).toInt();
  applyCurveStyleToMantid = settings.value("/ApplyMantid", true).toBool();
  drawAllErrors = settings.value("/DrawAllErrors", false).toBool();
  settings.endGroup(); // Curves

  settings.beginGroup("/Ticks");
  majTicksStyle = settings.value("/MajTicksStyle", ScaleDraw::In).toInt();
  minTicksStyle = settings.value("/MinTicksStyle", ScaleDraw::In).toInt();
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
  m_enableQtiPlotFitting = settings.value("/EnableQtiPlotFitting", false).toBool();
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
  settings.endGroup(); // Import ASCII

  settings.beginGroup("/ExportASCII");
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
  d_script_win_pos = settings.value("/pos", QPoint(250,200)).toPoint();
  if( d_script_win_pos.x() < 0 || d_script_win_pos.y() < 0 ) d_script_win_pos = QPoint(250,200);
  d_script_win_size = settings.value("/size", QSize(600,660)).toSize();
  if( !d_script_win_size.isValid() ) d_script_win_size = QSize(600,660);
  settings.endGroup();

  settings.beginGroup("/ToolBars");
  d_standard_tool_bar = settings.value("/FileToolBar", true).toBool();
  d_edit_tool_bar = settings.value("/EditToolBar", true).toBool();
  d_column_tool_bar = settings.value("/ColumnToolBar", true).toBool();
  d_matrix_tool_bar = settings.value("/MatrixToolBar", true).toBool();
  d_plot_tool_bar = settings.value("/PlotToolBar", true).toBool();
  d_display_tool_bar = settings.value("/DisplayToolBar", false).toBool();
  d_format_tool_bar = settings.value("/FormatToolBar", true).toBool();
  settings.endGroup();

  //---------------------------------
  // Mantid

  bool warning_shown = settings.value("/DuplicationDialogShown", false).toBool();

  //Check for user defined scripts in settings and create menus for them
  //Top level scripts group
  settings.beginGroup("CustomScripts");

  MantidQt::API::InterfaceManager interfaceManager;

  // Reference list of custom Interfaces that will be added to the Interfaces menu
  QStringList user_windows =interfaceManager.getUserSubWindowKeys();
  // List it user items that will be moved to the Interfaces menu
  QStringList duplicated_custom_menu = QStringList();

  foreach(QString menu, settings.childGroups())
  {
    // Specifically disallow the use of the Interfaces menu to users looking to
    // customise their own menus, since it is managed separately.  Also, there
    // may well be some left-over QSettings values from previous installations
    // that we do not want used.
    if( menu == "Interfaces" || menu == "&Interfaces" )
      continue;

    addUserMenu(menu);
    settings.beginGroup(menu);
    foreach(QString keyName, settings.childKeys())
    {
      QFileInfo fi(settings.value(keyName).toString());
      QString baseName = fi.fileName();
      const QStringList pyQtInterfaces = m_interfaceCategories.keys();
      if (pyQtInterfaces.contains(baseName))
        continue;

      if ( user_windows.grep(keyName).size() > 0 || pyQtInterfaces.grep(keyName).size() > 0 )
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
  settings.setValue("/RecentFiles", recentFiles);
  settings.setValue("/Style", appStyle);
  settings.setValue("/AutoSave", autoSave);
  settings.setValue("/AutoSaveTime", autoSaveTime);
  //save current logger level from the root logger ""
  int lastLoggingLevel = Mantid::Kernel::Logger("").getLevel();
  settings.setValue("/LastLoggingLevel", lastLoggingLevel);

  settings.setValue("/BackupProjects", d_backup_files);
  settings.setValue("/InitWindow", static_cast<int>(d_init_window_type));

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
  settings.setValue("/DeleteWorkspace",d_inform_delete_workspace);
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
  settings.setValue("/AutoDistribution1D", autoDistribution1D);
  settings.setValue("/CanvasFrameWidth", canvasFrameWidth);
  settings.setValue("/Margin", defaultPlotMargin);
  settings.setValue("/AxesBackbones", drawBackbones);
  settings.setValue("/AxisYScale", d_axes_scales[0]);
  settings.setValue("/AxisZScale", d_axes_scales[1]);
  settings.setValue("/AxisXScale", d_axes_scales[2]);
  settings.setValue("/AxisTScale", d_axes_scales[3]);
  settings.setValue("/AxesLineWidth", axesLineWidth);
  settings.setValue("/Autoscale", autoscale2DPlots);
  settings.setValue("/AutoScaleFonts", autoScaleFonts);
  settings.setValue("/AutoResizeLayers", autoResizeLayers);
  settings.setValue("/Antialiasing", antialiasing2DPlots);
  settings.setValue("/FixedAspectRatio2DPlots", fixedAspectRatio2DPlots);

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
  settings.setValue("/LabelsAxesDist", d_graph_axes_labels_dist);
  settings.setValue("/TickLabelsDist", d_graph_tick_labels_dist);
  settings.beginWriteArray("EnabledAxes");
  for (int i = 0; i < QwtPlot::axisCnt; ++i) {
    settings.setArrayIndex(i);
    settings.setValue("axis", i);
    settings.setValue("enabled", d_show_axes[i]);
    settings.setValue("labels", d_show_axes_labels[i]);
  }
  settings.endArray();
  settings.setValue("/SynchronizeScales", d_synchronize_graph_scales);
  settings.endGroup(); // General

  settings.beginGroup("/Curves");
  settings.setValue("/Style", defaultCurveStyle);
  settings.setValue("/LineWidth", defaultCurveLineWidth);
  settings.setValue("/SymbolSize", defaultSymbolSize);
  settings.setValue("/ApplyMantid", applyCurveStyleToMantid);
  settings.setValue("/DrawAllErrors", drawAllErrors);
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
  settings.setValue("/EnableQtiPlotFitting", m_enableQtiPlotFitting);
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
  settings.endGroup(); // ImportASCII

  settings.beginGroup("/ExportASCII");
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

  settings.beginGroup("/ScriptWindow");
  // Geometry is applied by the app window
  settings.setValue("/size", d_script_win_size);
  settings.setValue("/pos", d_script_win_pos);
  settings.endGroup();

  settings.beginGroup("/ToolBars");
  settings.setValue("/FileToolBar", d_standard_tool_bar);
  settings.setValue("/EditToolBar", d_edit_tool_bar);
  settings.setValue("/ColumnToolBar", d_column_tool_bar);
  settings.setValue("/MatrixToolBar", d_matrix_tool_bar);
  settings.setValue("/PlotToolBar", d_plot_tool_bar);
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
    plot2D = dynamic_cast<MultiLayer*>(w);
    if (plot2D->isEmpty()){
      QMessageBox::critical(this, tr("MantidPlot - Export Error"),//Mantid
          tr("<h4>There are no plot layers available in this window!</h4>"));
      return;
    }
  } else if (w->isA("Graph3D"))
    plot3D = dynamic_cast<Graph3D*>(w);
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

  Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
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
      plot2D = dynamic_cast<MultiLayer*>(w);
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
      plot3D = dynamic_cast<Graph3D*>(w);
    } else
      continue;

    QString file_name = output_dir + "/" + w->objectName() + file_suffix;
    QFile f(file_name);
    if (f.exists() && confirm_overwrite) {
      QApplication::restoreOverrideCursor();

      QString msg = tr("A file called: <p><b>%1</b><p>already exists. ""Do you want to overwrite it?").arg(file_name);
      QMessageBox msgBox(QMessageBox::Question, tr("MantidPlot - Overwrite file?"), msg,//Mantid
          QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel,
          dynamic_cast<ApplicationWindow *>(this));
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

std::string ApplicationWindow::windowGeometryInfo(MdiSubWindow *w)
{
  TSVSerialiser tsv;
  tsv.writeLine("geometry");
  if(w->status() == MdiSubWindow::Maximized)
  {
    tsv << "maximized";

    if(w == activeWindow())
      tsv << "active";

    return tsv.outputLines();
  }

  int x = w->x();
  int y = w->y();

  QWidget* wrapper = w->getWrapperWindow();
  if(wrapper)
  {
    x = wrapper->x();
    y = wrapper->y();
    if(w->getFloatingWindow())
    {
      QPoint pos = QPoint(x,y) - mdiAreaTopLeft();
      x = pos.x();
      y = pos.y();
    }
  }

  tsv << x << y;
  if(w->status() != MdiSubWindow::Minimized)
    tsv << w->width() << w->height();
  else
    tsv << w->minRestoreSize().width() << w->minRestoreSize().height() << "minimized";

  if(hidden(w))
    tsv << "hidden";
  else if(w == activeWindow())
    tsv << "active";

  return tsv.outputLines();
}

void ApplicationWindow::restoreWindowGeometry(ApplicationWindow *app, MdiSubWindow *w, const QString& s)
{
  if(!w)
    return;

  QString caption = w->objectName();

  if(s.contains("maximized"))
  {
    w->setStatus(MdiSubWindow::Maximized);
    app->setListView(caption, tr("Maximized"));
  }
  else
  {
    QStringList lst = s.split("\t");
    if(lst.count() > 4)
    {
      int x = lst[1].toInt();
      int y = lst[2].toInt();
      int width = lst[3].toInt();
      int height = lst[4].toInt();
      w->resize(width, height);
      w->move(x, y);
    }

    if(s.contains("minimized"))
    {
      w->setStatus(MdiSubWindow::Minimized);
      app->setListView(caption, tr("Minimized"));
    }
    else
    {
      w->setStatus(MdiSubWindow::Normal);
      if(lst.count() > 5 && lst[5] == "hidden")
        app->hideWindow(w);
    }
  }
  if(s.contains("active"))
    setActiveWindow(w);
}

Folder* ApplicationWindow::projectFolder() const
{
  return dynamic_cast<FolderListItem*>(folders->firstChild())->folder();
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

  saveProjectFile(projectFolder(), projectname, compress);

  setWindowTitle("MantidPlot - " + projectname);
  savedProject();

  if (autoSave){
    if (savingTimerId)
      killTimer(savingTimerId);
    savingTimerId=startTimer(autoSaveTime*60000);
  } else
    savingTimerId=0;

  // Back-up file to be removed because file has successfully saved.
  QFile::remove(projectname + "~");

  QApplication::restoreOverrideCursor();
  return true;
}

void ApplicationWindow::savetoNexusFile()
{
  QString filter = tr("Mantid Files")+" (*.nxs *.nx5 *.xml);;";
  QString selectedFilter;
  QString fileDir = MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString fileName =
    MantidQt::API::FileDialogHandler::getSaveFileName(this, tr("Save File As"), fileDir, filter, &selectedFilter);
  if ( !fileName.isEmpty() )
  {
    std::string wsName;
    MdiSubWindow *w = activeWindow();
    if(w)
    {
      if(w->isA("MantidMatrix"))
      {
        wsName=dynamic_cast<MantidMatrix*>(w)->getWorkspaceName();

      }
      else if(w->isA("MantidTable"))
      {
        wsName=dynamic_cast<MantidTable*>(w)->getWorkspaceName();
      }
      else
      {
        throw std::runtime_error("Invalid input for SaveNexus, you cannot save this type of object as a NeXus file");
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
    updateRecentFilesList(fileName);
  }
}

void ApplicationWindow::loadDataFile()
{
  // Ask user for file
  QString fn = QFileDialog::getOpenFileName( 0, tr("Mantidplot - Open file to load"), AlgorithmInputHistory::Instance().getPreviousDirectory());
  if(fn != "") {
    loadDataFileByName(fn);
  }
  saveSettings();   // save new list of recent files
}

void ApplicationWindow::loadDataFileByName(QString fn)
{
  QFileInfo fnInfo(fn);
  AlgorithmInputHistory::Instance().setPreviousDirectory(fnInfo.absoluteDir().path());
  if( fnInfo.suffix() == "py")
  {
    // We have a python file, just load it into script window
    loadScript( fn, true );
  }
  else if(mantidUI)
  {
    // Run Load algorithm on file
    QHash<QString,QString> params;
    params["Filename"] = fn;
    mantidUI->showAlgorithmDialog(QString("Load"),params);
  }
}

void ApplicationWindow::saveProjectAs(const QString& fileName, bool compress)
{
  QString fn = fileName;
  if (fileName.isEmpty()){
    QString filter = tr("MantidPlot project")+" (*.mantid);;"; //tr("QtiPlot project")+" (*.qti);;";
    filter += tr("Compressed MantidPlot project")+" (*.mantid.gz)";

    QString selectedFilter;
    fn = MantidQt::API::FileDialogHandler::getSaveFileName(this, tr("Save Project As"), workingDir, filter, &selectedFilter);
    if (selectedFilter.contains(".gz"))
      compress = true;
  }

  if ( !fn.isEmpty() )
  {
    // Check if exists. If not, create directory first.
    QFileInfo tempFile(fn);
    if(!tempFile.exists())
    {
      // Make the directory
      QString dir(fn);
      if (fn.contains('.'))
        dir = fn.left(fn.find('.'));
      QDir().mkdir(dir);

      // Get the file name
      QString file("temp");
      for(int i=0; i<dir.size(); ++i)
      {
        if(dir[i] == '/')
          file = dir.right(dir.size() - i);
        else if(dir[i] == '\\')
          file = dir.right(i);
      }
      fn = dir + file;
    }

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
      FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
      item->setText(0, baseName);
      item->folder()->setObjectName(baseName);
    }
  }
}

void ApplicationWindow::saveNoteAs()
{
  Note* w = dynamic_cast<Note*>(activeWindow(NoteWindow));
  if (!w)
    return;
  w->exportASCII();
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
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
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

  MdiSubWindow *w = dynamic_cast<WindowListItem*>(item)->window();
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

    Table *t = dynamic_cast<Table*>(w);
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

  if (dynamic_cast<MultiLayer*>(w)->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Error"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
  if (!g)
    return;

  if (g->isPiePlot()){
    QMessageBox::warning(this,tr("MantidPlot - Error"),//Mantid
        tr("This functionality is not available for pie plots!"));
  } else {
    CurvesDialog* crvDialog = new CurvesDialog(this,g);
    crvDialog->setAttribute(Qt::WA_DeleteOnClose);
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

  Graph *g = dynamic_cast<MultiLayer*>(w)->activeGraph();
  if (!g)
    return 0;

  AssociationsDialog* ad = new AssociationsDialog(g);
  ad->setAttribute(Qt::WA_DeleteOnClose);
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
    Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
    if (g){
      TextDialog* td= new TextDialog(TextDialog::LayerTitle, this,0);
      td->setGraph(g);
      td->exec();
    }
  } else if (w->isA("Graph3D")) {
    Plot3DDialog* pd = dynamic_cast<Plot3DDialog*>(showPlot3dDialog());
    if (pd)
      pd->showTitleTab();
  }
}

void ApplicationWindow::showAxisTitleDialog()
{
  MdiSubWindow* w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
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
    ed->setColumnSeparator(columnSeparator);
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
              success = (dynamic_cast<Table*>(w))->exportASCII(fileName, sep, colNames, colComments, expSelection);
            else if (w->isA("Matrix"))
              success = (dynamic_cast<Matrix*>(w))->exportASCII(fileName, sep, expSelection);
            break;

          case 1:
            confirmOverwrite = false;
            if (w->inherits("Table"))
              success = (dynamic_cast<Table*>(w))->exportASCII(fileName, sep, colNames, colComments, expSelection);
            else if (w->isA("Matrix"))
              success = (dynamic_cast<Matrix*>(w))->exportASCII(fileName, sep, expSelection);
            break;

          case 2:
            return;
            break;
          }
        } else if (w->inherits("Table"))
          success = (dynamic_cast<Table*>(w))->exportASCII(fileName, sep, colNames, colComments, expSelection);
        else if (w->isA("Matrix"))
          success = (dynamic_cast<Matrix*>(w))->exportASCII(fileName, sep, expSelection);

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
  QString fname = MantidQt::API::FileDialogHandler::getSaveFileName(this, tr("Choose a filename to save under"),
      asciiDirPath + "/" + w->objectName(), "*.txt;;*.dat;;*.DAT", &selectedFilter);
  if (!fname.isEmpty() ){
    QFileInfo fi(fname);
    QString baseName = fi.fileName();
    if (baseName.contains(".")==0)
      fname.append(selectedFilter.remove("*"));

    asciiDirPath = fi.dirPath(true);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if (w->inherits("Table"))
      dynamic_cast<Table*>(w)->exportASCII(fname, sep, colNames, colComments, expSelection);
    else if (w->isA("Matrix"))
      (dynamic_cast<Matrix*>(w))->exportASCII(fname, sep, expSelection);
    else if (w->isA("MantidMatrix"))
    {
      //call save ascii
      try
      {
        Mantid::API::IAlgorithm_sptr alg =mantidUI->createAlgorithm("SaveAscii");
        alg->setPropertyValue("Filename",fname.toStdString());
        alg->setPropertyValue("InputWorkspace",tableName.toStdString());
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
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *w = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!w)
    return;

  if (w->selectedColumns().count()>0 || w->table()->currentSelection() >= 0){
    SetColValuesDialog* vd = new SetColValuesDialog(scriptingEnv(), w);
    vd->setAttribute(Qt::WA_DeleteOnClose);
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
    (dynamic_cast<Table*>(w))->calculate();
  else if (w->isA("Matrix"))
    (dynamic_cast<Matrix*>(w))->calculate();
}

void ApplicationWindow::sortActiveTable()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  t->sortTableDialog();
}

void ApplicationWindow::sortSelection()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  t->sortColumnsDialog();
}

void ApplicationWindow::normalizeActiveTable()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count())>0)
    t->normalize();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::normalizeSelection()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count())>0)
    t->normalizeSelection();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"), tr("Please select a column first!"));//Mantid
}

void ApplicationWindow::correlate()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count()) > 0)
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
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
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
  Table *w = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!w)
    return;

  bool isSortable = w->isSortable();
  bool isFixedColumns = w->isFixedColumns();
  bool isEditable = w->isEditable();

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
    stat.insertSeparator();
    stat.addAction(actionStemPlot);
    stat.setTitle(tr("Statistical &Graphs"));
    plot.addMenu(&stat);

    plot.setTitle(tr("&Plot"));
    contextMenu.addMenu(&plot);
    contextMenu.insertSeparator();

    if (isEditable) contextMenu.addAction(QIcon(getQPixmap("cut_xpm")),tr("Cu&t"), w, SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")),tr("&Copy"), w, SLOT(copySelection()));
    if (isEditable) contextMenu.addAction(QIcon(getQPixmap("paste_xpm")),tr("Past&e"), w, SLOT(pasteSelection()));
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
      if (isEditable) contextMenu.insertSeparator();

      if (isEditable) contextMenu.addAction(actionShowColumnValuesDialog);
      if (isEditable) contextMenu.addAction(actionTableRecalculate);
      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Column With"));
      if (isEditable) contextMenu.addMenu(&fill);

      norm.addAction(tr("&Column"), w, SLOT(normalizeSelection()));
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      if (isEditable) contextMenu.addMenu(& norm);

      contextMenu.insertSeparator();
      contextMenu.addAction(actionShowColStatistics);

      contextMenu.insertSeparator();

      if (isEditable) contextMenu.addAction(QIcon(getQPixmap("erase_xpm")), tr("Clea&r"), w, SLOT(clearSelection()));
      if (!isFixedColumns) contextMenu.addAction(QIcon(getQPixmap("delete_column_xpm")), tr("&Delete"), w, SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.insertSeparator();
      if (!isFixedColumns) contextMenu.addAction(getQPixmap("insert_column_xpm"), tr("&Insert"), w, SLOT(insertCol()));
      if (!isFixedColumns) contextMenu.addAction(actionAddColToTable);
      contextMenu.insertSeparator();

      sorting.addAction(QIcon(getQPixmap("sort_ascending_xpm")), tr("&Ascending"), w, SLOT(sortColAsc()));
      sorting.addAction(QIcon(getQPixmap("sort_descending_xpm")), tr("&Descending"), w, SLOT(sortColDesc()));

      sorting.setTitle(tr("Sort Colu&mn"));
      if (isSortable) contextMenu.addMenu(&sorting);

      if (isSortable) contextMenu.addAction(actionSortTable);
    }

    contextMenu.insertSeparator();
    contextMenu.addAction(actionShowColumnOptionsDialog);
  }
  else if ((int)w->selectedColumns().count()>1)
  {
    plot.addAction(QIcon(getQPixmap("lPlot_xpm")),tr("&Line"), this, SLOT(plotL()));
    plot.addAction(QIcon(getQPixmap("pPlot_xpm")),tr("&Scatter"), this, SLOT(plotP()));
    plot.addAction(QIcon(getQPixmap("lpPlot_xpm")),tr("Line + S&ymbol"), this,SLOT(plotLP()));

    specialPlot.addAction(actionWaterfallPlot);
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
    stat.insertSeparator();
    stat.addAction(actionStemPlot);
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
    if (isEditable) contextMenu.addAction(QIcon(getQPixmap("cut_xpm")),tr("Cu&t"), w, SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")),tr("&Copy"), w, SLOT(copySelection()));
    if (isEditable) contextMenu.addAction(QIcon(getQPixmap("paste_xpm")),tr("Past&e"), w, SLOT(pasteSelection()));
    contextMenu.insertSeparator();

    if (w){
      if (isEditable) contextMenu.addAction(QIcon(getQPixmap("erase_xpm")),tr("Clea&r"), w, SLOT(clearSelection()));
      if (isEditable) contextMenu.addAction(QIcon(getQPixmap("close_xpm")),tr("&Delete"), w, SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.insertSeparator();
      if (isEditable) contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
      if (isEditable) contextMenu.addAction(actionAddColToTable);
      if (isEditable) contextMenu.insertSeparator();
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
      if (isEditable) contextMenu.insertSeparator();

      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Columns With"));
      if (isEditable) contextMenu.addMenu(&fill);

      norm.addAction(actionNormalizeSelection);
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      if (isEditable) contextMenu.addMenu(&norm);

      if (isSortable) contextMenu.insertSeparator();
      if (isSortable) contextMenu.addAction(actionSortSelection);
      if (isSortable) contextMenu.addAction(actionSortTable);
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
  Matrix *m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixDialog* md = new MatrixDialog(this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix (m);
  md->exec();
}

void ApplicationWindow::showMatrixSizeDialog()
{
  Matrix *m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixSizeDialog* md = new MatrixSizeDialog(m, this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->exec();
}

void ApplicationWindow::showMatrixValuesDialog()
{
  Matrix *m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixValuesDialog* md = new MatrixValuesDialog(scriptingEnv(), this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix(m);
  md->exec();
}

void ApplicationWindow::showColumnOptionsDialog()
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  if(t->selectedColumns().count()>0) {
    TableDialog* td = new TableDialog(t);
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

  if (plot->isA("MultiLayer") && dynamic_cast<MultiLayer*>(plot)->layers())
    showPlotDialog();
  else if (plot->isA("Graph3D")){
    QDialog* gd = showScaleDialog();
    dynamic_cast<Plot3DDialog*>(gd)->showGeneralTab();
  }
}

void ApplicationWindow::showAxisDialog()
{
  MdiSubWindow* plot = activeWindow();
  if (!plot)
    return;

  QDialog* gd = showScaleDialog();
  if (gd && plot->isA("MultiLayer") && dynamic_cast<MultiLayer*>(plot)->layers())
    dynamic_cast<AxesDialog*>(gd)->showAxesPage();
  else if (gd && plot->isA("Graph3D"))
    dynamic_cast<Plot3DDialog*>(gd)->showAxisTab();
}

void ApplicationWindow::showGridDialog()
{
  AxesDialog* gd = dynamic_cast<AxesDialog*>(showScaleDialog());
  if (gd)
    gd->showGridPage();
}

QDialog* ApplicationWindow::showScaleDialog()
{
  MdiSubWindow *w = activeWindow();
  if (!w)
    return 0;

  if (w->isA("MultiLayer")){
    if (dynamic_cast<MultiLayer*>(w)->isEmpty())
      return 0;

    Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
    if (g->isPiePlot()){
      QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("This functionality is not available for pie plots!"));//Mantid
      return 0;
    }

    AxesDialog* ad = new AxesDialog(this,g);
    ad->exec();
    return ad;
  } else if (w->isA("Graph3D"))
    return showPlot3dDialog();

  return 0;
}

AxesDialog* ApplicationWindow::showScalePageFromAxisDialog(int axisPos)
{
  AxesDialog* gd = dynamic_cast<AxesDialog*>(showScaleDialog());
  if (gd)
    gd->setCurrentScale(axisPos);

  return gd;
}

AxesDialog* ApplicationWindow::showAxisPageFromAxisDialog(int axisPos)
{
  AxesDialog* gd = dynamic_cast<AxesDialog*>(showScaleDialog());
  if (gd){
    gd->showAxesPage();
    gd->setCurrentScale(axisPos);
  }
  return gd;
}

QDialog* ApplicationWindow::showPlot3dDialog()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  PlotDialog* pd = new PlotDialog(d_extended_plot_dialog, this,w);
  pd->setAttribute(Qt::WA_DeleteOnClose);
  pd->insertColumnsList(columnsList(Table::All));
  //pd->setMultiLayer(w);
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  DataCurve *c = dynamic_cast<DataCurve *>(g->curve(g->curveIndex(curveKey)));
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
          connect(act, SIGNAL(activated()), dynamic_cast<RangeSelectorTool *>(g->activeTool()), SLOT(setCurveRange()));
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  QwtPlotItem *it = g->plotItem(curveIndex);
  if (!it)
    return;

  if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
  {
    Spectrogram *sp = dynamic_cast<Spectrogram*>(it);
    if (sp->matrix())
      sp->matrix()->showMaximized();
  }
  else if (dynamic_cast<PlotCurve*>(it)->type() == Graph::Function)
  {
    g->createTable(dynamic_cast<PlotCurve*>(it));
  }
  else
  {
    showTable(it->title().text());
  }
}

void ApplicationWindow::showCurveWorksheet()
{
  MultiLayer *w = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  if (dynamic_cast<Graph*>(plot->activeGraph())->isPiePlot())
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty() || dynamic_cast<Graph*>(plot->activeGraph())->isPiePlot())
    return;

  (dynamic_cast<Graph*>(plot->activeGraph()))->zoomOut();
  btnPointer->setOn(true);
}

void ApplicationWindow::setAutoScale()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph*>(plot->activeGraph());
  if (g)
    g->setAutoScale();
}

void ApplicationWindow::removePoints()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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

  if (w->isA("MultiLayer") && (dynamic_cast<MultiLayer*>(w))->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  QString fname = MantidQt::API::FileDialogHandler::getSaveFileName(this, tr("Choose a filename to save under"), workingDir, "*.pdf");
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

  if (w->isA("MultiLayer") && (dynamic_cast<MultiLayer*>(w))->isEmpty()){
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
        dynamic_cast<MultiLayer*>(w)->printAllLayers(paint);
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
    plot = dynamic_cast<MultiLayer*>(w);
  else if(w->inherits("Table"))
    plot = multilayerPlot(dynamic_cast<Table*>(w), dynamic_cast<Table*>(w)->drawableColumnSelection(), Graph::LineSymbols);

  if (!plot)
    return;

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
    Graph* g = dynamic_cast<MultiLayer*>(w)->activeGraph();
    if ( g && g->validCurvesDataSize() ){
      sd = new FFTDialog(FFTDialog::onGraph, this);
      sd->setAttribute(Qt::WA_DeleteOnClose);
      sd->setGraph(g);
    }
  } else if (w->inherits("Table")) {
    sd = new FFTDialog(FFTDialog::onTable, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setTable(dynamic_cast<Table*>(w));
  } else if (w->inherits("Matrix")) {
    sd = new FFTDialog(FFTDialog::onMatrix, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setMatrix(dynamic_cast<Matrix*>(w));
  }

  if (sd)
    sd->exec();
}

void ApplicationWindow::showSmoothDialog(int m)
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
    currentFolder()->appendLogInfo(result);
    showResults(true);
    emit modified();
  }
}

void ApplicationWindow::showIntegrationDialog()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  IntDialog *id = new IntDialog(this, g);
  id->exec();
}

void ApplicationWindow::showResults(bool ok)
{
  if (ok)
  {
    QString text;
    if (!currentFolder()->logInfo().isEmpty()) text = currentFolder()->logInfo();
    else text = "Sorry, there are no results to display!";
    using MantidQt::API::Message;
    resultsLog->replace(Message(text, Message::Priority::PRIO_INFORMATION));
  }
  logWindow->setVisible(ok);
}

void ApplicationWindow::showResults(const QString& s, bool ok)
{
  currentFolder()->appendLogInfo(s);
  QString logInfo = currentFolder()->logInfo();
  if (!logInfo.isEmpty()) {
    using MantidQt::API::Message;
    resultsLog->replace(Message(logInfo, Message::Priority::PRIO_INFORMATION));
  }
  showResults(ok);
}

void ApplicationWindow::showScreenReader()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no plot layers available in this window!"));//Mantid
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if (dynamic_cast<Graph*>(plot->activeGraph())->isPiePlot()){
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
void ApplicationWindow::selectMultiPeak(bool showFitPropertyBrowser)
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  selectMultiPeak(plot, showFitPropertyBrowser);
}

/**  Switch on the multi-peak selecting tool for fitting with the Fit algorithm.
 * @param plot :: The MultiLayer the tool will apply to.
 * @param showFitPropertyBrowser :: Set if FitPropertyBrowser must be shown as well.
 */
void ApplicationWindow::selectMultiPeak(MultiLayer* plot, bool showFitPropertyBrowser,
    double xmin, double xmax)
{
  setActiveWindow(plot);

  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if (dynamic_cast<Graph*>(plot->activeGraph())->isPiePlot()){
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
      //Called when setting up usual peakPickerTool
      PeakPickerTool* ppicker = new PeakPickerTool(g, mantidUI->fitFunctionBrowser(), mantidUI, showFitPropertyBrowser);
      if ( !ppicker->isInitialized() )
      {
        QMessageBox::warning(this,tr("MantidPlot - Warning"),
            tr("This functionality is not available for the underlying data."));
        delete ppicker;
        btnPointer->setOn(true);
        return;
      }
      if (xmin != xmax)
      {
        mantidUI->fitFunctionBrowser()->setStartX(xmin);
        mantidUI->fitFunctionBrowser()->setEndX(xmax);
      }
      g->setActiveTool(ppicker);
      // do we need this? PeakPickerTool::windowStateChanged does nothing
      //connect(plot,SIGNAL(windowStateChanged(Qt::WindowStates, Qt::WindowStates)),ppicker,SLOT(windowStateChanged(Qt::WindowStates, Qt::WindowStates)));
    }
  }

}

void ApplicationWindow::newLegend()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
  if ( g )
    g->newLegend();
}

void ApplicationWindow::addTimeStamp()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty())
  {
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
  if ( g )
    g->addTimeStamp();
}

void ApplicationWindow::addLabel()
{
    MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  Graph *g = dynamic_cast<Graph*>(plot->activeGraph());
  if (g)
  {
    g->setActiveTool(new LabelTool(g));
  }
}

void ApplicationWindow::addImage()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
  if (g)
  {
    g->drawLine(true);
    emit modified();
  }
}

void ApplicationWindow::drawArrow()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
  if (g)
  {
    g->drawLine(true, 1);
    emit modified();
  }
}

void ApplicationWindow::showImageDialog()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g)
  {
    ImageMarker *im = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
    if (!im)
      return;

    ImageDialog *id = new ImageDialog(this);
    id->setAttribute(Qt::WA_DeleteOnClose);
    connect (id, SIGNAL(setGeometry(int, int, int, int)),
        g, SLOT(updateImageMarker(int, int, int, int)));
    id->setOrigin(im->origin());
    id->setSize(im->size());
    id->exec();
  }
}

void ApplicationWindow::showLayerDialog()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if ( g ){
    LegendWidget *l = dynamic_cast<LegendWidget *>(g->selectedText());
    if (!l)
      return;

    TextDialog *td = new TextDialog(TextDialog::TextMarker, this, 0);
    td->setLegendWidget(l);
    td->exec();
  }
}

void ApplicationWindow::showLineDialog()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g){
    ArrowMarker *lm = dynamic_cast<ArrowMarker *>(g->selectedMarkerPtr());
    if (!lm)
      return;

    LineDialog *ld = new LineDialog(lm, this);
    ld->exec();
  }
}

void ApplicationWindow::addColToTable()
{
  Table* m = dynamic_cast<Table*>(activeWindow(TableWindow));
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
    dynamic_cast<Table*>(m)->clearSelection();
  else if (m->isA("Matrix"))
    dynamic_cast<Matrix*>(m)->clearSelection();
  else if (m->isA("MultiLayer"))
  {
    Graph* g = dynamic_cast<MultiLayer*>(m)->activeGraph();
    if (!g)
      return;

    if (g->activeTool())
    {
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
        dynamic_cast<RangeSelectorTool*>(g->activeTool())->clearSelection();

      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_LabelTool)
        dynamic_cast<LabelTool*>(g->activeTool())->removeTextBox();
    }

    else if (g->titleSelected())
      g->removeTitle();
    else if (g->markerSelected())
      g->removeMarker();
  }
  else if (m->isA("Note"))
    dynamic_cast<Note*>(m)->editor()->clear();
  emit modified();
}

void ApplicationWindow::copySelection()
{
  if(info->hasFocus()) {
    info->copy();
    return;
  }
  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

    if (m->inherits("Table"))
    dynamic_cast<Table*>(m)->copySelection();
  else if (m->isA("Matrix"))
    dynamic_cast<Matrix*>(m)->copySelection();
  else if (m->isA("MultiLayer")){
    MultiLayer* plot = dynamic_cast<MultiLayer*>(m);
    if (!plot || plot->layers() == 0)
      return;

    Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
    if (!g)
      return;

    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
      {
        dynamic_cast<RangeSelectorTool*>(g->activeTool())->copySelection();
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
    dynamic_cast<Note*>(m)->editor()->copy();
  else
    mantidUI->copyValues();//Mantid
}

void ApplicationWindow::cutSelection()
{

  MdiSubWindow* m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table"))
    dynamic_cast<Table*>(m)->cutSelection();
  else if (m->isA("Matrix"))
    dynamic_cast<Matrix*>(m)->cutSelection();
  else if(m->isA("MultiLayer")){
    MultiLayer* plot = dynamic_cast<MultiLayer*>(m);
    if (!plot || plot->layers() == 0)
      return;

    Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
    if (!g)
      return;

    if (g->activeTool()){
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
        dynamic_cast<RangeSelectorTool*>(g->activeTool())->cutSelection();
    } else {
      copyMarker();
      g->removeMarker();
    }
  }
  else if (m->isA("Note"))
    dynamic_cast<Note*>(m)->editor()->cut();

  emit modified();
}

void ApplicationWindow::copyMarker()
{
  lastCopiedLayer = NULL;

  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g && g->markerSelected()){
    if (g->selectedText()){
      d_text_copy = g->selectedText();
      d_image_copy = NULL;
      d_arrow_copy = NULL;
    } else if (g->arrowMarkerSelected()){
      d_arrow_copy = dynamic_cast<ArrowMarker *>(g->selectedMarkerPtr());
      d_image_copy = NULL;
      d_text_copy = NULL;
    } else if (g->imageMarkerSelected()){
      d_image_copy = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
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
    dynamic_cast<Table*>(m)->pasteSelection();
  else if (m->isA("Matrix"))
    dynamic_cast<Matrix*>(m)->pasteSelection();
  else if (m->isA("Note"))
    dynamic_cast<Note*>(m)->editor()->paste();
  else if (m->isA("MultiLayer")){
    MultiLayer* plot = dynamic_cast<MultiLayer*>(m);
    if (!plot)
      return;

    if (lastCopiedLayer){
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      Graph* g = plot->addLayer();
      g->copy(lastCopiedLayer);
      QPoint pos = plot->mapFromGlobal(QCursor::pos());
      plot->setGraphGeometry(pos.x(), pos.y()-20, lastCopiedLayer->width(), lastCopiedLayer->height());
      if (g->isWaterfallPlot())
        g->updateDataCurves();

      QApplication::restoreOverrideCursor();
    } else {
      if (plot->layers() == 0)
        return;

      Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
      if (!g)
        return;

      if (g->activeTool()){
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
          dynamic_cast<RangeSelectorTool*>(g->activeTool())->pasteSelection();
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

/**
 * Clone an MDI window. TODO: if this method is to be used it needs refactoring.
 *
 * @param w :: A window to clone.
 * @return :: Pointer to the cloned window if successful or NULL if failed.
 */
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
    MultiLayer *g = dynamic_cast<MultiLayer*>(w);
    nw = multilayerPlot(generateUniqueName(tr("Graph")), 0, g->getRows(), g->getCols());
    dynamic_cast<MultiLayer*>(nw)->copy(g);
  } else if (w->inherits("Table")){
    Table *t = dynamic_cast<Table*>(w);
    QString caption = generateUniqueName(tr("Table"));
    nw = newTable(caption, t->numRows(), t->numCols());
  } else if (w->isA("Graph3D")){
    Graph3D *g = dynamic_cast<Graph3D*>(w);
    if (!g->hasData()){
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, tr("MantidPlot - Duplicate error"), tr("Empty 3D surface plots cannot be duplicated!"));//Mantid
      return 0;
    }

    QString caption = generateUniqueName(tr("Graph"));
    QString s = g->formula();
    if (g->userFunction()){
      UserFunction2D *f = dynamic_cast<UserFunction2D*>( g->userFunction() );
      if ( f )
      {
          nw = plotSurface(f->formula(), g->xStart(), g->xStop(), g->yStart(), g->yStop(),
              g->zStart(), g->zStop(), f->columns(), f->rows());
      }
      else
      {
          QMessageBox::warning(this,"MantidPlot: warning", "Function cannot be cloned.");
          return NULL;
      }
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
    dynamic_cast<Graph3D*>(nw)->copy(g);
    customToolBars(nw);
  } else if (w->isA("Matrix")){
    nw = newMatrix((dynamic_cast<Matrix*>(w))->numRows(), (dynamic_cast<Matrix*>(w))->numCols());
    dynamic_cast<Matrix*>(nw)->copy(dynamic_cast<Matrix*>(w));
  } else if (w->isA("Note")){
    nw = newNote();
    if (nw)
      dynamic_cast<Note*>(nw)->setText(dynamic_cast<Note*>(w)->text());
  }

  if (nw){
    if (w->isA("MultiLayer")){
      if (status == MdiSubWindow::Maximized)
        nw->showMaximized();
    } else if (w->isA("Graph3D")){
      dynamic_cast<Graph3D*>(nw)->setIgnoreFonts(true);
      if (status != MdiSubWindow::Maximized){
        nw->resize(w->size());
        nw->showNormal();
      } else
        nw->showMaximized();
      dynamic_cast<Graph3D*>(nw)->setIgnoreFonts(false);
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
    dynamic_cast<Note*>(w)->editor()->undo();
  else if (qobject_cast<Matrix*>(w)){
    QUndoStack *stack = (dynamic_cast<Matrix*>(w))->undoStack();
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
    dynamic_cast<Note*>(w)->editor()->redo();
  else if (qobject_cast<Matrix*>(w)){
    QUndoStack *stack = (dynamic_cast<Matrix*>(w))->undoStack();
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
    QList<MdiSubWindow *> windows = currentFolder()->windowsList();
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
  activateNewWindow();
  emit modified();
}

void ApplicationWindow::hideWindow()
{
  WindowListItem *it = dynamic_cast<WindowListItem*>(lv->currentItem());
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
  WindowListItem *it = dynamic_cast<WindowListItem*>(lv->currentItem());
  MdiSubWindow *w = it->window();
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

void ApplicationWindow::setWindowGeometry(int x, int y, int w, int h)
{
  activeWindow()->setGeometry(x, y, w, h);
}

/**
  * Checks if a mdi sub-window exists.
  */
bool ApplicationWindow::existsWindow(MdiSubWindow* w) const
{
  if (!w) return false;
  FloatingWindow* fw = w->getFloatingWindow();
  if (fw && m_floatingWindows.contains(fw))
  {
    return true;
  }
  QMdiSubWindow* sw = w->getDockedWindow();
  return sw && d_workspace->subWindowList().contains(sw);
}

/**
  * Returns the active sub-window
  */
MdiSubWindow* ApplicationWindow::getActiveWindow() const
{
  if (!existsWindow(d_active_window))
  {
    d_active_window = NULL;
  }
  return d_active_window;
}

/**
  * Sets internal pointer to a new active sub-window.
  */
void ApplicationWindow::setActiveWindow(MdiSubWindow* w)
{
  d_active_window = w;
  if (!existsWindow(d_active_window))
  {
    d_active_window = NULL;
  }
  else
  {
    // This make sure that we don't have two versions of current active window (d_active_window and
    // active window of MdiArea) and they are either equal (when docked window is active> or the
    // latter one is NULL (when floating window is active).
    if ( d_active_window->getFloatingWindow() )
    {
      // If floating window is activated, we set MdiArea to not have any active sub-window.
      d_workspace->setActiveSubWindow(NULL);
    }
    else if ( QMdiSubWindow* w = d_active_window->getDockedWindow() )
    {
      // If docked window activated, activate it in MdiArea as well.
      d_workspace->setActiveSubWindow(w);
    }
  }
}


void ApplicationWindow::activateWindow()
{
  WindowListItem *it = dynamic_cast<WindowListItem*>(lv->currentItem());
  activateWindow(it->window());
}

/**
 * Activate a new MdiSubWindow: update the menu, tool bars, and folders.
 * @param w :: Subwindow to activate.
 */
void ApplicationWindow::activateWindow(MdiSubWindow *w, bool activateOuterWindow)
{

  if (blockWindowActivation) return;

  if (!w)
  {
    setActiveWindow(NULL);
    return;
  }

  // don't activat a window twice, but make sure it is visible
  if(getActiveWindow()  == w )
  {
    // this can happen
    if (w->status() == MdiSubWindow::Minimized || w->status() == MdiSubWindow::Hidden)
    {
      w->setNormal();
    }
    return;
  }

  // remember the active window
  setActiveWindow(w);

  updateWindowLists(w);
  customToolBars(w);
  customMenu(w);

  // ?
  if (d_opening_file)
  {
    return;
  }

  // return any non-active QMdiSubWindows to normal so that the active could be seen
  QMdiSubWindow* qw = dynamic_cast<QMdiSubWindow*>(w->parent());
  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  foreach(MdiSubWindow *ow, windows)
  {
    QMdiSubWindow* qww = dynamic_cast<QMdiSubWindow*>(ow->parent());
    if (qww && qww != qw && qww->isMaximized())
    {
      ow->setNormal();
      break;
    }
  }

  blockWindowActivation = true;
  FloatingWindow* fw = w->getFloatingWindow();
  if (fw)
  {
    if (activateOuterWindow)
    {
      w->setNormal();
    }
  }
  else
  {
    QMainWindow::activateWindow();
    w->setNormal();
  }
  blockWindowActivation = false;

  emit modified();
}

void ApplicationWindow::activateWindow(Q3ListViewItem * lbi)
{
  if (!lbi)
    lbi = lv->currentItem();

  if (!lbi || lbi->rtti() == FolderListItem::RTTI)
    return;

  activateWindow(dynamic_cast<WindowListItem*>(lbi)->window());
}

void ApplicationWindow::maximizeWindow(Q3ListViewItem * lbi)
{
  if (!lbi)
    lbi = lv->currentItem();

  if (!lbi || lbi->rtti() == FolderListItem::RTTI)
    return;

  maximizeWindow(dynamic_cast<WindowListItem*>(lbi)->window());
}

void ApplicationWindow::maximizeWindow(MdiSubWindow *w)
{
  if (!w || w->status() == MdiSubWindow::Maximized)
    return;

  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
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
    w = (dynamic_cast<WindowListItem*>(lv->currentItem()))->window();

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
    Table* m=dynamic_cast<Table*>(w);
    for (int i=0; i<m->numCols(); i++){
      QString name=m->colName(i);
      removeCurves(name);
    }
  } else if (w->isA("MultiLayer")){
    MultiLayer *ml =  dynamic_cast<MultiLayer*>(w);
    Graph *g = ml->activeGraph();
    if (g)
      btnPointer->setChecked(true);
  } else if (w->isA("Matrix"))
  {
    remove3DMatrixPlots(dynamic_cast<Matrix*>(w));
  }

  else  {  }

  if (hiddenWindows->contains(w))
  {
    hiddenWindows->takeAt(hiddenWindows->indexOf(w));
  }
}

void ApplicationWindow::closeWindow(MdiSubWindow* window)
{
  if (!window)
    return;

  if (getActiveWindow() == window)
  {
    activateNewWindow();
  }
  removeWindowFromLists(window);

  //update list view in project explorer
  Q3ListViewItem *it=lv->findItem (window->objectName(), 0, Q3ListView::ExactMatch|Q3ListView::CaseSensitive);
  if (it)
    lv->takeItem(it);

  if (show_windows_policy == ActiveFolder ){
    // the old code here relied on currentFolder() to remove its reference to window
    // before the call to this method
    // the following check makes it work in any case
    int cnt = currentFolder()->windowsList().count();
    if ( cnt == 0 || (cnt == 1 && currentFolder()->windowsList()[0] == window) )
    {
      customMenu(0);
      customToolBars(0);
    }
  } else if (show_windows_policy == SubFolders && !(currentFolder()->children()).isEmpty()){
    FolderListItem *fi = currentFolder()->folderListItem();
    FolderListItem *item = dynamic_cast<FolderListItem *>(fi->firstChild());
    int initial_depth = item->depth();
    bool emptyFolder = true;
    while (item && item->depth() >= initial_depth){
      QList<MdiSubWindow *> lst = item->folder()->windowsList();
      if (lst.count() > 0){
        emptyFolder = false;
        break;
      }
      item = dynamic_cast<FolderListItem *>(item->itemBelow());
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
    // The tool doesn't work yet (DataPickerTool)
    //QMenu *translateMenu = analysisMenu->addMenu (tr("&Translate"));
    //translateMenu->addAction(actionTranslateVert);
    //translateMenu->addAction(actionTranslateHor);
    //analysisMenu->insertSeparator();
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

    // The tool doesn't work yet (DataPickerTool)
    //multiPeakMenu->clear();
    //multiPeakMenu = analysisMenu->addMenu (tr("Fit &Multi-peak"));
    //multiPeakMenu->addAction(actionMultiPeakGauss);
    //multiPeakMenu->addAction(actionMultiPeakLorentz);

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
    if (w->isA("Table"))
    {
      analysisMenu->addAction(actionSortSelection);
    }
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

  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
  newMenu->addAction(actionNewTiledWindow);


  openMenu=fileMenu->addMenu(tr("&Load"));
  openMenu->addAction(actionOpenProj);
  openMenu->addAction(actionLoadFile);

  recentMenuID = fileMenu->insertItem(tr("&Recent Projects"), recentProjectsMenu);

  recentFilesMenuID = fileMenu->insertItem(tr("R&ecent Files"), recentFilesMenu);

  fileMenu->insertSeparator();
  fileMenu->addAction(actionManageDirs);
  fileMenu->insertSeparator();
  fileMenu->addAction(actionLoadImage);
  fileMenu->addAction(actionScriptRepo);

  MdiSubWindow *w = activeWindow();
  if (w && w->isA("Matrix"))
    fileMenu->addAction(actionExportMatrix);

  fileMenu->insertSeparator();
  fileMenu->addAction(actionSaveProjectAs);

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
  reloadCustomActions();
}

/**
 * Setup Windows menu.
 */
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
    foldersMenu->setItemChecked(id, f == currentFolder());

    f = f->folderBelow();
  }

  windowsMenu->insertItem(tr("&Folders"), foldersMenu);
  windowsMenu->insertSeparator();

  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  int n = static_cast<int>(windows.count());
  if (!n ){
    return;
  }

  windowsMenu->insertItem(tr("&Cascade"), this, SLOT(cascade()));
  windowsMenu->insertItem(tr("&Tile"), this, SLOT(tileMdiWindows()));
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
  if (activeWin->getFloatingWindow())
  {
    windowsMenu->insertItem(tr("Change to docked"), this, SLOT(changeActiveToDocked()));
  }
  else
  {
    windowsMenu->insertItem(tr("Change to floating"), this, SLOT(changeActiveToFloating()));
  }
  windowsMenu->insertItem(tr("&Hide Window"), this, SLOT(hideActiveWindow()));

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
      windowsMenu->setItemChecked( id, currentFolder()->activeWindow() == windows.at(i));
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

namespace // anonymous
{
  /**
   * Helper function used with Qt's qSort to make sure interfaces are in alphabetical order.
   */
  bool interfaceNameComparator(const QPair<QString, QString> & lhs, const QPair<QString, QString> & rhs)
  {
    return lhs.first.toLower() < rhs.first.toLower();
  }
} // anonymous namespace

void ApplicationWindow::interfaceMenuAboutToShow()
{
  interfaceMenu->clear();
  m_interfaceActions.clear();

  // Create a submenu for each category.  Make sure submenus are in alphabetical order,
  // and ignore any hidden categories.
  const QString hiddenProp = QString::fromStdString(
    Mantid::Kernel::ConfigService::Instance().getString("interfaces.categories.hidden")
  );
  auto hiddenCategories = hiddenProp.split(";", QString::SkipEmptyParts).toSet();
  QMap<QString, QMenu *> categoryMenus;
  auto sortedCategories = m_allCategories.toList();
  qSort(sortedCategories);
  foreach(const QString category, sortedCategories)
  {
    if( hiddenCategories.contains(category) )
      continue;
    QMenu * categoryMenu = new QMenu(interfaceMenu);
    categoryMenu->setObjectName(category + "Menu");
    interfaceMenu->insertItem(tr(category), categoryMenu);
    categoryMenus[category] = categoryMenu;
  }

  // Show the interfaces in alphabetical order in their respective submenus.
  qSort(m_interfaceNameDataPairs.begin(), m_interfaceNameDataPairs.end(),
    interfaceNameComparator);

  // Turn the name/data pairs into QActions with which we populate the menus.
  foreach(const auto interfaceNameDataPair, m_interfaceNameDataPairs)
  {
    const QString name = interfaceNameDataPair.first;
    const QString data = interfaceNameDataPair.second;

    foreach(const QString category, m_interfaceCategories[name])
    {
      if(!categoryMenus.contains(category))
        continue;
      QAction * openInterface = new QAction(tr(name), interfaceMenu);
      openInterface->setData(data);
      categoryMenus[category]->addAction(openInterface);

      // Update separate list containing all interface actions.
      m_interfaceActions.append(openInterface);
    }
  }

  foreach( auto categoryMenu, categoryMenus.values() )
  {
    connect(categoryMenu, SIGNAL(triggered(QAction*)), this, SLOT(performCustomAction(QAction*)));
  }

  interfaceMenu->insertSeparator();

  // Allow user to customise categories.
  QAction * customiseCategoriesAction = new QAction(tr("Add/Remove Categories"), this);
  connect(customiseCategoriesAction, SIGNAL(activated()), this, SLOT(showInterfaceCategoriesDialog()));
  interfaceMenu->addAction(customiseCategoriesAction);
}

void ApplicationWindow::tiledWindowMenuAboutToShow()
{
  tiledWindowMenu->clear();
  MdiSubWindow *w = activeWindow();
  if (!w) return;
  TiledWindow *tw = dynamic_cast<TiledWindow*>( w );
  if ( !tw ) return;
  tw->populateMenu( tiledWindowMenu );
}

void ApplicationWindow::showMarkerPopupMenu()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  MdiSubWindow* w = windows.at( id );
  if ( w )
  {
    this->activateWindow(w);
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
  //Save anything we need to
  saveSettings();
  mantidUI->saveProject(saved);

  //Clear out any old folders
  folders->blockSignals(true);
  lv->blockSignals(true);

  folders->clear();
  lv->clear();

  d_current_folder = new Folder( 0, tr("untitled"));
  FolderListItem *fli = new FolderListItem(folders, d_current_folder);
  d_current_folder->setFolderListItem(fli);
  fli->setOpen( true );

  lv->blockSignals(false);
  folders->blockSignals(false);

  //Reset everything else
  resultsLog->clear();
  setWindowTitle(tr("MantidPlot - untitled"));//Mantid
  projectname = "untitled";

  if(actionSaveProject)
    actionSaveProject->setEnabled(false);
}

void ApplicationWindow::savedProject()
{
  QCoreApplication::processEvents();
  if(actionSaveFile)
    actionSaveFile->setEnabled(false);
  if(actionSaveProject)
    actionSaveProject->setEnabled(false);
  saved = true;

  Folder *f = projectFolder();
  while (f)
  {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach(MdiSubWindow *w, folderWindows)
    {
      if (w->isA("Matrix"))
        (dynamic_cast<Matrix*>(w))->undoStack()->setClean();
    }
    f = f->folderBelow();
  }
}

void ApplicationWindow::modifiedProject()
{
  if (saved == false)
    return;
  // enable actionSaveProject, but not actionSaveFile (which is Save Nexus and doesn't
  // seem to make sense for qti objects (graphs, tables, matrices, notes, etc.)
  if(actionSaveProject)
    actionSaveProject->setEnabled(true);
  if(actionSaveProjectAs)
    actionSaveProjectAs->setEnabled(true);
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
    mantidUI->drop(e);
}


void ApplicationWindow::dragEnterEvent( QDragEnterEvent* e )
{
  if (e->source()){
    e->accept( mantidUI->canAcceptDrop(e) );
    return;
  }
  else
  {
    e->accept(Q3UriDrag::canDecode(e));
  }
  e->ignore();
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
  if(scriptingWindow && scriptingWindow->isExecuting())
  {
    if( ! (QMessageBox::question(this, tr("MantidPlot"), "A script is still running, abort and quit application?", tr("Yes"), tr("No")) == 0) )
    {
      ce->ignore();
      return;
    }
    // We used to cancel running algorithms here (if 'Yes' to the above question), but now that
    // happens in MantidUI::shutdown (called below) because we want it regardless of whether a
    // script is running.
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

  // Close all the MDI windows
  MDIWindowList windows = getAllWindows();
  foreach(MdiSubWindow* w,windows)
  {
    w->confirmClose(false);
    w->close();
  }

  mantidUI->shutdown();

  if (catalogSearch)
  {
    catalogSearch->disconnect();
    delete catalogSearch;
    catalogSearch = NULL;
  }

  if( scriptingWindow )
  {
    scriptingWindow->disconnect();
    this->showScriptWindow(true);
    // Other specific settings
    scriptingWindow->saveSettings();
    scriptingWindow->acceptCloseEvent(true);
    scriptingWindow->close();
    delete scriptingWindow;
    scriptingWindow = NULL;
  }
  /// Ensure interface python references are cleaned up before the interpreter shuts down
  delete m_iface_script;

  // Emit a shutting_down() signal that can be caught by
  // independent QMainWindow objects to know when MantidPlot
  // is shutting down.
  emit shutting_down();

  //Save the settings and exit
  saveSettings();
  scriptingEnv()->finalize();

  ce->accept();

}

void ApplicationWindow::customEvent(QEvent *e)
{
  if (e->type() == SCRIPTING_CHANGE_EVENT)
    scriptingChangeEvent(dynamic_cast<ScriptingChangeEvent*>(e));
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
      Folder *f = dynamic_cast<FolderListItem*>(item)->folder();
      if (deleteFolder(f))
        delete item;
    } else
      dynamic_cast<WindowListItem*>(item)->window()->close();
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
  window.addAction(actionNewTiledWindow);
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
    d_current_folder = dynamic_cast<FolderListItem*>(it)->folder();
    showFolderPopupMenu(it, p, false);
    return;
  }

  MdiSubWindow *w= dynamic_cast<WindowListItem*>(it)->window();
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
      if (static_cast<int>(graphs.count())>0){
        cm.insertSeparator();
        for (int i=0;i<static_cast<int>(graphs.count());i++)
          plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

        cm.insertItem(tr("D&epending Graphs"),&plots);
      }
    } else if (w->isA("Matrix")){
      QStringList graphs = depending3DPlots(dynamic_cast<Matrix*>(w));
      if (static_cast<int>(graphs.count())>0){
        cm.insertSeparator();
        for (int i=0;i<static_cast<int>(graphs.count());i++)
          plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

        cm.insertItem(tr("D&epending 3D Graphs"),&plots);
      }
    } else if (w->isA("MultiLayer")) {
      tablesDepend->clear();
      QStringList tbls=multilayerDependencies(w);
      int n = static_cast<int>(tbls.count());
      if (n > 0){
        cm.insertSeparator();
        for (int i=0; i<n; i++)
          tablesDepend->insertItem(tbls[i], i, -1);

        cm.insertItem(tr("D&epends on"), tablesDepend);
      }
    } else if (w->isA("Graph3D")) {
      Graph3D *sp=dynamic_cast<Graph3D*>(w);
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
    } else if (w->isA("TiledWindow")) {
      std::cerr << "Menu for TiledWindow" << std::endl;
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
    if (w->isA("Graph3D") && dynamic_cast<Graph3D*>(w)->matrix() == m)
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
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
      foreach(Graph *g, layers){
        onPlot = g->curvesList();
        onPlot = onPlot.grep (name,TRUE);
        if (static_cast<int>(onPlot.count()) && plots.contains(w->objectName())<=0)
          plots << w->objectName();
      }
    }else if (w->isA("Graph3D")){
      if ((dynamic_cast<Graph3D*>(w)->formula()).contains(name,TRUE) && plots.contains(w->objectName())<=0)
        plots << w->objectName();
    }
  }
  return plots;
}

QStringList ApplicationWindow::multilayerDependencies(QWidget *w)
{
  QStringList tables;
  MultiLayer *g=dynamic_cast<MultiLayer*>(w);
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  QMenu cm(this);
  Graph* ag = dynamic_cast<Graph*>(plot->activeGraph());
  PlotToolInterface* tool = ag->activeTool();
  if (dynamic_cast<PeakPickerTool*>(tool))
  {
    dynamic_cast<PeakPickerTool*>(tool)->prepareContextMenu(cm);
    cm.exec(QCursor::pos());
    return;
  }

  QMenu axes(this);
  QMenu colour(this);
  QMenu normalization(this);
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
    if (m_enableQtiPlotFitting)
    {
      cm.insertItem(tr("Anal&yze"), analysisMenu);
    }
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

  if(ag->normalizable())
  {
    QAction *noNorm = new QAction(tr("N&one"), &normalization);
    noNorm->setCheckable(true);
    connect(noNorm, SIGNAL(activated()), ag, SLOT(noNormalization()));
    normalization.addAction(noNorm);

    QAction *binNorm = new QAction(tr("&Bin Width"), &normalization);
    binNorm->setCheckable(true);
    connect(binNorm, SIGNAL(activated()), ag, SLOT(binWidthNormalization()));
    normalization.addAction(binNorm);

    QActionGroup *normalizationActions = new QActionGroup(this);
    normalizationActions->setExclusive(true);
    normalizationActions->addAction(noNorm);
    normalizationActions->addAction(binNorm);

    noNorm->setChecked(!ag->isDistribution());
    binNorm->setChecked(ag->isDistribution());
    cm.insertItem(tr("&Normalization"), &normalization);
  }

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
    MultiLayer *g = dynamic_cast<MultiLayer*>(w);
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
    Graph3D *g=dynamic_cast<Graph3D*>(w);
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
    Matrix *t = dynamic_cast<Matrix*>(w);
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

  menu->addAction(actionPrint);
  menu->addSeparator();
  menu->addAction(actionRename);
  menu->addAction(actionCopyWindow);
  menu->addSeparator();
}

void ApplicationWindow::showTableContextMenu(bool selection)
{
  Table *t = dynamic_cast<Table*>(activeWindow(TableWindow));
  if (!t)
    return;

  bool isEditable = t->isEditable();
  bool isFixedColumns = t->isFixedColumns();

  QMenu cm(this);
  if (selection){
    if ((int)t->selectedColumns().count() > 0){
      showColMenu(t->firstSelectedColumn());
      return;
    } else if (t->numSelectedRows() == 1) {
      if (isEditable) cm.addAction(actionShowColumnValuesDialog);
      if (isEditable) cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      if (isEditable) cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      if (isEditable) cm.addAction(actionTableRecalculate);
      if (isEditable) cm.insertItem(getQPixmap("insert_row_xpm"), tr("&Insert Row"), t, SLOT(insertRow()));
      cm.insertItem(getQPixmap("delete_row_xpm"), tr("&Delete Row"), t, SLOT(deleteSelectedRows()));
      if (isEditable) cm.insertItem(getQPixmap("erase_xpm"), tr("Clea&r Row"), t, SLOT(clearSelection()));
      cm.insertSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numSelectedRows() > 1) {
      if (isEditable) cm.addAction(actionShowColumnValuesDialog);
      if (isEditable) cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      if (isEditable) cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      if (isEditable) cm.addAction(actionTableRecalculate);
      cm.insertItem(getQPixmap("delete_row_xpm"), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
      if (isEditable) cm.insertItem(getQPixmap("erase_xpm"),tr("Clea&r Rows"), t, SLOT(clearSelection()));
      cm.insertSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numRows() > 0 && t->numCols() > 0){
      if (isEditable) cm.addAction(actionShowColumnValuesDialog);
      if (isEditable) cm.insertItem(getQPixmap("cut_xpm"),tr("Cu&t"), t, SLOT(cutSelection()));
      cm.insertItem(getQPixmap("copy_xpm"),tr("&Copy"), t, SLOT(copySelection()));
      if (isEditable) cm.insertItem(getQPixmap("paste_xpm"),tr("&Paste"), t, SLOT(pasteSelection()));
      cm.insertSeparator();
      if (isEditable) cm.addAction(actionTableRecalculate);
      if (isEditable) cm.insertItem(getQPixmap("erase_xpm"),tr("Clea&r"), t, SLOT(clearSelection()));
    }
  } else {
    cm.addAction(actionShowExportASCIIDialog);
    cm.insertSeparator();
    if (!isFixedColumns) cm.addAction(actionAddColToTable);
    if (isEditable) cm.addAction(actionClearTable);
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  FunctionDialog* fd = functionDialog(g);
  fd->setWindowTitle(tr("MantidPlot - Edit function"));//Mantid
  fd->setCurveToModify(g, curve);
  return fd;
}

FunctionDialog* ApplicationWindow::functionDialog(Graph* g)
{
  FunctionDialog* fd = new FunctionDialog(this,g);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  connect (fd,SIGNAL(clearParamFunctionsList()),this,SLOT(clearParamFunctionsList()));
  connect (fd,SIGNAL(clearPolarFunctionsList()),this,SLOT(clearPolarFunctionsList()));

  fd->insertParamFunctionsList(xFunctions, yFunctions);
  fd->insertPolarFunctionsList(rFunctions, thetaFunctions);
  fd->show();
  //fd->setActiveWindow();
  return fd;
}

void ApplicationWindow::addFunctionCurve()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
    functionDialog(g);
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
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFramed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::setBoxed3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBoxed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::removeAxes3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setNoAxes();
  actionShowAxisDialog->setEnabled(false);
}

void ApplicationWindow::removeGrid3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setPolygonStyle();
}

void ApplicationWindow::setHiddenLineGrid3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setHiddenLineStyle();
}

void ApplicationWindow::setPoints3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setDotStyle();
}

void ApplicationWindow::setCones3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setConeStyle();
}

void ApplicationWindow::setCrosses3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setCrossStyle();
}

void ApplicationWindow::setBars3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBarStyle();
}

void ApplicationWindow::setLineGrid3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setWireframeStyle();
}

void ApplicationWindow::setFilledMesh3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFilledMeshStyle();
}

void ApplicationWindow::setFloorData3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorData();
}

void ApplicationWindow::setFloorIso3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorIsolines();
}

void ApplicationWindow::setEmptyFloor3DPlot()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setEmptyFloor();
}

void ApplicationWindow::setFrontGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFrontGrid(on);
}

void ApplicationWindow::setBackGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBackGrid(on);
}

void ApplicationWindow::setFloorGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorGrid(on);
}

void ApplicationWindow::setCeilGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setCeilGrid(on);
}

void ApplicationWindow::setRightGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setRightGrid(on);
}

void ApplicationWindow::setLeftGrid3DPlot(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
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

void ApplicationWindow::custom3DActions(MdiSubWindow *w)
{
  if (w && w->isA("Graph3D"))
  {
    Graph3D* plot = dynamic_cast<Graph3D*>(w);
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
  /**
   * Only inits the actions that are later placed in the
   * Format menu in this->customMenu(MdiSubWindow* w)
   */

  coord = new QActionGroup( this );
  Box = new QAction( coord );
  Box->setIcon(QIcon(getQPixmap("box_xpm")));
  Box->setCheckable(true);
  Box->setChecked( true );

  Frame = new QAction( coord );
  Frame->setIcon(QIcon(getQPixmap("free_axes_xpm")) );
  Frame->setCheckable(true);

  None = new QAction( coord );
  None->setIcon(QIcon(getQPixmap("no_axes_xpm")) );
  None->setCheckable(true);

  // grid actions - Used when the format menu is active for the 3D plot
  grids = new QActionGroup( this );
  grids->setEnabled( true );
  grids->setExclusive( false );
  front = new QAction(grids);
  front->setMenuText(tr("Front"));
  front->setCheckable( true );
  front->setIcon(QIcon(getQPixmap("frontGrid_xpm")) );
  back = new QAction( grids);
  back->setMenuText(tr("Back"));
  back->setCheckable( true );
  back->setIcon(QIcon(getQPixmap("backGrid_xpm")));
  right = new QAction( grids);
  right->setMenuText(tr("Right"));
  right->setCheckable( true );
  right->setIcon(QIcon(getQPixmap("leftGrid_xpm")) );
  left = new QAction( grids);
  left->setMenuText(tr("Left"));
  left->setCheckable( true );
  left->setIcon(QIcon(getQPixmap("rightGrid_xpm")));
  ceil = new QAction( grids);
  ceil->setMenuText(tr("Ceiling"));
  ceil->setCheckable( true );
  ceil->setIcon(QIcon(getQPixmap("ceilGrid_xpm")) );
  floor = new QAction( grids);
  floor->setMenuText(tr("Floor"));
  floor->setCheckable( true );
  floor->setIcon(QIcon(getQPixmap("floorGrid_xpm")) );


  actionPerspective = new QAction( this );
  actionPerspective->setToggleAction( TRUE );
  actionPerspective->setIconSet(getQPixmap("perspective_xpm") );
  actionPerspective->setOn(!orthogonal3DPlots);
  connect(actionPerspective, SIGNAL(toggled(bool)), this, SLOT(togglePerspective(bool)));

  actionResetRotation = new QAction( this );
  actionResetRotation->setToggleAction( false );
  actionResetRotation->setIconSet(getQPixmap("reset_rotation_xpm"));
  connect(actionResetRotation, SIGNAL(activated()), this, SLOT(resetRotation()));

  actionFitFrame = new QAction( this );
  actionFitFrame->setToggleAction( false );
  actionFitFrame->setIconSet(getQPixmap("fit_frame_xpm"));
  connect(actionFitFrame, SIGNAL(activated()), this, SLOT(fitFrameToLayer()));

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
  filledmesh->setChecked( true );

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
  floornone->setChecked( true );

  actionAnimate = new QAction( this );
  actionAnimate->setToggleAction( true );
  actionAnimate->setIconSet(getQPixmap("movie_xpm"));

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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (g){
    ImageMarker *im = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
    if (im){
      QString fn = im->fileName();
      if (!fn.isEmpty())
        importImage(fn);
    }
  }
}

void ApplicationWindow::autoArrangeLayers()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  plot->setMargins(5, 5, 5, 5);
  plot->setSpacing(5, 5);
  plot->arrangeLayers(true, false);

  if (plot->isWaterfallPlot())
    plot->updateWaterfalls();
}

void ApplicationWindow::addLayer()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  plot->confirmRemoveLayer();
}

void ApplicationWindow::openMatrix(const std::string& lines, const int fileVersion)
{
  //The first line specifies the name, dimensions and date.
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));
  std::string firstLine = lineVec.front();
  lineVec.erase(lineVec.begin());
  std::string newLines = boost::algorithm::join(lineVec, "\n");

  //Parse the first line
  std::vector<std::string> values;
  boost::split(values, firstLine, boost::is_any_of("\t"));

  if(values.size() < 4)
  {
    return;
  }

  const std::string caption = values[0];
  const std::string date = values[3];

  int rows = 0;
  int cols = 0;
  Mantid::Kernel::Strings::convert<int>(values[1], rows);
  Mantid::Kernel::Strings::convert<int>(values[2], cols);

  Matrix* m = newMatrix(QString::fromStdString(caption), rows, cols);
  setListViewDate(QString::fromStdString(caption), QString::fromStdString(date));
  m->setBirthDate(QString::fromStdString(date));

  TSVSerialiser tsv(newLines);

  if(tsv.hasLine("geometry"))
  {
    std::string gStr = tsv.lineAsString("geometry");
    restoreWindowGeometry(this, m, QString::fromStdString(gStr));
  }

  m->loadFromProject(newLines, this, fileVersion);
}

void ApplicationWindow::openMantidMatrix(const std::string& lines)
{
  TSVSerialiser tsv(lines);

  MantidMatrix* m = 0;

  if(tsv.selectLine("WorkspaceName"))
  {
    m = mantidUI->openMatrixWorkspace(tsv.asString(1), -1, -1);
  }

  if(!m)
    return;

  if(tsv.selectLine("geometry"))
  {
    const std::string geometry = tsv.lineAsString("geometry");
    restoreWindowGeometry(this, m, QString::fromStdString(geometry));
  }

  if(tsv.selectLine("tgeometry"))
  {
    const std::string geometry = tsv.lineAsString("tgeometry");
    restoreWindowGeometry(this, m, QString::fromStdString(geometry));
  }

  //Append to the list of mantid matrix windows
  m_mantidmatrixWindows << m;
}

void ApplicationWindow::openMultiLayer(const std::string& lines, const int fileVersion)
{
  MultiLayer* plot = 0;
  std::string multiLayerLines = lines;

  //The very first line of a multilayer section has some important settings,
  //and lacks a name. Take it out and parse it manually.

  if(multiLayerLines.length() == 0)
    return;

  std::vector<std::string> lineVec;
  boost::split(lineVec, multiLayerLines, boost::is_any_of("\n"));

  std::string firstLine = lineVec.front();
  //Remove the first line
  lineVec.erase(lineVec.begin());
  multiLayerLines = boost::algorithm::join(lineVec, "\n");

  //Split the line up into its values
  std::vector<std::string> values;
  boost::split(values, firstLine, boost::is_any_of("\t"));

  std::string caption = values[0];
  int rows = 1;
  int cols = 1;
  Mantid::Kernel::Strings::convert<int>(values[1], rows);
  Mantid::Kernel::Strings::convert<int>(values[2], cols);
  std::string birthDate = values[3];

  plot = multilayerPlot(QString::fromUtf8(caption.c_str()), 0, rows, cols);
  plot->setBirthDate(QString::fromStdString(birthDate));
  setListViewDate(QString::fromStdString(caption), QString::fromStdString(birthDate));

  plot->loadFromProject(multiLayerLines, this, fileVersion);
}

/** This method opens script window with a list of scripts loaded
 */
void ApplicationWindow::openScriptWindow(const QStringList& files)
{
  showScriptWindow();
  if(!scriptingWindow)
    return;

  scriptingWindow->setWindowTitle("MantidPlot: " + scriptingEnv()->languageName() + " Window");

  //The first time we don't use a new tab, to re-use the blank script tab
  //on further iterations we open a new tab
  bool newTab = false;
  for(auto file = files.begin(); file != files.end(); ++file)
  {
    if(file->isEmpty())
      continue;
    scriptingWindow->open(*file, newTab);
    newTab = true;
  }
}

/** This method populates the mantid workspace tree when project file is loaded and
*   then groups all the workspaces that belonged to a group when the project was saved.
*
*   @params &s :: A QString that contains all the names of workspaces and group workspaces
*                 that the user is trying to load from a project.
*/
void ApplicationWindow::populateMantidTreeWidget(const QString &s)
{
  QStringList list = s.split("\t");
  QStringList::const_iterator line = list.begin();
  for (++line; line!=list.end(); ++line)
  {
    if ((*line).contains(',')) // ...it is a group and more work needs to be done
    {
      // Format of string is "GroupName, Workspace, Workspace, Workspace, .... and so on "
      QStringList groupWorkspaces = (*line).split(',');
      std::string groupName = groupWorkspaces[0].toStdString();
      std::vector<std::string> inputWsVec;
      // Work through workspaces, load into Mantid and then push into vectorgroup (ignore group name, start at 1)
      for (int i=1; i<groupWorkspaces.size(); i++)
      {
        std::string wsName = groupWorkspaces[i].toStdString();
        loadWsToMantidTree(wsName);
        inputWsVec.push_back(wsName);
      }

      try
      {
        bool smallGroup(inputWsVec.size() < 2);
        if (smallGroup) // if the group contains less than two items...
        {
          // ...create a new workspace and then delete it later on (group workspace requires two workspaces in order to run the alg)
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace",1);
          alg->setProperty("OutputWorkspace", "boevsMoreBoevs");
          alg->setProperty< std::vector<double> >("DataX", std::vector<double>(2,0.0) );
          alg->setProperty< std::vector<double> >("DataY", std::vector<double>(2,0.0) );
          // execute the algorithm
          alg->execute();
          // name picked because random and won't ever be used.
          inputWsVec.push_back("boevsMoreBoevs");
        }

        // Group the workspaces as they were when the project was saved
        std::string algName("GroupWorkspaces");
        Mantid::API::IAlgorithm_sptr groupingAlg = Mantid::API::AlgorithmManager::Instance().create(algName,1);
        groupingAlg->initialize();
        groupingAlg->setProperty("InputWorkspaces",inputWsVec);
        groupingAlg->setPropertyValue("OutputWorkspace",groupName);
        //execute the algorithm
        groupingAlg->execute();

        if (smallGroup)
        {
          // Delete the temporary workspace used to create a group of 1 or less (currently can't have group of 0)
          Mantid::API::AnalysisDataService::Instance().remove("boevsMoreBoevs");
        }
      }
      // Error catching for algorithms
      catch(std::invalid_argument &)
      {
        QMessageBox::critical(this,"MantidPlot - Algorithm error"," Error in Grouping Workspaces");
      }
      catch(Mantid::Kernel::Exception::NotFoundError&)
      {
        QMessageBox::critical(this,"MantidPlot - Algorithm error"," Error in Grouping Workspaces");
      }
      catch(std::runtime_error& )
      {
        QMessageBox::critical(this,"MantidPlot - Algorithm error"," Error in Grouping Workspaces");
      }
      catch(std::exception& )
      {
        QMessageBox::critical(this,"MantidPlot - Algorithm error"," Error in Grouping Workspaces");
      }
    }
    else // ...not a group so just load the workspace
    {
      loadWsToMantidTree((*line).toStdString());
    }
  }
}

/** This method populates the mantid workspace tree when  project file is loaded
 */
void ApplicationWindow::loadWsToMantidTree(const std::string & wsName)
{
  if(wsName.empty())
  {
    throw std::runtime_error("Workspace Name not found in project file ");
  }
  std::string fileName(workingDir.toStdString()+"/"+wsName);
  fileName.append(".nxs");
  mantidUI->loadWSFromFile(wsName,fileName);
}

void ApplicationWindow::openTable(const std::string& lines, const int fileVersion)
{
  std::vector<std::string> lineVec, valVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  const std::string firstLine = lineVec.front();
  boost::split(valVec, firstLine, boost::is_any_of("\t"));

  if(valVec.size() < 4)
    return;

  std::string caption = valVec[0];
  std::string date = valVec[3];
  int rows = 1;
  int cols = 1;
  Mantid::Kernel::Strings::convert<int>(valVec[1], rows);
  Mantid::Kernel::Strings::convert<int>(valVec[2], cols);

  Table* t = newTable(QString::fromStdString(caption), rows, cols);
  setListViewDate(QString::fromStdString(caption), QString::fromStdString(date));
  t->setBirthDate(QString::fromStdString(date));
  t->loadFromProject(lines, this, fileVersion);
}

void ApplicationWindow::openTableStatistics(const std::string& lines, const int fileVersion)
{
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  const std::string firstLine = lineVec.front();

  std::vector<std::string> firstLineVec;
  boost::split(firstLineVec, firstLine, boost::is_any_of("\t"));

  if(firstLineVec.size() < 4)
    return;

  const std::string name = firstLineVec[0];
  const std::string tableName = firstLineVec[1];
  const std::string type = firstLineVec[2];
  const std::string birthDate = firstLineVec[3];

  TSVSerialiser tsv(lines);

  if(!tsv.hasLine("Targets"))
    return;

  const std::string targetsLine = tsv.lineAsString("Targets");

  std::vector<std::string> targetsVec;
  boost::split(targetsVec, targetsLine, boost::is_any_of("\t"));

  //Erase the first item ("Targets")
  targetsVec.erase(targetsVec.begin());

  QList<int> targets;
  for(auto it = targetsVec.begin(); it != targetsVec.end(); ++it)
  {
    int target = 0;
    Mantid::Kernel::Strings::convert<int>(*it, target);
    targets << target;
  }

  TableStatistics* t = newTableStatistics(table(QString::fromStdString(tableName)),
      type == "row" ? TableStatistics::row : TableStatistics::column,
      targets, QString::fromStdString(name));

  if(!t)
    return;

  setListViewDate(QString::fromStdString(name), QString::fromStdString(birthDate));
  t->setBirthDate(QString::fromStdString(birthDate));

  t->loadFromProject(lines, this, fileVersion);
}

void ApplicationWindow::openSurfacePlot(const std::string& lines, const int fileVersion)
{
  std::vector<std::string> lineVec, valVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  //First line is name\tdate
  const std::string firstLine = lineVec[0];
  boost::split(valVec, firstLine, boost::is_any_of("\t"));

  if(valVec.size() < 2)
    return;

  const std::string caption = valVec[0];
  const std::string dateStr = valVec[1];
  valVec.clear();

  const std::string tsvLines = boost::algorithm::join(lineVec, "\n");

  TSVSerialiser tsv(tsvLines);

  Graph3D* plot = 0;

  if(tsv.selectLine("SurfaceFunction"))
  {
    std::string funcStr;
    double val2, val3, val4, val5, val6, val7;
    tsv >> funcStr >> val2 >> val3 >> val4 >> val5 >> val6 >> val7;

    const QString funcQStr = QString::fromStdString(funcStr);

    if(funcQStr.endsWith("(Y)", true))
    {
      plot = dataPlot3D(QString::fromStdString(caption), QString::fromStdString(funcStr), val2, val3,
          val4, val5, val6, val7);
    }
    else if(funcQStr.contains("(Z)", true) > 0)
    {
      plot = openPlotXYZ(QString::fromStdString(caption), QString::fromStdString(funcStr), val2, val3,
          val4, val5, val6, val7);
    }
    else if(funcQStr.startsWith("matrix<", true) && funcQStr.endsWith(">", false))
    {
      plot = openMatrixPlot3D(QString::fromStdString(caption), QString::fromStdString(funcStr), val2, val3,
          val4, val5, val6, val7);
    }
    else if(funcQStr.contains("mantidMatrix3D"))
    {
      MantidMatrix* m = 0;
      if(tsv.selectLine("title"))
      {
        std::string wsName = tsv.asString(1);

        //wsName is actually "Workspace workspacename", so we chop off
        //the first 10 characters.
        if(wsName.length() < 11)
          return;

        wsName = wsName.substr(10, std::string::npos);

        //Get the workspace this pertains to.
        for(auto mIt = m_mantidmatrixWindows.begin(); mIt != m_mantidmatrixWindows.end(); ++mIt)
        {
          if(*mIt && wsName == (*mIt)->getWorkspaceName())
          {
            m = *mIt;
            break;
          }
        }
      } //select line "title"

      int style = Qwt3D::WIREFRAME;
      if(tsv.selectLine("Style"))
        tsv >> style;

      if(m)
        plot = m->plotGraph3D(style);
    }
    else if(funcQStr.contains(","))
    {
      QStringList l = funcQStr.split(",", QString::SkipEmptyParts);
      plot = plotParametricSurface(l[0], l[1], l[2], l[3].toDouble(), l[4].toDouble(),
          l[5].toDouble(), l[6].toDouble(), l[7].toInt(), l[8].toInt(), l[9].toInt(), l[10].toInt());
    }
    else
    {
      QStringList l = funcQStr.split(";", QString::SkipEmptyParts);
      if (l.count() == 1)
      {
        plot = plotSurface(funcQStr, val2, val3, val4, val5, val6, val7);
      }
      else if (l.count() == 3)
      {
        plot = plotSurface(l[0], val2, val3, val4, val5, val6, val7, l[1].toInt(), l[2].toInt());
      }
      setWindowName(plot, QString::fromStdString(caption));
    }
  }

  if(!plot)
    return;

  setListViewDate(QString::fromStdString(caption), QString::fromStdString(dateStr));
  plot->setBirthDate(QString::fromStdString(dateStr));
  plot->setIgnoreFonts(true);
  restoreWindowGeometry(this, plot, QString::fromStdString(tsv.lineAsString("geometry")));
  plot->loadFromProject(tsvLines, this, fileVersion);
}

void ApplicationWindow::copyActiveLayer()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();

  lastCopiedLayer = g;
  connect (g, SIGNAL(destroyed()), this, SLOT(closedLastCopiedLayer()));
  g->copyImage();
}

void ApplicationWindow::showDataSetDialog(Analysis operation)
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  DataSetDialog *ad = new DataSetDialog(tr("Curve") + ": ", this,g);
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
      ScaleEngine *se = dynamic_cast<ScaleEngine *>(g->plotWidget()->axisScaleEngine(c->xAxis()));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
    info += QString::number((dynamic_cast<Matrix*>(w))->integrate()) + "\n";
    info += "-------------------------------------------------------------\n";
    currentFolder()->appendLogInfo(info);
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
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(w)->layersList();
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
  else if (action == btnLabel)
    addLabel();
  else if (action == btnArrow)
    drawArrow();
  else if (action == btnLine)
    drawLine();
  else if (action == btnMultiPeakPick)
    selectMultiPeak();
  else if (action == actionPanPlot)
    panOnPlot();
}

void ApplicationWindow::connectSurfacePlot(Graph3D *plot)
{
  connect (plot, SIGNAL(showOptionsDialog()), this,SLOT(showPlot3dDialog()));
  plot->confirmClose(confirmClosePlot3D);
}

void ApplicationWindow::connectMultilayerPlot(MultiLayer *g)
{
  connect (g,SIGNAL(showTextDialog()),this,SLOT(showTextDialog()));
  connect (g,SIGNAL(showPlotDialog(int)),this,SLOT(showPlotDialog(int)));
  connect (g,SIGNAL(showScaleDialog(int)), this, SLOT(showScalePageFromAxisDialog(int)));
  connect (g,SIGNAL(showAxisDialog(int)), this, SLOT(showAxisPageFromAxisDialog(int)));
  connect (g,SIGNAL(showCurveContextMenu(int)), this, SLOT(showCurveContextMenu(int)));
  connect (g,SIGNAL(showCurvesDialog()),this,SLOT(showCurvesDialog()));
  connect (g,SIGNAL(drawLineEnded(bool)), btnPointer, SLOT(setOn(bool)));
  connect (g, SIGNAL(showAxisTitleDialog()), this, SLOT(showAxisTitleDialog()));

  connect (g,SIGNAL(showMarkerPopupMenu()),this,SLOT(showMarkerPopupMenu()));
  connect (g,SIGNAL(cursorInfo(const QString&)),info,SLOT(setText(const QString&)));
  connect (g,SIGNAL(showImageDialog()),this,SLOT(showImageDialog()));
  connect (g,SIGNAL(createTable(const QString&,int,int,const QString&)),
      this,SLOT(newTable(const QString&,int,int,const QString&)));
  connect (g,SIGNAL(viewTitleDialog()),this,SLOT(showTitleDialog()));
  //connect (g,SIGNAL(modifiedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect (g,SIGNAL(modifiedPlot()), this, SLOT(modifiedProject()));
  connect (g,SIGNAL(showLineDialog()),this, SLOT(showLineDialog()));
  connect (g,SIGNAL(pasteMarker()),this,SLOT(pasteSelection()));
  connect (g,SIGNAL(showGraphContextMenu()),this,SLOT(showGraphContextMenu()));
  connect (g,SIGNAL(setPointerCursor()),this, SLOT(pickPointerCursor()));
  connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SLOT(setFormatBarFont(const QFont&)));
  connect (g,SIGNAL(enableTextEditor(Graph *)), this, SLOT(enableTextEditor(Graph *)));

  g->confirmClose(confirmClosePlot2D);
}

void ApplicationWindow::connectTable(Table* w)
{
  connect (w->table(), SIGNAL(selectionChanged()), this, SLOT(customColumnActions()));
  connect (w,SIGNAL(removedCol(const QString&)),this,SLOT(removeCurves(const QString&)));
  connect (w,SIGNAL(modifiedData(Table *, const QString&)),
      this, SLOT(updateCurves(Table *, const QString&)));
  connect (w,SIGNAL(optionsDialog()),this,SLOT(showColumnOptionsDialog()));
  connect (w,SIGNAL(colValuesDialog()),this,SLOT(showColumnValuesDialog()));
  connect (w,SIGNAL(showContextMenu(bool)),this,SLOT(showTableContextMenu(bool)));
  connect (w,SIGNAL(changedColHeader(const QString&,const QString&)),this,SLOT(updateColNames(const QString&,const QString&)));
  connect (w,SIGNAL(createTable(const QString&,int,int,const QString&)),this,SLOT(newTable(const QString&,int,int,const QString&)));

  w->confirmClose(confirmCloseTable);
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
  folders->setPalette(palette);
}

void ApplicationWindow::setPlot3DOptions()
{
  QList<MdiSubWindow *> windows = windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("Graph3D")){
      Graph3D *g = dynamic_cast<Graph3D*>(w);
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

  actionFirstTimeSetup = new QAction(tr("First Time Setup"), this);
  connect(actionFirstTimeSetup, SIGNAL(activated()), this, SLOT(showFirstTimeSetup()));

  actionSetupParaview = new QAction(tr("Setup 3D Visualisation"), this);
  connect(actionSetupParaview, SIGNAL(activated()), this, SLOT(showSetupParaview()));

  actionNewProject = new QAction(QIcon(":/NewProject16x16.png"), tr("New &Project"), this);
  actionNewProject->setShortcut( tr("Ctrl+N") );
  connect(actionNewProject, SIGNAL(activated()), this, SLOT(newProject()));

  actionSaveProject=new QAction(QIcon(":/SaveProject16x16.png"), tr("Save &Project"), this);
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

  actionNewTiledWindow = new QAction(QIcon(getQPixmap("tiledwindow_xpm")), tr("New Tiled &Window"), this);
  actionNewTiledWindow->setShortcut( tr("Ctrl+Shift+T") );
  connect(actionNewTiledWindow, SIGNAL(activated()), this, SLOT(newTiledWindow()));

  actionNewMatrix = new QAction(QIcon(getQPixmap("new_matrix_xpm")), tr("New &Matrix"), this);
  actionNewMatrix->setShortcut( tr("Ctrl+M") );
  connect(actionNewMatrix, SIGNAL(activated()), this, SLOT(newMatrix()));

  actionNewFunctionPlot = new QAction(QIcon(getQPixmap("newF_xpm")), tr("New &Function Plot"), this);
  connect(actionNewFunctionPlot, SIGNAL(activated()), this, SLOT(functionDialog()));

  actionNewSurfacePlot = new QAction(QIcon(getQPixmap("newFxy_xpm")), tr("New 3D &Surface Plot"), this);
  actionNewSurfacePlot->setShortcut( tr("Ctrl+ALT+Z") );
  connect(actionNewSurfacePlot, SIGNAL(activated()), this, SLOT(newSurfacePlot()));

  actionOpenProj=new QAction(QIcon(":/LoadProject16x16.png"), tr("&Project"), this);
  actionOpenProj->setShortcut( tr("Ctrl+Shift+O") );
  connect(actionOpenProj, SIGNAL(activated()), this, SLOT(open()));

  actionLoadFile=new QAction(QIcon(":/Open-icon16x16.png"), tr("Data File"), this);
  actionLoadFile->setShortcut( tr("Ctrl+Shift+F") );
  connect(actionLoadFile, SIGNAL(activated()), this, SLOT(loadDataFile()));

  actionLoadImage = new QAction(tr("Open Image &File"), this);
  actionLoadImage->setShortcut( tr("Ctrl+I") );
  connect(actionLoadImage, SIGNAL(activated()), this, SLOT(loadImage()));

  actionScriptRepo = new QAction(tr("Script Repositor&y"),this);
  connect(actionScriptRepo, SIGNAL(activated()), this, SLOT(loadScriptRepo()));

  actionImportImage = new QAction(tr("Import I&mage..."), this);
  connect(actionImportImage, SIGNAL(activated()), this, SLOT(importImage()));

  actionSaveProjectAs = new QAction(QIcon(":/SaveProject16x16.png"), tr("Save Project &As..."), this);
  connect(actionSaveProjectAs, SIGNAL(activated()), this, SLOT(saveProjectAs()));
  actionSaveProjectAs->setEnabled(false);

  actionSaveNote = new QAction(tr("Save Note As..."), this);
  connect(actionSaveNote, SIGNAL(activated()), this, SLOT(saveNoteAs()));

  actionLoad = new QAction(QIcon(getQPixmap("import_xpm")), tr("&Import ASCII..."), this);
  connect(actionLoad, SIGNAL(activated()), this, SLOT(importASCII()));

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

  actionAddLayer = new QAction(QIcon(getQPixmap("newLayer_xpm")), tr("Add La&yer"), this);
  actionAddLayer->setShortcut( tr("Alt+L") );
  connect(actionAddLayer, SIGNAL(activated()), this, SLOT(addLayer()));

  actionShowLayerDialog = new QAction(QIcon(getQPixmap("arrangeLayers_xpm")), tr("Arran&ge Layers"), this);
  actionShowLayerDialog->setShortcut( tr("Alt+A") );
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

  actionDeleteFitTables = new QAction(QIcon(getQPixmap("close_xpm")), tr("Delete &Fit Tables"), this);
  connect(actionDeleteFitTables, SIGNAL(activated()), this, SLOT(deleteFitTables()));

  actionShowPlotWizard = new QAction(QIcon(getQPixmap("wizard_xpm")), tr("Plot &Wizard"), this);
  actionShowPlotWizard->setShortcut( tr("Ctrl+Alt+W") );
  connect(actionShowPlotWizard, SIGNAL(activated()), this, SLOT(showPlotWizard()));

  actionShowConfigureDialog = new QAction(QIcon(":/configure.png"), tr("&Preferences..."), this);
  connect(actionShowConfigureDialog, SIGNAL(activated()), this, SLOT(showPreferencesDialog()));

  actionShowCurvesDialog = new QAction(QIcon(getQPixmap("curves_xpm")), tr("Add/Remove &Curve..."), this);
  actionShowCurvesDialog->setShortcut( tr("Ctrl+Alt+C") );
  connect(actionShowCurvesDialog, SIGNAL(activated()), this, SLOT(showCurvesDialog()));

  actionAddErrorBars = new QAction(QIcon(getQPixmap("errors_xpm")), tr("Add &Error Bars..."), this);
  actionAddErrorBars->setShortcut( tr("Ctrl+Alt+E") );
  connect(actionAddErrorBars, SIGNAL(activated()), this, SLOT(addErrorBars()));

  actionRemoveErrorBars = new QAction(QIcon(getQPixmap("errors_remove_xpm")), tr("&Remove Error Bars..."), this);
  actionRemoveErrorBars->setShortcut( tr("Ctrl+Alt+R") );
  connect(actionRemoveErrorBars, SIGNAL(activated()), this, SLOT(removeErrorBars()));

  actionAddFunctionCurve = new QAction(QIcon(getQPixmap("fx_xpm")), tr("Add &Function..."), this);
  actionAddFunctionCurve->setShortcut( tr("Ctrl+Alt+F") );
  connect(actionAddFunctionCurve, SIGNAL(activated()), this, SLOT(addFunctionCurve()));

  actionUnzoom = new QAction(QIcon(getQPixmap("unzoom_xpm")), tr("&Rescale to Show All"), this);
  actionUnzoom->setShortcut( tr("Ctrl+Shift+R") );
  connect(actionUnzoom, SIGNAL(activated()), this, SLOT(setAutoScale()));

  actionNewLegend = new QAction(QIcon(getQPixmap("legend_xpm")), tr("New &Legend"), this);
  actionNewLegend->setShortcut( tr("Ctrl+Alt+L") );
  connect(actionNewLegend, SIGNAL(activated()), this, SLOT(newLegend()));

  actionTimeStamp = new QAction(QIcon(getQPixmap("clock_xpm")), tr("Add Time &Stamp"), this);
  actionTimeStamp->setShortcut( tr("Ctrl+ALT+S") );
  connect(actionTimeStamp, SIGNAL(activated()), this, SLOT(addTimeStamp()));

  actionAddImage = new QAction(QIcon(getQPixmap("monalisa_xpm")), tr("Add &Image"), this);
  actionAddImage->setShortcut( tr("Ctrl+Alt+I") );
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

  actionStemPlot = new QAction(QIcon(":/leaf.png"), tr("Stem-and-&Leaf Plot"), this);
  connect(actionStemPlot, SIGNAL(activated()), this, SLOT(newStemPlot()));

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

  // JZ May 3, 2011: Removed this because it segfaults.
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

  actionConvertTableToWorkspace= new QAction(tr("Convert to Table&Workspace"), this);
  connect(actionConvertTableToWorkspace, SIGNAL(activated()), this, SLOT(convertTableToWorkspace()));

  actionConvertTableToMatrixWorkspace= new QAction(tr("Convert to MatrixWorkspace"), this);
  connect(actionConvertTableToMatrixWorkspace, SIGNAL(activated()), this, SLOT(convertTableToMatrixWorkspace()));

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

  actionAskHelp = new QAction(tr("Ask for Help"), this);
  connect(actionAskHelp, SIGNAL(triggered()), this, SLOT(showBugTracker()));

  //actionDownloadManual = new QAction(tr("Download &Manual"), this); // Mantid change
  //connect(actionDownloadManual, SIGNAL(activated()), this, SLOT(downloadManual())); // Mantid change

  //actionTranslations = new QAction(tr("&Translations"), this); // Mantid change
  //connect(actionTranslations, SIGNAL(activated()), this, SLOT(downloadTranslation())); // Mantid change

  //actionDonate = new QAction(tr("Make a &Donation"), this); // Mantid change
  //connect(actionDonate, SIGNAL(activated()), this, SLOT(showDonationsPage())); // Mantid change

  // 	actionTechnicalSupport = new QAction(tr("Technical &Support"), this); // Mantid change
  // 	connect(actionTechnicalSupport, SIGNAL(activated()), this, SLOT(showSupportPage())); // Mantid change

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow = new QAction(getQPixmap("python_xpm"), tr("Toggle &Script Window"), this);
#ifdef __APPLE__
  actionShowScriptWindow->setShortcut(tr("Ctrl+3")); // F3 is used by the window manager on Mac
#else
  actionShowScriptWindow->setShortcut(tr("F3"));
#endif
  actionShowScriptWindow->setToggleAction(true);
  connect(actionShowScriptWindow, SIGNAL(activated()), this, SLOT(showScriptWindow()));

  actionShowScriptInterpreter = new QAction(getQPixmap("python_xpm"), tr("Toggle Script &Interpreter"), this);
#ifdef __APPLE__
  actionShowScriptInterpreter->setShortcut(tr("Ctrl+4")); // F4 is used by the window manager on Mac
#else
  actionShowScriptInterpreter->setShortcut(tr("F4"));
#endif
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

  actionPanPlot = new QAction(QIcon(":/panning.png"), tr("Panning tool"), this);
  connect(actionPanPlot, SIGNAL(activated()), this, SLOT(panOnPlot()));

  actionCatalogLogin  = new QAction("Login",this);
  actionCatalogLogin->setToolTip(tr("Catalog Login"));
  connect(actionCatalogLogin, SIGNAL(activated()), this, SLOT(CatalogLogin()));

  actionCatalogSearch = new QAction("Search",this);
  actionCatalogSearch->setToolTip(tr("Search data in archives."));
  connect(actionCatalogSearch, SIGNAL(activated()), this, SLOT(CatalogSearch()));

  actionCatalogPublish = new QAction("Publish",this);
  actionCatalogPublish->setToolTip(tr("Publish data to the archives."));
  connect(actionCatalogPublish, SIGNAL(activated()), this, SLOT(CatalogPublish()));

  actionCatalogLogout = new QAction("Logout",this);
  actionCatalogLogout->setToolTip(tr("Catalog Logout"));
  connect(actionCatalogLogout, SIGNAL(activated()), this, SLOT(CatalogLogout()));

  actionWaterfallPlot = new QAction(QIcon(":/waterfall_plot.png"), tr("&Waterfall Plot"), this);
  connect(actionWaterfallPlot, SIGNAL(activated()), this, SLOT(waterfallPlot()));

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
  actionNewProject->setToolTip(tr("Open a New Project"));
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

  actionNewTiledWindow->setMenuText(tr("New Tiled &Window"));
  actionNewTiledWindow->setShortcut(tr("Ctrl+Shift+T"));
  actionNewTiledWindow->setToolTip(tr("New tiled window"));

  actionNewMatrix->setMenuText(tr("New &Matrix"));
  actionNewMatrix->setShortcut(tr("Ctrl+M"));
  actionNewMatrix->setToolTip(tr("New matrix"));

  actionNewFunctionPlot->setMenuText(tr("New &Function Plot"));
  actionNewFunctionPlot->setToolTip(tr("Create a new 2D function plot"));

  actionNewSurfacePlot->setMenuText(tr("New 3D &Surface Plot"));
  actionNewSurfacePlot->setToolTip(tr("Create a new 3D surface plot"));
  actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));

  actionOpenProj->setMenuText(tr("&Project"));
  actionOpenProj->setShortcut(tr("Ctrl+Shift+O"));
  actionOpenProj->setToolTip(tr("Load Mantid Project"));

  actionLoadFile->setMenuText(tr("&File"));
  actionLoadFile->setShortcut(tr("Ctrl+Shift+F"));
  actionLoadFile->setToolTip(tr("Load Data File"));


  actionLoadImage->setMenuText(tr("Open Image &File"));
  actionLoadImage->setShortcut(tr("Ctrl+I"));

  actionImportImage->setMenuText(tr("Import I&mage..."));

  actionSaveFile->setMenuText(tr("&Nexus"));
  actionSaveFile->setToolTip(tr("Save as NeXus file"));
  actionSaveFile->setShortcut(tr("Ctrl+S"));

  actionSaveProject->setMenuText(tr("&Project"));
  actionSaveProject->setToolTip(tr("Save Mantid Project"));
  actionSaveProject->setShortcut(tr("Ctrl+Shift+S"));

  actionSaveProjectAs->setMenuText(tr("Save Project &As..."));
  actionSaveProjectAs->setToolTip(tr("Save Mantid Project using a different name or path"));

  actionLoad->setMenuText(tr("&Import ASCII..."));
  actionLoad->setToolTip(tr("Import data file(s)"));
  actionLoad->setShortcut(tr("Ctrl+K"));

  actionCopyWindow->setMenuText(tr("&Duplicate"));
  actionCopyWindow->setToolTip(tr("Duplicate window"));

  actionCutSelection->setMenuText(tr("Cu&t Selection"));
  actionCutSelection->setToolTip(tr("Cut selection"));
  actionCutSelection->setShortcut(tr("Ctrl+X"));

  actionCopySelection->setMenuText(tr("&Copy Selection"));
  actionCopySelection->setToolTip(tr("Copy Selection"));
  actionCopySelection->setShortcut(tr("Ctrl+C"));


  actionPasteSelection->setMenuText(tr("&Paste Selection"));
  actionPasteSelection->setToolTip(tr("Paste Selection"));
  actionPasteSelection->setShortcut(tr("Ctrl+V"));


  actionClearSelection->setMenuText(tr("&Delete Selection"));
  actionClearSelection->setToolTip(tr("Delete selection"));
  actionClearSelection->setShortcut(tr("Del","delete key"));

  actionShowExplorer->setMenuText(tr("Project &Explorer"));
  actionShowExplorer->setShortcut(tr("Ctrl+E"));
  actionShowExplorer->setToolTip(tr("Show project explorer"));

  actionShowLog->setMenuText(tr("Results &Log"));
  actionShowLog->setToolTip(tr("Results Log"));

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow->setMenuText(tr("&Script Window"));
  actionShowScriptWindow->setToolTip(tr("Script Window"));
#endif

  actionCustomActionDialog->setMenuText(tr("Manage Custom Menus..."));

  actionAddLayer->setMenuText(tr("Add La&yer"));
  actionAddLayer->setToolTip(tr("Add Layer"));
  actionAddLayer->setShortcut(tr("Alt+L"));

  actionShowLayerDialog->setMenuText(tr("Arran&ge Layers"));
  actionShowLayerDialog->setToolTip(tr("Arrange Layers"));
  actionShowLayerDialog->setShortcut(tr("Alt+A"));

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

  actionDeleteFitTables->setMenuText(tr("Delete &Fit Tables"));
  actionShowPlotWizard->setMenuText(tr("Plot &Wizard"));
  actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));

  actionShowConfigureDialog->setMenuText(tr("&Preferences..."));

  actionShowCurvesDialog->setMenuText(tr("Add/Remove &Curve..."));
  actionShowCurvesDialog->setShortcut(tr("Ctrl+Alt+C"));
  actionShowCurvesDialog->setToolTip(tr("Add curve to graph"));

  actionAddErrorBars->setMenuText(tr("Add &Error Bars..."));
  actionAddErrorBars->setToolTip(tr("Add Error Bars..."));
  actionAddErrorBars->setShortcut(tr("Ctrl+Alt+E"));

  actionRemoveErrorBars->setMenuText(tr("&Remove Error Bars..."));
  actionRemoveErrorBars->setToolTip(tr("Remove Error Bars..."));
  actionRemoveErrorBars->setShortcut(tr("Ctrl+Alt+R"));

  actionAddFunctionCurve->setMenuText(tr("Add &Function..."));
  actionAddFunctionCurve->setToolTip(tr("Add Function..."));
  actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));

  actionUnzoom->setMenuText(tr("&Rescale to Show All"));
  actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
  actionUnzoom->setToolTip(tr("Rescale to Show All"));

  actionNewLegend->setMenuText( tr("Add New &Legend"));
  actionNewLegend->setShortcut(tr("Ctrl+Alt+L"));
  actionNewLegend->setToolTip(tr("Add New Legend"));

  actionTimeStamp->setMenuText(tr("Add Time &Stamp"));
  actionTimeStamp->setShortcut(tr("Ctrl+Alt+S"));
  actionTimeStamp->setToolTip(tr("Date & time "));

  actionAddImage->setMenuText(tr("Add &Image"));
  actionAddImage->setToolTip(tr("Add Image"));
  actionAddImage->setShortcut(tr("Ctrl+Alt+I"));

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

  actionStemPlot->setMenuText(tr("Stem-and-&Leaf Plot"));
  actionStemPlot->setToolTip(tr("Stem-and-Leaf Plot"));

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
  actionShowColumnValuesDialog->setMenuText(tr("Set Column &Values ...")); // Removed JZ May 3, 2011
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
  actionConvertTableToWorkspace->setMenuText(tr("Convert to Table&Workspace"));
  actionConvertTableToMatrixWorkspace->setMenuText(tr("Convert to MatrixWorkspace"));
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
  actionAskHelp->setText(tr("Ask for Help"));
  //actionDownloadManual->setMenuText(tr("Download &Manual"));//Mantid change - commented out
  //actionTranslations->setMenuText(tr("&Translations"));//Mantid change - commented out
  //actionDonate->setMenuText(tr("Make a &Donation"));
  //actionTechnicalSupport->setMenuText(tr("Technical &Support"));

  btnPointer->setMenuText(tr("Selection &Tools"));
  btnPointer->setToolTip( tr( "Selection Tools" ) );

  btnZoomIn->setMenuText(tr("&Zoom In"));
  btnZoomIn->setShortcut(tr("Ctrl++"));
  btnZoomIn->setToolTip(tr("Zoom In"));

  btnZoomOut->setMenuText(tr("Zoom &Out"));
  btnZoomOut->setShortcut(tr("Ctrl+-"));
  btnZoomOut->setToolTip(tr("Zoom Out"));

  actionPanPlot->setMenuText(tr("Panning Tool (zoom with mouse wheel)"));
  actionPanPlot->setToolTip(tr("Panning Tool (zoom with mouse wheel)"));

  btnCursor->setMenuText(tr("&Data Reader"));
  btnCursor->setShortcut(tr("CTRL+D"));
  btnCursor->setToolTip(tr("Data Reader"));

  btnPicker->setMenuText(tr("S&creen Reader"));
  btnPicker->setToolTip(tr("Screen reader"));

  btnLabel->setMenuText(tr("Add &Label"));
  btnLabel->setToolTip(tr("Add Label"));

  actionDrawPoints->setMenuText(tr("&Draw Data Points"));
  actionDrawPoints->setToolTip(tr("Draw Data Points"));

  btnMovePoints->setMenuText(tr("&Move Data Points..."));
  btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
  btnMovePoints->setToolTip(tr("Move data points"));

  btnRemovePoints->setMenuText(tr("Remove &Bad Data Points..."));
  btnRemovePoints->setShortcut(tr("Alt+B"));
  btnRemovePoints->setToolTip(tr("Remove data points"));

  btnArrow->setMenuText(tr("Draw &Arrow"));
  btnArrow->setShortcut(tr("Ctrl+Alt+A"));
  btnArrow->setToolTip(tr("Draw Arrow"));

  btnLine->setMenuText(tr("Draw Li&ne"));
  btnLine->setShortcut(tr("CtrL+Alt+N"));
  btnLine->setToolTip(tr("Draw Line"));

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

  actionWaterfallPlot->setMenuText(tr("&Waterfall Plot"));
  actionWaterfallPlot->setToolTip(tr("Waterfall Plot"));

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
    m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
    m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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
    m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
    if (!m)
      return 0;
  }

  return plotSpectrogram(m, Graph::Contour);
}

MultiLayer* ApplicationWindow::plotColorMap(Matrix *m)
{
  if (!m) {
    //Mantid
    MultiLayer* plot = mantidUI->plotSpectrogram(Graph::ColorMapContour);
    if (plot) return plot;
    m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
    if (!m)
      return 0;
  }

  return plotSpectrogram(m, Graph::ColorMapContour);
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

  setSpectrogramTickStyle(plot);

  plot->setAutoScale();//Mantid

  QApplication::restoreOverrideCursor();
  return g;
}

void ApplicationWindow::setSpectrogramTickStyle(Graph* g)
{
  //always use the out tick style for colour bar axes
  QList<int> ticksList;
  ticksList<<majTicksStyle<<Graph::Ticks::Out<<majTicksStyle<<majTicksStyle;
  g->setMajorTicksType(ticksList);
  ticksList.clear();
  ticksList<<minTicksStyle<<Graph::Ticks::Out<<minTicksStyle<<minTicksStyle;
  g->setMinorTicksType(ticksList);
  //reset this as the colourbar should now be detectable
  g->drawAxesBackbones(drawBackbones);
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

    // cppcheck-suppress unusedScopedObject
    ImportOPJ(app, filename);

    QApplication::restoreOverrideCursor();
    return app;
  }
  else if (filename.endsWith(".ogm", Qt::CaseInsensitive) || filename.endsWith(".ogw", Qt::CaseInsensitive))
  {
    // cppcheck-suppress unusedScopedObject
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
      QList<Graph *> layers = dynamic_cast<MultiLayer*>(ml)->layersList();
      foreach(Graph *g, layers){
        QList<QwtPlotCurve *> curves = g->fitCurvesList();
        foreach(QwtPlotCurve *c, curves){
          if (dynamic_cast<PlotCurve*>(c)->type() != Graph::Function){
            Table *t = dynamic_cast<DataCurve*>(c)->table();
            if (!t)
              continue;

            t->confirmClose(false);
            t->close();
          }
        }
      }
    }
  }
  delete mLst;
}

QList<MdiSubWindow *> ApplicationWindow::windowsList() const
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

/**
  * Return all windows in all folders.
  */
QList<MdiSubWindow *> ApplicationWindow::getAllWindows() const
{
  QList<MdiSubWindow *> out;
  // get the docked windows first
  QList<QMdiSubWindow*> wl = d_workspace->subWindowList();
  foreach(QMdiSubWindow* w,wl)
  {
    MdiSubWindow* sw = dynamic_cast<MdiSubWindow*>(w->widget());
    if (sw)
    {
      out.append(sw);
    }
  }

  // get the floating windows
  foreach(FloatingWindow* w, m_floatingWindows)
  {
    MdiSubWindow* sw = w->mdiSubWindow();
    if (sw)
    {
      out.append(sw);
    }
  }
  return out;
}


void ApplicationWindow::updateRecentProjectsList()
{
  if (recentProjects.isEmpty())
    return;

  while ((int)recentProjects.size() > MaxRecentProjects)
    recentProjects.pop_back();

  recentProjectsMenu->clear();

  for (int i = 0; i<(int)recentProjects.size(); i++ )
    recentProjectsMenu->insertItem("&" + QString::number(i+1) + " " + recentProjects[i]);
}

void ApplicationWindow::updateRecentFilesList(QString fname)
{
  if (!fname.isEmpty()) {
    recentFiles.remove(fname);
    recentFiles.push_front(fname);
  }
  while ((int)recentFiles.size() > MaxRecentFiles)
    recentFiles.pop_back();

  recentFilesMenu->clear();
  for (int i = 0; i<(int)recentFiles.size(); i++ )
    recentFilesMenu->insertItem("&" + QString::number(i+1) + " " + recentFiles[i]);
}

void ApplicationWindow::translateCurveHor()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), actionReadOnlyCol->isChecked());
}

void ApplicationWindow::setReadOnlyColumns()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]));
}

void ApplicationWindow::setReadWriteColumns()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i=0; i<(int) list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), false);
}

void ApplicationWindow::setAscValues()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setAscValues();
}

void ApplicationWindow::setRandomValues()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setRandomValues();
}

void ApplicationWindow::setXErrCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::xErr);
}

void ApplicationWindow::setYErrCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::yErr);
}

void ApplicationWindow::setXCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::X);
}

void ApplicationWindow::setYCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Y);
}

void ApplicationWindow::setZCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Z);
}

void ApplicationWindow::setLabelCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Label);
}

void ApplicationWindow::disregardCol()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()){
    QMessageBox::warning(this,tr("MantidPlot - Warning"),//Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
            "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph* g = dynamic_cast<Graph*>(plot->activeGraph());
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

void ApplicationWindow::showHomePage()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org"));
}
void ApplicationWindow::showMantidConcepts()
{
  //QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Category:Concepts"));
  HelpWindow::showConcept(this);
}
void ApplicationWindow::showalgorithmDescriptions()
{
  HelpWindow::showAlgorithm(this);
}

void ApplicationWindow::showSetupParaview()
{
  SetUpParaview* dialog = new SetUpParaview(SetUpParaview::MantidMenu);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  dialog->setFocus();
}

void ApplicationWindow::showFirstTimeSetup()
{
  FirstTimeSetup *dialog = new FirstTimeSetup(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  dialog->setFocus();
}

/*
 Show mantidplot help page
 */
void ApplicationWindow::showmantidplotHelp()
{
  HelpWindow::showPage(this);
}

void ApplicationWindow::showBugTracker()
{
  QDesktopServices::openUrl(QUrl("mailto:mantid-help@mantidproject.org"));
}

/*
@param arg: command argument
@return TRUE if argument suggests execution and quiting
*/
bool ApplicationWindow::shouldExecuteAndQuit(const QString& arg)
{
  return arg.endsWith("--execandquit") || arg.endsWith("-xq");
}

/*
@param arg: command argument
@return TRUE if argument suggests a silent startup
*/
bool ApplicationWindow::isSilentStartup(const QString& arg)
{
  return arg.endsWith("--silent") || arg.endsWith("-s");
}

void ApplicationWindow::parseCommandLineArguments(const QStringList& args)
{
  int num_args = args.count();
  if(num_args == 0)
  {
    initWindow();
    savedProject();
    return;
  }

  bool exec(false), quit(false), default_settings(false),
       unknown_opt_found(false);
  QString file_name;
  QString str;
  int filename_argindex(0), counter(0);
  foreach(str, args) {
    if( (str == "-v" || str == "--version") ||
        (str == "-r" || str == "--revision") ||
        (str == "-a" || str == "--about") ||
        (str == "-h" || str == "--help") )
    {
      g_log.warning() << str.ascii() << ": This command line option must be used without other arguments!";
    }
    else if( (str == "-d" || str == "--default-settings"))
    {
      default_settings = true;
    }
    else if (str.endsWith("--execute") || str.endsWith("-x"))
    {
      exec = true;
      quit = false;
    }
    else if (shouldExecuteAndQuit(str))
    {
      exec = true;
      quit = true;
    }
    else if (isSilentStartup(str))
    {
      g_log.debug("Starting in Silent mode");
    }\
    // if filename not found yet then these are all program arguments so we should
    // know what they all are
    else if (file_name.isEmpty() && (str.startsWith("-") || str.startsWith("--")))
    {
      g_log.warning() << "'" << str.ascii() << "' unknown command line option!\n"
                      << "Type 'MantidPlot -h'' to see the list of the valid options.";
      unknown_opt_found = true;
      break;
    }
    else
    {
      // First option that doesn't start "-" is considered a filename and the rest arguments to that file
      if(file_name.isEmpty())
      {
        file_name = str;
        filename_argindex = counter;
      }
    }
    ++counter;
  }

  if(unknown_opt_found || file_name.isEmpty())
  {// no file name given
    initWindow();
    savedProject();
    return;
  }
  else
  {
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

    QStringList cmdArgs = args;
    cmdArgs.erase(cmdArgs.begin(), cmdArgs.begin() + filename_argindex);
    // Set as arguments in script environment
    scriptingEnv()->setSysArgs(cmdArgs);

    if (exec)
    {
      if(quit)
      {
        // Minimize ourselves
        this->showMinimized();
        try
        {
          executeScriptFile(file_name, Script::Asynchronous);
        }
        catch(std::runtime_error& exc)
        {
          std::cerr << "Error thrown while running script file asynchronously '" << exc.what() << "'\n";
          setExitCode(1);
        }
        saved = true;
        this->close();
      }
      else
      {
        loadScript(file_name);
        scriptingWindow->executeCurrentTab(Script::Asynchronous);
      }
    }
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
  OpenProjectDialog* open_dialog = new OpenProjectDialog(this, false);
  open_dialog->setDirectory(workingDir);
  open_dialog->setExtensionWidget(0);

  if(open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
    return;

  workingDir = open_dialog->directory().path();
  appendProject(open_dialog->selectedFiles()[0]);
}

Folder* ApplicationWindow::appendProject(const QString& fn, Folder* parentFolder)
{
  d_opening_file = true;

  QFile file(fn);
  QFileInfo fileInfo(fn);

  file.open(QIODevice::ReadOnly);
  QTextStream fileTS(&file);
  fileTS.setEncoding(QTextStream::UnicodeUTF8);

  QString baseName = fileInfo.fileName();

  //Read version line
  QString versionLine = fileTS.readLine();
  QStringList versionParts = versionLine.split(QRegExp("\\s"), QString::SkipEmptyParts);
  QStringList vl = versionParts[1].split(".", QString::SkipEmptyParts);
  const int fileVersion = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();

  //Skip the <scripting-lang> line. We only really use python now anyway.
  fileTS.readLine();

  //Skip the <windows> line.
  fileTS.readLine();

  folders->blockSignals(true);
  blockSignals(true);

  //Read the rest of the project file in for parsing
  std::string lines = fileTS.readAll().toStdString();

  //Save the selected folder
  Folder* curFolder = currentFolder();

  //Change to parent folder, if given
  if(parentFolder)
    changeFolder(parentFolder, true);

  //Open folders
  openProjectFolder(lines, fileVersion, true);

  //Restore the selected folder
  folders->setCurrentItem(curFolder->folderListItem());
  changeFolder(curFolder, true);

  blockSignals(false);
  folders->blockSignals(false);

  restoreApplicationGeometry();

  d_opening_file = false;

  return 0;
}

void ApplicationWindow::saveProjectFile(Folder *folder, const QString& fn, bool compress)
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

  QString text;

  //Save the list of workspaces
  text += mantidUI->saveToString(workingDir.toStdString());

  if (scriptingWindow)
    text += scriptingWindow->saveToString();

  int windowCount = 0;
  text += saveProjectFolder(folder, windowCount, true);

  text.prepend("<windows>\t"+QString::number(windowCount)+"\n");
  text.prepend("<scripting-lang>\t"+QString(scriptingEnv()->name())+"\n");
  text.prepend("MantidPlot " + QString::number(maj_version)+"."+ QString::number(min_version)+"."+
      QString::number(patch_version)+" project file\n");

  QTextStream t( &f );
  t.setEncoding(QTextStream::UnicodeUTF8);
  t << text;
  f.close();

  if (compress)
  {
    file_compress(fn.ascii(), "w9");
  }

  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::saveAsProject()
{
  saveFolderAsProject(currentFolder());
}

void ApplicationWindow::saveFolderAsProject(Folder *f)
{
  QString filter = tr("MantidPlot project")+" (*.qti);;";//Mantid
  filter += tr("Compressed MantidPlot project")+" (*.qti.gz)";

  QString selectedFilter;
  QString fn = MantidQt::API::FileDialogHandler::getSaveFileName(this, tr("Save project as"), workingDir, filter, &selectedFilter);
  if ( !fn.isEmpty() ){
    QFileInfo fi(fn);
    workingDir = fi.dirPath(true);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fn.append(".qti");

    saveProjectFile(f, fn, selectedFilter.contains(".gz"));
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
  if (dynamic_cast<FolderListItem*>(it)->folder()->parent())
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

  if (dynamic_cast<FolderListItem*>(it)->folder()->parent())
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
    window.addAction(actionNewTiledWindow);
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
  FolderListItem *fi = currentFolder()->folderListItem();
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
    d_current_folder = dynamic_cast<FolderListItem*>(item)->folder();
    FolderListItem *it = d_current_folder->folderListItem();
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

  Folder *parent = dynamic_cast<Folder *>(currentFolder()->parent());
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
  lst.remove(currentFolder()->objectName());
  while(lst.contains(text)){
    QMessageBox::critical(this,tr("MantidPlot - Error"),//Mantid
        tr("Name already exists!")+"\n"+tr("Please choose another name!"));

    it->setRenameEnabled (0, true);
    it->startRename (0);
    return;
  }

  currentFolder()->setObjectName(text);
  it->setRenameEnabled (0, false);
  connect(folders, SIGNAL(currentChanged(Q3ListViewItem *)),
      this, SLOT(folderItemChanged(Q3ListViewItem *)));
  folders->setCurrentItem(parent->folderListItem());//update the list views
}

void ApplicationWindow::showAllFolderWindows()
{
  QList<MdiSubWindow *> lst = currentFolder()->windowsList();
  foreach(MdiSubWindow *w, lst)
  {//force show all windows in current folder
    if (w)
    {
      updateWindowLists(w);
      switch (w->status())
      {
      case MdiSubWindow::Hidden:
        w->setNormal();
        break;

      case MdiSubWindow::Normal:
        w->setNormal();
        break;

      case MdiSubWindow::Minimized:
        w->setMinimized();
        break;

      case MdiSubWindow::Maximized:
        w->setMaximized();
        break;
      }
    }
  }

  if ( (currentFolder()->children()).isEmpty() )
    return;

  FolderListItem *fi = currentFolder()->folderListItem();
  FolderListItem *item = dynamic_cast<FolderListItem *>(fi->firstChild());
  int initial_depth = item->depth();
  while (item && item->depth() >= initial_depth)
  {// show/hide windows in all subfolders
    lst = dynamic_cast<Folder *>(item->folder())->windowsList();
    foreach(MdiSubWindow *w, lst){
      if (w && show_windows_policy == SubFolders){
        updateWindowLists(w);
        switch (w->status())
        {
        case MdiSubWindow::Hidden:
          w->setNormal();
          break;

        case MdiSubWindow::Normal:
          w->setNormal();
          break;

        case MdiSubWindow::Minimized:
          w->setMinimized();
          break;

        case MdiSubWindow::Maximized:
          w->setMaximized();
          break;
        }
      }
      else
        w->hide();
    }

    item = dynamic_cast<FolderListItem *>(item->itemBelow());
  }
}

void ApplicationWindow::hideAllFolderWindows()
{
  QList<MdiSubWindow *> lst = currentFolder()->windowsList();
  foreach(MdiSubWindow *w, lst)
  hideWindow(w);

  if ( (currentFolder()->children()).isEmpty() )
    return;

  if (show_windows_policy == SubFolders)
  {
    FolderListItem *fi = currentFolder()->folderListItem();
    FolderListItem *item = dynamic_cast<FolderListItem *>(fi->firstChild());
    int initial_depth = item->depth();
    while (item && item->depth() >= initial_depth)
    {
      lst = item->folder()->windowsList();
      foreach(MdiSubWindow *w, lst)
      hideWindow(w);

      item = dynamic_cast<FolderListItem*>(item->itemBelow());
    }
  }
}

void ApplicationWindow::projectProperties()
{
  QString s = QString(currentFolder()->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Project")+"\n\n";
  if (projectname != "untitled")
  {
    s += tr("Path") + ": " + projectname + "\n\n";

    QFileInfo fi(projectname);
    s += tr("Size") + ": " + QString::number(fi.size()) + " " + tr("bytes")+ "\n\n";
  }

  s += tr("Contents") + ": " + QString::number(windowsList().size()) + " " + tr("windows");
  s += ", " + QString::number(currentFolder()->subfolders().count()) + " " + tr("folders") + "\n\n";
  s += "\n\n\n";

  if (projectname != "untitled")
  {
    QFileInfo fi(projectname);
    s += tr("Created") + ": " + fi.created().toString(Qt::LocalDate) + "\n\n";
    s += tr("Modified") + ": " + fi.lastModified().toString(Qt::LocalDate) + "\n\n";
  }
  else
    s += tr("Created") + ": " + currentFolder()->birthDate() + "\n\n";

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  //mbox->setIconPixmap(QPixmap( qtiplot_logo_xpm ));
  mbox->show();
}

void ApplicationWindow::folderProperties()
{
  if (!currentFolder()->parent())
  {
    projectProperties();
    return;
  }

  QString s = QString(currentFolder()->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Folder")+"\n\n";
  s += tr("Path") + ": " + currentFolder()->path() + "\n\n";
  s += tr("Size") + ": " + currentFolder()->sizeToString() + "\n\n";
  s += tr("Contents") + ": " + QString::number(currentFolder()->windowsList().count()) + " " + tr("windows");
  s += ", " + QString::number(currentFolder()->subfolders().count()) + " " + tr("folders") + "\n\n";
  //s += "\n\n\n";
  s += tr("Created") + ": " + currentFolder()->birthDate() + "\n\n";
  //s += tr("Modified") + ": " + currentFolder()->modificationDate() + "\n\n";

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  mbox->setIconPixmap(getQPixmap("folder_open_xpm"));
  mbox->show();
}

void ApplicationWindow::addFolder()
{
  if (!explorerWindow->isVisible())
    explorerWindow->show();

  QStringList lst = currentFolder()->subfolders();
  QString name =  tr("New Folder");
  lst = lst.grep( name );
  if (!lst.isEmpty())
    name += " ("+ QString::number(lst.size()+1)+")";

  Folder *f = new Folder(currentFolder(), name);
  addFolderListViewItem(f);

  FolderListItem *fi = new FolderListItem(currentFolder()->folderListItem(), f);
  if (fi){
    f->setFolderListItem(fi);
    fi->setRenameEnabled (0, true);
    fi->startRename(0);
  }
}

Folder* ApplicationWindow::addFolder(QString name, Folder* parent)
{
  if(!parent){
    if (currentFolder())
      parent = currentFolder();
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
    if (currentFolder()){
      if (currentFolder()->parent())
        parent = dynamic_cast<Folder*>(currentFolder()->parent());
    }

    folders->blockSignals(true);

    FolderListItem *fi = f->folderListItem();
    foreach(MdiSubWindow *w, f->windowsList())
    {
      if (!w->close())
      {
        QMessageBox::warning(this,"Mantid - Warning","Folder was not deleted.");
        return false;
      }
    }

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

    d_current_folder = parent;
    folders->setCurrentItem(parent->folderListItem());
    changeFolder(parent, true);
    folders->blockSignals(false);
    folders->setFocus();
    return true;
  }
}

void ApplicationWindow::deleteFolder()
{
  Folder *parent = dynamic_cast<Folder*>(currentFolder()->parent());
  if (!parent)
    parent = projectFolder();

  folders->blockSignals(true);

  if (deleteFolder(currentFolder())){
    d_current_folder = parent;
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

  FolderListItem *item = dynamic_cast<FolderListItem*>(it)->folder()->folderListItem();
  folders->setCurrentItem(item);
}

void ApplicationWindow::folderItemChanged(Q3ListViewItem *it)
{
  if (!it)
    return;

  it->setOpen(true);
  changeFolder (dynamic_cast<FolderListItem*>(it)->folder());
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

  if (currentFolder() == newFolder && !force)
    return false;

  desactivateFolders();
  newFolder->folderListItem()->setActive(true);

  Folder *oldFolder = currentFolder();
  MdiSubWindow::Status old_active_window_state = MdiSubWindow::Normal;
  MdiSubWindow *old_active_window = oldFolder->activeWindow();
  if (old_active_window)
    old_active_window_state = old_active_window->status();

  MdiSubWindow::Status active_window_state = MdiSubWindow::Normal;
  MdiSubWindow *active_window = newFolder->activeWindow();

  if (active_window)
    active_window_state = active_window->status();

  hideFolderWindows(oldFolder);
  d_current_folder = newFolder;

  resultsLog->clear();
  resultsLog->appendInformation(currentFolder()->logInfo());

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
      if(w->status() == MdiSubWindow::Normal || w->status() == MdiSubWindow::Hidden)
      {
        w->setNormal();
      }
      else if(w->status() == MdiSubWindow::Minimized)
        w->setMinimized();
      else if(w->status() == MdiSubWindow::Maximized)
        w->setMaximized();
    }
    //else
    //  w->setStatus(MdiSubWindow::Hidden);

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
              w->setNormal();
            else if (w->status() == MdiSubWindow::Minimized)
              w->setMinimized();
          } else
            w->hide();
        }
      }
      f = f->folderBelow();
    }
  }

  if (active_window){
    setActiveWindow(active_window);
    customMenu(active_window);
    customToolBars(active_window);
    if (active_window_state == MdiSubWindow::Minimized)
      active_window->showMinimized();//ws->setActiveWindow() makes minimized windows to be shown normally
    else if (active_window_state == MdiSubWindow::Maximized){
      if (active_window->isA("Graph3D"))
        static_cast<Graph3D*>(active_window)->setIgnoreFonts(true);
      active_window->showMaximized();
      if (active_window->isA("Graph3D"))
        static_cast<Graph3D*>(active_window)->setIgnoreFonts(false);
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
  FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
  while (item){
    item->setActive(false);
    item = dynamic_cast<FolderListItem*>(item->itemBelow());
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
  else
  {
    it->setText(1, tr("Custom window"));
  }

  it->setText(0, w->objectName());
  it->setText(2, w->aspect());
  it->setText(3, w->sizeToString());
  it->setText(4, w->birthDate());
  it->setText(5, w->windowLabel());
}

void ApplicationWindow::windowProperties()
{
  WindowListItem *it = dynamic_cast<WindowListItem*>(lv->currentItem());
  MdiSubWindow *w = it->window();
  if (!w)
    return;

  QMessageBox *mbox = new QMessageBox ( tr("Properties"), QString(), QMessageBox::NoIcon,
      QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

  QString s = QString(w->objectName()) + "\n\n";
  s += "\n\n\n";

  s += tr("Label") + ": " + static_cast<MdiSubWindow*>(w)->windowLabel() + "\n\n";

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
  s += tr("Path") + ": " + currentFolder()->path() + "\n\n";
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
    MdiSubWindow *w = currentFolder()->findWindow(s, windowNames, labels, caseSensitive, partialMatch);
    if (w){
      activateWindow(w);
      return;
    }

    if (subfolders){
      FolderListItem *item = dynamic_cast<FolderListItem *>(folders->currentItem()->firstChild());
      while (item){
        Folder *f = item->folder();
        MdiSubWindow *w = f->findWindow(s,windowNames,labels,caseSensitive,partialMatch);
        if (w){
          folders->setCurrentItem(f->folderListItem());
          activateWindow(w);
          return;
        }
        item = dynamic_cast<FolderListItem*>(item->itemBelow());
      }
    }
  }

  if (folderNames){
    Folder *f = currentFolder()->findSubfolder(s, caseSensitive, partialMatch);
    if (f){
      folders->setCurrentItem(f->folderListItem());
      return;
    }

    if (subfolders){
      FolderListItem *item = dynamic_cast<FolderListItem *>(folders->currentItem()->firstChild());
      while (item){
        Folder *f = item->folder()->findSubfolder(s, caseSensitive, partialMatch);
        if (f){
          folders->setCurrentItem(f->folderListItem());
          return;
        }

        item = dynamic_cast<FolderListItem*>(item->itemBelow());
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

  Folder *dest_f = dynamic_cast<FolderListItem *>(dest)->folder();

  Q3ListViewItem *it;
  QStringList subfolders = dest_f->subfolders();

  foreach(it, draggedItems){
    if (it->rtti() == FolderListItem::RTTI){
      Folder *f = dynamic_cast<FolderListItem*>(it)->folder();
      FolderListItem *src = f->folderListItem();
      if (dest_f == f){
        QMessageBox::critical(this, "MantidPlot - Error", tr("Cannot move an object to itself!"));//Mantid
        return;
      }

      if (dynamic_cast<FolderListItem *>(dest)->isChildOf(src)){
        QMessageBox::critical(this,"MantidPlot - Error",tr("Cannot move a parent folder into a child folder!"));//Mantid
        draggedItems.clear();
        folders->setCurrentItem(currentFolder()->folderListItem());
        return;
      }

      Folder *parent = dynamic_cast<Folder *>(f->parent());
      if (!parent)
        parent = projectFolder();
      if (dest_f == parent)
        return;

      if (subfolders.contains(f->objectName())){
        QMessageBox::critical(this, tr("MantidPlot") +" - " + tr("Skipped moving folder"),//Mantid
            tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(f->objectName()));
      } else
        moveFolder(src, dynamic_cast<FolderListItem *>(dest));
    } else {
      if (dest_f == currentFolder())
        return;

      MdiSubWindow *w = dynamic_cast<WindowListItem*>(it)->window();
      if (w){
        currentFolder()->removeWindow(w);
        w->hide();
        dest_f->addWindow(w);
        delete it;
      }
    }
  }

  draggedItems.clear();
  d_current_folder = dest_f;
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
          parentFolder = dynamic_cast<Folder*>(parentFolder->parent());
      }
    }
  }
  return true;
}

/**
  Turns 3D animation on or off
 */
void ApplicationWindow::toggle3DAnimation(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
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
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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
      dynamic_cast<Table*>(w)->goToRow(row);
    else if (w->isA("Matrix"))
      (dynamic_cast<Matrix*>(w))->goToRow(row);
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
      dynamic_cast<Table*>(w)->goToColumn(col);
    else if (w->isA("Matrix"))
      (dynamic_cast<Matrix*>(w))->goToColumn(col);
  }
}

/**
 * Show the script window, creating it if necessary
 * @param forceVisible - If true the window is forced to visible rather than toggling
 * @param quitting - If true then it is assumed MantidPlot will exit automatically so stdout redirect
 * from scripts is disabled.
 */
void ApplicationWindow::showScriptWindow(bool forceVisible, bool quitting)
{
  if( !scriptingWindow )
  {
    // MG 09/02/2010 : Removed parent from scripting window. If it has one then it doesn't respect the always on top
    // flag, it is treated as a sub window of its parent
    const bool capturePrint = !quitting;
    scriptingWindow = new ScriptingWindow(scriptingEnv(),capturePrint, NULL);
    scriptingWindow->setObjectName("ScriptingWindow");
    scriptingWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(scriptingWindow, SIGNAL(closeMe()), this, SLOT(saveScriptWindowGeometry()));
    connect(scriptingWindow, SIGNAL(hideMe()), this, SLOT(saveScriptWindowGeometry()));
    connect(scriptingWindow, SIGNAL(hideMe()), this, SLOT(showScriptWindow()));
    connect(scriptingWindow, SIGNAL(chooseScriptingLanguage()), this, SLOT(showScriptingLangDialog()));
  }

  if( forceVisible || scriptingWindow->isMinimized() || !scriptingWindow->isVisible() )
  {
    scriptingWindow->resize(d_script_win_size);
    scriptingWindow->move(d_script_win_pos);
    if( quitting )
    {
      scriptingWindow->showMinimized();
    }
    else
    {
      scriptingWindow->show();
    }
    scriptingWindow->setFocus();
  }
  else
  {
    saveScriptWindowGeometry();
    // Hide is connect to this function so block it temporarily
    scriptingWindow->blockSignals(true);
    scriptingWindow->hide();
    scriptingWindow->blockSignals(false);
  }
}

void ApplicationWindow::saveScriptWindowGeometry()
{
  d_script_win_size = scriptingWindow->size();
  d_script_win_pos = scriptingWindow->pos();
}

void ApplicationWindow::showScriptInterpreter()
{
  if( m_interpreterDock->isVisible() )
  {
    m_interpreterDock->hide();
  }
  else
  {
    m_interpreterDock->show();
    m_interpreterDock->setFocusPolicy(Qt::StrongFocus);
    m_interpreterDock->setFocusProxy(m_interpreterDock->widget());
    m_interpreterDock->setFocus();
    m_interpreterDock->activateWindow();
  }

}

/**
  Turns perspective mode on or off
 */
void ApplicationWindow::togglePerspective(bool on)
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setOrthogonal(!on);
}

/**
  Resets rotation of 3D plots to default values
 */
void ApplicationWindow::resetRotation()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setRotation(30, 0, 15);
}

/**
  Finds best layout for the 3D plot
 */
void ApplicationWindow::fitFrameToLayer()
{
  Graph3D *g = dynamic_cast<Graph3D*>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->findBestLayout();
}

ApplicationWindow::~ApplicationWindow()
{
  delete lastCopiedLayer;
  delete hiddenWindows;
  delete scriptingWindow;
  delete d_text_editor;
  delete catalogSearch;
  while(!d_user_menus.isEmpty())
  {
    QMenu *menu = d_user_menus.takeLast();
    delete menu;
  }
  delete d_current_folder;

  

  btnPointer->setChecked(true);
  delete mantidUI;
}

QString ApplicationWindow::versionString()
{
  QString version(Mantid::Kernel::MantidVersion::version());
  QString date(Mantid::Kernel::MantidVersion::releaseDate());
  return "This is MantidPlot version " + version + " of " + date;
}

void ApplicationWindow::cascade()
{
  const int xoffset = 13;
  const int yoffset = 20;
  int x = 0;
  int y = 0;
  QList<QMdiSubWindow*> windows = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach (QMdiSubWindow *w, windows){
    MdiSubWindow* innerWidget = dynamic_cast<MdiSubWindow*>(w->widget());
    if (!innerWidget)
    {
      throw std::runtime_error("A non-MdiSubWindow detected in the MDI area");
    }
    w->setActiveWindow();
    innerWidget->setNormal();
    w->setGeometry(x, y, w->geometry().width(), w->geometry().height());
    w->raise();
    x += xoffset;
    y += yoffset;
  }
  modifiedProject();
}

/**
*  Load a script file into a new or existing project
*
* @param fn :: is read as a Python script file and loaded in the command script window.
* @param existingProject :: True if loading into an already existing project
*/
ApplicationWindow* ApplicationWindow::loadScript(const QString& fn, bool existingProject)
{
#ifdef SCRIPTING_PYTHON
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setScriptingLanguage("Python");
  restoreApplicationGeometry();
  bool oldScriptingWindow = scriptingWindow;
  showScriptWindow(existingProject, false);
  scriptingWindow->open(fn, existingProject && oldScriptingWindow );
  QApplication::restoreOverrideCursor();
  return this;
#else
  QMessageBox::critical(this, tr("MantidPlot") + " - " + tr("Error"),//Mantid
      tr("MantidPlot was not built with Python scripting support included!"));//Mantid
  return 0;
#endif
}

/**
 *  Runs a script from a file. Mainly useful for automatically running scripts
 * @param filename The full path to the file
 * @param execMode How should the script be executed. If asynchronous
 *                 this method waits on the thread finishing
 */
void ApplicationWindow::executeScriptFile(const QString & filename, const Script::ExecutionMode execMode)
{
  QFile scriptFile(filename);
  if(!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text))
  {
    throw std::runtime_error("Unable to open script file");
  }
  QTextStream in(&scriptFile);
  QString code;
  while (!in.atEnd())
  {
    code += in.readLine() + "\n";
  }
  Script *runner = scriptingEnv()->newScript(filename, this, Script::NonInteractive);
  connect(runner, SIGNAL(finished(const QString &)), this, SLOT(onScriptExecuteSuccess(const QString &)));
  connect(runner, SIGNAL(error(const QString &, const QString &, int)), this, SLOT(onScriptExecuteError(const QString &, const QString &, int)));
  runner->redirectStdOut(false);
  scriptingEnv()->redirectStdOut(false);
  if(execMode == Script::Asynchronous)
  {
    QFuture<bool> job = runner->executeAsync(code);
    while(job.isRunning())
    {
      QCoreApplication::processEvents();
    }
    // Required for windows tests to work
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
  }
  else
  {
    runner->execute(code);
  }
  delete runner;
}

/**
 * This is the slot for handing script exits when it returns successfully
 *
 * @param lineNumber The line number in the script that caused the error.
 */
void ApplicationWindow::onScriptExecuteSuccess(const QString & message)
{
  g_log.notice() << message.toStdString() << "\n";
  this->setExitCode(0);
  this->exitWithPresetCode();
}

/**
 * This is the slot for handing script execution errors. It is only
 * attached by ::executeScriptFile which is only done in the '-xq'
 * command line option.
 *
 * @param message Normally the stacktrace of the error.
 * @param scriptName The name of the file.
 * @param lineNumber The line number in the script that caused the error.
 */
void ApplicationWindow::onScriptExecuteError(const QString & message, const QString & scriptName, int lineNumber)
{
  g_log.fatal() << "Fatal error on line " << lineNumber << " of \"" << scriptName.toStdString()
            << "\" encountered:\n"
            << message.toStdString();
  this->setExitCode(1);
  this->exitWithPresetCode();
}

/**
* Run Python code
* @param code :: An arbitrary string of python code
* @param async :: If true the code will be run asynchronously but only if it is called from the GUI thread
* @param quiet :: If true then no output is produced concerning script start/finished
* @param redirect :: If true redirect stdout/stderr to results log
*/
bool ApplicationWindow::runPythonScript(const QString & code, bool async,
    bool quiet, bool redirect)
{
  if( code.isEmpty() ) return false;

  if( m_iface_script == NULL )
  {
    if( setScriptingLanguage("Python") )
    {
      m_iface_script = scriptingEnv()->newScript("<Interface>", NULL, Script::NonInteractive);
    }
    else
    {
      return false;
    }
  }
  if( !quiet )
  {
    g_log.debug("Script execution started.\n");
  }
  if(redirect)
  {
    m_iface_script->redirectStdOut(true);
    connect(m_iface_script, SIGNAL(print(const QString &)), resultsLog,
            SLOT(appendNotice(const QString&)));
    connect(m_iface_script, SIGNAL(error(const QString &, const QString&, int)),
            resultsLog, SLOT(appendError(const QString &)));

  }
  bool success(false);
  if(async)
  {
    QFuture<bool> job = m_iface_script->executeAsync(code);
    while(job.isRunning())
    {
      QCoreApplication::instance()->processEvents();
    }
    // Ensure the remaining events are processed
    QCoreApplication::instance()->processEvents();
    success = job.result();
  }
  else
  {
    success = m_iface_script->execute(code);
  }
  if (redirect)
  {
    m_iface_script->redirectStdOut(false);
    disconnect(m_iface_script, SIGNAL(print(const QString &)), resultsLog,
               SLOT(appendNotice(const QString&)));
    disconnect(m_iface_script, SIGNAL(error(const QString &, const QString&, int)),
               resultsLog, SLOT(appendError(const QString &)));
  }
  if(success && !quiet)
  {
    g_log.debug("Script execution completed successfully.\n");
  }

  return success;
}



bool ApplicationWindow::validFor2DPlot(Table *table)
{
  if (!table->selectedYColumns().count()){
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Please select a Y column to plot!"));//Mantid
    return false;
  } else if (table->selectedXColumns().count() > 1){
    QMessageBox::warning(this, tr("MantidPlot - Error"), tr("Can't plot using multiple X columns!"));//Mantid
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
    return multilayerPlot(table, table->selectedColumns(), type, sel.topRow(), sel.bottomRow());
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
      hideWindow(dynamic_cast<WindowListItem*>(item)->window());
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
      activateWindow(dynamic_cast<WindowListItem*>(item)->window());
  }
  folders->blockSignals(false);
}

void ApplicationWindow::swapColumns()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList lst = t->selectedColumns();
  if(lst.count() != 2)
    return;

  t->swapColumns(t->colIndex(lst[0]), t->colIndex(lst[1]));
}

void ApplicationWindow::moveColumnRight()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(1);
}

void ApplicationWindow::moveColumnLeft()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(-1);
}

void ApplicationWindow::moveColumnFirst()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(0 - t->selectedColumn());
}

void ApplicationWindow::moveColumnLast()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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

//  QList<MdiSubWindow*> windows = windowsList();
//  foreach(MdiSubWindow *w, windows){
//    if (w->isA("Note"))
//      dynamic_cast<Note*>(w)->setDirPath(path);
//  }
}

void ApplicationWindow::makeToolbarsMenu()
// cppcheck-suppress publicAllocationError
{
  actionFileTools = new QAction(standardTools->windowTitle(), toolbarsMenu);
  actionFileTools->setCheckable(true);
  toolbarsMenu->addAction(actionFileTools);

  actionPlotTools = new QAction(plotTools->windowTitle(), toolbarsMenu);
  actionPlotTools->setCheckable(true);
  toolbarsMenu->addAction(actionPlotTools);

  actionDisplayBar = new QAction(displayBar->windowTitle(), toolbarsMenu);
  actionDisplayBar->setCheckable(true);
  toolbarsMenu->addAction(actionDisplayBar);

  actionFormatToolBar = new QAction(formatToolBar->windowTitle(), toolbarsMenu);
  actionFormatToolBar->setCheckable(true);
  toolbarsMenu->addAction(actionFormatToolBar);
}

void ApplicationWindow::displayToolbars()
{
  actionFileTools->setChecked(d_standard_tool_bar);
  actionPlotTools->setChecked(d_plot_tool_bar);
  actionDisplayBar->setChecked(d_display_tool_bar);
  actionFormatToolBar->setChecked(d_format_tool_bar);
  connect(actionFileTools, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionPlotTools, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionDisplayBar, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionFormatToolBar, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  setToolbars();
}
void ApplicationWindow::setToolbars()
{
  d_standard_tool_bar = actionFileTools->isChecked();
  d_plot_tool_bar = actionPlotTools->isChecked();
  d_display_tool_bar = actionDisplayBar->isChecked();
  d_format_tool_bar = actionFormatToolBar->isChecked();

  MdiSubWindow *w = activeWindow();

  standardTools->setVisible(d_standard_tool_bar);
  plotTools->setVisible(d_plot_tool_bar);
  displayBar->setVisible(d_display_tool_bar);
  formatToolBar->setVisible(d_format_tool_bar);
  plotTools->setEnabled(w && w->isA("MultiLayer"));
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
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->fft();
}

void ApplicationWindow::matrixInverseFFT()
{
  Matrix* m = dynamic_cast<Matrix*>(activeWindow(MatrixWindow));
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

  QSpinBox *sb = dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QSpinBox *sb = dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
  QFont f(font.family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::setItalicFont(bool italic)
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = dynamic_cast<QFontComboBox *>(formatToolBar->widgetForAction(actionFontBox));
  QSpinBox *sb = dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
  QFont f(fb->currentFont().family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(italic);
  g->setCurrentFont(f);
}

void ApplicationWindow::setBoldFont(bool bold)
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph* g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = dynamic_cast<QFontComboBox *>(formatToolBar->widgetForAction(actionFontBox));
  QSpinBox *sb = dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
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
    connect(d_text_editor,SIGNAL(textEditorDeleted()),this,SLOT(cleanTextEditor()));

    formatToolBar->setEnabled(true);
    actionSubscript->setEnabled(true);
    actionSuperscript->setEnabled(true);
    actionUnderline->setEnabled(true);
    actionGreekSymbol->setEnabled(true);
    actionGreekMajSymbol->setEnabled(true);
    actionMathSymbol->setEnabled(true);
  }
}

void ApplicationWindow::cleanTextEditor()
{
  d_text_editor = NULL;
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

void ApplicationWindow::showInterfaceCategoriesDialog()
{
  auto existingWindow = this->findChild<ManageInterfaceCategories *>();
  if( !existingWindow )
  {
    auto * diag = new ManageInterfaceCategories(this);
    diag->setAttribute(Qt::WA_DeleteOnClose);
    diag->show();
    diag->setFocus();
  }
  else
    existingWindow->activateWindow();
}

void ApplicationWindow::showUserDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
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
  if (!action || !(d_user_actions.contains(action) || m_interfaceActions.contains(action)))
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
    runPythonScript(code, false,true);
  code = "";
  while( !stream.atEnd() )
  {
    code.append(stream.readLine() + "\n");
  }
  runPythonScript(code, false, true);
  code = "";
  code.append(QString("\nsys.path.remove(%1)").arg(scriptPath));
    runPythonScript(code, false, true);
}
else
{
  // Check to see if the window is already open.  If so, just show it to the user.
  foreach( auto userSubWindow, this->findChildren<UserSubWindow *>() )
  {
    if( userSubWindow->objectName() == action_data )
    {
      userSubWindow->activateWindow();
      return;
    }
  }
      // Enables/Disables the toolbar

  MdiSubWindow* usr_win = new MdiSubWindow(this);
  usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::UserSubWindow *user_interface = interfaceManager.createSubWindow(action_data, usr_win);
  if(user_interface)
  {
    setGeometry(usr_win,user_interface);
    connect(user_interface, SIGNAL(runAsPythonScript(const QString&, bool)),
                      this, SLOT(runPythonScript(const QString&, bool)), Qt::DirectConnection);
    // Update the used fit property browser
    connect(user_interface, SIGNAL(setFitPropertyBrowser(MantidQt::MantidWidgets::FitPropertyBrowser*)),
                  mantidUI, SLOT(setFitFunctionBrowser(MantidQt::MantidWidgets::FitPropertyBrowser*)));
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
  myMenuBar()->insertItem(tr(topMenu), customMenu);
}

void ApplicationWindow::addUserMenuAction(const QString & parentMenu, const QString & itemName, const QString & itemData)
{
	QString niceName = QString(itemName).replace("_", " ");
  QMenu* topMenu(NULL);
  foreach(topMenu, d_user_menus)
  {
    if( topMenu->title() == parentMenu ) break;
  }

  if( !topMenu ) return;
  foreach(QAction* userAction, topMenu->actions())
  {
    if( userAction->text() == niceName ) return;
  }

  QAction* scriptAction = new QAction(tr(niceName), topMenu);
  scriptAction->setData(itemData);
  topMenu->addAction(scriptAction);
  d_user_actions.append(scriptAction);

  // Remove name from the list of removed interfaces if applicable
  removed_interfaces.remove(niceName);
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
  myMenuBar()->removeAction(menu->menuAction());
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
      lst << static_cast<QMenu *>(w);
  }
  return lst;
}

// End of a section of Mantid custom functions
//-------------------------------------------

QList<QToolBar *> ApplicationWindow::toolBarsList() const
{
  QList<QToolBar *> lst;
  QObjectList children = this->children();
  foreach (QObject *w, children){
    if (w->isA("QToolBar"))
      lst << static_cast<QToolBar *>(w);
  }
  return lst;
}

void ApplicationWindow::hideSelectedColumns()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->hideSelectedColumns();
}

void ApplicationWindow::showAllColumns()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
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
        QUndoStack *stack = (dynamic_cast<Matrix*>(w))->undoStack();
        if (!stack->count())// undo limit can only be changed for empty stacks
          stack->setUndoLimit(size);
      }
    }
    f = f->folderBelow();
  }
}

/**
 * Arange the mdi sub-windows in a tile pattern
 */
void ApplicationWindow::tileMdiWindows()
{
  d_workspace->tileSubWindows();
  // hack to redraw the graphs
  shakeViewport();
  // QMdiArea::tileSubWindows() aranges the windows and enables automatic tiling
  // after subsequent resizing of the mdi area until a window is moved or resized
  // separatly. Unfortunately Graph behaves badly during this.
  // The following code disables automatic tiling.
  auto winList = d_workspace->subWindowList();
  if ( !winList.isEmpty() )
  {
    auto p = winList[0]->pos();
    winList[0]->move(p.x()+1,p.y());
    winList[0]->move(p);
  }
}

/**
 * A hack to make the mdi area and the Graphs to redraw themselves in certain cases.
 */
void ApplicationWindow::shakeViewport()
{
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

    else if (g->areRangeSelectorsOn()){}
    else if (dynamic_cast<PeakPickerTool*>(tool))
      btnMultiPeakPick->setOn(true);
    else if (dynamic_cast<DataPickerTool*>(tool))
    {
      switch(dynamic_cast<DataPickerTool*>(tool)->getMode())
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
    else if (dynamic_cast<LabelTool*>(tool))
      btnLabel->setOn(true);
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
  if(fileName.empty()) return ;
  try
  {
    if(mantidUI)mantidUI->savedatainNexusFormat(fileName,wsName);
  }
  catch(...)
  {
  }
}

void ApplicationWindow::enableSaveNexus(const QString &wsName)
{
  if (actionSaveFile) actionSaveFile->setEnabled(true);

  m_nexusInputWSName=wsName;
}

void ApplicationWindow::disableSaveNexus()
{
  if (actionSaveFile)
    actionSaveFile->setEnabled(false);
}

/* For zooming the selected graph using the drag canvas tool and mouse drag.
 */
void ApplicationWindow::panOnPlot()
{
  MultiLayer *plot = dynamic_cast<MultiLayer*>(activeWindow(MultiLayerWindow));
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
void ApplicationWindow::populateCatalogLoginMenu()
{
  icat->clear();
  icat->addAction(actionCatalogLogin);
  if(Mantid::API::CatalogManager::Instance().numberActiveSessions() > 0)
  {
    icat->addAction(actionCatalogSearch);
    icat->addAction(actionCatalogPublish);
    icat->addAction(actionCatalogLogout);
  }
}

void ApplicationWindow::CatalogLogin()
{
  MantidQt::MantidWidgets::CatalogHelper().showLoginDialog();
}

void ApplicationWindow::CatalogSearch()
{
  if (catalogSearch == NULL || catalogSearch)
  {
    // Only one ICAT GUI will appear, and that the previous one will be overridden.
    // E.g. if a user opens the ICAT GUI without being logged into ICAT they will need to
    // login in and then click "Search" again.
    delete catalogSearch;
    catalogSearch = new MantidQt::MantidWidgets::CatalogSearch();

    catalogSearch->show();
    catalogSearch->raise();
  }
}

void ApplicationWindow::CatalogPublish()
{
  MantidQt::MantidWidgets::CatalogHelper().showPublishDialog();
}

void ApplicationWindow::CatalogLogout()
{
  auto logout = mantidUI->createAlgorithm("CatalogLogout");
  mantidUI->executeAlgorithmAsync(logout);
  icat->removeAction(actionCatalogSearch);
  icat->removeAction(actionCatalogPublish);
  icat->removeAction(actionCatalogLogout);
}

void ApplicationWindow::setGeometry(MdiSubWindow* usr_win,QWidget* user_interface)
{
  QRect frame = QRect(usr_win->frameGeometry().topLeft() - usr_win->geometry().topLeft(),
      usr_win->geometry().bottomRight() - usr_win->geometry().bottomRight());
  usr_win->setWidget(user_interface);
  QRect iface_geom = QRect(frame.topLeft() + user_interface->geometry().topLeft(),
      frame.bottomRight() + user_interface->geometry().bottomRight());
  usr_win->setGeometry(iface_geom);
  usr_win->setName(user_interface->windowTitle());
  addMdiSubWindow(usr_win);
}

/**
 * Write a message to the log window. The message priority will be information
 * or error if error=true
 * @param message :: A string containing the message
 * @param error :: A boolean indicating if this is an error
 */
void ApplicationWindow::writeToLogWindow(const MantidQt::API::Message & msg)
{
  resultsLog->append(msg);
}


MultiLayer* ApplicationWindow::waterfallPlot()
{
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return 0;

  return waterfallPlot(t, t->selectedYColumns());
}

MultiLayer* ApplicationWindow::waterfallPlot(Table *t, const QStringList& list)
{
  if (!t)
    return 0;

  if(list.count() < 1){
    QMessageBox::warning(this, tr("MantidPlot - Plot error"),tr("Please select a Y column to plot!"));
    return 0;
  }

  MultiLayer* ml = new MultiLayer(this);

  Graph *g = ml->activeGraph();//Layer();
  setPreferences(g);
  g->enableAxis(QwtPlot::xTop, false);
  g->enableAxis(QwtPlot::yRight, false);
  g->setCanvasFrame(0);
  g->setTitle(QString::null);
  g->setMargin(0);
  g->setFrame(0);
  g->addCurves(t, list, Graph::Line);
  g->setWaterfallOffset(10, 20);

  initMultilayerPlot(ml);
  ml->arrangeLayers(false, true);
  ml->hide(); // RJT: Fix for window not displaying properly prior to a resize
  ml->setWaterfallLayout();

  g->newLegend()->move(QPoint(g->x() + g->plotWidget()->canvas()->x() + 5, 5));

  ml->show(); // RJT: Fix for window not displaying properly prior to a resize
  return ml;
}

/**
 * Add a sub-window either as a docked or a floating window. The desision is made by isDefalutFloating() method.
 * @param w :: Pointer to a MdiSubWindow which to add.
 * @param showNormal :: If true (default) show as a normal window, if false show as a minimized docked window
 *   regardless of what isDefalutFloating() returns.
 */
void ApplicationWindow::addMdiSubWindow(MdiSubWindow *w, bool showNormal)
{
  addMdiSubWindow(w, isDefaultFloating(w), showNormal);
}

/**
 * Add a sub-window either as a docked or a floating window.
 * @param w :: Pointer to a MdiSubWindow which to add.
 * @param showFloating :: If true show as floating else make it docked.
 * @param showNormal :: If true show as a normal window, if false show as a minimized docked window
 *   regardless of what showFloating is.
 */
void ApplicationWindow::addMdiSubWindow(MdiSubWindow *w, bool showFloating, bool showNormal)
{
  connect(w, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
  connect(w, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
  connect(w, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
  connect(w, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
  connect(w, SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
  connect(w, SIGNAL(showContextMenu()), this, SLOT(showWindowContextMenu()));
  connect(w, SIGNAL(detachFromParent(MdiSubWindow*)),this, SLOT(detachMdiSubwindow(MdiSubWindow*)));

  if (showFloating && showNormal)
  {
    addMdiSubWindowAsFloating(w);
  }
  else
  {
    QMdiSubWindow* sw = addMdiSubWindowAsDocked(w);
    if (showNormal)
    {
      sw->showNormal();
    }
    else
    {
      sw->showMinimized();
    }
  }

  addListViewItem(w);
  currentFolder()->addWindow(w);
}

/**
 * Add a sub-window to as a floating window.
 * @param w :: Pointer to a MdiSubWindow which will be wrapped in a FloatingWindow.
 * @param pos :: Position of created window relative to the main window.
 *   Setting it to (-1,-1) means no autogenerate the position.
 */
FloatingWindow* ApplicationWindow::addMdiSubWindowAsFloating(MdiSubWindow* w, QPoint pos)
{
  const QPoint none(-1,-1);
  FloatingWindow* fw =new FloatingWindow(this);//, Qt::WindowStaysOnTopHint);
  QSize sz = w->size();
  if (pos == none)
  {
    pos = positionNewFloatingWindow(sz);
  }
  else
  {
    pos += mdiAreaTopLeft();
  }
  fw->setWindowTitle(w->name());
  fw->setMdiSubWindow(w);
  fw->resize(sz);
  fw->move(pos);
  m_floatingWindows.append(fw); // do it before show
  fw->show();
  return fw;
}

/**
  * Returns the top-left corner of the ADI area available for sub-windows relative
  * to the top-left corner of the monitor screen.
  *
  */
QPoint ApplicationWindow::mdiAreaTopLeft() const
{
  QPoint p = this->pos() + d_workspace->pos();

  // make sure the floating window doesn't overlap the tool bars
  QList<QToolBar *> toolBars = toolBarsList();
  foreach(QToolBar* bar,toolBars)
  {
    if (toolBarArea(bar) != Qt::TopToolBarArea) continue;
    int y = this->pos().y() + d_workspace->pos().y() + bar->rect().bottom();
    if (y > p.y()) p.setY(y + 1);
  }
  return p;
}

/**
  * Find the best position for a new floating window.
  * @param sz :: Size of the new window.
  */
QPoint ApplicationWindow::positionNewFloatingWindow(QSize sz) const
{
  const QPoint noPoint(-1,-1);

  static QPoint lastPoint(noPoint);

  if ( lastPoint == noPoint || m_floatingWindows.isEmpty() )
  { // If no other windows added - start from top-left corner
    lastPoint = mdiAreaTopLeft();
  }
  else
  {
    // Get window which was added last
    FloatingWindow* lastWindow = m_floatingWindows.last();

    if ( lastWindow->isVisible() )
    { // If it is still visibile - can't use it's location, so need to find a new one

      QPoint diff = lastWindow->pos() - lastPoint;

      if ( abs(diff.x()) < 20 && abs(diff.y()) < 20 )
      { // If window was moved far enough from it's previous location - can use it

        // Get a screen space which we can use
        const QRect screen = QApplication::desktop()->availableGeometry(this);

        // How mush we need to move in X so that cascading direction is diagonal according to
        // screen size
        const int yDelta = 40;
        const int xDelta = static_cast<int>( yDelta * ( 1.0 * screen.width() / screen.height() ) );

        lastPoint += QPoint(xDelta, yDelta);

        const QRect newPlace = QRect(lastPoint, sz);
        if ( newPlace.bottom() > screen.height() || newPlace.right() > screen.width() )
          // If new window doesn't fit to the screen - start anew
          lastPoint = mdiAreaTopLeft();
      }
    }
  }

  return lastPoint;

}

/**
 * Add a sub-window to as a docked MDI window.
 * @param w :: Pointer to a MdiSubWindow which will be wrapped in a QMdiSubWindow.
 * @param pos :: Position of the top-left corner of the new window.
 */
QMdiSubWindow* ApplicationWindow::addMdiSubWindowAsDocked(MdiSubWindow* w, QPoint pos)
{
  DockedWindow *dw = new DockedWindow(this);
  dw->setMdiSubWindow(w);
  QMdiSubWindow* sw = this->d_workspace->addSubWindow(dw);
  sw->resize(w->size());
  sw->setWindowIcon(w->windowIcon());
  if ( pos != QPoint(-1,-1) )
  {
    sw->move(pos);
  }
  return sw;
}

/**
 * Make a subwindow floating.
 */
void ApplicationWindow::changeToFloating(MdiSubWindow* w)
{
  if ( w->isFloating() ) return;
  QMdiSubWindow* sw =w->getDockedWindow();
  if ( sw )
  {
    // remove the subwindow from the mdi area
    d_workspace->removeSubWindow(w);
    sw->close();
    // create the outer floating window.
    addMdiSubWindowAsFloating(w,sw->pos());
  }
  else
  {
    // attach w to the ApplicationWindow and create the outer floating window (second argument == true)
    addMdiSubWindow(w,true,true);
  }
  activateWindow(w);
}

/**
 * Return a floating subwindow to the mdi area.
 */
void ApplicationWindow::changeToDocked(MdiSubWindow* w)
{
  if ( w->isDocked() ) return;
  FloatingWindow* fw = w->getFloatingWindow();
  if ( fw )
  {
    fw->removeMdiSubWindow();
    removeFloatingWindow(fw);
    // main window must be closed or application will freeze
    fw->close();
    // create the outer docked window.
    addMdiSubWindowAsDocked(w);
  }
  else
  {
    // attach w to the ApplicationWindow and create the outer docked window (second argument == false)
    addMdiSubWindow(w,false,true);
  }
  w->setNormal();
  return;
}

/**
 * Remove a closed floating window from internal lists.
 * @param w :: Pointer to the closed window.
 */
void ApplicationWindow::removeFloatingWindow(FloatingWindow* w)
{
  if (m_floatingWindows.contains(w))
  {
    m_floatingWindows.remove(w);
    if (w->mdiSubWindow())
    {
      closeWindow(w->mdiSubWindow());
    }
    // Make the FloatingWindow delete itself
    w->deleteLater();
  }
}

/**
 * Return a pointer to the active FloatingWindow if the active window is floating
 * or NULL otherwise.
 */
FloatingWindow* ApplicationWindow::getActiveFloating() const
{
  MdiSubWindow* w = getActiveWindow();
  if (!w) return NULL;
  return w->getFloatingWindow();
}

/**
 * Detach a subwindow from its parent - docked or floating.
 * It isn't full detachment - signals are still connected.
 */
void ApplicationWindow::detachMdiSubwindow(MdiSubWindow* w)
{
  // remove the window from all internal lists
  if ( currentFolder()->hasWindow(w) )
  {
    currentFolder()->removeWindow(w);
  }
  removeWindowFromLists(w);
  Q3ListViewItem *it=lv->findItem (w->objectName(), 0, Q3ListView::ExactMatch|Q3ListView::CaseSensitive);
  if (it)
    lv->takeItem(it);

  // if it's wrapped in a floating detach from it and close
  FloatingWindow *fw = w->getFloatingWindow();
  if ( fw )
  {
    fw->removeMdiSubWindow();
    m_floatingWindows.remove(fw);
    fw->deleteLater();
    return;
  }

  // the same in docked case
  QMdiSubWindow *dw = w->getDockedWindow();
  if ( dw )
  {
    d_workspace->removeSubWindow(w);
    dw->close();
  }
}

/**
 * Filter out the WindowActivate event and set the active subwindow correctly.
 * @param e :: An event.
 */
bool ApplicationWindow::event(QEvent * e)
{
  if (e->type() == QEvent::WindowActivate)
  {
    bool needToActivate = true;

    // check if old active window is a floating one and this window was activated by clicking
    // on a tool bar - in this case we shouldn't actvate another window
    if (getActiveFloating())
    {

      QPoint cur_pos = this->mapFromGlobal(QCursor::pos());
      const QWidget* clickedWidget = NULL;

      if (rect().contains(cur_pos))
      {
        clickedWidget = childAt(cur_pos);
      }

      if (clickedWidget)
      {
        QString class_name = clickedWidget->className();
        if (class_name == "QToolButton" || class_name == "QToolBar" || class_name == "QMenuBar")
        {
          needToActivate = false;
        }
      }
    }

    if (needToActivate)
    {// activate current MDI subwindow
      QMdiSubWindow* qCurrent = d_workspace->currentSubWindow();
      if (qCurrent)
      {
        QWidget *wgt = qCurrent->widget();
        MdiSubWindow* sw = dynamic_cast<MdiSubWindow*>(wgt);
        if (!sw)
        {// this should never happen - all MDI subwindow widgets must inherit from MdiSubwindow
          throw std::logic_error("Non-MdiSubwindow widget found in MDI area");
        }
        activateWindow(sw);
      }
    }
  }
  return QMainWindow::event(e);
}

/**
 * Necessary steps to activate a floating window.
 * @param w :: Activated window
 */
void ApplicationWindow::mdiWindowActivated(MdiSubWindow* w)
{
  if (!w) return;
  setActiveWindow(w);
}

/**
 * Activate a subwindow (docked or floating) other than current active one.
 * This is required when the current window is closing.
 */
void ApplicationWindow::activateNewWindow()
{
  MdiSubWindow* current = getActiveWindow();
  MdiSubWindow* newone = NULL;
  Folder* folder = currentFolder();

  // try the docked windows first
  QList<QMdiSubWindow*> wl = d_workspace->subWindowList(QMdiArea::ActivationHistoryOrder);
  if ( !wl.isEmpty() )
  {
    for( int i = wl.size() - 1; i >= 0; --i )
    {
      QMdiSubWindow *w = wl[i];
      if (w->widget() != static_cast<QWidget*>(current))
      {
        MdiSubWindow* sw = dynamic_cast<MdiSubWindow*>(w->widget());
          if (sw &&
              sw->status() != MdiSubWindow::Minimized &&
              sw->status() != MdiSubWindow::Hidden &&
              folder->hasWindow(sw))
        {
          newone = sw;
          break;
        }
      }
    }
  }

  // if unsuccessful try the floating windows
  if (!newone)
  {
    foreach(FloatingWindow* w, m_floatingWindows)
    {
      MdiSubWindow* sw = w->mdiSubWindow();
      if (sw != current)
      {
        if (sw &&
            sw->status() != MdiSubWindow::Minimized &&
            sw->status() != MdiSubWindow::Hidden &&
            folder->hasWindow(sw))
        {
          newone = sw;
          break;
        }
      }
    }
  }
  // activate a new sub-window or pass NULL if no window can be activated
  activateWindow(newone);
}

/**
 * The slot to change the active window from docked to floating.
 */
void ApplicationWindow::changeActiveToFloating()
{
  MdiSubWindow* activeWin = activeWindow();
  changeToFloating(activeWin);
}

/**
 * The slot to change the active window from floating to docked.
 */
void ApplicationWindow::changeActiveToDocked()
{
  MdiSubWindow* activeWin = activeWindow();
  changeToDocked(activeWin);
}

/**
 * Returns if a window should be made floating by default.
 * @param w :: Pointer to a MdiSubWindow.
 */
bool ApplicationWindow::isDefaultFloating(const MdiSubWindow* w) const
{
  QString wClassName = w->className();
  return isDefaultFloating(wClassName);
}

/**
 * Returns if a window should be made floating by default.
 * @param aClassName :: Class name of a MdiSubWindow or its internal widget in case of custom interfaces.
 */
bool ApplicationWindow::isDefaultFloating(const QString& aClassName) const
{
  bool theDefault = false;
#ifndef Q_OS_LINUX
  if (aClassName == "MultiLayer" || aClassName =="InstrumentWindow" || aClassName == "MdiSubWindow")
  {
    theDefault = true;
  }
#endif
  return settings.value("/General/FloatingWindows/"+aClassName,theDefault).toBool();
}


/**
 * Check that a widow will be visible if moved to these coordinates and
 * set them to default values otherwise.
 * @param w :: Pointer to a sub-window.
 * @param x :: Tested x coordinate
 * @param y :: Tested y coordinate
 */
void ApplicationWindow::validateWindowPos(MdiSubWindow* w, int& x, int& y)
{
  QSize sz = w->size();
  if ( w->getFloatingWindow() )
  {
    QWidget* desktop = QApplication::desktop()->screen();
    QPoint pos(x, y);
    pos += mdiAreaTopLeft();
    if ( pos.x() < 0 || pos.y() < 0 ||
      pos.x() + sz.width() > desktop->width() ||
      pos.y() + sz.height() > desktop->height() )
    {
      pos = positionNewFloatingWindow(sz);
    }
    x = pos.x();
    y = pos.y();
    return;
  }
  else if ( x < 0 || y < 0 ||
    x + sz.width() > d_workspace->width() ||
    y + sz.height() > d_workspace->height() )
  {
    x = y = 0;
  }
}

/**
 * Here it is possible to add all the methods that should be triggered
 * on MantidPlot initialization but which requires the eventloop to be
 * processing.
 *
 * Currently:
 *  - Update of Script Repository
 */
void ApplicationWindow::about2Start(){
  // triggers the execution of UpdateScriptRepository Algorithm in a separated thread.
  // this was necessary because in order to log while in a separate thread, it is necessary to have
  // the postEvents available, so, we need to execute it here at about2Start.
  std::string local_rep = Mantid::Kernel::ConfigService::Instance().getString("ScriptLocalRepository");
  if (!local_rep.empty()){
    // there is no reason to trigger UpdataScriptRepository if it has never been installed
    Mantid::API::IAlgorithm_sptr update_script_repo = mantidUI->createAlgorithm("UpdateScriptRepository");
    update_script_repo->initialize();
    update_script_repo->setLoggingOffset(1);
    mantidUI->executeAlgorithmAsync(update_script_repo);
  }

  // Make sure we see all of the startup messages
  resultsLog->scrollToTop();
}

/**
 * Create a new TiledWindow with default settings.
 */
TiledWindow *ApplicationWindow::newTiledWindow()
{
  TiledWindow *widget = new TiledWindow(this,"",generateUniqueName("TiledWindow"),2,2);
  addMdiSubWindow( widget );
  return widget;
}

/**
 * Check if there is an open TiledWindow.
 */
bool ApplicationWindow::hasTiledWindowOpen()
{
  // check the docked windows
  auto wl = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach( QMdiSubWindow *w, wl )
  {
    TiledWindow *tw = dynamic_cast<TiledWindow*>( w->widget() );
    if ( tw && tw->isVisible() )
    {
      return true;
    }
  }
  // check the floating windows
  foreach( FloatingWindow *w, m_floatingWindows )
  {
    TiledWindow *tw = dynamic_cast<TiledWindow*>( w->mdiSubWindow() );
    if ( tw && tw->isVisible() )
    {
      return true;
    }
  }
  return false;
}

/**
 * Return a pointer to the topmost TiledWindow that contains a point.
 * If the TiledWindow is overlapped by another window return NULL.
 * If there is no TiledWindows or the point doesn't fall inside
 * of any of them return NULL.
 *
 * @param x :: The x-coord to check (in global coordinates).
 * @param y :: The y-coord to check (in global coordinates).
 */
TiledWindow *ApplicationWindow::getTiledWindowAtPos( QPoint pos )
{
  // check the docked windows
  auto wl = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach( QMdiSubWindow *w, wl )
  {
    TiledWindow *tw = dynamic_cast<TiledWindow*>( w->widget() );
    if ( tw )
    {
      QPoint mdiOrigin = mapFromGlobal( pos );
      auto r = w->visibleRect();
      r.moveBy( mdiOrigin.x(), mdiOrigin.y() );
      if ( r.contains(pos) )
      {
        return tw;
      }
    }
  }
  // check the floating windows
  foreach(FloatingWindow *w, m_floatingWindows)
  {
    TiledWindow *tw = dynamic_cast<TiledWindow*>( w->mdiSubWindow() );
    if ( tw )
    {
      QPoint mdiOrigin = mapFromGlobal( pos );
      auto r = w->visibleRect();
      r.moveBy( mdiOrigin.x(), mdiOrigin.y() );
      if ( r.contains(pos) )
      {
        return tw;
      }
    }
  }
  return NULL;
}

/**
 * Check if a point is inside any of visible TiledWindows.
 * @param x :: The x-coord to check (in global coordinates).
 * @param y :: The y-coord to check (in global coordinates).
 */
bool ApplicationWindow::isInTiledWindow( QPoint pos )
{
  auto w = getTiledWindowAtPos( pos );
  if ( w != NULL )
  {
    w->showInsertPosition( pos );
    return true;
  }
  return false;
}

/**
 * @param w :: An MdiSubWindow.
 * @param x :: The x-coord to check (in global coordinates).
 * @param y :: The y-coord to check (in global coordinates).
 */
void ApplicationWindow::dropInTiledWindow( MdiSubWindow *w, QPoint pos )
{
  auto tw = getTiledWindowAtPos( pos );
  if ( tw != NULL )
  {
    tw->dropAtPosition( w, pos );
  }
}

QString ApplicationWindow::saveProjectFolder(Folder* folder, int &windowCount, bool isTopLevel)
{
  QString text;

  //Write the folder opening tag
  if(!isTopLevel)
  {
    text += "<folder>\t" + QString(folder->objectName()) + "\t" + folder->birthDate() + "\t" + folder->modificationDate();

    if(folder == currentFolder())
      text += "\tcurrent";
    text += "\n";
    text += "<open>" + QString::number(folder->folderListItem()->isOpen()) + "</open>\n";
  }

  //Write windows
  QList<MdiSubWindow*> windows = folder->windowsList();
  foreach(MdiSubWindow* w, windows)
  {
    Mantid::IProjectSerialisable* ips = dynamic_cast<Mantid::IProjectSerialisable*>(w);
    if(ips)
      text += QString::fromUtf8(ips->saveToProject(this).c_str());

    ++windowCount;
  }

  //Write subfolders
  QList<Folder*> subfolders = folder->folders();
  foreach(Folder* f, subfolders)
  {
    text += saveProjectFolder(f, windowCount);
  }

  //Write log info
  if(!folder->logInfo().isEmpty())
    text += "<log>\n" + folder->logInfo() + "</log>\n";

  //Write the folder closing tag
  if(!isTopLevel)
  {
    text += "</folder>\n";
  }

  return text;
}
