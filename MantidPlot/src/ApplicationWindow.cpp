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
#include "ApplicationWindow.h"
#include "ArrowMarker.h"
#include "AssociationsDialog.h"
#include "AxesDialog.h"
#include "ColorBox.h"
#include "ColorMapDialog.h"
#include "ConfigDialog.h"
#include "Convolution.h"
#include "Correlation.h"
#include "CurveRangeDialog.h"
#include "CurvesDialog.h"
#include "CustomActionDialog.h"
#include "DataPickerTool.h"
#include "DataSetDialog.h"
#include "Differentiation.h"
#include "DockedWindow.h"
#include "ErrDialog.h"
#include "ExpDecayDialog.h"
#include "ExportDialog.h"
#include "FFTDialog.h"
#include "FFTFilter.h"
#include "FilterDialog.h"
#include "FindDialog.h"
#include "Fit.h"
#include "FitDialog.h"
#include "FloatingWindow.h"
#include "Folder.h"
#include "FunctionCurve.h"
#include "FunctionDialog.h"
#include "Graph.h"
#include "Graph3D.h"
#include "Grid.h"
#include "ImageDialog.h"
#include "ImageExportDialog.h"
#include "ImageMarker.h"
#include "ImportASCIIDialog.h"
#include "IntDialog.h"
#include "Integration.h"
#include "InterpolationDialog.h"
#include "LayerDialog.h"
#include "LegendWidget.h"
#include "LineDialog.h"
#include "LogisticFit.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/LegacyQwt/ScaleEngine.h"
#include "MatrixCommand.h"
#include "MatrixDialog.h"
#include "MatrixModel.h"
#include "MatrixSizeDialog.h"
#include "MatrixValuesDialog.h"
#include "MdiSubWindow.h"
#include "MultiLayer.h"
#include "MultiPeakFit.h"
#include "NonLinearFit.h"
#include "Note.h"
#include "OpenProjectDialog.h"
#include "Plot.h"
#include "Plot3DDialog.h"
#include "PlotDialog.h"
#include "PlotWizard.h"
#include "PolynomFitDialog.h"
#include "PolynomialFit.h"
#include "ProjectRecovery.h"
#include "ProjectSerialiser.h"
#include "QwtErrorPlotCurve.h"
#include "QwtHistogram.h"
#include "QwtPieCurve.h"
#include "RenameWindowDialog.h"
#include "ScaleDraw.h"
#include "ScriptFileInterpreter.h"
#include "ScriptingLangDialog.h"
#include "ScriptingWindow.h"
#include "SetColValuesDialog.h"
#include "SigmoidalFit.h"
#include "SmoothCurveDialog.h"
#include "SmoothFilter.h"
#include "Spectrogram.h"
#include "SurfaceDialog.h"
#include "SymbolDialog.h"
#include "TableDialog.h"
#include "TableStatistics.h"
#include "TextDialog.h"
#include "TextEditor.h"
#include "TiledWindow.h"
#include "importOPJ.h"
#include <MantidQtWidgets/Common/pixmaps.h>

// TODO: move tool-specific code to an extension manager
#include "ContourLinesEditor.h"
#include "LabelTool.h"
#include "LineProfileTool.h"
#include "Mantid/InstrumentWidget/InstrumentWindow.h"
#include "Mantid/MantidMatrix.h"
#include "Mantid/MantidMatrixCurve.h"
#include "Mantid/MantidTable.h"
#include "Mantid/RemoveErrorsDialog.h"
#include "MultiPeakFitTool.h"
#include "PlotToolInterface.h"
#include "RangeSelectorTool.h"
#include "ScreenPickerTool.h"
#include "TranslateCurveTool.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QColorGroup>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontComboBox>
#include <QImageReader>
#include <QImageWriter>
#include <QInputDialog>
#include <QKeySequence>
#include <QList>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMenuItem>
#include <QMessageBox>
#include <QPair>
#include <QPixmapCache>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrinterInfo>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>
#include <QSignalMapper>
#include <QSpinBox>
#include <QSplitter>
#include <QTextCodec>
#include <QTextStream>
#include <QToolBar>
#include <QTranslator>
#include <QUrl>
#include <QVarLengthArray>
#include <QtAlgorithms>
#include <QtGlobal>
#include <qwt_scale_engine.h>
#include <zlib.h>

#include <gsl/gsl_sort.h>

#include <boost/regex.hpp>

#include <Poco/Path.h>

// Mantid
#include "Mantid/FirstTimeSetup.h"
#include "Mantid/ManageCustomMenus.h"
#include "Mantid/ManageInterfaceCategories.h"
#include "Mantid/MantidAbout.h"
#include "Mantid/MantidUI.h"
#include "Mantid/PeakPickerTool.h"

#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include "MantidQtWidgets/Common/CatalogHelper.h"
#include "MantidQtWidgets/Common/CatalogSearch.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/MessageDisplay.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MantidQtWidgets/Common/TrackedAction.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/ScriptRepositoryView.h"

#ifdef MAKE_VATES
#include "vtkPVDisplayInformation.h"
#endif

using namespace Qwt3D;
using namespace MantidQt::API;
using Mantid::Kernel::ConfigService;

using Mantid::API::FrameworkManager;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::LibraryManager;
using Mantid::Kernel::Logger;

namespace {
/// static logger
Logger g_log("ApplicationWindow");
} // namespace

extern "C" {
void file_compress(const char *file, const char *mode);
void file_uncompress(const char *file);
}

ApplicationWindow::ApplicationWindow(bool factorySettings)
    // Delegate with an empty string list for the arguments
    : ApplicationWindow(factorySettings, QStringList{}) {}

ApplicationWindow::ApplicationWindow(bool factorySettings,
                                     const QStringList &args)
    : QMainWindow(), Scripted(ScriptingLangManager::newEnv(this)),
      blockWindowActivation(false), m_enableQtiPlotFitting(false),
      m_projectRecovery(this), m_exitCode(0),
#ifdef Q_OS_MAC // Mac
      settings(QSettings::IniFormat, QSettings::UserScope, "Mantid",
               "MantidPlot")
#else
      settings("Mantid", "MantidPlot")
#endif
{
  init(factorySettings, args);
}

/**
 * This function is responsible for copying the old configuration
 * information from the ISIS\MantidPlot area to the new Mantid\MantidPlot
 * area. The old area is deleted once the transfer is complete. On subsequent
 * runs, if the old configuration area is missing or empty, the copying
 * is ignored.
 */
void ApplicationWindow::handleConfigDir() {
#ifdef Q_OS_WIN
  // We use the registry for settings on Windows
  QSettings oldSettings("ISIS", "MantidPlot");
  QStringList keys = oldSettings.allKeys();
  // If the keys are empty, we removed the MantidPlot entries
  if (!keys.empty()) {
    foreach (QString key, keys) {
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
  if (oldConfigDir.exists()) {
    QStringList entries = oldConfigDir.entryList();
    foreach (QString entry, entries) {
      if (!entry.startsWith(".")) {
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
 * Cache the working directory in the QSettings.
 *
 * store the working directory in the settings so it may be accessed
 * elsewhere in the Qt layer.
 *
 */
void ApplicationWindow::cacheWorkingDirectory() const {
  QSettings settings;
  settings.beginGroup("/Project");
  settings.setValue("/WorkingDirectory", workingDir);
  settings.endGroup();
}

/**
 * Calls QCoreApplication::exit(m_exitCode)
 */
void ApplicationWindow::exitWithPresetCode() {
  QCoreApplication::exit(m_exitCode);
  handleConfigDir();
}

void ApplicationWindow::init(bool factorySettings, const QStringList &args) {
  QCoreApplication::setOrganizationName("Mantid");
  QCoreApplication::setApplicationName("MantidPlot");
  setAttribute(Qt::WA_DeleteOnClose);

#ifdef SHARED_MENUBAR
  m_sharedMenuBar = new QMenuBar(nullptr);
  m_sharedMenuBar->setNativeMenuBar(true);
#endif
  setWindowTitle(tr("MantidPlot - untitled")); // Mantid
  setObjectName("main application");
  initGlobalConstants();
  QPixmapCache::setCacheLimit(20 * QPixmapCache::cacheLimit());

  // Logging as early as possible
  logWindow = new QDockWidget(this);
  logWindow->hide();
  logWindow->setObjectName(
      "logWindow"); // this is needed for QMainWindow::restoreState()
  logWindow->setWindowTitle(tr("Results Log"));
  addDockWidget(Qt::TopDockWidgetArea, logWindow);

  using MantidQt::MantidWidgets::Message;
  using MantidQt::MantidWidgets::MessageDisplay;
  qRegisterMetaType<Message>("Message"); // Required to use it in signals-slots
  resultsLog = new MessageDisplay(logWindow);
  logWindow->setWidget(resultsLog);
  connect(resultsLog, SIGNAL(errorReceived(const QString &)), logWindow,
          SLOT(show()));

  // Process all pending events before loading Mantid
  // Helps particularly on Windows with cleaning up the
  // splash screen after the 3D visualization dialog has closed
  qApp->processEvents();

  ConfigService::Instance();          // Starts logging
  resultsLog->attachLoggingChannel(); // Must be done after logging starts
  // Read settings early so that the log level is set before the framework
  // starts
  resultsLog->readSettings(settings);
  // Load Mantid core libraries by starting the framework
  FrameworkManager::Instance();
#ifdef MAKE_VATES
  if (!vtkPVDisplayInformation::SupportsOpenGLLocally())
    g_log.error("The OpenGL configuration does not support the VSI.");
#endif

  // Create UI object
  mantidUI = new MantidUI(this);

  // Everything else...
  tablesDepend = new QMenu(this);
  explorerWindow = new QDockWidget(this);
  explorerWindow->setWindowTitle(tr("Project Explorer"));
  explorerWindow->setObjectName(
      "explorerWindow"); // this is needed for QMainWindow::restoreState()
  explorerWindow->setMinimumHeight(150);
  addDockWidget(Qt::BottomDockWidgetArea, explorerWindow);

  actionSaveFile = nullptr;
  actionSaveProject = nullptr;
  actionSaveProjectAs = nullptr;
  folders = new FolderListView(this);
  folders->setContextMenuPolicy(Qt::CustomContextMenu);
  folders->setHeaderLabel("Folder");
  folders->setRootIsDecorated(true);
  folders->header()->hide();
  folders->setSelectionMode(QAbstractItemView::SingleSelection);

  connect(folders,
          SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
          this, SLOT(folderItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
  connect(folders, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showFolderPopupMenu(const QPoint &)));
  connect(folders, SIGNAL(deleteSelection()), this,
          SLOT(deleteSelectedItems()));

  d_current_folder = new Folder(nullptr, tr("untitled"));
  FolderListItem *fli = new FolderListItem(folders, d_current_folder);
  d_current_folder->setFolderListItem(fli);
  fli->setExpanded(true);

  lv = new FolderListView();
  lv->setContextMenuPolicy(Qt::CustomContextMenu);

  lv->setMinimumHeight(80);
  lv->setSelectionMode(QAbstractItemView::ExtendedSelection);

  explorerSplitter = new QSplitter(Qt::Horizontal, explorerWindow);
  explorerSplitter->addWidget(folders);
  explorerSplitter->addWidget(lv);
  explorerWindow->setWidget(explorerSplitter);

  QList<int> splitterSizes;
  explorerSplitter->setSizes(splitterSizes << 45 << 45);
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

  hiddenWindows = new QList<QWidget *>();

  scriptingWindow = nullptr;
  d_text_editor = nullptr;

  const QString scriptsDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "mantidqt.python_interfaces_directory"));

  // Parse the list of registered PyQt interfaces and their respective
  // categories.
  QString pyQtInterfacesProperty = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "mantidqt.python_interfaces"));
  foreach (const QString pyQtInterfaceInfo, pyQtInterfacesProperty.split(" ")) {
    QString pyQtInterfaceFile;
    QSet<QString> pyQtInterfaceCategories;
    const QStringList tokens = pyQtInterfaceInfo.split("/");

    if (tokens.size() == 0) // Empty token - ignore.
    {
      continue;
    } else if (tokens.size() == 1) // Assume missing category.
    {
      pyQtInterfaceCategories += "Uncatagorised";
      pyQtInterfaceFile = tokens[0];
    } else if (tokens.size() ==
               2) // Assume correct interface name and categories.
    {
      pyQtInterfaceCategories += tokens[0].split(";").toSet();
      pyQtInterfaceFile = tokens[1];
    } else // Too many forward slashes, or no space between two interfaces.
           // Warn user and move on.
    {
      g_log.warning() << "The mantidqt.python_interfaces property contains an "
                         "unparsable value: "
                      << pyQtInterfaceInfo.toStdString();
      continue;
    }

    const QString scriptPath = scriptsDir + '/' + pyQtInterfaceFile;

    if (QFileInfo(scriptPath).exists()) {
      const QString pyQtInterfaceName =
          QFileInfo(scriptPath).baseName().replace("_", " ");
      m_interfaceNameDataPairs.append(qMakePair(pyQtInterfaceName, scriptPath));

      // Keep track of the interface's categories as we go.
      m_interfaceCategories[pyQtInterfaceName] = pyQtInterfaceCategories;
      m_allCategories += pyQtInterfaceCategories;
    } else {
      g_log.warning() << "Could not find interface script: "
                      << scriptPath.toAscii().data() << "\n";
    }
  }

  MantidQt::API::InterfaceManager interfaceManager;
  // Add all interfaces inherited from UserSubWindow.
  foreach (const QString userSubWindowName,
           interfaceManager.getUserSubWindowKeys()) {
    m_interfaceNameDataPairs.append(
        qMakePair(userSubWindowName, userSubWindowName));

    const QSet<QString> categories =
        UserSubWindowFactory::Instance().getInterfaceCategories(
            userSubWindowName);

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
  actionNextWindow = new QAction(QIcon(getQPixmap("next_xpm")),
                                 tr("&Next", "next window"), this);
  actionNextWindow->setShortcut(tr("F5", "next window shortcut"));
  connect(actionNextWindow, SIGNAL(triggered()), d_workspace,
          SLOT(activateNextSubWindow()));

  actionPrevWindow = new QAction(QIcon(getQPixmap("prev_xpm")),
                                 tr("&Previous", "previous window"), this);
  actionPrevWindow->setShortcut(tr("F6", "previous window shortcut"));
  connect(actionPrevWindow, SIGNAL(triggered()), d_workspace,
          SLOT(activatePreviousSubWindow()));

  connect(tablesDepend, SIGNAL(triggered(QAction *)), this,
          SLOT(showTable(QAction *)));

  connect(this, SIGNAL(modified()), this, SLOT(modifiedProject()));
  connect(d_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)), this,
          SLOT(windowActivated(QMdiSubWindow *)));
  connect(lv, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showWindowPopupMenu(const QPoint &)));
  connect(lv, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));

  connect(recentProjectsMenu, SIGNAL(triggered(QAction *)), this,
          SLOT(openRecentProject(QAction *)));
  connect(recentFilesMenu, SIGNAL(triggered(QAction *)), this,
          SLOT(openRecentFile(QAction *)));

  // apply user settings
  updateAppFonts();
  setAppColors(workspaceColor, panelsColor, panelsTextColor, true);

  // Scripting
  m_script_envs = QHash<QString, ScriptingEnv *>();
  m_iface_script = nullptr;
  setScriptingLanguage(defaultScriptingLang);

  m_interpreterDock = new QDockWidget(this);
  m_interpreterDock->setObjectName(
      "interpreterDock"); // this is needed for QMainWindow::restoreState()
  m_interpreterDock->setWindowTitle("Script Interpreter");
  runPythonScript("from ipython_widget import *\nw = "
                  "_qti.app._getInterpreterDock()\nw.setWidget("
                  "MantidIPythonWidget())",
                  false, true, true);
  if (!restoreDockWidget(m_interpreterDock)) {
    // Restoring the widget fails if the settings aren't found or read.
    // Therefore, add it manually.
    addDockWidget(Qt::BottomDockWidgetArea, m_interpreterDock);
  }

  loadCustomActions();

  // Nullify catalogSearch
  catalogSearch.reset();

  // Print a warning message if the scripting language is set to muParser
  if (defaultScriptingLang == "muParser") {
    logWindow->show();
    g_log.warning("The scripting language is set to muParser. This is probably "
                  "not what you want! Change the default in "
                  "View->Preferences.");
  }

  // Need to show first time setup dialog?
  // It is raised in the about2start method as on OS X if the event loop is not
  // running then raise()
  // does not push the dialog to the top of the stack
  d_showFirstTimeSetup = shouldWeShowFirstTimeSetup(args);

  using namespace Mantid::API;
  // Do this as late as possible to avoid unnecessary updates
  AlgorithmFactory::Instance().enableNotifications();
  AlgorithmFactory::Instance().notificationCenter.postNotification(
      new AlgorithmFactoryUpdateNotification);
}

/** Determines if the first time dialog should be shown
 * @param commandArguments : all command line arguments.
 * @returns true if the dialog should be shown
 */
bool ApplicationWindow::shouldWeShowFirstTimeSetup(
    const QStringList &commandArguments) {
  // Early check of execute and quit command arguments used by system tests.
  QString str;
  foreach (str, commandArguments) {
    if ((this->shouldExecuteAndQuit(str)) || (this->isSilentStartup(str))) {
      return false;
    }
  }

  // first check the facility and instrument
  using Mantid::Kernel::ConfigService;
  auto &config = ConfigService::Instance();
  std::string facility = config.getString("default.facility");
  std::string instrument = config.getString("default.instrument");
  if (facility.empty() || instrument.empty()) {
    return true;
  } else {
    // check we can get the facility and instrument
    try {
      const Mantid::Kernel::FacilityInfo &facilityInfo =
          config.getFacility(facility);
      const Mantid::Kernel::InstrumentInfo &instrumentInfo =
          config.getInstrument(instrument);
      g_log.information() << "Default facility '" << facilityInfo.name()
                          << "', instrument '" << instrumentInfo.name()
                          << "'\n";
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      // failed to find the facility or instrument
      g_log.error() << "Could not find your default facility '" << facility
                    << "' or instrument '" << instrument
                    << "' in facilities.xml, showing please select again.\n";
      return true;
    }
  }

  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  const bool doNotShowUntilNextRelease =
      settings.value("DoNotShowUntilNextRelease", 0).toInt();
  const QString lastVersion = settings.value("LastVersion", "").toString();
  settings.endGroup();

  if (!doNotShowUntilNextRelease) {
    return true;
  }

  // Now check if the version has changed since last time
  const QString version =
      QString::fromStdString(Mantid::Kernel::MantidVersion::releaseNotes());

  return (version != lastVersion);
}

void ApplicationWindow::initWindow() {
  switch (d_init_window_type) {
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

void ApplicationWindow::initGlobalConstants() {
  d_auto_update_table_values = true;
  d_active_window = nullptr;
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
  lastCopiedLayer = nullptr;
  d_text_copy = nullptr;
  d_arrow_copy = nullptr;
  d_image_copy = nullptr;

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
  plotNumbersFont = QFont(family, pointSize);
  plotLegendFont = appFont;
  plotTitleFont = QFont(family, pointSize + 2, QFont::Bold, false);

  plot3DAxesFont = QFont(family, pointSize, QFont::Bold, false);
  plot3DNumbersFont = QFont(family, pointSize);
  plot3DTitleFont = QFont(family, pointSize + 2, QFont::Bold, false);

  autoSearchUpdates = false;
  appLanguage = QLocale::system().name().section('_', 0, 0);
  show_windows_policy = ApplicationWindow::ActiveFolder;

  workspaceColor = QColor("darkGray");
  panelsColor = QColor("#ffffff");
  panelsTextColor = QColor("#000000");
  tableBkgdColor = QColor("#ffffff");
  tableTextColor = QColor("#000000");
  tableHeaderColor = QColor("#000000");

  plot3DColors = {"blue", "#000000", "#000000", "#000000",
                  "red",  "#000000", "#000000", "#ffffff"};

  d_graph_tick_labels_dist = 4;
  d_graph_axes_labels_dist = 2;

  autoSave = false;
  autoSaveTime = 15;
  d_backup_files = true;
  defaultScriptingLang = "Python"; // Mantid M. Gigg
  // Scripting window geometry
  d_script_win_pos = QPoint(250, 200);
  d_script_win_size = QSize(600, 660);
  d_thousands_sep = true;
  d_locale = QLocale::system().name();
  if (!d_thousands_sep)
    d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

  d_decimal_digits = 13;
  d_graphing_digits = 13;

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
  confirmCloseInstrWindow = false;

  d_show_table_comments = false;

  titleOn = true;
  // 'Factory' default is to show top & right axes but without labels
  d_show_axes = QVector<bool>(QwtPlot::axisCnt, true);
  d_show_axes_labels = QVector<bool>(QwtPlot::axisCnt, true);
  d_show_axes_labels[1] = false;
  d_show_axes_labels[3] = false;
  autoDistribution1D = true;
  canvasFrameWidth = 0;
  defaultPlotMargin = 0;
  drawBackbones = true;

  // these settings are overridden, but the default axes scales are linear
  d_axes_scales = QVector<QString>(QwtPlot::axisCnt, "linear");

  axesLineWidth = 1;
  autoscale2DPlots = true;
  autoScaleFonts = true;
  autoResizeLayers = true;
  antialiasing2DPlots = true;
  fixedAspectRatio2DPlots = false; // Mantid
  d_scale_plots_on_print = false;
  d_print_cropmarks = false;
  d_synchronize_graph_scales = true;

  defaultCurveStyle = static_cast<int>(GraphOptions::Line);
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
  d_export_color = true;
  d_export_vector_size = static_cast<int>(QPrinter::Custom);
  d_keep_plot_aspect = true;
}

QMenuBar *ApplicationWindow::myMenuBar() {
#ifdef SHARED_MENUBAR
  return m_sharedMenuBar == nullptr ? menuBar() : m_sharedMenuBar;
#else
  return menuBar();
#endif
}

void ApplicationWindow::initToolBars() {
  initPlot3DToolBar();
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  QPixmap openIcon, saveIcon;

  standardTools = new QToolBar(tr("Standard Tools"), this);
  standardTools->setObjectName(
      "standardTools"); // this is needed for QMainWindow::restoreState()
  standardTools->setIconSize(QSize(18, 20));
  addToolBar(Qt::TopToolBarArea, standardTools);

  standardTools->addAction(actionLoadFile);
  standardTools->addSeparator();
  standardTools->addAction(actionNewProject);
  standardTools->addAction(actionOpenProj);
  standardTools->addAction(actionSaveProject);
  standardTools->addSeparator();

  standardTools->addAction(actionShowLog);
#ifdef SCRIPTING_PYTHON
  standardTools->addAction(actionShowScriptWindow);
#endif

  standardTools->addSeparator();
  standardTools->addAction(actionManageDirs);
  standardTools->addSeparator();

  standardTools->addAction(actionCopySelection);
  standardTools->addAction(actionPasteSelection);

  plotTools = new QToolBar(tr("Plot"), this);
  plotTools->setObjectName(
      "plotTools"); // this is needed for QMainWindow::restoreState()
  plotTools->setIconSize(QSize(16, 20));
  addToolBar(plotTools);

  dataTools = new QActionGroup(this);
  dataTools->setExclusive(true);

  btnPointer = new QAction(tr("Disable &Tools"), this);
  btnPointer->setActionGroup(dataTools);
  btnPointer->setCheckable(true);
  btnPointer->setIcon(QIcon(getQPixmap("pointer_xpm")));
  btnPointer->setChecked(true);
  plotTools->addAction(btnPointer);

  actionPanPlot->setActionGroup(dataTools);
  actionPanPlot->setCheckable(true);
  plotTools->addAction(actionPanPlot);

  btnZoomIn = new QAction(tr("&Zoom In"), this);
  btnZoomIn->setShortcut(tr("Ctrl++"));
  btnZoomIn->setActionGroup(dataTools);
  btnZoomIn->setCheckable(true);
  btnZoomIn->setIcon(QIcon(getQPixmap("zoom_xpm")));
  plotTools->addAction(btnZoomIn);

  btnZoomOut = new QAction(tr("&Zoom Out"), this);
  btnZoomOut->setShortcut(tr("Ctrl+-"));
  btnZoomOut->setActionGroup(dataTools);
  btnZoomOut->setCheckable(true);
  btnZoomOut->setIcon(QIcon(getQPixmap("zoomOut_xpm")));
  plotTools->addAction(btnZoomOut);
  plotTools->addAction(actionUnzoom);

  btnCursor = new QAction(tr("&Data Reader"), this);
  btnCursor->setShortcut(tr("CTRL+D"));
  btnCursor->setActionGroup(dataTools);
  btnCursor->setCheckable(true);
  btnCursor->setIcon(QIcon(getQPixmap("select_xpm")));
  btnPicker = new QAction(tr("S&creen Reader"), this);
  btnPicker->setActionGroup(dataTools);
  btnPicker->setCheckable(true);
  btnPicker->setIcon(QIcon(getQPixmap("cursor_16_xpm")));
  plotTools->addAction(btnPicker); // disabled until fixed (#2783)

  actionDrawPoints = new QAction(tr("&Draw Data Points"), this);
  actionDrawPoints->setActionGroup(dataTools);
  actionDrawPoints->setCheckable(true);
  actionDrawPoints->setIcon(QIcon(getQPixmap("draw_points_xpm")));

  btnMovePoints = new QAction(tr("&Move Data Points..."), this);
  btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
  btnMovePoints->setActionGroup(dataTools);
  btnMovePoints->setCheckable(true);
  btnMovePoints->setIcon(QIcon(getQPixmap("hand_xpm")));

  btnRemovePoints = new QAction(tr("Remove &Bad Data Points..."), this);
  btnRemovePoints->setShortcut(tr("Alt+B"));
  btnRemovePoints->setActionGroup(dataTools);
  btnRemovePoints->setCheckable(true);
  btnRemovePoints->setIcon(QIcon(getQPixmap("gomme_xpm")));

  if (mantidUI->fitFunctionBrowser()) {
    btnMultiPeakPick = new QAction(tr("Select Multiple Peaks..."), this);
    btnMultiPeakPick->setActionGroup(dataTools);
    btnMultiPeakPick->setCheckable(true);
    btnMultiPeakPick->setIcon(QIcon(getQPixmap("Fit_xpm")));
    plotTools->addAction(btnMultiPeakPick);
  } else {
    btnMultiPeakPick = nullptr;
  }

  connect(dataTools, SIGNAL(triggered(QAction *)), this,
          SLOT(pickDataTool(QAction *)));
  plotTools->addSeparator();

  btnLabel = new QAction(tr("Label &Tool"), this);
  btnLabel->setShortcut(tr("Ctrl+Alt+T"));
  btnLabel->setActionGroup(dataTools);
  btnLabel->setIcon(QIcon(getQPixmap("text_xpm")));
  btnLabel->setCheckable(true);
  plotTools->addAction(btnLabel);

  btnArrow = new QAction(tr("Draw &Arrow"), this);
  btnArrow->setShortcut(tr("Ctrl+Alt+A"));
  btnArrow->setActionGroup(dataTools);
  btnArrow->setCheckable(true);
  btnArrow->setIcon(QIcon(getQPixmap("arrow_xpm")));
  plotTools->addAction(btnArrow);

  btnLine = new QAction(tr("Draw Li&ne"), this);
  btnLine->setShortcut(tr("Ctrl+Alt+N"));
  btnLine->setActionGroup(dataTools);
  btnLine->setCheckable(true);
  btnLine->setIcon(QIcon(getQPixmap("lPlot_xpm")));
  plotTools->addAction(btnLine);

  plotTools->addSeparator();
  plotTools->addAction(actionAddFunctionCurve);
  plotTools->addAction(actionNewLegend);
  plotTools->addSeparator();

  plotTools->hide();

  displayBar = new QToolBar(tr("Data Display"), this);
  displayBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
  displayBar->setObjectName(
      "displayBar"); // this is needed for QMainWindow::restoreState()
  info = new QLineEdit(this);
  displayBar->addWidget(info);
  info->setReadOnly(true);
  QPalette palette;
  palette.setColor(QPalette::Text, QColor(Qt::green));
  palette.setColor(QPalette::HighlightedText, QColor(Qt::darkGreen));
  palette.setColor(QPalette::Base, QColor(Qt::black));
  info->setPalette(palette);

  addToolBar(Qt::TopToolBarArea, displayBar);
  displayBar->hide();

  insertToolBarBreak(displayBar);

  formatToolBar = new QToolBar(tr("Format"), this);
  formatToolBar->setObjectName("formatToolBar");
  addToolBar(Qt::TopToolBarArea, formatToolBar);

  QFontComboBox *fb = new QFontComboBox();
  connect(fb, SIGNAL(currentFontChanged(const QFont &)), this,
          SLOT(setFontFamily(const QFont &)));
  actionFontBox = formatToolBar->addWidget(fb);

  QSpinBox *sb = new QSpinBox();
  connect(sb, SIGNAL(valueChanged(int)), this, SLOT(setFontSize(int)));
  actionFontSize = formatToolBar->addWidget(sb);

  formatToolBar->addAction(actionFontBold);
  formatToolBar->addAction(actionFontItalic);

  formatToolBar->addAction(actionUnderline);
  formatToolBar->addAction(actionSuperscript);
  formatToolBar->addAction(actionSubscript);
  formatToolBar->addAction(actionGreekSymbol);
  formatToolBar->addAction(actionGreekMajSymbol);
  formatToolBar->addAction(actionMathSymbol);

  formatToolBar->setEnabled(false);
  formatToolBar->hide();
}

void ApplicationWindow::insertTranslatedStrings() {
  if (projectname == "untitled")
    setWindowTitle(tr("MantidPlot - untitled")); // Mantid

  QStringList labels;
  labels << "Name"
         << "Type"
         << "View"
         << "Size"
         << "Created"
         << "Label";
  lv->setHeaderLabels(labels);
  lv->resizeColumnToContents(0);
  lv->resizeColumnToContents(1);
  lv->resizeColumnToContents(2);
  lv->resizeColumnToContents(3);
  lv->resizeColumnToContents(4);
  lv->resizeColumnToContents(5);

  explorerWindow->setWindowTitle(tr("Project Explorer"));
  logWindow->setWindowTitle(tr("Results Log"));
  displayBar->setWindowTitle(tr("Data Display"));
  plotTools->setWindowTitle(tr("Plot"));
  standardTools->setWindowTitle(tr("Standard Tools"));
  formatToolBar->setWindowTitle(tr("Format"));

  auto recentProjectsMenuAction = recentProjectsMenu->menuAction();
  recentProjectsMenuAction->setText(tr("&Recent Projects"));

  auto recentFilesMenuAction = recentFilesMenu->menuAction();
  recentFilesMenuAction->setText(tr("R&ecent Files"));

  translateActionsStrings();
  customMenu(activeWindow());
}

void ApplicationWindow::initMainMenu() {
  fileMenu = new QMenu(this);
  fileMenu->setObjectName("fileMenu");
  connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));

  newMenu = new QMenu(this);
  recentProjectsMenu = new QMenu(this);
  recentFilesMenu = new MenuWithToolTips(this);
  newMenu->setObjectName("newMenu");
  exportPlotMenu = new QMenu(this);
  exportPlotMenu->setObjectName("exportPlotMenu");

  edit = new QMenu(this);
  edit->setObjectName("editMenu");

  edit->addSeparator();
  edit->addAction(actionCopySelection);
  edit->addAction(actionPasteSelection);
  edit->addSeparator();
  edit->addAction(actionDeleteFitTables);

  connect(edit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

  view = new QMenu(this);
  view->setObjectName("viewMenu");

  view->addAction(actionShowExplorer);
  view->addAction(actionShowLog);

  view->addSeparator();
  view->addAction(actionShowScriptWindow); // Mantid
  view->addAction(actionShowScriptInterpreter);
  view->addSeparator();

  mantidUI->addMenuItems(view);

  view->addSeparator();
  toolbarsMenu = view->addMenu(tr("&Toolbars"));
  view->addAction(actionShowConfigureDialog);
  view->addSeparator();
  view->addAction(actionCustomActionDialog);

  graph = new QMenu(this);
  graph->setObjectName("graphMenu");
  graph->addAction(actionAddErrorBars);
  graph->addAction(actionRemoveErrorBars);
  graph->addAction(actionShowCurvesDialog);
  graph->addAction(actionAddFunctionCurve);
  graph->addAction(actionNewLegend);
  graph->addSeparator();
  graph->addAction(btnLabel);
  graph->addAction(btnArrow);
  graph->addAction(btnLine);
  graph->addAction(actionTimeStamp);
  graph->addAction(actionAddImage);
  graph->addSeparator(); // layers section
  graph->addAction(actionAddLayer);
  graph->addAction(actionDeleteLayer);
  graph->addAction(actionShowLayerDialog);

  plot3DMenu = new QMenu(this);
  plot3DMenu->setObjectName("plot3DMenu");
  plot3DMenu->addAction(actionPlot3DWireFrame);
  plot3DMenu->addAction(actionPlot3DHiddenLine);
  plot3DMenu->addAction(actionPlot3DPolygons);
  plot3DMenu->addAction(actionPlot3DWireSurface);
  plot3DMenu->addSeparator();
  plot3DMenu->addAction(actionPlot3DBars);
  plot3DMenu->addAction(actionPlot3DScatter);
  plot3DMenu->addSeparator();
  plot3DMenu->addAction(actionImagePlot);
  plot3DMenu->addAction(actionColorMap);
  plot3DMenu->addAction(actionNoContourColorMap);
  plot3DMenu->addAction(actionContourMap);
  plot3DMenu->addAction(actionGrayMap);
  plot3DMenu->addSeparator();

  matrixMenu = new QMenu(this);
  matrixMenu->setObjectName("matrixMenu");
  connect(matrixMenu, SIGNAL(aboutToShow()), this,
          SLOT(matrixMenuAboutToShow()));

  plot2DMenu = new QMenu(this);
  plot2DMenu->setObjectName("plot2DMenu");
  connect(plot2DMenu, SIGNAL(aboutToShow()), this, SLOT(plotMenuAboutToShow()));

  plotDataMenu = new QMenu(this);
  plotDataMenu->setObjectName("plotDataMenu");
  connect(plotDataMenu, SIGNAL(aboutToShow()), this,
          SLOT(plotDataMenuAboutToShow()));

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

  analysisMenu = new QMenu(this);
  analysisMenu->setObjectName("analysisMenu");
  connect(analysisMenu, SIGNAL(aboutToShow()), this,
          SLOT(analysisMenuAboutToShow()));

  format = new QMenu(this);
  format->setObjectName("formatMenu");

  windowsMenu = new QMenu(this);
  windowsMenu->setObjectName("windowsMenu");
  connect(windowsMenu, SIGNAL(aboutToShow()), this,
          SLOT(windowsMenuAboutToShow()));

  interfaceMenu = new QMenu(this);
  interfaceMenu->setObjectName("interfaceMenu");
  connect(interfaceMenu, SIGNAL(aboutToShow()), this,
          SLOT(interfaceMenuAboutToShow()));

  tiledWindowMenu = new QMenu(this);
  tiledWindowMenu->setObjectName("tiledWindowMenu");
  connect(tiledWindowMenu, SIGNAL(aboutToShow()), this,
          SLOT(tiledWindowMenuAboutToShow()));

  help = new QMenu(this);
  help->setObjectName("helpMenu");

  help->addAction(actionHomePage);
  help->addAction(actionMantidConcepts);
  help->addAction(actionMantidAlgorithms);
  help->addAction(actionmantidplotHelp);
  help->addSeparator();
  help->addAction(actionHelpBugReports);
  help->addAction(actionAskHelp);
  help->addSeparator();
  help->addAction(actionFirstTimeSetup);
  help->addSeparator();

  help->addAction(actionAbout);

  icat = new QMenu(this);
  icat->setObjectName("CatalogMenu");
  connect(icat, SIGNAL(aboutToShow()), this, SLOT(populateCatalogLoginMenu()));

  disableActions();
}

void ApplicationWindow::tableMenuAboutToShow() {
  tableMenu->clear();
  fillMenu->clear();

  MdiSubWindow *t = activeWindow();
  if (t == nullptr)
    return;

  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!table)
    return;

  bool isFixedColumns = table->isFixedColumns();
  bool isEditable = table->isEditable();

  QMenu *setAsMenu = tableMenu->addMenu(tr("Set Columns &As"));
  setAsMenu->addAction(actionSetXCol);
  setAsMenu->addAction(actionSetYCol);
  setAsMenu->addAction(actionSetZCol);
  setAsMenu->addSeparator();
  setAsMenu->addAction(actionSetLabelCol);
  setAsMenu->addAction(actionDisregardCol);
  setAsMenu->addSeparator();
  setAsMenu->addAction(actionSetXErrCol);
  setAsMenu->addAction(actionSetYErrCol);
  setAsMenu->addSeparator();
  setAsMenu->addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
  setAsMenu->addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));

  tableMenu->addAction(actionShowColumnOptionsDialog);
  if (isEditable)
    tableMenu->addSeparator();

  if (isEditable)
    tableMenu->addAction(actionShowColumnValuesDialog);
  if (isEditable)
    tableMenu->addAction(actionTableRecalculate);

  if (isEditable) {
    fillMenu = tableMenu->addMenu(tr("&Fill Columns With"));
    fillMenu->addAction(actionSetAscValues);
    fillMenu->addAction(actionSetRandomValues);
  }

  if (isEditable)
    tableMenu->addAction(actionClearTable);
  tableMenu->addSeparator();
  if (!isFixedColumns)
    tableMenu->addAction(actionAddColToTable);
  tableMenu->addAction(actionShowColsDialog);
  tableMenu->addSeparator();
  tableMenu->addAction(actionHideSelectedColumns);
  tableMenu->addAction(actionShowAllColumns);
  if (!isFixedColumns)
    tableMenu->addSeparator();
  if (!isFixedColumns)
    tableMenu->addAction(actionMoveColFirst);
  if (!isFixedColumns)
    tableMenu->addAction(actionMoveColLeft);
  if (!isFixedColumns)
    tableMenu->addAction(actionMoveColRight);
  if (!isFixedColumns)
    tableMenu->addAction(actionMoveColLast);
  if (!isFixedColumns)
    tableMenu->addAction(actionSwapColumns);
  tableMenu->addSeparator();
  if (isOfType(t, "Table"))
    tableMenu->addAction(actionShowRowsDialog);
  tableMenu->addAction(actionDeleteRows);
  tableMenu->addSeparator();
  tableMenu->addAction(actionGoToRow);
  tableMenu->addAction(actionGoToColumn);
  tableMenu->addSeparator();
  tableMenu->addAction(actionConvertTable);
  if (isOfType(t, "Table")) // but not MantidTable
  {
    tableMenu->addAction(actionConvertTableToWorkspace);
  }
  tableMenu->addAction(actionConvertTableToMatrixWorkspace);
  tableMenu->addAction(actionSortTable);

  tableMenu->addSeparator();
  tableMenu->addAction(actionShowPlotWizard);

  reloadCustomActions();
}

void ApplicationWindow::plotDataMenuAboutToShow() {
  plotDataMenu->clear();
  plotDataMenu->addAction(btnPointer);
  plotDataMenu->addAction(btnZoomIn);
  plotDataMenu->addAction(btnZoomOut);
  plotDataMenu->addAction(actionPanPlot);
  plotDataMenu->addAction(actionUnzoom);
  plotDataMenu->addSeparator();
  plotDataMenu->addAction(btnCursor);
  plotDataMenu->addAction(btnPicker);
  plotDataMenu->addSeparator();
  plotDataMenu->addAction(actionDrawPoints);
  plotDataMenu->addAction(btnMovePoints);
  plotDataMenu->addAction(btnRemovePoints);

  reloadCustomActions();
}

void ApplicationWindow::plotMenuAboutToShow() {
  plot2DMenu->clear();

  plot2DMenu->addAction(actionPlotL);
  plot2DMenu->addAction(actionPlotP);
  plot2DMenu->addAction(actionPlotLP);

  QMenu *specialPlotMenu = plot2DMenu->addMenu(tr("Special Line/Symb&ol"));
  specialPlotMenu->addAction(actionWaterfallPlot);
  specialPlotMenu->addAction(actionPlotVerticalDropLines);
  specialPlotMenu->addAction(actionPlotSpline);
  specialPlotMenu->addAction(actionPlotVertSteps);
  specialPlotMenu->addAction(actionPlotHorSteps);
  plot2DMenu->addSeparator();
  plot2DMenu->addAction(actionPlotVerticalBars);
  plot2DMenu->addAction(actionPlotHorizontalBars);
  plot2DMenu->addAction(actionPlotArea);
  plot2DMenu->addAction(actionPlotPie);
  plot2DMenu->addAction(actionPlotVectXYXY);
  plot2DMenu->addAction(actionPlotVectXYAM);
  plot2DMenu->addSeparator();

  QMenu *statMenu = plot2DMenu->addMenu(tr("Statistical &Graphs"));
  statMenu->addAction(actionBoxPlot);
  statMenu->addAction(actionPlotHistogram);
  statMenu->addAction(actionPlotStackedHistograms);
  statMenu->addSeparator();
  statMenu->addAction(actionStemPlot);

  QMenu *panelsMenu = plot2DMenu->addMenu(tr("Pa&nel"));
  panelsMenu->addAction(actionPlot2VerticalLayers);
  panelsMenu->addAction(actionPlot2HorizontalLayers);
  panelsMenu->addAction(actionPlot4Layers);
  panelsMenu->addAction(actionPlotStackedLayers);

  QMenu *plot3D = plot2DMenu->addMenu(tr("3&D Plot"));
  plot3D->addAction(actionPlot3DRibbon);
  plot3D->addAction(actionPlot3DBars);
  plot3D->addAction(actionPlot3DScatter);
  plot3D->addAction(actionPlot3DTrajectory);

  reloadCustomActions();
}

void ApplicationWindow::customMenu(MdiSubWindow *w) {
  myMenuBar()->clear();
  auto fileMenuAction = myMenuBar()->addMenu(fileMenu);
  fileMenuAction->setText(tr("&File"));
  fileMenuAboutToShow();

  auto editMenuAction = myMenuBar()->addMenu(edit);
  editMenuAction->setText(tr("&Edit"));
  editMenuAboutToShow();

  auto viewMenuAction = myMenuBar()->addMenu(view);
  viewMenuAction->setText(tr("&View"));

  // these use the same keyboard shortcut (Ctrl+Return) and should not be
  // enabled at the same time
  actionTableRecalculate->setEnabled(false);

  if (w) {
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

    if (isOfType(w, "MultiLayer")) {
      auto graphMenuAction = myMenuBar()->addMenu(graph);
      graphMenuAction->setText(tr("&Graph"));

      auto plotDataMenuAction = myMenuBar()->addMenu(plotDataMenu);
      plotDataMenuAction->setText(tr("&Data"));

      plotDataMenuAboutToShow();
      if (m_enableQtiPlotFitting) {
        auto analysisMenuAction = myMenuBar()->addMenu(analysisMenu);
        analysisMenuAction->setText(tr("&Analysis"));
        analysisMenuAboutToShow();
      }
      auto formatMenuAction = myMenuBar()->addMenu(format);
      formatMenuAction->setText(tr("For&mat"));

      format->clear();
      format->addAction(actionShowPlotDialog);
      format->addSeparator();
      format->addAction(actionShowScaleDialog);
      format->addAction(actionShowAxisDialog);
      actionShowAxisDialog->setEnabled(true);
      format->addSeparator();
      format->addAction(actionShowGridDialog);
      format->addAction(actionShowTitleDialog);

    } else if (isOfType(w, "Graph3D")) {
      disableActions();

      auto formatMenuAction = myMenuBar()->addMenu(format);
      formatMenuAction->setText(tr("For&mat"));

      actionPrint->setEnabled(true);

      format->clear();
      format->addAction(actionShowPlotDialog);
      format->addAction(actionShowScaleDialog);
      format->addAction(actionShowAxisDialog);
      format->addAction(actionShowTitleDialog);

      auto g3d = dynamic_cast<Graph3D *>(w);
      if (g3d && g3d->coordStyle() == Qwt3D::NOCOORD)
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
      auto plot2DMenuAction = myMenuBar()->addMenu(plot2DMenu);
      plot2DMenuAction->setText(tr("&Plot"));

      auto analysisMenuAction = myMenuBar()->addMenu(analysisMenu);
      analysisMenuAction->setText(tr("&Analysis"));
      analysisMenuAboutToShow();

      auto tableMenuAction = myMenuBar()->addMenu(tableMenu);
      tableMenuAction->setText(tr("&Table"));

      tableMenuAboutToShow();
      actionTableRecalculate->setEnabled(true);
    } else if (isOfType(w, "Matrix")) {
      actionTableRecalculate->setEnabled(true);
      auto plot3DMenuAction = myMenuBar()->addMenu(plot3DMenu);
      plot3DMenuAction->setText(tr("3D &Plot"));

      auto matrixMenuAction = myMenuBar()->addMenu(matrixMenu);
      matrixMenuAction->setText(tr("&Matrix"));
      matrixMenuAboutToShow();

      auto analysisMenuAction = myMenuBar()->addMenu(analysisMenu);
      analysisMenuAction->setText(tr("&Analysis"));
      analysisMenuAboutToShow();
    } else if (isOfType(w, "TiledWindow")) {
      auto tiledWindowMenuAction = myMenuBar()->addMenu(tiledWindowMenu);
      tiledWindowMenuAction->setText(tr("Tiled Window"));
    } else if (!mantidUI->menuAboutToShow(w)) // Note that this call has a
                                              // side-effect (it enables menus)
      disableActions();

  } else
    disableActions();

  if (!currentFolder()->isEmpty()) {
    auto windowsMenuAction = myMenuBar()->addMenu(windowsMenu);
    windowsMenuAction->setText(tr("&Windows"));
    windowsMenuAboutToShow();
  }
  // -- Mantid: add script actions, if any exist --
  QListIterator<QMenu *> mIter(d_user_menus);
  while (mIter.hasNext()) {
    QMenu *item = mIter.next();
    auto itemMenuAction = myMenuBar()->addMenu(item);
    itemMenuAction->setText(item->title());
  }

  const auto &config = Mantid::Kernel::ConfigService::Instance();
  const auto showCatalogMenu = !config.getFacility(config.getFacility().name())
                                    .catalogInfo()
                                    .soapEndPoint()
                                    .empty();

  if (showCatalogMenu) {
    auto catalogMenuAction = myMenuBar()->addMenu(icat);
    catalogMenuAction->setText(tr("&Catalog"));
  }

  // -- INTERFACE MENU --
  auto interfaceMenuAction = myMenuBar()->addMenu(interfaceMenu);
  interfaceMenuAction->setText(tr("&Interfaces"));
  interfaceMenuAboutToShow();

  auto helpMenuAction = myMenuBar()->addMenu(help);
  helpMenuAction->setText(tr("&Help"));

  reloadCustomActions();
}

/**
 * Returns whether a custom interface should be added to the Interfaces menu.
 * @param menu_item: name of the custom interface
 */
bool ApplicationWindow::getMenuSettingsFlag(const QString &menu_item) {
  // Look for the interface in the user menu list
  // If we found the item in the user menu list, return true
  QMenu *menu = nullptr;
  foreach (menu, d_user_menus) {
    if (menu->title() == menu_item)
      return true;
  }

  // If we didn't find it, check whether is was manually removed
  return !removed_interfaces.contains(menu_item);
}

void ApplicationWindow::disableActions() {
  actionPrintAllPlots->setEnabled(false);
  actionPrint->setEnabled(false);

  actionCutSelection->setEnabled(false);
  actionCopySelection->setEnabled(false);
  actionPasteSelection->setEnabled(false);
  actionClearSelection->setEnabled(false);
}

void ApplicationWindow::customColumnActions() {
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

  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  int selectedCols = t->selectedColsNumber();
  if (selectedCols == 1) {
    int col = t->selectedColumn();
    if (col > 0) {
      actionMoveColFirst->setEnabled(true);
      actionMoveColLeft->setEnabled(true);
    }

    if (col < t->numCols() - 1) {
      actionMoveColRight->setEnabled(true);
      actionMoveColLast->setEnabled(true);
    }
  }

  if (selectedCols >= 1) {
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
void ApplicationWindow::setExitCode(int code) { m_exitCode = code; }

/** Get the exit code to be returned by the application at
 * exit. Used by MantidPlot unit tests to signal failure.
 *
 * @return code :: int code, non-zero for failure
 */
int ApplicationWindow::getExitCode() { return m_exitCode; }

void ApplicationWindow::customToolBars(MdiSubWindow *w) {
  disableToolbars();
  if (!w)
    return;

  if (isOfType(w, "MultiLayer") && d_plot_tool_bar) {
    if (!plotTools->isVisible())
      plotTools->show();
    plotTools->setEnabled(true);
    customMultilayerToolButtons(dynamic_cast<MultiLayer *>(w));
    if (d_format_tool_bar && !formatToolBar->isVisible()) {
      formatToolBar->setEnabled(true);
      formatToolBar->show();
    }
  } else if (isOfType(w, "Graph3D")) {
    custom3DActions(w);
  }
}

void ApplicationWindow::disableToolbars() { plotTools->setEnabled(false); }

/**
 * Show/hide MantidPlot toolbars.
 * @param visible If true, make toolbar visible, if false - hidden
 */
void ApplicationWindow::setToolbarsVisible(bool visible) {
  standardTools->setVisible(visible);
  displayBar->setVisible(visible);
  plotTools->setVisible(visible);
  formatToolBar->setVisible(visible);
}

void ApplicationWindow::plot3DRibbon() {
  MdiSubWindow *w = activeWindow(TableWindow);
  if (!w)
    return;

  Table *table = static_cast<Table *>(w);
  if (table->selectedColumns().count() == 1) {
    if (!validFor3DPlot(table))
      return;
    plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Ribbon);
  } else
    QMessageBox::warning(
        this, tr("MantidPLot - Plot error"),
        tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plot3DWireframe() {
  plot3DMatrix(nullptr, Qwt3D::WIREFRAME);
}

void ApplicationWindow::plot3DHiddenLine() {
  plot3DMatrix(nullptr, Qwt3D::HIDDENLINE);
}

void ApplicationWindow::plot3DPolygons() {
  plot3DMatrix(nullptr, Qwt3D::FILLED);
}

void ApplicationWindow::plot3DWireSurface() {
  plot3DMatrix(nullptr, Qwt3D::FILLEDMESH);
}

void ApplicationWindow::plot3DBars() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table")) {
    Table *table = static_cast<Table *>(w);
    if (!validFor3DPlot(table))
      return;

    if (table->selectedColumns().count() == 1)
      plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Bars);
    else
      QMessageBox::warning(
          this, tr("MantidPlot - Plot error"),
          tr("You must select exactly one column for plotting!")); // Mantid
  } else if (w->inherits("Matrix"))
    plot3DMatrix(nullptr, Qwt3D::USER);
}

void ApplicationWindow::plot3DScatter() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table")) {
    Table *table = static_cast<Table *>(w);
    if (!validFor3DPlot(table))
      return;

    if (table->selectedColumns().count() == 1)
      plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Scatter);
    else
      QMessageBox::warning(
          this, tr("MantidPlot - Plot error"),
          tr("You must select exactly one column for plotting!")); // Mantid
  } else if (w->inherits("Matrix"))
    plot3DMatrix(nullptr, Qwt3D::POINTS);
}

void ApplicationWindow::plot3DTrajectory() {
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!table)
    return;
  if (!validFor3DPlot(table))
    return;

  if (table->selectedColumns().count() == 1)
    plotXYZ(table, table->colName(table->selectedColumn()),
            Graph3D::Trajectory);
  else
    QMessageBox::warning(
        this, tr("MantidPlot - Plot error"),
        tr("You must select exactly one column for plotting!")); // Mantid
}

void ApplicationWindow::plotBoxDiagram() { generate2DGraph(GraphOptions::Box); }

void ApplicationWindow::plotVerticalBars() {
  generate2DGraph(GraphOptions::VerticalBars);
}

void ApplicationWindow::plotHorizontalBars() {
  generate2DGraph(GraphOptions::HorizontalBars);
}

MultiLayer *ApplicationWindow::plotHistogram() {
  return generate2DGraph(GraphOptions::Histogram);
}

MultiLayer *ApplicationWindow::plotHistogram(Matrix *m) {
  if (!m) {
    m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
    if (!m)
      return nullptr;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer *g = new MultiLayer(this);
  initMultilayerPlot(g, generateUniqueName(tr("Graph")));

  Graph *plot = g->activeGraph();
  setPreferences(plot);
  plot->addHistogram(m);

  QApplication::restoreOverrideCursor();
  return g;
}

void ApplicationWindow::plotArea() { generate2DGraph(GraphOptions::Area); }

void ApplicationWindow::plotPie() {
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!table)
    return;

  if (table->selectedColumns().count() != 1) {
    QMessageBox::warning(
        this, tr("MantidPlot - Plot error"), // Mantid
        tr("You must select exactly one column for plotting!"));
    return;
  }

  QStringList s = table->selectedColumns();
  if (s.count() > 0) {
    multilayerPlot(table, s, GraphOptions::Pie, table->topSelectedRow(),
                   table->bottomSelectedRow());
  } else
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Please select a column to plot!")); // Mantid
}

void ApplicationWindow::plotL() { generate2DGraph(GraphOptions::Line); }

void ApplicationWindow::plotP() { generate2DGraph(GraphOptions::Scatter); }

void ApplicationWindow::plotLP() { generate2DGraph(GraphOptions::LineSymbols); }

void ApplicationWindow::plotVerticalDropLines() {
  generate2DGraph(GraphOptions::VerticalDropLines);
}

void ApplicationWindow::plotSpline() { generate2DGraph(GraphOptions::Spline); }

void ApplicationWindow::plotVertSteps() {
  generate2DGraph(GraphOptions::VerticalSteps);
}

void ApplicationWindow::plotHorSteps() {
  generate2DGraph(GraphOptions::HorizontalSteps);
}

void ApplicationWindow::plotVectXYXY() {
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!table)
    return;
  if (!validFor2DPlot(table))
    return;

  QStringList s = table->selectedColumns();
  if (s.count() == 4) {
    multilayerPlot(table, s, GraphOptions::VectXYXY, table->topSelectedRow(),
                   table->bottomSelectedRow());
  } else
    QMessageBox::warning(
        this, tr("MantidPlot - Error"),
        tr("Please select four columns for this operation!")); // Mantid
}

void ApplicationWindow::plotVectXYAM() {
  Table *table = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!table)
    return;
  if (!validFor2DPlot(table))
    return;

  QStringList s = table->selectedColumns();
  if (s.count() == 4) {
    multilayerPlot(table, s, GraphOptions::VectXYAM, table->topSelectedRow(),
                   table->bottomSelectedRow());
  } else
    QMessageBox::warning(
        this, tr("MantidPlot - Error"),
        tr("Please select four columns for this operation!")); // Mantid
}

QString ApplicationWindow::stemPlot(Table *t, const QString &colName, int power,
                                    int startRow, int endRow) {
  if (!t)
    return QString();

  int col = t->colIndex(colName);
  if (col < 0) {
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
  for (int j = startRow; j <= endRow; j++) {
    if (!t->text(j, col).isEmpty())
      rows++;
  }

  if (rows >= 1) {
    double *data = (double *)malloc(rows * sizeof(double));
    if (!data) {
      result += tr("Not enough memory for this dataset!") + "\n";
      return result;
    }

    result += "\n" + tr("Stem") + " | " + tr("Leaf");
    result += "\n---------------------\n";

    int row = 0;
    for (int j = startRow; j <= endRow; j++) {
      if (!t->text(j, col).isEmpty()) {
        data[row] = t->cell(j, col);
        row++;
      }
    }
    gsl_sort(data, 1, rows);

    if (power > 1e3) {
      power = static_cast<int>(
          std::ceil(log10(data[rows - 1] - data[0]) - log10(rows - 1.0)));
      bool ok;
      int input = QInputDialog::getInteger(
          this, tr("Please confirm the stem unit!"),
          tr("Data set") + ": " + colName + ", " + tr("stem unit") +
              " = 10<sup>n</sup>, n = ",
          power, -1000, 1000, 1, &ok);
      if (ok)
        power = input;
    }

    double stem_unit = pow(10.0, power);
    double leaf_unit = stem_unit / 10.0;

    int prev_stem = int(data[0] / stem_unit);
    result += "      " + QString::number(prev_stem) + " | ";

    for (int j = 0; j < rows; j++) {
      double val = data[j];
      int stem = int(val / stem_unit);
      int leaf = int(qRound((val - stem * stem_unit) / leaf_unit));
      for (int k = prev_stem + 1; k < stem + 1; k++)
        result += "\n      " + QString::number(k) + " | ";
      result += QString::number(leaf);
      prev_stem = stem;
    }

    result += "\n---------------------\n";
    result += tr("Stem unit") + ": " + locale().toString(stem_unit) + "\n";
    result += tr("Leaf unit") + ": " + locale().toString(leaf_unit) + "\n";

    QString legend = tr("Key") + ": " + QString::number(prev_stem) + " | ";
    int leaf =
        int(qRound((data[rows - 1] - prev_stem * stem_unit) / leaf_unit));
    legend += QString::number(leaf);
    legend += " " + tr("means") + ": " +
              locale().toString(prev_stem * stem_unit + leaf * leaf_unit) +
              "\n";

    result += legend + "---------------------\n";
    free(data);
  } else
    result += "\t" + tr("Input error: empty data set!") + "\n";
  return result;
}

Note *ApplicationWindow::newStemPlot() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return nullptr;

  if (!t->hasSelection())
    return nullptr;

  Note *n = newNote();
  if (!n)
    return nullptr;
  n->hide();

  QStringList lst = t->selectedColumns();
  if (lst.isEmpty()) {
    for (int i = t->leftSelectedColumn(); i <= t->rightSelectedColumn(); i++)
      n->setText(n->text() +
                 stemPlot(t, t->colName(i), 1001, t->topSelectedRow() + 1,
                          t->bottomSelectedRow() + 1) +
                 "\n");
  } else {
    for (int i = 0; i < lst.count(); i++)
      n->setText(n->text() + stemPlot(t, lst[i], 1001) + "\n");
  }

  n->show();
  return n;
}

void ApplicationWindow::renameListViewItem(const QString &oldName,
                                           const QString &newName) {
  auto found =
      lv->findItems(oldName, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(0, newName);
}

void ApplicationWindow::setListViewLabel(const QString &caption,
                                         const QString &label) {
  auto found =
      lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(5, label);
}

void ApplicationWindow::setListViewDate(const QString &caption,
                                        const QString &date) {
  auto found =
      lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(4, date);
}

void ApplicationWindow::setListView(const QString &caption,
                                    const QString &view) {
  auto found =
      lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(2, view);
}

void ApplicationWindow::setListViewSize(const QString &caption,
                                        const QString &size) {
  auto found =
      lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(3, size);
}

QString ApplicationWindow::listViewDate(const QString &caption) {
  auto found =
      lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    return found[0]->text(4);
  else
    return "";
}

void ApplicationWindow::updateTableNames(const QString &oldName,
                                         const QString &newName) {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    auto ml = dynamic_cast<MultiLayer *>(w);
    auto g3d = dynamic_cast<Graph3D *>(w);
    if (ml) {
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers)
        g->updateCurveNames(oldName, newName);
    } else if (g3d) {
      QString name = g3d->formula();
      if (name.contains(oldName, Qt::CaseSensitive)) {
        name.replace(oldName, newName);
        g3d->setPlotAssociation(name);
      }
    }
  }
}

void ApplicationWindow::updateColNames(const QString &oldName,
                                       const QString &newName) {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    auto ml = dynamic_cast<MultiLayer *>(w);
    auto g3d = dynamic_cast<Graph3D *>(w);

    if (ml) {
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers)
        g->updateCurveNames(oldName, newName, false);
    } else if (g3d) {
      QString name = g3d->formula();
      if (name.contains(oldName)) {
        name.replace(oldName, newName);
        dynamic_cast<Graph3D *>(w)->setPlotAssociation(name);
      }
    }
  }
}

void ApplicationWindow::changeMatrixName(const QString &oldName,
                                         const QString &newName) {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(w);
      if (!g3d)
        return;
      QString s = g3d->formula();
      if (s.contains(oldName)) {
        s.replace(oldName, newName);
        g3d->setPlotAssociation(s);
      }
    } else if (isOfType(w, "MultiLayer")) {
      auto ml = dynamic_cast<MultiLayer *>(w);
      if (!ml)
        return;
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers) {
        for (int i = 0; i < g->curves(); i++) {
          QwtPlotItem *sp = dynamic_cast<QwtPlotItem *>(g->plotItem(i));
          if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram &&
              sp->title().text() == oldName)
            sp->setTitle(newName);
        }
      }
    }
  }
}

void ApplicationWindow::remove3DMatrixPlots(Matrix *m) {
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    auto g3d = dynamic_cast<Graph3D *>(w);
    if (g3d && g3d->matrix() == m)
      g3d->clearData();
    auto ml = dynamic_cast<MultiLayer *>(w);

    if (ml) {
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers) {
        for (int i = 0; i < g->curves(); i++) {
          if (g->curveType(i) == GraphOptions::Histogram) {
            QwtHistogram *h = dynamic_cast<QwtHistogram *>(g->plotItem(i));
            if (h && h->matrix() == m)
              g->removeCurve(i);
          } else {
            Spectrogram *sp = dynamic_cast<Spectrogram *>(g->plotItem(i));
            if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram &&
                sp->matrix() == m)
              g->removeCurve(i);
          }
        }
      }
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateMatrixPlots(MdiSubWindow *window) {
  Matrix *m = dynamic_cast<Matrix *>(window);
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(w);
      if (g3d && g3d->matrix() == m)
        g3d->updateMatrixData(m);
    } else if (isOfType(w, "MultiLayer")) {
      auto ml = dynamic_cast<MultiLayer *>(w);
      if (!ml)
        continue;

      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers) {
        for (int i = 0; i < g->curves(); i++) {
          if (g->curveType(i) == GraphOptions::Histogram) {
            QwtHistogram *h = dynamic_cast<QwtHistogram *>(g->plotItem(i));
            if (h && h->matrix() == m)
              h->loadData();
          } else {
            Spectrogram *sp = dynamic_cast<Spectrogram *>(g->plotItem(i));
            if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram &&
                sp->matrix() == m)
              sp->updateData(m);
          }
        }
        g->updatePlot();
      }
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::add3DData() {
  if (!hasTable()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no tables available in this project.</h4>"
           "<p><h4>Please create a table and try again!</h4>"));
    return;
  }

  QStringList zColumns = columnsList(Table::Z);
  if ((int)zColumns.count() <= 0) {
    QMessageBox::critical(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("There are no available columns with plot designation set to Z!"));
    return;
  }

  DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect(ad, SIGNAL(options(const QString &)), this,
          SLOT(insertNew3DData(const QString &)));
  ad->setWindowTitle(tr("MantidPlot - Choose data set")); // Mantid
  ad->setCurveNames(zColumns);
  ad->exec();
}

void ApplicationWindow::change3DData() {
  DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect(ad, SIGNAL(options(const QString &)), this,
          SLOT(change3DData(const QString &)));

  ad->setWindowTitle(tr("MantidPlot - Choose data set")); // Mantid
  ad->setCurveNames(columnsList(Table::Z));
  ad->exec();
}

void ApplicationWindow::change3DMatrix() {
  DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " : ", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect(ad, SIGNAL(options(const QString &)), this,
          SLOT(change3DMatrix(const QString &)));

  ad->setWindowTitle(tr("MantidPlot - Choose matrix to plot")); // Mantid
  ad->setCurveNames(matrixNames());

  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (g && g->matrix())
    ad->setCurentDataSet(g->matrix()->objectName());
  ad->exec();
}

void ApplicationWindow::change3DMatrix(const QString &matrix_name) {
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D *g = dynamic_cast<Graph3D *>(w);
  Matrix *m = matrix(matrix_name);
  if (m && g)
    g->addMatrixData(m);

  emit modified();
}

void ApplicationWindow::add3DMatrixPlot() {
  QStringList matrices = matrixNames();
  if (static_cast<int>(matrices.count()) <= 0) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no matrices available in this project.</h4>"
           "<p><h4>Please create a matrix and try again!</h4>"));
    return;
  }

  DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " :", this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  connect(ad, SIGNAL(options(const QString &)), this,
          SLOT(insert3DMatrixPlot(const QString &)));

  ad->setWindowTitle(tr("MantidPlot - Choose matrix to plot")); // Mantid
  ad->setCurveNames(matrices);
  ad->exec();
}

void ApplicationWindow::insert3DMatrixPlot(const QString &matrix_name) {
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  auto g3d = dynamic_cast<Graph3D *>(w);
  if (g3d)
    g3d->addMatrixData(matrix(matrix_name));

  emit modified();
}

void ApplicationWindow::insertNew3DData(const QString &colName) {
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  auto g3d = dynamic_cast<Graph3D *>(w);
  if (g3d)
    g3d->insertNewData(table(colName), colName);
  emit modified();
}

void ApplicationWindow::change3DData(const QString &colName) {
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  auto g3d = dynamic_cast<Graph3D *>(w);
  if (g3d)
    g3d->changeDataColumn(table(colName), colName);

  emit modified();
}

void ApplicationWindow::editSurfacePlot() {
  MdiSubWindow *w = activeWindow(Plot3DWindow);
  if (!w)
    return;

  Graph3D *g = dynamic_cast<Graph3D *>(w);
  if (!g)
    return;

  SurfaceDialog *sd = new SurfaceDialog(this);
  sd->setAttribute(Qt::WA_DeleteOnClose);

  if (g->hasData() && g->userFunction())
    sd->setFunction(g);
  else if (g->hasData() && g->parametricSurface())
    sd->setParametricSurface(g);
  sd->exec();
}

void ApplicationWindow::newSurfacePlot() {
  SurfaceDialog *sd = new SurfaceDialog(this);
  sd->setAttribute(Qt::WA_DeleteOnClose);
  sd->exec();
}

Graph3D *ApplicationWindow::plotSurface(const QString &formula, double xl,
                                        double xr, double yl, double yr,
                                        double zl, double zr, size_t columns,
                                        size_t rows) {
  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this);
  plot->resize(500, 400);
  plot->setWindowTitle(label);
  plot->setName(label);
  customPlot3D(plot);
  plot->addFunction(formula, xl, xr, yl, yr, zl, zr, columns, rows);

  initPlot3D(plot);

  emit modified();
  return plot;
}

Graph3D *ApplicationWindow::plotParametricSurface(
    const QString &xFormula, const QString &yFormula, const QString &zFormula,
    double ul, double ur, double vl, double vr, int columns, int rows,
    bool uPeriodic, bool vPeriodic) {
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

void ApplicationWindow::updateSurfaceFuncList(const QString &s) {
  surfaceFunc.removeAll(s);
  surfaceFunc.push_front(s);
  while ((int)surfaceFunc.size() > 10)
    surfaceFunc.pop_back();
}

Graph3D *ApplicationWindow::dataPlot3D(const QString &caption,
                                       const QString &formula, double xl,
                                       double xr, double yl, double yr,
                                       double zl, double zr) {
  int pos = formula.indexOf("_", 0);
  QString wCaption = formula.left(pos);

  Table *w = table(wCaption);
  if (!w)
    return nullptr;

  int posX = formula.indexOf("(", pos);
  QString xCol = formula.mid(pos + 1, posX - pos - 1);

  pos = formula.indexOf(",", posX);
  posX = formula.indexOf("(", pos);
  QString yCol = formula.mid(pos + 1, posX - pos - 1);

  Graph3D *plot = new Graph3D("", this, nullptr);
  plot->addData(w, xCol, yCol, xl, xr, yl, yr, zl, zr);
  plot->update();

  QString label = caption;
  while (alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  plot->setWindowTitle(label);
  plot->setName(label);
  initPlot3D(plot);

  return plot;
}

Graph3D *ApplicationWindow::newPlot3D() {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this, nullptr);
  plot->setWindowTitle(label);
  plot->setName(label);

  customPlot3D(plot);
  initPlot3D(plot);

  emit modified();
  QApplication::restoreOverrideCursor();
  return plot;
}

Graph3D *ApplicationWindow::plotXYZ(Table *table, const QString &zColName,
                                    int type) {
  int zCol = table->colIndex(zColName);
  if (zCol < 0)
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  Graph3D *plot = new Graph3D("", this, nullptr);
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

Graph3D *ApplicationWindow::openPlotXYZ(const QString &caption,
                                        const QString &formula, double xl,
                                        double xr, double yl, double yr,
                                        double zl, double zr) {
  int pos = formula.indexOf("_", 0);
  QString wCaption = formula.left(pos);

  Table *w = table(wCaption);
  if (!w)
    return nullptr;

  int posX = formula.indexOf("(X)", pos);
  QString xColName = formula.mid(pos + 1, posX - pos - 1);

  pos = formula.indexOf(",", posX);

  posX = formula.indexOf("(Y)", pos);
  QString yColName = formula.mid(pos + 1, posX - pos - 1);

  pos = formula.indexOf(",", posX);
  posX = formula.indexOf("(Z)", pos);
  QString zColName = formula.mid(pos + 1, posX - pos - 1);

  int xCol = w->colIndex(xColName);
  int yCol = w->colIndex(yColName);
  int zCol = w->colIndex(zColName);

  Graph3D *plot = new Graph3D("", this, nullptr);
  plot->loadData(w, xCol, yCol, zCol, xl, xr, yl, yr, zl, zr);

  QString label = caption;
  if (alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  plot->setWindowTitle(label);
  plot->setName(label);
  initPlot3D(plot);
  return plot;
}

void ApplicationWindow::customPlot3D(Graph3D *plot) {
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

void ApplicationWindow::initPlot3D(Graph3D *plot) {
  addMdiSubWindow(plot);
  connectSurfacePlot(plot);

  plot->setWindowIcon(getQPixmap("trajectory_xpm"));
  plot->show();
  plot->setFocus();

  customMenu(plot);
  customToolBars(plot);
  emit modified();
}

void ApplicationWindow::exportMatrix() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  ImageExportDialog *ied =
      new ImageExportDialog(this, m != nullptr, d_extended_export_dialog);
  ied->setDirectory(workingDir);
  ied->selectFilter(d_image_export_filter);
  if (ied->exec() != QDialog::Accepted)
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
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(
        this, tr("MantidPlot - Export error"), // Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that "
           "you have the right to write to this location!")
            .arg(file_name));
    return;
  }

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") ||
      selected_filter.contains(".ps"))
    m->exportVector(file_name, ied->resolution(), ied->color(),
                    ied->keepAspect(), ied->pageSize());
  else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i = 0; i < (int)list.count(); i++) {
      if (selected_filter.contains("." + (list[i]).toLower()))
        m->image().save(file_name, list[i], ied->quality());
    }
  }
}

Matrix *ApplicationWindow::importImage(const QString &fileName) {
  QString fn = fileName;
  if (fn.isEmpty()) {
    QList<QByteArray> list = QImageReader::supportedImageFormats();
    QString filter = tr("images") + " (", aux1, aux2;
    for (int i = 0; i < (int)list.count(); i++) {
      aux1 = " *." + list[i] + " ";
      aux2 += " *." + list[i] + ";;";
      filter += aux1;
    }
    filter += ");;" + aux2;

    fn = QFileDialog::getOpenFileName(this,
                                      tr("MantidPlot - Import image from file"),
                                      imagesDirPath, filter); // Mantid
    if (!fn.isEmpty()) {
      QFileInfo fi(fn);
      imagesDirPath = fi.absolutePath();
    }
  }

  QImage image(fn);
  if (image.isNull())
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MdiSubWindow *w = activeWindow(MatrixWindow);
  Matrix *m = dynamic_cast<Matrix *>(w);
  if (m) {
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

void ApplicationWindow::loadImage() {
  QList<QByteArray> list = QImageReader::supportedImageFormats();
  QString filter = tr("images") + " (", aux1, aux2;
  for (int i = 0; i < (int)list.count(); i++) {
    aux1 = " *." + list[i] + " ";
    aux2 += " *." + list[i] + ";;";
    filter += aux1;
  }
  filter += ");;" + aux2;

  QString fn = QFileDialog::getOpenFileName(
      this, tr("MantidPlot - Load image from file"), imagesDirPath,
      filter); // Mantid
  if (!fn.isEmpty()) {
    loadImage(fn);
    QFileInfo fi(fn);
    imagesDirPath = fi.absolutePath();
  }
}

void ApplicationWindow::loadImage(const QString &fn) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer *plot = multilayerPlot(generateUniqueName(tr("Graph")));
  plot->setWindowLabel(fn);
  plot->setCaptionPolicy(MdiSubWindow::Both);

  Graph *g = plot->activeGraph();
  g->setTitle("");
  for (int i = 0; i < 4; i++)
    g->enableAxis(i, false);
  g->removeLegend();
  g->addImage(fn);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::loadScriptRepo() {
  MantidQt::API::ScriptRepositoryView *ad =
      new MantidQt::API::ScriptRepositoryView(this);
  connect(ad, SIGNAL(loadScript(const QString)), this,
          SLOT(loadScript(const QString &)));
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void ApplicationWindow::polishGraph(Graph *g, int style) {
  if (style == GraphOptions::VerticalBars ||
      style == GraphOptions::HorizontalBars ||
      style == GraphOptions::Histogram) {
    QList<int> ticksList;
    int ticksStyle = ScaleDraw::Out;
    ticksList << ticksStyle << ticksStyle << ticksStyle << ticksStyle;
    g->setMajorTicksType(ticksList);
    g->setMinorTicksType(ticksList);
  }
  if (style == GraphOptions::HorizontalBars) {
    g->setAxisTitle(QwtPlot::xBottom, tr("X Axis Title"));
    g->setAxisTitle(QwtPlot::yLeft, tr("Y Axis Title"));
  }
}

MultiLayer *ApplicationWindow::multilayerPlot(const QString &caption,
                                              int layers, int rows, int cols) {
  MultiLayer *ml = new MultiLayer(this, layers, rows, cols);
  QString label = caption;
  initMultilayerPlot(ml, label.replace(QRegExp("_"), "-"));
  return ml;
}

MultiLayer *ApplicationWindow::newGraph(const QString &caption) {
  MultiLayer *ml = multilayerPlot(generateUniqueName(caption));
  if (ml) {
    Graph *g = ml->activeGraph();
    setPreferences(g);
    g->newLegend();
  }
  return ml;
}

/**
 * Prepares MultiLayer window for plotting - creates it if necessary, clears it,
 * applies initial
 * settings etc.
 * @param isNew         :: Whether the Graph used for plotting was created, or
 * the old one was used
 * @param window        :: Existing MultiLayer window. If NULL - a new one will
 * be created
 * @param newWindowName :: Name of the new window if one is created
 * @param clearWindow   :: Whether to clear existing window before plotting.
 * Ignored if window is NULL
 * @return Pointer to created window if window == NULL, otherwise - window.
 */
MultiLayer *ApplicationWindow::prepareMultiLayer(bool &isNew,
                                                 MultiLayer *window,
                                                 const QString &newWindowName,
                                                 bool clearWindow) {
  isNew = false;

  if (window == nullptr) { // If plot window is not specified, create a new one
    window = multilayerPlot(generateUniqueName(newWindowName + "-"));
    window->setCloseOnEmpty(true);
    isNew = true;
  } else if (clearWindow) {
    window->setLayersNumber(0); // Clear by removing all the layers
  }

  if (window->isEmpty()) { // This will add a new layer in two situations: when
                           // we've cleared the window manually,
    // or when the window specified didn't actually have any layers
    window->addLayer();
    isNew = true;
  }

  if (isNew) { // If new graph was created, need to set some initial stuff

    Graph *g = window->activeGraph(); // We use active graph only. No support
                                      // for proper _multi_ layers yet.

    connect(g, SIGNAL(curveRemoved()), window, SLOT(maybeNeedToClose()),
            Qt::QueuedConnection);
    setPreferences(g);
    g->newLegend();
    g->setTitle(newWindowName);
  }

  return window;
}

MultiLayer *ApplicationWindow::multilayerPlot(
    Table *w, const QStringList &colList, int style, int startRow,
    int endRow) { // used when plotting selected columns
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer *g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph *ag = g->activeGraph();
  if (!ag)
    return nullptr;

  setPreferences(ag);
  ag->addCurves(w, colList, style, defaultCurveLineWidth, defaultSymbolSize,
                startRow, endRow);

  polishGraph(ag, style);
  ag->newLegend();

  ag->setAutoScale(); // Mantid
  /* The 'setAutoScale' above is needed to make sure that the plot initially
   * encompasses all the
   * data points. However, this has the side-effect suggested by its name: all
   * the axes become
   * auto-scaling if the data changes. If, in the plot preferences, autoscaling
   * has been disabled
   * the the next line re-fixes the axes
   */
  if (!autoscale2DPlots)
    ag->enableAutoscaling(false);

  // Set graph title to the same as the table
  if (auto mantidTable = dynamic_cast<MantidTable *>(w)) {
    ag->setTitle(mantidTable->getWorkspaceName().c_str());
  }
  QApplication::restoreOverrideCursor();
  return g;
}

MultiLayer *ApplicationWindow::multilayerPlot(
    int c, int r, int style) { // used when plotting from the panel menu
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return nullptr;

  if (!validFor2DPlot(t))
    return nullptr;

  QStringList list = t->selectedYColumns();
  if ((int)list.count() < 1) {
    QMessageBox::warning(this, tr("MantidPlot - Plot error"),
                         tr("Please select a Y column to plot!")); // Mantid
    return nullptr;
  }

  int curves = list.count();
  if (r < 0)
    r = curves;

  int layers = c * r;
  MultiLayer *g = multilayerPlot(generateUniqueName(tr("Graph")), layers, r, c);
  QList<Graph *> layersList = g->layersList();
  int i = 0;
  foreach (Graph *ag, layersList) {
    setPreferences(ag);
    if (i < curves)
      ag->addCurves(t, QStringList(list[i]), style, defaultCurveLineWidth,
                    defaultSymbolSize);
    ag->newLegend();
    polishGraph(ag, style);
    i++;
  }
  g->arrangeLayers(false, false);
  return g;
}

MultiLayer *ApplicationWindow::multilayerPlot(
    const QStringList &colList) { // used when plotting from wizard
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  MultiLayer *g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph *ag = g->activeGraph();
  setPreferences(ag);
  polishGraph(ag, defaultCurveStyle);
  int curves = (int)colList.count();
  int errorBars = 0;
  for (int i = 0; i < curves; i++) {
    if (colList[i].contains("(yErr)") || colList[i].contains("(xErr)"))
      errorBars++;
  }

  for (int i = 0; i < curves; i++) {
    QString s = colList[i];
    int pos = s.indexOf(":", 0);
    QString caption = s.left(pos) + "_";
    Table *w = dynamic_cast<Table *>(table(caption));

    int posX = s.indexOf("(X)", pos);
    QString xColName = caption + s.mid(pos + 2, posX - pos - 2);
    int xCol = w->colIndex(xColName);

    posX = s.indexOf(",", posX);
    int posY = s.indexOf("(Y)", posX);
    QString yColName = caption + s.mid(posX + 2, posY - posX - 2);

    PlotCurve *c = nullptr;
    if (s.contains("(yErr)") || s.contains("(xErr)")) {
      posY = s.indexOf(",", posY);
      int posErr, errType;
      if (s.contains("(yErr)")) {
        errType = QwtErrorPlotCurve::Vertical;
        posErr = s.indexOf("(yErr)", posY);
      } else {
        errType = QwtErrorPlotCurve::Horizontal;
        posErr = s.indexOf("(xErr)", posY);
      }

      QString errColName = caption + s.mid(posY + 2, posErr - posY - 2);
      c = dynamic_cast<PlotCurve *>(
          ag->addErrorBars(xColName, yColName, w, errColName, errType));
    } else
      c = dynamic_cast<PlotCurve *>(
          ag->insertCurve(w, xCol, yColName, defaultCurveStyle));

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

void ApplicationWindow::initMultilayerPlot(MultiLayer *g, const QString &name) {
  QString label = name;
  while (alreadyUsedName(label))
    label = generateUniqueName(tr("Graph"));

  g->setWindowTitle(label);
  g->setName(label);
  g->setScaleLayersOnPrint(d_scale_plots_on_print);
  g->printCropmarks(d_print_cropmarks);

  connectMultilayerPlot(g);

  addMdiSubWindow(g);
}

void ApplicationWindow::customizeTables(
    const QColor &bgColor, const QColor &textColor, const QColor &headerColor,
    const QFont &textFont, const QFont &headerFont, bool showComments) {
  tableBkgdColor = bgColor;
  tableTextColor = textColor;
  tableHeaderColor = headerColor;
  tableTextFont = textFont;
  tableHeaderFont = headerFont;
  d_show_table_comments = showComments;

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (w->inherits("Table")) {
      auto table = dynamic_cast<Table *>(w);
      if (table)
        customTable(table);
    }
  }
}

void ApplicationWindow::setAutoUpdateTableValues(bool on) {
  if (d_auto_update_table_values == on)
    return;

  d_auto_update_table_values = on;

  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->inherits("Table"))
        dynamic_cast<Table *>(w)->setAutoUpdateValues(
            d_auto_update_table_values);
    }
    f = f->folderBelow();
  }
}

void ApplicationWindow::customTable(Table *w) {
  QPalette palette;
  palette.setColor(QPalette::Base, QColor(tableBkgdColor));
  palette.setColor(QPalette::Text, QColor(tableTextColor));
  w->setPalette(palette);

  w->setHeaderColor(tableHeaderColor);
  w->setTextFont(tableTextFont);
  w->setHeaderFont(tableHeaderFont);
  w->showComments(d_show_table_comments);
  w->setNumericPrecision(d_decimal_digits);
}

void ApplicationWindow::setPreferences(Graph *g) {
  if (!g)
    return;

  if (!g->isPiePlot()) {
    for (int i = 0; i < QwtPlot::axisCnt; i++) {
      bool show = d_show_axes[i];
      g->enableAxis(i, show);
      if (show) {
        ScaleDraw *sd =
            static_cast<ScaleDraw *>(g->plotWidget()->axisScaleDraw(i));
        sd->enableComponent(QwtAbstractScaleDraw::Labels,
                            d_show_axes_labels[i]);
        sd->setSpacing(d_graph_tick_labels_dist);
        if (i == QwtPlot::yRight && !d_show_axes_labels[i])
          g->setAxisTitle(i, tr(" "));
      }
    }

    // set the scale type i.e. log or linear
    g->setScale(QwtPlot::yLeft, d_axes_scales[0]);
    g->setScale(QwtPlot::yRight, d_axes_scales[1]);
    g->setScale(QwtPlot::xBottom, d_axes_scales[2]);
    g->setScale(QwtPlot::xTop, d_axes_scales[3]);

    QList<int> ticksList;
    ticksList << majTicksStyle << majTicksStyle << majTicksStyle
              << majTicksStyle;
    g->setMajorTicksType(ticksList);
    ticksList.clear();
    ticksList << minTicksStyle << minTicksStyle << minTicksStyle
              << minTicksStyle;
    g->setMinorTicksType(ticksList);

    g->setTicksLength(minTicksLength, majTicksLength);
    g->setAxesLinewidth(axesLineWidth);
    g->drawAxesBackbones(drawBackbones);
    for (int i = 0; i < QwtPlot::axisCnt; i++)
      g->setAxisTitleDistance(i, d_graph_axes_labels_dist);
    //    need to call the plot functions for log/linear, errorbars and
    //    distribution stuff
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
Table *ApplicationWindow::newTable() {
  Table *w = new Table(scriptingEnv(), 30, 2, "", this, nullptr);
  initTable(w, generateUniqueName(tr("Table")));
  w->showNormal();
  return w;
}

/*
 *used when opening a project file
 */
Table *ApplicationWindow::newTable(const QString &caption, int r, int c) {
  Table *w = new Table(scriptingEnv(), r, c, "", this, nullptr);
  initTable(w, caption);
  if (w->objectName() != caption) { // the table was renamed
    renamedTables << caption << w->objectName();
    if (d_inform_rename_table) {
      QMessageBox::warning(
          this, tr("MantidPlot - Renamed Window"), // Mantid
          tr("The table '%1' already exists. It has been renamed '%2'.")
              .arg(caption)
              .arg(w->objectName()));
    }
  }
  w->showNormal();
  return w;
}

bool ApplicationWindow::isDeleteWorkspacePromptEnabled() {
  return d_inform_delete_workspace;
}

Table *ApplicationWindow::newTable(int r, int c, const QString &name,
                                   const QString &legend) {
  Table *w = new Table(scriptingEnv(), r, c, legend, this, nullptr);
  initTable(w, name);
  return w;
}

Table *ApplicationWindow::newTable(const QString &caption, int r, int c,
                                   const QString &text) {
  QStringList lst = caption.split("\t", QString::SkipEmptyParts);
  QString legend = QString();
  if (lst.count() == 2)
    legend = lst[1];

  Table *w = new Table(scriptingEnv(), r, c, legend, this, nullptr);

  QStringList rows = text.split("\n", QString::SkipEmptyParts);
  QString rlist = rows[0];
  QStringList list = rlist.split("\t");
  w->setHeader(list);

  for (int i = 0; i < r; i++) {
    rlist = rows[i + 1];
    list = rlist.split("\t");
    for (int j = 0; j < c; j++)
      w->setText(i, j, list[j]);
  }

  initTable(w, lst[0]);
  w->showNormal();
  return w;
}

Table *ApplicationWindow::newHiddenTable(const QString &name,
                                         const QString &label, int r, int c,
                                         const QString &text) {
  Table *w = new Table(scriptingEnv(), r, c, label, this, nullptr);

  if (!text.isEmpty()) {
    QStringList rows = text.split("\n", QString::SkipEmptyParts);
    QStringList list = rows[0].split("\t");
    w->setHeader(list);

    QString rlist;
    for (int i = 0; i < r; i++) {
      rlist = rows[i + 1];
      list = rlist.split("\t");
      for (int j = 0; j < c; j++)
        w->setText(i, j, list[j]);
    }
  }

  initTable(w, name);
  hideWindow(w);
  return w;
}

/* Perform initialization on a Table?
 * @param w :: table that was created
 * @param caption :: title to set
 */
void ApplicationWindow::initTable(Table *w, const QString &caption) {
  QString name = caption;

  while (name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Table"));

  connectTable(w);
  customTable(w);

  w->setName(name);
  if (!isOfType(w, "MantidTable"))
    w->setWindowIcon(getQPixmap("worksheet_xpm"));

  addMdiSubWindow(w);
}

/*
 * !creates a new table with type statistics on target columns/rows of table
 * base
 */
TableStatistics *ApplicationWindow::newTableStatistics(Table *base, int type,
                                                       QList<int> target,
                                                       const QString &caption) {
  TableStatistics *s = new TableStatistics(scriptingEnv(), this, base,
                                           (TableStatistics::Type)type, target);
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
Note *ApplicationWindow::newNote(const QString &caption) {
  Note *m = new Note("", this);

  QString name = caption;
  while (name.isEmpty() || alreadyUsedName(name))
    name = generateUniqueName(tr("Notes"));

  m->setName(name);
  m->confirmClose(confirmCloseNotes);

  addMdiSubWindow(m);
  m->showNormal();
  return m;
}

Matrix *ApplicationWindow::newMatrix(int rows, int columns) {
  Matrix *m = new Matrix(scriptingEnv(), rows, columns, "", this, nullptr);
  initMatrix(m, generateUniqueName(tr("Matrix")));
  m->showNormal();
  return m;
}

Matrix *ApplicationWindow::newMatrix(const QString &caption, int r, int c) {
  Matrix *w = new Matrix(scriptingEnv(), r, c, "", this, nullptr);
  initMatrix(w, caption);
  if (w->objectName() != caption) // the matrix was renamed
    renamedTables << caption << w->objectName();

  w->showNormal();
  return w;
}

void ApplicationWindow::viewMatrixImage() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetViewCommand(
      m, m->viewType(), Matrix::ImageView, tr("Set Image Mode")));
  m->setViewType(Matrix::ImageView);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixTable() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetViewCommand(
      m, m->viewType(), Matrix::TableView, tr("Set Data Mode")));
  m->setViewType(Matrix::TableView);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixXY() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetHeaderViewCommand(
      m, m->headerViewType(), Matrix::XY, tr("Show X/Y")));
  m->setHeaderViewType(Matrix::XY);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixColumnRow() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetHeaderViewCommand(
      m, m->headerViewType(), Matrix::ColumnRow, tr("Show Column/Row")));
  m->setHeaderViewType(Matrix::ColumnRow);
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixGrayScale() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetColorMapCommand(
      m, m->colorMapType(), m->colorMap(), Matrix::GrayScale,
      QwtLinearColorMap(), tr("Set Gray Scale Palette")));
  m->setGrayScale();
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixRainbowScale() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m->undoStack()->push(new MatrixSetColorMapCommand(
      m, m->colorMapType(), m->colorMap(), Matrix::Rainbow, QwtLinearColorMap(),
      tr("Set Rainbow Palette")));
  m->setRainbowColorMap();
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::showColorMapDialog() {
  Matrix *m = static_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  ColorMapDialog *cmd = new ColorMapDialog(this);
  cmd->setAttribute(Qt::WA_DeleteOnClose);
  cmd->setMatrix(m);
  cmd->exec();
}

void ApplicationWindow::transposeMatrix() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->transpose();
}

void ApplicationWindow::flipMatrixVertically() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->flipVertically();
}

void ApplicationWindow::flipMatrixHorizontally() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->flipHorizontally();
}

void ApplicationWindow::rotateMatrix90() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->rotate90();
}

void ApplicationWindow::rotateMatrixMinus90() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->rotate90(false);
}

void ApplicationWindow::matrixDeterminant() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  QDateTime dt = QDateTime::currentDateTime();
  QString info = dt.toString(Qt::LocalDate);
  info += "\n" + tr("Determinant of ") + QString(m->objectName()) + ":\t";
  info += "det = " + QString::number(m->determinant()) + "\n";
  info += "-------------------------------------------------------------\n";

  currentFolder()->appendLogInfo(info);

  showResults(true);
}

void ApplicationWindow::invertMatrix() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->invert();
}

Table *ApplicationWindow::convertMatrixToTableDirect() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return nullptr;

  return matrixToTable(m, Direct);
}

Table *ApplicationWindow::convertMatrixToTableXYZ() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return nullptr;

  return matrixToTable(m, XYZ);
}

Table *ApplicationWindow::convertMatrixToTableYXZ() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return nullptr;

  return matrixToTable(m, YXZ);
}

Table *
ApplicationWindow::matrixToTable(Matrix *m,
                                 MatrixToTableConversion conversionType) {
  if (!m)
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int rows = m->numRows();
  int cols = m->numCols();
  MatrixModel *mModel = m->matrixModel();

  Table *w = nullptr;
  if (conversionType == Direct) {
    w = new Table(scriptingEnv(), rows, cols, "", this, nullptr);
    for (int i = 0; i < rows; i++) {
      for (int j = 0; j < cols; j++)
        w->setCell(i, j, m->cell(i, j));
    }
  } else if (conversionType == XYZ) {
    int tableRows = rows * cols;
    w = new Table(scriptingEnv(), tableRows, 3, "", this, nullptr);
    for (int i = 0; i < rows; i++) {
      for (int j = 0; j < cols; j++) {
        int cell = i * cols + j;
        w->setCell(cell, 0, mModel->x(j));
        w->setCell(cell, 1, mModel->y(i));
        w->setCell(cell, 2, mModel->cell(i, j));
      }
    }
  } else if (conversionType == YXZ) {
    int tableRows = rows * cols;
    w = new Table(scriptingEnv(), tableRows, 3, "", this, nullptr);
    for (int i = 0; i < cols; i++) {
      for (int j = 0; j < rows; j++) {
        int cell = i * rows + j;
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

void ApplicationWindow::initMatrix(Matrix *m, const QString &caption) {
  QString name = caption;
  while (alreadyUsedName(name)) {
    name = generateUniqueName(tr("Matrix"));
  }

  m->setWindowTitle(name);
  m->setName(name);
  m->confirmClose(confirmCloseMatrix);
  m->setNumericPrecision(d_decimal_digits);

  addMdiSubWindow(m);

  connect(m, SIGNAL(modifiedWindow(MdiSubWindow *)), this,
          SLOT(updateMatrixPlots(MdiSubWindow *)));

  emit modified();
}

Matrix *ApplicationWindow::convertTableToMatrix() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return nullptr;

  return tableToMatrix(t);
}

/**
 * Convert Table in the active window to a TableWorkspace
 */
void ApplicationWindow::convertTableToWorkspace() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  convertTableToTableWorkspace(t);
}

/**
 * Convert Table in the active window to a MatrixWorkspace
 */
void ApplicationWindow::convertTableToMatrixWorkspace() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  // dynamic_cast is successful when converting MantidTable to MatrixWorkspace
  auto *mt = dynamic_cast<MantidTable *>(t);

  if (!mt) {
    // if dynamic_cast is unsuccessful, create MantidTable from which to create
    // MatrixWorkspace
    mt = convertTableToTableWorkspace(t);
  }

  if (mt) {
    QHash<QString, QString> params;
    params["InputWorkspace"] = QString::fromStdString(mt->getWorkspaceName());
    mantidUI->showAlgorithmDialog(QString("ConvertTableToMatrixWorkspace"),
                                  params);
  }
}

/**
 * Convert a Table to a TableWorkspace. Columns with plot designations
 * X,Y,Z,xErr,yErr
 * are transformed to doubles, others - to strings.
 * @param t :: The Table to convert.
 */
MantidTable *ApplicationWindow::convertTableToTableWorkspace(Table *t) {
  if (!t)
    return nullptr;
  std::vector<int> format(t->numCols(), -1);
  std::vector<int> precision(t->numCols(), -1);
  Mantid::API::ITableWorkspace_sptr tws =
      Mantid::API::WorkspaceFactory::Instance().createTable();
  for (int col = 0; col < t->numCols(); ++col) {
    Table::PlotDesignation des =
        (Table::PlotDesignation)t->colPlotDesignation(col);
    QString name = t->colLabel(col);
    std::string type;
    int plotType = 6; // Label
    switch (des) {
    case Table::X: {
      plotType = 1;
      type = "double";
      break;
    }
    case Table::Y: {
      plotType = 2;
      type = "double";
      break;
    }
    case Table::Z: {
      plotType = 3;
      type = "double";
      break;
    }
    case Table::xErr: {
      plotType = 4;
      type = "double";
      break;
    }
    case Table::yErr: {
      plotType = 5;
      type = "double";
      break;
    }
    default:
      type = "string";
      plotType = 6;
    }

    if (plotType < 6) {
      // temporarily convert numeric columns to format that doesn't use commas
      // in numbers
      t->columnNumericFormat(col, &format[col], &precision[col]);
      t->setColNumericFormat(2, precision[col], col);
    }
    std::string columnName = name.toStdString();
    tws->addColumn(type, columnName);
    Mantid::API::Column_sptr column = tws->getColumn(columnName);
    column->setPlotType(plotType);
  }
  // copy data from table to workspace
  tws->setRowCount(t->numRows());
  for (int col = 0; col < t->numCols(); ++col) {
    Mantid::API::Column_sptr column = tws->getColumn(col);
    for (int row = 0; row < t->numRows(); ++row) {
      column->read(row, t->text(row, col).toStdString());
    }
  }
  // restore original format of numeric columns
  for (int col = 0; col < t->numCols(); ++col) {
    if (format[col] >= 0) {
      t->setColNumericFormat(format[col], precision[col], col);
    }
  }
  std::string wsName = t->objectName().toStdString();
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
    if (QMessageBox::question(this, "MantidPlot",
                              "Workspace with name " + t->objectName() +
                                  " already exists\n"
                                  "Do you want to overwrite it?",
                              QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName, tws);
    } else {
      return nullptr;
    }
  } else {
    Mantid::API::AnalysisDataService::Instance().add(wsName, tws);
  }
  return new MantidTable(scriptingEnv(), tws, t->objectName(), this);
}

Matrix *ApplicationWindow::tableToMatrix(Table *t) {
  if (!t)
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int rows = t->numRows();
  int cols = t->numCols();

  QString caption = generateUniqueName(tr("Matrix"));
  Matrix *m = new Matrix(scriptingEnv(), rows, cols, "", this, nullptr);
  initMatrix(m, caption);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++)
      m->setCell(i, j, t->cell(i, j));
  }

  m->setWindowLabel(m->windowLabel());
  m->setCaptionPolicy(m->captionPolicy());
  m->resize(m->size());
  m->showNormal();

  QApplication::restoreOverrideCursor();
  return m;
}

MdiSubWindow *ApplicationWindow::window(const QString &name) {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (w->objectName() == name)
      return w;
  }
  return nullptr;
}

Table *ApplicationWindow::table(const QString &name) {
  int pos = name.indexOf("_");
  QString caption = name.left(pos);

  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->inherits("Table") && w->objectName() == caption)
        return dynamic_cast<Table *>(w);
    }
    f = f->folderBelow();
  }

  return nullptr;
}

Matrix *ApplicationWindow::matrix(const QString &name) {
  QString caption = name;
  if (!renamedTables.isEmpty() && renamedTables.contains(caption)) {
    int index = renamedTables.indexOf(caption);
    caption = renamedTables[index + 1];
  }

  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (isOfType(w, "Matrix") && w->objectName() == caption)
        return dynamic_cast<Matrix *>(w);
    }
    f = f->folderBelow();
  }
  return nullptr;
}

MdiSubWindow *ApplicationWindow::activeWindow(WindowType type) {
  MdiSubWindow *active = getActiveWindow();
  if (!active)
    return nullptr;

  switch (type) {
  case NoWindow:
    break;

  case TableWindow:
    if (active->inherits("Table"))
      return active;
    else
      return nullptr;
    break;

  case MatrixWindow:
    if (active->inherits("Matrix")) // Mantid
      return active;
    else
      return nullptr;
    break;

  case MultiLayerWindow:
    if (isOfType(active, "MultiLayer"))
      return active;
    else
      return nullptr;
    break;

  case NoteWindow:
    if (isOfType(active, "Note"))
      return active;
    else
      return nullptr;
    break;

  case Plot3DWindow:
    if (isOfType(active, "Graph3D"))
      return active;
    else
      return nullptr;
    break;
  }
  return active;
}

void ApplicationWindow::windowActivated(QMdiSubWindow *w) {
  if (!w)
    return;

  MdiSubWindow *qti_subwin = qobject_cast<MdiSubWindow *>(w->widget());
  if (!qti_subwin)
    return;

  activateWindow(qti_subwin);
}

void ApplicationWindow::addErrorBars() {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  MultiLayer *plot = dynamic_cast<MultiLayer *>(w);
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g)
    return;

  if (!g->curves()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("There are no curves available on this plot!")); // Mantid
    return;
  }

  if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("This functionality is not available for pie plots!")); // Mantid
    return;
  }

  ErrDialog *ed = new ErrDialog(this);
  ed->setAttribute(Qt::WA_DeleteOnClose);
  connect(
      ed, SIGNAL(options(const QString &, int, const QString &, int, bool)),
      this,
      SLOT(defineErrorBars(const QString &, int, const QString &, int, bool)));
  connect(ed, SIGNAL(options(const QString &, const QString &, int)), this,
          SLOT(defineErrorBars(const QString &, const QString &, int)));

  ed->setCurveNames(g->analysableCurvesList());
  ed->setSrcTables(tableList());
  ed->exec();
}

void ApplicationWindow::removeErrorBars() {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  MultiLayer *plot = dynamic_cast<MultiLayer *>(w);
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g)
    return;

  if (!g->curves()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("There are no curves available on this plot!")); // Mantid
    return;
  }

  if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("This functionality is not available for pie plots!")); // Mantid
    return;
  }

  RemoveErrorsDialog *ed = new RemoveErrorsDialog(this);
  connect(ed, SIGNAL(curveName(const QString &)), this,
          SLOT(removeErrorBars(const QString &)));

  ed->setCurveNames(g->analysableCurvesList());
  ed->exec();
}

void ApplicationWindow::removeErrorBars(const QString &name) {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return;

  Graph *g = ml->activeGraph();
  if (!g)
    return;

  g->removeMantidErrorBars(name);
}

void ApplicationWindow::defineErrorBars(const QString &name, int type,
                                        const QString &percent, int direction,
                                        bool drawAll) {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return;

  Graph *g = ml->activeGraph();
  if (!g)
    return;

  if (type == 2) // A MantidCurve - do all the work in the Graph method
  {
    g->addMantidErrorBars(name, drawAll);
    return;
  }

  Table *t = table(name);
  if (!t) { // user defined function
    QMessageBox::critical(
        this, tr("MantidPlot - Error bars error"), // Mantid
        tr("This feature is not available for user defined function curves!"));
    return;
  }

  DataCurve *master_curve = dynamic_cast<DataCurve *>(g->curve(name));
  if (!master_curve)
    return;

  QString xColName = master_curve->xColumnName();
  if (xColName.isEmpty())
    return;

  if (direction == QwtErrorPlotCurve::Horizontal)
    t->addCol(Table::xErr);
  else
    t->addCol(Table::yErr);

  int r = t->numRows();
  int c = t->numCols() - 1;
  int ycol = t->colIndex(name);
  if (!direction)
    ycol = t->colIndex(xColName);

  QVarLengthArray<double> Y(t->col(ycol));
  QString errColName = t->colName(c);

  double prc = percent.toDouble();
  if (type == 0) {
    for (int i = 0; i < r; i++) {
      if (!t->text(i, ycol).isEmpty())
        t->setText(i, c, QString::number(Y[i] * prc / 100.0, 'g', 15));
    }
  } else if (type == 1) {
    int i;
    double dev = 0.0;
    double moyenne = 0.0;
    for (i = 0; i < r; i++)
      moyenne += Y[i];
    moyenne /= r;
    for (i = 0; i < r; i++)
      dev += (Y[i] - moyenne) * (Y[i] - moyenne);
    dev = sqrt(dev / (r - 1));
    for (i = 0; i < r; i++) {
      if (!t->table()->item(i, ycol)->text().isEmpty())
        t->setText(i, c, QString::number(dev, 'g', 15));
    }
  }
  QwtErrorPlotCurve *errs =
      g->addErrorBars(xColName, name, t, errColName, direction);
  if (errs) {
    // Error bars should be the same color as the curve line
    errs->setColor(master_curve->pen().color());
    g->updatePlot();
  }
}

void ApplicationWindow::defineErrorBars(const QString &curveName,
                                        const QString &errColumnName,
                                        int direction) {
  Table *w = table(curveName);
  if (!w) { // user defined function --> no worksheet available
    QMessageBox::critical(
        this, tr("MantidPlot - Error"), // Mantid
        tr("This feature is not available for user defined function curves!"));
    return;
  }

  Table *errTable = table(errColumnName);
  if (w->numRows() != errTable->numRows()) {
    QMessageBox::critical(
        this, tr("MantidPlot - Error"), // Mantid
        tr("The selected columns have different numbers of rows!"));

    addErrorBars();
    return;
  }

  int errCol = errTable->colIndex(errColumnName);
  if (errTable->isEmptyColumn(errCol)) {
    QMessageBox::critical(this, tr("MantidPlot - Error"), // Mantid
                          tr("The selected error column is empty!"));
    addErrorBars();
    return;
  }

  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  QwtErrorPlotCurve *errs =
      g->addErrorBars(curveName, errTable, errColumnName, direction);
  if (errs) {
    QwtPlotCurve *master_curve = g->curve(curveName);
    if (master_curve)
      errs->setColor(master_curve->pen().color());
    g->updatePlot();
  }
  emit modified();
}

void ApplicationWindow::removeCurves(const QString &name) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "MultiLayer")) {
      auto ml = dynamic_cast<MultiLayer *>(w);
      if (!ml)
        return;
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers)
        g->removeCurves(name);
    } else if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(w);
      if (g3d && g3d->formula().contains(name))
        g3d->clearData();
    }
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateCurves(Table *t, const QString &name) {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "MultiLayer")) {
      MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
      if (ml) {
        QList<Graph *> layers = ml->layersList();
        foreach (Graph *g, layers)
          g->updateCurvesData(t, name);
      }
    } else if (isOfType(w, "Graph3D")) {
      Graph3D *g = dynamic_cast<Graph3D *>(w);
      if (g && (g->formula()).contains(name))
        g->updateData(t);
    }
  }
}

void ApplicationWindow::showPreferencesDialog() {
  ConfigDialog *cd = new ConfigDialog(this);
  cd->setAttribute(Qt::WA_DeleteOnClose);
  cd->setColumnSeparator(columnSeparator);
  cd->exec();
}

void ApplicationWindow::setSaveSettings(bool autoSaving, int min) {
  if (autoSave == autoSaving && autoSaveTime == min)
    return;

  autoSave = autoSaving;
  autoSaveTime = min;

  killTimer(savingTimerId);

  if (autoSave)
    savingTimerId = startTimer(autoSaveTime * 60000);
  else
    savingTimerId = 0;
}

void ApplicationWindow::changeAppStyle(const QString &s) {
  // style keys are case insensitive
  if (appStyle.toLower() == s.toLower())
    return;

  qApp->setStyle(s);
  appStyle = qApp->style()->objectName();

  QPalette pal = qApp->palette();
  pal.setColor(QPalette::Active, QPalette::Base, QColor(panelsColor));
  qApp->setPalette(pal);
}

void ApplicationWindow::changeAppFont(const QFont &f) {
  if (appFont == f)
    return;

  appFont = f;
  updateAppFonts();
}

void ApplicationWindow::updateAppFonts() {
  qApp->setFont(appFont);
  this->setFont(appFont);
  info->setFont(
      QFont(appFont.family(), 2 + appFont.pointSize(), QFont::Bold, false));
}

void ApplicationWindow::updateConfirmOptions(bool askTables, bool askMatrices,
                                             bool askPlots2D, bool askPlots3D,
                                             bool askNotes,
                                             bool askInstrWindow) {
  QList<MdiSubWindow *> windows = windowsList();

  if (confirmCloseTable != askTables) {
    confirmCloseTable = askTables;
    foreach (MdiSubWindow *w, windows) {

      if (w->inherits("Table")) {
        w->confirmClose(confirmCloseTable);
      }
    }
  }

  if (confirmCloseMatrix != askMatrices) {
    confirmCloseMatrix = askMatrices;
    foreach (MdiSubWindow *w, windows) {
      if (isOfType(w, "Matrix")) {
        w->confirmClose(confirmCloseMatrix);
      }
    }
  }

  if (confirmClosePlot2D != askPlots2D) {
    confirmClosePlot2D = askPlots2D;
    foreach (MdiSubWindow *w, windows) {
      if (isOfType(w, "MultiLayer")) {
        w->confirmClose(confirmClosePlot2D);
      }
    }
  }

  if (confirmClosePlot3D != askPlots3D) {
    confirmClosePlot3D = askPlots3D;
    foreach (MdiSubWindow *w, windows) {
      if (isOfType(w, "Graph3D"))
        w->confirmClose(confirmClosePlot3D);
    }
  }

  if (confirmCloseNotes != askNotes) {
    confirmCloseNotes = askNotes;
    foreach (MdiSubWindow *w, windows) {
      if (isOfType(w, "Note"))
        w->confirmClose(confirmCloseNotes);
    }
  }

  if (confirmCloseInstrWindow != askInstrWindow) {
    confirmCloseInstrWindow = askInstrWindow;

    foreach (MdiSubWindow *w, windows) {
      if (isOfType(w, "InstrumentWindow")) {
        w->confirmClose(confirmCloseInstrWindow);
      }
    }
  }
}

void ApplicationWindow::setGraphDefaultSettings(bool autoscale, bool scaleFonts,
                                                bool resizeLayers,
                                                bool antialiasing,
                                                bool fixedAspectRatio) {
  if (autoscale2DPlots == autoscale && autoScaleFonts == scaleFonts &&
      autoResizeLayers != resizeLayers && antialiasing2DPlots == antialiasing &&
      fixedAspectRatio2DPlots == fixedAspectRatio)
    return;

  autoscale2DPlots = autoscale;
  autoScaleFonts = scaleFonts;
  autoResizeLayers = !resizeLayers;
  antialiasing2DPlots = antialiasing;
  fixedAspectRatio2DPlots = fixedAspectRatio;

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "MultiLayer")) {
      auto ml = dynamic_cast<MultiLayer *>(w);
      if (!ml)
        continue;
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers) {
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

void ApplicationWindow::setLegendDefaultSettings(int frame, const QFont &font,
                                                 const QColor &textCol,
                                                 const QColor &backgroundCol) {
  if (legendFrameStyle == frame && legendTextColor == textCol &&
      legendBackground == backgroundCol && plotLegendFont == font)
    return;

  legendFrameStyle = frame;
  legendTextColor = textCol;
  legendBackground = backgroundCol;
  plotLegendFont = font;
  saveSettings();
}

void ApplicationWindow::setArrowDefaultSettings(double lineWidth,
                                                const QColor &c,
                                                Qt::PenStyle style,
                                                int headLength, int headAngle,
                                                bool fillHead) {
  if (defaultArrowLineWidth == lineWidth && defaultArrowColor == c &&
      defaultArrowLineStyle == style && defaultArrowHeadLength == headLength &&
      defaultArrowHeadAngle == headAngle && defaultArrowHeadFill == fillHead)
    return;

  defaultArrowLineWidth = lineWidth;
  defaultArrowColor = c;
  defaultArrowLineStyle = style;
  defaultArrowHeadLength = headLength;
  defaultArrowHeadAngle = headAngle;
  defaultArrowHeadFill = fillHead;
  saveSettings();
}

ApplicationWindow *ApplicationWindow::plotFile(const QString &fn) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  ApplicationWindow *app = new ApplicationWindow();
  app->restoreApplicationGeometry();

  Table *t = app->newTable();
  if (!t)
    return nullptr;

  t->importASCII(fn, app->columnSeparator, 0, app->renameColumns,
                 app->strip_spaces, app->simplify_spaces,
                 app->d_ASCII_import_comments, app->d_ASCII_comment_string,
                 app->d_ASCII_import_read_only, Table::Overwrite, app->d_eol);
  t->setCaptionPolicy(MdiSubWindow::Both);
  app->multilayerPlot(t, t->YColumns(), GraphOptions::LineSymbols);
  QApplication::restoreOverrideCursor();
  return nullptr;
}

void ApplicationWindow::importASCII() {
  ImportASCIIDialog *import_dialog = new ImportASCIIDialog(
      !activeWindow(TableWindow) && !activeWindow(MatrixWindow), this,
      d_extended_import_ASCII_dialog);
  import_dialog->setDirectory(asciiDirPath);
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

  importASCII(import_dialog->selectedFiles(), import_dialog->importMode(),
              import_dialog->columnSeparator(), import_dialog->ignoredLines(),
              import_dialog->renameColumns(), import_dialog->stripSpaces(),
              import_dialog->simplifySpaces(), import_dialog->importComments(),
              import_dialog->updateDecimalSeparators(),
              import_dialog->decimalSeparators(),
              import_dialog->commentString(), import_dialog->readOnly(),
              import_dialog->endLineChar(),
              import_dialog->getselectedColumnSeparator());
}

void ApplicationWindow::importASCII(
    const QStringList &files, int import_mode,
    const QString &local_column_separator, int local_ignored_lines,
    bool local_rename_columns, bool local_strip_spaces,
    bool local_simplify_spaces, bool local_import_comments,
    bool update_dec_separators, QLocale local_separators,
    const QString &local_comment_string, bool import_read_only, int endLineChar,
    const QString &sepforloadAscii) {
  if (files.isEmpty())
    return;
  switch (import_mode) {
  case ImportASCIIDialog::NewTables: {
    int dx = 0, dy = 0;
    QStringList sorted_files = files;
    sorted_files.sort();
    int filesCount = sorted_files.size();
    for (int i = 0; i < filesCount; i++) {
      Table *w = newTable();
      if (!w)
        continue;

      w->importASCII(sorted_files[i], local_column_separator,
                     local_ignored_lines, local_rename_columns,
                     local_strip_spaces, local_simplify_spaces,
                     local_import_comments, local_comment_string,
                     import_read_only, Table::Overwrite, endLineChar);
      if (!w)
        continue;
      w->setWindowLabel(sorted_files[i]);
      w->setCaptionPolicy(MdiSubWindow::Both);
      if (i == 0) {
        dx = w->verticalHeaderWidth();
        dy = w->frameGeometry().height() - w->widget()->height();
      }
      if (filesCount > 1)
        w->move(QPoint(i * dx, i * dy));

      if (update_dec_separators)
        w->updateDecimalSeparators(local_separators);
    }
    modifiedProject();
    break;
  }
  case ImportASCIIDialog::NewMatrices: {
    int dx = 0, dy = 0;
    QStringList sorted_files = files;
    sorted_files.sort();
    int filesCount = sorted_files.size();
    for (int i = 0; i < filesCount; i++) {
      Matrix *w = newMatrix();
      if (!w)
        continue;
      w->importASCII(sorted_files[i], local_column_separator,
                     local_ignored_lines, local_strip_spaces,
                     local_simplify_spaces, local_comment_string,
                     Matrix::Overwrite, local_separators, endLineChar);
      w->setWindowLabel(sorted_files[i]);
      w->setCaptionPolicy(MdiSubWindow::Both);
      if (i == 0) {
        dx = w->verticalHeaderWidth();
        dy = w->frameGeometry().height() - w->widget()->height();
      }
      if (filesCount > 1)
        w->move(QPoint(i * dx, i * dy));
    }
    modifiedProject();
    break;
  }

  case ImportASCIIDialog::NewColumns:
  case ImportASCIIDialog::NewRows: {
    MdiSubWindow *w = activeWindow();
    if (!w)
      return;

    if (w->inherits("Table")) {
      Table *t = dynamic_cast<Table *>(w);
      if (t) {
        for (int i = 0; i < files.size(); i++)
          t->importASCII(files[i], local_column_separator, local_ignored_lines,
                         local_rename_columns, local_strip_spaces,
                         local_simplify_spaces, local_import_comments,
                         local_comment_string, import_read_only,
                         (Table::ImportMode)(import_mode - 2), endLineChar);

        if (update_dec_separators)
          t->updateDecimalSeparators(local_separators);
        t->notifyChanges();
        emit modifiedProject(t);
      }
    } else if (isOfType(w, "Matrix")) {
      Matrix *m = dynamic_cast<Matrix *>(w);
      if (m) {
        for (int i = 0; i < files.size(); i++) {
          m->importASCII(files[i], local_column_separator, local_ignored_lines,
                         local_strip_spaces, local_simplify_spaces,
                         local_comment_string,
                         (Matrix::ImportMode)(import_mode - 2),
                         local_separators, endLineChar);
        }
      }
    }
    w->setWindowLabel(files.join("; "));
    w->setCaptionPolicy(MdiSubWindow::Name);
    break;
  }
  case ImportASCIIDialog::Overwrite: {
    MdiSubWindow *w = activeWindow();
    if (!w)
      return;

    if (w->inherits("Table")) {
      Table *t = dynamic_cast<Table *>(w);
      if (!t)
        return;
      t->importASCII(files[0], local_column_separator, local_ignored_lines,
                     local_rename_columns, local_strip_spaces,
                     local_simplify_spaces, local_import_comments,
                     local_comment_string, import_read_only, Table::Overwrite,
                     endLineChar);
      if (update_dec_separators)
        t->updateDecimalSeparators(local_separators);
      t->notifyChanges();
    } else if (isOfType(w, "Matrix")) {
      Matrix *m = dynamic_cast<Matrix *>(w);
      if (!m)
        return;
      m->importASCII(files[0], local_column_separator, local_ignored_lines,
                     local_strip_spaces, local_simplify_spaces,
                     local_comment_string, Matrix::Overwrite, local_separators,
                     endLineChar);
    }

    w->setWindowLabel(files[0]);
    w->setCaptionPolicy(MdiSubWindow::Both);
    modifiedProject();
    break;
  }
  case ImportASCIIDialog::NewWorkspace: {
    try {
      Mantid::API::IAlgorithm_sptr alg = mantidUI->createAlgorithm("LoadAscii");
      QStringList sorted_files = files;
      sorted_files.sort();
      for (int i = 0; i < sorted_files.size(); i++) {
        QStringList ws = sorted_files[i].split(".", QString::SkipEmptyParts);
        QString temp = ws[0];
        int index = temp.lastIndexOf("\\");
        if (index == -1)
          return;
        QString wsName = temp.right(temp.size() - (index + 1));
        alg->setPropertyValue("Filename", sorted_files[i].toStdString());
        alg->setPropertyValue("OutputWorkspace", wsName.toStdString());
        alg->setPropertyValue("Separator", sepforloadAscii.toStdString());
        alg->execute();
      }

    } catch (...) {
      throw std::runtime_error(
          "LoadAscii failed when importing the file as workspace");
    }
    break;
  }
  }
}

void ApplicationWindow::open() {
  OpenProjectDialog *open_dialog =
      new OpenProjectDialog(this, d_extended_open_dialog);
  open_dialog->setDirectory(workingDir);
  if (open_dialog->exec() != QDialog::Accepted ||
      open_dialog->selectedFiles().isEmpty())
    return;
  workingDir = open_dialog->directory().path();

  switch (open_dialog->openMode()) {
  case OpenProjectDialog::NewProject: {
    QString fn = open_dialog->selectedFiles()[0];
    QFileInfo fi(fn);

    if (projectname != "untitled") {
      QFileInfo fi(projectname);
      QString pn = fi.absoluteFilePath();
      if (fn == pn) {
        QMessageBox::warning(
            this, tr("MantidPlot - File opening error"), // Mantid
            tr("The file: <b>%1</b> is the current file!").arg(fn));
        return;
      }
    }

    if (fn.endsWith(".qti", Qt::CaseInsensitive) ||
        fn.endsWith(".qti~", Qt::CaseInsensitive) ||
        fn.endsWith(".opj", Qt::CaseInsensitive) ||
        fn.endsWith(".ogm", Qt::CaseInsensitive) ||
        fn.endsWith(".ogw", Qt::CaseInsensitive) ||
        fn.endsWith(".ogg", Qt::CaseInsensitive) ||
        fn.endsWith(".qti.gz", Qt::CaseInsensitive) ||
        fn.endsWith(".mantid", Qt::CaseInsensitive) ||
        fn.endsWith(".mantid~", Qt::CaseInsensitive)) {
      if (!fi.exists()) {
        QMessageBox::critical(this,
                              tr("MantidPlot - File opening error"), // Mantid
                              tr("The file: <b>%1</b> doesn't exist!").arg(fn));
        return;
      }

      saveSettings(); // the recent projects must be saved

      ApplicationWindow *a = open(fn, false, false);
      if (a) {
        a->workingDir = workingDir;
        if (fn.endsWith(".qti", Qt::CaseInsensitive) ||
            fn.endsWith(".qti~", Qt::CaseInsensitive) ||
            fn.endsWith(".opj", Qt::CaseInsensitive) ||
            fn.endsWith(".ogg", Qt::CaseInsensitive) ||
            fn.endsWith(".qti.gz", Qt::CaseInsensitive)) { // this->close();
        }
      }
    } else {
      QMessageBox::critical(
          this, tr("MantidPlot - File opening error"), // Mantid
          tr("The file: <b>%1</b> is not a MantidPlot or Origin project file!")
              .arg(fn));
      return;
    }
    break;
  }
  case OpenProjectDialog::NewFolder:
    appendProject(open_dialog->selectedFiles()[0]);
    break;
  }
}

ApplicationWindow *ApplicationWindow::open(const QString &fn,
                                           bool factorySettings,
                                           bool newProject) {
  if (fn.endsWith(".opj", Qt::CaseInsensitive) ||
      fn.endsWith(".ogm", Qt::CaseInsensitive) ||
      fn.endsWith(".ogw", Qt::CaseInsensitive) ||
      fn.endsWith(".ogg", Qt::CaseInsensitive))
    return importOPJ(fn, factorySettings, newProject);
  else if (fn.endsWith(".py", Qt::CaseInsensitive))
    return loadScript(fn);
  else if (!(fn.endsWith(".qti", Qt::CaseInsensitive) ||
             fn.endsWith(".qti.gz", Qt::CaseInsensitive) ||
             fn.endsWith(".qti~", Qt::CaseInsensitive) ||
             fn.endsWith(".mantid", Qt::CaseInsensitive) ||
             fn.endsWith(".mantid~", Qt::CaseInsensitive))) {
    return plotFile(fn);
  }

  QString fname = fn;
  if (fn.endsWith(".qti.gz", Qt::CaseInsensitive) ||
      fn.endsWith(".mantid.gz", Qt::CaseInsensitive)) { // decompress using zlib
    file_uncompress(fname.toAscii().data());
    fname = fname.left(fname.size() - 3);
  }

  QFile f(fname);
  QTextStream t(&f);
  if (!f.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(
        this, tr("MantidPlot - File opening error"),
        tr("The file: <b> %1 </b> could not be opened!").arg(fn));
    return nullptr;
  }
  QString s = t.readLine();
  QStringList list = s.split(QRegExp("\\s"), QString::SkipEmptyParts);
  if (list.count() < 2 || list[0] != "MantidPlot") {
    f.close();
    if (QFile::exists(fname + "~")) {
      int choice = QMessageBox::question(
          this, tr("MantidPlot - File opening error"), // Mantid
          tr("The file <b>%1</b> is corrupted, but there exists a backup "
             "copy.<br>Do you want to open the backup instead?")
              .arg(fn),
          QMessageBox::Yes | QMessageBox::Default,
          QMessageBox::No | QMessageBox::Escape);
      if (choice == QMessageBox::Yes)
        return open(fname + "~");
      else
        QMessageBox::critical(
            this, tr("MantidPlot - File opening error"),
            tr("The file: <b> %1 </b> was not created using MantidPlot!")
                .arg(fn)); // Mantid
      return nullptr;
    }
  }

  QStringList vl = list[1].split(".", QString::SkipEmptyParts);
  const int fileVersion =
      100 * (vl[0]).toInt() + 10 * (vl[1]).toInt() + (vl[2]).toInt();
  ApplicationWindow *app = openProject(fname, fileVersion);
  f.close();
  return app;
}

void ApplicationWindow::openRecentFile(QAction *action) {
  auto fn = action->data().toString();
  // if "," found in the QString
  if (fn.indexOf(",", 0)) {
    try {
      int pos = fn.indexOf(" ", 0);
      fn = fn.right(fn.length() - pos - 1);
      loadDataFileByName(fn);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw;
    }
  } else {
    int pos = fn.indexOf(" ", 0);
    fn = fn.right(fn.length() - pos - 1);
    QFile f(fn);
    if (!f.exists()) {
      QMessageBox::critical(
          this, tr("MantidPlot - File Open Error"), // Mantid
          tr("The file: <b> %1 </b> <p>is not there anymore!"
             "<p>It will be removed from the list of recent files.")
              .arg(fn));

      recentFiles.removeAll(fn);
      updateRecentFilesList();
      return;
    }
    loadDataFileByName(fn);
  }
  saveSettings(); // save new list of recent files
}

void ApplicationWindow::openRecentProject(QAction *action) {
  QString fn = action->text();
  int pos = fn.indexOf(" ", 0);
  fn = fn.right(fn.length() - pos - 1);

  QFile f(fn);
  if (!f.exists()) {
    QMessageBox::critical(
        this, tr("MantidPlot - File Open Error"), // Mantid
        tr("The file: <b> %1 </b> <p>does not exist anymore!"
           "<p>It will be removed from the list of recent projects.")
            .arg(fn));

    recentProjects.removeAll(fn);
    updateRecentProjectsList();
    return;
  }

  if (projectname != "untitled") {
    QFileInfo fi(projectname);
    QString pn = fi.absoluteFilePath();
    if (fn == pn) {
      QMessageBox::warning(
          this, tr("MantidPlot - File open error"), // Mantid
          tr("The file: <p><b> %1 </b><p> is the current file!").arg(fn));
      return;
    }
  }

  if (!fn.isEmpty()) {
    saveSettings(); // the recent projects must be saved
    bool isSaved = saved;
    // Have to change the working directory here because that is used when
    // finding the nexus files to load
    workingDir = QFileInfo(f).absolutePath();
    cacheWorkingDirectory();

    ApplicationWindow *a = open(fn, false, false);
    if (a && (fn.endsWith(".qti", Qt::CaseInsensitive) ||
              fn.endsWith(".qti~", Qt::CaseInsensitive) ||
              fn.endsWith(".opj", Qt::CaseInsensitive) ||
              fn.endsWith(".ogg", Qt::CaseInsensitive)))
      if (isSaved)
        savedProject(); // force saved state
    // close();
  }
}

/**
 * Open project with the given working directory.
 *
 * This function allows a project to be opened without
 * using a GUI from the Python interface.
 *
 * @param workingDir :: the file directiory to use
 * @param filename :: the path of the project file to open
 * @param fileVersion :: the file version to use
 * @return updated application window handler
 */
ApplicationWindow *ApplicationWindow::openProject(const QString &workingDir,
                                                  const QString &filename,
                                                  const int fileVersion) {
  this->workingDir = workingDir;
  return openProject(filename, fileVersion);
}

ApplicationWindow *ApplicationWindow::openProject(const QString &filename,
                                                  const int fileVersion) {
  newProject();
  m_mantidmatrixWindows.clear();

  cacheWorkingDirectory();
  projectname = filename;
  setWindowTitle("MantidPlot - " + filename);

  d_opening_file = true;

  folders->blockSignals(true);
  blockSignals(true);

  // Open as a top level folder
  ProjectSerialiser serialiser(this);
  try {
    serialiser.load(filename.toStdString(), fileVersion);
  } catch (std::runtime_error &e) {
    g_log.error(e.what());
    // We failed to load - reset and bail out
    d_opening_file = false;
    folders->blockSignals(false);
    blockSignals(false);
    return this;
  }

  Folder *curFolder = projectFolder();

  // rename project folder item
  FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
  if (!item)
    throw std::runtime_error("Couldn't retrieve folder list items.");

  QFile file(filename);
  QFileInfo fileInfo(filename);
  QString baseName = fileInfo.fileName();
  item->setText(0, fileInfo.baseName());
  item->folder()->setObjectName(fileInfo.baseName());

  d_loaded_current = nullptr;

  if (d_loaded_current)
    curFolder = d_loaded_current;

  QString fileName = fileInfo.absoluteFilePath();
  recentProjects.removeAll(filename);
  recentProjects.push_front(filename);
  updateRecentProjectsList();

  folders->setCurrentItem(curFolder->folderListItem());
  folders->blockSignals(false);

  // change folder to user defined current folder
  changeFolder(curFolder, true);

  blockSignals(false);

  renamedTables.clear();

  restoreApplicationGeometry();

  savedProject();
  d_opening_file = false;
  d_workspace->blockSignals(false);

  return this;
}

bool ApplicationWindow::setScriptingLanguage(const QString &lang) {
  if (lang.isEmpty())
    return false;
  if (scriptingEnv() && lang == scriptingEnv()->objectName())
    return true;

  if (m_bad_script_envs.contains(lang)) {
    using MantidQt::MantidWidgets::Message;
    writeToLogWindow(
        Message("Previous initialization of " + lang + " failed, cannot retry.",
                Message::Priority::PRIO_ERROR));
    return false;
  }

  ScriptingEnv *newEnv(nullptr);
  if (m_script_envs.contains(lang)) {
    newEnv = m_script_envs.value(lang);
  } else {
    newEnv = ScriptingLangManager::newEnv(lang, this);
    connect(newEnv, SIGNAL(print(const QString &)), resultsLog,
            SLOT(appendNotice(const QString &)));

    if (newEnv->initialize()) {
      m_script_envs.insert(lang, newEnv);
    } else {
      delete newEnv;
      m_bad_script_envs.insert(lang);
      QMessageBox::information(this, "MantidPlot",
                               QString("Failed to initialize ") + lang +
                                   ". Please contact support.");
      return false;
    }
  }

  // notify everyone who might be interested
  ScriptingChangeEvent *sce = new ScriptingChangeEvent(newEnv);
  QApplication::sendEvent(this, sce);
  delete sce;

  foreach (QObject *i, findChildren<QObject *>())
    QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));

  if (scriptingWindow) {
    // Mantid - This is so that the title of the script window reflects the
    // current scripting language
    QApplication::postEvent(scriptingWindow, new ScriptingChangeEvent(newEnv));

    foreach (QObject *i, scriptingWindow->findChildren<QObject *>())
      QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));
  }

  return true;
}

void ApplicationWindow::showScriptingLangDialog() {
  // If a script is currently active, don't let a new one be selected
  if (scriptingWindow->isExecuting()) {
    QMessageBox msg_box;
    msg_box.setText(
        "Cannot change scripting language, a script is still running.");
    msg_box.exec();
    return;
  }
  ScriptingLangDialog *d = new ScriptingLangDialog(scriptingEnv(), this);
  d->exec();
}

void ApplicationWindow::readSettings() {
#ifdef Q_OS_MAC // Mac
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     QCoreApplication::organizationName(),
                     QCoreApplication::applicationName());
#else
  QSettings settings;
#endif

  /* ---------------- group General --------------- */
  settings.beginGroup("/General");
  settings.beginGroup("/ApplicationGeometry"); // main window geometry
  d_app_rect =
      QRect(settings.value("/x", 0).toInt(), settings.value("/y", 0).toInt(),
            settings.value("/width", 0).toInt(),
            settings.value("/height", 0).toInt());
  settings.endGroup();

  autoSearchUpdates = settings.value("/AutoSearchUpdates", false).toBool();
  appLanguage =
      settings.value("/Language", QLocale::system().name().section('_', 0, 0))
          .toString();
  show_windows_policy =
      (ShowWindowsPolicy)settings
          .value("/ShowWindowsPolicy", ApplicationWindow::ActiveFolder)
          .toInt();

  recentProjects = settings.value("/RecentProjects").toStringList();
  recentFiles = settings.value("/RecentFiles").toStringList();
// Follows an ugly hack added by Ion in order to fix Qt4 porting issues
//(only needed on Windows due to a Qt bug?)
#ifdef Q_OS_WIN
  if (!recentProjects.isEmpty() && recentProjects[0].contains("^e"))
    recentProjects = recentProjects[0].split("^e", QString::SkipEmptyParts);
  else if (recentProjects.count() == 1) {
    QString s = recentProjects[0];
    if (s.remove(QRegExp("\\s")).isEmpty())
      recentProjects = QStringList();
  }

  if (!recentFiles.isEmpty() && recentFiles[0].contains("^e"))
    recentFiles = recentFiles[0].split("^e", QString::SkipEmptyParts);
  else if (recentFiles.count() == 1) {
    QString s = recentFiles[0];
    if (s.remove(QRegExp("\\s")).isEmpty())
      recentFiles = QStringList();
  }
#endif

  updateRecentProjectsList();
  updateRecentFilesList();

  changeAppStyle(settings.value("/Style", appStyle).toString());
  autoSave = settings.value("/AutoSave", false).toBool();
  autoSaveTime = settings.value("/AutoSaveTime", 15).toInt();
  d_backup_files = settings.value("/BackupProjects", true).toBool();
  d_init_window_type =
      (WindowType)settings.value("/InitWindow", NoWindow).toInt();
  defaultScriptingLang =
      settings.value("/ScriptingLang", "Python").toString(); // Mantid M. Gigg
  d_thousands_sep = settings.value("/ThousandsSeparator", true).toBool();
  d_locale =
      QLocale(settings.value("/Locale", QLocale::system().name()).toString());
  if (!d_thousands_sep)
    d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

  d_decimal_digits = settings.value("/DecimalDigits", 13).toInt();
  d_matrix_undo_stack_size = settings.value("/MatrixUndoStackSize", 10).toInt();
  d_eol = (EndLineChar)settings.value("/EndOfLine", d_eol).toInt();

  // restore dock windows and tool bars
  restoreState(settings.value("/DockWindows").toByteArray());
  explorerSplitter->restoreState(
      settings.value("/ExplorerSplitter").toByteArray());
  QList<int> lst = explorerSplitter->sizes();
  for (int i = 0; i < lst.count(); i++) {
    if (lst[i] == 0) {
      lst[i] = 45;
      explorerSplitter->setSizes(lst);
    }
  }

  QStringList applicationFont = settings.value("/Font").toStringList();
  if (applicationFont.size() == 4)
    appFont = QFont(applicationFont[0], applicationFont[1].toInt(),
                    applicationFont[2].toInt(), applicationFont[3].toInt());

  settings.beginGroup("/Dialogs");
  d_extended_open_dialog = settings.value("/ExtendedOpenDialog", true).toBool();
  d_extended_export_dialog =
      settings.value("/ExtendedExportDialog", true).toBool();
  d_extended_import_ASCII_dialog =
      settings.value("/ExtendedImportAsciiDialog", true).toBool();
  d_extended_plot_dialog = settings.value("/ExtendedPlotDialog", true)
                               .toBool(); // used by PlotDialog

  settings.beginGroup("/AddRemoveCurves");
  d_add_curves_dialog_size = QSize(settings.value("/Width", 700).toInt(),
                                   settings.value("/Height", 400).toInt());
  d_show_current_folder = settings.value("/ShowCurrentFolder", false).toBool();
  settings.endGroup(); // AddRemoveCurves Dialog
  settings.endGroup(); // Dialogs

  settings.beginGroup("/Colors");
  workspaceColor =
      QColor(settings.value("/Workspace", "darkGray").value<QColor>());
  // see http://doc.trolltech.com/4.2/qvariant.html for instructions on qcolor
  // <-> qvariant conversion
  panelsColor = QColor(settings.value("/Panels", "#ffffff").value<QColor>());
  panelsTextColor =
      QColor(settings.value("/PanelsText", "#000000").value<QColor>());
  settings.endGroup(); // Colors

  settings.beginGroup("/Paths");
  QString appPath = qApp->applicationDirPath();
  workingDir = settings.value("/WorkingDir", appPath).toString();
#ifdef Q_OS_WIN
  fitPluginsPath = settings.value("/FitPlugins", "fitPlugins").toString();
  templatesDir = settings.value("/TemplatesDir", appPath).toString();
  asciiDirPath = settings.value("/ASCII", appPath).toString();
  imagesDirPath = settings.value("/images", appPath).toString();
#else
  fitPluginsPath =
      settings.value("/FitPlugins", "/usr/lib/MantidPlot/plugins").toString();
  templatesDir = settings.value("/TemplatesDir", QDir::homePath()).toString();
  asciiDirPath = settings.value("/ASCII", QDir::homePath()).toString();
  imagesDirPath = settings.value("/images", QDir::homePath()).toString();
  workingDir = settings.value("/WorkingDir", QDir::homePath()).toString();
#endif
  scriptsDirPath = settings.value("/ScriptsDir", appPath).toString();
  fitModelsPath = settings.value("/FitModelsDir", "").toString();
  customActionsDirPath = settings.value("/CustomActionsDir", "").toString();
  helpFilePath = settings.value("/HelpFile", helpFilePath).toString();
  d_translations_folder =
      settings.value("/Translations", d_translations_folder).toString();
  d_python_config_folder =
      settings.value("/PythonConfigDir", d_python_config_folder).toString();
  settings.endGroup(); // Paths
  settings.endGroup();
  /* ------------- end group General ------------------- */

  settings.beginGroup("/UserFunctions");
  surfaceFunc = settings.value("/SurfaceFunctions").toStringList();
  xFunctions = settings.value("/xFunctions").toStringList();
  yFunctions = settings.value("/yFunctions").toStringList();
  rFunctions = settings.value("/rFunctions").toStringList();
  thetaFunctions = settings.value("/thetaFunctions").toStringList();
  d_param_surface_func = settings.value("/ParametricSurfaces").toStringList();
  settings.endGroup(); // UserFunctions

  settings.beginGroup("/Confirmations");
  // Once only for each Qsettings instance set all of the confirmations to false
  // - they are annoying
  // however if people consciously turn them back on then leave them alone.
  // leaving renameTable out of this as it is bit different
  bool setConfirmationDefaultsToFalseOnce =
      settings.value("/DefaultsSetToFalseOnce", false).toBool();
  if (!setConfirmationDefaultsToFalseOnce) {
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
  confirmCloseInstrWindow = settings.value("/InstrumentWindow", false).toBool();
  settings.endGroup(); // Confirmations

  /* ---------------- group Tables --------------- */
  settings.beginGroup("/Tables");
  d_show_table_comments = settings.value("/DisplayComments", false).toBool();
  d_auto_update_table_values =
      settings.value("/AutoUpdateValues", true).toBool();

  QStringList tableFonts = settings.value("/Fonts").toStringList();
  if (tableFonts.size() == 8) {
    tableTextFont = QFont(tableFonts[0], tableFonts[1].toInt(),
                          tableFonts[2].toInt(), tableFonts[3].toInt());
    tableHeaderFont = QFont(tableFonts[4], tableFonts[5].toInt(),
                            tableFonts[6].toInt(), tableFonts[7].toInt());
  }

  settings.beginGroup("/Colors");
  tableBkgdColor =
      QColor(settings.value("/Background", "#ffffff").value<QColor>());
  tableTextColor = QColor(settings.value("/Text", "#000000").value<QColor>());
  tableHeaderColor =
      QColor(settings.value("/Header", "#000000").value<QColor>());
  settings.endGroup(); // Colors
  settings.endGroup();
  /* --------------- end group Tables ------------------------ */

  /* --------------- group 2D Plots ----------------------------- */

  settings.beginGroup("/2DPlots");

  // Transform from the old setting for plot defaults, will only happen once.
  if (!settings.contains("/UpdateForPlotImprovements1")) {
    settings.setValue("/UpdateForPlotImprovements1", "true");
    settings.beginGroup("/General");

    settings.setValue("/Antialiasing", "true");

    // enable right and top axes without labels
    settings.beginWriteArray("EnabledAxes");
    int i = 1;
    settings.setArrayIndex(i);
    settings.setValue("enabled", "true");
    settings.setValue("labels", "false");
    i = 3;
    settings.setArrayIndex(i);
    settings.setValue("enabled", "true");
    settings.setValue("labels", "false");
    settings.endArray();
    settings.endGroup();

    // ticks should be in
    settings.beginGroup("/Ticks");
    settings.setValue("/MajTicksStyle", ScaleDraw::In);
    settings.setValue("/MinTicksStyle", ScaleDraw::In);
    settings.endGroup();

    // legend to opaque
    settings.beginGroup("/Legend");
    settings.setValue("/Transparency", 255);
    settings.endGroup(); // Legend
  }
  // Transform from the old setting for plot defaults, will only happen once.
  if (!settings.contains("/UpdateForPlotImprovements2")) {
    settings.setValue("/UpdateForPlotImprovements2", "true");
    settings.beginGroup("/General");

    // turn axes backbones off as these rarely join at the corners
    settings.setValue("/AxesBackbones", "false");

    settings.setValue("/CanvasFrameWidth", "1");
    settings.endGroup();
  }

  settings.beginGroup("/General");
  titleOn = settings.value("/Title", true).toBool();
  // The setting for this was originally stored as a QSetting but then was
  // migrated to
  // the Mantid ConfigService and is now saved by the ConfigDialog
  auto &cfgSvc = ConfigService::Instance();
  if (settings.contains("/AutoDistribution1D")) {
    // if the setting was false then the user changed it
    // sync this to the new location and remove the key for the future
    bool qsettingsFlag = settings.value("/AutoDistribution1D", true).toBool();
    if (!qsettingsFlag) {
      cfgSvc.setString("graph1d.autodistribution", "Off");
      try {
        cfgSvc.saveConfig(cfgSvc.getUserFilename());
      } catch (std::runtime_error &) {
        g_log.warning("Unable to update autodistribution property from "
                      "ApplicationWindow");
      }
    }
    settings.remove("/AutoDistribution1D");
  }
  // Pull default from config service
  autoDistribution1D =
      cfgSvc.getValue<bool>("graph1d.autodistribution").get_value_or(false);

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

  antialiasing2DPlots =
      settings.value("/Antialiasing", false).toBool(); // Mantid
  fixedAspectRatio2DPlots =
      settings.value("/FixedAspectRatio2DPlots", false).toBool(); // Mantid
  d_scale_plots_on_print =
      settings.value("/ScaleLayersOnPrint", false).toBool();
  d_print_cropmarks = settings.value("/PrintCropmarks", false).toBool();

  QStringList graphFonts = settings.value("/Fonts").toStringList();
  if (graphFonts.size() == 16) {
    plotAxesFont = QFont(graphFonts[0], graphFonts[1].toInt(),
                         graphFonts[2].toInt(), graphFonts[3].toInt());
    plotNumbersFont = QFont(graphFonts[4], graphFonts[5].toInt(),
                            graphFonts[6].toInt(), graphFonts[7].toInt());
    plotLegendFont = QFont(graphFonts[8], graphFonts[9].toInt(),
                           graphFonts[10].toInt(), graphFonts[11].toInt());
    plotTitleFont = QFont(graphFonts[12], graphFonts[13].toInt(),
                          graphFonts[14].toInt(), graphFonts[15].toInt());
  }
  d_in_place_editing = settings.value("/InPlaceEditing", true).toBool();
  d_graph_axes_labels_dist =
      settings.value("/LabelsAxesDist", d_graph_axes_labels_dist).toInt();
  d_graph_tick_labels_dist =
      settings.value("/TickLabelsDist", d_graph_tick_labels_dist).toInt();
  // Transform from the old setting for controlling visible axes. Will only
  // happen once, after which it's deleted.
  if (settings.contains("/AllAxes")) {
    if (settings.value("/AllAxes").toBool()) {
      d_show_axes = QVector<bool>(QwtPlot::axisCnt, true);
    }
    settings.remove("/AllAxes");
  } else {
    int size = settings.beginReadArray("EnabledAxes");
    for (int i = 0; i < size; ++i) {
      settings.setArrayIndex(i);
      d_show_axes[i] = settings.value("enabled", true).toBool();
      d_show_axes_labels[i] = settings.value("labels", true).toBool();
    }
    settings.endArray();
  }
  d_synchronize_graph_scales =
      settings.value("/SynchronizeScales", d_synchronize_graph_scales).toBool();
  settings.endGroup(); // General

  settings.beginGroup("/Curves");
  defaultCurveStyle =
      settings.value("/Style", GraphOptions::LineSymbols).toInt();
  defaultCurveLineWidth = settings.value("/LineWidth", 1).toDouble();
  defaultSymbolSize = settings.value("/SymbolSize", 3).toInt();
  applyCurveStyleToMantid = settings.value("/ApplyMantid", true).toBool();
  // Once only for DrawAllErrors set to true, by SSC request
  bool setDrawAllErrorsSetToTrueOnce =
      settings.value("/DrawAllErrorsSetToTrueOnce", false).toBool();
  if (!setDrawAllErrorsSetToTrueOnce) {
    settings.setValue("/DrawAllErrors", true);
    settings.setValue("/DrawAllErrorsSetToTrueOnce", true);
  }
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
  legendTextColor = QColor(settings.value("/TextColor", "#000000")
                               .value<QColor>()); // default color Qt::black
  legendBackground = QColor(settings.value("/BackgroundColor", "#ffffff")
                                .value<QColor>()); // default color Qt::white
  legendBackground.setAlpha(
      settings.value("/Transparency", 0).toInt()); // transparent by default;
  settings.endGroup();                             // Legend

  settings.beginGroup("/Arrows");
  defaultArrowLineWidth = settings.value("/Width", 1).toDouble();
  defaultArrowColor = QColor(settings.value("/Color", "#000000")
                                 .value<QColor>()); // default color Qt::black
  defaultArrowHeadLength = settings.value("/HeadLength", 4).toInt();
  defaultArrowHeadAngle = settings.value("/HeadAngle", 45).toInt();
  defaultArrowHeadFill = settings.value("/HeadFill", true).toBool();
  defaultArrowLineStyle =
      Graph::getPenStyle(settings.value("/LineStyle", "SolidLine").toString());
  settings.endGroup(); // Arrows
  settings.endGroup();
  /* ----------------- end group 2D Plots --------------------------- */

  /* ----------------- group 3D Plots --------------------------- */
  settings.beginGroup("/3DPlots");
  showPlot3DLegend = settings.value("/Legend", true).toBool();
  showPlot3DProjection = settings.value("/Projection", false).toBool();
  smooth3DMesh = settings.value("/Antialiasing", false).toBool(); // Mantid
  plot3DResolution = settings.value("/Resolution", 1).toInt();
  orthogonal3DPlots = settings.value("/Orthogonal", false).toBool();
  autoscale3DPlots = settings.value("/Autoscale", true).toBool();

  QStringList plot3DFonts = settings.value("/Fonts").toStringList();
  if (plot3DFonts.size() == 12) {
    plot3DTitleFont = QFont(plot3DFonts[0], plot3DFonts[1].toInt(),
                            plot3DFonts[2].toInt(), plot3DFonts[3].toInt());
    plot3DNumbersFont = QFont(plot3DFonts[4], plot3DFonts[5].toInt(),
                              plot3DFonts[6].toInt(), plot3DFonts[7].toInt());
    plot3DAxesFont = QFont(plot3DFonts[8], plot3DFonts[9].toInt(),
                           plot3DFonts[10].toInt(), plot3DFonts[11].toInt());
  }

  settings.beginGroup("/Colors");
  plot3DColors = {
      QColor(settings.value("/MaxData", "blue").value<QColor>()).name(),
      QColor(settings.value("/Labels", "#000000").value<QColor>()).name(),
      QColor(settings.value("/Mesh", "#000000").value<QColor>()).name(),
      QColor(settings.value("/Grid", "#000000").value<QColor>()).name(),
      QColor(settings.value("/MinData", "red").value<QColor>()).name(),
      QColor(settings.value("/Numbers", "#000000").value<QColor>()).name(),
      QColor(settings.value("/Axes", "#000000").value<QColor>()).name(),
      QColor(settings.value("/Background", "#ffffff").value<QColor>()).name()};
  settings.endGroup(); // Colors
  settings.endGroup();
  /* ----------------- end group 3D Plots --------------------------- */

  settings.beginGroup("/Fitting");
  m_enableQtiPlotFitting =
      settings.value("/EnableQtiPlotFitting", false).toBool();
  fit_output_precision = settings.value("/OutputPrecision", 15).toInt();
  pasteFitResultsToPlot = settings.value("/PasteResultsToPlot", false).toBool();
  writeFitResultsToLog = settings.value("/WriteResultsToLog", true).toBool();
  generateUniformFitPoints = settings.value("/GenerateFunction", true).toBool();
  fitPoints = settings.value("/Points", 100).toInt();
  generatePeakCurves = settings.value("/GeneratePeakCurves", true).toBool();
  peakCurvesColor = settings.value("/PeaksColor", 2).toInt(); // green color
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
  d_ASCII_import_locale =
      settings.value("/AsciiImportLocale", QLocale::system().name()).toString();
  d_import_dec_separators =
      settings.value("/UpdateDecSeparators", true).toBool();
  d_ASCII_import_mode =
      settings.value("/ImportMode", ImportASCIIDialog::NewTables).toInt();
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
  d_image_export_filter =
      settings.value("/ImageFileTypeFilter", ".png").toString();
  d_export_transparency = settings.value("/ExportTransparency", false).toBool();
  d_export_quality = settings.value("/ImageQuality", 100).toInt();
  d_export_color = settings.value("/ExportColor", true).toBool();
  d_export_vector_size =
      settings.value("/ExportPageSize", QPrinter::Custom).toInt();
  d_keep_plot_aspect = settings.value("/KeepAspect", true).toBool();
  settings.endGroup(); // ExportImage

  settings.beginGroup("/ScriptWindow");
  d_script_win_pos = settings.value("/pos", QPoint(250, 200)).toPoint();
  if (d_script_win_pos.x() < 0 || d_script_win_pos.y() < 0)
    d_script_win_pos = QPoint(250, 200);
  d_script_win_size = settings.value("/size", QSize(600, 660)).toSize();
  if (!d_script_win_size.isValid())
    d_script_win_size = QSize(600, 660);
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

  bool warning_shown =
      settings.value("/DuplicationDialogShown", false).toBool();

  // Check for user defined scripts in settings and create menus for them
  // Top level scripts group
  settings.beginGroup("CustomScripts");

  MantidQt::API::InterfaceManager interfaceManager;

  // Reference list of custom Interfaces that will be added to the Interfaces
  // menu
  QStringList user_windows = interfaceManager.getUserSubWindowKeys();
  // List it user items that will be moved to the Interfaces menu
  QStringList duplicated_custom_menu = QStringList();

  foreach (QString menu, settings.childGroups()) {
    // Specifically disallow the use of the Interfaces menu to users looking to
    // customise their own menus, since it is managed separately.  Also, there
    // may well be some left-over QSettings values from previous installations
    // that we do not want used.
    if (menu == "Interfaces" || menu == "&Interfaces")
      continue;

    addUserMenu(menu);
    settings.beginGroup(menu);
    foreach (QString keyName, settings.childKeys()) {
      QFileInfo fi(settings.value(keyName).toString());
      QString baseName = fi.fileName();
      const QStringList pyQtInterfaces = m_interfaceCategories.keys();
      if (pyQtInterfaces.contains(baseName))
        continue;

      if (user_windows.filter(keyName).size() > 0 ||
          pyQtInterfaces.filter(keyName).size() > 0) {
        duplicated_custom_menu.append(menu + "/" + keyName);
      }
      if (QFileInfo(settings.value(keyName).toString()).exists())
        addUserMenuAction(menu, keyName, settings.value(keyName).toString());
    }
    settings.endGroup();
  }

  // Mantid - Remember which interfaces the user explicitly removed
  // from the Interfaces menu
  removed_interfaces = settings.value("RemovedInterfaces").toStringList();

  settings.endGroup();

  if (duplicated_custom_menu.size() > 0 && !warning_shown) {
    QString mess =
        "The following menus are now part of the Interfaces menu:\n\n";
    mess += duplicated_custom_menu.join("\n");
    mess += "\n\nYou may consider removing them from your custom menus.";
    // FIXME: A nice alternative to showing a message in the log window would
    // be to pop up a message box. This should be done AFTER MantidPlot has
    // started.
    // QMessageBox::warning(this, tr("MantidPlot - Menu Warning"),
    // tr(mess.ascii()));
    g_log.warning() << tr(mess.toAscii()).toStdString() << "\n";
    settings.setValue("/DuplicationDialogShown", true);
  }

  // Mantid Muon interface one time only change
  settings.beginGroup("/CustomInterfaces");
  settings.beginGroup("/MuonAnalysis");
  if (!settings.contains("/UpdateForPlotPolicy1")) {
    settings.setValue("/UpdateForPlotPolicy1", "true");
    settings.beginGroup("/GeneralOptions");
    if (settings.value("/newPlotPolicy", 0).toInt() == 0) {
      settings.setValue("/newPlotPolicy", 1);
      settings.setValue("/fitsToKeep", 0);
    }
    settings.endGroup();
  }
  settings.endGroup();
  settings.endGroup();
  // END Mantid Muon interface one time only change

  emit configModified();
}

void ApplicationWindow::saveSettings() {

#ifdef Q_OS_MAC // Mac
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     QCoreApplication::organizationName(),
                     QCoreApplication::applicationName());
#else
  QSettings settings; //(QSettings::NativeFormat,QSettings::UserScope,
                      //"ProIndependent", "MantidPlot");
#endif

  // Root level is named "General" by Qt
  resultsLog->writeSettings(settings);

  // Our named group General, displayed as %General in the file
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
  applicationFont << appFont.family();
  applicationFont << QString::number(appFont.pointSize());
  applicationFont << QString::number(appFont.weight());
  applicationFont << QString::number(appFont.italic());
  settings.setValue("/Font", applicationFont);

  settings.beginGroup("/Dialogs");
  settings.setValue("/ExtendedOpenDialog", d_extended_open_dialog);
  settings.setValue("/ExtendedExportDialog", d_extended_export_dialog);
  settings.setValue("/ExtendedImportAsciiDialog",
                    d_extended_import_ASCII_dialog);
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
  settings.setValue("/images", imagesDirPath);
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
  settings.setValue("/DeleteWorkspace", d_inform_delete_workspace);
  settings.setValue("/RenameTable", d_inform_rename_table);
  settings.value("/InstrumentWindow", confirmCloseInstrWindow).toBool();
  settings.endGroup(); // Confirmations

  /* ----------------- group Tables -------------- */
  settings.beginGroup("/Tables");
  settings.setValue("/DisplayComments", d_show_table_comments);
  settings.setValue("/AutoUpdateValues", d_auto_update_table_values);
  QStringList tableFonts;
  tableFonts << tableTextFont.family();
  tableFonts << QString::number(tableTextFont.pointSize());
  tableFonts << QString::number(tableTextFont.weight());
  tableFonts << QString::number(tableTextFont.italic());
  tableFonts << tableHeaderFont.family();
  tableFonts << QString::number(tableHeaderFont.pointSize());
  tableFonts << QString::number(tableHeaderFont.weight());
  tableFonts << QString::number(tableHeaderFont.italic());
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
  graphFonts << plotAxesFont.family();
  graphFonts << QString::number(plotAxesFont.pointSize());
  graphFonts << QString::number(plotAxesFont.weight());
  graphFonts << QString::number(plotAxesFont.italic());
  graphFonts << plotNumbersFont.family();
  graphFonts << QString::number(plotNumbersFont.pointSize());
  graphFonts << QString::number(plotNumbersFont.weight());
  graphFonts << QString::number(plotNumbersFont.italic());
  graphFonts << plotLegendFont.family();
  graphFonts << QString::number(plotLegendFont.pointSize());
  graphFonts << QString::number(plotLegendFont.weight());
  graphFonts << QString::number(plotLegendFont.italic());
  graphFonts << plotTitleFont.family();
  graphFonts << QString::number(plotTitleFont.pointSize());
  graphFonts << QString::number(plotTitleFont.weight());
  graphFonts << QString::number(plotTitleFont.italic());
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
  settings.setValue("/MajTicksStyle", majTicksStyle);
  settings.setValue("/MinTicksStyle", minTicksStyle);
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
  plot3DFonts << plot3DTitleFont.family();
  plot3DFonts << QString::number(plot3DTitleFont.pointSize());
  plot3DFonts << QString::number(plot3DTitleFont.weight());
  plot3DFonts << QString::number(plot3DTitleFont.italic());
  plot3DFonts << plot3DNumbersFont.family();
  plot3DFonts << QString::number(plot3DNumbersFont.pointSize());
  plot3DFonts << QString::number(plot3DNumbersFont.weight());
  plot3DFonts << QString::number(plot3DNumbersFont.italic());
  plot3DFonts << plot3DAxesFont.family();
  plot3DFonts << QString::number(plot3DAxesFont.pointSize());
  plot3DFonts << QString::number(plot3DAxesFont.weight());
  plot3DFonts << QString::number(plot3DAxesFont.italic());
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
  settings.setValue("/ColumnSeparator",
                    sep.replace("\t", "\\t").replace(" ", "\\s"));
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

  // Save mantid settings
  mantidUI->saveSettings();

  //--------------------------------------
  // Mantid - Save custom scripts
  settings.beginGroup("CustomScripts");
  settings.remove("");
  foreach (QMenu *menu, d_user_menus) {
    settings.beginGroup(menu->title());
    foreach (QAction *action, menu->actions()) {
      settings.setValue(action->text(), action->data().toString());
    }
    settings.endGroup();
  }

  // Mantid - Remember which interfaces the user explicitly removed
  // from the Interfaces menu
  settings.setValue("RemovedInterfaces", removed_interfaces);

  settings.endGroup();
  //-----------------------------------
}

void ApplicationWindow::exportGraph() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  MultiLayer *plot2D = nullptr;
  Graph3D *plot3D = nullptr;
  if (isOfType(w, "MultiLayer")) {
    plot2D = dynamic_cast<MultiLayer *>(w);
    if (!plot2D)
      return;
    if (plot2D->isEmpty()) {
      QMessageBox::critical(
          this, tr("MantidPlot - Export Error"), // Mantid
          tr("<h4>There are no plot layers available in this window!</h4>"));
      return;
    }
  } else if (std::string(w->metaObject()->className()) == "Graph3D")
    plot3D = dynamic_cast<Graph3D *>(w);
  else
    return;

  ImageExportDialog *ied =
      new ImageExportDialog(this, plot2D != nullptr, d_extended_export_dialog);
  ied->setDirectory(workingDir);
  ied->selectFilter(d_image_export_filter);
  if (ied->exec() != QDialog::Accepted)
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
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(
        this, tr("MantidPlot - Export error"), // Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that "
           "you have the right to write to this location!")
            .arg(file_name));
    return;
  }
  file.close();

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") ||
      selected_filter.contains(".ps") || selected_filter.contains(".svg")) {
    if (plot3D)
      plot3D->exportVector(file_name);
    else if (plot2D) {
      if (selected_filter.contains(".svg"))
        plot2D->exportSVG(file_name);
      else
        plot2D->exportVector(file_name, ied->resolution(), ied->color(),
                             ied->keepAspect(), ied->pageSize());
    }
  } else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i = 0; i < (int)list.count(); i++) {
      if (selected_filter.contains("." + (list[i]).toLower())) {
        if (plot2D)
          plot2D->exportImage(file_name, ied->quality(), ied->transparency());
        else if (plot3D)
          plot3D->exportImage(file_name, ied->quality(), ied->transparency());
      }
    }
  }
}

void ApplicationWindow::exportLayer() {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return;

  Graph *g = ml->activeGraph();
  if (!g)
    return;

  ImageExportDialog *ied =
      new ImageExportDialog(this, g != nullptr, d_extended_export_dialog);
  ied->setDirectory(workingDir);
  ied->selectFilter(d_image_export_filter);
  if (ied->exec() != QDialog::Accepted)
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
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(
        this, tr("MantidPlot - Export error"), // Mantid
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that "
           "you have the right to write to this location!")
            .arg(file_name));
    return;
  }
  file.close();

  if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") ||
      selected_filter.contains(".ps"))
    g->exportVector(file_name, ied->resolution(), ied->color(),
                    ied->keepAspect(), ied->pageSize());
  else if (selected_filter.contains(".svg"))
    g->exportSVG(file_name);
  else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for (int i = 0; i < (int)list.count(); i++)
      if (selected_filter.contains("." + (list[i]).toLower()))
        g->exportImage(file_name, ied->quality(), ied->transparency());
  }
}

void ApplicationWindow::exportAllGraphs() {
  ImageExportDialog *ied =
      new ImageExportDialog(this, true, d_extended_export_dialog);
  ied->setWindowTitle(tr("Choose a directory to export the graphs to"));
  QStringList tmp = ied->filters();
  ied->setFileMode(QFileDialog::Directory);
  ied->setFilters(tmp);
  ied->setLabelText(QFileDialog::FileType, tr("Output format:"));
  ied->setLabelText(QFileDialog::FileName, tr("Directory:"));

  ied->setDirectory(workingDir);
  ied->selectFilter(d_image_export_filter);

  if (ied->exec() != QDialog::Accepted)
    return;
  workingDir = ied->directory().path();
  if (ied->selectedFiles().isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString output_dir = ied->selectedFiles()[0];
  QString file_suffix = ied->selectedFilter();
  file_suffix = file_suffix.toLower();
  file_suffix.remove("*");

  bool confirm_overwrite = true;
  MultiLayer *plot2D;
  Graph3D *plot3D;

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    const std::string windowClassName = w->metaObject()->className();
    if (windowClassName == "MultiLayer") {
      plot3D = nullptr;
      plot2D = dynamic_cast<MultiLayer *>(w);
      if (!plot2D)
        continue;
      if (plot2D->isEmpty()) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(
            this, tr("MantidPlot - Warning"), // Mantid
            tr("There are no plot layers available in window <b>%1</b>.<br>"
               "Graph window not exported!")
                .arg(plot2D->objectName()));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        continue;
      }
    } else if (windowClassName == "Graph3D") {
      plot2D = nullptr;
      plot3D = dynamic_cast<Graph3D *>(w);
      if (!plot3D)
        continue;
    } else
      continue;

    QString file_name = output_dir + "/" + w->objectName() + file_suffix;
    QFile f(file_name);
    if (f.exists() && confirm_overwrite) {
      QApplication::restoreOverrideCursor();

      QString msg = tr("A file called: <p><b>%1</b><p>already exists. "
                       "Do you want to overwrite it?")
                        .arg(file_name);
      QMessageBox msgBox(QMessageBox::Question,
                         tr("MantidPlot - Overwrite file?"), msg, // Mantid
                         QMessageBox::Yes | QMessageBox::YesToAll |
                             QMessageBox::No | QMessageBox::Cancel,
                         this);
      msgBox.exec();
      switch (msgBox.standardButton(msgBox.clickedButton())) {
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
    if (!f.open(QIODevice::WriteOnly)) {
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(
          this, tr("MantidPlot - Export error"), // Mantid
          tr("Could not write to file: <br><h4>%1</h4><p>"
             "Please verify that you have the right to write to this location!")
              .arg(file_name));
      return;
    }
    f.close();

    if (file_suffix.contains(".eps") || file_suffix.contains(".pdf") ||
        file_suffix.contains(".ps") || file_suffix.contains(".svg")) {
      if (plot3D)
        plot3D->exportVector(file_name);
      else if (plot2D) {
        if (file_suffix.contains(".svg"))
          plot2D->exportSVG(file_name);
        else
          plot2D->exportVector(file_name, ied->resolution(), ied->color(),
                               ied->keepAspect(), ied->pageSize());
      }
    } else {
      QList<QByteArray> list = QImageWriter::supportedImageFormats();
      for (int i = 0; i < (int)list.count(); i++) {
        if (file_suffix.contains("." + (list[i]).toLower())) {
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

std::string ApplicationWindow::windowGeometryInfo(MdiSubWindow *w) {
  TSVSerialiser tsv;
  tsv.writeLine("geometry");
  if (w->status() == MdiSubWindow::Maximized) {
    tsv << "maximized";

    if (w == activeWindow())
      tsv << "active";

    return tsv.outputLines();
  }

  int x = w->x();
  int y = w->y();

  QWidget *wrapper = w->getWrapperWindow();
  if (wrapper) {
    x = wrapper->x();
    y = wrapper->y();
  }

  tsv << x << y;
  tsv << w->width() << w->height();

  if (w->status() == MdiSubWindow::Minimized)
    tsv << "minimized";
  else if (hidden(w))
    tsv << "hidden";
  else if (w == activeWindow())
    tsv << "active";

  return tsv.outputLines();
}

void ApplicationWindow::restoreWindowGeometry(ApplicationWindow *app,
                                              MdiSubWindow *w,
                                              const QString &s) {
  if (!w)
    return;

  QString caption = w->objectName();

  if (s.contains("maximized")) {
    w->setMaximized();
    app->setListView(caption, tr("Maximized"));
  } else {
    QStringList lst = s.split("\t");
    if (lst.count() > 4) {
      int x = lst[1].toInt();
      int y = lst[2].toInt();
      int width = lst[3].toInt();
      int height = lst[4].toInt();

      QWidget *wrapper = w->getWrapperWindow();
      if (wrapper) {
        wrapper->resize(width, height);
        wrapper->move(x, y);
      } else {
        w->resize(width, height);
        w->move(x, y);
      }
    }

    if (s.contains("minimized")) {
      w->setMinimized();
      app->setListView(caption, tr("Minimized"));
    } else {
      w->setNormal();
      if (lst.count() > 5 && lst[5] == "hidden")
        app->hideWindow(w);
    }
  }
  if (s.contains("active"))
    setActiveWindow(w);
}

Folder *ApplicationWindow::projectFolder() const {
  auto fli = dynamic_cast<FolderListItem *>(folders->firstChild());
  if (fli)
    return fli->folder();
  else
    throw std::runtime_error("Couldn't retrieve project folder");
}

bool ApplicationWindow::saveProject(bool compress) {
  if (projectname == "untitled" ||
      projectname.endsWith(".opj", Qt::CaseInsensitive) ||
      projectname.endsWith(".ogm", Qt::CaseInsensitive) ||
      projectname.endsWith(".ogw", Qt::CaseInsensitive) ||
      projectname.endsWith(".ogg", Qt::CaseInsensitive)) {
    saveProjectAs();
    return true;
  }

  ProjectSerialiser serialiser(this);
  serialiser.save(projectname, compress);

  setWindowTitle("MantidPlot - " + projectname);
  savedProject();

  if (autoSave) {
    if (savingTimerId)
      killTimer(savingTimerId);
    savingTimerId = startTimer(autoSaveTime * 60000);
  } else
    savingTimerId = 0;

  // Back-up file to be removed because file has successfully saved.
  QFile::remove(projectname + "~");

  QApplication::restoreOverrideCursor();
  return true;
}

int ApplicationWindow::execSaveProjectDialog() {
  std::vector<IProjectSerialisable *> windows;

  for (auto window : getSerialisableWindows()) {
    auto win = dynamic_cast<IProjectSerialisable *>(window);
    if (win)
      windows.push_back(win);
  }

  for (auto window : getAllWindows()) {
    auto win = dynamic_cast<IProjectSerialisable *>(window);
    if (win)
      windows.push_back(win);
  }

  const QString pyInterfaceMarkerProperty("launcher");
  std::vector<std::string> activePythonInterfaces;
  auto serialisablePythonInterfaces =
      ProjectSerialiser::serialisablePythonInterfaces();
  auto activeWidgets = QApplication::allWidgets();
  for (auto widget : activeWidgets) {
    QVariant launcherScript =
        widget->property(pyInterfaceMarkerProperty.toLatin1().data());
    if (launcherScript.isValid()) {
      auto launcherScriptName = launcherScript.toString();
      if (serialisablePythonInterfaces.contains(launcherScriptName)) {
        activePythonInterfaces.emplace_back(launcherScriptName.toStdString());
      } else {
        g_log.warning()
            << "Widget contains property "
            << pyInterfaceMarkerProperty.toStdString() << " with value "
            << launcherScriptName.toStdString()
            << " but this is not an interface we know how to save.\n";
      }
    }
  }

  auto serialiser = new ProjectSerialiser(this, currentFolder());
  m_projectSaveView = new MantidQt::MantidWidgets::ProjectSaveView(
      projectname, *serialiser, windows, activePythonInterfaces, this);
  connect(m_projectSaveView, SIGNAL(projectSaved()), this,
          SLOT(postSaveProject()));
  return m_projectSaveView->exec();
}

void ApplicationWindow::prepareSaveProject() { execSaveProjectDialog(); }

/**
 * The project was just saved. Update the main window.
 */
void ApplicationWindow::postSaveProject() {
  setWindowTitle("MantidPlot - " + projectname);

  if (autoSave) {
    if (savingTimerId)
      killTimer(savingTimerId);
    savingTimerId = startTimer(autoSaveTime * 60000);
  } else
    savingTimerId = 0;

  // Back-up file to be removed because file has successfully saved.
  QFile::remove(projectname + "~");

  QApplication::restoreOverrideCursor();

  recentProjects.removeAll(projectname);
  recentProjects.push_front(projectname);
  updateRecentProjectsList();

  QFileInfo fi(projectname);
  QString baseName = fi.baseName();
  FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
  if (item) {
    item->setText(0, baseName);
    item->folder()->setObjectName(baseName);
  }

  savedProject();
}

void ApplicationWindow::savetoNexusFile() {
  QString filter = tr("Mantid Files") + " (*.nxs *.nx5 *.xml);;";
  QString selectedFilter;
  QString fileDir =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save File As"), fileDir, filter, &selectedFilter);
  if (!fileName.isEmpty()) {
    std::string wsName;
    MdiSubWindow *w = activeWindow();
    if (w) {
      const std::string windowClassName = w->metaObject()->className();
      if (windowClassName == "MantidMatrix") {
        wsName = dynamic_cast<MantidMatrix *>(w)->getWorkspaceName();

      } else if (windowClassName == "MantidTable") {
        wsName = dynamic_cast<MantidTable *>(w)->getWorkspaceName();
      } else {
        throw std::runtime_error("Invalid input for SaveNexus, you cannot save "
                                 "this type of object as a NeXus file");
      }
    } else {
      wsName = m_nexusInputWSName.toStdString();
    }
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
      throw std::runtime_error("Invalid input workspace for SaveNexus");
    }

    savedatainNexusFormat(wsName, fileName.toStdString());
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        QFileInfo(fileName).absoluteDir().path());
    updateRecentFilesList(fileName);
  }
}

void ApplicationWindow::loadDataFile() {
  // Ask user for file
  QString fn = QFileDialog::getOpenFileName(
      nullptr, tr("Mantidplot - Open file to load"),
      AlgorithmInputHistory::Instance().getPreviousDirectory());
  if (fn != "") {
    loadDataFileByName(fn);
  }
  saveSettings(); // save new list of recent files
}

void ApplicationWindow::loadDataFileByName(QString fn) {
  QFileInfo fnInfo(fn);
  AlgorithmInputHistory::Instance().setPreviousDirectory(
      fnInfo.absoluteDir().path());
  if (fnInfo.suffix() == "py") {
    // We have a python file, just load it into script window
    loadScript(fn, true);
  } else if (fnInfo.suffix() == "mantid") {
    // We have a mantid project file, pass on to project loading
    open(fn);
  } else if (mantidUI) {
    // Run Load algorithm on file
    QHash<QString, QString> params;
    params["Filename"] = fn;
    mantidUI->showAlgorithmDialog(QString("Load"), params);
  }
}

void ApplicationWindow::saveProjectAs(const QString &fileName, bool compress) {
  QString fn = fileName;
  if (fileName.isEmpty()) {
    QString filter = tr("MantidPlot project") + " (*.mantid);;";
    filter += tr("Compressed MantidPlot project") + " (*.mantid.gz)";

    QString selectedFilter;
    fn = QFileDialog::getSaveFileName(this, tr("Save Project As"), workingDir,
                                      filter, &selectedFilter);
    if (selectedFilter.contains(".gz"))
      compress = true;
  }

  if (!fn.isEmpty()) {
    QFileInfo fileInfo(fn);
    bool isFile = fileInfo.fileName().endsWith(".mantid") ||
                  fileInfo.fileName().endsWith(".mantid.gz");

    if (!isFile) {
      QDir directory(fn);
      if (!directory.exists()) {
        // Make the directory
        directory.mkdir(fn);
      }

      workingDir = directory.absolutePath();
      cacheWorkingDirectory();
      QString projectFileName = directory.dirName();
      projectFileName.append(".mantid");
      projectname = directory.absoluteFilePath(projectFileName);

    } else {
      workingDir = fileInfo.absoluteDir().absolutePath();
      projectname = fileInfo.absoluteFilePath();
    }

    if (saveProject(compress)) {
      recentProjects.removeAll(projectname);
      recentProjects.push_front(projectname);
      updateRecentProjectsList();

      QFileInfo fi(fn);
      QString baseName = fi.baseName();
      FolderListItem *item =
          dynamic_cast<FolderListItem *>(folders->firstChild());
      if (item) {
        item->setText(0, baseName);
        item->folder()->setObjectName(baseName);
      }
    }
  }
}

void ApplicationWindow::saveNoteAs() {
  Note *w = dynamic_cast<Note *>(activeWindow(NoteWindow));
  if (!w)
    return;
  w->exportASCII();
}

void ApplicationWindow::rename() {
  MdiSubWindow *m = activeWindow();
  if (!m)
    return;

  RenameWindowDialog *rwd = new RenameWindowDialog(this);
  rwd->setAttribute(Qt::WA_DeleteOnClose);
  rwd->setWidget(m);
  rwd->exec();
}

void ApplicationWindow::renameWindow() {
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
  if (!it)
    return;

  MdiSubWindow *w = it->window();
  if (!w)
    return;

  RenameWindowDialog *rwd = new RenameWindowDialog(this);
  rwd->setAttribute(Qt::WA_DeleteOnClose);
  rwd->setWidget(w);
  rwd->exec();
}

void ApplicationWindow::renameWindow(QTreeWidgetItem *item, int /*unused*/,
                                     const QString &text) {
  if (!item)
    return;

  WindowListItem *wli = dynamic_cast<WindowListItem *>(item);
  if (!wli)
    return;

  MdiSubWindow *w = wli->window();
  if (!w)
    return;

  if (text == w->objectName())
    return;

  if (!setWindowName(w, text))
    item->setText(0, w->objectName());
}

bool ApplicationWindow::setWindowName(MdiSubWindow *w, const QString &text) {

  if (!w)
    return false;

  QString name = w->objectName();
  if (name == text)
    return true;

  QString newName = text;
  newName.replace("-", "_");
  if (newName.isEmpty()) {
    QMessageBox::critical(this, tr("MantidPlot - Error"),
                          tr("Please enter a valid name!")); // Mantid
    return false;
  } else if (newName.contains(QRegExp("\\W"))) {
    QMessageBox::critical(this, tr("MantidPlot - Error"), // Mantid
                          tr("The name you chose is not valid: only letters "
                             "and digits are allowed!") +
                              "<p>" + tr("Please choose another name!"));
    return false;
  }

  newName.replace("_", "-");

  while (alreadyUsedName(newName)) {
    QMessageBox::critical(
        this, tr("MantidPlot - Error"),
        tr("Name <b>%1</b> already exists!").arg(newName) + // Mantid
            "<p>" + tr("Please choose another name!") + "<p>" +
            tr("Warning: for internal consistency reasons the underscore "
               "character is replaced with a minus sign."));
    return false;
  }

  if (w->inherits("Table"))
    updateTableNames(name, newName);
  else if (std::string(w->metaObject()->className()) == "Matrix")
    changeMatrixName(name, newName);

  w->setCaptionPolicy(w->captionPolicy());
  w->setName(newName);
  renameListViewItem(name, newName);
  return true;
}

QStringList ApplicationWindow::columnsList(Table::PlotDesignation plotType) {
  QStringList list;
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (!w->inherits("Table"))
      continue;

    Table *t = dynamic_cast<Table *>(w);
    for (int i = 0; t && i < t->numCols(); i++) {
      if (t->colPlotDesignation(i) == plotType || plotType == Table::All)
        list << QString(t->objectName()) + "_" + t->colLabel(i);
    }
  }
  return list;
}

void ApplicationWindow::showCurvesDialog() {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return;

  if (ml->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Error"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = ml->activeGraph();
  if (!g)
    return;

  if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Error"), // Mantid
        tr("This functionality is not available for pie plots!"));
  } else {
    CurvesDialog *crvDialog = new CurvesDialog(this, g);
    crvDialog->setAttribute(Qt::WA_DeleteOnClose);
    crvDialog->resize(d_add_curves_dialog_size);
    crvDialog->setModal(true);
    crvDialog->show();
  }
}

bool ApplicationWindow::hasTable() {
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->inherits("Table"))
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

QStringList ApplicationWindow::tableNames() {
  QStringList lst = QStringList();
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->inherits("Table"))
        lst << w->objectName();
    }
    f = f->folderBelow();
  }
  return lst;
}

QList<MdiSubWindow *> ApplicationWindow::tableList() {
  QList<MdiSubWindow *> lst;
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->inherits("Table"))
        lst << w;
    }
    f = f->folderBelow();
  }
  return lst;
}

AssociationsDialog *ApplicationWindow::showPlotAssociations(int curve) {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return nullptr;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return nullptr;

  Graph *g = ml->activeGraph();
  if (!g)
    return nullptr;

  AssociationsDialog *ad = new AssociationsDialog(g);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->initTablesList(tableList(), curve);
  ad->show();
  return ad;
}

void ApplicationWindow::showTitleDialog() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;
  const std::string windowClassName = w->metaObject()->className();
  if (windowClassName == "MultiLayer") {
    auto ml = dynamic_cast<MultiLayer *>(w);
    if (!ml)
      return;

    Graph *g = ml->activeGraph();
    if (g) {
      TextDialog *td = new TextDialog(TextDialog::LayerTitle, this, nullptr);
      td->setGraph(g);
      td->exec();
    }
  } else if (windowClassName == "Graph3D") {
    Plot3DDialog *pd = dynamic_cast<Plot3DDialog *>(showPlot3dDialog());
    if (pd)
      pd->showTitleTab();
  }
}

void ApplicationWindow::showAxisTitleDialog() {
  MdiSubWindow *w = activeWindow(MultiLayerWindow);
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (!ml)
    return;

  Graph *g = ml->activeGraph();
  if (!g)
    return;

  TextDialog *td = new TextDialog(TextDialog::AxisTitle, this, nullptr);
  td->setGraph(g);
  td->exec();
}

void ApplicationWindow::showExportASCIIDialog() {
  QString tableName = QString::null;
  MdiSubWindow *t = activeWindow();
  if (t) {
    const std::string tClassName = t->metaObject()->className();
    if (tClassName == "Matrix" || t->inherits("Table") ||
        tClassName == "MantidMatrix") {
      tableName = t->objectName();

      ExportDialog *ed =
          new ExportDialog(tableName, this, Qt::WindowContextHelpButtonHint);
      ed->setAttribute(Qt::WA_DeleteOnClose);
      ed->setColumnSeparator(columnSeparator);
      ed->exec();
    }
  }
}

void ApplicationWindow::exportAllTables(const QString &sep, bool colNames,
                                        bool colComments, bool expSelection) {
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Choose a directory to export the tables to"), workingDir,
      QFileDialog::ShowDirsOnly);
  if (!dir.isEmpty()) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    workingDir = dir;

    bool confirmOverwrite = true;
    bool success = true;
    QList<MdiSubWindow *> windows = windowsList();
    foreach (MdiSubWindow *w, windows) {
      if (w->inherits("Table") ||
          std::string(w->metaObject()->className()) == "Matrix") {
        QString fileName = dir + "/" + w->objectName() + ".txt";
        QFile f(fileName);
        if (f.exists(fileName) && confirmOverwrite) {
          QApplication::restoreOverrideCursor();
          auto result = QMessageBox::question(
              this, tr("MantidPlot - Overwrite file?"),
              tr("A file called: <p><b>%1</b><p>already exists. "
                 "Do you want to overwrite it?")
                  .arg(fileName),
              tr("&Yes"), tr("&All"), tr("&Cancel"), 0, 1);

          if (result == 1)
            confirmOverwrite = false;
          else if (result == 2)
            return;
        }

        auto table = dynamic_cast<Table *>(w);
        auto matrix = dynamic_cast<Matrix *>(w);

        if (table)
          success = table->exportASCII(fileName, sep, colNames, colComments,
                                       expSelection);
        else if (matrix)
          success = matrix->exportASCII(fileName, sep, expSelection);

        if (!success)
          break;
      }
    }
    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::exportASCII(const QString &tableName,
                                    const QString &sep, bool colNames,
                                    bool colComments, bool expSelection) {
  MdiSubWindow *w = window(tableName);
  if (!w)
    return;
  const std::string windowClassName = w->metaObject()->className();
  if (!(windowClassName == "Matrix" || w->inherits("Table") ||
        windowClassName == "MantidMatrix"))
    return;

  QString selectedFilter;
  QString fname =
      QFileDialog::getSaveFileName(this, tr("Choose a filename to save under"),
                                   asciiDirPath + "/" + w->objectName(),
                                   "*.txt;;*.dat;;*.DAT", &selectedFilter);
  if (!fname.isEmpty()) {
    QFileInfo fi(fname);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fname.append(selectedFilter.remove("*"));

    asciiDirPath = fi.absolutePath();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    auto t = dynamic_cast<Table *>(w);
    auto m = dynamic_cast<Matrix *>(w);
    if (t)
      t->exportASCII(fname, sep, colNames, colComments, expSelection);
    else if (m)
      m->exportASCII(fname, sep, expSelection);
    else if (windowClassName == "MantidMatrix") {
      // call save ascii
      try {
        Mantid::API::IAlgorithm_sptr alg =
            mantidUI->createAlgorithm("SaveAscii");
        alg->setPropertyValue("Filename", fname.toStdString());
        alg->setPropertyValue("InputWorkspace", tableName.toStdString());
        alg->execute();
      } catch (...) {
      }
    }

    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::showRowsDialog() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  bool ok;
  int rows = QInputDialog::getInteger(
      this, tr("MantidPlot - Enter rows number"), tr("Rows"), // Mantid
      t->numRows(), 0, 1000000, 1, &ok);
  if (ok)
    t->resizeRows(rows);
}

void ApplicationWindow::showDeleteRowsDialog() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  bool ok;
  int start_row = QInputDialog::getInteger(this, tr("MantidPlot - Delete rows"),
                                           tr("Start row"), // Mantid
                                           1, 1, t->numRows(), 1, &ok);
  if (ok) {
    int end_row = QInputDialog::getInteger(
        this, tr("MantidPlot - Delete rows"), tr("End row"), // Mantid
        t->numRows(), 1, t->numRows(), 1, &ok);
    if (ok)
      t->deleteRows(start_row, end_row);
  }
}

void ApplicationWindow::showColsDialog() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  bool ok;
  int cols = QInputDialog::getInteger(
      this, tr("MantidPlot - Enter columns number"), tr("Columns"), // Mantid
      t->numCols(), 0, 1000000, 1, &ok);
  if (ok)
    t->resizeCols(cols);
}

void ApplicationWindow::showColumnValuesDialog() {
  Table *w = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!w)
    return;

  if (w->selectedColumns().count() > 0 || w->hasSelection()) {
    SetColValuesDialog *vd = new SetColValuesDialog(scriptingEnv(), w);
    vd->setAttribute(Qt::WA_DeleteOnClose);
    vd->exec();
  } else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"),
                         tr("Please select a column first!")); // Mantid
}

void ApplicationWindow::recalculateTable() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  auto table = dynamic_cast<Table *>(w);
  auto matrix = dynamic_cast<Matrix *>(w);

  if (table)
    table->calculate();
  else if (matrix)
    matrix->calculate();
}

void ApplicationWindow::sortActiveTable() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->sortTableDialog();
}

void ApplicationWindow::sortSelection() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->sortColumnsDialog();
}

void ApplicationWindow::normalizeActiveTable() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count()) > 0)
    t->normalize();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"),
                         tr("Please select a column first!")); // Mantid
}

void ApplicationWindow::normalizeSelection() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count()) > 0)
    t->normalizeSelection();
  else
    QMessageBox::warning(this, tr("MantidPlot - Column selection error"),
                         tr("Please select a column first!")); // Mantid
}

void ApplicationWindow::correlate() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2) {
    QMessageBox::warning(
        this, tr("MantidPlot - Error"),
        tr("Please select two columns for this operation!")); // Mantid
    return;
  }

  Correlation *cor = new Correlation(this, t, s[0], s[1]);
  cor->run();
  delete cor;
}

void ApplicationWindow::autoCorrelate() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 1) {
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Please select exactly one columns for this "
                            "operation!")); // Mantid
    return;
  }

  Correlation *cor = new Correlation(this, t, s[0], s[0]);
  cor->run();
  delete cor;
}

void ApplicationWindow::convolute() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2) {
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Please select two columns for this operation:\n "
                            "the first represents the signal and the second "
                            "the response function!")); // Mantid
    return;
  }

  Convolution *cv = new Convolution(this, t, s[0], s[1]);
  cv->run();
  delete cv;
}

void ApplicationWindow::deconvolute() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  QStringList s = t->selectedColumns();
  if ((int)s.count() != 2) {
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Please select two columns for this operation:\n "
                            "the first represents the signal and the second "
                            "the response function!")); // Mantid
    return;
  }

  Deconvolution *dcv = new Deconvolution(this, t, s[0], s[1]);
  dcv->run();
  delete dcv;
}

void ApplicationWindow::showColStatistics() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (static_cast<int>(t->selectedColumns().count()) > 0) {
    QList<int> targets;
    for (int i = 0; i < t->numCols(); i++)
      if (t->isColumnSelected(i, true))
        targets << i;
    newTableStatistics(t, TableStatistics::column, targets)->showNormal();
  } else
    QMessageBox::warning(this,
                         tr("MantidPlot - Column selection error"), // Mantid
                         tr("Please select a column first!"));
}

void ApplicationWindow::showRowStatistics() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (t->numSelectedRows() > 0) {
    QList<int> targets;
    for (int i = 0; i < t->numRows(); i++)
      if (t->isRowSelected(i, true))
        targets << i;
    newTableStatistics(t, TableStatistics::row, targets)->showNormal();
  } else
    QMessageBox::warning(this,
                         tr("MantidPlot - Row selection error"), // Mantid
                         tr("Please select a row first!"));
}

void ApplicationWindow::showColMenu(int c) {
  Table *w = dynamic_cast<Table *>(activeWindow(TableWindow));
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
  QMenu panels(this);
  QMenu stat(this);
  QMenu norm(this);

  if ((int)w->selectedColumns().count() == 1) {
    w->setSelectedCol(c);
    plot.addAction(QIcon(getQPixmap("lPlot_xpm")), tr("&Line"), this,
                   SLOT(plotL()));
    plot.addAction(QIcon(getQPixmap("pPlot_xpm")), tr("&Scatter"), this,
                   SLOT(plotP()));
    plot.addAction(QIcon(getQPixmap("lpPlot_xpm")), tr("Line + S&ymbol"), this,
                   SLOT(plotLP()));

    specialPlot.addAction(QIcon(getQPixmap("dropLines_xpm")),
                          tr("Vertical &Drop Lines"), this,
                          SLOT(plotVerticalDropLines()));
    specialPlot.addAction(QIcon(getQPixmap("spline_xpm")), tr("&Spline"), this,
                          SLOT(plotSpline()));
    specialPlot.addAction(QIcon(getQPixmap("vert_steps_xpm")),
                          tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
    specialPlot.addAction(QIcon(getQPixmap("hor_steps_xpm")),
                          tr("&Horizontal Steps"), this, SLOT(plotHorSteps()));
    specialPlot.setTitle(tr("Special Line/Symb&ol"));
    plot.addMenu(&specialPlot);
    plot.addSeparator();

    plot.addAction(QIcon(getQPixmap("vertBars_xpm")), tr("&Columns"), this,
                   SLOT(plotVerticalBars()));
    plot.addAction(QIcon(getQPixmap("hBars_xpm")), tr("&Rows"), this,
                   SLOT(plotHorizontalBars()));
    plot.addAction(QIcon(getQPixmap("area_xpm")), tr("&Area"), this,
                   SLOT(plotArea()));

    plot.addAction(QIcon(getQPixmap("pie_xpm")), tr("&Pie"), this,
                   SLOT(plotPie()));
    plot.addSeparator();

    plot.addAction(QIcon(getQPixmap("ribbon_xpm")), tr("3D Ribbo&n"), this,
                   SLOT(plot3DRibbon()));
    plot.addAction(QIcon(getQPixmap("bars_xpm")), tr("3D &Bars"), this,
                   SLOT(plot3DBars()));
    plot.addAction(QIcon(getQPixmap("scatter_xpm")), tr("3&D Scatter"), this,
                   SLOT(plot3DScatter()));
    plot.addAction(QIcon(getQPixmap("trajectory_xpm")), tr("3D &Trajectory"),
                   this, SLOT(plot3DTrajectory()));

    plot.addSeparator();

    stat.addAction(actionBoxPlot);
    stat.addAction(QIcon(getQPixmap("histogram_xpm")), tr("&Histogram"), this,
                   SLOT(plotHistogram()));
    stat.addAction(QIcon(getQPixmap("stacked_hist_xpm")),
                   tr("&Stacked Histograms"), this,
                   SLOT(plotStackedHistograms()));
    stat.addSeparator();
    stat.addAction(actionStemPlot);
    stat.setTitle(tr("Statistical &Graphs"));
    plot.addMenu(&stat);

    plot.setTitle(tr("&Plot"));
    contextMenu.addMenu(&plot);
    contextMenu.addSeparator();

    if (isEditable)
      contextMenu.addAction(QIcon(getQPixmap("cut_xpm")), tr("Cu&t"), w,
                            SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")), tr("&Copy"), w,
                          SLOT(copySelection()));
    if (isEditable)
      contextMenu.addAction(QIcon(getQPixmap("paste_xpm")), tr("Past&e"), w,
                            SLOT(pasteSelection()));
    contextMenu.addSeparator();

    QAction *xColID = colType.addAction(QIcon(getQPixmap("x_col_xpm")),
                                        tr("&X"), this, SLOT(setXCol()));
    xColID->setCheckable(true);
    QAction *yColID = colType.addAction(QIcon(getQPixmap("y_col_xpm")),
                                        tr("&Y"), this, SLOT(setYCol()));
    yColID->setCheckable(true);
    QAction *zColID = colType.addAction(QIcon(getQPixmap("z_col_xpm")),
                                        tr("&Z"), this, SLOT(setZCol()));
    zColID->setCheckable(true);
    colType.addSeparator();
    QAction *labelID =
        colType.addAction(QIcon(getQPixmap("set_label_col_xpm")), tr("&Label"),
                          this, SLOT(setLabelCol()));
    labelID->setCheckable(true);
    QAction *noneID =
        colType.addAction(QIcon(getQPixmap("disregard_col_xpm")), tr("&None"),
                          this, SLOT(disregardCol()));
    noneID->setCheckable(true);
    colType.addSeparator();
    QAction *xErrColID =
        colType.addAction(tr("X E&rror"), this, SLOT(setXErrCol()));
    xErrColID->setCheckable(true);
    QAction *yErrColID =
        colType.addAction(QIcon(getQPixmap("errors_xpm")), tr("Y &Error"), this,
                          SLOT(setYErrCol()));
    yErrColID->setCheckable(true);
    colType.addSeparator();

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

    colType.addAction(actionReadOnlyCol);
    actionReadOnlyCol->setCheckable(true);
    actionReadOnlyCol->setChecked(w->isReadOnlyColumn(c));

    colType.setTitle(tr("Set As"));
    contextMenu.addMenu(&colType);

    if (w) {
      if (isEditable)
        contextMenu.addSeparator();

      if (isEditable)
        contextMenu.addAction(actionShowColumnValuesDialog);
      if (isEditable)
        contextMenu.addAction(actionTableRecalculate);
      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Column With"));
      if (isEditable)
        contextMenu.addMenu(&fill);

      norm.addAction(tr("&Column"), w, SLOT(normalizeSelection()));
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      if (isEditable)
        contextMenu.addMenu(&norm);

      contextMenu.addSeparator();
      contextMenu.addAction(actionShowColStatistics);

      contextMenu.addSeparator();

      if (isEditable)
        contextMenu.addAction(QIcon(getQPixmap("erase_xpm")), tr("Clea&r"), w,
                              SLOT(clearSelection()));
      if (!isFixedColumns)
        contextMenu.addAction(QIcon(getQPixmap("delete_column_xpm")),
                              tr("&Delete"), w, SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.addSeparator();
      if (!isFixedColumns)
        contextMenu.addAction(getQPixmap("insert_column_xpm"), tr("&Insert"), w,
                              SLOT(insertCol()));
      if (!isFixedColumns)
        contextMenu.addAction(actionAddColToTable);
      contextMenu.addSeparator();

      sorting.addAction(QIcon(getQPixmap("sort_ascending_xpm")),
                        tr("&Ascending"), w, SLOT(sortColAsc()));
      sorting.addAction(QIcon(getQPixmap("sort_descending_xpm")),
                        tr("&Descending"), w, SLOT(sortColDesc()));

      sorting.setTitle(tr("Sort Colu&mn"));
      if (isSortable)
        contextMenu.addMenu(&sorting);

      if (isSortable)
        contextMenu.addAction(actionSortTable);
    }

    contextMenu.addSeparator();
    contextMenu.addAction(actionShowColumnOptionsDialog);
  } else if ((int)w->selectedColumns().count() > 1) {
    plot.addAction(QIcon(getQPixmap("lPlot_xpm")), tr("&Line"), this,
                   SLOT(plotL()));
    plot.addAction(QIcon(getQPixmap("pPlot_xpm")), tr("&Scatter"), this,
                   SLOT(plotP()));
    plot.addAction(QIcon(getQPixmap("lpPlot_xpm")), tr("Line + S&ymbol"), this,
                   SLOT(plotLP()));

    specialPlot.addAction(actionWaterfallPlot);
    specialPlot.addAction(QIcon(getQPixmap("dropLines_xpm")),
                          tr("Vertical &Drop Lines"), this,
                          SLOT(plotVerticalDropLines()));
    specialPlot.addAction(QIcon(getQPixmap("spline_xpm")), tr("&Spline"), this,
                          SLOT(plotSpline()));
    specialPlot.addAction(QIcon(getQPixmap("vert_steps_xpm")),
                          tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
    specialPlot.addAction(QIcon(getQPixmap("hor_steps_xpm")),
                          tr("&Vertical Steps"), this, SLOT(plotHorSteps()));
    specialPlot.setTitle(tr("Special Line/Symb&ol"));
    plot.addMenu(&specialPlot);
    plot.addSeparator();

    plot.addAction(QIcon(getQPixmap("vertBars_xpm")), tr("&Columns"), this,
                   SLOT(plotVerticalBars()));
    plot.addAction(QIcon(getQPixmap("hBars_xpm")), tr("&Rows"), this,
                   SLOT(plotHorizontalBars()));
    plot.addAction(QIcon(getQPixmap("area_xpm")), tr("&Area"), this,
                   SLOT(plotArea()));
    plot.addAction(QIcon(getQPixmap("vectXYXY_xpm")), tr("Vectors &XYXY"), this,
                   SLOT(plotVectXYXY()));
    plot.addSeparator();

    stat.addAction(actionBoxPlot);
    stat.addAction(QIcon(getQPixmap("histogram_xpm")), tr("&Histogram"), this,
                   SLOT(plotHistogram()));
    stat.addAction(QIcon(getQPixmap("stacked_hist_xpm")),
                   tr("&Stacked Histograms"), this,
                   SLOT(plotStackedHistograms()));
    stat.addSeparator();
    stat.addAction(actionStemPlot);
    stat.setTitle(tr("Statistical &Graphs"));
    plot.addMenu(&stat);

    panels.addAction(QIcon(getQPixmap("panel_v2_xpm")),
                     tr("&Vertical 2 Layers"), this,
                     SLOT(plot2VerticalLayers()));
    panels.addAction(QIcon(getQPixmap("panel_h2_xpm")),
                     tr("&Horizontal 2 Layers"), this,
                     SLOT(plot2HorizontalLayers()));
    panels.addAction(QIcon(getQPixmap("panel_4_xpm")), tr("&4 Layers"), this,
                     SLOT(plot4Layers()));
    panels.addAction(QIcon(getQPixmap("stacked_xpm")), tr("&Stacked Layers"),
                     this, SLOT(plotStackedLayers()));
    panels.setTitle(tr("Pa&nel"));
    plot.addMenu(&panels);

    plot.setTitle(tr("&Plot"));
    contextMenu.addMenu(&plot);
    contextMenu.addSeparator();
    if (isEditable)
      contextMenu.addAction(QIcon(getQPixmap("cut_xpm")), tr("Cu&t"), w,
                            SLOT(cutSelection()));
    contextMenu.addAction(QIcon(getQPixmap("copy_xpm")), tr("&Copy"), w,
                          SLOT(copySelection()));
    if (isEditable)
      contextMenu.addAction(QIcon(getQPixmap("paste_xpm")), tr("Past&e"), w,
                            SLOT(pasteSelection()));
    contextMenu.addSeparator();

    if (w) {
      if (isEditable)
        contextMenu.addAction(QIcon(getQPixmap("erase_xpm")), tr("Clea&r"), w,
                              SLOT(clearSelection()));
      if (isEditable)
        contextMenu.addAction(QIcon(getQPixmap("close_xpm")), tr("&Delete"), w,
                              SLOT(removeCol()));
      contextMenu.addAction(actionHideSelectedColumns);
      contextMenu.addAction(actionShowAllColumns);
      contextMenu.addSeparator();
      if (isEditable)
        contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
      if (isEditable)
        contextMenu.addAction(actionAddColToTable);
      if (isEditable)
        contextMenu.addSeparator();
    }

    colType.addAction(actionSetXCol);
    colType.addAction(actionSetYCol);
    colType.addAction(actionSetZCol);
    colType.addSeparator();
    colType.addAction(actionSetLabelCol);
    colType.addAction(actionDisregardCol);
    colType.addSeparator();
    colType.addAction(actionSetXErrCol);
    colType.addAction(actionSetYErrCol);
    colType.addSeparator();
    colType.addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
    colType.addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));
    colType.setTitle(tr("Set As"));
    contextMenu.addMenu(&colType);

    if (w) {
      if (isEditable)
        contextMenu.addSeparator();

      fill.addAction(actionSetAscValues);
      fill.addAction(actionSetRandomValues);
      fill.setTitle(tr("&Fill Columns With"));
      if (isEditable)
        contextMenu.addMenu(&fill);

      norm.addAction(actionNormalizeSelection);
      norm.addAction(actionNormalizeTable);
      norm.setTitle(tr("&Normalize"));
      if (isEditable)
        contextMenu.addMenu(&norm);

      if (isSortable)
        contextMenu.addSeparator();
      if (isSortable)
        contextMenu.addAction(actionSortSelection);
      if (isSortable)
        contextMenu.addAction(actionSortTable);
      contextMenu.addSeparator();
      contextMenu.addAction(actionShowColStatistics);
    }
  }

  QPoint posMouse = QCursor::pos();
  contextMenu.exec(posMouse);
}

void ApplicationWindow::plot2VerticalLayers() {
  multilayerPlot(1, 2, defaultCurveStyle);
}

void ApplicationWindow::plot2HorizontalLayers() {
  multilayerPlot(2, 1, defaultCurveStyle);
}

void ApplicationWindow::plot4Layers() {
  multilayerPlot(2, 2, defaultCurveStyle);
}

void ApplicationWindow::plotStackedLayers() {
  multilayerPlot(1, -1, defaultCurveStyle);
}

void ApplicationWindow::plotStackedHistograms() {
  multilayerPlot(1, -1, GraphOptions::Histogram);
}

void ApplicationWindow::showMatrixDialog() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixDialog *md = new MatrixDialog(this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix(m);
  md->exec();
}

void ApplicationWindow::showMatrixSizeDialog() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixSizeDialog *md = new MatrixSizeDialog(m, this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->exec();
}

void ApplicationWindow::showMatrixValuesDialog() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  MatrixValuesDialog *md = new MatrixValuesDialog(scriptingEnv(), this);
  md->setAttribute(Qt::WA_DeleteOnClose);
  md->setMatrix(m);
  md->exec();
}

void ApplicationWindow::showColumnOptionsDialog() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (t->selectedColumns().count() > 0) {
    TableDialog *td = new TableDialog(t);
    td->setAttribute(Qt::WA_DeleteOnClose);
    td->exec();
  } else
    QMessageBox::warning(this, tr("MantidPlot"),
                         tr("Please select a column first!")); // Mantid
}

void ApplicationWindow::showGeneralPlotDialog() {
  MdiSubWindow *plot = activeWindow();
  if (!plot)
    return;

  const std::string plotClassName = plot->metaObject()->className();
  if (plotClassName == "MultiLayer") {
    // do the dynamic_casts inside a try/c, as there may be issues with the
    // plots
    // (source data/table/matrix has been removed/closed, etc.)
    MultiLayer *ml;
    try {
      ml = dynamic_cast<MultiLayer *>(plot);
    } catch (std::runtime_error &) {
      g_log.error()
          << "Failed to open general plot dialog for multi layer plot";
      return;
    }
    if (ml && ml->layers())
      showPlotDialog();
  } else if (plotClassName == "Graph3D") {
    QDialog *gd = showScaleDialog();
    Plot3DDialog *plot3D;
    try {
      plot3D = dynamic_cast<Plot3DDialog *>(gd);
    } catch (std::runtime_error &) {
      g_log.error() << "Failed to open general plot dialog for 3D plot";
      return;
    }
    if (plot3D)
      plot3D->showGeneralTab();
  }
}

void ApplicationWindow::showAxisDialog() {
  MdiSubWindow *plot = activeWindow();
  if (!plot)
    return;

  const std::string plotClassName = plot->metaObject()->className();
  QDialog *gd = showScaleDialog();
  if (gd && plotClassName == "MultiLayer") {
    MultiLayer *ml = dynamic_cast<MultiLayer *>(plot);
    if (!ml || !ml->layers())
      return;

    auto ad = dynamic_cast<AxesDialog *>(gd);
    if (ad)
      ad->showAxesPage();
  } else if (gd && plotClassName == "Graph3D") {
    Plot3DDialog *p3d = dynamic_cast<Plot3DDialog *>(gd);
    if (p3d)
      p3d->showAxisTab();
  }
}

void ApplicationWindow::showGridDialog() {
  AxesDialog *gd = dynamic_cast<AxesDialog *>(showScaleDialog());
  if (gd)
    gd->showGridPage();
}

QDialog *ApplicationWindow::showScaleDialog() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return nullptr;
  const std::string windowClassName = w->metaObject()->className();
  if (windowClassName == "MultiLayer") {
    auto ml = dynamic_cast<MultiLayer *>(w);
    if (!ml || ml->isEmpty())
      return nullptr;

    Graph *g = ml->activeGraph();
    if (!g)
      return nullptr;

    if (g->isPiePlot()) {
      QMessageBox::warning(
          this, tr("MantidPlot - Warning"),
          tr("This functionality is not available for pie plots!")); // Mantid
      return nullptr;
    }

    AxesDialog *ad = new AxesDialog(this, g);
    ad->exec();
    return ad;
  } else if (windowClassName == "Graph3D")
    return showPlot3dDialog();

  return nullptr;
}

AxesDialog *ApplicationWindow::showScalePageFromAxisDialog(int axisPos) {
  AxesDialog *gd = dynamic_cast<AxesDialog *>(showScaleDialog());
  if (gd)
    gd->setCurrentScale(axisPos);

  return gd;
}

AxesDialog *ApplicationWindow::showAxisPageFromAxisDialog(int axisPos) {
  AxesDialog *gd = dynamic_cast<AxesDialog *>(showScaleDialog());
  if (gd) {
    gd->showAxesPage();
    gd->setCurrentScale(axisPos);
  }
  return gd;
}

QDialog *ApplicationWindow::showPlot3dDialog() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return nullptr;

  if (!g->hasData()) {
    QApplication::restoreOverrideCursor();
    QMessageBox::warning(this, tr("MantidPlot - Warning"), // Mantid
                         tr("Not available for empty 3D surface plots!"));
    return nullptr;
  }

  Plot3DDialog *pd = new Plot3DDialog(this);
  pd->setPlot(g);
  pd->show();
  return pd;
}

void ApplicationWindow::showPlotDialog(int curveKey) {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  PlotDialog *pd = new PlotDialog(d_extended_plot_dialog, this, w);
  pd->setAttribute(Qt::WA_DeleteOnClose);
  pd->insertColumnsList(columnsList(Table::All));
  if (curveKey >= 0) {
    Graph *g = w->activeGraph();
    if (g)
      pd->selectCurve(g->curveIndex(curveKey));
  }
  pd->initFonts(plotTitleFont, plotAxesFont, plotNumbersFont, plotLegendFont);
  pd->showAll(d_extended_plot_dialog);
  pd->show();
}

void ApplicationWindow::showCurvePlotDialog() {
  showPlotDialog(actionShowCurvePlotDialog->data().toInt());
}

void ApplicationWindow::showCurveContextMenu(int curveKey) {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  DataCurve *c = dynamic_cast<DataCurve *>(g->curve(g->curveIndex(curveKey)));
  if (!c || !c->isVisible())
    return;

  QMenu curveMenu(this);
  curveMenu.addAction(c->title().text(), this, SLOT(showCurvePlotDialog()));
  curveMenu.addSeparator();

  curveMenu.addAction(actionHideCurve);
  actionHideCurve->setData(curveKey);

  if (g->visibleCurves() > 1 && c->type() == GraphOptions::Function) {
    curveMenu.addAction(actionHideOtherCurves);
    actionHideOtherCurves->setData(curveKey);
  } else if (c->type() != GraphOptions::Function) {
    if ((g->visibleCurves() - c->errorBarsList().count()) > 1) {
      curveMenu.addAction(actionHideOtherCurves);
      actionHideOtherCurves->setData(curveKey);
    }
  }

  if (g->visibleCurves() != g->curves())
    curveMenu.addAction(actionShowAllCurves);
  curveMenu.addSeparator();

  if (g->activeTool()) {
    if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
        g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
      curveMenu.addAction(actionCopySelection);
  }

  if (c->type() == GraphOptions::Function) {
    curveMenu.addSeparator();
    curveMenu.addAction(actionEditFunction);
    actionEditFunction->setData(curveKey);
  } else if (c->type() != GraphOptions::ErrorBars) {
    if (g->activeTool()) {
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
          g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker) {
        curveMenu.addAction(actionCutSelection);
        curveMenu.addAction(actionPasteSelection);
        curveMenu.addAction(actionClearSelection);
        curveMenu.addSeparator();
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector) {
          QAction *act = new QAction(tr("Set Display Range"), this);
          connect(act, SIGNAL(triggered()),
                  dynamic_cast<RangeSelectorTool *>(g->activeTool()),
                  SLOT(setCurveRange()));
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

    curveMenu.addSeparator();
  }

  curveMenu.addAction(actionShowCurveWorksheet);
  actionShowCurveWorksheet->setData(curveKey);

  curveMenu.addAction(actionShowCurvePlotDialog);
  actionShowCurvePlotDialog->setData(curveKey);

  curveMenu.addSeparator();

  curveMenu.addAction(actionRemoveCurve);
  actionRemoveCurve->setData(curveKey);
  curveMenu.exec(QCursor::pos());
}

void ApplicationWindow::showAllCurves() {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  if (!g)
    return;

  for (int i = 0; i < g->curves(); i++)
    g->showCurve(i);
  g->replot();
}

void ApplicationWindow::hideOtherCurves() {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionHideOtherCurves->data().toInt();
  for (int i = 0; i < g->curves(); i++)
    g->showCurve(i, false);

  g->showCurve(g->curveIndex(curveKey));
  g->replot();
}

void ApplicationWindow::hideCurve() {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionHideCurve->data().toInt();
  g->showCurve(g->curveIndex(curveKey), false);
}

void ApplicationWindow::removeCurve() {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionRemoveCurve->data().toInt();
  g->removeCurve(g->curveIndex(curveKey));
  g->updatePlot();
}

void ApplicationWindow::showCurveWorksheet(Graph *g, int curveIndex) {
  if (!g)
    return;

  QwtPlotItem *it = g->plotItem(curveIndex);
  if (!it)
    return;

  auto sp = dynamic_cast<Spectrogram *>(it);
  auto pc = dynamic_cast<PlotCurve *>(it);

  if (sp && sp->matrix())
    sp->matrix()->showMaximized();
  if (pc && pc->type() == GraphOptions::Function)
    g->createTable(pc);

  if (!pc && !sp)
    showTable(it->title().text());
}

void ApplicationWindow::showCurveWorksheet() {
  MultiLayer *w = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!w)
    return;

  Graph *g = w->activeGraph();
  if (!g)
    return;

  int curveKey = actionShowCurveWorksheet->data().toInt();
  showCurveWorksheet(g, g->curveIndex(curveKey));
}

void ApplicationWindow::zoomIn() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if (dynamic_cast<Graph *>(plot->activeGraph())->isPiePlot()) {
    if (btnZoomIn->isChecked())
      QMessageBox::warning(
          this, tr("MantidPlot - Warning"), // Mantid
          tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers) {
    if (!g->isPiePlot())
      g->zoomMode(true);
  }
}

void ApplicationWindow::zoomOut() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty() ||
      dynamic_cast<Graph *>(plot->activeGraph())->isPiePlot())
    return;

  (dynamic_cast<Graph *>(plot->activeGraph()))->zoomOut();
  btnPointer->setChecked(true);
}

void ApplicationWindow::setAutoScale() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g)
    g->setAutoScale();
}

void ApplicationWindow::removePoints() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g || !g->validCurvesDataSize()) {
    btnPointer->setChecked(true);
    return;
  }

  if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  } else {
    switch (QMessageBox::warning(
        this, tr("MantidPlot"), // Mantid
        tr("This will modify the data in the worksheets!\nAre you sure you "
           "want to continue?"),
        tr("Continue"), tr("Cancel"), nullptr, 1)) {
    case 0:
      g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Remove, info,
                                          SLOT(setText(const QString &))));
      displayBar->show();
      break;

    case 1:
      btnPointer->setChecked(true);
      break;
    }
  }
}

void ApplicationWindow::movePoints() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g || !g->validCurvesDataSize()) {
    btnPointer->setChecked(true);
    return;
  }

  if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("This functionality is not available for pie plots!"));

    btnPointer->setChecked(true);
    return;
  } else {
    switch (QMessageBox::warning(
        this, tr("MantidPlot"), // Mantid
        tr("This will modify the data in the worksheets!\nAre you sure you "
           "want to continue?"),
        tr("Continue"), tr("Cancel"), nullptr, 1)) {
    case 0:
      if (g) {
        g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Move, info,
                                            SLOT(setText(const QString &))));
        displayBar->show();
      }
      break;

    case 1:
      btnPointer->setChecked(true);
      break;
    }
  }
}

void ApplicationWindow::exportPDF() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);

  if (std::string(w->metaObject()->className()) == "MultiLayer" && ml &&
      ml->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }

  QString fname = QFileDialog::getSaveFileName(
      this, tr("Choose a filename to save under"), workingDir, "*.pdf");
  if (!fname.isEmpty()) {
    QFileInfo fi(fname);
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fname.append(".pdf");

    workingDir = fi.absolutePath();

    QFile f(fname);
    if (!f.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(
          this, tr("MantidPlot - Export error"), // Mantid
          tr("Could not write to file: <h4>%1</h4><p>Please verify that you "
             "have the right to write to this location or that the file is "
             "not "
             "being used by another application!")
              .arg(fname));
      return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    w->exportPDF(fname);
    QApplication::restoreOverrideCursor();
  }
}

// print active window
void ApplicationWindow::print() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  auto ml = dynamic_cast<MultiLayer *>(w);
  if (std::string(w->metaObject()->className()) == "MultiLayer" && ml &&
      ml->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"));
    return;
  }
  w->print();
}

void ApplicationWindow::printAllPlots() {
  QPrinter printer;
  printer.setOrientation(QPrinter::Landscape);
  printer.setColorMode(QPrinter::Color);
  printer.setFullPage(true);
  QPrintDialog dialog(&printer);
  if (dialog.exec()) {
    QPainter *paint = new QPainter(&printer);

    int plots = 0;
    QList<MdiSubWindow *> windows = windowsList();
    foreach (MdiSubWindow *w, windows) {
      if (std::string(w->metaObject()->className()) == "MultiLayer")
        plots++;
    }

    dialog.setMinMax(0, plots);
    printer.setFromTo(0, plots);

    bool firstPage = true;
    foreach (MdiSubWindow *w, windows) {
      if (std::string(w->metaObject()->className()) == "MultiLayer") {
        if (firstPage || printer.newPage()) {
          MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
          if (ml)
            ml->printAllLayers(paint);
          firstPage = false;
        }
      }
    }
    paint->end();
    delete paint;
  }
}

void ApplicationWindow::showExpGrowthDialog() { showExpDecayDialog(-1); }

void ApplicationWindow::showExpDecayDialog() { showExpDecayDialog(1); }

void ApplicationWindow::showExpDecayDialog(int type) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  ExpDecayDialog *edd = new ExpDecayDialog(type, this);
  edd->setAttribute(Qt::WA_DeleteOnClose);
  connect(g, SIGNAL(destroyed()), edd, SLOT(close()));

  edd->setGraph(g);
  edd->show();
}

void ApplicationWindow::showTwoExpDecayDialog() { showExpDecayDialog(2); }

void ApplicationWindow::showExpDecay3Dialog() { showExpDecayDialog(3); }

void ApplicationWindow::showFitDialog() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  MultiLayer *plot = nullptr;
  if (std::string(w->metaObject()->className()) == "MultiLayer")
    plot = dynamic_cast<MultiLayer *>(w);
  else if (w->inherits("Table")) {
    Table *t = dynamic_cast<Table *>(w);
    if (t)
      plot = multilayerPlot(t, t->drawableColumnSelection(),
                            GraphOptions::LineSymbols);
  }

  if (!plot)
    return;

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g || !g->validCurvesDataSize())
    return;

  FitDialog *fd = new FitDialog(g, this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  connect(plot, SIGNAL(destroyed()), fd, SLOT(close()));

  fd->setSrcTables(tableList());
  fd->show();
  fd->resize(fd->minimumSize());
}

void ApplicationWindow::showFilterDialog(int filter) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g && g->validCurvesDataSize()) {
    FilterDialog *fd = new FilterDialog(filter, this);
    fd->setAttribute(Qt::WA_DeleteOnClose);
    fd->setGraph(g);
    fd->exec();
  }
}

void ApplicationWindow::lowPassFilterDialog() {
  showFilterDialog(FFTFilter::LowPass);
}

void ApplicationWindow::highPassFilterDialog() {
  showFilterDialog(FFTFilter::HighPass);
}

void ApplicationWindow::bandPassFilterDialog() {
  showFilterDialog(FFTFilter::BandPass);
}

void ApplicationWindow::bandBlockFilterDialog() {
  showFilterDialog(FFTFilter::BandBlock);
}

void ApplicationWindow::showFFTDialog() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  FFTDialog *sd = nullptr;
  if (std::string(w->metaObject()->className()) == "MultiLayer") {
    MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
    if (!ml)
      return;

    Graph *g = ml->activeGraph();
    if (!g)
      return;

    if (g->validCurvesDataSize()) {
      sd = new FFTDialog(FFTDialog::onGraph, this);
      sd->setAttribute(Qt::WA_DeleteOnClose);
      sd->setGraph(g);
    }
  } else if (w->inherits("Table")) {
    Table *t = dynamic_cast<Table *>(w);
    if (!t)
      return;
    sd = new FFTDialog(FFTDialog::onTable, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setTable(t);
  } else if (w->inherits("Matrix")) {
    Matrix *m = dynamic_cast<Matrix *>(w);
    if (!m)
      return;
    sd = new FFTDialog(FFTDialog::onMatrix, this);
    sd->setAttribute(Qt::WA_DeleteOnClose);
    sd->setMatrix(m);
  }

  if (sd)
    sd->exec();
}

void ApplicationWindow::showSmoothDialog(int m) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  SmoothCurveDialog *sd = new SmoothCurveDialog(m, this);
  sd->setAttribute(Qt::WA_DeleteOnClose);
  sd->setGraph(g);
  sd->exec();
}

void ApplicationWindow::showSmoothSavGolDialog() {
  showSmoothDialog(SmoothFilter::SavitzkyGolay);
}

void ApplicationWindow::showSmoothFFTDialog() {
  showSmoothDialog(SmoothFilter::FFT);
}

void ApplicationWindow::showSmoothAverageDialog() {
  showSmoothDialog(SmoothFilter::Average);
}

void ApplicationWindow::showInterpolationDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  InterpolationDialog *id = new InterpolationDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect(g, SIGNAL(destroyed()), id, SLOT(close()));
  id->setGraph(g);
  id->show();
}

void ApplicationWindow::showFitPolynomDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  PolynomFitDialog *pfd = new PolynomFitDialog(this);
  pfd->setAttribute(Qt::WA_DeleteOnClose);
  connect(g, SIGNAL(destroyed()), pfd, SLOT(close()));
  pfd->setGraph(g);
  pfd->show();
}

void ApplicationWindow::updateLog(const QString &result) {
  if (!result.isEmpty()) {
    currentFolder()->appendLogInfo(result);
    showResults(true);
    emit modified();
  }
}

void ApplicationWindow::showIntegrationDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  IntDialog *id = new IntDialog(this, g);
  id->exec();
}

void ApplicationWindow::showResults(bool ok) {
  if (ok) {
    QString text;
    if (!currentFolder()->logInfo().isEmpty())
      text = currentFolder()->logInfo();
    else
      text = "Sorry, there are no results to display!";
    using MantidQt::MantidWidgets::Message;
    resultsLog->replace(Message(text, Message::Priority::PRIO_INFORMATION));
  }
  logWindow->setVisible(ok);
}

void ApplicationWindow::showResults(const QString &s, bool ok) {
  currentFolder()->appendLogInfo(s);
  QString logInfo = currentFolder()->logInfo();
  if (!logInfo.isEmpty()) {
    using MantidQt::MantidWidgets::Message;
    resultsLog->replace(Message(logInfo, Message::Priority::PRIO_INFORMATION));
  }
  showResults(ok);
}

void ApplicationWindow::showScreenReader() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers)
    g->setActiveTool(
        new ScreenPickerTool(g, info, SLOT(setText(const QString &))));

  displayBar->show();
}

void ApplicationWindow::drawPoints() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers)
    g->setActiveTool(
        new DrawPointTool(this, g, info, SLOT(setText(const QString &))));

  displayBar->show();
}

void ApplicationWindow::showRangeSelectors() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("There are no plot layers available in this window!")); // Mantid
    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g)
    return;

  if (!g->curves()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("There are no curves available on this plot!")); // Mantid
    btnPointer->setChecked(true);
    return;
  } else if (g->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"),
        tr("This functionality is not available for pie plots!")); // Mantid
    btnPointer->setChecked(true);
    return;
  }

  displayBar->show();
  g->enableRangeSelectors(info, SLOT(setText(const QString &)));
}

void ApplicationWindow::showCursor() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if (dynamic_cast<Graph *>(plot->activeGraph())->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers) {
    if (g->isPiePlot() || !g->curves())
      continue;
    if (g->validCurvesDataSize())
      g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Display,
                                          info,
                                          SLOT(setText(const QString &))));
  }
  displayBar->show();
}

/**  Switch on the multi-peak selecting tool for fitting
 * with the Fit algorithm of multiple peaks on a single background
 */
void ApplicationWindow::selectMultiPeak(bool showFitPropertyBrowser) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  selectMultiPeak(plot, showFitPropertyBrowser);
}

/**  Switch on the multi-peak selecting tool for fitting with the Fit
 * algorithm.
 * @param plot :: The MultiLayer the tool will apply to.
 * @param showFitPropertyBrowser :: Set if FitPropertyBrowser must be shown as
 * well.
 */
void ApplicationWindow::selectMultiPeak(MultiLayer *plot,
                                        bool showFitPropertyBrowser,
                                        double xmin, double xmax) {
  setActiveWindow(plot);

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  if (dynamic_cast<Graph *>(plot->activeGraph())->isPiePlot()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("This functionality is not available for pie plots!"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers) {
    if (g->isPiePlot() || !g->curves()) {
      continue;
    }
    if (g->validCurvesDataSize()) {
      // Called when setting up usual peakPickerTool
      PeakPickerTool *ppicker = new PeakPickerTool(
          g, mantidUI->fitFunctionBrowser(), mantidUI, showFitPropertyBrowser);
      if (!ppicker->isInitialized()) {
        QMessageBox::warning(this, tr("MantidPlot - Warning"),
                             tr("This functionality is not available for the "
                                "underlying data."));
        delete ppicker;
        btnPointer->setChecked(true);
        return;
      }
      if (xmin != xmax) {
        mantidUI->fitFunctionBrowser()->setStartX(xmin);
        mantidUI->fitFunctionBrowser()->setEndX(xmax);
      }
      g->setActiveTool(ppicker);
    }
  }
}

void ApplicationWindow::newLegend() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g)
    g->newLegend();
}

void ApplicationWindow::addTimeStamp() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g)
    g->addTimeStamp();
}

void ApplicationWindow::addLabel() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));

    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g) {
    g->setActiveTool(new LabelTool(g));
  }
}

void ApplicationWindow::addImage() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (!g)
    return;

  QList<QByteArray> list = QImageReader::supportedImageFormats();
  QString filter = tr("images") + " (", aux1, aux2;
  for (int i = 0; i < (int)list.count(); i++) {
    aux1 = " *." + list[i] + " ";
    aux2 += " *." + list[i] + ";;";
    filter += aux1;
  }
  filter += ");;" + aux2;

  QString fn = QFileDialog::getOpenFileName(
      this, tr("MantidPlot - Insert image from file"), imagesDirPath,
      filter); // Mantid
  if (!fn.isEmpty()) {
    QFileInfo fi(fn);
    imagesDirPath = fi.absolutePath();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    g->addImage(fn);
    QApplication::restoreOverrideCursor();
  }
}

void ApplicationWindow::drawLine() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));

    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g) {
    g->drawLine(true);
    emit modified();
  }
}

void ApplicationWindow::drawArrow() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));

    btnPointer->setChecked(true);
    return;
  }

  Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
  if (g) {
    g->drawLine(true, true);
    emit modified();
  }
}

void ApplicationWindow::showImageDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g) {
    ImageMarker *im = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
    if (!im)
      return;

    ImageDialog *id = new ImageDialog(this);
    id->setAttribute(Qt::WA_DeleteOnClose);
    connect(id, SIGNAL(setGeometry(int, int, int, int)), g,
            SLOT(updateImageMarker(int, int, int, int)));
    id->setOrigin(im->origin());
    id->setSize(im->size());
    id->exec();
  }
}

void ApplicationWindow::showLayerDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("There are no plot layers available in this window."));
    return;
  }

  LayerDialog *id = new LayerDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  id->setMultiLayer(plot);
  id->exec();
}

void ApplicationWindow::showTextDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g) {
    LegendWidget *l = dynamic_cast<LegendWidget *>(g->selectedText());
    if (!l)
      return;

    TextDialog *td = new TextDialog(TextDialog::TextMarker, this, nullptr);
    td->setLegendWidget(l);
    td->exec();
  }
}

void ApplicationWindow::showLineDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g) {
    ArrowMarker *lm = dynamic_cast<ArrowMarker *>(g->selectedMarkerPtr());
    if (!lm)
      return;

    LineDialog *ld = new LineDialog(lm, this);
    ld->exec();
  }
}

void ApplicationWindow::addColToTable() {
  Table *m = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (m)
    m->addCol();
}

void ApplicationWindow::clearSelection() {
  if (lv->hasFocus()) {
    deleteSelectedItems();
    return;
  }

  MdiSubWindow *m = activeWindow();
  if (!m)
    return;

  if (isOfType(m, "Table")) {
    auto t = dynamic_cast<Table *>(m);
    if (t)
      t->clearSelection();
  } else if (isOfType(m, "Matrix")) {
    auto matrix = dynamic_cast<Matrix *>(m);
    if (matrix)
      matrix->clearSelection();
  } else if (isOfType(m, "MultiLayer")) {
    auto ml = dynamic_cast<MultiLayer *>(m);
    if (!ml)
      return;

    Graph *g = ml->activeGraph();
    if (!g)
      return;

    if (g->activeTool() && !dynamic_cast<PeakPickerTool *>(g->activeTool())) {
      auto rst = dynamic_cast<RangeSelectorTool *>(g->activeTool());
      auto lbt = dynamic_cast<LabelTool *>(g->activeTool());

      if (rst)
        rst->clearSelection();
      else if (lbt)
        lbt->removeTextBox();
    }

    else if (g->titleSelected())
      g->removeTitle();
    else if (g->markerSelected())
      g->removeMarker();
  } else if (isOfType(m, "Note")) {
    auto note = dynamic_cast<Note *>(m);
    if (note)
      note->editor()->clear();
  }
  emit modified();
}

void ApplicationWindow::copySelection() {
  if (info->hasFocus()) {
    info->copy();
    return;
  }
  MdiSubWindow *m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table")) {
    Table *table = dynamic_cast<Table *>(m);
    if (table)
      table->copySelection();
  } else if (isOfType(m, "Matrix")) {
    Matrix *matrix = dynamic_cast<Matrix *>(m);
    if (matrix)
      matrix->copySelection();
  } else if (isOfType(m, "MultiLayer")) {
    MultiLayer *plot = dynamic_cast<MultiLayer *>(m);
    if (!plot || plot->layers() == 0)
      return;

    Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
    if (!g)
      return;

    if (g->activeTool()) {
      if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector) {
        RangeSelectorTool *rst =
            dynamic_cast<RangeSelectorTool *>(g->activeTool());
        if (rst)
          rst->copySelection();
      }
    } else if (g->markerSelected())
      copyMarker();
    else
      copyActiveLayer();

    plot->copyAllLayers();
  } else if (isOfType(m, "Note")) {
    Note *note = dynamic_cast<Note *>(m);
    if (note)
      note->editor()->copy();
  } else
    mantidUI->copyValues(); // Mantid
}

void ApplicationWindow::cutSelection() {

  MdiSubWindow *m = activeWindow();
  if (!m)
    return;

  auto t = dynamic_cast<Table *>(m);
  auto mat = dynamic_cast<Matrix *>(m);
  auto plot = dynamic_cast<MultiLayer *>(m);
  auto note = dynamic_cast<Note *>(m);

  if (t)
    t->cutSelection();
  else if (mat)
    mat->cutSelection();
  else if (plot && plot->layers()) {
    Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
    if (!g)
      return;

    if (g->activeTool()) {
      auto rst = dynamic_cast<RangeSelectorTool *>(g->activeTool());
      if (rst)
        rst->cutSelection();
    } else {
      copyMarker();
      g->removeMarker();
    }
  } else if (note)
    note->editor()->cut();

  emit modified();
}

void ApplicationWindow::copyMarker() {
  lastCopiedLayer = nullptr;

  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g && g->markerSelected()) {
    if (g->selectedText()) {
      d_text_copy = g->selectedText();
      d_image_copy = nullptr;
      d_arrow_copy = nullptr;
    } else if (g->arrowMarkerSelected()) {
      d_arrow_copy = dynamic_cast<ArrowMarker *>(g->selectedMarkerPtr());
      d_image_copy = nullptr;
      d_text_copy = nullptr;
    } else if (g->imageMarkerSelected()) {
      d_image_copy = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
      d_text_copy = nullptr;
      d_arrow_copy = nullptr;
    }
  }
}

void ApplicationWindow::pasteSelection() {
  MdiSubWindow *m = activeWindow();
  if (!m)
    return;

  if (m->inherits("Table")) {
    auto table = dynamic_cast<Table *>(m);
    if (table)
      table->pasteSelection();
  } else if (isOfType(m, "Matrix")) {
    auto matrix = dynamic_cast<Matrix *>(m);
    if (matrix)
      matrix->pasteSelection();
  } else if (isOfType(m, "Note")) {
    auto note = dynamic_cast<Note *>(m);
    if (note)
      note->editor()->paste();
  } else if (isOfType(m, "MultiLayer")) {
    MultiLayer *plot = dynamic_cast<MultiLayer *>(m);
    if (!plot)
      return;

    if (lastCopiedLayer) {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      Graph *g = plot->addLayer();
      g->copy(lastCopiedLayer);
      QPoint pos = plot->mapFromGlobal(QCursor::pos());
      plot->setGraphGeometry(pos.x(), pos.y() - 20, lastCopiedLayer->width(),
                             lastCopiedLayer->height());
      if (g->isWaterfallPlot())
        g->updateDataCurves();

      QApplication::restoreOverrideCursor();
    } else {
      if (plot->layers() == 0)
        return;

      Graph *g = dynamic_cast<Graph *>(plot->activeGraph());
      if (!g)
        return;

      if (g->activeTool()) {
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector) {
          auto rst = dynamic_cast<RangeSelectorTool *>(g->activeTool());
          if (rst)
            rst->pasteSelection();
        }
      } else if (d_text_copy) {
        LegendWidget *t = g->insertText(d_text_copy);
        t->move(g->mapFromGlobal(QCursor::pos()));
      } else if (d_arrow_copy) {
        ArrowMarker *a = g->addArrow(d_arrow_copy);
        a->setStartPoint(QPoint(d_arrow_copy->startPoint().x() + 10,
                                d_arrow_copy->startPoint().y() + 10));
        a->setEndPoint(QPoint(d_arrow_copy->endPoint().x() + 10,
                              d_arrow_copy->endPoint().y() + 10));
        g->replot();
        g->deselectMarker();
      } else if (d_image_copy) {
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
 * Clone an MDI window. TODO: if this method is to be used it needs
 * refactoring.
 *
 * @param w :: A window to clone.
 * @return :: Pointer to the cloned window if successful or NULL if failed.
 */
MdiSubWindow *ApplicationWindow::clone(MdiSubWindow *w) {
  if (!w) {
    w = activeWindow();
    if (!w) {
      QMessageBox::critical(
          this, tr("MantidPlot - Duplicate window error"), // Mantid
          tr("There are no windows available in this project!"));
      return nullptr;
    }
  }

  MdiSubWindow *nw = nullptr;
  MdiSubWindow::Status status = w->status();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if (isOfType(w, "MultiLayer")) {
    MultiLayer *g = dynamic_cast<MultiLayer *>(w);
    if (!g)
      return nullptr;
    nw = multilayerPlot(generateUniqueName(tr("Graph")), 0, g->getRows(),
                        g->getCols());
    auto nwg = dynamic_cast<MultiLayer *>(nw);
    if (nwg)
      nwg->copy(g);
  } else if (w->inherits("Table")) {
    Table *t = dynamic_cast<Table *>(w);
    if (!t)
      return nullptr;
    QString caption = generateUniqueName(tr("Table"));
    nw = newTable(caption, t->numRows(), t->numCols());

    Table *nt = dynamic_cast<Table *>(nw);
    if (!nt)
      return nullptr;
    nt->setHeader(t->colNames());

    for (auto i = 0; i < nt->numCols(); i++) {
      for (auto j = 0; j < nt->numRows(); j++) {
        auto io = t->table()->item(j, i);
        nt->table()->setItem(j, i, io);
      }
    }

  } else if (isOfType(w, "Graph3D")) {
    Graph3D *g = dynamic_cast<Graph3D *>(w);
    if (!g)
      return nullptr;
    if (!g->hasData()) {
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(
          this, tr("MantidPlot - Duplicate error"),
          tr("Empty 3D surface plots cannot be duplicated!")); // Mantid
      return nullptr;
    }

    QString caption = generateUniqueName(tr("Graph"));
    QString s = g->formula();
    if (g->userFunction()) {
      UserFunction2D *f = dynamic_cast<UserFunction2D *>(g->userFunction());
      if (f) {
        nw = plotSurface(f->formula(), g->xStart(), g->xStop(), g->yStart(),
                         g->yStop(), g->zStart(), g->zStop(), f->columns(),
                         f->rows());
      } else {
        QMessageBox::warning(this, "MantidPlot: warning",
                             "Function cannot be cloned.");
        return nullptr;
      }
    } else if (g->parametricSurface()) {
      UserParametricSurface *s = g->parametricSurface();
      nw = plotParametricSurface(s->xFormula(), s->yFormula(), s->zFormula(),
                                 s->uStart(), s->uEnd(), s->vStart(), s->vEnd(),
                                 s->columns(), s->rows(), s->uPeriodic(),
                                 s->vPeriodic());
    } else if (s.endsWith("(Z)"))
      nw = openPlotXYZ(caption, s, g->xStart(), g->xStop(), g->yStart(),
                       g->yStop(), g->zStart(), g->zStop());
    else if (s.endsWith("(Y)")) // Ribbon plot
      nw = dataPlot3D(caption, s, g->xStart(), g->xStop(), g->yStart(),
                      g->yStop(), g->zStart(), g->zStop());
    else
      nw = openMatrixPlot3D(caption, s, g->xStart(), g->xStop(), g->yStart(),
                            g->yStop(), g->zStart(), g->zStop());

    if (!nw)
      return nullptr;

    if (status == MdiSubWindow::Maximized)
      nw->hide();
    auto g3d = dynamic_cast<Graph3D *>(nw);
    if (g3d)
      g3d->copy(g);
    customToolBars(nw);
  } else if (isOfType(w, "Matrix")) {
    auto matrix = dynamic_cast<Matrix *>(w);
    if (!matrix)
      return nullptr;
    nw = newMatrix(matrix->numRows(), matrix->numCols());
    auto nwmatrix = dynamic_cast<Matrix *>(nw);
    if (nwmatrix)
      nwmatrix->copy(matrix);
  } else if (isOfType(w, "Note")) {
    auto note = dynamic_cast<Note *>(w);
    if (!note)
      return nullptr;

    nw = newNote();
    if (!nw)
      return nullptr;

    auto nwnote = dynamic_cast<Note *>(nw);
    if (!nwnote)
      return nullptr;

    nwnote->setText(note->text());
  }

  if (nw) {
    if (isOfType(w, "MultiLayer")) {
      if (status == MdiSubWindow::Maximized)
        nw->showMaximized();
    } else if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(nw);
      if (!g3d)
        return nullptr;
      g3d->setIgnoreFonts(true);
      if (status != MdiSubWindow::Maximized) {
        nw->resize(w->size());
        nw->showNormal();
      } else
        nw->showMaximized();
      g3d->setIgnoreFonts(false);
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

void ApplicationWindow::undo() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto note = dynamic_cast<Note *>(w);
  auto matrix = dynamic_cast<Matrix *>(w);

  if (note)
    note->editor()->undo();
  else if (matrix) {
    QUndoStack *stack = matrix->undoStack();
    if (stack && stack->canUndo())
      stack->undo();
  }
  QApplication::restoreOverrideCursor();
}

void ApplicationWindow::redo() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto note = dynamic_cast<Note *>(w);
  auto matrix = dynamic_cast<Matrix *>(w);

  if (note)
    note->editor()->redo();
  else if (matrix) {
    QUndoStack *stack = matrix->undoStack();
    if (stack && stack->canRedo())
      stack->redo();
  }

  QApplication::restoreOverrideCursor();
}

bool ApplicationWindow::hidden(QWidget *window) {
  return hiddenWindows->contains(window);
}

void ApplicationWindow::updateWindowStatus(MdiSubWindow *w) {
  setListView(w->objectName(), w->aspect());
  if (w->status() == MdiSubWindow::Maximized) {
    QList<MdiSubWindow *> windows = currentFolder()->windowsList();
    foreach (MdiSubWindow *oldMaxWindow, windows) {
      if (oldMaxWindow != w &&
          oldMaxWindow->status() == MdiSubWindow::Maximized)
        oldMaxWindow->setStatus(MdiSubWindow::Normal);
    }
  }
  modifiedProject();
}

void ApplicationWindow::hideActiveWindow() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  hideWindow(w);
}

void ApplicationWindow::hideWindow(MdiSubWindow *w) {
  hiddenWindows->append(w);
  w->setHidden();
  activateNewWindow();
  emit modified();
}

void ApplicationWindow::hideWindow() {
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
  if (!it)
    return;

  MdiSubWindow *w = it->window();
  if (!w)
    return;

  hideWindow(w);
}

void ApplicationWindow::resizeActiveWindow() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  ImageDialog *id = new ImageDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect(id, SIGNAL(setGeometry(int, int, int, int)), this,
          SLOT(setWindowGeometry(int, int, int, int)));

  id->setWindowTitle(tr("MantidPlot - Window Geometry")); // Mantid
  id->setOrigin(w->pos());
  id->setSize(w->size());
  id->exec();
}

void ApplicationWindow::resizeWindow() {
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
  if (!it)
    return;

  MdiSubWindow *w = it->window();
  if (!w)
    return;

  ImageDialog *id = new ImageDialog(this);
  id->setAttribute(Qt::WA_DeleteOnClose);
  connect(id, SIGNAL(setGeometry(int, int, int, int)), this,
          SLOT(setWindowGeometry(int, int, int, int)));

  id->setWindowTitle(tr("MantidPlot - Window Geometry")); // Mantid
  id->setOrigin(w->pos());
  id->setSize(w->size());
  id->exec();
}

void ApplicationWindow::setWindowGeometry(int x, int y, int w, int h) {
  activeWindow()->setGeometry(x, y, w, h);
}

/**
 * Checks if a mdi sub-window exists.
 */
bool ApplicationWindow::existsWindow(MdiSubWindow *w) const {
  if (!w)
    return false;
  FloatingWindow *fw = w->getFloatingWindow();
  if (fw && m_floatingWindows.contains(fw)) {
    return true;
  }
  QMdiSubWindow *sw = w->getDockedWindow();
  return sw && d_workspace->subWindowList().contains(sw);
}

/**
 * Returns the active sub-window
 */
MdiSubWindow *ApplicationWindow::getActiveWindow() const {
  if (!existsWindow(d_active_window)) {
    d_active_window = nullptr;
  }
  return d_active_window;
}

/**
 * Sets internal pointer to a new active sub-window.
 */
void ApplicationWindow::setActiveWindow(MdiSubWindow *w) {
  d_active_window = w;
  if (!existsWindow(d_active_window)) {
    d_active_window = nullptr;
  } else {
    // This make sure that we don't have two versions of current active window
    // (d_active_window and
    // active window of MdiArea) and they are either equal (when docked window
    // is active> or the
    // latter one is NULL (when floating window is active).
    if (d_active_window->getFloatingWindow()) {
      // If floating window is activated, we set MdiArea to not have any
      // active sub-window.
      d_workspace->setActiveSubWindow(nullptr);
    } else if (QMdiSubWindow *w = d_active_window->getDockedWindow()) {
      // If docked window activated, activate it in MdiArea as well.
      d_workspace->setActiveSubWindow(w);
    }
  }
}

void ApplicationWindow::activateWindow() {
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
  if (it)
    activateWindow(it->window());
}

/**
 * Activate a new MdiSubWindow: update the menu, tool bars, and folders.
 * @param w :: Subwindow to activate.
 */
void ApplicationWindow::activateWindow(MdiSubWindow *w,
                                       bool activateOuterWindow) {

  if (blockWindowActivation)
    return;

  if (!w) {
    setActiveWindow(nullptr);
    customMenu(nullptr);
    return;
  }

  // don't activat a window twice, but make sure it is visible
  if (getActiveWindow() == w) {
    // this can happen
    if (w->status() == MdiSubWindow::Minimized ||
        w->status() == MdiSubWindow::Hidden) {
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
  if (d_opening_file) {
    return;
  }

  // return any non-active QMdiSubWindows to normal so that the active could
  // be seen
  QMdiSubWindow *qw = dynamic_cast<QMdiSubWindow *>(w->parent());
  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  foreach (MdiSubWindow *ow, windows) {
    QMdiSubWindow *qww = dynamic_cast<QMdiSubWindow *>(ow->parent());
    if (qww && qww != qw && qww->isMaximized()) {
      ow->setNormal();
      break;
    }
  }

  blockWindowActivation = true;
  FloatingWindow *fw = w->getFloatingWindow();
  if (fw) {
    if (activateOuterWindow) {
      if (fw->isMaximized()) {
        w->setMaximized();
      } else {
        w->setNormal();
      }
    }
  } else {
    QMainWindow::activateWindow();
    w->setNormal();
  }
  blockWindowActivation = false;

  emit modified();
}

void ApplicationWindow::activateWindow(QTreeWidgetItem *lbi) {
  if (!lbi)
    lbi = lv->currentItem();

  if (!lbi)
    return;

  auto wli = dynamic_cast<WindowListItem *>(lbi);
  if (wli)
    activateWindow(wli->window());
}

void ApplicationWindow::maximizeWindow(QTreeWidgetItem *lbi) {
  if (!lbi)
    lbi = lv->currentItem();

  if (!lbi)
    return;

  auto wli = dynamic_cast<WindowListItem *>(lbi);
  if (wli)
    maximizeWindow(wli->window());
}

void ApplicationWindow::maximizeWindow(MdiSubWindow *w) {
  if (!w || w->status() == MdiSubWindow::Maximized)
    return;

  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  foreach (MdiSubWindow *ow, windows) {
    if (ow != w && ow->status() == MdiSubWindow::Maximized) {
      ow->setNormal();
      break;
    }
  }

  w->setMaximized();
  updateWindowLists(w);
  emit modified();
}

void ApplicationWindow::minimizeWindow(MdiSubWindow *w) {
  auto wli = dynamic_cast<WindowListItem *>(lv->currentItem());

  if (!wli)
    return;

  w = wli->window();

  if (!w)
    return;

  updateWindowLists(w);
  w->setMinimized();
  emit modified();
}

void ApplicationWindow::updateWindowLists(MdiSubWindow *w) {
  if (!w)
    return;

  if (hiddenWindows->contains(w))
    hiddenWindows->takeAt(hiddenWindows->indexOf(w));
}

void ApplicationWindow::closeActiveWindow() {
  MdiSubWindow *w = activeWindow();
  if (w)
    w->close();
}

void ApplicationWindow::closeSimilarWindows() {
  std::string windowType = activeWindow()->getWindowType();

  QMessageBox::StandardButton pressed = QMessageBox::question(
      this, "MantidPlot",
      QString::fromStdString("All " + windowType +
                             " windows will be removed. Are you sure?"),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

  if (pressed != QMessageBox::Ok)
    return;

  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  for (auto win : windows) {
    if (win->getWindowType() == windowType) {
      win->close();
    }
  }
}

void ApplicationWindow::removeWindowFromLists(MdiSubWindow *w) {
  if (!w)
    return;

  QString caption = w->objectName();
  if (w->inherits("Table")) {
    Table *m = dynamic_cast<Table *>(w);
    if (!m)
      return;
    for (int i = 0; i < m->numCols(); i++) {
      QString name = m->colName(i);
      removeCurves(name);
    }
  } else if (isOfType(w, "MultiLayer")) {
    MultiLayer *ml = dynamic_cast<MultiLayer *>(w);
    if (!ml)
      return;
    Graph *g = ml->activeGraph();
    if (!g)
      return;
    btnPointer->setChecked(true);
  } else if (isOfType(w, "Matrix")) {
    auto matrix = dynamic_cast<Matrix *>(w);
    if (matrix)
      remove3DMatrixPlots(matrix);
  }

  if (hiddenWindows->contains(w)) {
    hiddenWindows->takeAt(hiddenWindows->indexOf(w));
  }
}

void ApplicationWindow::closeWindow(MdiSubWindow *window) {
  if (!window)
    return;

  if (getActiveWindow() == window) {
    activateNewWindow();
  }
  removeWindowFromLists(window);

  // update list view in project explorer
  auto found = lv->findItems(window->objectName(),
                             Qt::MatchExactly | Qt::MatchCaseSensitive, 0);

  if (!found.isEmpty())
    lv->takeTopLevelItem(lv->indexOfTopLevelItem(found[0]));

  if (show_windows_policy == ActiveFolder) {
    // the old code here relied on currentFolder() to remove its reference to
    // window
    // before the call to this method
    // the following check makes it work in any case
    int cnt = currentFolder()->windowsList().count();
    if (cnt == 0 || (cnt == 1 && currentFolder()->windowsList()[0] == window)) {
      customMenu(nullptr);
      customToolBars(nullptr);
    }
  } else if (show_windows_policy == SubFolders &&
             !(currentFolder()->children()).isEmpty()) {
    FolderListItem *fi = currentFolder()->folderListItem();
    FolderListItem *item = dynamic_cast<FolderListItem *>(fi->child(0));
    bool emptyFolder = true;
    while (item) {
      QList<MdiSubWindow *> lst = item->folder()->windowsList();
      if (lst.count() > 0) {
        emptyFolder = false;
        break;
      }
      item = dynamic_cast<FolderListItem *>(lv->itemBelow(item));
    }
    if (emptyFolder) {
      customMenu(nullptr);
      customToolBars(nullptr);
    }
  }
  emit modified();
}

/** Add a serialisable window to the application
 * @param window :: the window to add
 */
void ApplicationWindow::addSerialisableWindow(QObject *window) {
  // Here we must store the window as a QObject to avoid multiple inheritance
  // issues with Qt and the IProjectSerialisable class as well as being able
  // to respond to the destroyed signal
  // We can still check here that the window conforms to the interface and
  // discard it if it does not.
  if (!dynamic_cast<IProjectSerialisable *>(window))
    return;

  m_serialisableWindows.push_back(window);
  // Note that destroyed is emitted directly before the QObject itself
  // is destroyed. This means the destructor of the specific window type
  // will have already been called.
  connect(window, SIGNAL(destroyed(QObject *)), this,
          SLOT(removeSerialisableWindow(QObject *)));
}

/** Remove a serialisable window from the application
 * @param window :: the window to remove
 */
void ApplicationWindow::removeSerialisableWindow(QObject *window) {
  if (m_serialisableWindows.contains(window)) {
    m_serialisableWindows.removeAt(m_serialisableWindows.indexOf(window));
  }
}

void ApplicationWindow::about() {
  MantidAbout *ma = new MantidAbout();
  ma->exec();
  delete ma;
}

void ApplicationWindow::analysisMenuAboutToShow() {
  analysisMenu->clear();
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (isOfType(w, "MultiLayer")) {
    analysisMenu->addAction(actionDifferentiate);
    analysisMenu->addAction(actionIntegrate);
    analysisMenu->addAction(actionShowIntDialog);
    analysisMenu->addSeparator();

    smoothMenu->clear();
    smoothMenu = analysisMenu->addMenu(tr("&Smooth"));
    smoothMenu->addAction(actionSmoothSavGol);
    smoothMenu->addAction(actionSmoothAverage);
    smoothMenu->addAction(actionSmoothFFT);

    filterMenu->clear();
    filterMenu = analysisMenu->addMenu(tr("&FFT filter"));
    filterMenu->addAction(actionLowPassFilter);
    filterMenu->addAction(actionHighPassFilter);
    filterMenu->addAction(actionBandPassFilter);
    filterMenu->addAction(actionBandBlockFilter);

    analysisMenu->addSeparator();
    analysisMenu->addAction(actionInterpolate);
    analysisMenu->addAction(actionFFT);
    analysisMenu->addSeparator();
    analysisMenu->addAction(actionFitLinear);
    analysisMenu->addAction(actionShowFitPolynomDialog);
    analysisMenu->addSeparator();

    decayMenu->clear();
    decayMenu = analysisMenu->addMenu(tr("Fit E&xponential Decay"));
    decayMenu->addAction(actionShowExpDecayDialog);
    decayMenu->addAction(actionShowTwoExpDecayDialog);
    decayMenu->addAction(actionShowExpDecay3Dialog);

    analysisMenu->addAction(actionFitExpGrowth);
    analysisMenu->addAction(actionFitSigmoidal);
    analysisMenu->addAction(actionFitGauss);
    analysisMenu->addAction(actionFitLorentz);

    analysisMenu->addSeparator();
    analysisMenu->addAction(actionShowFitDialog);
  } else if (isOfType(w, "Matrix")) {
    analysisMenu->addAction(actionIntegrate);
    analysisMenu->addSeparator();
    analysisMenu->addAction(actionFFT);
    analysisMenu->addAction(actionMatrixFFTDirect);
    analysisMenu->addAction(actionMatrixFFTInverse);
  } else if (w->inherits("Table")) {
    analysisMenu->addAction(actionShowColStatistics);
    analysisMenu->addAction(actionShowRowStatistics);
    analysisMenu->addSeparator();
    if (isOfType(w, "Table")) {
      analysisMenu->addAction(actionSortSelection);
    }
    analysisMenu->addAction(actionSortTable);

    normMenu->clear();
    normMenu = analysisMenu->addMenu(tr("&Normalize"));
    normMenu->addAction(actionNormalizeSelection);
    normMenu->addAction(actionNormalizeTable);

    analysisMenu->addSeparator();
    analysisMenu->addAction(actionFFT);
    analysisMenu->addSeparator();
    analysisMenu->addAction(actionCorrelate);
    analysisMenu->addAction(actionAutoCorrelate);
    analysisMenu->addSeparator();
    analysisMenu->addAction(actionConvolute);
    analysisMenu->addAction(actionDeconvolute);
    analysisMenu->addSeparator();
    analysisMenu->addAction(actionShowFitDialog);
  }
  reloadCustomActions();
}

void ApplicationWindow::matrixMenuAboutToShow() {
  matrixMenu->clear();
  matrixMenu->addAction(actionSetMatrixProperties);
  matrixMenu->addAction(actionSetMatrixDimensions);
  matrixMenu->addSeparator();
  matrixMenu->addAction(actionSetMatrixValues);
  matrixMenu->addAction(actionTableRecalculate);
  matrixMenu->addSeparator();
  matrixMenu->addAction(actionRotateMatrix);
  matrixMenu->addAction(actionRotateMatrixMinus);
  matrixMenu->addAction(actionFlipMatrixVertically);
  matrixMenu->addAction(actionFlipMatrixHorizontally);
  matrixMenu->addSeparator();
  matrixMenu->addAction(actionTransposeMatrix);
  matrixMenu->addAction(actionInvertMatrix);
  matrixMenu->addAction(actionMatrixDeterminant);
  matrixMenu->addSeparator();
  matrixMenu->addAction(actionGoToRow);
  matrixMenu->addAction(actionGoToColumn);
  matrixMenu->addSeparator();
  QMenu *matrixViewMenu = matrixMenu->addMenu(tr("Vie&w"));
  matrixViewMenu->addAction(actionViewMatrixImage);
  matrixViewMenu->addAction(actionViewMatrix);
  QMenu *matrixPaletteMenu = matrixMenu->addMenu(tr("&Palette"));
  matrixPaletteMenu->addAction(actionMatrixGrayScale);
  matrixPaletteMenu->addAction(actionMatrixRainbowScale);
  matrixPaletteMenu->addAction(actionMatrixCustomScale);
  matrixMenu->addSeparator();
  matrixMenu->addAction(actionMatrixColumnRow);
  matrixMenu->addAction(actionMatrixXY);
  matrixMenu->addSeparator();
  QMenu *convertToTableMenu =
      matrixMenu->addMenu(tr("&Convert to Spreadsheet"));
  convertToTableMenu->addAction(actionConvertMatrixDirect);
  convertToTableMenu->addAction(actionConvertMatrixXYZ);
  convertToTableMenu->addAction(actionConvertMatrixYXZ);

  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
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

void ApplicationWindow::fileMenuAboutToShow() {
  fileMenu->clear();
  newMenu->clear();
  exportPlotMenu->clear();

  newMenu = fileMenu->addMenu(tr("&New"));
  newMenu->addAction(actionNewProject);
  newMenu->addAction(actionNewTable);
  newMenu->addAction(actionNewMatrix);
  newMenu->addAction(actionNewNote);
  newMenu->addAction(actionNewGraph);
  newMenu->addAction(actionNewFunctionPlot);
  newMenu->addAction(actionNewSurfacePlot);
  newMenu->addAction(actionNewTiledWindow);

  openMenu = fileMenu->addMenu(tr("&Load"));
  openMenu->addAction(actionOpenProj);
  openMenu->addAction(actionLoadFile);

  auto recentProjectsMenuAction = fileMenu->addMenu(recentProjectsMenu);
  recentProjectsMenuAction->setText(tr("&Recent Projects"));

  auto recentFilesMenuAction = fileMenu->addMenu(recentFilesMenu);
  recentFilesMenuAction->setText(tr("R&ecent Files"));

  fileMenu->addSeparator();
  fileMenu->addAction(actionManageDirs);
  fileMenu->addSeparator();
  fileMenu->addAction(actionLoadImage);
  fileMenu->addAction(actionScriptRepo);

  MdiSubWindow *w = activeWindow();

  if (w && isOfType(w, "Matrix"))
    fileMenu->addAction(actionExportMatrix);

  fileMenu->addSeparator();
  fileMenu->addAction(actionSaveProjectAs);

  saveMenu = fileMenu->addMenu(tr("&Save"));
  saveMenu->addAction(actionSaveFile);
  saveMenu->addAction(actionSaveProject);

  fileMenu->addSeparator();

  fileMenu->addAction(actionPrint);
  fileMenu->addAction(actionPrintAllPlots);
  fileMenu->addSeparator();
  MdiSubWindow *t = activeWindow();

  if (t && (isOfType(t, "Matrix") || isOfType(t, "Table") ||
            isOfType(t, "MantidMatrix"))) {
    actionShowExportASCIIDialog->setEnabled(true);
  } else {
    actionShowExportASCIIDialog->setEnabled(false);
  }

  fileMenu->addAction(actionShowExportASCIIDialog);
  fileMenu->addAction(actionLoad);
  fileMenu->addSeparator();
  fileMenu->addAction(actionclearAllMemory);

  fileMenu->addSeparator();
  fileMenu->addAction(actionCloseAllWindows);

  reloadCustomActions();
}

void ApplicationWindow::editMenuAboutToShow() { reloadCustomActions(); }

/**
 * Setup Windows menu.
 */
void ApplicationWindow::windowsMenuAboutToShow() {
  windowsMenu->clear();

  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  int n = static_cast<int>(windows.count());
  if (!n) {
    return;
  }

  windowsMenu->addAction(tr("&Cascade"), this, SLOT(cascade()));
  windowsMenu->addAction(tr("&Tile"), this, SLOT(tileMdiWindows()));
  windowsMenu->addSeparator();
  windowsMenu->addAction(actionNextWindow);
  windowsMenu->addAction(actionPrevWindow);
  windowsMenu->addSeparator();
  windowsMenu->addAction(actionRename);

  windowsMenu->addAction(actionCopyWindow);
  MdiSubWindow *activeWin = activeWindow();
  if (!activeWin)
    return;

  if (isOfType(activeWin, "MantidMatrix") ||
      isOfType(activeWin, "InstrumentWindow")) {
    actionCopyWindow->setEnabled(false);
  } else {
    actionCopyWindow->setEnabled(true);
  }

  windowsMenu->addSeparator();

  windowsMenu->addAction(actionResizeActiveWindow);
  if (activeWin->getFloatingWindow()) {
    windowsMenu->addAction(tr("Change to docked"), this,
                           SLOT(changeActiveToDocked()));
  } else {
    windowsMenu->addAction(tr("Change to floating"), this,
                           SLOT(changeActiveToFloating()));
  }
  windowsMenu->addAction(tr("&Hide Window"), this, SLOT(hideActiveWindow()));

// Having the shortcut set here is necessary on Windows, but
// leads to an error message elsewhere. Don't know why and don't
// have a better solution than this right now.
#ifdef _WIN32
  windowsMenu->addAction(getQPixmap("close_xpm"), tr("Close &Window"), this,
                         SLOT(closeActiveWindow()), Qt::CTRL + Qt::Key_W);
#else
  windowsMenu->addAction(getQPixmap("close_xpm"), tr("Close &Window"), this,
                         SLOT(closeActiveWindow()));
#endif

  // Add an option to close all windows of a similar type
  std::string windowType = activeWin->getWindowType();
  // count the number of similar windows
  int winTypeCount = 0;
  for (auto win : windows) {
    if (win->getWindowType() == windowType) {
      winTypeCount++;
    }
  }
  if (winTypeCount > 1) {
#ifdef _WIN32
    windowsMenu->addAction(
        getQPixmap("close_xpm"),
        QString::fromStdString("Close All " + windowType + " Windows"), this,
        SLOT(closeSimilarWindows()), Qt::CTRL + Qt::SHIFT + Qt::Key_W);
#else
    windowsMenu->addAction(
        getQPixmap("close_xpm"),
        QString::fromStdString("Close All " + windowType + " Windows"), this,
        SLOT(closeSimilarWindows()));
#endif
  }

  if (n > 0 && n < 10) {
    windowsMenu->addSeparator();
    for (int i = 0; i < n; ++i) {
      auto activated = windowsMenu->addAction(windows.at(i)->objectName(), this,
                                              SLOT(windowsMenuActivated()));
      activated->setData(i);
      auto isChecked = currentFolder()->activeWindow() == windows.at(i);
      activated->setChecked(isChecked);
    }
  } else if (n >= 10) {
    windowsMenu->addSeparator();
    for (int i = 0; i < 9; ++i) {
      auto activated = windowsMenu->addAction(windows.at(i)->objectName(), this,
                                              SLOT(windowsMenuActivated()));
      activated->setData(i);
      auto isChecked = activeWindow() == windows.at(i);
      activated->setChecked(isChecked);
    }
    windowsMenu->addSeparator();
    windowsMenu->addAction(tr("More windows..."), this,
                           SLOT(showMoreWindows()));
  }
  reloadCustomActions();
}

namespace // anonymous
{
/**
 * Helper function used with Qt's qSort to make sure interfaces are in
 * alphabetical order.
 */
bool interfaceNameComparator(const QPair<QString, QString> &lhs,
                             const QPair<QString, QString> &rhs) {
  return lhs.first.toLower() < rhs.first.toLower();
}
} // anonymous namespace

void ApplicationWindow::interfaceMenuAboutToShow() {
  interfaceMenu->clear();
  m_interfaceActions.clear();

  // Create a submenu for each category.  Make sure submenus are in
  // alphabetical order, and ignore any hidden categories.
  const QString hiddenProp = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "interfaces.categories.hidden"));
  auto hiddenCategories =
      hiddenProp.split(";", QString::SkipEmptyParts).toSet();
  QMap<QString, QMenu *> categoryMenus;
  auto sortedCategories = m_allCategories.toList();
  qSort(sortedCategories);
  foreach (const QString category, sortedCategories) {
    if (hiddenCategories.contains(category))
      continue;
    QMenu *categoryMenu = new QMenu(interfaceMenu);
    categoryMenu->setObjectName(category + "Menu");
    auto categoryMenuAction = interfaceMenu->addMenu(categoryMenu);
    categoryMenuAction->setText(category);
    categoryMenus[category] = categoryMenu;
  }

  // Show the interfaces in alphabetical order in their respective submenus.
  qSort(m_interfaceNameDataPairs.begin(), m_interfaceNameDataPairs.end(),
        interfaceNameComparator);

  // Turn the name/data pairs into QActions with which we populate the menus.
  foreach (const auto interfaceNameDataPair, m_interfaceNameDataPairs) {
    const QString name = interfaceNameDataPair.first;
    const QString data = interfaceNameDataPair.second;

    foreach (const QString category, m_interfaceCategories[name]) {
      if (!categoryMenus.contains(category))
        continue;
      QAction *openInterface = new QAction(interfaceMenu);
      openInterface->setObjectName(name);
      openInterface->setText(name);
      openInterface->setData(data);
      categoryMenus[category]->addAction(openInterface);

      // Update separate list containing all interface actions.
      m_interfaceActions.append(openInterface);
    }
  }

  foreach (auto categoryMenu, categoryMenus.values()) {
    connect(categoryMenu, SIGNAL(triggered(QAction *)), this,
            SLOT(performCustomAction(QAction *)));
  }

  interfaceMenu->addSeparator();

  // Allow user to customise categories.
  QAction *customiseCategoriesAction =
      new QAction(tr("Add/Remove Categories"), this);
  connect(customiseCategoriesAction, SIGNAL(triggered()), this,
          SLOT(showInterfaceCategoriesDialog()));
  interfaceMenu->addAction(customiseCategoriesAction);
}

void ApplicationWindow::tiledWindowMenuAboutToShow() {
  tiledWindowMenu->clear();
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;
  TiledWindow *tw = dynamic_cast<TiledWindow *>(w);
  if (!tw)
    return;
  tw->populateMenu(tiledWindowMenu);
}

void ApplicationWindow::showMarkerPopupMenu() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  QMenu markerMenu(this);

  if (g->imageMarkerSelected()) {
    markerMenu.addAction(getQPixmap("pixelProfile_xpm"),
                         tr("&View Pixel Line profile"), this,
                         SLOT(pixelLineProfile()));
    markerMenu.addAction(tr("&Intensity Matrix"), this, SLOT(intensityTable()));
    markerMenu.addSeparator();
  }

  if (!(g->activeTool() && dynamic_cast<PeakPickerTool *>(g->activeTool()))) {
    markerMenu.addAction(getQPixmap("cut_xpm"), tr("&Cut"), this,
                         SLOT(cutSelection()));
    markerMenu.addAction(getQPixmap("copy_xpm"), tr("&Copy"), this,
                         SLOT(copySelection()));
  }

  markerMenu.addAction(getQPixmap("erase_xpm"), tr("&Delete"), this,
                       SLOT(clearSelection()));
  markerMenu.addSeparator();
  if (g->arrowMarkerSelected())
    markerMenu.addAction(tr("&Properties..."), this, SLOT(showLineDialog()));
  else if (g->imageMarkerSelected())
    markerMenu.addAction(tr("&Properties..."), this, SLOT(showImageDialog()));
  else
    markerMenu.addAction(tr("&Properties..."), this, SLOT(showTextDialog()));

  markerMenu.exec(QCursor::pos());
}

void ApplicationWindow::showMoreWindows() {
  if (explorerWindow->isVisible())
    QMessageBox::information(
        this, "MantidPlot",
        tr("Please use the project explorer to select a window!")); // Mantid
  else
    explorerWindow->show();
}

void ApplicationWindow::windowsMenuActivated() {
  QList<MdiSubWindow *> windows = currentFolder()->windowsList();
  auto obj = sender();
  auto action = qobject_cast<QAction *>(obj);
  auto id = action->data().toInt();
  MdiSubWindow *w = windows.at(id);
  if (w) {
    this->activateWindow(w);
  }
}

void ApplicationWindow::foldersMenuActivated(int id) {
  int folder_param = 0;
  Folder *f = projectFolder();
  while (f) {
    if (folder_param == id) {
      changeFolder(f);
      return;
    }

    folder_param++;
    f = f->folderBelow();
  }
}

void ApplicationWindow::newProject(const bool doNotSave) {

  if (doNotSave) {
    // Save anything we need to
    saveSettings();
    mantidUI->saveProject(saved);
  }

  // Clear out any old folders
  folders->blockSignals(true);
  lv->blockSignals(true);

  folders->clear();
  lv->clear();

  d_current_folder = new Folder(nullptr, tr("untitled"));
  FolderListItem *fli = new FolderListItem(folders, d_current_folder);
  d_current_folder->setFolderListItem(fli);
  fli->setExpanded(true);

  lv->blockSignals(false);
  folders->blockSignals(false);

  // Reset everything else
  setWindowTitle(tr("MantidPlot - untitled")); // Mantid
  projectname = "untitled";

  if (actionSaveProject)
    actionSaveProject->setEnabled(false);
}

void ApplicationWindow::savedProject() {
  QCoreApplication::processEvents();
  if (actionSaveFile)
    actionSaveFile->setEnabled(false);
  if (actionSaveProject)
    actionSaveProject->setEnabled(false);
  saved = true;

  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (isOfType(w, "Matrix")) {
        Matrix *m = dynamic_cast<Matrix *>(w);
        if (m)
          m->undoStack()->setClean();
      }
    }
    f = f->folderBelow();
  }
}

void ApplicationWindow::modifiedProject() {
  if (!saved)
    return;
  // enable actionSaveProject, but not actionSaveFile (which is Save Nexus and
  // doesn't
  // seem to make sense for qti objects (graphs, tables, matrices, notes,
  // etc.)
  if (actionSaveProject)
    actionSaveProject->setEnabled(true);
  if (actionSaveProjectAs)
    actionSaveProjectAs->setEnabled(true);
  saved = false;
}

void ApplicationWindow::modifiedProject(MdiSubWindow * /*unused*/) {
  modifiedProject();
}

void ApplicationWindow::timerEvent(QTimerEvent *e) {
  if (e->timerId() == savingTimerId)
    saveProject();
  else
    QWidget::timerEvent(e);
}

void ApplicationWindow::dropEvent(QDropEvent *e) { mantidUI->drop(e); }

void ApplicationWindow::dragEnterEvent(QDragEnterEvent *e) {
  if (e->source()) {
    e->setAccepted(mantidUI->canAcceptDrop(e));
    return;
  }
  e->ignore();
}

// Mantid
void ApplicationWindow::dragMoveEvent(QDragMoveEvent *e) {
  if (centralWidget()->geometry().contains(e->pos()))
    e->accept();
  else
    e->ignore();
}

void ApplicationWindow::closeEvent(QCloseEvent *ce) {
  if (scriptingWindow && scriptingWindow->isExecuting()) {
    if (!(QMessageBox::question(
              this, tr("MantidPlot"),
              "A script is still running, abort and quit application?",
              tr("Yes"), tr("No")) == 0)) {
      ce->ignore();
      return;
    }
    // We used to cancel running algorithms here (if 'Yes' to the above
    // question), but now that
    // happens in MantidUI::shutdown (called below) because we want it
    // regardless of whether a
    // script is running.
  }

  if (!saved) {
    QString savemsg =
        tr("Save changes to project: <p><b> %1 </b> ?").arg(projectname);
    int result =
        QMessageBox::information(this, tr("MantidPlot"), savemsg, tr("Yes"),
                                 tr("No"), tr("Cancel"), 0, 2);
    if (result == 0) {
      auto response = execSaveProjectDialog();
      if (response != QDialog::Accepted) {
        ce->ignore();
        return;
      }
    } else if (result == 2) {
      // User wanted to cancel, do nothing
      ce->ignore();
      return;
    }
  }

  if (m_projectRecoveryRunOnStart) {
    // Stop background saving thread, so it doesn't try to use a destroyed
    // resource
    m_projectRecovery.stopProjectSaving();
    m_projectRecovery.clearAllCheckpoints(
        Poco::Path(m_projectRecovery.getRecoveryFolderOutputPR()));
  }

  // Close the remaining MDI windows. The Python API is required to be active
  // when the MDI window destructor is called so that those references can be
  // cleaned up meaning we cannot rely on the deleteLater functionality to
  // work correctly as this will happen in the next iteration of the event
  // loop, i.e after the python shutdown code has been run below.
  m_shuttingDown = true;

  MDIWindowList windows = getAllWindows();
  for (auto &win : windows) {
    win->confirmClose(false);
    win->setAttribute(Qt::WA_DeleteOnClose, false);
    win->close();
    delete win;
  }

  mantidUI->shutdown();
  if (catalogSearch) {
    catalogSearch->disconnect();
  }

  if (scriptingWindow) {
    scriptingWindow->disconnect();
    this->showScriptWindow(true);
    // Other specific settings
    scriptingWindow->saveSettings();
    scriptingWindow->acceptCloseEvent(true);
    scriptingWindow->close();
    delete scriptingWindow;
    scriptingWindow = nullptr;
  }
  // Ensure all python references are cleaned up before the interpreter shuts
  // down
  delete m_iface_script;
  delete m_interpreterDock;

  // Emit a shutting_down() signal that can be caught by
  // independent QMainWindow objects to know when MantidPlot
  // is shutting down.
  emit shutting_down();

  // Save the settings and exit
  saveSettings();
  scriptingEnv()->finalize();

  ce->accept();
  qApp->closeAllWindows();

  // Delete the file finding thread pool if it still exists.
  MantidQt::API::FindFilesThreadPoolManager::destroyThreadPool();
}

void ApplicationWindow::customEvent(QEvent *e) {
  if (!e)
    return;

  if (e->type() == SCRIPTING_CHANGE_EVENT) {
    auto se = dynamic_cast<ScriptingChangeEvent *>(e);
    if (se)
      scriptingChangeEvent(se);
  }
}

void ApplicationWindow::deleteSelectedItems() {
  if (folders->hasFocus() &&
      folders->currentItem() != folders->firstChild()) { // we never allow the
                                                         // user to delete the
                                                         // project folder item
    deleteFolder();
    return;
  }

  QTreeWidgetItem *item;
  QList<QTreeWidgetItem *> lst;
  for (item = lv->firstChild(); item; item = lv->itemBelow(item)) {
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach (item, lst) {
    auto wli = dynamic_cast<WindowListItem *>(item);
    if (wli)
      wli->window()->close();
  }
  folders->blockSignals(false);
}

void ApplicationWindow::showListViewSelectionMenu(const QPoint &p) {
  QMenu cm(this);
  cm.addAction(tr("&Show All Windows"), this, SLOT(showSelectedWindows()));
  cm.addAction(tr("&Hide All Windows"), this, SLOT(hideSelectedWindows()));
  cm.addSeparator();
  cm.addAction(tr("&Delete Selection"), this, SLOT(deleteSelectedItems()),
               Qt::Key_F8);
  cm.exec(lv->mapToGlobal(p));
}

void ApplicationWindow::showListViewPopupMenu(const QPoint &p) {
  QMenu cm(this);

  auto window = cm.addMenu(tr("New &Window"));
  window->addAction(actionNewTable);
  window->addAction(actionNewMatrix);
  window->addAction(actionNewNote);
  window->addAction(actionNewGraph);
  window->addAction(actionNewFunctionPlot);
  window->addAction(actionNewSurfacePlot);
  window->addAction(actionNewTiledWindow);

  cm.addSeparator();
  cm.addAction(tr("Auto &Column Width"), lv, SLOT(adjustColumns()));
  cm.exec(lv->mapToGlobal(p));
}

void ApplicationWindow::showWindowPopupMenu(const QPoint &p) {

  auto it = lv->itemAt(p);

  if (!it) {
    showListViewPopupMenu(p);
    return;
  }

  QTreeWidgetItem *item;
  int selected = 0;
  for (item = lv->firstChild(); item; item = lv->itemBelow(item)) {
    if (item->isSelected())
      selected++;

    if (selected > 1) {
      showListViewSelectionMenu(p);
      return;
    }
  }

  auto fli = dynamic_cast<FolderListItem *>(it);
  if (fli) {
    d_current_folder = fli->folder();
    showFolderPopupMenu(it, p, false);
    return;
  }

  auto wli = dynamic_cast<WindowListItem *>(it);
  if (!wli)
    return;

  MdiSubWindow *w = wli->window();
  if (w) {
    QMenu cm(this);

    cm.addAction(actionActivateWindow);
    cm.addAction(actionMinimizeWindow);
    cm.addAction(actionMaximizeWindow);
    cm.addSeparator();
    if (!hidden(w))
      cm.addAction(actionHideWindow);
    cm.addAction(getQPixmap("close_xpm"), tr("&Delete Window"), w,
                 SLOT(close()), Qt::Key_F8);
    cm.addSeparator();
    cm.addAction(tr("&Rename Window"), this, SLOT(renameWindow()), Qt::Key_F2);
    cm.addAction(actionResizeWindow);
    cm.addSeparator();
    cm.addAction(getQPixmap("fileprint_xpm"), tr("&Print Window"), w,
                 SLOT(print()));
    cm.addSeparator();
    cm.addAction(tr("&Properties..."), this, SLOT(windowProperties()));

    if (w->inherits("Table")) {
      QStringList graphs = dependingPlots(w->objectName());
      if (static_cast<int>(graphs.count()) > 0) {
        cm.addSeparator();
        auto plots = cm.addMenu(tr("D&epending Graphs"));
        for (int i = 0; i < static_cast<int>(graphs.count()); i++)
          plots->addAction(graphs[i], window(graphs[i]), SLOT(showMaximized()));
      }
    } else if (isOfType(w, "Matrix")) {
      QStringList graphs = depending3DPlots(dynamic_cast<Matrix *>(w));
      if (static_cast<int>(graphs.count()) > 0) {
        cm.addSeparator();
        auto plots = cm.addMenu(tr("D&epending 3D Graphs"));
        for (int i = 0; i < static_cast<int>(graphs.count()); i++)
          plots->addAction(graphs[i], window(graphs[i]), SLOT(showMaximized()));
      }
    } else if (isOfType(w, "MultiLayer")) {
      tablesDepend->clear();
      QStringList tbls = multilayerDependencies(w);
      int n = static_cast<int>(tbls.count());
      if (n > 0) {
        cm.addSeparator();
        for (int i = 0; i < n; i++)
          tablesDepend->addAction(tbls[i]);
        auto tablesDependMenuAction = cm.addMenu(tablesDepend);
        tablesDependMenuAction->setText("D&epends on");
      }
    } else if (isOfType(w, "Graph3D")) {
      Graph3D *sp = dynamic_cast<Graph3D *>(w);
      if (!sp)
        return;
      Matrix *m = sp->matrix();
      QString formula = sp->formula();
      if (!formula.isEmpty()) {
        cm.addSeparator();
        if (formula.contains("_")) {
          QStringList tl = formula.split("_", QString::SkipEmptyParts);
          tablesDepend->clear();
          tablesDepend->addAction(tl[0]);
          auto tablesDependMenuAction = cm.addMenu(tablesDepend);
          tablesDependMenuAction->setText("D&epends on");
        } else if (m) {
          auto plots = cm.addMenu(tr("D&epends on"));
          plots->addAction(m->objectName(), m, SLOT(showNormal()));
        } else {
          auto plots = cm.addMenu(tr("Function"));
          plots->addAction(formula, w, SLOT(showNormal()));
        }
      }
    } else if (isOfType(w, "TiledWindow")) {
      std::cerr << "Menu for TiledWindow\n";
    }
    cm.exec(lv->mapToGlobal(p));
  }
}

void ApplicationWindow::showTable(QAction *selectedAction) {
  Table *t = table(selectedAction->text());
  if (!t)
    return;

  updateWindowLists(t);

  t->showMaximized();
  auto found = lv->findItems(t->objectName(),
                             Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(2, tr("Maximized"));
}

void ApplicationWindow::showTable(const QString &curve) {
  Table *w = table(curve);
  if (!w)
    return;

  updateWindowLists(w);
  int colIndex = w->colIndex(curve);
  w->setSelectedCol(colIndex);
  w->table()->clearSelection();
  w->table()->selectColumn(colIndex);
  w->showMaximized();
  auto found = lv->findItems(w->objectName(),
                             Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    found[0]->setText(2, tr("Maximized"));
  emit modified();
}

QStringList ApplicationWindow::depending3DPlots(Matrix *m) {
  QStringList plots;
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(w);
      if (g3d && g3d->matrix() == m)
        plots << w->objectName();
    }
  }
  return plots;
}

QStringList ApplicationWindow::dependingPlots(const QString &name) {
  QStringList onPlot, plots;

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "MultiLayer")) {
      auto ml = dynamic_cast<MultiLayer *>(w);
      if (!ml)
        return plots;
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers) {
        onPlot = g->curvesList();
        onPlot = onPlot.filter(name, Qt::CaseSensitive);
        if (static_cast<int>(onPlot.count()) &&
            !plots.contains(w->objectName()))
          plots << w->objectName();
      }
    } else if (isOfType(w, "Graph3D")) {
      auto g3d = dynamic_cast<Graph3D *>(w);
      if (g3d && (g3d->formula()).contains(name, Qt::CaseSensitive) &&
          !plots.contains(w->objectName()))
        plots << w->objectName();
    }
  }
  return plots;
}

QStringList ApplicationWindow::multilayerDependencies(QWidget *w) {
  QStringList tables;
  MultiLayer *g = dynamic_cast<MultiLayer *>(w);
  if (!g)
    return tables;

  QList<Graph *> layers = g->layersList();
  foreach (Graph *ag, layers) {
    QStringList onPlot = ag->curvesList();
    for (int j = 0; j < onPlot.count(); j++) {
      QStringList tl = onPlot[j].split("_", QString::SkipEmptyParts);
      if (!tables.contains(tl[0]))
        tables << tl[0];
    }
  }
  return tables;
}

void ApplicationWindow::showGraphContextMenu() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  QMenu cm(this);
  Graph *ag = dynamic_cast<Graph *>(plot->activeGraph());
  if (!ag)
    return;
  PlotToolInterface *tool = ag->activeTool();
  if (dynamic_cast<PeakPickerTool *>(tool)) {
    auto ppt = dynamic_cast<PeakPickerTool *>(tool);
    if (!ppt)
      return;
    ppt->prepareContextMenu(cm);
    cm.exec(QCursor::pos());
    return;
  }

  if (ag->isPiePlot())
    cm.addAction(tr("Re&move Pie Curve"), ag, SLOT(removePie()));
  else {
    if (ag->visibleCurves() != ag->curves()) {
      cm.addAction(actionShowAllCurves);
      cm.addSeparator();
    }
    cm.addAction(actionShowCurvesDialog);
    cm.addAction(actionAddFunctionCurve);
    if (m_enableQtiPlotFitting) {
      auto analysisMenuAction = cm.addMenu(analysisMenu);
      analysisMenuAction->setText(tr("Anal&yze"));
    }
  }

  if (lastCopiedLayer) {
    cm.addSeparator();
    cm.addAction(getQPixmap("paste_xpm"), tr("&Paste Layer"), this,
                 SLOT(pasteSelection()));
  } else if (d_text_copy) {
    cm.addSeparator();
    cm.addAction(getQPixmap("paste_xpm"), tr("&Paste Text"), plot,
                 SIGNAL(pasteMarker()));
  } else if (d_arrow_copy) {
    cm.addSeparator();
    cm.addAction(getQPixmap("paste_xpm"), tr("&Paste Line/Arrow"), plot,
                 SIGNAL(pasteMarker()));
  } else if (d_image_copy) {
    cm.addSeparator();
    cm.addAction(getQPixmap("paste_xpm"), tr("&Paste Image"), plot,
                 SIGNAL(pasteMarker()));
  }
  cm.addSeparator();

  auto axes = cm.addMenu(tr("&Axes"));
  axes->addAction(tr("Lo&g(x),Log(y)"), ag, SLOT(logLogAxes()));
  axes->addAction(tr("Log(&x),Linear(y)"), ag, SLOT(logXLinY()));
  axes->addAction(tr("Linear(x),Log(&y)"), ag, SLOT(logYlinX()));
  axes->addAction(tr("&Linear(x),Linear(y)"), ag, SLOT(linearAxes()));

  auto colour = cm.addMenu(tr("&Color Bar"));
  colour->addAction(tr("Lo&g Scale"), ag, SLOT(logColor()));
  colour->addAction(tr("&Linear"), ag, SLOT(linColor()));

  if (ag->normalizable()) {
    auto normalization = cm.addMenu(tr("&Normalization"));
    auto noNorm = new QAction(tr("N&one"), normalization);
    noNorm->setCheckable(true);
    connect(noNorm, SIGNAL(triggered()), ag, SLOT(noNormalization()));
    normalization->addAction(noNorm);

    auto binNorm = new QAction(tr("&Bin Width"), normalization);
    binNorm->setCheckable(true);
    connect(binNorm, SIGNAL(triggered()), ag, SLOT(binWidthNormalization()));
    normalization->addAction(binNorm);

    auto normalizationActions = new QActionGroup(this);
    normalizationActions->setExclusive(true);
    normalizationActions->addAction(noNorm);
    normalizationActions->addAction(binNorm);

    noNorm->setChecked(!ag->isDistribution());
    binNorm->setChecked(ag->isDistribution());
  } else if (ag->normalizableMD()) {
    auto normMD = cm.addMenu("MD &Normalization");
    auto noNormMD = new QAction(tr("N&one"), normMD);
    noNormMD->setCheckable(true);
    connect(noNormMD, SIGNAL(triggered()), ag, SLOT(noNormalizationMD()));
    normMD->addAction(noNormMD);

    auto volNormMD = new QAction(tr("&Volume"), normMD);
    volNormMD->setCheckable(true);
    connect(volNormMD, SIGNAL(triggered()), ag, SLOT(volumeNormalizationMD()));
    normMD->addAction(volNormMD);

    auto eventsNormMD = new QAction(tr("&Events"), normMD);
    eventsNormMD->setCheckable(true);
    connect(eventsNormMD, SIGNAL(triggered()), ag,
            SLOT(numEventsNormalizationMD()));
    normMD->addAction(eventsNormMD);

    auto normalization = ag->normalizationMD();
    noNormMD->setChecked(0 == normalization);
    volNormMD->setChecked(1 == normalization);
    eventsNormMD->setChecked(2 == normalization);
  }

  if (ag->curves() > 1) {
    auto plotType = cm.addMenu(tr("&Plot Type"));
    auto waterfall = new QAction(tr("&Waterfall"), plotType);
    waterfall->setCheckable(true);
    waterfall->setChecked(ag->isWaterfallPlot());
    connect(waterfall, SIGNAL(toggled(bool)), plot,
            SLOT(toggleWaterfall(bool)));
    plotType->addAction(waterfall);
  }

  cm.addSeparator();

  auto copy = cm.addMenu(getQPixmap("copy_xpm"), tr("&Copy"));
  copy->addAction(tr("&Layer"), this, SLOT(copyActiveLayer()));
  copy->addAction(tr("&Window"), plot, SLOT(copyAllLayers()));

  auto exports = cm.addMenu(tr("E&xport"));
  exports->addAction(tr("&Layer"), this, SLOT(exportLayer()));
  exports->addAction(tr("&Window"), this, SLOT(exportGraph()));

  auto prints = cm.addMenu(getQPixmap("fileprint_xpm"), tr("&Print"));
  prints->addAction(tr("&Layer"), plot, SLOT(printActiveLayer()));
  prints->addAction(tr("&Window"), plot, SLOT(print()));

  cm.addSeparator();

  cm.addAction(tr("P&roperties..."), this, SLOT(showGeneralPlotDialog()));
  cm.addSeparator();
  cm.addAction(getQPixmap("close_xpm"), tr("&Delete Layer"), plot,
               SLOT(confirmRemoveLayer()));
  cm.exec(QCursor::pos());
}

void ApplicationWindow::showWindowContextMenu() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  QMenu cm(this);
  QMenu plot3D(tr("3D &Plot"), this);
  const std::string windowType = w->metaObject()->className();
  if (windowType == "MultiLayer") {
    MultiLayer *g = dynamic_cast<MultiLayer *>(w);
    if (!g)
      return;
    if (lastCopiedLayer) {
      cm.addAction(getQPixmap("paste_xpm"), tr("&Paste Layer"), this,
                   SLOT(pasteSelection()));
      cm.addSeparator();
    }

    cm.addAction(actionAddLayer);
    if (g->layers() != 0)
      cm.addAction(actionDeleteLayer);

    cm.addAction(actionShowLayerDialog);
    cm.addSeparator();
    cm.addAction(actionRename);
    cm.addAction(actionCopyWindow);
    cm.addSeparator();
    cm.addAction(getQPixmap("copy_xpm"), tr("&Copy Page"), g,
                 SLOT(copyAllLayers()));
    cm.addAction(tr("E&xport Page"), this, SLOT(exportGraph()));
    cm.addAction(actionPrint);
    cm.addSeparator();
    cm.addAction(actionCloseWindow);
  } else if (windowType == "Graph3D") {
    Graph3D *g = dynamic_cast<Graph3D *>(w);
    if (!g)
      return;
    if (!g->hasData()) {
      cm.addMenu(&plot3D);
      plot3D.addAction(actionAdd3DData);
      plot3D.addAction(tr("&Matrix..."), this, SLOT(add3DMatrixPlot()));
      plot3D.addAction(actionEditSurfacePlot);
    } else {
      if (g->table())
        cm.addAction(tr("Choose &Data Set..."), this, SLOT(change3DData()));
      else if (g->matrix())
        cm.addAction(tr("Choose &Matrix..."), this, SLOT(change3DMatrix()));
      else if (g->userFunction() || g->parametricSurface())
        cm.addAction(actionEditSurfacePlot);
      cm.addAction(getQPixmap("erase_xpm"), tr("C&lear"), g, SLOT(clearData()));
    }

    cm.addSeparator();
    cm.addAction(actionRename);
    cm.addAction(actionCopyWindow);
    cm.addSeparator();
    cm.addAction(tr("&Copy Graph"), g, SLOT(copyImage()));
    cm.addAction(tr("&Export"), this, SLOT(exportGraph()));
    cm.addAction(actionPrint);
    cm.addSeparator();
    cm.addAction(actionCloseWindow);
  } else if (windowType == "Matrix") {
    Matrix *t = dynamic_cast<Matrix *>(w);
    if (!t)
      return;
    if (t->viewType() == Matrix::TableView) {
      cm.addAction(getQPixmap("cut_xpm"), tr("Cu&t"), t, SLOT(cutSelection()));
      cm.addAction(getQPixmap("copy_xpm"), tr("&Copy"), t,
                   SLOT(copySelection()));
      cm.addAction(getQPixmap("paste_xpm"), tr("&Paste"), t,
                   SLOT(pasteSelection()));
      cm.addSeparator();
      cm.addAction(getQPixmap("insert_row_xpm"), tr("&Insert Row"), t,
                   SLOT(insertRow()));
      cm.addAction(getQPixmap("insert_column_xpm"), tr("&Insert Column"), t,
                   SLOT(insertColumn()));
      if (t->numSelectedRows() > 0)
        cm.addAction(getQPixmap("delete_row_xpm"), tr("&Delete Rows"), t,
                     SLOT(deleteSelectedRows()));
      else if (t->numSelectedColumns() > 0)
        cm.addAction(getQPixmap("delete_column_xpm"), tr("&Delete Columns"), t,
                     SLOT(deleteSelectedColumns()));

      cm.addAction(getQPixmap("erase_xpm"), tr("Clea&r"), t,
                   SLOT(clearSelection()));
    } else if (t->viewType() == Matrix::ImageView) {
      cm.addAction(actionImportImage);
      cm.addAction(actionExportMatrix);
      cm.addSeparator();
      cm.addAction(actionSetMatrixProperties);
      cm.addAction(actionSetMatrixDimensions);
      cm.addSeparator();
      cm.addAction(actionSetMatrixValues);
      cm.addAction(actionTableRecalculate);
      cm.addSeparator();
      cm.addAction(actionRotateMatrix);
      cm.addAction(actionRotateMatrixMinus);
      cm.addSeparator();
      cm.addAction(actionFlipMatrixVertically);
      cm.addAction(actionFlipMatrixHorizontally);
      cm.addSeparator();
      cm.addAction(actionTransposeMatrix);
      cm.addAction(actionInvertMatrix);
    }
  } else
    mantidUI->showContextMenu(cm, w); // Mantid
  cm.exec(QCursor::pos());
}

void ApplicationWindow::customWindowTitleBarMenu(MdiSubWindow *w, QMenu *menu) {
  menu->addAction(actionHideActiveWindow);
  menu->addSeparator();
  if (w->inherits("Table")) {
    menu->addAction(actionShowExportASCIIDialog);
    menu->addSeparator();
  }
  const std::string windowClassName = w->metaObject()->className();
  if (windowClassName == "Note")
    menu->addAction(actionSaveNote);

  menu->addAction(actionPrint);
  menu->addSeparator();
  menu->addAction(actionRename);
  menu->addAction(actionCopyWindow);
  menu->addSeparator();
}

void ApplicationWindow::showTableContextMenu(bool selection) {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  bool isEditable = t->isEditable();
  bool isFixedColumns = t->isFixedColumns();

  QMenu cm(this);
  if (selection) {
    if ((int)t->selectedColumns().count() > 0) {
      showColMenu(t->firstSelectedColumn());
      return;
    } else if (t->numSelectedRows() == 1) {
      if (isEditable)
        cm.addAction(actionShowColumnValuesDialog);
      if (isEditable)
        cm.addAction(getQPixmap("cut_xpm"), tr("Cu&t"), t,
                     SLOT(cutSelection()));
      cm.addAction(getQPixmap("copy_xpm"), tr("&Copy"), t,
                   SLOT(copySelection()));
      if (isEditable)
        cm.addAction(getQPixmap("paste_xpm"), tr("&Paste"), t,
                     SLOT(pasteSelection()));
      cm.addSeparator();
      if (isEditable)
        cm.addAction(actionTableRecalculate);
      if (isEditable)
        cm.addAction(getQPixmap("insert_row_xpm"), tr("&Insert Row"), t,
                     SLOT(insertRow()));
      cm.addAction(getQPixmap("delete_row_xpm"), tr("&Delete Row"), t,
                   SLOT(deleteSelectedRows()));
      if (isEditable)
        cm.addAction(getQPixmap("erase_xpm"), tr("Clea&r Row"), t,
                     SLOT(clearSelection()));
      cm.addSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numSelectedRows() > 1) {
      if (isEditable)
        cm.addAction(actionShowColumnValuesDialog);
      if (isEditable)
        cm.addAction(getQPixmap("cut_xpm"), tr("Cu&t"), t,
                     SLOT(cutSelection()));
      cm.addAction(getQPixmap("copy_xpm"), tr("&Copy"), t,
                   SLOT(copySelection()));
      if (isEditable)
        cm.addAction(getQPixmap("paste_xpm"), tr("&Paste"), t,
                     SLOT(pasteSelection()));
      cm.addSeparator();
      if (isEditable)
        cm.addAction(actionTableRecalculate);
      cm.addAction(getQPixmap("delete_row_xpm"), tr("&Delete Rows"), t,
                   SLOT(deleteSelectedRows()));
      if (isEditable)
        cm.addAction(getQPixmap("erase_xpm"), tr("Clea&r Rows"), t,
                     SLOT(clearSelection()));
      cm.addSeparator();
      cm.addAction(actionShowRowStatistics);
    } else if (t->numRows() > 0 && t->numCols() > 0) {
      if (isEditable)
        cm.addAction(actionShowColumnValuesDialog);
      if (isEditable)
        cm.addAction(getQPixmap("cut_xpm"), tr("Cu&t"), t,
                     SLOT(cutSelection()));
      cm.addAction(getQPixmap("copy_xpm"), tr("&Copy"), t,
                   SLOT(copySelection()));
      if (isEditable)
        cm.addAction(getQPixmap("paste_xpm"), tr("&Paste"), t,
                     SLOT(pasteSelection()));
      cm.addSeparator();
      if (isEditable)
        cm.addAction(actionTableRecalculate);
      if (isEditable)
        cm.addAction(getQPixmap("erase_xpm"), tr("Clea&r"), t,
                     SLOT(clearSelection()));
    }
  } else {
    cm.addAction(actionShowExportASCIIDialog);
    cm.addSeparator();
    if (!isFixedColumns)
      cm.addAction(actionAddColToTable);
    if (isEditable)
      cm.addAction(actionClearTable);
    cm.addSeparator();
    cm.addAction(actionGoToRow);
    cm.addAction(actionGoToColumn);
  }
  cm.exec(QCursor::pos());
}

void ApplicationWindow::chooseHelpFolder() {
  QFileInfo hfi(helpFilePath);
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Choose the location of the MantidPlot help folder!"),
      hfi.dir().absolutePath(), nullptr /**QFileDialog::ShowDirsOnly*/);

  if (!dir.isEmpty()) {
    helpFilePath = dir + "index.html";

    QFile helpFile(helpFilePath);
    if (!helpFile.exists()) {
      QMessageBox::critical(
          this, tr("MantidPlot - index.html File Not Found!"), // Mantid
          tr("There is no file called <b>index.html</b> in this "
             "folder.<br>Please choose another folder!"));
    }
  }
}

void ApplicationWindow::showHelp() {
  QFile helpFile(helpFilePath);
  if (!helpFile.exists()) {
    QMessageBox::critical(
        this, tr("MantidPlot - Help Files Not Found!"), // Mantid
        tr("Please indicate the location of the help file!") + "<br>" +
            tr("The manual can be found at the following internet "
               "address:") +
            "<p><a href = "
            "http://www.mantidproject.org/MantidPlot:_Help>http://"
            "www.mantidproject.org/MantidPlot:_Help</a></p>");
    QString fn = QFileDialog::getOpenFileName(this, "Open help file",
                                              QDir::currentPath(), "*.html");
    if (!fn.isEmpty()) {
      QFileInfo fi(fn);
      helpFilePath = fi.absoluteFilePath();
      saveSettings();
    }
  }

  QFileInfo fi(helpFilePath);
  QString profilePath = QString(fi.absolutePath() + "/qtiplot.adp");
  if (!QFile(profilePath).exists()) {
    QMessageBox::critical(
        this, tr("MantidPlot - Help Profile Not Found!"), // Mantid
        tr("The assistant could not start because the file <b>%1</b> was not "
           "found in the help file directory!")
                .arg("qtiplot.adp") +
            "<br>" +
            tr("This file is provided with the MantidPlot manual which can "
               "be "
               "downloaded from the following internet address:") +
            "<p><a href = "
            "http://www.mantidproject.org/MantidPlot:_Help>http://"
            "www.mantidproject.org/MantidPlot:_Help</a></p>");
    return;
  }
}

void ApplicationWindow::showPlotWizard() {
  QStringList lst = tableNames();
  if (lst.count() > 0) {
    PlotWizard *pw = new PlotWizard(this, nullptr);
    pw->setAttribute(Qt::WA_DeleteOnClose);
    connect(pw, SIGNAL(plot(const QStringList &)), this,
            SLOT(multilayerPlot(const QStringList &)));

    pw->insertTablesList(lst);
    pw->setColumnsList(columnsList(Table::All));
    pw->changeColumnsList(lst[0]);
    pw->exec();
  } else
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no tables available in this project.</h4>"
           "<p><h4>Please create a table and try again!</h4>"));
}

void ApplicationWindow::setCurveFullRange() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  int curveKey = actionCurveFullRange->data().toInt();
  g->setCurveFullRange(g->curveIndex(curveKey));
}

void ApplicationWindow::showCurveRangeDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  int curveKey = actionEditCurveRange->data().toInt();
  showCurveRangeDialog(g, g->curveIndex(curveKey));
}

CurveRangeDialog *ApplicationWindow::showCurveRangeDialog(Graph *g, int curve) {
  if (!g)
    return nullptr;

  CurveRangeDialog *crd = new CurveRangeDialog(this);
  crd->setAttribute(Qt::WA_DeleteOnClose);
  crd->setCurveToModify(g, curve);
  crd->exec();
  return crd;
}

FunctionDialog *ApplicationWindow::showFunctionDialog() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return nullptr;

  Graph *g = plot->activeGraph();
  if (!g)
    return nullptr;

  int curveKey = actionEditFunction->data().toInt();
  return showFunctionDialog(g, g->curveIndex(curveKey));
}

FunctionDialog *ApplicationWindow::showFunctionDialog(Graph *g, int curve) {
  if (!g)
    return nullptr;

  FunctionDialog *fd = functionDialog(g);
  fd->setWindowTitle(tr("MantidPlot - Edit function")); // Mantid
  fd->setCurveToModify(g, curve);
  return fd;
}

FunctionDialog *ApplicationWindow::functionDialog(Graph *g) {
  FunctionDialog *fd = new FunctionDialog(this, g);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  connect(fd, SIGNAL(clearParamFunctionsList()), this,
          SLOT(clearParamFunctionsList()));
  connect(fd, SIGNAL(clearPolarFunctionsList()), this,
          SLOT(clearPolarFunctionsList()));

  fd->insertParamFunctionsList(xFunctions, yFunctions);
  fd->insertPolarFunctionsList(rFunctions, thetaFunctions);
  fd->show();
  return fd;
}

void ApplicationWindow::addFunctionCurve() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("MantidPlot - Warning"), // Mantid
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    return;
  }

  Graph *g = plot->activeGraph();
  if (g) {
    functionDialog(g);
  }
}

void ApplicationWindow::updateFunctionLists(int type, QStringList &formulas) {
  int maxListSize = 10;
  if (type == 2) {
    rFunctions.removeAll(formulas[0]);
    rFunctions.push_front(formulas[0]);

    thetaFunctions.removeAll(formulas[1]);
    thetaFunctions.push_front(formulas[1]);

    while ((int)rFunctions.size() > maxListSize)
      rFunctions.pop_back();
    while ((int)thetaFunctions.size() > maxListSize)
      thetaFunctions.pop_back();
  } else if (type == 1) {
    xFunctions.removeAll(formulas[0]);
    xFunctions.push_front(formulas[0]);

    yFunctions.removeAll(formulas[1]);
    yFunctions.push_front(formulas[1]);

    while ((int)xFunctions.size() > maxListSize)
      xFunctions.pop_back();
    while ((int)yFunctions.size() > maxListSize)
      yFunctions.pop_back();
  }
}

MultiLayer *ApplicationWindow::newFunctionPlot(QStringList &formulas,
                                               double start, double end,
                                               int points, const QString &var,
                                               int type) {
  MultiLayer *ml = newGraph();
  if (ml)
    ml->activeGraph()->addFunction(formulas, start, end, points, var, type);

  updateFunctionLists(type, formulas);
  return ml;
}

void ApplicationWindow::clearParamFunctionsList() {
  xFunctions.clear();
  yFunctions.clear();
}

void ApplicationWindow::clearPolarFunctionsList() {
  rFunctions.clear();
  thetaFunctions.clear();
}

void ApplicationWindow::clearSurfaceFunctionsList() { surfaceFunc.clear(); }

void ApplicationWindow::setFramed3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFramed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::setBoxed3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBoxed();
  actionShowAxisDialog->setEnabled(TRUE);
}

void ApplicationWindow::removeAxes3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setNoAxes();
  actionShowAxisDialog->setEnabled(false);
}

void ApplicationWindow::removeGrid3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setPolygonStyle();
}

void ApplicationWindow::setHiddenLineGrid3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setHiddenLineStyle();
}

void ApplicationWindow::setPoints3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setDotStyle();
}

void ApplicationWindow::setCones3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setConeStyle();
}

void ApplicationWindow::setCrosses3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setCrossStyle();
}

void ApplicationWindow::setBars3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBarStyle();
}

void ApplicationWindow::setLineGrid3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setWireframeStyle();
}

void ApplicationWindow::setFilledMesh3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFilledMeshStyle();
}

void ApplicationWindow::setFloorData3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorData();
}

void ApplicationWindow::setFloorIso3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorIsolines();
}

void ApplicationWindow::setEmptyFloor3DPlot() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setEmptyFloor();
}

void ApplicationWindow::setFrontGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFrontGrid(on);
}

void ApplicationWindow::setBackGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setBackGrid(on);
}

void ApplicationWindow::setFloorGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setFloorGrid(on);
}

void ApplicationWindow::setCeilGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setCeilGrid(on);
}

void ApplicationWindow::setRightGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setRightGrid(on);
}

void ApplicationWindow::setLeftGrid3DPlot(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setLeftGrid(on);
}

void ApplicationWindow::pickPlotStyle(QAction *action) {
  if (!action)
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

void ApplicationWindow::pickCoordSystem(QAction *action) {
  if (!action)
    return;

  if (action == Box || action == Frame) {
    if (action == Box)
      setBoxed3DPlot();
    if (action == Frame)
      setFramed3DPlot();
    grids->setEnabled(true);
  } else if (action == None) {
    removeAxes3DPlot();
    grids->setEnabled(false);
  }

  emit modified();
}

void ApplicationWindow::pickFloorStyle(QAction *action) {
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

void ApplicationWindow::custom3DActions(MdiSubWindow *w) {
  if (w && std::string(w->metaObject()->className()) == "Graph3D") {
    Graph3D *plot = dynamic_cast<Graph3D *>(w);
    if (!plot)
      return;
    actionAnimate->setChecked(plot->isAnimated());
    actionPerspective->setChecked(!plot->isOrthogonal());
    switch (plot->plotStyle()) {
    case FILLEDMESH:
      wireframe->setChecked(false);
      hiddenline->setChecked(false);
      polygon->setChecked(false);
      filledmesh->setChecked(true);
      pointstyle->setChecked(false);
      barstyle->setChecked(false);
      conestyle->setChecked(false);
      crossHairStyle->setChecked(false);
      break;

    case FILLED:
      wireframe->setChecked(false);
      hiddenline->setChecked(false);
      polygon->setChecked(true);
      filledmesh->setChecked(false);
      pointstyle->setChecked(false);
      barstyle->setChecked(false);
      conestyle->setChecked(false);
      crossHairStyle->setChecked(false);
      break;

    case Qwt3D::USER:
      wireframe->setChecked(false);
      hiddenline->setChecked(false);
      polygon->setChecked(false);
      filledmesh->setChecked(false);

      if (plot->pointType() == Graph3D::VerticalBars) {
        pointstyle->setChecked(false);
        conestyle->setChecked(false);
        crossHairStyle->setChecked(false);
        barstyle->setChecked(true);
      } else if (plot->pointType() == Graph3D::Dots) {
        pointstyle->setChecked(true);
        barstyle->setChecked(false);
        conestyle->setChecked(false);
        crossHairStyle->setChecked(false);
      } else if (plot->pointType() == Graph3D::HairCross) {
        pointstyle->setChecked(false);
        barstyle->setChecked(false);
        conestyle->setChecked(false);
        crossHairStyle->setChecked(true);
      } else if (plot->pointType() == Graph3D::Cones) {
        pointstyle->setChecked(false);
        barstyle->setChecked(false);
        conestyle->setChecked(true);
        crossHairStyle->setChecked(false);
      }
      break;

    case WIREFRAME:
      wireframe->setChecked(true);
      hiddenline->setChecked(false);
      polygon->setChecked(false);
      filledmesh->setChecked(false);
      pointstyle->setChecked(false);
      barstyle->setChecked(false);
      conestyle->setChecked(false);
      crossHairStyle->setChecked(false);
      break;

    case HIDDENLINE:
      wireframe->setChecked(false);
      hiddenline->setChecked(true);
      polygon->setChecked(false);
      filledmesh->setChecked(false);
      pointstyle->setChecked(false);
      barstyle->setChecked(false);
      conestyle->setChecked(false);
      crossHairStyle->setChecked(false);
      break;

    default:
      break;
    }

    switch (plot->coordStyle()) {
    case Qwt3D::NOCOORD:
      None->setChecked(true);
      Box->setChecked(false);
      Frame->setChecked(false);
      break;

    case Qwt3D::BOX:
      None->setChecked(false);
      Box->setChecked(true);
      Frame->setChecked(false);
      break;

    case Qwt3D::FRAME:
      None->setChecked(false);
      Box->setChecked(false);
      Frame->setChecked(true);
      break;
    }

    switch (plot->floorStyle()) {
    case NOFLOOR:
      floornone->setChecked(true);
      flooriso->setChecked(false);
      floordata->setChecked(false);
      break;

    case FLOORISO:
      floornone->setChecked(false);
      flooriso->setChecked(true);
      floordata->setChecked(false);
      break;

    case FLOORDATA:
      floornone->setChecked(false);
      flooriso->setChecked(false);
      floordata->setChecked(true);
      break;
    }
    custom3DGrids(plot->grids());
  }
}

void ApplicationWindow::custom3DGrids(int grids) {
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

void ApplicationWindow::initPlot3DToolBar() {
  /**
   * Only inits the actions that are later placed in the
   * Format menu in this->customMenu(MdiSubWindow* w)
   */

  coord = new QActionGroup(this);
  Box = new QAction(coord);
  Box->setIcon(QIcon(getQPixmap("box_xpm")));
  Box->setCheckable(true);
  Box->setChecked(true);

  Frame = new QAction(coord);
  Frame->setIcon(QIcon(getQPixmap("free_axes_xpm")));
  Frame->setCheckable(true);

  None = new QAction(coord);
  None->setIcon(QIcon(getQPixmap("no_axes_xpm")));
  None->setCheckable(true);

  // grid actions - Used when the format menu is active for the 3D plot
  grids = new QActionGroup(this);
  grids->setEnabled(true);
  grids->setExclusive(false);
  front = new QAction(grids);
  front->setText(tr("Front"));
  front->setCheckable(true);
  front->setIcon(QIcon(getQPixmap("frontGrid_xpm")));
  back = new QAction(grids);
  back->setText(tr("Back"));
  back->setCheckable(true);
  back->setIcon(QIcon(getQPixmap("backGrid_xpm")));
  right = new QAction(grids);
  right->setText(tr("Right"));
  right->setCheckable(true);
  right->setIcon(QIcon(getQPixmap("leftGrid_xpm")));
  left = new QAction(grids);
  left->setText(tr("Left"));
  left->setCheckable(true);
  left->setIcon(QIcon(getQPixmap("rightGrid_xpm")));
  ceil = new QAction(grids);
  ceil->setText(tr("Ceiling"));
  ceil->setCheckable(true);
  ceil->setIcon(QIcon(getQPixmap("ceilGrid_xpm")));
  floor = new QAction(grids);
  floor->setText(tr("Floor"));
  floor->setCheckable(true);
  floor->setIcon(QIcon(getQPixmap("floorGrid_xpm")));

  actionPerspective = new QAction(this);
  actionPerspective->setCheckable(true);
  actionPerspective->setIcon(getQPixmap("perspective_xpm"));
  actionPerspective->setChecked(!orthogonal3DPlots);
  connect(actionPerspective, SIGNAL(toggled(bool)), this,
          SLOT(togglePerspective(bool)));

  actionResetRotation = new QAction(this);
  actionResetRotation->setCheckable(false);
  actionResetRotation->setIcon(getQPixmap("reset_rotation_xpm"));
  connect(actionResetRotation, SIGNAL(triggered()), this,
          SLOT(resetRotation()));

  actionFitFrame = new QAction(this);
  actionFitFrame->setCheckable(false);
  actionFitFrame->setIcon(getQPixmap("fit_frame_xpm"));
  connect(actionFitFrame, SIGNAL(triggered()), this, SLOT(fitFrameToLayer()));

  // plot style actions
  plotstyle = new QActionGroup(this);

  wireframe = new QAction(plotstyle);
  wireframe->setCheckable(true);
  wireframe->setEnabled(true);
  wireframe->setIcon(QIcon(getQPixmap("lineMesh_xpm")));

  hiddenline = new QAction(plotstyle);
  hiddenline->setCheckable(true);
  hiddenline->setEnabled(true);
  hiddenline->setIcon(QIcon(getQPixmap("grid_only_xpm")));

  polygon = new QAction(plotstyle);
  polygon->setCheckable(true);
  polygon->setEnabled(true);
  polygon->setIcon(QIcon(getQPixmap("no_grid_xpm")));

  filledmesh = new QAction(plotstyle);
  filledmesh->setCheckable(true);
  filledmesh->setIcon(QIcon(getQPixmap("grid_poly_xpm")));
  filledmesh->setChecked(true);

  pointstyle = new QAction(plotstyle);
  pointstyle->setCheckable(true);
  pointstyle->setIcon(QIcon(getQPixmap("pointsMesh_xpm")));

  conestyle = new QAction(plotstyle);
  conestyle->setCheckable(true);
  conestyle->setIcon(QIcon(getQPixmap("cones_xpm")));

  crossHairStyle = new QAction(plotstyle);
  crossHairStyle->setCheckable(true);
  crossHairStyle->setIcon(QIcon(getQPixmap("crosses_xpm")));

  barstyle = new QAction(plotstyle);
  barstyle->setCheckable(true);
  barstyle->setIcon(QIcon(getQPixmap("plot_bars_xpm")));

  // floor actions
  floorstyle = new QActionGroup(this);
  floordata = new QAction(floorstyle);
  floordata->setCheckable(true);
  floordata->setIcon(QIcon(getQPixmap("floor_xpm")));
  flooriso = new QAction(floorstyle);
  flooriso->setCheckable(true);
  flooriso->setIcon(QIcon(getQPixmap("isolines_xpm")));
  floornone = new QAction(floorstyle);
  floornone->setCheckable(true);
  floornone->setIcon(QIcon(getQPixmap("no_floor_xpm")));
  floornone->setChecked(true);

  actionAnimate = new QAction(this);
  actionAnimate->setCheckable(true);
  actionAnimate->setIcon(getQPixmap("movie_xpm"));

  connect(actionAnimate, SIGNAL(toggled(bool)), this,
          SLOT(toggle3DAnimation(bool)));
  connect(coord, SIGNAL(triggered(QAction *)), this,
          SLOT(pickCoordSystem(QAction *)));
  connect(floorstyle, SIGNAL(triggered(QAction *)), this,
          SLOT(pickFloorStyle(QAction *)));
  connect(plotstyle, SIGNAL(triggered(QAction *)), this,
          SLOT(pickPlotStyle(QAction *)));

  connect(left, SIGNAL(triggered(bool)), this, SLOT(setLeftGrid3DPlot(bool)));
  connect(right, SIGNAL(triggered(bool)), this, SLOT(setRightGrid3DPlot(bool)));
  connect(ceil, SIGNAL(triggered(bool)), this, SLOT(setCeilGrid3DPlot(bool)));
  connect(floor, SIGNAL(triggered(bool)), this, SLOT(setFloorGrid3DPlot(bool)));
  connect(back, SIGNAL(triggered(bool)), this, SLOT(setBackGrid3DPlot(bool)));
  connect(front, SIGNAL(triggered(bool)), this, SLOT(setFrontGrid3DPlot(bool)));
}

void ApplicationWindow::pixelLineProfile() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  bool ok;
  auto res = QInputDialog::getInt(
      this, tr("MantidPlot - Set the number of pixels to average"),
      tr("Number of averaged pixels"), 1, 1, 2000, 2, &ok);
  if (!ok)
    return;

  LineProfileTool *lpt = new LineProfileTool(g, this, res);
  g->setActiveTool(lpt);
}

void ApplicationWindow::intensityTable() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (g) {
    ImageMarker *im = dynamic_cast<ImageMarker *>(g->selectedMarkerPtr());
    if (im) {
      QString fn = im->fileName();
      if (!fn.isEmpty())
        importImage(fn);
    }
  }
}

void ApplicationWindow::autoArrangeLayers() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  plot->setMargins(5, 5, 5, 5);
  plot->setSpacing(5, 5);
  plot->arrangeLayers(true, false);

  if (plot->isWaterfallPlot())
    plot->updateWaterfalls();
}

void ApplicationWindow::addLayer() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;
  switch (QMessageBox::information(
      this,
      tr("MantidPlot - Guess best origin for the new layer?"), // Mantid
      tr("Do you want MantidPlot to guess the best position for the new "
         "layer?\n Warning: this will rearrange existing layers!"), // Mantid
      tr("&Guess"), tr("&Top-left corner"), tr("&Cancel"), 0, 2)) {
  case 0: {
    setPreferences(plot->addLayer());
    plot->arrangeLayers(true, false);
  } break;

  case 1:
    setPreferences(
        plot->addLayer(0, 0, plot->size().width(), plot->size().height()));
    break;

  case 2:
    return;
    break;
  }
}

void ApplicationWindow::deleteLayer() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  plot->confirmRemoveLayer();
}

void ApplicationWindow::copyActiveLayer() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();

  lastCopiedLayer = g;
  connect(g, SIGNAL(destroyed()), this, SLOT(closedLastCopiedLayer()));
  g->copyImage();
}

void ApplicationWindow::showDataSetDialog(Analysis operation) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  DataSetDialog *ad = new DataSetDialog(tr("Curve") + ": ", this, g);
  ad->setOperationType(operation);
  ad->exec();
}

void ApplicationWindow::analyzeCurve(Graph *g, Analysis operation,
                                     const QString &curveTitle) {
  if (!g)
    return;

  Fit *fitter = nullptr;
  switch (operation) {
  case NoAnalysis:
    break;
  case Integrate: {
    Integration *i = new Integration(this, g, curveTitle);
    i->run();
    delete i;
  } break;
  case Diff: {
    Differentiation *diff = new Differentiation(this, g, curveTitle);
    diff->enableGraphicsDisplay(true);
    diff->run();
    delete diff;
  } break;
  case FitLinear:
    fitter = new LinearFit(this, g);
    break;
  case FitLorentz:
    fitter = new LorentzFit(this, g);
    break;
  case FitGauss:
    fitter = new GaussFit(this, g);
    break;
  case FitSigmoidal: {
    QwtPlotCurve *c = g->curve(curveTitle);
    if (c) {
      ScaleEngine *se = dynamic_cast<ScaleEngine *>(
          g->plotWidget()->axisScaleEngine(c->xAxis()));
      if (se) {
        if (se->type() == ScaleTransformation::Log10)
          fitter = new LogisticFit(this, g);
        else
          fitter = new SigmoidalFit(this, g);
      }
    }
  } break;
  }

  if (!fitter)
    return;

  if (fitter->setDataFromCurve(curveTitle)) {
    if (operation != FitLinear) {
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

void ApplicationWindow::analysis(Analysis operation) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g || !g->validCurvesDataSize())
    return;

  QString curve_title = g->selectedCurveTitle();
  if (!curve_title.isNull()) {
    analyzeCurve(g, operation, curve_title);
    return;
  }

  QStringList lst = g->analysableCurvesList();
  if (lst.count() == 1) {
    const QwtPlotCurve *c = g->curve(lst[0]);
    if (c)
      analyzeCurve(g, operation, lst[0]);
  } else
    showDataSetDialog(operation);
}

void ApplicationWindow::integrate() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (isOfType(w, "MultiLayer"))
    analysis(Integrate);
  else if (isOfType(w, "Matrix")) {
    auto matrix = dynamic_cast<Matrix *>(w);
    if (!matrix)
      return;

    QDateTime dt = QDateTime::currentDateTime();
    QString info = dt.toString(Qt::LocalDate);
    info += "\n" +
            tr("Integration of %1 from zero is").arg(QString(w->objectName())) +
            ":\t";
    info += QString::number(matrix->integrate()) + "\n";
    info += "-------------------------------------------------------------\n";
    currentFolder()->appendLogInfo(info);
    showResults(true);
  }
}

void ApplicationWindow::differentiate() { analysis(Diff); }

void ApplicationWindow::fitLinear() { analysis(FitLinear); }

void ApplicationWindow::fitSigmoidal() { analysis(FitSigmoidal); }

void ApplicationWindow::fitGauss() { analysis(FitGauss); }

void ApplicationWindow::fitLorentz()

{
  analysis(FitLorentz);
}

void ApplicationWindow::pickPointerCursor() { btnPointer->setChecked(true); }

void ApplicationWindow::disableTools() {
  if (displayBar->isVisible())
    displayBar->hide();

  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    auto ml = dynamic_cast<MultiLayer *>(w);
    if (ml) {
      QList<Graph *> layers = ml->layersList();
      foreach (Graph *g, layers)
        g->disableTools();
    }
  }
}

void ApplicationWindow::pickDataTool(QAction *action) {
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

void ApplicationWindow::connectSurfacePlot(Graph3D *plot) {
  connect(plot, SIGNAL(showOptionsDialog()), this, SLOT(showPlot3dDialog()));
  plot->confirmClose(confirmClosePlot3D);
}

void ApplicationWindow::connectMultilayerPlot(MultiLayer *g) {
  connect(g, SIGNAL(showTextDialog()), this, SLOT(showTextDialog()));
  connect(g, SIGNAL(showPlotDialog(int)), this, SLOT(showPlotDialog(int)));
  connect(g, SIGNAL(showScaleDialog(int)), this,
          SLOT(showScalePageFromAxisDialog(int)));
  connect(g, SIGNAL(showAxisDialog(int)), this,
          SLOT(showAxisPageFromAxisDialog(int)));
  connect(g, SIGNAL(showCurveContextMenu(int)), this,
          SLOT(showCurveContextMenu(int)));
  connect(g, SIGNAL(showCurvesDialog()), this, SLOT(showCurvesDialog()));
  connect(g, SIGNAL(drawLineEnded(bool)), btnPointer, SLOT(setOn(bool)));
  connect(g, SIGNAL(showAxisTitleDialog()), this, SLOT(showAxisTitleDialog()));

  connect(g, SIGNAL(showMarkerPopupMenu()), this, SLOT(showMarkerPopupMenu()));
  connect(g, SIGNAL(cursorInfo(const QString &)), info,
          SLOT(setText(const QString &)));
  connect(g, SIGNAL(showImageDialog()), this, SLOT(showImageDialog()));
  connect(g, SIGNAL(createTable(const QString &, int, int, const QString &)),
          this, SLOT(newTable(const QString &, int, int, const QString &)));
  connect(g, SIGNAL(viewTitleDialog()), this, SLOT(showTitleDialog()));
  connect(g, SIGNAL(modifiedPlot()), this, SLOT(modifiedProject()));
  connect(g, SIGNAL(showLineDialog()), this, SLOT(showLineDialog()));
  connect(g, SIGNAL(pasteMarker()), this, SLOT(pasteSelection()));
  connect(g, SIGNAL(showGraphContextMenu()), this,
          SLOT(showGraphContextMenu()));
  connect(g, SIGNAL(setPointerCursor()), this, SLOT(pickPointerCursor()));
  connect(g, SIGNAL(currentFontChanged(const QFont &)), this,
          SLOT(setFormatBarFont(const QFont &)));
  connect(g, SIGNAL(enableTextEditor(Graph *)), this,
          SLOT(enableTextEditor(Graph *)));

  g->confirmClose(confirmClosePlot2D);
}

void ApplicationWindow::connectTable(Table *w) {
  connect(w->table(), SIGNAL(itemSelectionChanged()), this,
          SLOT(customColumnActions()));
  setUpdateCurvesFromTable(w, true);
  connect(w, SIGNAL(optionsDialog()), this, SLOT(showColumnOptionsDialog()));
  connect(w, SIGNAL(colValuesDialog()), this, SLOT(showColumnValuesDialog()));
  connect(w, SIGNAL(showContextMenu(bool)), this,
          SLOT(showTableContextMenu(bool)));
  connect(w, SIGNAL(changedColHeader(const QString &, const QString &)), this,
          SLOT(updateColNames(const QString &, const QString &)));
  connect(w, SIGNAL(createTable(const QString &, int, int, const QString &)),
          this, SLOT(newTable(const QString &, int, int, const QString &)));

  w->confirmClose(confirmCloseTable);
}

/**
 * Connect or disconnect the auto-update of curves from a table
 * @param table :: [input] Table to connect/disconnect signal from
 * @param on :: [bool] True to turn auto-update on, false to turn off
 */
void ApplicationWindow::setUpdateCurvesFromTable(Table *table, bool on) {
  if (table) { // If no table, nothing to do
    if (on) {
      connect(table, SIGNAL(removedCol(const QString &)), this,
              SLOT(removeCurves(const QString &)));
      connect(table, SIGNAL(modifiedData(Table *, const QString &)), this,
              SLOT(updateCurves(Table *, const QString &)));
    } else {
      disconnect(table, SIGNAL(removedCol(const QString &)), this,
                 SLOT(removeCurves(const QString &)));
      disconnect(table, SIGNAL(modifiedData(Table *, const QString &)), this,
                 SLOT(updateCurves(Table *, const QString &)));
    }
  }
}

/** Fixes the colour palette so that the hints are readable.

  On Linux Fedora 26+ and Ubuntu 14.4+ the palette colour for
  ToolTipBase has no effect on the colour of tooltips, but does change
  the colour of 'What's This' boxes and and Line Edit hints. The palette
  colour for ToolTipText on the other hand affects all three of
  these.

  The default palette shows light text on a pale background which, although
  not affecting tooltips, makes LineEdit hints and 'What's This' boxes
  difficuilt if not impossible to read.

  Changing the tooltip text to a darker colour fixes the problem for
  'LineEdit' hints and 'What's This' boxes but creates one for ordinary
  tooltips.

  Setting the tooltip background colour to a darker colour works fine on
  Fedora 26-7+ and Ubuntu 14.04 but not for RHEL7 where the tooltip text
  colour is also dark.

  One option is to simply hardcode the values to a dark-on-light colour
  scheme which works consistently on Fedora, Ubuntu and RHEL7.
  However, RHEL7 users were not happy with this solution.

  Further investigation revealed that the issue may be related to
  https://bugs.launchpad.net/ubuntu/+source/qt4-x11/+bug/877236 and the
  fact that in the qt source gui/styles/qgtkstyle.cpp ~line 299 only loads
  the ToolTipText colour, leaving ToolTibBase to the default from
  CleanLooksStyle.

  This problem can be worked around on fedora by switching to the
  Adwaita theme in Mantid Preferences. However, the adwaita-qt
  theme is not available on certain versions of Ubuntu.

  Any easy solution for now is to detect unity clients and apply the
  fix only to them.
*/
void ApplicationWindow::patchPaletteForLinux(QPalette &palette) const {
  if (isUnityDesktop()) {
    auto tooltipBaseColor = QColor("black");
    auto tooltipTextColor = QColor("white");

    palette.setColor(QPalette::ColorGroup::Inactive, QPalette::ToolTipBase,
                     tooltipBaseColor);
    palette.setColor(QPalette::ColorGroup::Active, QPalette::ToolTipBase,
                     tooltipBaseColor);
    palette.setColor(QPalette::ColorGroup::Inactive, QPalette::ToolTipText,
                     tooltipTextColor);
    palette.setColor(QPalette::ColorGroup::Active, QPalette::ToolTipText,
                     tooltipTextColor);
  }
}

bool ApplicationWindow::isUnityDesktop() const {
  return QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")) == "Unity" ||
         QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")) == "Unity" ||
         QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")) ==
             "ubuntu:GNOME" ||
         QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")) ==
             "ubuntu:GNOME";
}

void ApplicationWindow::setAppColors(const QColor &wc, const QColor &pc,
                                     const QColor &tpc, bool force) {
  if (force || workspaceColor != wc) {
    workspaceColor = wc;
    d_workspace->setBackground(wc);
  }

  if (!force && panelsColor == pc && panelsTextColor == tpc)
    return;

  panelsColor = pc;
  panelsTextColor = tpc;

  QPalette palette;

#ifdef Q_OS_LINUX
  patchPaletteForLinux(palette);
#endif

  palette.setColor(QPalette::Base, QColor(panelsColor));
  qApp->setPalette(palette);

  palette.setColor(QPalette::Text, QColor(panelsTextColor));
  palette.setColor(QPalette::WindowText, QColor(panelsTextColor));

  lv->setPalette(palette);
  folders->setPalette(palette);
}

void ApplicationWindow::setPlot3DOptions() {
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (isOfType(w, "Graph3D")) {
      Graph3D *g = dynamic_cast<Graph3D *>(w);
      if (!g)
        continue;
      g->setOrthogonal(orthogonal3DPlots);
      g->setAutoscale(autoscale3DPlots);
      g->setAntialiasing(smooth3DMesh);
    }
  }
}

void ApplicationWindow::createActions() {
  actionCustomActionDialog = new MantidQt::MantidWidgets::TrackedAction(
      tr("Manage Custom Menus..."), this);
  connect(actionCustomActionDialog, SIGNAL(triggered()), this,
          SLOT(showCustomActionDialog()));

  actionManageDirs = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("managefolders_xpm")), tr("Manage User Directories"),
      this);
  connect(actionManageDirs, SIGNAL(triggered()), this,
          SLOT(showUserDirectoryDialog()));

  actionFirstTimeSetup =
      new MantidQt::MantidWidgets::TrackedAction(tr("First Time Setup"), this);
  connect(actionFirstTimeSetup, SIGNAL(triggered()), this,
          SLOT(showFirstTimeSetup()));

  actionNewProject = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/NewProject16x16.png"), tr("New &Project"), this);
  actionNewProject->setShortcut(tr("Ctrl+N"));
  connect(actionNewProject, SIGNAL(triggered()), this, SLOT(newProject()));

  actionSaveProject = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/SaveProject16x16.png"), tr("Save &Project"), this);
  actionSaveProject->setShortcut(tr("Ctrl+Shift+S"));
  connect(actionSaveProject, SIGNAL(triggered()), this,
          SLOT(prepareSaveProject()));

  actionSaveFile = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("filesave_nexus_xpm")), tr("Save Nexus &File"), this);
  actionSaveFile->setShortcut(tr("Ctrl+S"));
  connect(actionSaveFile, SIGNAL(triggered()), this, SLOT(savetoNexusFile()));

  actionNewGraph = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("new_graph_xpm")), tr("New &Graph"), this);
  actionNewGraph->setShortcut(tr("Ctrl+G"));
  connect(actionNewGraph, SIGNAL(triggered()), this, SLOT(newGraph()));

  actionNewNote = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("new_note_xpm")), tr("New &Note"), this);
  connect(actionNewNote, SIGNAL(triggered()), this, SLOT(newNote()));

  actionNewTable = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("table_xpm")), tr("New &Table"), this);
  actionNewTable->setShortcut(tr("Ctrl+T"));
  connect(actionNewTable, SIGNAL(triggered()), this, SLOT(newTable()));

  actionNewTiledWindow = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("tiledwindow_xpm")), tr("New Tiled &Window"), this);
  actionNewTiledWindow->setShortcut(tr("Ctrl+Shift+T"));
  connect(actionNewTiledWindow, SIGNAL(triggered()), this,
          SLOT(newTiledWindow()));

  actionNewMatrix = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("new_matrix_xpm")), tr("New &Matrix"), this);
  actionNewMatrix->setShortcut(tr("Ctrl+M"));
  connect(actionNewMatrix, SIGNAL(triggered()), this, SLOT(newMatrix()));

  actionNewFunctionPlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("newF_xpm")), tr("New &Function Plot"), this);
  connect(actionNewFunctionPlot, SIGNAL(triggered()), this,
          SLOT(functionDialog()));

  actionNewSurfacePlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("newFxy_xpm")), tr("New 3D &Surface Plot"), this);
  actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));
  connect(actionNewSurfacePlot, SIGNAL(triggered()), this,
          SLOT(newSurfacePlot()));

  actionOpenProj = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/LoadProject16x16.png"), tr("&Project"), this);
  actionOpenProj->setShortcut(tr("Ctrl+Shift+O"));
  connect(actionOpenProj, SIGNAL(triggered()), this, SLOT(open()));

  actionLoadFile = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/Open-icon16x16.png"), tr("Data File"), this);
  actionLoadFile->setShortcut(tr("Ctrl+Shift+F"));
  connect(actionLoadFile, SIGNAL(triggered()), this, SLOT(loadDataFile()));

  actionLoadImage =
      new MantidQt::MantidWidgets::TrackedAction(tr("Open Image &File"), this);
  actionLoadImage->setShortcut(tr("Ctrl+I"));
  connect(actionLoadImage, SIGNAL(triggered()), this, SLOT(loadImage()));

  actionScriptRepo = new MantidQt::MantidWidgets::TrackedAction(
      tr("Script Repositor&y"), this);
  connect(actionScriptRepo, SIGNAL(triggered()), this, SLOT(loadScriptRepo()));

  actionImportImage =
      new MantidQt::MantidWidgets::TrackedAction(tr("Import I&mage..."), this);
  connect(actionImportImage, SIGNAL(triggered()), this, SLOT(importImage()));

  actionSaveProjectAs = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/SaveProject16x16.png"), tr("Save Project &As..."), this);
  connect(actionSaveProjectAs, SIGNAL(triggered()), this,
          SLOT(prepareSaveProject()));
  actionSaveProjectAs->setEnabled(false);

  actionSaveNote =
      new MantidQt::MantidWidgets::TrackedAction(tr("Save Note As..."), this);
  connect(actionSaveNote, SIGNAL(triggered()), this, SLOT(saveNoteAs()));

  actionLoad = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("import_xpm")), tr("&Import ASCII..."), this);
  connect(actionLoad, SIGNAL(triggered()), this, SLOT(importASCII()));

  actionCopyWindow = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("duplicate_xpm")), tr("&Duplicate"), this);
  connect(actionCopyWindow, SIGNAL(triggered()), this, SLOT(clone()));

  actionCutSelection = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("cut_xpm")), tr("Cu&t Selection"), this);
  actionCutSelection->setShortcut(tr("Ctrl+X"));
  connect(actionCutSelection, SIGNAL(triggered()), this, SLOT(cutSelection()));

  actionCopySelection = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("copy_xpm")), tr("&Copy Selection"), this);
  actionCopySelection->setShortcut(tr("Ctrl+C"));
  connect(actionCopySelection, SIGNAL(triggered()), this,
          SLOT(copySelection()));

  actionPasteSelection = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("paste_xpm")), tr("&Paste Selection"), this);
  actionPasteSelection->setShortcut(tr("Ctrl+V"));
  connect(actionPasteSelection, SIGNAL(triggered()), this,
          SLOT(pasteSelection()));

  actionClearSelection = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("erase_xpm")), tr("&Delete Selection"), this);
  actionClearSelection->setShortcut(tr("Del", "delete key"));
  connect(actionClearSelection, SIGNAL(triggered()), this,
          SLOT(clearSelection()));

  actionShowExplorer = explorerWindow->toggleViewAction();
  actionShowExplorer->setIcon(getQPixmap("folder_xpm"));
  actionShowExplorer->setShortcut(tr("Ctrl+E"));

  actionShowLog = logWindow->toggleViewAction();
  actionShowLog->setIcon(getQPixmap("log_xpm"));

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("python_xpm"), tr("Toggle &Script Window"), this);
#ifdef __APPLE__
  actionShowScriptWindow->setShortcut(
      tr("Ctrl+3")); // F3 is used by the window manager on Mac
#else
  actionShowScriptWindow->setShortcut(tr("F3"));
#endif
  actionShowScriptWindow->setCheckable(true);
  connect(actionShowScriptWindow, SIGNAL(triggered()), this,
          SLOT(showScriptWindow()));

  actionShowScriptInterpreter = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("python_xpm"), tr("Toggle Script &Interpreter"), this);
#ifdef __APPLE__
  actionShowScriptInterpreter->setShortcut(
      tr("Ctrl+4")); // F4 is used by the window manager on Mac
#else
  actionShowScriptInterpreter->setShortcut(tr("F4"));
#endif
  actionShowScriptInterpreter->setCheckable(true);
  connect(actionShowScriptInterpreter, SIGNAL(triggered()), this,
          SLOT(showScriptInterpreter()));
#endif

  actionAddLayer = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("newLayer_xpm")), tr("Add La&yer"), this);
  actionAddLayer->setShortcut(tr("Alt+L"));
  connect(actionAddLayer, SIGNAL(triggered()), this, SLOT(addLayer()));

  actionShowLayerDialog = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("arrangeLayers_xpm")), tr("Arran&ge Layers"), this);
  actionShowLayerDialog->setShortcut(tr("Alt+A"));
  connect(actionShowLayerDialog, SIGNAL(triggered()), this,
          SLOT(showLayerDialog()));

  actionAutomaticLayout = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("auto_layout_xpm")), tr("Automatic Layout"), this);
  connect(actionAutomaticLayout, SIGNAL(triggered()), this,
          SLOT(autoArrangeLayers()));

  actionExportGraph =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Current"), this);
  actionExportGraph->setShortcut(tr("Alt+G"));
  connect(actionExportGraph, SIGNAL(triggered()), this, SLOT(exportGraph()));

  actionExportAllGraphs =
      new MantidQt::MantidWidgets::TrackedAction(tr("&All"), this);
  actionExportAllGraphs->setShortcut(tr("Alt+X"));
  connect(actionExportAllGraphs, SIGNAL(triggered()), this,
          SLOT(exportAllGraphs()));

  actionExportPDF = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("pdf_xpm")), tr("&Export PDF"), this);
  actionExportPDF->setShortcut(tr("Ctrl+Alt+P"));
  connect(actionExportPDF, SIGNAL(triggered()), this, SLOT(exportPDF()));

  actionPrint = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("fileprint_xpm")), tr("&Print"), this);
  actionPrint->setShortcut(tr("Ctrl+P"));
  connect(actionPrint, SIGNAL(triggered()), this, SLOT(print()));

  actionPrintAllPlots =
      new MantidQt::MantidWidgets::TrackedAction(tr("Print All Plo&ts"), this);
  connect(actionPrintAllPlots, SIGNAL(triggered()), this,
          SLOT(printAllPlots()));

  actionShowExportASCIIDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("E&xport ASCII"), this);
  connect(actionShowExportASCIIDialog, SIGNAL(triggered()), this,
          SLOT(showExportASCIIDialog()));

  actionCloseAllWindows = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("quit_xpm")), tr("&Quit"), this);
  actionCloseAllWindows->setShortcut(tr("Ctrl+Q"));
  connect(actionCloseAllWindows, SIGNAL(triggered()), this, SLOT(close()));

  actionDeleteFitTables = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("close_xpm")), tr("Delete &Fit Tables"), this);
  connect(actionDeleteFitTables, SIGNAL(triggered()), this,
          SLOT(deleteFitTables()));

  actionShowPlotWizard = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("wizard_xpm")), tr("Plot &Wizard"), this);
  actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));
  connect(actionShowPlotWizard, SIGNAL(triggered()), this,
          SLOT(showPlotWizard()));

  actionShowConfigureDialog = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/configure.png"), tr("&Preferences..."), this);
  connect(actionShowConfigureDialog, SIGNAL(triggered()), this,
          SLOT(showPreferencesDialog()));

  actionShowCurvesDialog = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("curves_xpm")), tr("Add/Remove &Curve..."), this);
  actionShowCurvesDialog->setShortcut(tr("Ctrl+Alt+C"));
  connect(actionShowCurvesDialog, SIGNAL(triggered()), this,
          SLOT(showCurvesDialog()));

  actionAddErrorBars = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("errors_xpm")), tr("Add &Error Bars..."), this);
  actionAddErrorBars->setShortcut(tr("Ctrl+Alt+E"));
  connect(actionAddErrorBars, SIGNAL(triggered()), this, SLOT(addErrorBars()));

  actionRemoveErrorBars = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("errors_remove_xpm")), tr("&Remove Error Bars..."),
      this);
  actionRemoveErrorBars->setShortcut(tr("Ctrl+Alt+R"));
  connect(actionRemoveErrorBars, SIGNAL(triggered()), this,
          SLOT(removeErrorBars()));

  actionAddFunctionCurve = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("fx_xpm")), tr("Add &Function..."), this);
  actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));
  connect(actionAddFunctionCurve, SIGNAL(triggered()), this,
          SLOT(addFunctionCurve()));

  actionUnzoom = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("unzoom_xpm")), tr("&Rescale to Show All"), this);
  actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
  connect(actionUnzoom, SIGNAL(triggered()), this, SLOT(setAutoScale()));

  actionNewLegend = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("legend_xpm")), tr("New &Legend"), this);
  actionNewLegend->setShortcut(tr("Ctrl+Alt+L"));
  connect(actionNewLegend, SIGNAL(triggered()), this, SLOT(newLegend()));

  actionTimeStamp = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("clock_xpm")), tr("Add Time &Stamp"), this);
  actionTimeStamp->setShortcut(tr("Ctrl+ALT+S"));
  connect(actionTimeStamp, SIGNAL(triggered()), this, SLOT(addTimeStamp()));

  actionAddImage = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("monalisa_xpm")), tr("Add &Image"), this);
  actionAddImage->setShortcut(tr("Ctrl+Alt+I"));
  connect(actionAddImage, SIGNAL(triggered()), this, SLOT(addImage()));

  actionPlotL = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("lPlot_xpm")), tr("&Line"), this);
  connect(actionPlotL, SIGNAL(triggered()), this, SLOT(plotL()));

  actionPlotP = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("pPlot_xpm")), tr("&Scatter"), this);
  connect(actionPlotP, SIGNAL(triggered()), this, SLOT(plotP()));

  actionPlotLP = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("lpPlot_xpm")), tr("Line + S&ymbol"), this);
  connect(actionPlotLP, SIGNAL(triggered()), this, SLOT(plotLP()));

  actionPlotVerticalDropLines = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("dropLines_xpm")), tr("Vertical &Drop Lines"), this);
  connect(actionPlotVerticalDropLines, SIGNAL(triggered()), this,
          SLOT(plotVerticalDropLines()));

  actionPlotSpline = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("spline_xpm")), tr("&Spline"), this);
  connect(actionPlotSpline, SIGNAL(triggered()), this, SLOT(plotSpline()));

  actionPlotHorSteps = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("hor_steps_xpm"), tr("&Horizontal Steps"), this);
  connect(actionPlotHorSteps, SIGNAL(triggered()), this, SLOT(plotHorSteps()));

  actionPlotVertSteps = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("vert_steps_xpm")), tr("&Vertical Steps"), this);
  connect(actionPlotVertSteps, SIGNAL(triggered()), this,
          SLOT(plotVertSteps()));

  actionPlotVerticalBars = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("vertBars_xpm")), tr("&Columns"), this);
  connect(actionPlotVerticalBars, SIGNAL(triggered()), this,
          SLOT(plotVerticalBars()));

  actionPlotHorizontalBars = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("hBars_xpm")), tr("&Rows"), this);
  connect(actionPlotHorizontalBars, SIGNAL(triggered()), this,
          SLOT(plotHorizontalBars()));

  actionPlotArea = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("area_xpm")), tr("&Area"), this);
  connect(actionPlotArea, SIGNAL(triggered()), this, SLOT(plotArea()));

  actionPlotPie = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("pie_xpm")), tr("&Pie"), this);
  connect(actionPlotPie, SIGNAL(triggered()), this, SLOT(plotPie()));

  actionPlotVectXYAM = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("vectXYAM_xpm")), tr("Vectors XY&AM"), this);
  connect(actionPlotVectXYAM, SIGNAL(triggered()), this, SLOT(plotVectXYAM()));

  actionPlotVectXYXY = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("vectXYXY_xpm")), tr("&Vectors &XYXY"), this);
  connect(actionPlotVectXYXY, SIGNAL(triggered()), this, SLOT(plotVectXYXY()));

  actionPlotHistogram = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("histogram_xpm")), tr("&Histogram"), this);
  connect(actionPlotHistogram, SIGNAL(triggered()), this,
          SLOT(plotHistogram()));

  actionPlotStackedHistograms = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("stacked_hist_xpm")), tr("&Stacked Histogram"), this);
  connect(actionPlotStackedHistograms, SIGNAL(triggered()), this,
          SLOT(plotStackedHistograms()));

  actionStemPlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/leaf.png"), tr("Stem-and-&Leaf Plot"), this);
  connect(actionStemPlot, SIGNAL(triggered()), this, SLOT(newStemPlot()));

  actionPlot2VerticalLayers = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("panel_v2_xpm")), tr("&Vertical 2 Layers"), this);
  connect(actionPlot2VerticalLayers, SIGNAL(triggered()), this,
          SLOT(plot2VerticalLayers()));

  actionPlot2HorizontalLayers = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("panel_h2_xpm")), tr("&Horizontal 2 Layers"), this);
  connect(actionPlot2HorizontalLayers, SIGNAL(triggered()), this,
          SLOT(plot2HorizontalLayers()));

  actionPlot4Layers = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("panel_4_xpm")), tr("&4 Layers"), this);
  connect(actionPlot4Layers, SIGNAL(triggered()), this, SLOT(plot4Layers()));

  actionPlotStackedLayers = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("stacked_xpm")), tr("&Stacked Layers"), this);
  connect(actionPlotStackedLayers, SIGNAL(triggered()), this,
          SLOT(plotStackedLayers()));

  actionPlot3DRibbon = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("ribbon_xpm")), tr("&Ribbon"), this);
  connect(actionPlot3DRibbon, SIGNAL(triggered()), this, SLOT(plot3DRibbon()));

  actionPlot3DBars = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("bars_xpm")), tr("&Bars"), this);
  connect(actionPlot3DBars, SIGNAL(triggered()), this, SLOT(plot3DBars()));

  actionPlot3DScatter = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("scatter_xpm")), tr("&Scatter"), this);
  connect(actionPlot3DScatter, SIGNAL(triggered()), this,
          SLOT(plot3DScatter()));

  actionPlot3DTrajectory = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("trajectory_xpm")), tr("&Trajectory"), this);
  connect(actionPlot3DTrajectory, SIGNAL(triggered()), this,
          SLOT(plot3DTrajectory()));

  actionShowColStatistics = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("col_stat_xpm")), tr("Statistics on &Columns"), this);
  connect(actionShowColStatistics, SIGNAL(triggered()), this,
          SLOT(showColStatistics()));

  actionShowRowStatistics = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("stat_rows_xpm")), tr("Statistics on &Rows"), this);
  connect(actionShowRowStatistics, SIGNAL(triggered()), this,
          SLOT(showRowStatistics()));

  actionIntegrate =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Integrate"), this);
  connect(actionIntegrate, SIGNAL(triggered()), this, SLOT(integrate()));

  actionShowIntDialog = new MantidQt::MantidWidgets::TrackedAction(
      tr("Integr&ate Function..."), this);
  connect(actionShowIntDialog, SIGNAL(triggered()), this,
          SLOT(showIntegrationDialog()));

  actionInterpolate =
      new MantidQt::MantidWidgets::TrackedAction(tr("Inte&rpolate ..."), this);
  connect(actionInterpolate, SIGNAL(triggered()), this,
          SLOT(showInterpolationDialog()));

  actionLowPassFilter =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Low Pass..."), this);
  connect(actionLowPassFilter, SIGNAL(triggered()), this,
          SLOT(lowPassFilterDialog()));

  actionHighPassFilter =
      new MantidQt::MantidWidgets::TrackedAction(tr("&High Pass..."), this);
  connect(actionHighPassFilter, SIGNAL(triggered()), this,
          SLOT(highPassFilterDialog()));

  actionBandPassFilter =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Band Pass..."), this);
  connect(actionBandPassFilter, SIGNAL(triggered()), this,
          SLOT(bandPassFilterDialog()));

  actionBandBlockFilter =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Band Block..."), this);
  connect(actionBandBlockFilter, SIGNAL(triggered()), this,
          SLOT(bandBlockFilterDialog()));

  actionFFT = new MantidQt::MantidWidgets::TrackedAction(tr("&FFT..."), this);
  connect(actionFFT, SIGNAL(triggered()), this, SLOT(showFFTDialog()));

  actionSmoothSavGol = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Savitzky-Golay..."), this);
  connect(actionSmoothSavGol, SIGNAL(triggered()), this,
          SLOT(showSmoothSavGolDialog()));

  actionSmoothFFT =
      new MantidQt::MantidWidgets::TrackedAction(tr("&FFT Filter..."), this);
  connect(actionSmoothFFT, SIGNAL(triggered()), this,
          SLOT(showSmoothFFTDialog()));

  actionSmoothAverage = new MantidQt::MantidWidgets::TrackedAction(
      tr("Moving Window &Average..."), this);
  connect(actionSmoothAverage, SIGNAL(triggered()), this,
          SLOT(showSmoothAverageDialog()));

  actionDifferentiate =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Differentiate"), this);
  connect(actionDifferentiate, SIGNAL(triggered()), this,
          SLOT(differentiate()));

  actionFitLinear =
      new MantidQt::MantidWidgets::TrackedAction(tr("Fit &Linear"), this);
  connect(actionFitLinear, SIGNAL(triggered()), this, SLOT(fitLinear()));

  actionShowFitPolynomDialog = new MantidQt::MantidWidgets::TrackedAction(
      tr("Fit &Polynomial ..."), this);
  connect(actionShowFitPolynomDialog, SIGNAL(triggered()), this,
          SLOT(showFitPolynomDialog()));

  actionShowExpDecayDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&First Order ..."), this);
  connect(actionShowExpDecayDialog, SIGNAL(triggered()), this,
          SLOT(showExpDecayDialog()));

  actionShowTwoExpDecayDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Second Order ..."), this);
  connect(actionShowTwoExpDecayDialog, SIGNAL(triggered()), this,
          SLOT(showTwoExpDecayDialog()));

  actionShowExpDecay3Dialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Third Order ..."), this);
  connect(actionShowExpDecay3Dialog, SIGNAL(triggered()), this,
          SLOT(showExpDecay3Dialog()));

  actionFitExpGrowth = new MantidQt::MantidWidgets::TrackedAction(
      tr("Fit Exponential Gro&wth ..."), this);
  connect(actionFitExpGrowth, SIGNAL(triggered()), this,
          SLOT(showExpGrowthDialog()));

  actionFitSigmoidal = new MantidQt::MantidWidgets::TrackedAction(
      tr("Fit &Boltzmann (Sigmoidal)"), this);
  connect(actionFitSigmoidal, SIGNAL(triggered()), this, SLOT(fitSigmoidal()));

  actionFitGauss =
      new MantidQt::MantidWidgets::TrackedAction(tr("Fit &Gaussian"), this);
  connect(actionFitGauss, SIGNAL(triggered()), this, SLOT(fitGauss()));

  actionFitLorentz =
      new MantidQt::MantidWidgets::TrackedAction(tr("Fit Lorent&zian"), this);
  connect(actionFitLorentz, SIGNAL(triggered()), this, SLOT(fitLorentz()));

  actionShowFitDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("Fit &Wizard..."), this);
  actionShowFitDialog->setShortcut(tr("Ctrl+Y"));
  connect(actionShowFitDialog, SIGNAL(triggered()), this,
          SLOT(showFitDialog()));

  actionShowPlotDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Plot ..."), this);
  connect(actionShowPlotDialog, SIGNAL(triggered()), this,
          SLOT(showGeneralPlotDialog()));

  actionShowScaleDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Scales..."), this);
  connect(actionShowScaleDialog, SIGNAL(triggered()), this,
          SLOT(showScaleDialog()));

  actionShowAxisDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Axes..."), this);
  connect(actionShowAxisDialog, SIGNAL(triggered()), this,
          SLOT(showAxisDialog()));

  actionShowGridDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Grid ..."), this);
  connect(actionShowGridDialog, SIGNAL(triggered()), this,
          SLOT(showGridDialog()));

  actionShowTitleDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Title ..."), this);
  connect(actionShowTitleDialog, SIGNAL(triggered()), this,
          SLOT(showTitleDialog()));

  actionShowColumnOptionsDialog = new MantidQt::MantidWidgets::TrackedAction(
      tr("Column &Options ..."), this);
  actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
  connect(actionShowColumnOptionsDialog, SIGNAL(triggered()), this,
          SLOT(showColumnOptionsDialog()));

  actionShowColumnValuesDialog = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("formula_xpm")), tr("Set Column &Values ..."), this);
  connect(actionShowColumnValuesDialog, SIGNAL(triggered()), this,
          SLOT(showColumnValuesDialog()));
  actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));

  actionTableRecalculate =
      new MantidQt::MantidWidgets::TrackedAction(tr("Recalculate"), this);
  actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
  connect(actionTableRecalculate, SIGNAL(triggered()), this,
          SLOT(recalculateTable()));

  actionHideSelectedColumns =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Hide Selected"), this);
  connect(actionHideSelectedColumns, SIGNAL(triggered()), this,
          SLOT(hideSelectedColumns()));

  actionShowAllColumns =
      new MantidQt::MantidWidgets::TrackedAction(tr("Sho&w All Columns"), this);
  connect(actionShowAllColumns, SIGNAL(triggered()), this,
          SLOT(showAllColumns()));

  actionSwapColumns = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("swap_columns_xpm")), tr("&Swap columns"), this);
  connect(actionSwapColumns, SIGNAL(triggered()), this, SLOT(swapColumns()));

  actionMoveColRight = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("move_col_right_xpm")), tr("Move &Right"), this);
  connect(actionMoveColRight, SIGNAL(triggered()), this,
          SLOT(moveColumnRight()));

  actionMoveColLeft = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("move_col_left_xpm")), tr("Move &Left"), this);
  connect(actionMoveColLeft, SIGNAL(triggered()), this, SLOT(moveColumnLeft()));

  actionMoveColFirst = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("move_col_first_xpm")), tr("Move to F&irst"), this);
  connect(actionMoveColFirst, SIGNAL(triggered()), this,
          SLOT(moveColumnFirst()));

  actionMoveColLast = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("move_col_last_xpm")), tr("Move to Las&t"), this);
  connect(actionMoveColLast, SIGNAL(triggered()), this, SLOT(moveColumnLast()));

  actionShowColsDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Columns..."), this);
  connect(actionShowColsDialog, SIGNAL(triggered()), this,
          SLOT(showColsDialog()));

  actionShowRowsDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Rows..."), this);
  connect(actionShowRowsDialog, SIGNAL(triggered()), this,
          SLOT(showRowsDialog()));

  actionDeleteRows = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Delete Rows Interval..."), this);
  connect(actionDeleteRows, SIGNAL(triggered()), this,
          SLOT(showDeleteRowsDialog()));

  actionAbout = new MantidQt::MantidWidgets::TrackedAction(
      tr("&About MantidPlot"), this); // Mantid
  actionAbout->setShortcut(tr("F1"));
  connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));

  actionShowHelp =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Help"), this);
  actionShowHelp->setShortcut(tr("Ctrl+H"));
  connect(actionShowHelp, SIGNAL(triggered()), this, SLOT(showHelp()));

  actionMantidConcepts =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Mantid Concepts"), this);
  connect(actionMantidConcepts, SIGNAL(triggered()), this,
          SLOT(showMantidConcepts()));

  actionMantidAlgorithms = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Algorithm Descriptions"), this);
  connect(actionMantidAlgorithms, SIGNAL(triggered()), this,
          SLOT(showalgorithmDescriptions()));

  actionmantidplotHelp =
      new MantidQt::MantidWidgets::TrackedAction(tr("&MantidPlot Help"), this);
  connect(actionmantidplotHelp, SIGNAL(triggered()), this,
          SLOT(showmantidplotHelp()));

  actionChooseHelpFolder = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Choose Help Folder..."), this);
  connect(actionChooseHelpFolder, SIGNAL(triggered()), this,
          SLOT(chooseHelpFolder()));

  actionRename =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Rename Window"), this);
  connect(actionRename, SIGNAL(triggered()), this, SLOT(rename()));

  actionCloseWindow = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("close_xpm")), tr("Close &Window"), this);
  actionCloseWindow->setShortcut(tr("Ctrl+W"));
  connect(actionCloseWindow, SIGNAL(triggered()), this,
          SLOT(closeActiveWindow()));

  actionAddColToTable = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("addCol_xpm")), tr("Add Column"), this);
  connect(actionAddColToTable, SIGNAL(triggered()), this,
          SLOT(addColToTable()));

  actionGoToRow =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Go to Row..."), this);
  actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));
  connect(actionGoToRow, SIGNAL(triggered()), this, SLOT(goToRow()));

  actionGoToColumn =
      new MantidQt::MantidWidgets::TrackedAction(tr("Go to Colum&n..."), this);
  actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));
  connect(actionGoToColumn, SIGNAL(triggered()), this, SLOT(goToColumn()));

  actionClearTable = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("erase_xpm"), tr("Clear"), this);
  connect(actionClearTable, SIGNAL(triggered()), this, SLOT(clearTable()));

  actionDeleteLayer = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("erase_xpm")), tr("&Remove Layer"), this);
  actionDeleteLayer->setShortcut(tr("Alt+R"));
  connect(actionDeleteLayer, SIGNAL(triggered()), this, SLOT(deleteLayer()));

  actionResizeActiveWindow = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("resize_xpm")), tr("Window &Geometry..."), this);
  connect(actionResizeActiveWindow, SIGNAL(triggered()), this,
          SLOT(resizeActiveWindow()));

  actionHideActiveWindow =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Hide Window"), this);
  connect(actionHideActiveWindow, SIGNAL(triggered()), this,
          SLOT(hideActiveWindow()));

  actionShowMoreWindows =
      new MantidQt::MantidWidgets::TrackedAction(tr("More windows..."), this);
  connect(actionShowMoreWindows, SIGNAL(triggered()), this,
          SLOT(showMoreWindows()));

  actionPixelLineProfile = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("pixelProfile_xpm")), tr("&View Pixel Line Profile"),
      this);
  connect(actionPixelLineProfile, SIGNAL(triggered()), this,
          SLOT(pixelLineProfile()));

  actionIntensityTable =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Intensity Table"), this);
  connect(actionIntensityTable, SIGNAL(triggered()), this,
          SLOT(intensityTable()));

  actionShowLineDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Properties"), this);
  connect(actionShowLineDialog, SIGNAL(triggered()), this,
          SLOT(showLineDialog()));

  actionShowImageDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Properties"), this);
  connect(actionShowImageDialog, SIGNAL(triggered()), this,
          SLOT(showImageDialog()));

  actionShowTextDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Properties"), this);
  connect(actionShowTextDialog, SIGNAL(triggered()), this,
          SLOT(showTextDialog()));

  actionActivateWindow =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Activate Window"), this);
  connect(actionActivateWindow, SIGNAL(triggered()), this,
          SLOT(activateWindow()));

  actionMinimizeWindow =
      new MantidQt::MantidWidgets::TrackedAction(tr("Mi&nimize Window"), this);
  connect(actionMinimizeWindow, SIGNAL(triggered()), this,
          SLOT(minimizeWindow()));

  actionMaximizeWindow =
      new MantidQt::MantidWidgets::TrackedAction(tr("Ma&ximize Window"), this);
  connect(actionMaximizeWindow, SIGNAL(triggered()), this,
          SLOT(maximizeWindow()));

  actionHideWindow =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Hide Window"), this);
  connect(actionHideWindow, SIGNAL(triggered()), this, SLOT(hideWindow()));

  actionResizeWindow = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("resize_xpm")), tr("Re&size Window..."), this);
  connect(actionResizeWindow, SIGNAL(triggered()), this, SLOT(resizeWindow()));

  actionEditSurfacePlot =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Surface..."), this);
  connect(actionEditSurfacePlot, SIGNAL(triggered()), this,
          SLOT(editSurfacePlot()));

  actionAdd3DData =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Data Set..."), this);
  connect(actionAdd3DData, SIGNAL(triggered()), this, SLOT(add3DData()));

  actionSetMatrixProperties = new MantidQt::MantidWidgets::TrackedAction(
      tr("Set &Properties..."), this);
  connect(actionSetMatrixProperties, SIGNAL(triggered()), this,
          SLOT(showMatrixDialog()));

  actionSetMatrixDimensions = new MantidQt::MantidWidgets::TrackedAction(
      tr("Set &Dimensions..."), this);
  connect(actionSetMatrixDimensions, SIGNAL(triggered()), this,
          SLOT(showMatrixSizeDialog()));
  actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));

  actionSetMatrixValues = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("formula_xpm")), tr("Set &Values..."), this);
  connect(actionSetMatrixValues, SIGNAL(triggered()), this,
          SLOT(showMatrixValuesDialog()));
  actionSetMatrixValues->setShortcut(tr("Alt+Q"));

  actionImagePlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("image_plot_xpm")), tr("&Image Plot"), this);
  connect(actionImagePlot, SIGNAL(triggered()), this, SLOT(plotImage()));

  actionTransposeMatrix =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Transpose"), this);
  connect(actionTransposeMatrix, SIGNAL(triggered()), this,
          SLOT(transposeMatrix()));

  actionFlipMatrixVertically = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("flip_vertical_xpm")), tr("Flip &V"), this);
  actionFlipMatrixVertically->setShortcut(tr("Ctrl+Shift+V"));
  connect(actionFlipMatrixVertically, SIGNAL(triggered()), this,
          SLOT(flipMatrixVertically()));

  actionFlipMatrixHorizontally = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("flip_horizontal_xpm")), tr("Flip &H"), this);
  actionFlipMatrixHorizontally->setShortcut(tr("Ctrl+Shift+H"));
  connect(actionFlipMatrixHorizontally, SIGNAL(triggered()), this,
          SLOT(flipMatrixHorizontally()));

  actionRotateMatrix = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("rotate_clockwise_xpm")), tr("R&otate 90"), this);
  actionRotateMatrix->setShortcut(tr("Ctrl+Shift+R"));
  connect(actionRotateMatrix, SIGNAL(triggered()), this,
          SLOT(rotateMatrix90()));

  actionRotateMatrixMinus = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("rotate_counterclockwise_xpm")), tr("Rotate &-90"),
      this);
  actionRotateMatrixMinus->setShortcut(tr("Ctrl+Alt+R"));
  connect(actionRotateMatrixMinus, SIGNAL(triggered()), this,
          SLOT(rotateMatrixMinus90()));

  actionInvertMatrix =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Invert"), this);
  connect(actionInvertMatrix, SIGNAL(triggered()), this, SLOT(invertMatrix()));

  actionMatrixDeterminant =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Determinant"), this);
  connect(actionMatrixDeterminant, SIGNAL(triggered()), this,
          SLOT(matrixDeterminant()));

  actionViewMatrixImage =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Image mode"), this);
  actionViewMatrixImage->setShortcut(tr("Ctrl+Shift+I"));
  connect(actionViewMatrixImage, SIGNAL(triggered()), this,
          SLOT(viewMatrixImage()));
  actionViewMatrixImage->setCheckable(true);

  actionViewMatrix =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Data mode"), this);
  actionViewMatrix->setShortcut(tr("Ctrl+Shift+D"));
  connect(actionViewMatrix, SIGNAL(triggered()), this, SLOT(viewMatrixTable()));
  actionViewMatrix->setCheckable(true);

  actionMatrixXY =
      new MantidQt::MantidWidgets::TrackedAction(tr("Show &X/Y"), this);
  actionMatrixXY->setShortcut(tr("Ctrl+Shift+X"));
  connect(actionMatrixXY, SIGNAL(triggered()), this, SLOT(viewMatrixXY()));
  actionMatrixXY->setCheckable(true);

  actionMatrixColumnRow =
      new MantidQt::MantidWidgets::TrackedAction(tr("Show &Column/Row"), this);
  actionMatrixColumnRow->setShortcut(tr("Ctrl+Shift+C"));
  connect(actionMatrixColumnRow, SIGNAL(triggered()), this,
          SLOT(viewMatrixColumnRow()));
  actionMatrixColumnRow->setCheckable(true);

  actionMatrixGrayScale =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Gray Scale"), this);
  connect(actionMatrixGrayScale, SIGNAL(triggered()), this,
          SLOT(setMatrixGrayScale()));
  actionMatrixGrayScale->setCheckable(true);

  actionMatrixRainbowScale =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Rainbow"), this);
  connect(actionMatrixRainbowScale, SIGNAL(triggered()), this,
          SLOT(setMatrixRainbowScale()));
  actionMatrixRainbowScale->setCheckable(true);

  actionMatrixCustomScale =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Custom"), this);
  connect(actionMatrixCustomScale, SIGNAL(triggered()), this,
          SLOT(showColorMapDialog()));
  actionMatrixCustomScale->setCheckable(true);

  actionExportMatrix =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Export Image ..."), this);
  connect(actionExportMatrix, SIGNAL(triggered()), this, SLOT(exportMatrix()));

  actionConvertMatrixDirect =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Direct"), this);
  connect(actionConvertMatrixDirect, SIGNAL(triggered()), this,
          SLOT(convertMatrixToTableDirect()));

  actionConvertMatrixXYZ =
      new MantidQt::MantidWidgets::TrackedAction(tr("&XYZ Columns"), this);
  connect(actionConvertMatrixXYZ, SIGNAL(triggered()), this,
          SLOT(convertMatrixToTableXYZ()));

  actionConvertMatrixYXZ =
      new MantidQt::MantidWidgets::TrackedAction(tr("&YXZ Columns"), this);
  connect(actionConvertMatrixYXZ, SIGNAL(triggered()), this,
          SLOT(convertMatrixToTableYXZ()));

  actionMatrixFFTDirect =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Forward FFT"), this);
  connect(actionMatrixFFTDirect, SIGNAL(triggered()), this,
          SLOT(matrixDirectFFT()));

  actionMatrixFFTInverse =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Inverse FFT"), this);
  connect(actionMatrixFFTInverse, SIGNAL(triggered()), this,
          SLOT(matrixInverseFFT()));

  actionConvertTable = new MantidQt::MantidWidgets::TrackedAction(
      tr("Convert to &Matrix"), this);
  connect(actionConvertTable, SIGNAL(triggered()), this,
          SLOT(convertTableToMatrix()));

  actionConvertTableToWorkspace = new MantidQt::MantidWidgets::TrackedAction(
      tr("Convert to Table&Workspace"), this);
  connect(actionConvertTableToWorkspace, SIGNAL(triggered()), this,
          SLOT(convertTableToWorkspace()));

  actionConvertTableToMatrixWorkspace =
      new MantidQt::MantidWidgets::TrackedAction(
          tr("Convert to MatrixWorkspace"), this);
  connect(actionConvertTableToMatrixWorkspace, SIGNAL(triggered()), this,
          SLOT(convertTableToMatrixWorkspace()));

  actionPlot3DWireFrame = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("lineMesh_xpm")), tr("3D &Wire Frame"), this);
  connect(actionPlot3DWireFrame, SIGNAL(triggered()), this,
          SLOT(plot3DWireframe()));

  actionPlot3DHiddenLine = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("grid_only_xpm")), tr("3D &Hidden Line"), this);
  connect(actionPlot3DHiddenLine, SIGNAL(triggered()), this,
          SLOT(plot3DHiddenLine()));

  actionPlot3DPolygons = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("no_grid_xpm")), tr("3D &Polygons"), this);
  connect(actionPlot3DPolygons, SIGNAL(triggered()), this,
          SLOT(plot3DPolygons()));

  actionPlot3DWireSurface = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("grid_poly_xpm")), tr("3D Wire &Surface"), this);
  connect(actionPlot3DWireSurface, SIGNAL(triggered()), this,
          SLOT(plot3DWireSurface()));

  actionColorMap = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("color_map_xpm")), tr("Contour - &Color Fill"), this);
  connect(actionColorMap, SIGNAL(triggered()), this, SLOT(plotColorMap()));

  actionContourMap = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("contour_map_xpm")), tr("Contour &Lines"), this);
  connect(actionContourMap, SIGNAL(triggered()), this, SLOT(plotContour()));

  actionGrayMap = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("gray_map_xpm")), tr("&Gray Scale Map"), this);
  connect(actionGrayMap, SIGNAL(triggered()), this, SLOT(plotGrayScale()));

  actionNoContourColorMap = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("color_map_xpm")), tr("Color &Fill"), this);
  connect(actionNoContourColorMap, SIGNAL(triggered()), this,
          SLOT(plotNoContourColorMap()));

  actionSortTable =
      new MantidQt::MantidWidgets::TrackedAction(tr("Sort Ta&ble"), this);
  connect(actionSortTable, SIGNAL(triggered()), this, SLOT(sortActiveTable()));

  actionSortSelection =
      new MantidQt::MantidWidgets::TrackedAction(tr("Sort Columns"), this);
  connect(actionSortSelection, SIGNAL(triggered()), this,
          SLOT(sortSelection()));

  actionNormalizeTable =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Table"), this);
  connect(actionNormalizeTable, SIGNAL(triggered()), this,
          SLOT(normalizeActiveTable()));

  actionNormalizeSelection =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Columns"), this);
  connect(actionNormalizeSelection, SIGNAL(triggered()), this,
          SLOT(normalizeSelection()));

  actionCorrelate =
      new MantidQt::MantidWidgets::TrackedAction(tr("Co&rrelate"), this);
  connect(actionCorrelate, SIGNAL(triggered()), this, SLOT(correlate()));

  actionAutoCorrelate =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Autocorrelate"), this);
  connect(actionAutoCorrelate, SIGNAL(triggered()), this,
          SLOT(autoCorrelate()));

  actionConvolute =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Convolute"), this);
  connect(actionConvolute, SIGNAL(triggered()), this, SLOT(convolute()));

  actionDeconvolute =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Deconvolute"), this);
  connect(actionDeconvolute, SIGNAL(triggered()), this, SLOT(deconvolute()));

  actionSetAscValues = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("rowNumbers_xpm")), tr("Ro&w Numbers"), this);
  connect(actionSetAscValues, SIGNAL(triggered()), this, SLOT(setAscValues()));

  actionSetRandomValues = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("randomNumbers_xpm")), tr("&Random Values"), this);
  connect(actionSetRandomValues, SIGNAL(triggered()), this,
          SLOT(setRandomValues()));

  actionReadOnlyCol =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Read Only"), this);
  connect(actionReadOnlyCol, SIGNAL(triggered()), this, SLOT(setReadOnlyCol()));

  actionSetXCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("x_col_xpm")), tr("&X"), this);
  connect(actionSetXCol, SIGNAL(triggered()), this, SLOT(setXCol()));

  actionSetYCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("y_col_xpm")), tr("&Y"), this);
  connect(actionSetYCol, SIGNAL(triggered()), this, SLOT(setYCol()));

  actionSetZCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("z_col_xpm")), tr("&Z"), this);
  connect(actionSetZCol, SIGNAL(triggered()), this, SLOT(setZCol()));

  actionSetXErrCol =
      new MantidQt::MantidWidgets::TrackedAction(tr("X E&rror"), this);
  connect(actionSetXErrCol, SIGNAL(triggered()), this, SLOT(setXErrCol()));

  actionSetYErrCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("errors_xpm")), tr("Y &Error"), this);
  connect(actionSetYErrCol, SIGNAL(triggered()), this, SLOT(setYErrCol()));

  actionDisregardCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("disregard_col_xpm")), tr("&Disregard"), this);
  connect(actionDisregardCol, SIGNAL(triggered()), this, SLOT(disregardCol()));

  actionSetLabelCol = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("set_label_col_xpm")), tr("&Label"), this);
  connect(actionSetLabelCol, SIGNAL(triggered()), this, SLOT(setLabelCol()));

  actionBoxPlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(getQPixmap("boxPlot_xpm")), tr("&Box Plot"), this);
  connect(actionBoxPlot, SIGNAL(triggered()), this, SLOT(plotBoxDiagram()));

  actionHomePage = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Mantid Homepage"), this); // Mantid change
  connect(actionHomePage, SIGNAL(triggered()), this, SLOT(showHomePage()));

  actionHelpBugReports =
      new MantidQt::MantidWidgets::TrackedAction(tr("Report a &Bug"), this);
  connect(actionHelpBugReports, SIGNAL(triggered()), this,
          SLOT(showBugTracker()));

  actionAskHelp =
      new MantidQt::MantidWidgets::TrackedAction(tr("Ask for Help"), this);
  connect(actionAskHelp, SIGNAL(triggered()), this, SLOT(showBugTracker()));

  actionShowCurvePlotDialog =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Plot details..."), this);
  connect(actionShowCurvePlotDialog, SIGNAL(triggered()), this,
          SLOT(showCurvePlotDialog()));

  actionShowCurveWorksheet =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Worksheet"), this);
  connect(actionShowCurveWorksheet, SIGNAL(triggered()), this,
          SLOT(showCurveWorksheet()));

  actionCurveFullRange = new MantidQt::MantidWidgets::TrackedAction(
      tr("&Reset to Full Range"), this);
  connect(actionCurveFullRange, SIGNAL(triggered()), this,
          SLOT(setCurveFullRange()));

  actionEditCurveRange =
      new MantidQt::MantidWidgets::TrackedAction(tr("Edit &Range..."), this);
  connect(actionEditCurveRange, SIGNAL(triggered()), this,
          SLOT(showCurveRangeDialog()));

  actionRemoveCurve = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("close_xpm"), tr("&Delete"), this);
  connect(actionRemoveCurve, SIGNAL(triggered()), this, SLOT(removeCurve()));

  actionHideCurve =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Hide"), this);
  connect(actionHideCurve, SIGNAL(triggered()), this, SLOT(hideCurve()));

  actionHideOtherCurves = new MantidQt::MantidWidgets::TrackedAction(
      tr("Hide &Other Curves"), this);
  connect(actionHideOtherCurves, SIGNAL(triggered()), this,
          SLOT(hideOtherCurves()));

  actionShowAllCurves =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Show All Curves"), this);
  connect(actionShowAllCurves, SIGNAL(triggered()), this,
          SLOT(showAllCurves()));

  actionEditFunction =
      new MantidQt::MantidWidgets::TrackedAction(tr("&Edit Function..."), this);
  connect(actionEditFunction, SIGNAL(triggered()), this,
          SLOT(showFunctionDialog()));

  actionFontBold = new MantidQt::MantidWidgets::TrackedAction("B", this);
  actionFontBold->setToolTip(tr("Bold"));
  QFont font = appFont;
  font.setBold(true);
  actionFontBold->setFont(font);
  actionFontBold->setCheckable(true);
  connect(actionFontBold, SIGNAL(toggled(bool)), this, SLOT(setBoldFont(bool)));

  actionFontItalic = new MantidQt::MantidWidgets::TrackedAction("It", this);
  actionFontItalic->setToolTip(tr("Italic"));
  font = appFont;
  font.setItalic(true);
  actionFontItalic->setFont(font);
  actionFontItalic->setCheckable(true);
  connect(actionFontItalic, SIGNAL(toggled(bool)), this,
          SLOT(setItalicFont(bool)));

  actionSuperscript = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("exp_xpm"), tr("Superscript"), this);
  connect(actionSuperscript, SIGNAL(triggered()), this,
          SLOT(insertSuperscript()));
  actionSuperscript->setEnabled(false);

  actionSubscript = new MantidQt::MantidWidgets::TrackedAction(
      getQPixmap("index_xpm"), tr("Subscript"), this);
  connect(actionSubscript, SIGNAL(triggered()), this, SLOT(insertSubscript()));
  actionSubscript->setEnabled(false);

  actionUnderline = new MantidQt::MantidWidgets::TrackedAction("U", this);
  actionUnderline->setToolTip(tr("Underline (Ctrl+U)"));
  actionUnderline->setShortcut(tr("Ctrl+U"));
  font = appFont;
  font.setUnderline(true);
  actionUnderline->setFont(font);
  connect(actionUnderline, SIGNAL(triggered()), this, SLOT(underline()));
  actionUnderline->setEnabled(false);

  actionGreekSymbol = new MantidQt::MantidWidgets::TrackedAction(
      QString(QChar(0x3B1)) + QString(QChar(0x3B2)), this);
  actionGreekSymbol->setToolTip(tr("Greek"));
  connect(actionGreekSymbol, SIGNAL(triggered()), this,
          SLOT(insertGreekSymbol()));

  actionGreekMajSymbol =
      new MantidQt::MantidWidgets::TrackedAction(QString(QChar(0x393)), this);
  actionGreekMajSymbol->setToolTip(tr("Greek"));
  connect(actionGreekMajSymbol, SIGNAL(triggered()), this,
          SLOT(insertGreekMajSymbol()));

  actionMathSymbol =
      new MantidQt::MantidWidgets::TrackedAction(QString(QChar(0x222B)), this);
  actionMathSymbol->setToolTip(tr("Mathematical Symbols"));
  connect(actionMathSymbol, SIGNAL(triggered()), this,
          SLOT(insertMathSymbol()));

  actionclearAllMemory =
      new MantidQt::MantidWidgets::TrackedAction("&Clear All Memory", this);
  actionclearAllMemory->setShortcut(QKeySequence::fromString("Ctrl+Shift+L"));
  connect(actionclearAllMemory, SIGNAL(triggered()), mantidUI,
          SLOT(clearAllMemory()));

  actionPanPlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/panning.png"), tr("Panning tool"), this);
  connect(actionPanPlot, SIGNAL(triggered()), this, SLOT(panOnPlot()));

  actionCatalogLogin =
      new MantidQt::MantidWidgets::TrackedAction("Login", this);
  actionCatalogLogin->setToolTip(tr("Catalog Login"));
  connect(actionCatalogLogin, SIGNAL(triggered()), this, SLOT(CatalogLogin()));

  actionCatalogSearch =
      new MantidQt::MantidWidgets::TrackedAction("Search", this);
  actionCatalogSearch->setToolTip(tr("Search data in archives."));
  connect(actionCatalogSearch, SIGNAL(triggered()), this,
          SLOT(CatalogSearch()));

  actionCatalogPublish =
      new MantidQt::MantidWidgets::TrackedAction("Publish", this);
  actionCatalogPublish->setToolTip(tr("Publish data to the archives."));
  connect(actionCatalogPublish, SIGNAL(triggered()), this,
          SLOT(CatalogPublish()));

  actionCatalogLogout =
      new MantidQt::MantidWidgets::TrackedAction("Logout", this);
  actionCatalogLogout->setToolTip(tr("Catalog Logout"));
  connect(actionCatalogLogout, SIGNAL(triggered()), this,
          SLOT(CatalogLogout()));

  actionWaterfallPlot = new MantidQt::MantidWidgets::TrackedAction(
      QIcon(":/waterfall_plot.png"), tr("&Waterfall Plot"), this);
  connect(actionWaterfallPlot, SIGNAL(triggered()), this,
          SLOT(waterfallPlot()));
}

void ApplicationWindow::translateActionsStrings() {
  actionFontBold->setToolTip(tr("Bold"));
  actionFontItalic->setToolTip(tr("Italic"));
  actionUnderline->setStatusTip(tr("Underline (Ctrl+U)"));
  actionUnderline->setShortcut(tr("Ctrl+U"));
  actionGreekSymbol->setToolTip(tr("Greek"));
  actionGreekMajSymbol->setToolTip(tr("Greek"));
  actionMathSymbol->setToolTip(tr("Mathematical Symbols"));

  actionShowCurvePlotDialog->setText(tr("&Plot details..."));
  actionShowCurveWorksheet->setText(tr("&Worksheet"));
  actionRemoveCurve->setText(tr("&Delete"));
  actionEditFunction->setText(tr("&Edit Function..."));

  actionCurveFullRange->setText(tr("&Reset to Full Range"));
  actionEditCurveRange->setText(tr("Edit &Range..."));
  actionHideCurve->setText(tr("&Hide"));
  actionHideOtherCurves->setText(tr("Hide &Other Curves"));
  actionShowAllCurves->setText(tr("&Show All Curves"));

  actionNewProject->setText(tr("New &Project"));
  actionNewProject->setToolTip(tr("Open a New Project"));
  actionNewProject->setShortcut(tr("Ctrl+N"));

  actionNewGraph->setText(tr("New &Graph"));
  actionNewGraph->setToolTip(tr("Create an empty 2D plot"));
  actionNewGraph->setShortcut(tr("Ctrl+G"));

  actionNewNote->setText(tr("New &Note"));
  actionNewNote->setToolTip(tr("Create an empty note window"));

  actionNewTable->setText(tr("New &Table"));
  actionNewTable->setShortcut(tr("Ctrl+T"));
  actionNewTable->setToolTip(tr("New table"));

  actionNewTiledWindow->setText(tr("New Tiled &Window"));
  actionNewTiledWindow->setShortcut(tr("Ctrl+Shift+T"));
  actionNewTiledWindow->setToolTip(tr("New tiled window"));

  actionNewMatrix->setText(tr("New &Matrix"));
  actionNewMatrix->setShortcut(tr("Ctrl+M"));
  actionNewMatrix->setToolTip(tr("New matrix"));

  actionNewFunctionPlot->setText(tr("New &Function Plot"));
  actionNewFunctionPlot->setToolTip(tr("Create a new 2D function plot"));

  actionNewSurfacePlot->setText(tr("New 3D &Surface Plot"));
  actionNewSurfacePlot->setToolTip(tr("Create a new 3D surface plot"));
  actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));

  actionOpenProj->setText(tr("&Project"));
  actionOpenProj->setShortcut(tr("Ctrl+Shift+O"));
  actionOpenProj->setToolTip(tr("Load Mantid Project"));

  actionLoadFile->setText(tr("&File"));
  actionLoadFile->setShortcut(tr("Ctrl+Shift+F"));
  actionLoadFile->setToolTip(tr("Load Data File"));

  actionLoadImage->setText(tr("Open Image &File"));
  actionLoadImage->setShortcut(tr("Ctrl+I"));

  actionImportImage->setText(tr("Import I&mage..."));

  actionSaveFile->setText(tr("&Nexus"));
  actionSaveFile->setToolTip(tr("Save as NeXus file"));
  actionSaveFile->setShortcut(tr("Ctrl+S"));

  actionSaveProject->setText(tr("&Project"));
  actionSaveProject->setToolTip(tr("Save Mantid Project"));
  actionSaveProject->setShortcut(tr("Ctrl+Shift+S"));

  actionSaveProjectAs->setText(tr("Save Project &As..."));
  actionSaveProjectAs->setToolTip(
      tr("Save Mantid Project using a different name or path"));

  actionLoad->setText(tr("&Import ASCII..."));
  actionLoad->setToolTip(tr("Import data file(s)"));
  actionLoad->setShortcut(tr("Ctrl+K"));

  actionCopyWindow->setText(tr("&Duplicate"));
  actionCopyWindow->setToolTip(tr("Duplicate window"));

  actionCutSelection->setText(tr("Cu&t Selection"));
  actionCutSelection->setToolTip(tr("Cut selection"));
  actionCutSelection->setShortcut(tr("Ctrl+X"));

  actionCopySelection->setText(tr("&Copy Selection"));
  actionCopySelection->setToolTip(tr("Copy Selection"));
  actionCopySelection->setShortcut(tr("Ctrl+C"));

  actionPasteSelection->setText(tr("&Paste Selection"));
  actionPasteSelection->setToolTip(tr("Paste Selection"));
  actionPasteSelection->setShortcut(tr("Ctrl+V"));

  actionClearSelection->setText(tr("&Delete Selection"));
  actionClearSelection->setToolTip(tr("Delete selection"));
  actionClearSelection->setShortcut(tr("Del", "delete key"));

  actionShowExplorer->setText(tr("Project &Explorer"));
  actionShowExplorer->setShortcut(tr("Ctrl+E"));
  actionShowExplorer->setToolTip(tr("Show project explorer"));

  actionShowLog->setText(tr("Results &Log"));
  actionShowLog->setToolTip(tr("Results Log"));

#ifdef SCRIPTING_PYTHON
  actionShowScriptWindow->setText(tr("&Script Window"));
  actionShowScriptWindow->setToolTip(tr("Script Window"));
#endif

  actionCustomActionDialog->setText(tr("Manage Custom Menus..."));

  actionAddLayer->setText(tr("Add La&yer"));
  actionAddLayer->setToolTip(tr("Add Layer"));
  actionAddLayer->setShortcut(tr("Alt+L"));

  actionShowLayerDialog->setText(tr("Arran&ge Layers"));
  actionShowLayerDialog->setToolTip(tr("Arrange Layers"));
  actionShowLayerDialog->setShortcut(tr("Alt+A"));

  actionAutomaticLayout->setText(tr("Automatic Layout"));
  actionAutomaticLayout->setToolTip(tr("Automatic Layout"));

  actionExportGraph->setText(tr("&Current"));
  actionExportGraph->setShortcut(tr("Alt+G"));
  actionExportGraph->setToolTip(tr("Export current graph"));

  actionExportAllGraphs->setText(tr("&All"));
  actionExportAllGraphs->setShortcut(tr("Alt+X"));
  actionExportAllGraphs->setToolTip(tr("Export all graphs"));

  actionExportPDF->setText(tr("&Export PDF"));
  actionExportPDF->setShortcut(tr("Ctrl+Alt+P"));
  actionExportPDF->setToolTip(tr("Export to PDF"));

  actionPrint->setText(tr("&Print"));
  actionPrint->setShortcut(tr("Ctrl+P"));
  actionPrint->setToolTip(tr("Print window"));

  actionPrintAllPlots->setText(tr("Print All Plo&ts"));
  actionShowExportASCIIDialog->setText(tr("E&xport ASCII"));

  actionCloseAllWindows->setText(tr("&Quit"));
  actionCloseAllWindows->setShortcut(tr("Ctrl+Q"));

  actionDeleteFitTables->setText(tr("Delete &Fit Tables"));
  actionShowPlotWizard->setText(tr("Plot &Wizard"));
  actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));

  actionShowConfigureDialog->setText(tr("&Preferences..."));

  actionShowCurvesDialog->setText(tr("Add/Remove &Curve..."));
  actionShowCurvesDialog->setShortcut(tr("Ctrl+Alt+C"));
  actionShowCurvesDialog->setToolTip(tr("Add curve to graph"));

  actionAddErrorBars->setText(tr("Add &Error Bars..."));
  actionAddErrorBars->setToolTip(tr("Add Error Bars..."));
  actionAddErrorBars->setShortcut(tr("Ctrl+Alt+E"));

  actionRemoveErrorBars->setText(tr("&Remove Error Bars..."));
  actionRemoveErrorBars->setToolTip(tr("Remove Error Bars..."));
  actionRemoveErrorBars->setShortcut(tr("Ctrl+Alt+R"));

  actionAddFunctionCurve->setText(tr("Add &Function..."));
  actionAddFunctionCurve->setToolTip(tr("Add Function..."));
  actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));

  actionUnzoom->setText(tr("&Rescale to Show All"));
  actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
  actionUnzoom->setToolTip(tr("Rescale to Show All"));

  actionNewLegend->setText(tr("Add New &Legend"));
  actionNewLegend->setShortcut(tr("Ctrl+Alt+L"));
  actionNewLegend->setToolTip(tr("Add New Legend"));

  actionTimeStamp->setText(tr("Add Time &Stamp"));
  actionTimeStamp->setShortcut(tr("Ctrl+Alt+S"));
  actionTimeStamp->setToolTip(tr("Date & time "));

  actionAddImage->setText(tr("Add &Image"));
  actionAddImage->setToolTip(tr("Add Image"));
  actionAddImage->setShortcut(tr("Ctrl+Alt+I"));

  actionPlotL->setText(tr("&Line"));
  actionPlotL->setToolTip(tr("Plot as line"));

  actionPlotP->setText(tr("&Scatter"));
  actionPlotP->setToolTip(tr("Plot as symbols"));

  actionPlotLP->setText(tr("Line + S&ymbol"));
  actionPlotLP->setToolTip(tr("Plot as line + symbols"));

  actionPlotVerticalDropLines->setText(tr("Vertical &Drop Lines"));

  actionPlotSpline->setText(tr("&Spline"));
  actionPlotVertSteps->setText(tr("&Vertical Steps"));
  actionPlotHorSteps->setText(tr("&Horizontal Steps"));

  actionPlotVerticalBars->setText(tr("&Columns"));
  actionPlotVerticalBars->setToolTip(tr("Plot with vertical bars"));

  actionPlotHorizontalBars->setText(tr("&Rows"));
  actionPlotHorizontalBars->setToolTip(tr("Plot with horizontal bars"));

  actionPlotArea->setText(tr("&Area"));
  actionPlotArea->setToolTip(tr("Plot area"));

  actionPlotPie->setText(tr("&Pie"));
  actionPlotPie->setToolTip(tr("Plot pie"));

  actionPlotVectXYXY->setText(tr("&Vectors XYXY"));
  actionPlotVectXYXY->setToolTip(tr("Vectors XYXY"));

  actionPlotVectXYAM->setText(tr("Vectors XY&AM"));
  actionPlotVectXYAM->setToolTip(tr("Vectors XYAM"));

  actionPlotHistogram->setText(tr("&Histogram"));
  actionPlotStackedHistograms->setText(tr("&Stacked Histogram"));
  actionPlot2VerticalLayers->setText(tr("&Vertical 2 Layers"));
  actionPlot2HorizontalLayers->setText(tr("&Horizontal 2 Layers"));
  actionPlot4Layers->setText(tr("&4 Layers"));
  actionPlotStackedLayers->setText(tr("&Stacked Layers"));

  actionStemPlot->setText(tr("Stem-and-&Leaf Plot"));
  actionStemPlot->setToolTip(tr("Stem-and-Leaf Plot"));

  actionPlot3DRibbon->setText(tr("&Ribbon"));
  actionPlot3DRibbon->setToolTip(tr("Plot 3D ribbon"));

  actionPlot3DBars->setText(tr("&Bars"));
  actionPlot3DBars->setToolTip(tr("Plot 3D bars"));

  actionPlot3DScatter->setText(tr("&Scatter"));
  actionPlot3DScatter->setToolTip(tr("Plot 3D scatter"));

  actionPlot3DTrajectory->setText(tr("&Trajectory"));
  actionPlot3DTrajectory->setToolTip(tr("Plot 3D trajectory"));

  actionColorMap->setText(tr("Contour + &Color Fill"));
  actionColorMap->setToolTip(tr("Contour Lines + Color Fill"));

  actionNoContourColorMap->setText(tr("Color &Fill"));
  actionNoContourColorMap->setToolTip(tr("Color Fill (No contours)"));

  actionContourMap->setText(tr("Contour &Lines"));
  actionContourMap->setToolTip(tr("Contour Lines"));

  actionGrayMap->setText(tr("&Gray Scale Map"));
  actionGrayMap->setToolTip(tr("Gray Scale Map"));

  actionShowColStatistics->setText(tr("Statistics on &Columns"));
  actionShowColStatistics->setToolTip(tr("Selected columns statistics"));

  actionShowRowStatistics->setText(tr("Statistics on &Rows"));
  actionShowRowStatistics->setToolTip(tr("Selected rows statistics"));
  actionShowIntDialog->setText(tr("Integr&ate Function..."));
  actionIntegrate->setText(tr("&Integrate"));
  actionInterpolate->setText(tr("Inte&rpolate ..."));
  actionLowPassFilter->setText(tr("&Low Pass..."));
  actionHighPassFilter->setText(tr("&High Pass..."));
  actionBandPassFilter->setText(tr("&Band Pass..."));
  actionBandBlockFilter->setText(tr("&Band Block..."));
  actionFFT->setText(tr("&FFT..."));
  actionSmoothSavGol->setText(tr("&Savitzky-Golay..."));
  actionSmoothFFT->setText(tr("&FFT Filter..."));
  actionSmoothAverage->setText(tr("Moving Window &Average..."));
  actionDifferentiate->setText(tr("&Differentiate"));
  actionFitLinear->setText(tr("Fit &Linear"));
  actionShowFitPolynomDialog->setText(tr("Fit &Polynomial ..."));
  actionShowExpDecayDialog->setText(tr("&First Order ..."));
  actionShowTwoExpDecayDialog->setText(tr("&Second Order ..."));
  actionShowExpDecay3Dialog->setText(tr("&Third Order ..."));
  actionFitExpGrowth->setText(tr("Fit Exponential Gro&wth ..."));
  actionFitSigmoidal->setText(tr("Fit &Boltzmann (Sigmoidal)"));
  actionFitGauss->setText(tr("Fit &Gaussian"));
  actionFitLorentz->setText(tr("Fit Lorent&zian"));

  actionShowFitDialog->setText(tr("Fit &Wizard..."));
  actionShowFitDialog->setShortcut(tr("Ctrl+Y"));

  actionShowPlotDialog->setText(tr("&Plot ..."));
  actionShowScaleDialog->setText(tr("&Scales..."));
  actionShowAxisDialog->setText(tr("&Axes..."));
  actionShowGridDialog->setText(tr("&Grid ..."));
  actionShowTitleDialog->setText(tr("&Title ..."));
  actionShowColumnOptionsDialog->setText(tr("Column &Options ..."));
  actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
  actionShowColumnValuesDialog->setText(
      tr("Set Column &Values ...")); // Removed JZ May 3, 2011
  actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));
  actionTableRecalculate->setText(tr("Recalculate"));
  actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
  actionHideSelectedColumns->setText(tr("&Hide Selected"));
  actionHideSelectedColumns->setToolTip(tr("Hide selected columns"));
  actionShowAllColumns->setText(tr("Sho&w All Columns"));
  actionHideSelectedColumns->setToolTip(tr("Show all table columns"));
  actionSwapColumns->setText(tr("&Swap columns"));
  actionSwapColumns->setToolTip(tr("Swap selected columns"));
  actionMoveColRight->setText(tr("Move &Right"));
  actionMoveColRight->setToolTip(tr("Move Right"));
  actionMoveColLeft->setText(tr("Move &Left"));
  actionMoveColLeft->setToolTip(tr("Move Left"));
  actionMoveColFirst->setText(tr("Move to F&irst"));
  actionMoveColFirst->setToolTip(tr("Move to First"));
  actionMoveColLast->setText(tr("Move to Las&t"));
  actionMoveColLast->setToolTip(tr("Move to Last"));
  actionShowColsDialog->setText(tr("&Columns..."));
  actionShowRowsDialog->setText(tr("&Rows..."));
  actionDeleteRows->setText(tr("&Delete Rows Interval..."));

  actionAbout->setText(tr("&About MantidPlot")); // Mantid
  actionAbout->setShortcut(tr("F1"));

  actionMantidConcepts->setText(tr("&Mantid Concepts"));

  actionMantidAlgorithms->setText("&Algorithm Descriptions");

  actionmantidplotHelp->setText("&MantidPlot Help");

  actionCloseWindow->setText(tr("Close &Window"));
  actionCloseWindow->setShortcut(tr("Ctrl+W"));

  actionAddColToTable->setText(tr("Add Column"));
  actionAddColToTable->setToolTip(tr("Add Column"));

  actionClearTable->setText(tr("Clear"));
  actionGoToRow->setText(tr("&Go to Row..."));
  actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));

  actionGoToColumn->setText(tr("Go to Colum&n..."));
  actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));

  actionDeleteLayer->setText(tr("&Remove Layer"));
  actionDeleteLayer->setShortcut(tr("Alt+R"));

  actionResizeActiveWindow->setText(tr("Window &Geometry..."));
  actionHideActiveWindow->setText(tr("&Hide Window"));
  actionShowMoreWindows->setText(tr("More Windows..."));
  actionPixelLineProfile->setText(tr("&View Pixel Line Profile"));
  actionIntensityTable->setText(tr("&Intensity Table"));
  actionShowLineDialog->setText(tr("&Properties"));
  actionShowImageDialog->setText(tr("&Properties"));
  actionShowTextDialog->setText(tr("&Properties"));
  actionActivateWindow->setText(tr("&Activate Window"));
  actionMinimizeWindow->setText(tr("Mi&nimize Window"));
  actionMaximizeWindow->setText(tr("Ma&ximize Window"));
  actionHideWindow->setText(tr("&Hide Window"));
  actionResizeWindow->setText(tr("Re&size Window..."));
  actionEditSurfacePlot->setText(tr("&Surface..."));
  actionAdd3DData->setText(tr("&Data Set..."));
  actionSetMatrixProperties->setText(tr("Set &Properties..."));
  actionSetMatrixDimensions->setText(tr("Set &Dimensions..."));

  actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));
  actionSetMatrixValues->setText(tr("Set &Values..."));
  actionSetMatrixValues->setToolTip(tr("Set Matrix Values"));
  actionSetMatrixValues->setShortcut(tr("Alt+Q"));
  actionImagePlot->setText(tr("&Image Plot"));
  actionImagePlot->setToolTip(tr("Image Plot"));
  actionTransposeMatrix->setText(tr("&Transpose"));
  actionRotateMatrix->setText(tr("R&otate 90"));
  actionRotateMatrix->setToolTip(tr("Rotate 90° Clockwise"));
  actionRotateMatrixMinus->setText(tr("Rotate &-90"));
  actionRotateMatrixMinus->setToolTip(tr("Rotate 90° Counterclockwise"));
  actionFlipMatrixVertically->setText(tr("Flip &V"));
  actionFlipMatrixVertically->setToolTip(tr("Flip Vertically"));
  actionFlipMatrixHorizontally->setText(tr("Flip &H"));
  actionFlipMatrixHorizontally->setToolTip(tr("Flip Horizontally"));

  actionMatrixXY->setText(tr("Show &X/Y"));
  actionMatrixColumnRow->setText(tr("Show &Column/Row"));
  actionViewMatrix->setText(tr("&Data mode"));
  actionViewMatrixImage->setText(tr("&Image mode"));
  actionMatrixGrayScale->setText(tr("&Gray Scale"));
  actionMatrixRainbowScale->setText(tr("&Rainbow"));
  actionMatrixCustomScale->setText(tr("&Custom"));
  actionInvertMatrix->setText(tr("&Invert"));
  actionMatrixDeterminant->setText(tr("&Determinant"));
  actionConvertMatrixDirect->setText(tr("&Direct"));
  actionConvertMatrixXYZ->setText(tr("&XYZ Columns"));
  actionConvertMatrixYXZ->setText(tr("&YXZ Columns"));
  actionExportMatrix->setText(tr("&Export Image ..."));

  actionConvertTable->setText(tr("Convert to &Matrix"));
  actionConvertTableToWorkspace->setText(tr("Convert to Table&Workspace"));
  actionConvertTableToMatrixWorkspace->setText(
      tr("Convert to MatrixWorkspace"));
  actionPlot3DWireFrame->setText(tr("3D &Wire Frame"));
  actionPlot3DHiddenLine->setText(tr("3D &Hidden Line"));
  actionPlot3DPolygons->setText(tr("3D &Polygons"));
  actionPlot3DWireSurface->setText(tr("3D Wire &Surface"));
  actionSortTable->setText(tr("Sort Ta&ble"));
  actionSortSelection->setText(tr("Sort Columns"));
  actionNormalizeTable->setText(tr("&Table"));
  actionNormalizeSelection->setText(tr("&Columns"));
  actionCorrelate->setText(tr("Co&rrelate"));
  actionAutoCorrelate->setText(tr("&Autocorrelate"));
  actionConvolute->setText(tr("&Convolute"));
  actionDeconvolute->setText(tr("&Deconvolute"));
  actionSetAscValues->setText(tr("Ro&w Numbers"));
  actionSetAscValues->setToolTip(tr("Fill selected columns with row numbers"));
  actionSetRandomValues->setText(tr("&Random Values"));
  actionSetRandomValues->setToolTip(
      tr("Fill selected columns with random numbers"));
  actionSetXCol->setText(tr("&X"));
  actionSetXCol->setToolTip(tr("Set column as X"));
  actionSetYCol->setText(tr("&Y"));
  actionSetYCol->setToolTip(tr("Set column as Y"));
  actionSetZCol->setText(tr("&Z"));
  actionSetZCol->setToolTip(tr("Set column as Z"));
  actionSetXErrCol->setText(tr("X E&rror"));
  actionSetYErrCol->setText(tr("Y &Error"));
  actionSetYErrCol->setToolTip(tr("Set as Y Error Bars"));
  actionSetLabelCol->setText(tr("&Label"));
  actionSetLabelCol->setToolTip(tr("Set as Labels"));
  actionDisregardCol->setText(tr("&Disregard"));
  actionDisregardCol->setToolTip(tr("Disregard Columns"));
  actionReadOnlyCol->setText(tr("&Read Only"));

  actionBoxPlot->setText(tr("&Box Plot"));
  actionBoxPlot->setToolTip(tr("Box and whiskers plot"));

  actionHomePage->setText(tr("&Mantid Homepage")); // Mantid change
  actionHelpBugReports->setText(tr("Report a &Bug"));
  actionAskHelp->setText(tr("Ask for Help"));

  btnPointer->setText(tr("Selection &Tools"));
  btnPointer->setToolTip(tr("Selection Tools"));

  btnZoomIn->setText(tr("&Zoom In"));
  btnZoomIn->setShortcut(tr("Ctrl++"));
  btnZoomIn->setToolTip(tr("Zoom In"));

  btnZoomOut->setText(tr("Zoom &Out"));
  btnZoomOut->setShortcut(tr("Ctrl+-"));
  btnZoomOut->setToolTip(tr("Zoom Out"));

  actionPanPlot->setText(tr("Panning Tool (zoom with mouse wheel)"));
  actionPanPlot->setToolTip(tr("Panning Tool (zoom with mouse wheel)"));

  btnCursor->setText(tr("&Data Reader"));
  btnCursor->setShortcut(tr("CTRL+D"));
  btnCursor->setToolTip(tr("Data Reader"));

  btnPicker->setText(tr("S&creen Reader"));
  btnPicker->setToolTip(tr("Screen reader"));

  btnLabel->setText(tr("Add &Label"));
  btnLabel->setToolTip(tr("Add Label"));

  actionDrawPoints->setText(tr("&Draw Data Points"));
  actionDrawPoints->setToolTip(tr("Draw Data Points"));

  btnMovePoints->setText(tr("&Move Data Points..."));
  btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
  btnMovePoints->setToolTip(tr("Move data points"));

  btnRemovePoints->setText(tr("Remove &Bad Data Points..."));
  btnRemovePoints->setShortcut(tr("Alt+B"));
  btnRemovePoints->setToolTip(tr("Remove data points"));

  btnArrow->setText(tr("Draw &Arrow"));
  btnArrow->setShortcut(tr("Ctrl+Alt+A"));
  btnArrow->setToolTip(tr("Draw Arrow"));

  btnLine->setText(tr("Draw Li&ne"));
  btnLine->setShortcut(tr("CtrL+Alt+N"));
  btnLine->setToolTip(tr("Draw Line"));

  // FIXME: is setText necessary for action groups?
  Box->setText(tr("Box"));
  Box->setText(tr("Box"));
  Box->setToolTip(tr("Box"));
  Box->setStatusTip(tr("Box"));
  Frame->setText(tr("Frame"));
  Frame->setText(tr("&Frame"));
  Frame->setToolTip(tr("Frame"));
  Frame->setStatusTip(tr("Frame"));
  None->setText(tr("No Axes"));
  None->setText(tr("No Axes"));
  None->setToolTip(tr("No axes"));
  None->setStatusTip(tr("No axes"));

  front->setToolTip(tr("Front grid"));
  back->setToolTip(tr("Back grid"));
  right->setToolTip(tr("Right grid"));
  left->setToolTip(tr("Left grid"));
  ceil->setToolTip(tr("Ceiling grid"));
  floor->setToolTip(tr("Floor grid"));

  wireframe->setText(tr("Wireframe"));
  wireframe->setText(tr("Wireframe"));
  wireframe->setToolTip(tr("Wireframe"));
  wireframe->setStatusTip(tr("Wireframe"));
  hiddenline->setText(tr("Hidden Line"));
  hiddenline->setText(tr("Hidden Line"));
  hiddenline->setToolTip(tr("Hidden line"));
  hiddenline->setStatusTip(tr("Hidden line"));
  polygon->setText(tr("Polygon Only"));
  polygon->setText(tr("Polygon Only"));
  polygon->setToolTip(tr("Polygon only"));
  polygon->setStatusTip(tr("Polygon only"));
  filledmesh->setText(tr("Mesh & Filled Polygons"));
  filledmesh->setText(tr("Mesh & Filled Polygons"));
  filledmesh->setToolTip(tr("Mesh & filled Polygons"));
  filledmesh->setStatusTip(tr("Mesh & filled Polygons"));
  pointstyle->setText(tr("Dots"));
  pointstyle->setText(tr("Dots"));
  pointstyle->setToolTip(tr("Dots"));
  pointstyle->setStatusTip(tr("Dots"));
  barstyle->setText(tr("Bars"));
  barstyle->setText(tr("Bars"));
  barstyle->setToolTip(tr("Bars"));
  barstyle->setStatusTip(tr("Bars"));
  conestyle->setText(tr("Cones"));
  conestyle->setText(tr("Cones"));
  conestyle->setToolTip(tr("Cones"));
  conestyle->setStatusTip(tr("Cones"));
  crossHairStyle->setText(tr("Crosshairs"));
  crossHairStyle->setToolTip(tr("Crosshairs"));
  crossHairStyle->setStatusTip(tr("Crosshairs"));

  floordata->setText(tr("Floor Data Projection"));
  floordata->setToolTip(tr("Floor data projection"));
  floordata->setStatusTip(tr("Floor data projection"));

  flooriso->setText(tr("Floor Isolines"));
  flooriso->setToolTip(tr("Floor isolines"));
  flooriso->setStatusTip(tr("Floor isolines"));

  floornone->setText(tr("Empty Floor"));
  floornone->setToolTip(tr("Empty floor"));
  floornone->setStatusTip(tr("Empty floor"));

  actionAnimate->setText(tr("Animation"));
  actionAnimate->setToolTip(tr("Animation"));
  actionAnimate->setStatusTip(tr("Animation"));

  actionPerspective->setText(tr("Enable perspective"));
  actionPerspective->setToolTip(tr("Enable perspective"));
  actionPerspective->setStatusTip(tr("Enable perspective"));

  actionResetRotation->setText(tr("Reset rotation"));
  actionResetRotation->setToolTip(tr("Reset rotation"));
  actionResetRotation->setStatusTip(tr("Reset rotation"));

  actionFitFrame->setText(tr("Fit frame to window"));
  actionFitFrame->setToolTip(tr("Fit frame to window"));
  actionFitFrame->setStatusTip(tr("Fit frame to window"));

  actionWaterfallPlot->setText(tr("&Waterfall Plot"));
  actionWaterfallPlot->setToolTip(tr("Waterfall Plot"));
}

Graph3D *ApplicationWindow::openMatrixPlot3D(const QString &caption,
                                             const QString &matrix_name,
                                             double xl, double xr, double yl,
                                             double yr, double zl, double zr) {
  QString name = matrix_name;
  name.remove("matrix<", Qt::CaseSensitive);
  name.remove(">", Qt::CaseSensitive);
  Matrix *m = matrix(name);
  if (!m)
    return nullptr;

  Graph3D *plot = new Graph3D("", this, nullptr, nullptr);
  plot->setWindowTitle(caption);
  plot->setName(caption);
  plot->addMatrixData(m, xl, xr, yl, yr, zl, zr);
  plot->update();

  initPlot3D(plot);
  return plot;
}

Graph3D *ApplicationWindow::plot3DMatrix(Matrix *m, int style) {
  if (!m) {
    // Mantid
    Graph3D *plot = mantidUI->plot3DMatrix(style);
    if (plot)
      return plot;
    m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
    if (!m)
      return nullptr;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString label = generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", this, nullptr);
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

MultiLayer *ApplicationWindow::plotGrayScale(Matrix *m) {
  if (!m) {
    // Mantid
    MultiLayer *plot = mantidUI->plotSpectrogram(GraphOptions::GrayScale);
    if (plot)
      return plot;
    m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
    if (!m)
      return nullptr;
  }

  return plotSpectrogram(m, GraphOptions::GrayScale);
}

MultiLayer *ApplicationWindow::plotContour(Matrix *m) {
  if (!m) {
    // Mantid
    MultiLayer *plot = mantidUI->plotSpectrogram(GraphOptions::Contour);
    if (plot)
      return plot;
    m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
    if (!m)
      return nullptr;
  }

  return plotSpectrogram(m, GraphOptions::Contour);
}

MultiLayer *ApplicationWindow::plotColorMap(Matrix *m) {
  if (!m) {
    // Mantid
    MultiLayer *plot = mantidUI->plotSpectrogram(GraphOptions::ColorMapContour);
    if (plot)
      return plot;
    m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
    if (!m)
      return nullptr;
  }

  return plotSpectrogram(m, GraphOptions::ColorMapContour);
}

MultiLayer *ApplicationWindow::plotNoContourColorMap(Matrix *m) {
  MultiLayer *ml = nullptr;
  if (!m) {
    m = qobject_cast<Matrix *>(activeWindow(MatrixWindow));
  }
  if (m) {
    ml = plotSpectrogram(m, GraphOptions::ColorMap);
  } else {
    ml = mantidUI->plotSpectrogram(GraphOptions::ColorMap);
  }
  if (!ml) {
    QApplication::restoreOverrideCursor();
    return nullptr;
  }

  return ml;
}

MultiLayer *ApplicationWindow::plotImage(Matrix *m) {
  MultiLayer *g = nullptr;
  Graph *plot = nullptr;
  if (!m) {
    m = qobject_cast<Matrix *>(activeWindow(MatrixWindow));
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (m) {
    g = multilayerPlot(generateUniqueName(tr("Graph")));
    plot = g->activeGraph();
    setPreferences(plot);

    Spectrogram *s = plot->plotSpectrogram(m, GraphOptions::GrayScale);
    if (!s) {
      QApplication::restoreOverrideCursor();
      return nullptr;
    }
    s->setAxis(QwtPlot::xTop, QwtPlot::yLeft);
    plot->setScale(QwtPlot::xTop, qMin(m->xStart(), m->xEnd()),
                   qMax(m->xStart(), m->xEnd()));
    plot->setScale(QwtPlot::yLeft, qMin(m->yStart(), m->yEnd()),
                   qMax(m->yStart(), m->yEnd()), 0.0, 5, 5,
                   GraphOptions::Linear, true);
  } else {
    g = mantidUI->plotSpectrogram(GraphOptions::GrayScale);
    if (!g) {
      QApplication::restoreOverrideCursor();
      return nullptr;
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

MultiLayer *ApplicationWindow::plotSpectrogram(Matrix *m,
                                               GraphOptions::CurveType type) {
  if (type == GraphOptions::ImagePlot)
    return plotImage(m);
  else if (type == GraphOptions::Histogram)
    return plotHistogram(m);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  MultiLayer *g = multilayerPlot(generateUniqueName(tr("Graph")));
  Graph *plot = g->activeGraph();
  setPreferences(plot);

  plot->plotSpectrogram(m, type);

  setSpectrogramTickStyle(plot);

  plot->setAutoScale(); // Mantid

  QApplication::restoreOverrideCursor();
  return g;
}

void ApplicationWindow::setSpectrogramTickStyle(Graph *g) {
  // always use the out tick style for colour bar axes
  QList<int> ticksList;
  ticksList << majTicksStyle << Graph::Ticks::Out << majTicksStyle
            << majTicksStyle;
  g->setMajorTicksType(ticksList);
  ticksList.clear();
  ticksList << minTicksStyle << Graph::Ticks::Out << minTicksStyle
            << minTicksStyle;
  g->setMinorTicksType(ticksList);
  // reset this as the colourbar should now be detectable
  g->drawAxesBackbones(drawBackbones);
}

ApplicationWindow *ApplicationWindow::importOPJ(const QString &filename,
                                                bool factorySettings,
                                                bool newProject) {
  if (filename.endsWith(".opj", Qt::CaseInsensitive) ||
      filename.endsWith(".ogg", Qt::CaseInsensitive)) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    ApplicationWindow *app = this;
    if (newProject)
      app = new ApplicationWindow(factorySettings);

    app->setWindowTitle("MantidPlot - " + filename); // Mantid
    app->restoreApplicationGeometry();
    app->projectname = filename;
    app->recentProjects.removeAll(filename);
    app->recentProjects.push_front(filename);
    app->updateRecentProjectsList();

    ImportOPJ(app, filename);

    QApplication::restoreOverrideCursor();
    return app;
  } else if (filename.endsWith(".ogm", Qt::CaseInsensitive) ||
             filename.endsWith(".ogw", Qt::CaseInsensitive)) {
    ImportOPJ(this, filename);
    recentProjects.removeAll(filename);
    recentProjects.push_front(filename);
    updateRecentProjectsList();
    return this;
  }
  return nullptr;
}

void ApplicationWindow::deleteFitTables() {
  QList<QWidget *> *mLst = new QList<QWidget *>();
  QList<MdiSubWindow *> windows = windowsList();
  foreach (MdiSubWindow *w, windows) {
    if (QString(w->metaObject()->className()) == "MultiLayer")
      mLst->append(w);
  }

  foreach (QWidget *ml, *mLst) {
    if (QString(ml->metaObject()->className()) == "MultiLayer") {
      auto cml = dynamic_cast<MultiLayer *>(ml);
      if (!cml)
        continue;

      QList<Graph *> layers = cml->layersList();
      foreach (Graph *g, layers) {
        QList<QwtPlotCurve *> curves = g->fitCurvesList();
        foreach (QwtPlotCurve *c, curves) {
          auto curve = dynamic_cast<PlotCurve *>(c);
          if (!curve)
            continue;

          if (curve->type() != GraphOptions::Function) {
            auto dc = dynamic_cast<DataCurve *>(c);
            if (!dc)
              continue;
            Table *t = dc->table();
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

QList<MdiSubWindow *> ApplicationWindow::windowsList() const {
  QList<MdiSubWindow *> lst;

  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows)
      lst << w;
    f = f->folderBelow();
  }
  return lst;
}

/**
 * Return all windows in all folders.
 */
QList<MdiSubWindow *> ApplicationWindow::getAllWindows() const {
  QList<MdiSubWindow *> out;
  // get the docked windows first
  QList<QMdiSubWindow *> wl = d_workspace->subWindowList();
  foreach (QMdiSubWindow *w, wl) {
    MdiSubWindow *sw = dynamic_cast<MdiSubWindow *>(w->widget());
    if (sw) {
      out.append(sw);
    }
  }

  // get the floating windows
  foreach (FloatingWindow *w, m_floatingWindows) {
    MdiSubWindow *sw = w->mdiSubWindow();
    if (sw) {
      out.append(sw);
    }
  }
  return out;
}

void ApplicationWindow::updateRecentProjectsList() {
  if (recentProjects.isEmpty())
    return;

  while ((int)recentProjects.size() > MaxRecentProjects)
    recentProjects.pop_back();

  recentProjectsMenu->clear();

  for (int i = 0; i < (int)recentProjects.size(); i++)
    recentProjectsMenu->addAction("&" + QString::number(i + 1) + " " +
                                  recentProjects[i]);
}

void ApplicationWindow::updateRecentFilesList(QString fname) {
  if (!fname.isEmpty()) {
    recentFiles.removeAll(fname);
    recentFiles.push_front(fname);
  }
  while ((int)recentFiles.size() > MaxRecentFiles) {
    recentFiles.pop_back();
  }

  recentFilesMenu->clear();
  const QString itemTemplate("&%1 %2");
  const int maxItemLength(50);
  for (int i = 0; i < (int)recentFiles.size(); i++) {
    QString filePath = recentFiles[i];
    // elide the text if it is over the allowed limit
    QString itemText = filePath;
    if (filePath.size() > maxItemLength) {
      itemText = "..." + filePath.right(maxItemLength);
    }
    QString actionText = itemTemplate.arg(QString::number(i + 1), itemText);
    QAction *ma = new QAction(actionText, recentFilesMenu);
    ma->setToolTip("<p>" + filePath + "</p>");
    ma->setData(recentFiles[i]);
    recentFilesMenu->addAction(ma);
  }
}

void ApplicationWindow::setReadOnlyCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i = 0; i < (int)list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), actionReadOnlyCol->isChecked());
}

void ApplicationWindow::setReadOnlyColumns() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i = 0; i < (int)list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]));
}

void ApplicationWindow::setReadWriteColumns() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList list = t->selectedColumns();
  for (int i = 0; i < (int)list.count(); i++)
    t->setReadOnlyColumn(t->colIndex(list[i]), false);
}

void ApplicationWindow::setAscValues() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setAscValues();
}

void ApplicationWindow::setRandomValues() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setRandomValues();
}

void ApplicationWindow::setXErrCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::xErr);
}

void ApplicationWindow::setYErrCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::yErr);
}

void ApplicationWindow::setXCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::X);
}

void ApplicationWindow::setYCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Y);
}

void ApplicationWindow::setZCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Z);
}

void ApplicationWindow::setLabelCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::Label);
}

void ApplicationWindow::disregardCol() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  t->setPlotDesignation(Table::None);
}

void ApplicationWindow::showHomePage() {
  MantidDesktopServices::openUrl(QUrl("http://www.mantidproject.org"));
}
void ApplicationWindow::showMantidConcepts() { HelpWindow::showConcept(this); }
void ApplicationWindow::showalgorithmDescriptions() {
  HelpWindow::showAlgorithm(this);
}

void ApplicationWindow::showFirstTimeSetup() {
  FirstTimeSetup *dialog = new FirstTimeSetup(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  dialog->setFocus();
}

/*
 Show mantidplot help page
 */
void ApplicationWindow::showmantidplotHelp() { HelpWindow::showPage(this); }

void ApplicationWindow::showBugTracker() {
  MantidDesktopServices::openUrl(QUrl("http://forum.mantidproject.org/"));
}

/*
@param arg: command argument
@return TRUE if argument suggests execution and quitting
*/
bool ApplicationWindow::shouldExecuteAndQuit(const QString &arg) {
  return arg.endsWith("--execandquit") || arg.endsWith("-xq");
}

/*
@param arg: command argument
@return TRUE if argument suggests a silent startup
*/
bool ApplicationWindow::isSilentStartup(const QString &arg) {
  return arg.endsWith("--silent") || arg.endsWith("-s");
}

void ApplicationWindow::parseCommandLineArguments(const QStringList &args) {
  m_exec_on_start = false;
  m_quit_after_exec = false;
  m_cmdline_filename = "";

  int num_args = args.count();
  if (num_args == 0) {
    initWindow();
    savedProject();
    return;
  }

  bool default_settings(false), unknown_opt_found(false);
  QString str;
  int filename_argindex(0), counter(0);
  foreach (str, args) {
    if ((str == "-v" || str == "--version") ||
        (str == "-r" || str == "--revision") ||
        (str == "-a" || str == "--about") || (str == "-h" || str == "--help")) {
      g_log.warning() << qPrintable(str)
                      << ": This command line option must be used without "
                         "other arguments!";
    } else if ((str == "-d" || str == "--default-settings")) {
      default_settings = true;
    } else if (str.endsWith("--execute") || str.endsWith("-x")) {
      m_exec_on_start = true;
      m_quit_after_exec = false;
    } else if (shouldExecuteAndQuit(str)) {
      m_exec_on_start = true;
      m_quit_after_exec = true;
    } else if (isSilentStartup(str)) {
      g_log.debug("Starting in Silent mode");
    } // if filename not found yet then these are all program arguments so we
    // should
    // know what they all are
    else if (m_cmdline_filename.isEmpty() &&
             (str.startsWith("-") || str.startsWith("--"))) {
      g_log.warning()
          << "'" << qPrintable(str) << "' unknown command line option!\n"
          << "Type 'MantidPlot -h'' to see the list of the valid options.";
      unknown_opt_found = true;
      break;
    } else {
      // First option that doesn't start "-" is considered a filename and the
      // rest arguments to that file
      if (m_cmdline_filename.isEmpty()) {
        m_cmdline_filename = str;
        filename_argindex = counter;
      }
    }
    ++counter;
  }

  if (unknown_opt_found || m_cmdline_filename.isEmpty()) { // no file name given
    initWindow();
    savedProject();
    return;
  } else {
    QFileInfo fi(m_cmdline_filename);
    if (fi.isDir()) {
      QMessageBox::critical(
          this, tr("MantidPlot - Error opening file"), // Mantid
          tr("<b>%1</b> is a directory, please specify a file name!")
              .arg(m_cmdline_filename));
      return;
    } else if (!fi.exists()) {
      QMessageBox::critical(
          this, tr("MantidPlot - Error opening file"), // Mantid
          tr("The file: <b>%1</b> doesn't exist!").arg(m_cmdline_filename));
      return;
    } else if (!fi.isReadable()) {
      QMessageBox::critical(
          this, tr("MantidPlot - Error opening file"), // Mantid
          tr("You don't have the permission to open this file: <b>%1</b>")
              .arg(m_cmdline_filename));
      return;
    }

    workingDir = fi.absolutePath();
    saveSettings(); // the recent projects must be saved

    QStringList cmdArgs = args;
    cmdArgs.erase(cmdArgs.begin(), cmdArgs.begin() + filename_argindex);
    // Set as arguments in script environment
    scriptingEnv()->setSysArgs(cmdArgs);

    if (!m_quit_after_exec && !m_cmdline_filename.isEmpty()) {
      saved = true;
      open(m_cmdline_filename, default_settings, false);
    }
  }
}

void ApplicationWindow::createLanguagesList() {
  locales.clear();

  appTranslator = new QTranslator(this);
  qtTranslator = new QTranslator(this);
  qApp->installTranslator(appTranslator);
  qApp->installTranslator(qtTranslator);

  QString qmPath = d_translations_folder;
  QDir dir(qmPath);
  QStringList fileNames = dir.entryList(QStringList("qtiplot_*.qm"));
  for (int i = 0; i < (int)fileNames.size(); i++) {
    QString locale = fileNames[i];
    locale = locale.mid(locale.indexOf('_') + 1);
    locale.truncate(locale.indexOf('.'));
    locales.push_back(locale);
  }
  locales.push_back("en");
  locales.sort();

  if (appLanguage != "en") {
    appTranslator->load("qtiplot_" + appLanguage, qmPath);
    qtTranslator->load("qt_" + appLanguage, qmPath + "/qt");
  }
}

void ApplicationWindow::switchToLanguage(int param) {
  if (param < (int)locales.size())
    switchToLanguage(locales[param]);
}

void ApplicationWindow::switchToLanguage(const QString &locale) {
  if (!locales.contains(locale) || appLanguage == locale)
    return;

  appLanguage = locale;
  if (locale == "en") {
    qApp->removeTranslator(appTranslator);
    qApp->removeTranslator(qtTranslator);
    delete appTranslator;
    delete qtTranslator;
    appTranslator = new QTranslator(this);
    qtTranslator = new QTranslator(this);
    qApp->installTranslator(appTranslator);
    qApp->installTranslator(qtTranslator);
  } else {
    QString qmPath = d_translations_folder;
    appTranslator->load("qtiplot_" + locale, qmPath);
    qtTranslator->load("qt_" + locale, qmPath + "/qt");
  }
  insertTranslatedStrings();
}

QStringList ApplicationWindow::matrixNames() {
  QStringList names;
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (QString(w->metaObject()->className()) == "Matrix")
        names << w->objectName();
    }
    f = f->folderBelow();
  }
  return names;
}
QStringList ApplicationWindow::mantidmatrixNames() {
  QStringList names;
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (QString(w->metaObject()->className()) == "MantidMatrix")
        names << w->objectName();
    }
    f = f->folderBelow();
  }
  return names;
}

/**
 * Add a MantidMatrix to the application window instance
 * @param matrix :: the MantidMatrix to add
 */
void ApplicationWindow::addMantidMatrixWindow(MantidMatrix *matrix) {
  m_mantidmatrixWindows << matrix;
}

/**
 * Find a MantidMatrix instance using its name.
 * @param wsName :: the name of the workspace
 * @return a pointer to a MantidMatrix or NULL
 */
MantidMatrix *
ApplicationWindow::findMantidMatrixWindow(const std::string &wsName) {
  MantidMatrix *m = nullptr;

  // lambda to check if a workspace name matches a string
  auto nameMatches = [&](MantidMatrix *matrix) {
    return (matrix && matrix->getWorkspaceName() == wsName);
  };

  auto result = std::find_if(m_mantidmatrixWindows.begin(),
                             m_mantidmatrixWindows.end(), nameMatches);

  if (result != m_mantidmatrixWindows.end()) {
    m = *result;
  }
  return m;
}

bool ApplicationWindow::alreadyUsedName(const QString &label) {
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (w->objectName() == label)
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

bool ApplicationWindow::projectHas2DPlots() {
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (QString(w->metaObject()->className()) == "MultiLayer")
        return true;
    }
    f = f->folderBelow();
  }
  return false;
}

void ApplicationWindow::appendProject() {
  OpenProjectDialog *open_dialog = new OpenProjectDialog(this, false);
  open_dialog->setDirectory(workingDir);
  open_dialog->setExtensionWidget(nullptr);

  if (open_dialog->exec() != QDialog::Accepted ||
      open_dialog->selectedFiles().isEmpty())
    return;

  workingDir = open_dialog->directory().path();
  appendProject(open_dialog->selectedFiles()[0]);
}

Folder *ApplicationWindow::appendProject(const QString &fn,
                                         Folder *parentFolder) {
  d_opening_file = true;

  QFile file(fn);
  QFileInfo fileInfo(fn);

  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(
        this, tr("MantidPlot - File opening error"),
        tr("The file: <b> %1 </b> could not be opened!").arg(fn));
    return nullptr;
  }

  QTextStream fileTS(&file);
  fileTS.setCodec(QTextCodec::codecForName("UTF-8"));

  QString baseName = fileInfo.fileName();

  // Read version line
  QString versionLine = fileTS.readLine();
  QStringList versionParts =
      versionLine.split(QRegExp("\\s"), QString::SkipEmptyParts);
  QStringList vl = versionParts[1].split(".", QString::SkipEmptyParts);
  const int fileVersion =
      100 * (vl[0]).toInt() + 10 * (vl[1]).toInt() + (vl[2]).toInt();

  // Skip the <scripting-lang> line. We only really use python now anyway.
  fileTS.readLine();

  // Skip the <windows> line.
  fileTS.readLine();

  folders->blockSignals(true);
  blockSignals(true);

  // Read the rest of the project file in for parsing
  std::string lines = fileTS.readAll().toStdString();

  // Save the selected folder
  Folder *curFolder = currentFolder();

  // Change to parent folder, if given
  if (parentFolder)
    changeFolder(parentFolder, true);

  // Open folders
  ProjectSerialiser serialiser(this);

  try {
    serialiser.load(lines, fileVersion);
  } catch (std::runtime_error &e) {
    g_log.error(e.what());
    // We failed to load - bail out
    return nullptr;
  }

  // Restore the selected folder
  folders->setCurrentItem(curFolder->folderListItem());
  changeFolder(curFolder, true);

  blockSignals(false);
  folders->blockSignals(false);

  restoreApplicationGeometry();

  d_opening_file = false;

  return nullptr;
}

void ApplicationWindow::saveAsProject() {
  QString filter = tr("MantidPlot project") + " (*.qti);;"; // Mantid
  filter += tr("Compressed MantidPlot project") + " (*.qti.gz)";

  QString selectedFilter;
  QString fn = QFileDialog::getSaveFileName(
      this, tr("Save project as"), workingDir, filter, &selectedFilter);
  if (!fn.isEmpty()) {
    QFileInfo fi(fn);
    workingDir = fi.absolutePath();
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fn.append(".qti");

    ProjectSerialiser serialiser(this);
    serialiser.save(fn, selectedFilter.contains(".gz"));
  }
}

void ApplicationWindow::showFolderPopupMenu(const QPoint &p) {
  auto item = folders->itemAt(p);
  showFolderPopupMenu(item, p, true);
}

void ApplicationWindow::showFolderPopupMenu(QTreeWidgetItem *it,
                                            const QPoint &p, bool fromFolders) {

  QMenu cm(this);
  QMenu window(this);
  QMenu viewWindowsMenu(this);

  cm.addAction(tr("&Find..."), this, SLOT(showFindDialogue()));
  cm.addSeparator();
  cm.addAction(tr("App&end Project..."), this, SLOT(appendProject()));

  auto fli = dynamic_cast<FolderListItem *>(it);
  if (!fli)
    return;

  if (fli->folder()->parent())
    cm.addAction(tr("Save &As Project..."), this, SLOT(saveAsProject()));
  else
    cm.addAction(tr("Save Project &As..."), this, SLOT(prepareSaveProject()));
  cm.addSeparator();

  if (fromFolders && show_windows_policy != HideAll) {
    cm.addAction(tr("&Show All Windows"), this, SLOT(showAllFolderWindows()));
    cm.addAction(tr("&Hide All Windows"), this, SLOT(hideAllFolderWindows()));
    cm.addSeparator();
  }

  if (fromFolders) {
    window.addAction(actionNewTable);
    window.addAction(actionNewMatrix);
    window.addAction(actionNewNote);
    window.addAction(actionNewGraph);
    window.addAction(actionNewFunctionPlot);
    window.addAction(actionNewSurfacePlot);
    window.addAction(actionNewTiledWindow);
    cm.addMenu(&window)->setText(tr("New &Window"));
  }

  QStringList lst;
  lst << tr("&None") << tr("&Windows in Active Folder");
  for (int i = 0; i < lst.size(); ++i) {
    auto action = viewWindowsMenu.addAction(lst[i], this,
                                            SLOT(setShowWindowsPolicy(int)));
    action->setData(i);
    action->setChecked(show_windows_policy == i);
  }
  cm.addMenu(&viewWindowsMenu)->setText(tr("&View Windows"));
  cm.addSeparator();
  cm.addAction(tr("&Properties..."), this, SLOT(folderProperties()));
  if (fromFolders) {
    cm.exec(folders->mapToGlobal(p));
  } else {
    cm.exec(lv->mapToGlobal(p));
  }
}

void ApplicationWindow::setShowWindowsPolicy(int p) {
  if (show_windows_policy == (ShowWindowsPolicy)p)
    return;

  show_windows_policy = (ShowWindowsPolicy)p;
  if (show_windows_policy == HideAll) {
    QList<MdiSubWindow *> windows = windowsList();
    foreach (MdiSubWindow *w, windows) {
      hiddenWindows->append(w);
      w->hide();
      setListView(w->objectName(), tr("Hidden"));
    }
  } else
    showAllFolderWindows();
}

void ApplicationWindow::showFindDialogue() {
  FindDialog *fd = new FindDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  fd->exec();
}

void ApplicationWindow::showAllFolderWindows() {
  QList<MdiSubWindow *> lst = currentFolder()->windowsList();
  foreach (MdiSubWindow *w, lst) { // force show all windows in current folder
    if (w) {
      updateWindowLists(w);
      switch (w->status()) {
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
}

void ApplicationWindow::hideAllFolderWindows() {
  QList<MdiSubWindow *> lst = currentFolder()->windowsList();
  foreach (MdiSubWindow *w, lst)
    hideWindow(w);

  if ((currentFolder()->children()).isEmpty())
    return;
}

void ApplicationWindow::projectProperties() {
  QString s = QString(currentFolder()->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Project") + "\n\n";
  if (projectname != "untitled") {
    s += tr("Path") + ": " + projectname + "\n\n";

    QFileInfo fi(projectname);
    s += tr("Size") + ": " + QString::number(fi.size()) + " " + tr("bytes") +
         "\n\n";
  }

  s += tr("Contents") + ": " + QString::number(windowsList().size()) + " " +
       tr("windows");
  s += ", " + QString::number(currentFolder()->subfolders().count()) + " " +
       tr("folders") + "\n\n";
  s += "\n\n\n";

  if (projectname != "untitled") {
    QFileInfo fi(projectname);
    s += tr("Created") + ": " + fi.created().toString(Qt::LocalDate) + "\n\n";
    s += tr("Modified") + ": " + fi.lastModified().toString(Qt::LocalDate) +
         "\n\n";
  } else
    s += tr("Created") + ": " + currentFolder()->birthDate() + "\n\n";

  QMessageBox *mbox =
      new QMessageBox(tr("Properties"), s, QMessageBox::NoIcon, QMessageBox::Ok,
                      QMessageBox::NoButton, QMessageBox::NoButton, this);

  mbox->show();
}

void ApplicationWindow::folderProperties() {
  if (!currentFolder()->parent()) {
    projectProperties();
    return;
  }

  QString s = QString(currentFolder()->objectName()) + "\n\n";
  s += "\n\n\n";
  s += tr("Type") + ": " + tr("Folder") + "\n\n";
  s += tr("Path") + ": " + currentFolder()->path() + "\n\n";
  s += tr("Size") + ": " + currentFolder()->sizeToString() + "\n\n";
  s += tr("Contents") + ": " +
       QString::number(currentFolder()->windowsList().count()) + " " +
       tr("windows");
  s += ", " + QString::number(currentFolder()->subfolders().count()) + " " +
       tr("folders") + "\n\n";
  s += tr("Created") + ": " + currentFolder()->birthDate() + "\n\n";

  QMessageBox *mbox =
      new QMessageBox(tr("Properties"), s, QMessageBox::NoIcon, QMessageBox::Ok,
                      QMessageBox::NoButton, QMessageBox::NoButton, this);

  mbox->setIconPixmap(getQPixmap("folder_open_xpm"));
  mbox->show();
}

void ApplicationWindow::addFolder() {
  if (!explorerWindow->isVisible())
    explorerWindow->show();

  QStringList lst = currentFolder()->subfolders();
  QString name = tr("New Folder");
  lst = lst.filter(name);
  if (!lst.isEmpty())
    name += " (" + QString::number(lst.size() + 1) + ")";

  Folder *f = new Folder(currentFolder(), name);
  addFolderListViewItem(f);

  FolderListItem *fi = new FolderListItem(currentFolder()->folderListItem(), f);
  if (fi) {
    f->setFolderListItem(fi);
  }
}

Folder *ApplicationWindow::addFolder(QString name, Folder *parent) {
  if (!parent) {
    if (currentFolder())
      parent = currentFolder();
    else
      parent = projectFolder();
  }

  QStringList lst = parent->subfolders();
  lst = lst.filter(name);
  if (!lst.isEmpty())
    name += " (" + QString::number(lst.size() + 1) + ")";

  Folder *f = new Folder(parent, name);
  addFolderListViewItem(f);

  FolderListItem *fi = new FolderListItem(parent->folderListItem(), f);
  if (fi)
    f->setFolderListItem(fi);

  return f;
}

bool ApplicationWindow::deleteFolder(Folder *f) {
  if (!f)
    return false;

  if (confirmCloseFolder &&
      QMessageBox::information(
          this, tr("MantidPlot - Delete folder?"), // Mantid
          tr("Delete folder '%1' and all the windows it contains?")
              .arg(f->objectName()),
          tr("Yes"), tr("No"), nullptr, 0))
    return false;
  else {
    Folder *parent = projectFolder();
    if (currentFolder()) {
      if (currentFolder()->parent()) {
        auto newParent = dynamic_cast<Folder *>(currentFolder()->parent());
        if (newParent)
          parent = newParent;
      }
    }

    folders->blockSignals(true);

    FolderListItem *fi = f->folderListItem();
    foreach (MdiSubWindow *w, f->windowsList()) {
      if (!w->close()) {
        QMessageBox::warning(this, "Mantid - Warning",
                             "Folder was not deleted.");
        return false;
      }
    }

    if (!(f->children()).isEmpty()) {
      Folder *subFolder = f->folderBelow();
      int initial_depth = f->depth();
      while (subFolder && subFolder->depth() > initial_depth) {
        foreach (MdiSubWindow *w, subFolder->windowsList()) {
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

void ApplicationWindow::deleteFolder() {
  Folder *parent = dynamic_cast<Folder *>(currentFolder()->parent());
  if (!parent)
    parent = projectFolder();

  folders->blockSignals(true);

  if (deleteFolder(currentFolder())) {
    d_current_folder = parent;
    folders->setCurrentItem(parent->folderListItem());
    changeFolder(parent, true);
  }

  folders->blockSignals(false);
  folders->setFocus();
}

void ApplicationWindow::folderItemDoubleClicked(QTreeWidgetItem *it) {
  if (!it)
    return;

  auto fli = dynamic_cast<FolderListItem *>(it);
  if (!fli)
    return;

  FolderListItem *item = fli->folder()->folderListItem();
  folders->setCurrentItem(item);
}

void ApplicationWindow::folderItemChanged(QTreeWidgetItem *it,
                                          QTreeWidgetItem * /*unused*/) {
  if (!it)
    return;

  it->setExpanded(true);

  auto fli = dynamic_cast<FolderListItem *>(it);

  if (!fli)
    return;

  changeFolder(fli->folder());
  folders->setFocus();
}

void ApplicationWindow::hideFolderWindows(Folder *f) {
  QList<MdiSubWindow *> lst = f->windowsList();
  foreach (MdiSubWindow *w, lst)
    w->hide();

  if ((f->children()).isEmpty())
    return;

  Folder *dir = f->folderBelow();
  int initial_depth = f->depth();
  while (dir && dir->depth() > initial_depth) {
    lst = dir->windowsList();
    foreach (MdiSubWindow *w, lst)
      w->hide();

    dir = dir->folderBelow();
  }
}

bool ApplicationWindow::changeFolder(Folder *newFolder, bool force) {
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

  if (newFolder != oldFolder)
    hideFolderWindows(oldFolder);

  d_current_folder = newFolder;

  resultsLog->appendInformation(currentFolder()->logInfo());

  lv->clear();

  QObjectList folderLst = newFolder->children();
  if (!folderLst.isEmpty()) {
    foreach (QObject *f, folderLst)
      addFolderListViewItem(static_cast<Folder *>(f));
  }

  QList<MdiSubWindow *> lst = newFolder->windowsList();
  foreach (MdiSubWindow *w, lst) {
    if (!hiddenWindows->contains(w) && show_windows_policy != HideAll) {
      // show only windows in the current folder which are not hidden by the
      // user
      if (w->status() == MdiSubWindow::Normal ||
          w->status() == MdiSubWindow::Hidden) {
        w->setNormal();
      } else if (w->status() == MdiSubWindow::Minimized)
        w->setMinimized();
      else if (w->status() == MdiSubWindow::Maximized)
        w->setMaximized();
    }

    addListViewItem(w);
  }

  if (!(newFolder->children()).isEmpty()) {
    Folder *f = newFolder->folderBelow();
    int initial_depth = newFolder->depth();
    while (f && f->depth() > initial_depth) { // show/hide windows in subfolders
      lst = f->windowsList();
      foreach (MdiSubWindow *w, lst) {
        if (!hiddenWindows->contains(w)) {
          if (show_windows_policy == SubFolders) {
            if (w->status() == MdiSubWindow::Normal ||
                w->status() == MdiSubWindow::Maximized)
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

  if (active_window) {
    setActiveWindow(active_window);
    customMenu(active_window);
    customToolBars(active_window);
    if (active_window_state == MdiSubWindow::Minimized)
      active_window->showMinimized(); // ws->setActiveWindow() makes minimized
                                      // windows to be shown normally
    else if (active_window_state == MdiSubWindow::Maximized) {
      if (QString(active_window->metaObject()->className()) == "Graph3D")
        static_cast<Graph3D *>(active_window)->setIgnoreFonts(true);
      active_window->showMaximized();
      if (QString(active_window->metaObject()->className()) == "Graph3D")
        static_cast<Graph3D *>(active_window)->setIgnoreFonts(false);
    }
  }

  if (old_active_window) {
    old_active_window->setStatus(old_active_window_state);
    oldFolder->setActiveWindow(old_active_window);
  }

  if (d_opening_file)
    modifiedProject();
  return true;
}

void ApplicationWindow::desactivateFolders() {
  FolderListItem *item = dynamic_cast<FolderListItem *>(folders->firstChild());
  while (item) {
    item->setActive(false);
    item = dynamic_cast<FolderListItem *>(folders->itemBelow(item));
  }
}

void ApplicationWindow::addListViewItem(MdiSubWindow *w) {
  if (!w)
    return;

  WindowListItem *it = new WindowListItem(lv, w);
  if (QString(w->metaObject()->className()) == "Matrix") {
    it->setIcon(0, getQPixmap("matrix_xpm"));
    it->setText(1, tr("Matrix"));
  } else if (w->inherits("Table")) {
    it->setIcon(0, getQPixmap("worksheet_xpm"));
    it->setText(1, tr("Table"));
  } else if (QString(w->metaObject()->className()) == "Note") {
    it->setIcon(0, getQPixmap("note_xpm"));
    it->setText(1, tr("Note"));
  } else if (QString(w->metaObject()->className()) == "MultiLayer") {
    it->setIcon(0, getQPixmap("graph_xpm"));
    it->setText(1, tr("Graph"));
  } else if (QString(w->metaObject()->className()) == "Graph3D") {
    it->setIcon(0, getQPixmap("trajectory_xpm"));
    it->setText(1, tr("3D Graph"));
  } else if (QString(w->metaObject()->className()) == "MantidMatrix") {
    it->setIcon(0, getQPixmap("mantid_matrix_xpm"));
    it->setText(1, tr("Workspace"));
  } else if (QString(w->metaObject()->className()) == "InstrumentWindow") {
    it->setText(1, tr("Instrument"));
  } else {
    it->setText(1, tr("Custom window"));
  }

  it->setText(0, w->objectName());
  it->setText(2, w->aspect());
  it->setText(3, w->sizeToString());
  it->setText(4, w->birthDate());
  it->setText(5, w->windowLabel());
  lv->adjustColumns();
}

void ApplicationWindow::windowProperties() {
  WindowListItem *it = dynamic_cast<WindowListItem *>(lv->currentItem());
  if (!it)
    return;

  MdiSubWindow *w = it->window();
  if (!w)
    return;

  QMessageBox *mbox = new QMessageBox(
      tr("Properties"), QString(), QMessageBox::NoIcon, QMessageBox::Ok,
      QMessageBox::NoButton, QMessageBox::NoButton, this);

  QString s = QString(w->objectName()) + "\n\n";
  s += "\n\n\n";

  s += tr("Label") + ": " + static_cast<MdiSubWindow *>(w)->windowLabel() +
       "\n\n";

  if (QString(w->metaObject()->className()) == "Matrix") {
    mbox->setIconPixmap(getQPixmap("matrix_xpm"));
    s += tr("Type") + ": " + tr("Matrix") + "\n\n";
  } else if (w->inherits("Table")) {
    mbox->setIconPixmap(getQPixmap("worksheet_xpm"));
    s += tr("Type") + ": " + tr("Table") + "\n\n";
  } else if (QString(w->metaObject()->className()) == "Note") {
    mbox->setIconPixmap(getQPixmap("note_xpm"));
    s += tr("Type") + ": " + tr("Note") + "\n\n";
  } else if (QString(w->metaObject()->className()) == "MultiLayer") {
    mbox->setIconPixmap(getQPixmap("graph_xpm"));
    s += tr("Type") + ": " + tr("Graph") + "\n\n";
  } else if (QString(w->metaObject()->className()) == "Graph3D") {
    mbox->setIconPixmap(getQPixmap("trajectory_xpm"));
    s += tr("Type") + ": " + tr("3D Graph") + "\n\n";
  }
  s += tr("Path") + ": " + currentFolder()->path() + "\n\n";
  s += tr("Size") + ": " + w->sizeToString() + "\n\n";
  s += tr("Created") + ": " + w->birthDate() + "\n\n";
  s += tr("Status") + ": " + it->text(2) + "\n\n";
  mbox->setText(s);
  mbox->show();
}

void ApplicationWindow::addFolderListViewItem(Folder *f) {
  if (!f)
    return;

  FolderListItem *it = new FolderListItem(lv, f);
  it->setActive(false);
  it->setText(0, f->objectName());
  it->setText(1, tr("Folder"));
  it->setText(3, f->sizeToString());
  it->setText(4, f->birthDate());
}

void ApplicationWindow::find(const QString &s, bool windowNames, bool labels,
                             bool /*unused*/, bool caseSensitive,
                             bool partialMatch, bool /*unused*/) {
  if (windowNames || labels) {
    MdiSubWindow *w = currentFolder()->findWindow(s, windowNames, labels,
                                                  caseSensitive, partialMatch);
    if (w) {
      activateWindow(w);
      return;
    }
  }

  QMessageBox::warning(this, tr("MantidPlot - No match found"), // Mantid
                       tr("Sorry, no match found for string: '%1'").arg(s));
}

/**
  Turns 3D animation on or off
 */
void ApplicationWindow::toggle3DAnimation(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->animate(on);
}

QString ApplicationWindow::generateUniqueName(const QString &name,
                                              bool increment) {
  int index = 0;
  QStringList lst;
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      lst << QString(w->objectName());
      if (QString(w->objectName()).startsWith(name))
        index++;
    }
    f = f->folderBelow();
  }

  QString newName = name;
  if (increment) // force return of a different name
    newName += QString::number(++index);
  else if (index > 0)
    newName += QString::number(index);

  while (lst.contains(newName))
    newName = name + QString::number(++index);

  return newName;
}

void ApplicationWindow::clearTable() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;

  if (QMessageBox::question(this, tr("MantidPlot - Warning"), // Mantid
                            tr("This will clear the contents of all the data "
                               "associated with the table. Are you sure?"),
                            tr("&Yes"), tr("&No"), QString(), 0, 1))
    return;
  else
    t->clear();
}

void ApplicationWindow::goToRow() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table") ||
      QString(w->metaObject()->className()) == "Matrix") {
    bool ok;
    int row = QInputDialog::getInteger(
        this, tr("MantidPlot - Enter row number"), tr("Row"), // Mantid
        1, 0, 1000000, 1, &ok,
        windowFlags() & ~Qt::WindowContextHelpButtonHint &
            ~Qt::WindowMinMaxButtonsHint);
    if (!ok)
      return;

    auto table = dynamic_cast<Table *>(w);
    auto matrix = dynamic_cast<Matrix *>(w);

    if (table)
      table->goToRow(row);
    else if (matrix)
      matrix->goToRow(row);
  }
}

void ApplicationWindow::goToColumn() {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return;

  if (w->inherits("Table") ||
      QString(w->metaObject()->className()) == "Matrix") {
    bool ok;
    int col = QInputDialog::getInteger(
        this, tr("MantidPlot - Enter column number"), tr("Column"), // Mantid
        1, 0, 1000000, 1, &ok,
        windowFlags() & ~Qt::WindowContextHelpButtonHint &
            ~Qt::WindowMinMaxButtonsHint);
    if (!ok)
      return;

    auto t = dynamic_cast<Table *>(w);
    auto m = dynamic_cast<Matrix *>(w);
    if (t)
      t->goToColumn(col);
    else if (m)
      m->goToColumn(col);
  }
}

/**
 * Show the script window, creating it if necessary
 * @param forceVisible - If true the window is forced to visible rather than
 * toggling
 * @param quitting - If true then it is assumed MantidPlot will exit
 * automatically so stdout redirect
 * from scripts is disabled.
 */
void ApplicationWindow::showScriptWindow(bool forceVisible, bool quitting) {
  if (!scriptingWindow) {
    // MG 09/02/2010 : Removed parent from scripting window. If it has one
    // then it doesn't respect the always on top flag, it is treated as a sub
    // window of its parent
    const bool capturePrint = !quitting;
    scriptingWindow =
        new ScriptingWindow(scriptingEnv(), capturePrint, nullptr);
    scriptingWindow->setObjectName("ScriptingWindow");
    scriptingWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(scriptingWindow, SIGNAL(closeMe()), this,
            SLOT(saveScriptWindowGeometry()));
    connect(scriptingWindow, SIGNAL(hideMe()), this,
            SLOT(saveScriptWindowGeometry()));
    connect(scriptingWindow, SIGNAL(hideMe()), this, SLOT(showScriptWindow()));
    connect(scriptingWindow, SIGNAL(chooseScriptingLanguage()), this,
            SLOT(showScriptingLangDialog()));
    // keep toolbar button status in sync with this window visibility
    connect(scriptingWindow, SIGNAL(closeMe()), actionShowScriptWindow,
            SLOT(toggle()));
    connect(scriptingWindow, SIGNAL(hideMe()), actionShowScriptWindow,
            SLOT(toggle()));
  }

  if (forceVisible || scriptingWindow->isMinimized() ||
      !scriptingWindow->isVisible()) {
    scriptingWindow->resize(d_script_win_size);
    scriptingWindow->move(d_script_win_pos);
    if (quitting) {
      scriptingWindow->showMinimized();
    } else {
      scriptingWindow->show();
    }
    scriptingWindow->setFocus();
  } else {
    saveScriptWindowGeometry();
    // Hide is connect to this function so block it temporarily
    scriptingWindow->blockSignals(true);
    scriptingWindow->hide();
    scriptingWindow->blockSignals(false);
  }
}

void ApplicationWindow::saveScriptWindowGeometry() {
  d_script_win_size = scriptingWindow->size();
  d_script_win_pos = scriptingWindow->pos();
}

void ApplicationWindow::showScriptInterpreter() {
  if (m_interpreterDock->isVisible()) {
    m_interpreterDock->hide();
  } else {
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
void ApplicationWindow::togglePerspective(bool on) {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setOrthogonal(!on);
}

/**
  Resets rotation of 3D plots to default values
 */
void ApplicationWindow::resetRotation() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->setRotation(30, 0, 15);
}

/**
  Finds best layout for the 3D plot
 */
void ApplicationWindow::fitFrameToLayer() {
  Graph3D *g = dynamic_cast<Graph3D *>(activeWindow(Plot3DWindow));
  if (!g)
    return;

  g->findBestLayout();
}

ApplicationWindow::~ApplicationWindow() {
  delete lastCopiedLayer;
  delete hiddenWindows;
  delete scriptingWindow;
  delete d_text_editor;
  while (!d_user_menus.isEmpty()) {
    QMenu *menu = d_user_menus.takeLast();
    delete menu;
  }
  delete d_current_folder;

  btnPointer->setChecked(true);
  delete mantidUI;
}

QString ApplicationWindow::versionString() {
  QString version(Mantid::Kernel::MantidVersion::version());
  QString date(Mantid::Kernel::MantidVersion::releaseDate());
  return "This is MantidPlot version " + version + " of " + date;
}

void ApplicationWindow::cascade() {
  const int xoffset = 13;
  const int yoffset = 20;
  int x = 0;
  int y = 0;
  QList<QMdiSubWindow *> windows =
      d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach (QMdiSubWindow *w, windows) {
    MdiSubWindow *innerWidget = dynamic_cast<MdiSubWindow *>(w->widget());
    if (!innerWidget) {
      throw std::runtime_error("A non-MdiSubWindow detected in the MDI area");
    }
    w->activateWindow();
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
 * @param fn :: is read as a Python script file and loaded in the command
 * script window.
 * @param existingProject :: True if loading into an already existing project
 */
ApplicationWindow *ApplicationWindow::loadScript(const QString &fn,
                                                 bool existingProject) {
#ifdef SCRIPTING_PYTHON
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setScriptingLanguage("Python");
  restoreApplicationGeometry();
  showScriptWindow(existingProject, false);
  scriptingWindow->openUnique(fn);
  QApplication::restoreOverrideCursor();
  return this;
#else
  QMessageBox::critical(this,
                        tr("MantidPlot") + " - " + tr("Error"), // Mantid
                        tr("MantidPlot was not built with Python scripting "
                           "support included!")); // Mantid
  return 0;
#endif
}

/**
 *  Runs a script from a file. Mainly useful for automatically running scripts
 * @param filename The full path to the file
 * @param execMode How should the script be executed. If asynchronous
 *                 this method waits on the thread finishing
 */
void ApplicationWindow::executeScriptFile(
    const QString &filename, const Script::ExecutionMode execMode) {
  QFile scriptFile(filename);
  if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    throw std::runtime_error("Unable to open script file");
  }
  QTextStream in(&scriptFile);
  QString code;
  while (!in.atEnd()) {
    code += in.readLine() + "\n";
  }
  Script *runner =
      scriptingEnv()->newScript(filename, this, Script::NonInteractive);
  connect(runner, SIGNAL(finished(const QString &)), this,
          SLOT(onScriptExecuteSuccess(const QString &)));
  connect(runner, SIGNAL(error(const QString &, const QString &, int)), this,
          SLOT(onScriptExecuteError(const QString &, const QString &, int)));
  runner->redirectStdOut(false);
  scriptingEnv()->redirectStdOut(false);
  if (execMode == Script::Asynchronous) {
    QFuture<bool> job = runner->executeAsync(ScriptCode(code));
    while (job.isRunning()) {
      QCoreApplication::processEvents();
    }
    // Required for windows tests to work
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
  } else {
    runner->execute(ScriptCode(code));
  }
  delete runner;
}

/**
 * This is the slot for handing script execution errors. It is only
 * attached by ::executeScriptFile which is only done in the '-xq'
 * command line option.
 *
 * @param lineNumber The line number in the script that caused the error.
 */
void ApplicationWindow::onScriptExecuteSuccess(const QString &message) {
  g_log.notice() << message.toStdString() << "\n";
  this->setExitCode(0);
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
void ApplicationWindow::onScriptExecuteError(const QString &message,
                                             const QString &scriptName,
                                             int lineNumber) {
  g_log.fatal() << "Fatal error on line " << lineNumber << " of \""
                << scriptName.toStdString() << "\" encountered:\n"
                << message.toStdString();
  this->setExitCode(1);
}

/**
 * Run Python code
 * @param code :: An arbitrary string of python code
 * @param async :: If true the code will be run asynchronously but only if it
 * is called from the GUI thread
 * @param quiet :: If true then no output is produced concerning script
 * start/finished
 * @param redirect :: If true redirect stdout/stderr to results log
 */
bool ApplicationWindow::runPythonScript(const QString &code, bool async,
                                        bool quiet, bool redirect) {
  if (code.isEmpty() || m_shuttingDown)
    return false;
  if (!m_iface_script) {
    if (setScriptingLanguage("Python")) {
      m_iface_script = scriptingEnv()->newScript("<Interface>", nullptr,
                                                 Script::NonInteractive);
    } else {
      return false;
    }
  }
  if (!quiet) {
    g_log.debug("Script execution started.\n");
  }
  if (redirect) {
    m_iface_script->redirectStdOut(true);
    connect(m_iface_script, SIGNAL(print(const QString &)), resultsLog,
            SLOT(appendNotice(const QString &)));
    connect(m_iface_script,
            SIGNAL(error(const QString &, const QString &, int)), resultsLog,
            SLOT(appendError(const QString &)));
  }
  bool success(false);
  if (async) {
    const bool locked = m_iface_script->recursiveAsyncSetup();
    auto job = m_iface_script->executeAsync(ScriptCode(code));
    // Start a local event loop to keep processing events
    // while we are running the script. Inspired by the IPython
    // Qt inputhook in IPython.terminal.pt_inputhooks.qt
    QEventLoop eventLoop(QApplication::instance());
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    while (!job.isFinished()) {
      timer.start(50);
      eventLoop.exec();
      timer.stop();
    }
    m_iface_script->recursiveAsyncTeardown(locked);
    success = job.result();
  } else {
    success = m_iface_script->execute(ScriptCode(code));
  }
  if (redirect) {
    m_iface_script->redirectStdOut(false);
    disconnect(m_iface_script, SIGNAL(print(const QString &)), resultsLog,
               SLOT(appendNotice(const QString &)));
    disconnect(m_iface_script,
               SIGNAL(error(const QString &, const QString &, int)), resultsLog,
               SLOT(appendError(const QString &)));
  }
  if (success && !quiet) {
    g_log.debug("Script execution completed successfully.\n");
  }

  return success;
}

bool ApplicationWindow::validFor2DPlot(Table *table) {
  if (!table->selectedYColumns().count()) {
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Please select a Y column to plot!")); // Mantid
    return false;
  } else if (table->selectedXColumns().count() > 1) {
    QMessageBox::warning(this, tr("MantidPlot - Error"),
                         tr("Can't plot using multiple X columns!")); // Mantid
    return false;
  } else if (table->numCols() < 2) {
    QMessageBox::critical(
        this, tr("MantidPlot - Error"),
        tr("You need at least two columns for this operation!")); // Mantid
    return false;
  } else if (table->noXColumn()) {
    QMessageBox::critical(
        this, tr("MantidPlot - Error"),
        tr("Please set a default X column for this table, first!")); // Mantid
    return false;
  }
  return true;
}

MultiLayer *ApplicationWindow::generate2DGraph(GraphOptions::CurveType type) {
  MdiSubWindow *w = activeWindow();
  if (!w)
    return nullptr;

  if (w->inherits("Table")) {
    Table *table = static_cast<Table *>(w);
    if (!validFor2DPlot(table))
      return nullptr;

    return multilayerPlot(table, table->selectedColumns(), type,
                          table->topSelectedRow(), table->bottomSelectedRow());
  } else if (QString(w->metaObject()->className()) == "Matrix") {
    Matrix *m = static_cast<Matrix *>(w);
    return plotHistogram(m);
  }
  return nullptr;
}

bool ApplicationWindow::validFor3DPlot(Table *table) {
  if (table->numCols() < 2) {
    QMessageBox::critical(
        nullptr, tr("MantidPlot - Error"),
        tr("You need at least two columns for this operation!")); // Mantid
    return false;
  }
  if (table->selectedColumn() < 0 ||
      table->colPlotDesignation(table->selectedColumn()) != Table::Z) {
    QMessageBox::critical(
        nullptr, tr("MantidPlot - Error"),
        tr("Please select a Z column for this operation!")); // Mantid
    return false;
  }
  if (table->noXColumn()) {
    QMessageBox::critical(nullptr, tr("MantidPlot - Error"),
                          tr("You need to define a X column first!")); // Mantid
    return false;
  }
  if (table->noYColumn()) {
    QMessageBox::critical(nullptr, tr("MantidPlot - Error"),
                          tr("You need to define a Y column first!")); // Mantid
    return false;
  }
  return true;
}

void ApplicationWindow::hideSelectedWindows() {
  QTreeWidgetItem *item;
  QList<QTreeWidgetItem *> lst;
  for (item = lv->firstChild(); item; item = lv->itemBelow(item)) {
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach (item, lst) {
    auto wli = dynamic_cast<WindowListItem *>(item);
    if (wli)
      hideWindow(wli->window());
  }
  folders->blockSignals(false);
}

void ApplicationWindow::showSelectedWindows() {
  QTreeWidgetItem *item;
  QList<QTreeWidgetItem *> lst;
  for (item = lv->firstChild(); item; item = lv->itemBelow(item)) {
    if (item->isSelected())
      lst.append(item);
  }

  folders->blockSignals(true);
  foreach (item, lst) {
    auto wli = dynamic_cast<WindowListItem *>(item);
    if (wli)
      activateWindow(wli->window());
  }
  folders->blockSignals(false);
}

void ApplicationWindow::swapColumns() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return;
  QStringList lst = t->selectedColumns();
  if (lst.count() != 2)
    return;

  t->swapColumns(t->colIndex(lst[0]), t->colIndex(lst[1]));
}

void ApplicationWindow::moveColumnRight() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(1);
}

void ApplicationWindow::moveColumnLeft() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(-1);
}

void ApplicationWindow::moveColumnFirst() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(0 - t->selectedColumn());
}

void ApplicationWindow::moveColumnLast() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->moveColumnBy(t->numCols() - t->selectedColumn() - 1);
}

void ApplicationWindow::restoreApplicationGeometry() {
  if (d_app_rect.isNull())
    showMaximized();
  else {
    resize(d_app_rect.size());
    move(d_app_rect.topLeft());
    show();
  }
}

void ApplicationWindow::scriptsDirPathChanged(const QString &path) {
  scriptsDirPath = path;
}

void ApplicationWindow::makeToolbarsMenu() {
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

void ApplicationWindow::displayToolbars() {
  actionFileTools->setChecked(d_standard_tool_bar);
  actionPlotTools->setChecked(d_plot_tool_bar);
  actionDisplayBar->setChecked(d_display_tool_bar);
  actionFormatToolBar->setChecked(d_format_tool_bar);
  connect(actionFileTools, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionPlotTools, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionDisplayBar, SIGNAL(toggled(bool)), this, SLOT(setToolbars()));
  connect(actionFormatToolBar, SIGNAL(toggled(bool)), this,
          SLOT(setToolbars()));
  setToolbars();
}
void ApplicationWindow::setToolbars() {
  d_standard_tool_bar = actionFileTools->isChecked();
  d_plot_tool_bar = actionPlotTools->isChecked();
  d_display_tool_bar = actionDisplayBar->isChecked();
  d_format_tool_bar = actionFormatToolBar->isChecked();

  MdiSubWindow *w = activeWindow();

  standardTools->setVisible(d_standard_tool_bar);
  plotTools->setVisible(d_plot_tool_bar);
  displayBar->setVisible(d_display_tool_bar);
  formatToolBar->setVisible(d_format_tool_bar);
  plotTools->setEnabled(w &&
                        QString(w->metaObject()->className()) == "MultiLayer");
}

void ApplicationWindow::saveFitFunctions(const QStringList &lst) {
  if (!lst.count())
    return;

  QString explain = tr("Starting with version 0.9.1 MantidPlot stores the user "
                       "defined fit models to a different location.");
  explain += " " + tr("If you want to save your already defined models, please "
                      "choose a destination folder.");
  if (QMessageBox::Ok != QMessageBox::information(
                             this,
                             tr("MantidPlot") + " - " + tr("Import fit models"),
                             explain, // Mantid
                             QMessageBox::Ok, QMessageBox::Cancel))
    return;

  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Choose a directory to export the fit models to"), fitModelsPath,
      QFileDialog::ShowDirsOnly);
  if (!dir.isEmpty()) {
    fitModelsPath = dir;

    for (int i = 0; i < lst.count(); i++) {
      QString s = lst[i].simplified();
      if (!s.isEmpty()) {
        NonLinearFit *fit = new NonLinearFit(this, nullptr);

        int pos1 = s.indexOf("(", 0);
        fit->setObjectName(s.left(pos1));

        int pos2 = s.indexOf(")", pos1);
        QString par = s.mid(pos1 + 4, pos2 - pos1 - 4);
        QStringList paramList =
            par.split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts);
        fit->setParametersList(paramList);

        QStringList l = s.split("=");
        if (l.count() == 2)
          fit->setFormula(l[1]);

        fit->save(fitModelsPath + "/" + fit->objectName() + ".fit");
      }
    }
  }
}

void ApplicationWindow::matrixDirectFFT() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->fft();
}

void ApplicationWindow::matrixInverseFFT() {
  Matrix *m = dynamic_cast<Matrix *>(activeWindow(MatrixWindow));
  if (!m)
    return;

  m->fft(true);
}

void ApplicationWindow::setFormatBarFont(const QFont &font) {
  formatToolBar->setEnabled(true);

  QFontComboBox *fb =
      (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  fb->blockSignals(true);
  fb->setCurrentFont(font);
  fb->blockSignals(false);

  QSpinBox *sb =
      dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
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

void ApplicationWindow::setFontSize(int size) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb =
      (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
  QFont f(fb->currentFont().family(), size);
  f.setBold(actionFontBold->isChecked());
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::setFontFamily(const QFont &font) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  QSpinBox *sb =
      dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
  QFont f(font.family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::setItalicFont(bool italic) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = dynamic_cast<QFontComboBox *>(
      formatToolBar->widgetForAction(actionFontBox));
  QSpinBox *sb =
      dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
  QFont f(fb->currentFont().family(), sb->value());
  f.setBold(actionFontBold->isChecked());
  f.setItalic(italic);
  g->setCurrentFont(f);
}

void ApplicationWindow::setBoldFont(bool bold) {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  Graph *g = plot->activeGraph();
  if (!g)
    return;

  QFontComboBox *fb = dynamic_cast<QFontComboBox *>(
      formatToolBar->widgetForAction(actionFontBox));
  QSpinBox *sb =
      dynamic_cast<QSpinBox *>(formatToolBar->widgetForAction(actionFontSize));
  QFont f(fb->currentFont().family(), sb->value());
  f.setBold(bold);
  f.setItalic(actionFontItalic->isChecked());
  g->setCurrentFont(f);
}

void ApplicationWindow::enableTextEditor(Graph *g) {
  if (!g) {
    formatToolBar->setEnabled(false);
    if (d_text_editor) {
      d_text_editor->close();
      d_text_editor = nullptr;
    }
  } else if (g) {
    d_text_editor = new TextEditor(g);
    connect(d_text_editor, SIGNAL(textEditorDeleted()), this,
            SLOT(cleanTextEditor()));

    formatToolBar->setEnabled(true);
    actionSubscript->setEnabled(true);
    actionSuperscript->setEnabled(true);
    actionUnderline->setEnabled(true);
    actionGreekSymbol->setEnabled(true);
    actionGreekMajSymbol->setEnabled(true);
    actionMathSymbol->setEnabled(true);
  }
}

void ApplicationWindow::cleanTextEditor() { d_text_editor = nullptr; }

void ApplicationWindow::insertSuperscript() {
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<sup>", "</sup>");
}

void ApplicationWindow::insertSubscript() {
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<sub>", "</sub>");
}

void ApplicationWindow::underline() {
  if (!d_text_editor)
    return;

  d_text_editor->formatText("<u>", "</u>");
}

void ApplicationWindow::insertGreekSymbol() {
  if (!d_text_editor)
    return;

  SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::lowerGreek, this);
  connect(greekLetters, SIGNAL(addLetter(const QString &)), d_text_editor,
          SLOT(addSymbol(const QString &)));
  greekLetters->exec();
}

void ApplicationWindow::insertGreekMajSymbol() {
  if (!d_text_editor)
    return;

  SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::upperGreek, this);
  connect(greekLetters, SIGNAL(addLetter(const QString &)), d_text_editor,
          SLOT(addSymbol(const QString &)));
  greekLetters->exec();
}

void ApplicationWindow::insertMathSymbol() {
  if (!d_text_editor)
    return;

  SymbolDialog *ms = new SymbolDialog(SymbolDialog::mathSymbols, this);
  connect(ms, SIGNAL(addLetter(const QString &)), d_text_editor,
          SLOT(addSymbol(const QString &)));
  ms->exec();
}

void ApplicationWindow::showCustomActionDialog() {
  ManageCustomMenus *ad = new ManageCustomMenus(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void ApplicationWindow::showInterfaceCategoriesDialog() {
  auto existingWindow = this->findChild<ManageInterfaceCategories *>();
  if (!existingWindow) {
    auto *diag = new ManageInterfaceCategories(this);
    diag->setAttribute(Qt::WA_DeleteOnClose);
    diag->show();
    diag->setFocus();
  } else
    existingWindow->activateWindow();
}

void ApplicationWindow::showUserDirectoryDialog() {
  MantidQt::API::ManageUserDirectories *ad =
      new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void ApplicationWindow::addCustomAction(QAction *action,
                                        const QString &parentName, int index) {
  if (!action)
    return;

  QList<QToolBar *> toolBars = toolBarsList();
  foreach (QToolBar *t, toolBars) {
    if (t->objectName() == parentName) {
      t->addAction(action);
      if (index < 0)
        d_user_actions << action;
      else if (index >= 0 && index < d_user_actions.size())
        d_user_actions.replace(index, action);
      return;
    }
  }

  QList<QMenu *> menus = customizableMenusList();
  foreach (QMenu *m, menus) {
    if (m->objectName() == parentName) {
      m->addAction(action);
      if (index < 0)
        d_user_actions << action;
      else if (index >= 0 && index < d_user_actions.size())
        d_user_actions.replace(index, action);
      return;
    }
  }
}

void ApplicationWindow::reloadCustomActions() {
  QList<QMenu *> menus = customizableMenusList();
  foreach (QAction *a, d_user_actions) {
    if (!a->statusTip().isEmpty()) {
      foreach (QMenu *m, menus) {
        if (m->objectName() == a->statusTip()) {
          QList<QAction *> lst = m->actions();
          if (!lst.contains(a))
            m->addAction(a);
        }
      }
    }
  }
}

void ApplicationWindow::removeCustomAction(QAction *action) {
  int index = d_user_actions.indexOf(action);
  if (index >= 0 && index < d_user_actions.count()) {
    d_user_actions.removeAt(index);
    delete action;
  }
}

void ApplicationWindow::performCustomAction(QAction *action) {
  if (!action ||
      !(d_user_actions.contains(action) || m_interfaceActions.contains(action)))
    return;
#ifdef SCRIPTING_PYTHON
  QString action_data = action->data().toString();
  if (QFileInfo(action_data).exists()) {
    QFile script_file(action_data);
    if (!script_file.open(QIODevice::ReadOnly)) {
      QMessageBox::information(this, "MantidPlot",
                               "Error: There was a problem reading\n" +
                                   action_data);
      return;
    }

    QTextStream stream(&script_file);
    QString scriptPath =
        QString("r'%1'").arg(QFileInfo(action_data).absolutePath());
    QString code = QString("sys.path.append(%1)\n").arg(scriptPath);
    runPythonScript(code, false, true);
    code = "";
    while (!stream.atEnd()) {
      code.append(stream.readLine() + "\n");
    }
    runPythonScript(code, false, true);
    code = "";
    code.append(QString("\nsys.path.remove(%1)").arg(scriptPath));
    runPythonScript(code, false, true);
  } else {
    // Check to see if the window is already open.  If so, just show it to the
    // user.
    foreach (auto userSubWindow, this->findChildren<UserSubWindow *>()) {
      if (userSubWindow->objectName() == action_data) {
        userSubWindow->activateWindow();
        return;
      }
    }
    // Enables/Disables the toolbar

    MdiSubWindow *usr_win = new MdiSubWindow(this);
    usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
    MantidQt::API::InterfaceManager interfaceManager;
    MantidQt::API::UserSubWindow *user_interface =
        interfaceManager.createSubWindow(action_data, usr_win);
    if (user_interface) {
      setGeometry(usr_win, user_interface);
      connect(user_interface, SIGNAL(runAsPythonScript(const QString &, bool)),
              this, SLOT(runPythonScript(const QString &, bool)),
              Qt::DirectConnection);
      // Update the used fit property browser
      connect(user_interface,
              SIGNAL(setFitPropertyBrowser(
                  MantidQt::MantidWidgets::FitPropertyBrowser *)),
              mantidUI,
              SLOT(setFitFunctionBrowser(
                  MantidQt::MantidWidgets::FitPropertyBrowser *)));
      user_interface->initializeLocalPython();
    } else {
      delete usr_win;
    }
  }
#else
  QMessageBox::critical(
      this, tr("MantidPlot") + " - " + tr("Error"), // Mantid
      tr("MantidPlot was not built with Python scripting support included!"));
#endif
}

void ApplicationWindow::loadCustomActions() {
  QString path = customActionsDirPath + "/";
  QDir dir(path);
  QStringList lst = dir.entryList(QDir::Files | QDir::NoSymLinks, QDir::Name);
  for (int i = 0; i < lst.count(); i++) {
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

QList<QMenu *> ApplicationWindow::customizableMenusList() {
  QList<QMenu *> lst;
  lst << windowsMenu << view << graph << fileMenu << format << edit;
  lst << help << plot2DMenu << analysisMenu;
  lst << matrixMenu << plot3DMenu << plotDataMenu; // scriptingMenu;
  lst << tableMenu << fillMenu << normMenu << newMenu << exportPlotMenu
      << smoothMenu;
  lst << filterMenu << decayMenu;
  return lst;
}

//-------------------------------
// Mantid
void ApplicationWindow::addUserMenu(const QString &topMenu) {
  if (topMenu.isEmpty())
    return;

  foreach (QMenu *menu, d_user_menus) {
    if (menu->title() == topMenu)
      return;
  }

  QMenu *customMenu = new QMenu(topMenu);
  customMenu->setTitle(topMenu);
  customMenu->setObjectName(topMenu.toAscii().constData());
  connect(customMenu, SIGNAL(triggered(QAction *)), this,
          SLOT(performCustomAction(QAction *)));
  d_user_menus.append(customMenu);
  myMenuBar()->addMenu(customMenu)->setText(tr(topMenu.toAscii().constData()));
}

void ApplicationWindow::addUserMenuAction(const QString &parentMenu,
                                          const QString &itemName,
                                          const QString &itemData) {
  QString niceName = QString(itemName).replace("_", " ");
  QMenu *topMenu(nullptr);
  foreach (topMenu, d_user_menus) {
    if (topMenu->title() == parentMenu)
      break;
  }

  if (!topMenu)
    return;
  foreach (QAction *userAction, topMenu->actions()) {
    if (userAction->text() == niceName)
      return;
  }

  QAction *scriptAction =
      new QAction(tr(niceName.toAscii().constData()), topMenu);
  scriptAction->setData(itemData);
  topMenu->addAction(scriptAction);
  d_user_actions.append(scriptAction);

  // Remove name from the list of removed interfaces if applicable
  removed_interfaces.removeAll(niceName);
}

void ApplicationWindow::removeUserMenu(const QString &parentMenu) {
  int i(0);
  QMenu *menu = nullptr;
  foreach (menu, d_user_menus) {
    if (menu->title() == parentMenu)
      break;
    ++i;
  }
  if (!menu)
    return;

  d_user_menus.removeAt(i);
  myMenuBar()->removeAction(menu->menuAction());
}

void ApplicationWindow::removeUserMenuAction(const QString &parentMenu,
                                             const QString &userAction) {
  QMenu *menu = nullptr;
  foreach (menu, d_user_menus) {
    if (menu->title() == parentMenu)
      break;
  }
  if (!menu)
    return;

  QAction *action = nullptr;
  int menu_count(0);
  foreach (action, d_user_actions) {
    if (action->text() == userAction)
      break;
    ++menu_count;
  }
  if (!action)
    return;

  d_user_actions.removeAt(menu_count);
  menu->removeAction(action);

  // Add interface name to the list of removed interfaces
  removed_interfaces.append(userAction);
}

const QList<QMenu *> &ApplicationWindow::getCustomMenus() const {
  return d_user_menus;
}

QList<QMenu *> ApplicationWindow::menusList() {
  QList<QMenu *> lst;
  QObjectList children = this->children();
  foreach (QObject *w, children) {
    if (QString(w->metaObject()->className()) == "QMenu")
      lst << static_cast<QMenu *>(w);
  }
  return lst;
}

// End of a section of Mantid custom functions
//-------------------------------------------

QList<QToolBar *> ApplicationWindow::toolBarsList() const {
  QList<QToolBar *> lst;
  QObjectList children = this->children();
  foreach (QObject *w, children) {
    if (QString(w->metaObject()->className()) == "QToolBar")
      lst << static_cast<QToolBar *>(w);
  }
  return lst;
}

void ApplicationWindow::hideSelectedColumns() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->hideSelectedColumns();
}

void ApplicationWindow::showAllColumns() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (t)
    t->showAllColumns();
}

void ApplicationWindow::setMatrixUndoStackSize(int size) {
  if (d_matrix_undo_stack_size == size)
    return;

  d_matrix_undo_stack_size = size;
  Folder *f = projectFolder();
  while (f) {
    QList<MdiSubWindow *> folderWindows = f->windowsList();
    foreach (MdiSubWindow *w, folderWindows) {
      if (this->isOfType(w, "Matrix")) {
        auto matrix = dynamic_cast<Matrix *>(w);
        if (!matrix)
          continue;
        QUndoStack *stack = matrix->undoStack();
        if (!stack->count()) // undo limit can only be changed for empty stacks
          stack->setUndoLimit(size);
      }
    }
    f = f->folderBelow();
  }
}

/**
 * Arange the mdi sub-windows in a tile pattern
 */
void ApplicationWindow::tileMdiWindows() {
  d_workspace->tileSubWindows();
  // hack to redraw the graphs
  shakeViewport();
  // QMdiArea::tileSubWindows() aranges the windows and enables automatic
  // tiling after subsequent resizing of the mdi area until a window is moved
  // or resized separately. Unfortunately Graph behaves badly during this. The
  // following code disables automatic tiling.
  auto winList = d_workspace->subWindowList();
  if (!winList.isEmpty()) {
    auto p = winList[0]->pos();
    winList[0]->move(p.x() + 1, p.y());
    winList[0]->move(p);
  }
}

/**
 * A hack to make the mdi area and the Graphs to redraw themselves in certain
 * cases.
 */
void ApplicationWindow::shakeViewport() {
  QWidget *viewPort = d_workspace->viewport();
  QSize size = viewPort->size();
  viewPort->resize(QSize(size.width() + 1, size.height() + 1));
  viewPort->resize(size);
}

QString ApplicationWindow::endOfLine() {
  switch (d_eol) {
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
void ApplicationWindow::customMultilayerToolButtons(MultiLayer *w) {
  if (!w) {
    btnPointer->setChecked(true);
    return;
  }

  btnMultiPeakPick->setEnabled(w->layers() == 1);

  Graph *g = w->activeGraph();
  if (g) {
    PlotToolInterface *tool = g->activeTool();
    if (g->zoomOn())
      btnZoomIn->setChecked(true);

    else if (g->areRangeSelectorsOn()) {
    } else if (dynamic_cast<PeakPickerTool *>(tool))
      btnMultiPeakPick->setChecked(true);
    else if (dynamic_cast<DataPickerTool *>(tool)) {
      switch (dynamic_cast<DataPickerTool *>(tool)->getMode()) {
      case DataPickerTool::Move:
        btnMovePoints->setChecked(true);
        break;
      case DataPickerTool::Remove:
        btnRemovePoints->setChecked(true);
        break;
      case DataPickerTool::Display:
        btnCursor->setChecked(true);
        break;
      default:
        btnPointer->setChecked(true);
      }
    } else if (dynamic_cast<DrawPointTool *>(tool))
      actionDrawPoints->setChecked(true);
    else if (dynamic_cast<ScreenPickerTool *>(tool))
      btnPicker->setChecked(true);
    else if (dynamic_cast<LabelTool *>(tool))
      btnLabel->setChecked(true);
    else
      btnPointer->setChecked(true);
  } else
    btnPointer->setChecked(true);
}
/**  save workspace data in nexus format
 *   @param wsName :: name of the output file.
 *   @param fileName :: name of the output file.
 */
void ApplicationWindow::savedatainNexusFormat(const std::string &wsName,
                                              const std::string &fileName) {
  if (fileName.empty())
    return;
  try {
    if (mantidUI)
      mantidUI->savedatainNexusFormat(fileName, wsName);
  } catch (...) {
  }
}

void ApplicationWindow::enableSaveNexus(const QString &wsName) {
  if (actionSaveFile)
    actionSaveFile->setEnabled(true);

  m_nexusInputWSName = wsName;
}

void ApplicationWindow::disableSaveNexus() {
  if (actionSaveFile)
    actionSaveFile->setEnabled(false);
}

/* For zooming the selected graph using the drag canvas tool and mouse drag.
 */
void ApplicationWindow::panOnPlot() {
  MultiLayer *plot = dynamic_cast<MultiLayer *>(activeWindow(MultiLayerWindow));
  if (!plot)
    return;

  if (plot->isEmpty()) {
    QMessageBox::warning(
        this, tr("QtiPlot - Warning"),
        tr("<h4>There are no plot layers available in this window.</h4>"
           "<p><h4>Please add a layer and try again!</h4>"));
    btnPointer->setChecked(true);
    return;
  }

  QList<Graph *> layers = plot->layersList();
  foreach (Graph *g, layers)
    g->enablePanningMagnifier();
}
/// Handler for ICat Login Menu
void ApplicationWindow::populateCatalogLoginMenu() {
  icat->clear();
  icat->addAction(actionCatalogLogin);
  if (Mantid::API::CatalogManager::Instance().numberActiveSessions() > 0) {
    icat->addAction(actionCatalogSearch);
    icat->addAction(actionCatalogPublish);
    icat->addAction(actionCatalogLogout);
  }
}

void ApplicationWindow::CatalogLogin() {
  MantidQt::MantidWidgets::CatalogHelper().showLoginDialog();
}

void ApplicationWindow::CatalogSearch() {
  // Only one ICAT GUI will appear, and that the previous one will be
  // overridden.
  // E.g. if a user opens the ICAT GUI without being logged into ICAT they
  // will need to
  // login in and then click "Search" again.
  catalogSearch.reset(new MantidQt::MantidWidgets::CatalogSearch());
  catalogSearch->show();
  catalogSearch->raise();
}

void ApplicationWindow::CatalogPublish() {
  MantidQt::MantidWidgets::CatalogHelper().showPublishDialog();
}

void ApplicationWindow::CatalogLogout() {
  auto logout = mantidUI->createAlgorithm("CatalogLogout");
  mantidUI->executeAlgorithmAsync(logout);
  icat->removeAction(actionCatalogSearch);
  icat->removeAction(actionCatalogPublish);
  icat->removeAction(actionCatalogLogout);
}

void ApplicationWindow::setGeometry(MdiSubWindow *usr_win,
                                    QWidget *user_interface) {
  QRect frame = QRect(
      usr_win->frameGeometry().topLeft() - usr_win->geometry().topLeft(),
      usr_win->geometry().bottomRight() - usr_win->geometry().bottomRight());
  usr_win->setWidget(user_interface);
  QRect iface_geom =
      QRect(frame.topLeft() + user_interface->geometry().topLeft(),
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
void ApplicationWindow::writeToLogWindow(
    const MantidQt::MantidWidgets::Message &msg) {
  resultsLog->append(msg);
}

MultiLayer *ApplicationWindow::waterfallPlot() {
  Table *t = dynamic_cast<Table *>(activeWindow(TableWindow));
  if (!t)
    return nullptr;

  return waterfallPlot(t, t->selectedYColumns());
}

MultiLayer *ApplicationWindow::waterfallPlot(Table *t,
                                             const QStringList &list) {
  if (!t)
    return nullptr;

  if (list.count() < 1) {
    QMessageBox::warning(this, tr("MantidPlot - Plot error"),
                         tr("Please select a Y column to plot!"));
    return nullptr;
  }

  MultiLayer *ml = new MultiLayer(this);

  Graph *g = ml->activeGraph(); // Layer();
  setPreferences(g);
  g->enableAxis(QwtPlot::xTop, false);
  g->enableAxis(QwtPlot::yRight, false);
  g->setCanvasFrame(0);
  g->setTitle(QString::null);
  g->setMargin(0);
  g->setFrame(0);
  g->addCurves(t, list, GraphOptions::Line);
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
 * Add a sub-window either as a docked or a floating window. The desision is
 * made by isDefalutFloating() method.
 * @param w :: Pointer to a MdiSubWindow which to add.
 * @param showNormal :: If true (default) show as a normal window, if false
 * show as a minimized docked window regardless of what isDefalutFloating()
 * returns.
 */
void ApplicationWindow::addMdiSubWindow(MdiSubWindow *w, bool showNormal) {
  addMdiSubWindow(w, isDefaultFloating(w), showNormal);
}

/**
 * Add a sub-window either as a docked or a floating window.
 * @param w :: Pointer to a MdiSubWindow which to add.
 * @param showFloating :: If true show as floating else make it docked.
 * @param showNormal :: If true show as a normal window, if false show as a
 * minimized docked window
 *   regardless of what showFloating is.
 */
void ApplicationWindow::addMdiSubWindow(MdiSubWindow *w, bool showFloating,
                                        bool showNormal) {
  addListViewItem(w);
  currentFolder()->addWindow(w);

  connect(w, SIGNAL(modifiedWindow(MdiSubWindow *)), this,
          SLOT(modifiedProject(MdiSubWindow *)));
  connect(w, SIGNAL(resizedWindow(MdiSubWindow *)), this,
          SLOT(modifiedProject(MdiSubWindow *)));
  connect(w, SIGNAL(closedWindow(MdiSubWindow *)), this,
          SLOT(closeWindow(MdiSubWindow *)));
  connect(w, SIGNAL(hiddenWindow(MdiSubWindow *)), this,
          SLOT(hideWindow(MdiSubWindow *)));
  connect(w, SIGNAL(statusChanged(MdiSubWindow *)), this,
          SLOT(updateWindowStatus(MdiSubWindow *)));
  connect(w, SIGNAL(showContextMenu()), this, SLOT(showWindowContextMenu()));
  connect(w, SIGNAL(detachFromParent(MdiSubWindow *)), this,
          SLOT(detachMdiSubwindow(MdiSubWindow *)));

  if (showFloating && showNormal) {
    addMdiSubWindowAsFloating(w);
  } else {
    QMdiSubWindow *sw = addMdiSubWindowAsDocked(w);
    if (showNormal) {
      sw->showNormal();
    } else {
      sw->showMinimized();
    }
  }

  modifiedProject(w);
}

/**
 * Add a sub-window to as a floating window.
 * @param w :: Pointer to a MdiSubWindow which will be wrapped in a
 * FloatingWindow.
 * @param pos :: Position of created window relative to the main window.
 *   Setting it to (-1,-1) means no autogenerate the position.
 */
FloatingWindow *ApplicationWindow::addMdiSubWindowAsFloating(MdiSubWindow *w,
                                                             QPoint pos) {
  const QPoint none(-1, -1);
  FloatingWindow *fw = new FloatingWindow(this); //, Qt::WindowStaysOnTopHint);
  QSize sz = w->size();
  if (pos == none) {
    pos = positionNewFloatingWindow(sz);
  } else {
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
 * Returns the top-left corner of the ADI area available for sub-windows
 * relative
 * to the top-left corner of the monitor screen.
 *
 */
QPoint ApplicationWindow::mdiAreaTopLeft() const {
  QPoint p = this->pos() + d_workspace->pos();

  // make sure the floating window doesn't overlap the tool bars
  QList<QToolBar *> toolBars = toolBarsList();
  foreach (QToolBar *bar, toolBars) {
    if (toolBarArea(bar) != Qt::TopToolBarArea)
      continue;
    int y = this->pos().y() + d_workspace->pos().y() + bar->rect().bottom();
    if (y > p.y())
      p.setY(y + 1);
  }
  return p;
}

/**
 * Find the best position for a new floating window.
 * @param sz :: Size of the new window.
 */
QPoint ApplicationWindow::positionNewFloatingWindow(QSize sz) const {
  const QPoint noPoint(-1, -1);

  static QPoint lastPoint(noPoint);

  if (lastPoint == noPoint ||
      m_floatingWindows.isEmpty()) { // If no other windows added - start from
                                     // top-left corner
    lastPoint = mdiAreaTopLeft();
  } else {
    // Get window which was added last
    FloatingWindow *lastWindow = m_floatingWindows.last();

    if (lastWindow->isVisible()) { // If it is still visible - can't use it's
                                   // location, so need to find a new one

      QPoint diff = lastWindow->pos() - lastPoint;

      if (abs(diff.x()) < 20 &&
          abs(diff.y()) < 20) { // If window was moved far enough from it's
                                // previous location - can use it

        // Get a screen space which we can use
        const QRect screen = QApplication::desktop()->availableGeometry(this);

        // How mush we need to move in X so that cascading direction is
        // diagonal according to screen size
        const int yDelta = 40;
        const int xDelta =
            static_cast<int>(yDelta * (1.0 * screen.width() / screen.height()));

        lastPoint += QPoint(xDelta, yDelta);

        const QRect newPlace = QRect(lastPoint, sz);
        if (newPlace.bottom() > screen.height() ||
            newPlace.right() > screen.width())
          // If new window doesn't fit to the screen - start anew
          lastPoint = mdiAreaTopLeft();
      }
    }
  }

  return lastPoint;
}

/**
 * Add a sub-window to as a docked MDI window.
 * @param w :: Pointer to a MdiSubWindow which will be wrapped in a
 * QMdiSubWindow.
 * @param pos :: Position of the top-left corner of the new window.
 */
QMdiSubWindow *ApplicationWindow::addMdiSubWindowAsDocked(MdiSubWindow *w,
                                                          QPoint pos) {
  DockedWindow *dw = new DockedWindow(this);
  dw->setMdiSubWindow(w);
  QMdiSubWindow *sw = this->d_workspace->addSubWindow(dw);
  sw->resize(w->size());
  sw->setWindowIcon(w->windowIcon());
  if (pos != QPoint(-1, -1)) {
    sw->move(pos);
  }
  return sw;
}

/**
 * Make a subwindow floating.
 */
void ApplicationWindow::changeToFloating(MdiSubWindow *w) {
  if (w->isFloating())
    return;
  QMdiSubWindow *sw = w->getDockedWindow();
  if (sw) {
    // remove the subwindow from the mdi area
    d_workspace->removeSubWindow(w);
    sw->close();
    // create the outer floating window.
    addMdiSubWindowAsFloating(w, sw->pos());
  } else {
    // attach w to the ApplicationWindow and create the outer floating window
    // (second argument == true)
    addMdiSubWindow(w, true, true);
  }
  activateWindow(w);
}

/**
 * Return a floating subwindow to the mdi area.
 */
void ApplicationWindow::changeToDocked(MdiSubWindow *w) {
  if (w->isDocked())
    return;
  FloatingWindow *fw = w->getFloatingWindow();
  if (fw) {
    fw->removeMdiSubWindow();
    removeFloatingWindow(fw);
    // main window must be closed or application will freeze
    fw->close();
    // create the outer docked window.
    addMdiSubWindowAsDocked(w);
  } else {
    // attach w to the ApplicationWindow and create the outer docked window
    // (second argument == false)
    addMdiSubWindow(w, false, true);
  }
  w->setNormal();
  return;
}

/**
 * Remove a closed floating window from internal lists.
 * @param w :: Pointer to the closed window.
 */
void ApplicationWindow::removeFloatingWindow(FloatingWindow *w) {
  if (m_floatingWindows.contains(w)) {
    m_floatingWindows.removeAll(w);
    if (w->mdiSubWindow()) {
      closeWindow(w->mdiSubWindow());
    }
    // Make the FloatingWindow delete itself
    w->deleteLater();
  }
}

/**
 * Return a pointer to the active FloatingWindow if the active window is
 * floating
 * or NULL otherwise.
 */
FloatingWindow *ApplicationWindow::getActiveFloating() const {
  MdiSubWindow *w = getActiveWindow();
  if (!w)
    return nullptr;
  return w->getFloatingWindow();
}

/**
 * Detach a subwindow from its parent - docked or floating.
 * It isn't full detachment - signals are still connected.
 */
void ApplicationWindow::detachMdiSubwindow(MdiSubWindow *w) {
  // remove the window from all internal lists
  if (currentFolder()->hasWindow(w)) {
    currentFolder()->removeWindow(w);
  }
  removeWindowFromLists(w);
  auto found = lv->findItems(w->objectName(),
                             Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
  if (!found.isEmpty())
    lv->takeTopLevelItem(lv->indexOfTopLevelItem(found[0]));

  // if it's wrapped in a floating detach from it and close
  FloatingWindow *fw = w->getFloatingWindow();
  if (fw) {
    fw->removeMdiSubWindow();
    m_floatingWindows.removeAll(fw);
    fw->deleteLater();
    return;
  }

  // the same in docked case
  QMdiSubWindow *dw = w->getDockedWindow();
  if (dw) {
    d_workspace->removeSubWindow(w);
    dw->close();
  }
}

/**
 * Filter out the WindowActivate event and set the active subwindow correctly.
 * @param e :: An event.
 */
bool ApplicationWindow::event(QEvent *e) {
  if (e->type() == QEvent::WindowActivate) {
    bool needToActivate = true;

    // check if old active window is a floating one and this window was
    // activated by clicking
    // on a tool bar - in this case we shouldn't actvate another window
    if (getActiveFloating()) {

      QPoint cur_pos = this->mapFromGlobal(QCursor::pos());
      const QWidget *clickedWidget = nullptr;

      if (rect().contains(cur_pos)) {
        clickedWidget = childAt(cur_pos);
      }

      if (clickedWidget) {
        QString class_name = clickedWidget->metaObject()->className();
        if (class_name == "QToolButton" || class_name == "QToolBar" ||
            class_name == "QMenuBar") {
          needToActivate = false;
        }
      }
    }

    if (needToActivate) { // activate current MDI subwindow
      QMdiSubWindow *qCurrent = d_workspace->currentSubWindow();
      if (qCurrent) {
        QWidget *wgt = qCurrent->widget();
        MdiSubWindow *sw = dynamic_cast<MdiSubWindow *>(wgt);
        if (!sw) { // this should never happen - all MDI subwindow widgets
                   // must inherit from MdiSubwindow
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
void ApplicationWindow::mdiWindowActivated(MdiSubWindow *w) {
  if (!w)
    return;
  setActiveWindow(w);
}

/**
 * Activate a subwindow (docked or floating) other than current active one.
 * This is required when the current window is closing.
 */
void ApplicationWindow::activateNewWindow() {
  MdiSubWindow *current = getActiveWindow();
  MdiSubWindow *newone = nullptr;
  Folder *folder = currentFolder();

  // try the docked windows first
  QList<QMdiSubWindow *> wl =
      d_workspace->subWindowList(QMdiArea::ActivationHistoryOrder);
  if (!wl.isEmpty()) {
    for (int i = wl.size() - 1; i >= 0; --i) {
      QMdiSubWindow *w = wl[i];
      if (w->widget() != static_cast<QWidget *>(current)) {
        MdiSubWindow *sw = dynamic_cast<MdiSubWindow *>(w->widget());
        if (sw && sw->status() != MdiSubWindow::Minimized &&
            sw->status() != MdiSubWindow::Hidden && folder->hasWindow(sw)) {
          newone = sw;
          break;
        }
      }
    }
  }

  // if unsuccessful try the floating windows
  if (!newone) {
    foreach (FloatingWindow *w, m_floatingWindows) {
      MdiSubWindow *sw = w->mdiSubWindow();
      if (sw != current) {
        if (sw && sw->status() != MdiSubWindow::Minimized &&
            sw->status() != MdiSubWindow::Hidden && folder->hasWindow(sw)) {
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
void ApplicationWindow::changeActiveToFloating() {
  MdiSubWindow *activeWin = activeWindow();
  changeToFloating(activeWin);
}

/**
 * The slot to change the active window from floating to docked.
 */
void ApplicationWindow::changeActiveToDocked() {
  MdiSubWindow *activeWin = activeWindow();
  changeToDocked(activeWin);
}

/**
 * Returns if a window should be made floating by default.
 * @param w :: Pointer to a MdiSubWindow.
 */
bool ApplicationWindow::isDefaultFloating(const MdiSubWindow *w) const {
  QString wClassName = w->metaObject()->className();
  return isDefaultFloating(wClassName);
}

/**
 * Returns if a window should be made floating by default.
 * @param aClassName :: Class name of a MdiSubWindow or its internal widget in
 * case of custom interfaces.
 */
bool ApplicationWindow::isDefaultFloating(const QString &aClassName) const {
  bool theDefault = false;
#ifndef Q_OS_LINUX
  if (aClassName == "MultiLayer" || aClassName == "InstrumentWindow" ||
      aClassName == "MdiSubWindow") {
    theDefault = true;
  }
#endif
  return settings.value("/General/FloatingWindows/" + aClassName, theDefault)
      .toBool();
}

/**
 * Check that a widow will be visible if moved to these coordinates and
 * set them to default values otherwise.
 * @param w :: Pointer to a sub-window.
 * @param x :: Tested x coordinate
 * @param y :: Tested y coordinate
 */
void ApplicationWindow::validateWindowPos(MdiSubWindow *w, int &x, int &y) {
  QSize sz = w->size();
  if (w->getFloatingWindow()) {
    QWidget *desktop = QApplication::desktop()->screen();
    QPoint pos(x, y);
    pos += mdiAreaTopLeft();
    if (pos.x() < 0 || pos.y() < 0 || pos.x() + sz.width() > desktop->width() ||
        pos.y() + sz.height() > desktop->height()) {
      pos = positionNewFloatingWindow(sz);
    }
    x = pos.x();
    y = pos.y();
    return;
  } else if (x < 0 || y < 0 || x + sz.width() > d_workspace->width() ||
             y + sz.height() > d_workspace->height()) {
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
void ApplicationWindow::onAboutToStart() {
  if (m_exec_on_start) {
    if (m_quit_after_exec) {
      try {
        // Script completion triggers close with correct code automatically
        executeScriptFile(m_cmdline_filename, Script::Asynchronous);
      } catch (std::runtime_error &exc) {
        std::cerr << "Error thrown while running script file asynchronously '"
                  << exc.what() << "'\n";
        this->setExitCode(1);
      }
      saved = true;
      this->close();
      return;
    } else {
      scriptingWindow->executeCurrentTab(Script::Asynchronous);
    }
  }

  // Show first time set up
  if (d_showFirstTimeSetup)
    showFirstTimeSetup();

  // triggers the execution of UpdateScriptRepository Algorithm in a separated
  // thread.
  // this was necessary because in order to log while in a separate thread, it
  // is necessary to have
  // the postEvents available, so, we need to execute it here at about2Start.
  std::string local_rep = Mantid::Kernel::ConfigService::Instance().getString(
      "ScriptLocalRepository");
  if (!local_rep.empty()) {
    // there is no reason to trigger UpdataScriptRepository if it has never
    // been installed
    Mantid::API::IAlgorithm_sptr update_script_repo =
        mantidUI->createAlgorithm("UpdateScriptRepository");
    update_script_repo->initialize();
    update_script_repo->setLoggingOffset(1);
    mantidUI->executeAlgorithmAsync(update_script_repo);
  }

  // Make sure we see all of the startup messages
  resultsLog->scrollToTop();

  // Kick off project recovery
  if (Mantid::Kernel::ConfigService::Instance().getString(
          "projectRecovery.enabled") == "true") {
    g_log.debug("Starting project autosaving.");
    checkForProjectRecovery();
  } else {
    g_log.debug("Project Recovery is disabled.");
  }
}

/**
 * Create a new TiledWindow with default settings.
 */
TiledWindow *ApplicationWindow::newTiledWindow() {
  TiledWindow *widget =
      new TiledWindow(this, "", generateUniqueName("TiledWindow"), 2, 2);
  addMdiSubWindow(widget);
  return widget;
}

/**
 * Check if there is an open TiledWindow.
 */
bool ApplicationWindow::hasTiledWindowOpen() {
  // check the docked windows
  auto wl = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach (QMdiSubWindow *w, wl) {
    TiledWindow *tw = dynamic_cast<TiledWindow *>(w->widget());
    if (tw && tw->isVisible()) {
      return true;
    }
  }
  // check the floating windows
  foreach (FloatingWindow *w, m_floatingWindows) {
    TiledWindow *tw = dynamic_cast<TiledWindow *>(w->mdiSubWindow());
    if (tw && tw->isVisible()) {
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
TiledWindow *ApplicationWindow::getTiledWindowAtPos(QPoint pos) {
  // check the docked windows
  auto wl = d_workspace->subWindowList(QMdiArea::StackingOrder);
  foreach (QMdiSubWindow *w, wl) {
    TiledWindow *tw = dynamic_cast<TiledWindow *>(w->widget());
    if (tw) {
      QPoint mdiOrigin = mapToGlobal(w->pos());
      auto r = w->visibleRegion().boundingRect();
      r.translate(mdiOrigin);
      if (r.contains(pos)) {
        return tw;
      }
    }
  }
  // check the floating windows
  foreach (FloatingWindow *w, m_floatingWindows) {
    TiledWindow *tw = dynamic_cast<TiledWindow *>(w->mdiSubWindow());
    if (tw) {
      QPoint mdiOrigin = mapToGlobal(w->pos());
      auto r = w->visibleRegion().boundingRect();
      r.translate(mdiOrigin);
      if (r.contains(pos)) {
        return tw;
      }
    }
  }
  return nullptr;
}

/**
 * Check if a point is inside any of visible TiledWindows.
 * @param x :: The x-coord to check (in global coordinates).
 * @param y :: The y-coord to check (in global coordinates).
 */
bool ApplicationWindow::isInTiledWindow(QPoint pos) {
  auto w = getTiledWindowAtPos(pos);
  if (w != nullptr) {
    w->showInsertPosition(pos);
    return true;
  }
  return false;
}

/**
 * @param w :: An MdiSubWindow.
 * @param x :: The x-coord to check (in global coordinates).
 * @param y :: The y-coord to check (in global coordinates).
 */
void ApplicationWindow::dropInTiledWindow(MdiSubWindow *w, QPoint pos) {
  auto tw = getTiledWindowAtPos(pos);
  if (tw != nullptr) {
    tw->dropAtPosition(w, pos);
  }
}

bool ApplicationWindow::isOfType(const QObject *obj,
                                 const char *toCompare) const {
  return strcmp(obj->metaObject()->className(), toCompare) == 0;
}

/**
 * Loads a project file as part of project recovery
 *
 * @param sourceFile The full path to the .project file
 * @return True is loading was successful, false otherwise
 */
bool ApplicationWindow::loadProjectRecovery(std::string sourceFile,
                                            std::string recoveryFolder) {
  // Wait on this thread until scriptWindow is finished (Should be a seperate
  // thread)
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  } while (scriptingWindow->isExecuting());
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(this, isRecovery);
  // File version is not applicable to project recovery - so set to 0
  const auto loadSuccess = projectWriter.load(sourceFile, 0);

  // Handle the removal of old checkpoints and start project saving again
  Poco::Path deletePath(recoveryFolder);
  deletePath.setFileName("");
  deletePath.popDirectory();
  m_projectRecovery.clearAllCheckpoints(deletePath);
  m_projectRecovery.startProjectSaving();

  return loadSuccess;
}

/**
 * Triggers saving project recovery on behalf of an external thread
 * or caller, such as project recovery.
 *
 * @param destination:: The full path to write the recovery file to
 * @return True if saving is successful, false otherwise
 */
bool ApplicationWindow::saveProjectRecovery(std::string destination) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(this, isRecovery);
  return projectWriter.save(QString::fromStdString(destination));
}

/**
 * Checks for any recovery checkpoint and starts project
 * saving if one doesn't exist. If one does, it prompts
 * the user whether they would like to recover
 */
void ApplicationWindow::checkForProjectRecovery() {
  m_projectRecoveryRunOnStart = true;

  m_projectRecovery.repairCheckpointDirectory();

  if (!m_projectRecovery.checkForRecovery()) {
    m_projectRecovery.startProjectSaving();
    return;
  }

  // Recovery file present
  try {
    m_projectRecovery.attemptRecovery();
  } catch (std::exception &e) {
    std::string err{
        "Project Recovery failed to recover this checkpoint. Details: "};
    err.append(e.what());
    g_log.error(err);
    QMessageBox::information(this, "Could Not Recover",
                             "We could not fully recover your work.\nMantid "
                             "will continue to run normally now.",
                             "OK");

    // Restart project recovery manually
    m_projectRecovery.startProjectSaving();
  }
}

void ApplicationWindow::saveRecoveryCheckpoint() {
  m_projectRecovery.saveAll(false);
}
