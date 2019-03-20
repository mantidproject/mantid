// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidUI.h"
#include "AlgorithmDockWidget.h"
#include "AlgorithmMonitor.h"
#include "ImportWorkspaceDlg.h"
#include "MantidMDCurve.h"
#include "MantidMDCurveDialog.h"
#include "MantidMatrix.h"
#include "MantidMatrixCurve.h"
#include "MantidQtWidgets/Common/DropEventHelper.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"
#include "MantidSampleLogDialog.h"
#include "MantidSampleMaterialDialog.h"
#include "MantidSurfaceContourPlotGenerator.h"
#include "MantidTable.h"
#include "ProjectSerialiser.h"

#include "../Folder.h"
#include "../ScriptingWindow.h"
#include "../Spectrogram.h"
#include "../TiledWindow.h"
#include "MantidQtWidgets/Common/pixmaps.h"
#include "ui_SequentialFitDialog.h"

#include "Mantid/InstrumentWidget/InstrumentWindow.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitConversion.h"

#pragma push_macro("slots")
#undef slots
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#pragma pop_macro("slots")

#include "InstrumentWidget/InstrumentWindow.h"

#include "MantidQtWidgets/Common/AlgorithmHistoryWindow.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/PlotAxis.h"
#include "MantidQtWidgets/Common/VatesViewerInterface.h"

#include "MantidQtWidgets/Common/MantidTreeWidget.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/LogFilterGenerator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <QListWidget>
#include <QMdiArea>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextEdit>
#include <QToolBar>

#include <algorithm>
#include <ctime>
#include <qwt_plot_curve.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <fstream>
#include <locale>
#include <set>
#include <sstream>

#include <boost/tokenizer.hpp>

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtWidgets/Factory/WidgetFactory.h"
#include "MantidQtWidgets/SliceViewer/SliceViewerWindow.h"

#include "MantidQtWidgets/SpectrumViewer/SpectrumView.h"
#include <typeinfo>

using namespace std;

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Core::time_duration;
using MantidQt::MantidWidgets::MantidWSIndexDialog;
using MantidQt::SliceViewer::SliceViewerWindow;

namespace MantidException = Mantid::Kernel::Exception;

namespace {
/// The number of detectors to show within a group before eliding
size_t DET_TABLE_NDETS_GROUP = 10;

// Initialize logger
Mantid::Kernel::Logger g_log("MantidUI");

bool isOfType(const QObject *obj, const char *toCompare) {
  return strcmp(obj->metaObject()->className(), toCompare) == 0;
}

/// Number of subplots above which user confirmation will be required
constexpr int REASONABLE_NUM_SUBPLOTS(12);

/// Get graph legend key given workspace name and spectrum number
QString getLegendKey(const QString &wsName, const int spectrum) {
  const auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  if (ws) {
    const auto axis = ws->getAxis(1); // y
    return QString::fromStdString(axis->label(spectrum));
  }
  return QString();
}

/// Get all graph legend keys in one string
QString getLegendKeys(const QString &wsName, const std::set<int> &spectra) {
  QString legendText = wsName + '\n';
  int curveIndex(0);
  for (const auto &spec : spectra) {
    legendText += "\\l(" + QString::number(++curveIndex) + ")" +
                  getLegendKey(wsName, spec) + "\n";
  }
  return legendText;
}

/// Decide whether this graph in a multilayer plot should have an X axis label
bool drawXAxisLabel(const int row, const int col, const int nRows,
                    const int nCols, const int nPlots) {
  if (row == nRows - 1) {
    return true; // last row
  } else if (row == nRows - 2) {
    // Needs a label if there is no subplot below it
    return ((row + 1) * nCols) + col + 1 > nPlots;
  } else {
    return false;
  }
}

/// Spectra names for a fit results workspace
const std::vector<std::string> FIT_RESULTS_SPECTRA_NAMES{"Data", "Calc",
                                                         "Diff"};

/// Decide whether the named workspace is the results from a fit
/// (will have 3 spectra called "Data", "Calc" and "Diff")
bool workspaceIsFitResult(const QString &wsName) {
  bool isFit = false;
  const auto &ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  if (ws) {
    if (FIT_RESULTS_SPECTRA_NAMES.size() == ws->getNumberHistograms()) {
      std::vector<std::string> spectraNames;
      const auto specAxis = ws->getAxis(1); // y
      for (size_t iSpec = 0; iSpec < FIT_RESULTS_SPECTRA_NAMES.size();
           ++iSpec) {
        spectraNames.push_back(specAxis->label(iSpec));
      }
      isFit = spectraNames == FIT_RESULTS_SPECTRA_NAMES;
    }
  }
  return isFit;
}

/// Return curve type for spectrum of a set of fit results
GraphOptions::CurveType getCurveTypeForFitResult(const size_t spectrum) {
  switch (spectrum) {
  case 0:
    return GraphOptions::CurveType::LineSymbols;
  case 1:
    return GraphOptions::CurveType::Line;
  default:
    return GraphOptions::CurveType::Unspecified;
  }
}

std::vector<Mantid::API::MatrixWorkspace_const_sptr>
getWorkspacesFromAds(const QList<QString> &workspaceNames) {
  std::vector<Mantid::API::MatrixWorkspace_const_sptr> workspaces;
  for (auto &workspaceName : workspaceNames) {
    Mantid::API::MatrixWorkspace_const_sptr workspace =
        boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                workspaceName.toStdString()));
    workspaces.push_back(workspace);
  }
  return workspaces;
}
} // namespace

MantidUI::MantidUI(ApplicationWindow *aw)
    : m_finishedLoadDAEObserver(*this,
                                &MantidUI::handleLoadDAEFinishedNotification),
      m_configServiceObserver(*this, &MantidUI::handleConfigServiceUpdate),
      m_appWindow(aw), m_lastShownInstrumentWin(nullptr),
      m_lastShownSliceViewWin(nullptr), m_lastShownSpectrumViewerWin(nullptr),
      m_lastShownColorFillWin(nullptr), m_lastShown1DPlotWin(nullptr),
      m_vatesSubWindow(nullptr)
//, m_spectrumViewWindow(NULL)
{

  // To be able to use them in queued signals they need to be registered
  static bool registered_addtional_types = false;
  if (!registered_addtional_types) {
    registered_addtional_types = true;
    qRegisterMetaType<Mantid::API::Workspace_sptr>();
    qRegisterMetaType<Mantid::API::MatrixWorkspace_sptr>();
    qRegisterMetaType<Mantid::API::MatrixWorkspace_const_sptr>();
    // Register std::string as well as we use it a lot
    qRegisterMetaType<std::string>();
  }

  m_exploreMantid = new WorkspaceTreeWidget(this);
  m_exploreMantid->enableDeletePrompt(
      appWindow()->isDeleteWorkspacePromptEnabled());

  // Add QWorkspaceWidget to a QDockWidget
  m_workspaceDockWidget = new QDockWidget(tr("Workspaces"), aw);
  m_workspaceDockWidget->setObjectName("exploreMantid");
  m_workspaceDockWidget->setMinimumHeight(150);
  m_workspaceDockWidget->setMinimumWidth(200);
  m_workspaceDockWidget->setWidget(m_exploreMantid);
  aw->addDockWidget(Qt::RightDockWidgetArea, m_workspaceDockWidget);

  m_exploreAlgorithms = new AlgorithmDockWidget(this, aw);

  actionCopyRowToTable = new QAction(this);
  actionCopyRowToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyRowToTable, SIGNAL(triggered()), this,
          SLOT(copyRowToTable()));

  actionCopyRowToGraph = new QAction(this);
  actionCopyRowToGraph->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyRowToGraph, SIGNAL(triggered()), this,
          SLOT(copyRowToGraph()));

  actionCopyRowToGraphErr = new QAction(this);
  actionCopyRowToGraphErr->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyRowToGraphErr, SIGNAL(triggered()), this,
          SLOT(copyRowToGraphErr()));

  actionWaterfallPlot = new QAction(QIcon(":/waterfall_plot.png"),
                                    tr("Plot spectra as waterfall"), this);
  connect(actionWaterfallPlot, SIGNAL(triggered()), this,
          SLOT(copyRowsToWaterfall()));

  actionCopyDetectorsToTable = new QAction(tr("View detectors table"), this);
  actionCopyDetectorsToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyDetectorsToTable, SIGNAL(triggered()), this,
          SLOT(copyDetectorsToTable()));

  actionCopyValues = new QAction(tr("Copy"), this);
  actionCopyValues->setIcon(QIcon(getQPixmap("copy_xpm")));
  connect(actionCopyValues, SIGNAL(triggered()), this, SLOT(copyValues()));

  actionCopyColumnToTable = new QAction(this);
  actionCopyColumnToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyColumnToTable, SIGNAL(triggered()), this,
          SLOT(copyColumnToTable()));

  actionCopyColumnToGraph = new QAction(this);
  actionCopyColumnToGraph->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyColumnToGraph, SIGNAL(triggered()), this,
          SLOT(copyColumnToGraph()));

  actionCopyColumnToGraphErr = new QAction(this);
  actionCopyColumnToGraphErr->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyColumnToGraphErr, SIGNAL(triggered()), this,
          SLOT(copyColumnToGraphErr()));

  connect(this, SIGNAL(needToCreateLoadDAEMantidMatrix(const QString &)), this,
          SLOT(createLoadDAEMantidMatrix(const QString &)));
  connect(this, SIGNAL(needToShowCritical(const QString &)), this,
          SLOT(showCritical(const QString &)));

  m_algMonitor = new AlgorithmMonitor(this);
  connect(m_algMonitor, SIGNAL(algorithmStarted(void *)), m_exploreAlgorithms,
          SLOT(algorithmStarted(void *)), Qt::QueuedConnection);
  connect(m_algMonitor, SIGNAL(algorithmFinished(void *)), m_exploreAlgorithms,
          SLOT(algorithmFinished(void *)), Qt::QueuedConnection);
  connect(
      m_algMonitor,
      SIGNAL(needUpdateProgress(void *, double, const QString &, double, int)),
      m_exploreAlgorithms,
      SLOT(updateProgress(void *, double, const QString &, double, int)),
      Qt::QueuedConnection);

  m_algMonitor->start();

  mantidMenu = new QMenu(m_appWindow);
  mantidMenu->setObjectName("mantidMenu");
  mantidMenuAboutToShow();

  menuMantidMatrix = new QMenu(m_appWindow);
  connect(menuMantidMatrix, SIGNAL(aboutToShow()), this,
          SLOT(menuMantidMatrixAboutToShow()));

  connect(m_appWindow, SIGNAL(configModified(void)), this,
          SLOT(configModified(void)));
  init();
}

// Should it be moved to the constructor?
void MantidUI::init() {
  Mantid::Kernel::ConfigService::Instance().addObserver(
      m_configServiceObserver);

  m_exploreAlgorithms->update();

  try {
    m_defaultFitFunction =
        new MantidQt::MantidWidgets::FitPropertyBrowser(m_appWindow, this);
    m_defaultFitFunction->init();
    // this make the progress bar work with Fit algorithm running form the fit
    // browser
    connect(m_defaultFitFunction,
            SIGNAL(executeFit(QString, QHash<QString, QString>,
                              Mantid::API::AlgorithmObserver *)),
            this,
            SLOT(showAlgorithmDialog(QString, QHash<QString, QString>,
                                     Mantid::API::AlgorithmObserver *)));
    m_defaultFitFunction->hide();
    m_appWindow->addDockWidget(Qt::LeftDockWidgetArea, m_defaultFitFunction);

    m_fitFunction = m_defaultFitFunction;
  } catch (...) {
    m_defaultFitFunction = nullptr;
    m_fitFunction = nullptr;
    g_log.warning("Curve fitting plugin not loaded. Some functionality will be "
                  "unavailable.");
  }
}

/// Slot: Receives a new X range from a FitPropertyBrowser and re-emits it.
void MantidUI::x_range_from_picker(double xmin, double xmax) {
  emit x_range_update(xmin, xmax);
}

/// Updates the algorithms tree as this may have changed
void MantidUI::updateAlgorithms() { m_exploreAlgorithms->update(); }

/// Updates the workspace tree
void MantidUI::updateWorkspaces() { m_exploreMantid->refreshWorkspaces(); }

void MantidUI::addMenuItems(QMenu *menu) {
  actionToggleMantid = m_workspaceDockWidget->toggleViewAction();
  actionToggleMantid->setIcon(getQPixmap("mantid_matrix_xpm"));
  actionToggleMantid->setShortcut(tr("Ctrl+Shift+M"));
  menu->addAction(actionToggleMantid);

  actionToggleAlgorithms = m_exploreAlgorithms->toggleViewAction();
  actionToggleAlgorithms->setShortcut(tr("Ctrl+Shift+A"));
  menu->addAction(actionToggleAlgorithms);

  if (m_fitFunction) {
    actionToggleFitFunction = m_fitFunction->toggleViewAction();
    menu->addAction(actionToggleFitFunction);
  }
}

// Show / hide the FitPropertyBrowser
void MantidUI::showFitPropertyBrowser(bool on) {
  if (!m_fitFunction)
    return;
  if (on) {
    m_fitFunction->show();
  } else {
    m_fitFunction->hide();
  }
}

/**
 * Be careful where this is called, if it is a called too late in the Qt
 * shutdown the application crashes
 */
void MantidUI::shutdown() {
  g_log.notice("MantidPlot is shutting down...");

  // First we need to cancel any running algorithms otherwise bad things can
  // happen if they call
  // the logging framework after it's been shutdown. The cancel calls within
  // cancelAll are not
  // blocking, hence the loop to make sure they're all done before moving on.
  // (N.B. Tried copying
  // the wait/exit/wait business from the AlgorithmMonitor dtor, but that gave
  // occasional crashes.)
  if (m_algMonitor) {
    m_algMonitor->cancelAll();
    while (m_algMonitor->count() > 0) {
      Poco::Thread::sleep(100);
    }
  }
  // Close any open algorithm dialogs. They contain alorithm references so
  // should be cleaned up before the framework (and the Python environment)
  // is destroyed. We traverse the object tree rather than tracking the
  // creation as it is possible to create a dialog without going through
  // factory methods.
  const auto &childWidgets = m_appWindow->children();
  for (auto child : childWidgets) {
    if (auto *widget = qobject_cast<MantidQt::API::AlgorithmDialog *>(child)) {
      // We want to delete this now and not defer it to later in the
      // event loop
      widget->setAttribute(Qt::WA_DeleteOnClose, false);
      widget->close();
      delete widget;
      child = nullptr;
    }
  }

  // If any python objects need to be cleared away then the GIL needs to be
  // held.
  GlobalInterpreterLock lock;
  // Relevant notifications are connected to signals that will close all
  // dependent windows
  Mantid::API::FrameworkManager::Instance().shutdown();
}

MantidUI::~MantidUI() {
  delete m_algMonitor;

  Mantid::Kernel::ConfigService::Instance().removeObserver(
      m_configServiceObserver);

  delete m_fitFunction;
}

void MantidUI::saveSettings() const {
  // Save algorithm dialog input
  MantidQt::API::AlgorithmInputHistory::Instance().save();
}

QStringList MantidUI::getWorkspaceNames() {
  QStringList sl;
  auto sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for (const auto &name : sv)
    sl << QString::fromStdString(name);
  return sl;
}

QStringList MantidUI::getAlgorithmNames() {
  QStringList sl;
  std::vector<std::string> algorithmKeys =
      Mantid::API::AlgorithmFactory::Instance().getKeys();
  sl.reserve(static_cast<size_t>(algorithmKeys.size()));
  for (const auto &algorithmKey : algorithmKeys)
    sl << QString::fromStdString(algorithmKey);
  return sl;
}

/**
 *  Returns the number of algorithms currently executing
 */
int MantidUI::runningAlgCount() const { return m_algMonitor->count(); }

/**
 * Alerts applicationWindow that the ADS has been modified.
 */
void MantidUI::updateProject() { m_appWindow->modifiedProject(); }
/**
 * Ticket #678
 */
void MantidUI::saveNexusWorkspace() { executeSaveNexus(); }

/**
* DeleteWorkspace
@param workspaceName :: Name of the workspace to delete
*/
void MantidUI::deleteWorkspace(const QString &workspaceName) {
  auto alg = createAlgorithm("DeleteWorkspace");
  alg->setLogging(false);
  alg->setPropertyValue("Workspace", workspaceName.toStdString());
  executeAlgorithmAsync(alg);
}

void MantidUI::deleteWorkspaces(const QStringList &wsNames) {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(m_appWindow->activeWindow());

  try {
    if (!wsNames.isEmpty()) {
      auto alg = createAlgorithm("DeleteWorkspaces");
      alg->setLogging(false);
      std::vector<std::string> vecWsNames;
      vecWsNames.reserve(wsNames.size());
      foreach (auto wsName, wsNames) {
        vecWsNames.push_back(wsName.toStdString());
      }
      alg->setProperty("WorkspaceList", vecWsNames);
      executeAlgorithmAsync(alg);
    } else if ((m &&
                (strcmp(m->metaObject()->className(), "MantidMatrix") == 0)) &&
               !m->workspaceName().isEmpty()) {
      deleteWorkspace(m->workspaceName());
    }
  } catch (...) {
    QMessageBox::warning(m_appWindow, "",
                         "Could not delete selected workspaces.");
  }
}

/**
getSelectedWorkspaceName
*/
QString MantidUI::getSelectedWorkspaceName() {
  auto names = m_exploreMantid->getSelectedWorkspaceNames();
  QString str;

  if (!names.empty())
    str = QString::fromStdString(names[0]);

  if (str.isEmpty()) {
    // Check if a mantid matrix is selected
    MantidMatrix *m = qobject_cast<MantidMatrix *>(appWindow()->activeWindow());
    if (!m)
      return "";

    str = m->workspaceName();
  }
  return str;
}

Mantid::API::Workspace_const_sptr MantidUI::getSelectedWorkspace() {
  return m_exploreMantid->getSelectedWorkspace();
}

Mantid::API::Workspace_const_sptr
MantidUI::getWorkspace(const QString &workspaceName) {
  if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString())) {
    return AnalysisDataService::Instance().retrieve(
        workspaceName.toStdString());
  }

  Workspace_sptr empty;

  return empty; //??
}

/**   Extension to ApplicationWindow::menuAboutToShow() to deal with Mantid.
 */
bool MantidUI::menuAboutToShow(MdiSubWindow *w) {

  if (w && isOfType(w, "MantidMatrix")) {
    auto plotMenuAction =
        appWindow()->myMenuBar()->addMenu(appWindow()->plot3DMenu);
    plotMenuAction->setText(tr("3D &Plot"));
    appWindow()->actionCopySelection->setEnabled(true);
    appWindow()->actionPasteSelection->setEnabled(false);
    appWindow()->actionClearSelection->setEnabled(false);

    auto menuMantidMatrixAction =
        appWindow()->myMenuBar()->addMenu(menuMantidMatrix);
    menuMantidMatrixAction->setText(tr("&Workspace"));
    return true;
  }

  return false;
}

Graph3D *MantidUI::plot3DMatrix(int style) {
  MdiSubWindow *w = appWindow()->activeWindow();
  if (isOfType(w, "MantidMatrix")) {
    return static_cast<MantidMatrix *>(w)->plotGraph3D(style);
  }

  return nullptr;
}

MultiLayer *MantidUI::plotSpectrogram(GraphOptions::CurveType type) {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (m) {
    return drawSingleColorFillPlot(
        QString::fromStdString(m->getWorkspaceName()), type);
  }
  return nullptr;
}

/**  Import a MatrixWorkspace into a MantidMatrix.
@param wsName :: Workspace name
@param lower :: An optional lower boundary
@param upper :: An optional upper boundary
@param showDlg :: If true show a dialog box to set some import parameters
@param makeVisible :: If true show the created MantidMatrix, hide otherwise.
@return A pointer to the new MantidMatrix.
*/
MantidMatrix *MantidUI::importMatrixWorkspace(const QString &wsName, int lower,
                                              int upper, bool showDlg,
                                              bool makeVisible) {
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString())) {
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        wsName.toStdString());
  }

  MantidMatrix *matrix = importMatrixWorkspace(ws, lower, upper, showDlg);
  if (matrix) {
    appWindow()->addMdiSubWindow(matrix, makeVisible);
  }
  return matrix;
}

/**  Import a MatrixWorkspace into a MantidMatrix.
@param workspace :: Workspace
@param lower :: An optional lower boundary
@param upper :: An optional upper boundary
@param showDlg :: If true show a dialog box to set some import parameters
@return A pointer to the new MantidMatrix.
*/
MantidMatrix *
MantidUI::importMatrixWorkspace(const MatrixWorkspace_sptr workspace, int lower,
                                int upper, bool showDlg) {
  MantidMatrix *matrix = nullptr;
  if (workspace) {
    const QString wsName(workspace->getName().c_str());
    if (showDlg) {
      ImportWorkspaceDlg dlg(appWindow(), workspace->getNumberHistograms());
      if (dlg.exec() == QDialog::Accepted) {
        int start = dlg.getLowerLimit();
        int end = dlg.getUpperLimit();
        matrix = new MantidMatrix(workspace, appWindow(), "Mantid", wsName,
                                  start, end);
        if (dlg.isFiltered())
          matrix->setRange(0, dlg.getMaxValue());
      }
    } else {
      matrix = new MantidMatrix(workspace, appWindow(), "Mantid", wsName, lower,
                                upper);
    }
  }
  return matrix;
}

/**  Import a Workspace into MantidPlot.
@param wsName :: Workspace name
@param showDlg :: If true show a dialog box to set some import parameters
@param makeVisible :: If true show the created widget, hide otherwise.
*/
void MantidUI::importWorkspace(const QString &wsName, bool showDlg,
                               bool makeVisible) {
  MantidMatrix *mm =
      importMatrixWorkspace(wsName, -1, -1, showDlg, makeVisible);
  ScopedOverrideCursor waitCursor;
  if (!mm) {
    importTableWorkspace(wsName, showDlg, makeVisible);
  }
}

/**  Import the selected workspace, if any. Displays the import dialog.
 */
void MantidUI::importWorkspace() {
  QString wsName = getSelectedWorkspaceName();
  importWorkspace(wsName, true, true);
}

/**  Import the selected table workspace transposed.
 */
void MantidUI::importTransposed() {
  ScopedOverrideCursor waitCursor;
  QString wsName = getSelectedWorkspaceName();
  ITableWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString())) {
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        wsName.toStdString());
    importTableWorkspace(wsName, true, true, true);
  }
}

/**  Create a TableWorkspace of box data from the MDEventWorkspace
 */
void MantidUI::importBoxDataTable() {
  std::cout << "MantidUI::importBoxDataTable()\n";
  QString wsName = getSelectedWorkspaceName();
  try {
    // Get the MD event table
    IMDEventWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    if (!ws)
      return;
    ITableWorkspace_sptr tabWs = ws->makeBoxTable(0, 0);
    if (!tabWs)
      return;
    std::string tableName = wsName.toStdString() + std::string("_boxdata");
    AnalysisDataService::Instance().addOrReplace(tableName, tabWs);
    // Now show that table
    importWorkspace(QString::fromStdString(tableName), true, true);
  } catch (...) {
  }
}

/** Plots a Curve showing intensities for a MDWorkspace.
 * But only if the workspace meets certain criteria, such as
 * having only one non-integrated dimension. Should exit gracefully otherwise.
 */
void MantidUI::showMDPlot() {
  QString wsName = getSelectedWorkspaceName();

  // Create a dialog to ask for options
  MantidMDCurveDialog dlg(appWindow(), wsName);
  if (dlg.exec() == QDialog::Rejected)
    return;
  // Extract the settings from the dialog opened earlier
  bool showErrors = dlg.showErrorBars();
  LinePlotOptions *opts = dlg.getLineOptionsWidget();
  QStringList all;
  all << wsName;
  plotMDList(all, opts->getPlotAxis(), opts->getNormalization(), showErrors);
}

/**
 * Plots a curve showing intensities for MDWorkspaces
 * @param wsNames : Names of the workspaces to plot
 * @param plotAxis : Axis number to plot
 * @param normalization: Normalization option to use
 * @param showErrors: True if errors are to be show
 * @param plotWindow :: Window to use for plotting. If NULL a new one will be
 * created
 * @param clearWindow :: Whether to clean the plotWindow before plotting.Ignored
 * if plotWindow == NULL
 * @return NULL if failure. Otherwise, if plotWindow == NULL - created window,
 * if not NULL - plotWindow
 */
MultiLayer *
MantidUI::plotMDList(const QStringList &wsNames, const int plotAxis,
                     const Mantid::API::MDNormalization normalization,
                     const bool showErrors, MultiLayer *plotWindow,
                     bool clearWindow) {
  ScopedOverrideCursor waitCursor;
  const auto &firstName = wsNames.at(0);

  bool isGraphNew = false;
  MultiLayer *ml = appWindow()->prepareMultiLayer(isGraphNew, plotWindow,
                                                  firstName, clearWindow);

  Graph *g = ml->activeGraph();
  try {
    for (int i = 0; i < wsNames.size(); ++i) {
      // Create the curve with defaults
      const auto &wsName = wsNames.at(i);
      MantidMDCurve *curve = new MantidMDCurve(wsName, g, showErrors);
      MantidQwtIMDWorkspaceData *data = curve->mantidData();

      // Apply the settings
      data->setPreviewMode(false);
      data->setPlotAxisChoice(plotAxis);
      data->setNormalization(normalization);

      g->setNormalizableMD(true);
      g->setNormalizationMD(normalization);

      // Using information from the first graph
      if (i == 0 && isGraphNew)
        g->setAutoScale();
    }

  } catch (std::invalid_argument &e) {
    g_log.warning() << e.what() << '\n';
  } catch (std::runtime_error &e) {
    g_log.warning() << e.what() << '\n';
  } catch (...) {
  }

  if (!isGraphNew)
    // Replot graph is we've added curves to existing one
    g->replot();

  // Check if window does not contain any curves and should be closed
  ml->maybeNeedToClose();

  return ml;
}

/*
Generates a table workspace from a md workspace and pulls up
a grid to display the results.
*/
void MantidUI::showListData() {
  QString wsName = getSelectedWorkspaceName();
  QString tableWsName = wsName + "_data_list_table";

  Mantid::API::IAlgorithm_sptr queryWorkspace =
      this->createAlgorithm("QueryMDWorkspace");
  queryWorkspace->initialize();
  queryWorkspace->setPropertyValue("InputWorkspace", wsName.toStdString());
  std::string sTableWorkspaceName = tableWsName.toStdString();
  queryWorkspace->setPropertyValue("OutputWorkspace", sTableWorkspaceName);
  queryWorkspace->setProperty("LimitRows", false);
  queryWorkspace->execute();

  importWorkspace(tableWsName);
}

void MantidUI::showVatesSimpleInterface() {
  QString wsName = getSelectedWorkspaceName();
  try {
    IMDEventWorkspace_sptr mdews =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve(wsName.toStdString()));

    IPeaksWorkspace_sptr pws = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(wsName.toStdString()));

    IMDHistoWorkspace_sptr mdhist =
        boost::dynamic_pointer_cast<IMDHistoWorkspace>(
            AnalysisDataService::Instance().retrieve(wsName.toStdString()));

    if (!mdews && !pws && !mdhist) {
      return;
    }

    // Set the type of workspace, the GUI needs it and
    // extract the instrument which was used to measure the workspace data
    int wsType = MantidQt::API::VatesViewerInterface::MDEW;

    std::string instrumentName;

    // check for peak workspace
    if (pws) {
      wsType = MantidQt::API::VatesViewerInterface::PEAKS;

      instrumentName = pws->getInstrument()->getFullName();
    }

    // Check for histo workspace
    if (mdhist) {
      wsType = MantidQt::API::VatesViewerInterface::MDHW;

      // Get the instrument name
      if (mdhist->getNumExperimentInfo() > 0) {
        instrumentName =
            mdhist->getExperimentInfo(0)->getInstrument()->getFullName();
      }
    }

    // Check for event workspace
    if (mdews) {
      // Get the instrument name
      if (mdews->getNumExperimentInfo() > 0) {
        instrumentName =
            mdews->getExperimentInfo(0)->getInstrument()->getFullName();
      }
    }

    if (m_vatesSubWindow) {
      QWidget *vwidget = m_vatesSubWindow->widget();
      vwidget->show();
      qobject_cast<MantidQt::API::VatesViewerInterface *>(vwidget)
          ->renderWorkspace(wsName, wsType, instrumentName);
      return;
    } else {
      m_vatesSubWindow = new QMdiSubWindow;
      m_vatesSubWindow->setAttribute(Qt::WA_DeleteOnClose, false);
#ifdef Q_OS_MAC
      // Work around to ensure that floating windows remain on top of the main
      // application window, but below other applications on Mac
      // Note: Qt::Tool cannot have both a max and min button on OSX
      Qt::WindowFlags flags = m_vatesSubWindow->windowFlags();
      flags |= Qt::Tool;
      flags |= Qt::CustomizeWindowHint;
      flags |= Qt::WindowMinimizeButtonHint;
      flags |= Qt::WindowCloseButtonHint;
      m_vatesSubWindow->setWindowFlags(flags);
#endif
      QIcon icon;
      icon.addFile(
          QString::fromUtf8(":/VatesSimpleGuiViewWidgets/icons/pvIcon.png"),
          QSize(), QIcon::Normal, QIcon::Off);
      m_vatesSubWindow->setWindowIcon(icon);
      connect(m_appWindow, SIGNAL(shutting_down()), m_vatesSubWindow,
              SLOT(close()));

      MantidQt::API::InterfaceManager interfaceManager;
      MantidQt::API::VatesViewerInterface *vsui =
          interfaceManager.createVatesSimpleGui();
      if (vsui) {
        connect(m_appWindow, SIGNAL(shutting_down()), vsui, SLOT(shutdown()));
        connect(vsui, SIGNAL(requestClose()), m_vatesSubWindow, SLOT(close()));
        vsui->setParent(m_vatesSubWindow);
        m_vatesSubWindow->setWindowTitle("Vates Simple Interface");
        vsui->setupPluginMode(wsType, instrumentName);
        m_vatesSubWindow->setWidget(vsui);
        m_vatesSubWindow->widget()->show();
        vsui->renderWorkspace(wsName, wsType, instrumentName);
        // Keep and handle to the window for later serialisation
        appWindow()->addSerialisableWindow(vsui);
        appWindow()->modifiedProject();
      } else {
        delete m_vatesSubWindow;
        m_vatesSubWindow = nullptr;
        return;
      }
    }
  } catch (std::runtime_error &e) {
    throw std::runtime_error(e);
  } catch (...) {
  }
  // reset the qt error redirection that Paraview puts in place
  // this may not be necessary if we move to qt5
  qInstallMsgHandler(nullptr);
}

void MantidUI::showSpectrumViewer() {
  QString wsName = getSelectedWorkspaceName();
  try {
    MatrixWorkspace_sptr wksp = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    if (wksp) {
      MantidQt::SpectrumView::SpectrumView *viewer;
      try {
        viewer = new MantidQt::SpectrumView::SpectrumView(m_appWindow);
      } catch (std::runtime_error &e) {
        m_lastShownSpectrumViewerWin = nullptr;
        g_log.error() << "Could not create spectrum viewer: " << e.what()
                      << "\n";
        throw std::runtime_error(e);
      }
      // Delete on close so we don't hold a shared pointer to a workspace
      // which has been deleted in the ADS and is "inaccessible"
      viewer->setAttribute(Qt::WA_DeleteOnClose, true);
      viewer->resize(1050, 800);
      connect(m_appWindow, SIGNAL(shutting_down()), viewer, SLOT(close()));

      if (workspacesDockPlot1To1()) {
        // only one at any given time
        if (m_lastShownSpectrumViewerWin) {
          m_lastShownSpectrumViewerWin->close();
          QPoint p = m_lastShownSpectrumViewerWin->pos();
          delete m_lastShownSpectrumViewerWin;
          viewer->move(p);
        }
      }
      m_lastShownSpectrumViewerWin = viewer;

      viewer->show();
      viewer->renderWorkspace(wksp);
      // Add to the list of serialisable windows
      appWindow()->addSerialisableWindow(viewer);
      appWindow()->modifiedProject();
    } else {
      g_log.information()
          << "Only event or matrix workspaces are currently supported.\n"
          << "Please convert to one of these before using the ImageView.\n";
    }
  } catch (std::runtime_error &e) {
    g_log.error() << e.what() << "\n";
    throw std::runtime_error(e);
  } catch (...) {
    g_log.error() << "Image View: Exception getting workspace\n";
  }
}

/** Create a window with a SliceViewer widget to show
 * the selected workspace
 */
void MantidUI::showSliceViewer() {
  // Retrieve the MDWorkspace
  QString wsName = getSelectedWorkspaceName();
  IMDWorkspace_sptr mdws = boost::dynamic_pointer_cast<IMDWorkspace>(
      AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  MatrixWorkspace_sptr mw = boost::dynamic_pointer_cast<MatrixWorkspace>(mdws);
  if (mdws) {
    // Create the slice viewer window
    SliceViewerWindow *w;
    try {
      w = MantidQt::Factory::WidgetFactory::Instance()->createSliceViewerWindow(
          wsName, "");
    } catch (std::runtime_error &e) {
      m_lastShownSliceViewWin = nullptr;
      g_log.error() << "Could not create slice viewer: " << e.what() << "\n";
      throw std::runtime_error(e);
    }

    // Special options for viewing MatrixWorkspaces
    if (mw) {
      w->getSlicer()->setTransparentZeros(false);
    }

    // Global option for color bar autoscaling
    w->getSlicer()->setColorBarAutoScale(m_appWindow->autoscale2DPlots);

    // Connect the MantidPlot close() event with the the window's close().
    QObject::connect(appWindow(), SIGNAL(destroyed()), w, SLOT(close()));

    if (workspacesDockPlot1To1()) {
      // only one at any given time
      if (m_lastShownSliceViewWin) {
        m_lastShownSliceViewWin->close();
        QPoint p = m_lastShownSliceViewWin->pos();
        // the factory keeps a list of all opened slice viewers
        MantidQt::Factory::WidgetFactory::Instance()->closeSliceViewerWindow(
            m_lastShownSliceViewWin);
        delete m_lastShownSliceViewWin;
        w->move(p);
      }
    }
    m_lastShownSliceViewWin = w;

    // Pop up the window
    w->show();
    // Keep and handle to the window for later serialisation
    appWindow()->addSerialisableWindow(w);
    appWindow()->modifiedProject();
  }
}

/** #539: For adding Workspace History display to MantidPlot
Show Algorithm History Details in a window .
*/
void MantidUI::showAlgorithmHistory() {
  QString wsName = getSelectedWorkspaceName();
  Mantid::API::Workspace_const_sptr wsptr = getWorkspace(wsName);
  if (nullptr != wsptr) {
    // If the workspace has any AlgorithHistory ...
    if (!wsptr->getHistory().empty()) {
      // ... create and display the window.
      AlgorithmHistoryWindow *palgHist =
          new AlgorithmHistoryWindow(m_appWindow, wsptr);
      if (nullptr != palgHist) {
        palgHist->show();
      }
    }
  } else {
    QMessageBox::information(appWindow(), "Mantid", "Invalid WorkSpace");
    return;
  }
}

/**  Create a new Table and fill it with the data from a Tableworkspace
@param wsName :: Workspace name
@param showDlg :: If true show a dialog box to set some import parameters
@param makeVisible :: If true show the created Table, hide otherwise.
@param transpose :: Transpose the table
@return A pointer to the new Table.
*/
Table *MantidUI::importTableWorkspace(const QString &wsName, bool,
                                      bool makeVisible, bool transpose) {
  ITableWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString())) {
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        wsName.toStdString());
  }

  if (!ws.get())
    return nullptr;

  if (ws->rowCount() == 0 || ws->columnCount() == 0) {
    showCritical("Cannot create an empty table");
    return nullptr;
  }

  Table *t = new MantidTable(appWindow()->scriptingEnv(), ws, wsName,
                             appWindow(), transpose);
  if (makeVisible)
    t->showNormal();
  else
    t->showMinimized();
  return t;
}

void MantidUI::showContextMenu(QMenu &cm, MdiSubWindow *w) {
  if (MantidMatrix *mm = dynamic_cast<MantidMatrix *>(w)) {

    bool areSpectraSelected = mm->setSelectedRows();
    bool areColumnsSelected = mm->setSelectedColumns();
    cm.addAction(actionCopyValues);
    if (areSpectraSelected)
      cm.addAction(actionCopyRowToTable);
    if (areColumnsSelected)
      cm.addAction(actionCopyColumnToTable);
    cm.addSeparator();
    cm.addAction(actionCopyDetectorsToTable);
    cm.addSeparator();

    if (areSpectraSelected && mm->numCols() > 1) {
      // Enable the appropriate options
      cm.addAction(actionCopyRowToGraph);
      cm.addAction(actionCopyRowToGraphErr);
      if (mm->getSelectedRows().size() > 1) {
        cm.addAction(actionWaterfallPlot);
      }
    }
    if (areColumnsSelected && mm->numRows() > 1) {
      cm.addAction(actionCopyColumnToGraph);
      cm.addAction(actionCopyColumnToGraphErr);
    }

    // Set the option texts to the correct plurality
    if (mm->getSelectedRows().size() > 1) {
      actionCopyRowToTable->setText("Copy spectra to table");
      actionCopyRowToGraph->setText("Plot spectra (values only)");
      actionCopyRowToGraphErr->setText("Plot spectra (values + errors)");
    } else {
      actionCopyRowToTable->setText("Copy spectrum to table");
      actionCopyRowToGraph->setText("Plot spectrum (values only)");
      actionCopyRowToGraphErr->setText("Plot spectrum (values + errors)");
    }
    if (mm->getSelectedColumns().size() > 1) {
      actionCopyColumnToTable->setText("Copy bins to table");
      actionCopyColumnToGraph->setText("Plot bins (values only)");
      actionCopyColumnToGraphErr->setText("Plot bins (values + errors)");
    } else {
      actionCopyColumnToTable->setText("Copy bin to table");
      actionCopyColumnToGraph->setText("Plot bin (values only)");
      actionCopyColumnToGraphErr->setText("Plot bin (values + errors)");
    }
  }
}

void MantidUI::copyRowToTable() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  Table *t = createTableFromSelectedRows(m, true, true);
  t->showNormal();
}

void MantidUI::copyColumnToTable() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  Table *t = createTableFromSelectedColumns(m, true);
  t->showNormal();
}

void MantidUI::copyRowToGraph() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  plotSelectedRows(m, MantidQt::DistributionDefault, false);
}

void MantidUI::copyColumnToGraph() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  plotSelectedColumns(m, false);
}

void MantidUI::copyColumnToGraphErr() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  plotSelectedColumns(m, true);
}

void MantidUI::copyRowToGraphErr() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  plotSelectedRows(m, MantidQt::DistributionDefault, true);
}

void MantidUI::copyRowsToWaterfall() {
  const MantidMatrix *const m =
      dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  MultiLayer *ml = plotSelectedRows(m, MantidQt::DistributionDefault, false);
  if (ml)
    convertToWaterfall(ml);
}

void MantidUI::plotWholeAsWaterfall() {
  const MantidMatrix *const m =
      dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  MultiLayer *ml = plotSpectraRange(m->workspaceName(), 0, m->numRows() - 1,
                                    MantidQt::DistributionDefault, false);
  if (ml)
    convertToWaterfall(ml);
}

void MantidUI::convertToWaterfall(MultiLayer *ml) {
  ml->hide();
  ml->activeGraph()->setWaterfallOffset(10, 20);
  ml->setWaterfallLayout();
  // Next two lines replace the legend so that it works on reversing the curve
  // order
  ml->activeGraph()->removeLegend();
  ml->activeGraph()->newLegend();
  ml->show();
}

void MantidUI::copyDetectorsToTable() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  createTableDetectors(m);
}

void MantidUI::copyValues() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  m->copySelection();
}

Table *MantidUI::createTableDetectors(MantidMatrix *m) {
  std::vector<int> indices(m->numRows(), 0);
  for (int i = 0; i < m->numRows(); i++) {
    indices[i] = m->workspaceIndex(i);
  }
  Table *t = createDetectorTable(m->workspaceName(), indices);
  return t;
}

/**
 * Show the detector table - this method is here for the Python interface
 */
Table *MantidUI::createDetectorTable(const QString &wsName) {
  std::vector<int> indices;
  Table *t = createDetectorTable(wsName, indices);
  return t;
}

/**
 * Create the relevant detector table for the given workspace
 * @param wsName :: The name of the workspace
 * @param indices :: Limit the table to these workspace indices (MatrixWorkspace
 * only). If the vector is empty,
 * all the indices are used.
 * @param include_data :: If true then first value from the each spectrum is
 * displayed (MatrixWorkspace only)
 */
Table *MantidUI::createDetectorTable(const QString &wsName,
                                     const std::vector<int> &indices,
                                     bool include_data) {
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString())) {
    auto ws = AnalysisDataService::Instance().retrieve(wsName.toStdString());
    // Standard MatrixWorkspace
    auto matrix = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrix) {
      return createDetectorTable(wsName, matrix, indices, include_data);
    }
    auto peaks = boost::dynamic_pointer_cast<IPeaksWorkspace>(ws);
    if (peaks) {
      return createDetectorTable(wsName, peaks);
    }
  }
  return nullptr;
}

/**
 * Create the instrument detector table from a MatrixWorkspace
 * @param wsName :: The name of the workspace
 * @param ws :: A pointer to a MatrixWorkspace
 * @param indices :: Limit the table to these workspace indices
 * @param include_data :: If true then first value from the each spectrum is
 * displayed
 */
Table *MantidUI::createDetectorTable(
    const QString &wsName, const Mantid::API::MatrixWorkspace_sptr &ws,
    const std::vector<int> &indices, bool include_data) {
  using namespace Mantid::Geometry;

  IComponent_const_sptr sample = ws->getInstrument()->getSample();
  if (!sample) {
    return nullptr;
  }

  // check if efixed value is available
  bool calcQ(true);

  // check if we have a scanning workspace
  bool isScanning = ws->detectorInfo().isScanning();

  const auto &spectrumInfo = ws->spectrumInfo();
  if (spectrumInfo.hasDetectors(0)) {
    try {
      boost::shared_ptr<const IDetector> detector(&spectrumInfo.detector(0),
                                                  Mantid::NoDeleting());
      ws->getEFixed(detector);
    } catch (std::runtime_error &) {
      calcQ = false;
    }
  } else {
    // No detectors available
    calcQ = false;
  }

  // Prepare column names. Types will be determined from QVariant
  QStringList colNames;
  colNames << "Index"
           << "Spectrum No"
           << "Detector ID(s)";
  if (isScanning)
    colNames << "Time Indexes";
  if (include_data) {
    colNames << "Data Value"
             << "Data Error";
  }

  colNames << "R"
           << "Theta";
  if (calcQ) {
    colNames << "Q";
  }
  colNames << "Phi"
           << "Monitor";

  const int ncols = static_cast<int>(colNames.size());
  const int nrows = indices.empty()
                        ? static_cast<int>(ws->getNumberHistograms())
                        : static_cast<int>(indices.size());
  Table *t = new Table(appWindow()->scriptingEnv(), nrows, ncols, "",
                       appWindow(), nullptr);
  appWindow()->initTable(
      t, appWindow()->generateUniqueName(wsName + "-Detectors-"));
  // Set the column names
  for (int col = 0; col < ncols; ++col) {
    t->setColName(col, colNames[col]);
    t->setColPlotDesignation(col, Table::None);
  }
  t->setHeaderColType();
  t->setTextFormat(2);
  if (isScanning)
    t->setTextFormat(3);
  t->setTextFormat(ncols - 1);

  // Cache some frequently used values
  const auto beamAxisIndex =
      ws->getInstrument()->getReferenceFrame()->pointingAlongBeam();
  const auto sampleDist = sample->getPos()[beamAxisIndex];
  bool signedThetaParamRetrieved(false),
      showSignedTwoTheta(false); // If true,  signedVersion of the two theta
                                 // value should be displayed
  QVector<QList<QVariant>> tableColValues;
  tableColValues.resize(nrows);
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*ws))
  for (int row = 0; row < nrows; ++row) {
    // Note PARALLEL_START_INTERUPT_REGION & friends apparently not needed (like
    // in algorithms)
    // as there's an extensive try...catch below. If it was need, using those
    // macros would
    // require data members and methods that are available in algorithm classed
    // but not here,
    // including m_cancel, m_parallelException, interrrupt_point().
    QList<QVariant> &colValues = tableColValues[row];
    size_t wsIndex = indices.empty() ? static_cast<size_t>(row) : indices[row];
    colValues << QVariant(static_cast<double>(wsIndex));
    const double dataY0(ws->y(wsIndex)[0]), dataE0(ws->e(wsIndex)[0]);
    try {
      auto &spectrum = ws->getSpectrum(wsIndex);
      Mantid::specnum_t specNo = spectrum.getSpectrumNo();
      const auto &ids =
          dynamic_cast<const std::set<int> &>(spectrum.getDetectorIDs());
      QString detIds = createTruncatedList(ids);

      // Geometry
      if (!spectrumInfo.hasDetectors(wsIndex))
        throw std::runtime_error("No detectors found.");
      if (!signedThetaParamRetrieved) {
        const std::vector<std::string> &parameters =
            spectrumInfo.detector(wsIndex).getStringParameter(
                "show-signed-theta", true); // recursive
        showSignedTwoTheta =
            (!parameters.empty() && find(parameters.begin(), parameters.end(),
                                         "Always") != parameters.end());
        signedThetaParamRetrieved = true;
      }

      double R(0.0), theta(0.0), phi(0.0);
      // theta used as a dummy variable
      // Note: phi is the angle around Z, not necessarily the beam direction.
      spectrumInfo.position(wsIndex).getSpherical(R, theta, phi);
      // R is actually L2 (same as R if sample is at (0,0,0)), except for
      // monitors which are handled below.
      R = spectrumInfo.l2(wsIndex);
      // Theta is actually 'twoTheta' for detectors (twice the scattering
      // angle), if Z is the beam direction this corresponds to theta in
      // spherical coordinates.
      // For monitors we follow historic behaviour and display theta
      const bool isMonitor = spectrumInfo.isMonitor(wsIndex);
      if (!isMonitor) {
        try {
          theta = showSignedTwoTheta ? spectrumInfo.signedTwoTheta(wsIndex)
                                     : spectrumInfo.twoTheta(wsIndex);
          theta *= 180.0 / M_PI; // To degrees
        } catch (const std::exception &ex) {
          // Log the error and leave theta as it is
          g_log.error(ex.what());
        }
      } else {
        const auto dist = spectrumInfo.position(wsIndex)[beamAxisIndex];
        theta = sampleDist > dist ? 180.0 : 0.0;
      }
      const QString isMonitorDisplay = isMonitor ? "yes" : "no";
      colValues << QVariant(specNo) << QVariant(detIds);
      if (isScanning) {
        std::set<int> timeIndexSet;
        for (auto def : spectrumInfo.spectrumDefinition(wsIndex)) {
          timeIndexSet.insert(int(def.second));
        }

        QString timeIndexes = createTruncatedList(timeIndexSet);
        colValues << QVariant(timeIndexes);
      }
      // Y/E
      if (include_data) {
        colValues << QVariant(dataY0) << QVariant(dataE0); // data
      }
      // If monitors are before the sample in the beam, DetectorInfo
      // returns a negative l2 distance.
      if (isMonitor) {
        R = std::abs(R);
      }
      colValues << QVariant(R) << QVariant(theta);

      if (calcQ) {
        if (isMonitor) {
          // twoTheta is not defined for monitors.
          colValues << QVariant(std::nan(""));
        } else {
          try {

            // Get unsigned theta and efixed value
            IDetector_const_sptr det(&spectrumInfo.detector(wsIndex),
                                     Mantid::NoDeleting());
            double efixed = ws->getEFixed(det);
            double usignTheta = spectrumInfo.twoTheta(wsIndex) * 0.5;

            double q = Mantid::Kernel::UnitConversion::convertToElasticQ(
                usignTheta, efixed);
            colValues << QVariant(q);
          } catch (std::runtime_error &) {
            colValues << QVariant("No Efixed");
          }
        }
      }

      colValues << QVariant(phi)               // rtp
                << QVariant(isMonitorDisplay); // monitor
    } catch (...) {
      // spectrumNo=-1, detID=0
      colValues << QVariant(-1) << QVariant("0");
      // Y/E
      if (include_data) {
        colValues << QVariant(dataY0) << QVariant(dataE0); // data
      }
      colValues << QVariant("0") << QVariant("0") // rt
                << QVariant("0")                  // efixed
                << QVariant("0")                  // rtp
                << QVariant("n/a");               // monitor
    }                                             // End catch for no spectrum
  }

  // This modifies widgets, so it needs to run in the Qt GUI thread: no openmp
  // here
  for (int row = 0; row < nrows; ++row) {
    const QList<QVariant> &colValues = tableColValues[row];
    for (int col = 0; col < ncols; ++col) {
      const QVariant &colValue = colValues[col];
      if (QMetaType::QString == colValue.userType()) // Avoid a compiler warning
                                                     // with type() about
                                                     // comparing different
                                                     // enums...
      {
        t->setText(row, col, colValue.toString());
      } else {
        t->setCell(row, col, colValue.toDouble());
      }
    }
  }

  // want all the detector tables as read-only
  t->setReadOnlyAllColumns(true);
  t->showNormal();

  return t;
}

/**
 * Creates a table showing the detectors contributing to the peaks within a
 * PeaksWorkspace
 * @param wsName :: The name of the workspace
 * @param ws :: A pointer to an IPeaksWorkspace object
 */
Table *
MantidUI::createDetectorTable(const QString &wsName,
                              const Mantid::API::IPeaksWorkspace_sptr &ws) {
  // Import the peaks table too for reference
  bool dialog(false), visible(true);
  importTableWorkspace(wsName, dialog, visible);

  auto idtable = ws->createDetectorTable();
  bool transpose = false;
  QString tableName = wsName + "-Detectors";
  Table *t = new MantidTable(appWindow()->scriptingEnv(), idtable, tableName,
                             appWindow(), transpose);
  if (!t)
    return nullptr;
  // want all the detector tables as read-only
  t->setReadOnlyAllColumns(true);
  t->showNormal();
  return t;
}

QString MantidUI::createTruncatedList(const std::set<int> &elements) {
  QString qString("");
  size_t ndets = elements.size();
  auto iter = elements.begin();
  auto itEnd = elements.end();
  if (ndets > DET_TABLE_NDETS_GROUP) {
    qString = QString("%1,%2...(%3 more)...%4,%5");
    // post-fix increments and returns last value
    // NOTE: Doing this detIds.arg(*iter++).arg(*iter++).arg(ndets-4) seems
    // to result
    // in an undefined order in which the iterator is dereference and
    // incremented leading
    // to the first two items being backward on some systems
    const Mantid::detid_t first(*iter++), second(*iter++);
    qString =
        qString.arg(first).arg(second).arg(ndets - 4); // First two + n extra
    auto revIter = elements.rbegin(); // Set iterators are unidirectional ... so
                                      // no operator-()
    const Mantid::detid_t last(*revIter++), lastm1(*revIter++);
    qString = qString.arg(lastm1).arg(last);
  } else {
    for (; iter != itEnd; ++iter) {
      qString += QString::number(*iter) + ",";
    }
    qString.chop(1); // Drop last comma
  }

  return qString;
}

/**
 * Triggered by a delete key press, and attempts to delete a workspace if it
 * passes the focus checks
 */
void MantidUI::deletePressEvent() {
  m_exploreMantid->onClickDeleteWorkspaces();
}

/**
 * Check if drop event can be accepted
 */
bool MantidUI::canAcceptDrop(QDragEnterEvent *e) {
  QString name = e->mimeData()->objectName();

  return (name == "MantidWorkspace" || e->mimeData()->hasUrls() ||
          name == "TiledWindow");
}

bool MantidUI::drop(QDropEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    QStringList wsNames = e->mimeData()->text().split("\n");
    for (const auto &wsName : wsNames) {
      importWorkspace(wsName, false);
    }
    return true;
  } else if (e->mimeData()->hasUrls()) {
    const auto pyFiles = DropEventHelper::extractPythonFiles(e);
    if (!pyFiles.empty()) {
      try {
        MantidQt::API::ProjectSerialiser serialiser(m_appWindow);
        serialiser.openScriptWindow(pyFiles);
      } catch (std::runtime_error &error) {
        g_log.error()
            << "Failed to Load the python files. The reason for failure is: "
            << error.what() << '\n';
      } catch (std::logic_error &error) {
        g_log.error()
            << "Failed to Load the python files. The reason for failure is: "
            << error.what() << '\n';
      }
    } else {
      // pass to Loading of mantid workspaces
      m_exploreMantid->dropEvent(e);
    }
    return true;
  } else if (name == "TiledWindow") {
    MdiSubWindow *w =
        m_appWindow->currentFolder()->window(e->mimeData()->text());
    if (!w)
      return false;
    TiledWindow *tw = dynamic_cast<TiledWindow *>(w);
    if (!tw)
      return false;
    tw->removeSelectionToDefaultWindowType();

    return true;
  }

  return false;
}

/**
Executes the Save Nexus dialogue from the right click context menu.

The Save > Nexus function from the button in the Dock (with Load, Delete, Group,
Sort, Save buttons) is in MantidDock in function handleShowSaveAlgorithm()

saveNexus Input Dialog is a generic dialog.Below code is added to remove
the workspaces except the selected workspace from the InputWorkspace combo

*/
void MantidUI::executeSaveNexus() {
  QString wsName = getSelectedWorkspaceName();
  QHash<QString, QString> presets;
  if (!wsName.isEmpty()) {
    presets["InputWorkspace"] = wsName;
  }
  showAlgorithmDialog("SaveNexus", presets);
}

//-----------------------------------------------------------------------------
/** Open an algorithm dialog to execute the named algorithm.
 *
 * @param algName :: name of the algorithm
 * @param version :: version number, -1 for latest
 * @return true if successful.
 */
void MantidUI::showAlgorithmDialog(const QString &algName, int version) {
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
  if (!alg)
    return;
  MantidQt::API::AlgorithmDialog *dlg = createAlgorithmDialog(alg);

  if (algName == "Load") {
    // when loading files, we'll need to update the list of recent files
    // hook up MantidUI::fileDialogAccept() to the LoadDialog dialog accepted()
    // signal
    connect(dlg, SIGNAL(accepted()), this, SLOT(loadFileDialogAccept()));
  }

  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * Execute an algorithm. Show the algorithm dialog before executing. The
 * property widgets will be preset with values in paramList.
 * @param algName :: The algorithm name
 * @param paramList :: A list of algorithm properties to be passed to
 * Algorithm::setProperties
 * @param obs :: A pointer to an instance of AlgorithmObserver which will be
 * attached to the finish notification
 * @param version :: version number, -1 for latest
 */
void MantidUI::showAlgorithmDialog(const QString &algName,
                                   QHash<QString, QString> paramList,
                                   Mantid::API::AlgorithmObserver *obs,
                                   int version) {
  // Get latest version of the algorithm
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
  if (!alg)
    return;

  for (QHash<QString, QString>::Iterator it = paramList.begin();
       it != paramList.end(); ++it) {
    alg->setPropertyValue(it.key().toStdString(), it.value().toStdString());
  }
  MantidQt::API::AlgorithmDialog *dlg = createAlgorithmDialog(alg);

  if (algName == "Load") {
    // when loading files, we'll need to update the list of recent files
    // hook up MantidUI::fileDialogAccept() to the LoadDialog dialog accepted()
    // signal
    connect(dlg, SIGNAL(accepted()), this, SLOT(loadFileDialogAccept()));
  }

  if (obs) {
    dlg->addAlgorithmObserver(obs);
  }

  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * Slot for executing an algorithm.
 * @param alg :: Shared pointer to an algorithm to execute with all properties
 * already set.
 */
void MantidUI::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  executeAlgorithmAsync(alg);
}

/**
 * This creates an algorithm dialog (the default property entry thingie).
 */
MantidQt::API::AlgorithmDialog *
MantidUI::createAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg) {
  QHash<QString, QString> presets;
  QStringList enabled;

  // If a property was explicitly set show it as preset in the dialog
  const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
  std::vector<Mantid::Kernel::Property *>::const_iterator p = props.begin();
  for (; p != props.end(); ++p) {
    if (!(**p).isDefault()) {
      QString property_name = QString::fromStdString((**p).name());
      presets.insert(property_name, QString::fromStdString((**p).value()));
      enabled.append(property_name);
    }
  }

  // If a workspace is selected in the dock then set this as a preset for the
  // dialog
  QString selected = getSelectedWorkspaceName();
  if (!selected.isEmpty()) {
    QString property_name = findInputWorkspaceProperty(alg);
    if (!presets.contains(property_name)) {
      presets.insert(property_name, selected);
      // Keep it enabled
      enabled.append(property_name);
    }
  }

  // Check if a workspace is selected in the dock and set this as a preference
  // for the input workspace
  // This is an optional message displayed at the top of the GUI.
  QString optional_msg(alg->summary().c_str());

  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
      alg, m_appWindow, false, presets, optional_msg, enabled);
  return dlg;
}

/**
 * Find the first input workspace for an algorithm
 * @param algorithm :: A pointer to the algorithm instance
 */
QString MantidUI::findInputWorkspaceProperty(
    Mantid::API::IAlgorithm_sptr algorithm) const {
  // Iterate through the properties and find the first input one
  std::vector<Mantid::Kernel::Property *> props = algorithm->getProperties();
  std::vector<Mantid::Kernel::Property *>::const_iterator pend = props.end();
  for (std::vector<Mantid::Kernel::Property *>::const_iterator pitr =
           props.begin();
       pitr != pend; ++pitr) {
    Mantid::Kernel::Property *base_prop = *pitr;
    const Mantid::API::IWorkspaceProperty *ws_prop =
        dynamic_cast<Mantid::API::IWorkspaceProperty *>(base_prop);
    if (ws_prop) {
      unsigned int direction = base_prop->direction();
      if (direction == Mantid::Kernel::Direction::Input ||
          direction == Mantid::Kernel::Direction::InOut) {
        return QString::fromStdString(base_prop->name());
      }
    }
  }
  return QString();
}

void MantidUI::copyWorkspacestoVector(
    const QList<QTreeWidgetItem *> &selectedItems,
    std::vector<std::string> &inputWSVec) {
  // iterate through each of the selected workspaces
  QList<QTreeWidgetItem *>::const_iterator itr;
  for (itr = selectedItems.begin(); itr != selectedItems.end(); ++itr) {
    std::string inputWSName = (*itr)->text(0).toStdString();
    inputWSVec.push_back(inputWSName);
  } // end of for loop for input workspaces
}

/**
 * Determine if the workspace has one or more UB matrixes on one of it's
 * samples.
 * @param wsName
 * @return True if present
 */
bool MantidUI::hasUB(const QString &wsName) {
  const std::string algName("HasUB");
  Mantid::API::IAlgorithm_sptr alg;
  try {

    alg = Mantid::API::AlgorithmManager::Instance().create(algName);
  } catch (...) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          "Cannot create algorithm " +
                              QString::fromStdString(algName));
    return false;
  }
  if (!alg) {
    return false;
  }

  alg->setLogging(false);
  alg->setPropertyValue("Workspace", wsName.toStdString());
  executeAlgorithmAsync(alg, true);

  bool hasUB = alg->getProperty("HasUB");
  return hasUB;
}

/**
 * Clears the UB from the selected workspace
 * @param wsName :: selected workspace name
 */
void MantidUI::clearUB(const QStringList &wsName) {
  const std::string algName("ClearUB");
  const int version = -1;
  for (int i = 0; i < wsName.size(); ++i) {
    Mantid::API::IAlgorithm_sptr alg;
    try {
      alg = Mantid::API::AlgorithmManager::Instance().create(algName, version);
    } catch (...) {
      QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                            "Cannot create algorithm " +
                                QString::fromStdString(algName) + " version " +
                                QString::number(version));
      return;
    }
    if (!alg) {
      return;
    }

    alg->setPropertyValue("Workspace", wsName[i].toStdString());
    executeAlgorithmAsync(alg);
  }
}

/**
 * Renames selected workspace
 * @param wsName :: selected workspace name
 */
void MantidUI::renameWorkspace(QStringList wsName) {
  // If the wsname is blank look for an active window and assume this workspace
  // is
  // the one to rename
  if (wsName.isEmpty()) {
    MantidMatrix *matrix =
        dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
    if (matrix) {
      wsName.append(matrix->workspaceName());
    } else {
      return;
    }
  }

  // Determine the algorithm
  QString algName("RenameWorkspace");
  if (wsName.size() > 1)
    algName = "RenameWorkspaces";

  QHash<QString, QString> presets;
  if (wsName.size() > 1) {
    presets["InputWorkspaces"] = wsName.join(",");
  } else {
    presets["InputWorkspace"] = wsName[0];
  }
  showAlgorithmDialog(algName, presets);
}

void MantidUI::setFitFunctionBrowser(
    MantidQt::MantidWidgets::FitPropertyBrowser *newBrowser) {
  if (newBrowser == nullptr)
    m_fitFunction = m_defaultFitFunction;
  else
    m_fitFunction = newBrowser;
}

void MantidUI::groupWorkspaces() {
  try {
    std::string sgrpName("NewGroup");
    QString qwsGrpName = QString::fromStdString(sgrpName);
    // get selected workspaces
    auto selectedItems = m_exploreMantid->getSelectedWorkspaceNames();
    if (selectedItems.size() < 2) {
      throw std::runtime_error("Select at least two workspaces to group ");
    }
    if (Mantid::API::AnalysisDataService::Instance().doesExist(sgrpName)) {
      if (QMessageBox::question(
              appWindow(), "",
              "Workspace " + qwsGrpName +
                  " already exists. Do you want to replace it?",
              QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
        return;
    }
    //
    std::string algName("GroupWorkspaces");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, 1);
    alg->initialize();
    alg->setProperty("InputWorkspaces", selectedItems);
    alg->setPropertyValue("OutputWorkspace", sgrpName);
    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                            " Error in GroupWorkspaces algorithm");
    }

  } catch (std::invalid_argument &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in GroupWorkspaces algorithm");
  } catch (Mantid::Kernel::Exception::NotFoundError &) // if not a valid object
                                                       // in analysis data
                                                       // service
  {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in GroupWorkspaces algorithm");
  } catch (std::runtime_error &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in GroupWorkspaces algorithm");
  } catch (std::exception &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in GroupWorkspaces algorithm");
  }
}
void MantidUI::ungroupWorkspaces() {
  try {
    auto selectedItems = m_exploreMantid->getSelectedWorkspaceNames();
    if (selectedItems.empty()) {
      throw std::runtime_error("Select a group workspace to Ungroup.");
    }

    // workspace name
    std::string wsname = selectedItems[0];

    std::string algName("UnGroupWorkspace");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, 1);
    alg->initialize();
    alg->setProperty("InputWorkspace", wsname);

    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                            " Error in UnGroupWorkspace algorithm");
    }

  } catch (std::invalid_argument &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in UnGroupWorkspace algorithm");
  } catch (std::runtime_error &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in UnGroupWorkspace algorithm");
  } catch (std::exception &) {
    QMessageBox::critical(appWindow(), "MantidPlot - Algorithm error",
                          " Error in UnGroupWorkspace algorithm");
  }
}

Mantid::API::IAlgorithm_sptr MantidUI::createAlgorithm(const QString &algName,
                                                       int version) {
  emit algorithmAboutToBeCreated();
  Mantid::API::IAlgorithm_sptr alg;
  try {
    alg = Mantid::API::AlgorithmManager::Instance().create(
        algName.toStdString(), version);
  } catch (...) {
    QString message = "Cannot create algorithm \"" + algName + "\"";
    if (version != -1) {
      message += " version " + QString::number(version);
    }
    QMessageBox::warning(appWindow(), "MantidPlot", message);
    alg = Mantid::API::IAlgorithm_sptr();
  }
  return alg;
}

bool MantidUI::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                                     const bool wait) {
  if (wait) {
    Poco::ActiveResult<bool> result(alg->executeAsync());
    while (!result.available()) {
      QCoreApplication::processEvents();
    }
    result.wait();

    try {
      return result.data();
    } catch (Poco::NullPointerException &) {
      return false;
    }
  } else {
    try {
      alg->executeAsync();
    } catch (Poco::NoThreadAvailableException &) {
      g_log.error() << "No thread was available to run the " << alg->name()
                    << " algorithm in the background.\n";
      return false;
    }
    return true;
  }
}

/**
 * Slot to update the recent files list (from main appWindow) when accepting
 * LoadDialog dialogs
 */
void MantidUI::loadFileDialogAccept() {
  QObject *sender = QObject::sender();
  MantidQt::API::AlgorithmDialog *dlg =
      reinterpret_cast<MantidQt::API::AlgorithmDialog *>(sender);
  if (!dlg)
    return; // should never happen

  QString fn = MantidQt::API::AlgorithmInputHistory::Instance().previousInput(
      "Load", "Filename");
  appWindow()->updateRecentFilesList(fn);
  // recent files list updated. After this point, the Qt signal handler will go
  // to LoadDialog::accept()
}

void MantidUI::handleLoadDAEFinishedNotification(
    const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf) {
  std::string wsNAme = pNf->algorithm()->getProperty("OutputWorkspace");
  emit needToCreateLoadDAEMantidMatrix(QString::fromStdString(wsNAme));
}

void MantidUI::createLoadDAEMantidMatrix(const QString &wsQName) {
  std::string wsName = wsQName.toStdString();
  Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);

  if (ws.use_count() == 0) {
    QMessageBox::warning(m_appWindow, tr("Mantid"),
                         tr("A workspace with this name already exists.\n"),
                         QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  importMatrixWorkspace(QString::fromStdString(wsName), -1, -1, false, true);

  int updateInterval = m_DAE_map[wsName];
  if (updateInterval > 0) {
    IAlgorithm_sptr updater = createAlgorithm("UpdateDAE");
    updater->setPropertyValue("Workspace", wsName);
    updater->setPropertyValue("UpdateRate",
                              QString::number(updateInterval).toStdString());
    executeAlgorithmAsync(updater);
  }
}

void MantidUI::showCritical(const QString &text) {
  QMessageBox::critical(appWindow(), "Mantid - Error", text);
}

void MantidUI::showAlgMonitor() { m_algMonitor->showDialog(); }

void MantidUI::handleConfigServiceUpdate(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  if (pNf->key() == "pythonscripts.directories") {
    // this code ad the filepaths inside the pythonscripts.directories to the
    // python sys if they are not already there. This is to cope with the
    // requirement
    // at #7097 of letting python scripts usable when downloaded from Script
    // Repository.
    // This code was added because changing the pythonscripts.directories update
    // the
    // python path just after restarting MantidPlot.
    QString code =
        QString("import sys\n"
                "paths = '%1'\n"
                "list_of_path = paths.split(';')\n"
                "if isinstance(list_of_path,str):\n"
                "  list_of_path = [list_of_path,]\n"
                "for value in list_of_path:\n"
                "  if value not in sys.path: sys.path.append(value)\n")
            .arg(QString::fromStdString(pNf->curValue()));
    // run this code silently
    appWindow()->runPythonScript(code, false, true, true);
  }
}

void MantidUI::manageMantidWorkspaces() {
#ifdef _WIN32
  memoryImage();
#else
  QMessageBox::warning(appWindow(), tr("Mantid Workspace"),
                       tr("Clicked on Manage Workspace"), tr("Ok"),
                       tr("Cancel"), QString(), 0, 1);
#endif
}

/** Create an instrument window from a named workspace.
 *  The window will be returned hidden.
 *  @param wsName The name of the workspace for which to generate the instrument
 * view.
 *  @param tab    The index of the tab (starting from 0) to initially display
 * (default: 0)
 *  @return A pointer to the instrument window widget if created. NULL
 * otherwise.
 */
InstrumentWindow *MantidUI::getInstrumentView(const QString &wsName, int tab) {
  if (!Mantid::API::AnalysisDataService::Instance().doesExist(
          wsName.toStdString()))
    return nullptr;
  MatrixWorkspace_const_sptr ws =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(getWorkspace(wsName));
  if (!ws)
    return nullptr;
  ScopedOverrideCursor waitCursor;
  Mantid::Geometry::Instrument_const_sptr instr = ws->getInstrument();
  if (!instr || instr->getName().empty()) {
    QMessageBox::critical(appWindow(), "MantidPlot - Error",
                          "Instrument view cannot be opened");
    return nullptr;
  }

  // Need a new window
  const QString windowName(QString("InstrumentWindow:") + wsName);

  try {
    InstrumentWindow *insWin = new InstrumentWindow(
        wsName, QString("Instrument"), appWindow(), windowName);

    insWin->selectTab(tab);

    appWindow()->addMdiSubWindow(insWin);

    return insWin;
  } catch (const std::exception &e) {
    QString errorMessage =
        "Instrument view cannot be created:\n\n" + QString(e.what());
    QMessageBox::critical(appWindow(), "MantidPlot - Error", errorMessage);

    return nullptr;
  }
}

void MantidUI::showMantidInstrument(const QString &wsName) {
  InstrumentWindow *insWin = getInstrumentView(wsName);

  if (!insWin) {
    m_lastShownInstrumentWin = nullptr;
    return;
  }

  if (workspacesDockPlot1To1()) {
    // replace last one
    if (m_lastShownInstrumentWin) {
      m_lastShownInstrumentWin->close();
      QPoint p = m_lastShownInstrumentWin->pos();
      delete m_lastShownInstrumentWin;
      insWin->move(p);
    }
  }
  m_lastShownInstrumentWin = insWin;

  if (!insWin->isVisible()) {
    insWin->show();
  }
}

void MantidUI::showMantidInstrument() {
  MantidMatrix *m = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  if (!m || !isOfType(m, "MantidMatrix"))
    return;
  if (!m->workspaceName().isEmpty()) {
    showMantidInstrument(m->workspaceName());
  }
}

void MantidUI::showMantidInstrumentSelected() {
  QString wsName = getSelectedWorkspaceName();
  if (!wsName.isEmpty())
    showMantidInstrument(wsName);
}

void MantidUI::mantidMenuAboutToShow() {
  mantidMenu->clear();
  // Ticket #672 Mantid Menu Improvements

  /*mantidMenu->insertItem(tr("&Manage Workspaces"), this,
  SLOT(manageMantidWorkspaces() ) );
  mantidMenu->insertItem(tr("&Instrument Window"), this,
  SLOT(showMantidInstrument() ) );
  mantidMenu->insertItem(tr("&Plot Memory Usage"), this,
  SLOT(manageMantidWorkspaces() ));
  */

  QAction *tstAction = new QAction("&Plot Memory Usage", this);
  connect(tstAction, SIGNAL(triggered()), this, SLOT(manageMantidWorkspaces()));
  mantidMenu->addAction(tstAction);
}

void MantidUI::insertMenu() {
  auto mantidMenuAction = appWindow()->myMenuBar()->addMenu(mantidMenu);
  mantidMenuAction->setText(tr("Man&tid"));
}

void MantidUI::clearAllMemory(const bool prompt) {
  if (prompt) {
    QMessageBox::StandardButton pressed = QMessageBox::question(
        appWindow(), "MantidPlot",
        "All workspaces and windows will be removed. Are you sure?",
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

    if (pressed != QMessageBox::Ok)
      return;
  }
  // If any python objects need to be cleared away then the GIL needs to be
  // held. This doesn't feel like
  // it is in the right place but it will do no harm
  GlobalInterpreterLock lock;
  // Relevant notifications are connected to signals that will close all
  // dependent windows
  Mantid::API::FrameworkManager::Instance().clear();
}

void MantidUI::saveProject(bool saved) {
  if (!saved) {
    QString savemsg =
        tr("Save changes to project: <p><b> %1 </b> ?").arg("untitled");
    int result =
        QMessageBox::information(appWindow(), tr("MantidPlot"), savemsg,
                                 tr("Yes"), tr("No"), nullptr, 2);
    if (result == 0)
      appWindow()->saveProject();
  }
  // close all the windows opened
  foreach (MdiSubWindow *sub_win, appWindow()->windowsList()) {
    sub_win->setconfirmcloseFlag(false);
    sub_win->close();
  }
  Mantid::API::FrameworkManager::Instance().clear();
}

void MantidUI::enableSaveNexus(const QString &wsName) {
  appWindow()->enableSaveNexus(wsName);
}

void MantidUI::disableSaveNexus() { appWindow()->disableSaveNexus(); }

/**
 *  Prepares the Mantid Menu depending on the state of the active MantidMatrix.
 */
void MantidUI::menuMantidMatrixAboutToShow() {
  menuMantidMatrix->clear();
  MantidMatrix *w = dynamic_cast<MantidMatrix *>(appWindow()->activeWindow());
  // menuMantidMatrix->addAction(actionCopyValues);
  menuMantidMatrix->addAction(actionCopyDetectorsToTable);
  menuMantidMatrix->addSeparator();
  menuMantidMatrix->addAction(tr("Set &Properties..."), w,
                              SLOT(setMatrixProperties()));

  ///
  menuMantidMatrix->addSeparator();
  QAction *action = new QAction("Show instrument", this);
  connect(action, SIGNAL(triggered()), this, SLOT(showMantidInstrument()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Plot spectrum...", this);
  connect(action, SIGNAL(triggered()), m_exploreMantid, SLOT(plotSpectra()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Plot as waterfall", this);
  connect(action, SIGNAL(triggered()), SLOT(plotWholeAsWaterfall()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Sample Logs...", this);
  connect(action, SIGNAL(triggered()), this, SLOT(showLogFileWindow()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Sample Material...", this);
  connect(action, SIGNAL(triggered()), this, SLOT(showSampleMaterialWindow()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Show History", this);
  connect(action, SIGNAL(triggered()), this, SLOT(showAlgorithmHistory()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Save Nexus", this);
  connect(action, SIGNAL(triggered()), this, SLOT(saveNexusWorkspace()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Rename", this);
  connect(action, SIGNAL(triggered()), this, SLOT(renameWorkspace()));
  menuMantidMatrix->addAction(action);

  // separate delete
  menuMantidMatrix->addSeparator();

  action = new QAction("Delete", this);
  connect(action, SIGNAL(triggered()), m_exploreMantid,
          SLOT(deleteWorkspaces()));
  menuMantidMatrix->addAction(action);
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer *MantidUI::plotInstrumentSpectrum(const QString &wsName, int spec) {
  QMessageBox::information(appWindow(), "OK",
                           wsName + " " + QString::number(spec));
  return plotSpectraRange(wsName, spec, spec, MantidQt::DistributionDefault,
                          false);
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer *MantidUI::plotInstrumentSpectrumList(const QString &wsName,
                                                 const std::set<int> &spec) {
  return plot1D(wsName, spec, true, MantidQt::DistributionDefault, false);
}

/**
 * Sets the flag that tells the scripting environment that
 * a script is currently running
 */
void MantidUI::setIsRunning(bool) {
  // deprecated
}

/**
 * Merge the curves from the two given MultiLayer objects
 */
MultiLayer *MantidUI::mergePlots(MultiLayer *mlayer_1, MultiLayer *mlayer_2) {
  if (!mlayer_1)
    return nullptr;
  if (!mlayer_2)
    return mlayer_1;
  int ncurves_on_two = mlayer_2->activeGraph()->visibleCurves();
  for (int c = 0; c < ncurves_on_two; ++c) {
    mlayer_1->insertCurve(mlayer_2, c);
  }

  // Hide the second graph for now as closing it
  // deletes the curves that were associated with it
  mlayer_2->close();

  return mlayer_1;
}

MantidMatrix *MantidUI::getMantidMatrix(const QString &wsName) {
  QList<MdiSubWindow *> windows = appWindow()->windowsList();
  QListIterator<MdiSubWindow *> itr(windows);
  MantidMatrix *m(nullptr);
  while (itr.hasNext()) {
    MdiSubWindow *w = itr.next();
    if (isOfType(w, "MantidMatrix") && w->name() == wsName) {
      m = qobject_cast<MantidMatrix *>(w);
    }
  }
  return m;
}

bool MantidUI::createScriptInputDialog(const QString &alg_name,
                                       const QString &preset_values,
                                       const QString &optional_msg,
                                       const QStringList &enabled,
                                       const QStringList &disabled) {
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().newestInstanceOf(alg_name.toStdString());
  if (!alg) {
    return false;
  }

  // PyQt can't pass a dictionary across the boundary as a dictionary can
  // contain arbitrary data types
  QHash<QString, QString> presets;
  QStringList chopped = preset_values.split('|', QString::SkipEmptyParts);
  QStringListIterator itr(chopped);
  while (itr.hasNext()) {
    QString namevalue = itr.next();
    QString name = namevalue.section('=', 0, 0);
    // Simplified removes trims from start and end and replaces all n counts of
    // whitespace with a single whitespace
    QString value = namevalue.section('=', 1, 1).simplified();
    presets.insert(name, value);
  }

  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
      alg, m_appWindow->getScriptWindowHandle(), true, presets, optional_msg,
      enabled, disabled);
  dlg->setShowKeepOpen(false);
  return (dlg->exec() == QDialog::Accepted);
}

/** Displays a string in a Qtiplot table
 *  @param logName :: the title of the table is based on this
 *  @param data :: the string to display
 */
void MantidUI::importString(const QString &logName, const QString &data) {
  importString(logName, data, QString(""));
}

/** Displays a string in a Qtiplot table
 *  @param logName :: the title of the table is based on this
 *  @param data :: the string to display
 *  @param sep :: the separator character
 *  @param wsName :: add workspace name to the table window title bar, defaults
 * to logname if left blank
 */
void MantidUI::importString(const QString &logName, const QString &data,
                            const QString &sep, const QString &wsName) {
  QStringList loglines = QStringList(data);
  if (sep.length() > 0) {
    loglines = data.split(sep, QString::SkipEmptyParts);
  }

  Table *t = new Table(appWindow()->scriptingEnv(), loglines.size(), 1, "",
                       appWindow(), nullptr);
  if (!t)
    return;
  // Have to replace "_" since the legend widget uses them to separate things
  QString label;
  label = logName;
  formatLogName(label, wsName);

  appWindow()->initTable(t, appWindow()->generateUniqueName(label + "-"));
  t->setColName(0, "Log entry");
  t->setColumnType(0, Table::Text);
  t->setReadOnlyColumn(0, true); // Read-only

  for (int i = 0; i < loglines.size(); ++i) {
    t->setText(i, 0, loglines[i]);
  }

  // Show table
  t->resize(2 * t->table()->horizontalHeader()->sectionSize(0) + 55,
            (qMin(10, 1) + 1) * t->table()->verticalHeader()->sectionSize(0) +
                100);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->resizeColumnsToContents();
  t->showNormal();
}
/** Displays a string in a Qtiplot table
 *  @param logName :: the title of the table is based on this
 *  @param data :: a formatted string with the time series data to display
 *  @param wsName :: add workspace name to the table window title bar, defaults
 * to logname if left blank
 */
void MantidUI::importStrSeriesLog(const QString &logName, const QString &data,
                                  const QString &wsName) {
  QStringList loglines = data.split("\n", QString::SkipEmptyParts);

  int rowcount(loglines.count());
  Table *t = new Table(appWindow()->scriptingEnv(), rowcount, 2, "",
                       appWindow(), nullptr);
  if (!t)
    return;
  QString label;
  label = logName;
  formatLogName(label, wsName);

  appWindow()->initTable(t, appWindow()->generateUniqueName(label + "-"));
  t->setColName(0, "Time");
  t->setColumnType(0, Table::Time);
  t->setTimeFormat("HH:mm:ss", 0, false);
  t->setColName(1, label.section("-", 1));
  t->setColumnType(1, Table::Text);

  // Make both columns read-only
  t->setReadOnlyColumn(0, true);
  t->setReadOnlyColumn(1, true);

  QStringList::const_iterator sEnd = loglines.end();
  int row(0);
  for (QStringList::const_iterator sItr = loglines.begin(); sItr != sEnd;
       ++sItr, ++row) {
    QStringList ts = (*sItr).split(QRegExp("\\s+"));
    t->setText(row, 0, ts[1]);
    QStringList ds(ts);
    ds.removeFirst(); // remove date
    ds.removeFirst(); // and time
    t->setText(row, 1, ds.join(" "));
    t->setTextAlignment(row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  }

  // Show table
  t->resize(2 * t->table()->horizontalHeader()->sectionSize(0) + 55,
            (qMin(10, rowcount) + 1) *
                    t->table()->verticalHeader()->sectionSize(0) +
                100);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->resizeColumnsToContents();
  t->showNormal();
}

//------------------------------------------------------------------------------------------------
/**  Import a numeric log data. It will be shown in a graph and copied into a
 * table
 * @param wsName :: The workspace name which log data will be imported
 * @param logName :: The name of the log property to import
 * @param filter :: Filter flag telling how to filter the log data.
 * - 0 means no filtering
 * - 1 filter by running status
 * - 2 filter by period
 * - 3 filter by status & period
 */
void MantidUI::importNumSeriesLog(const QString &wsName, const QString &logName,
                                  int filter) {
  // if you need to add a final filter value to the end of the filter to match
  // the extent of the data, then set this to the index of the row to add the
  // value
  int addFinalFilterValueIndex = 0;
  Mantid::Types::Core::DateAndTime lastFilterTime;

  // Convert input int into enum value
  const Mantid::API::LogFilterGenerator::FilterType filterType = [&filter]() {
    switch (filter) {
    case 0:
      return Mantid::API::LogFilterGenerator::FilterType::None;
    case 1:
      return Mantid::API::LogFilterGenerator::FilterType::Status;
    case 2:
      return Mantid::API::LogFilterGenerator::FilterType::Period;
    case 3:
      return Mantid::API::LogFilterGenerator::FilterType::StatusAndPeriod;
    default:
      return Mantid::API::LogFilterGenerator::FilterType::None;
    }
  }();

  // Make sure the workspace exists and contains the log
  MatrixWorkspace_const_sptr ws =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(getWorkspace(wsName));
  if (!ws)
    return;

  Mantid::Kernel::Property *logData =
      ws->run().getLogData(logName.toStdString());
  if (!logData)
    return;

  // Generate the filter
  Mantid::API::LogFilterGenerator filterGenerator(filterType, ws);
  const auto &flt = filterGenerator.generateFilter(logName.toStdString());

  // Get a map of time/value. This greatly speeds up display.
  // NOTE: valueAsMap() skips repeated values.
  std::map<DateAndTime, double> time_value_map =
      flt->data()->valueAsCorrectMap();
  int rowcount = static_cast<int>(time_value_map.size());
  int colCount = 2;

  Table *t = new Table(appWindow()->scriptingEnv(), rowcount, colCount, "",
                       appWindow(), nullptr);
  if (!t)
    return;
  // Have to replace "_" since the legend widget uses them to separate things
  QString label;
  label = logName;
  formatLogName(label, wsName);

  // Get the starting time of the log.
  Mantid::Types::Core::DateAndTime startTime;
  // Toggle to switch between using the real date or the change in seconds.
  bool useAbsoluteDate = false;

  if (!time_value_map.empty()) {
    try {
      startTime = ws->run().startTime();
    } catch (std::runtime_error &) {
      // This means the start time is missing, use absolute times instead
      useAbsoluteDate = true;
    }
  }

  // Make a unique title, and put in the start time of the log
  QString title =
      label + QString::fromStdString(" (" + startTime.toSimpleString() + ")");
  appWindow()->initTable(t, appWindow()->generateUniqueName(title));

  // Make both columns read-only
  t->setReadOnlyColumn(0, true);
  t->setReadOnlyColumn(1, true);
  // Set numeric precision.
  // It's the number of all digits
  t->setNumericPrecision(16);

  if (useAbsoluteDate) {
    // --------- Date
    t->setColName(0, "Time");
    t->setColumnType(0, Table::Date);
    t->setDateFormat("yyyy-MMM-dd HH:mm:ss.ffffff", 0, false);
  } else {
    // Seconds offset
    t->setColName(0, "Time (sec)");
    t->setColumnType(0, Table::Numeric);
  }

  // Make the column header with the units, if any
  QString column1 = label.section("-", 1);
  if (logData->units() != "")
    column1 =
        column1 + QString::fromStdString(" (in " + logData->units() + ")");
  t->setColName(1, column1);

  int iValueCurve = 0;

  // Applying filter column to table
  if (filterType != Mantid::API::LogFilterGenerator::FilterType::None) {
    if (flt->filter()) {
      // Valid filter was found
      t->addColumns(2);
      t->setColName(2, "FTime");

      if (useAbsoluteDate) {
        t->setColumnType(2, Table::Date);
        t->setDateFormat("yyyy-MMM-dd HH:mm:ss", 2,
                         false); // This is the format of the date column
      } else {
        t->setColumnType(2, Table::Numeric);
      }

      t->setColPlotDesignation(2, Table::X);
      t->setColName(3, "Filter");

      if (flt->filter()->size() > rowcount) {
        t->addRows(flt->filter()->size() - rowcount);
      }

      if (flt->data()->size() > rowcount) {
        t->addRows(flt->data()->size() - rowcount);
      }

      for (int i = 0; i < flt->filter()->size(); i++) {
        if (flt->filter()->nthInterval(i).begin() >
            0) // protect against bizarre values we sometimes get
        {
          std::string time_string =
              extractLogTime(flt->filter()->nthInterval(i).begin(),
                             useAbsoluteDate, startTime);

          t->setText(i, 2, QString::fromStdString(time_string));
          t->setCell(i, 3, !flt->filter()->nthValue(i));
          if ((i + 1 == flt->filter()->size()) &&
              (!flt->filter()->nthValue(
                  i))) // last filter value and set to be filtering
          {
            addFinalFilterValueIndex = i + 1;
            lastFilterTime = flt->filter()->nthInterval(i).begin();
          }
        }
      }

    } // end (valid filter exists)
  }

  Mantid::Types::Core::DateAndTime lastTime;
  double lastValue = 0;

  // Iterate through the time-value map.
  std::map<DateAndTime, double>::iterator it = time_value_map.begin();
  if (it != time_value_map.end()) {
    for (int i = 0; it != time_value_map.end(); ++i, ++it) {
      lastTime = it->first;
      lastValue = it->second;

      std::string time_string =
          extractLogTime(lastTime, useAbsoluteDate, startTime);

      t->setText(i, 0, QString::fromStdString(time_string));
      t->setCell(i, 1, lastValue);
    }
  }

  try {
    // Set the filter strings
    if (filter && flt->filter() && lastTime < flt->filter()->lastTime()) {
      rowcount = static_cast<int>(time_value_map.size());
      if (rowcount == t->numRows())
        t->addRows(1);

      std::string time_string =
          extractLogTime(flt->filter()->lastTime(), useAbsoluteDate, startTime);

      t->setText(rowcount, 0, QString::fromStdString(time_string));
      t->setCell(rowcount, 1, lastValue);
    }
  } catch (...) {
  }

  // add a final filter value if needed and the data exceed the filter range
  if ((addFinalFilterValueIndex > 0) && (lastFilterTime < lastTime)) {
    if (addFinalFilterValueIndex >= t->numRows()) {
      t->addRows(1);
    }
    std::string end_string =
        extractLogTime(lastTime, useAbsoluteDate, startTime);
    t->setText(addFinalFilterValueIndex, 2, QString::fromStdString(end_string));
    t->setCell(addFinalFilterValueIndex, 3,
               1); // only need to add it if filter =1
  }

  // Show table

  t->resize(2 * t->table()->horizontalHeader()->sectionSize(0) + 55,
            (qMin(10, t->numRows()) + 1) *
                    t->table()->verticalHeader()->sectionSize(0) +
                100);
  // t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();

  // Do not create graph if there is only one value in the table or using
  // absolute dates
  if (t->numRows() < 2 || useAbsoluteDate)
    return;

  QStringList colNames;
  if (filter && flt->filter()) {
    colNames << t->colName(3);
  }
  colNames << t->colName(1);
  MultiLayer *ml = appWindow()->multilayerPlot(t, colNames, GraphOptions::Line);
  // ml->askOnCloseEvent(false);
  ml->setAttribute(Qt::WA_DeleteOnClose);

  Graph *g = ml->activeGraph();

  // Set x-axis label format
  if (useAbsoluteDate) {
    Mantid::Types::Core::DateAndTime label_as_ptime =
        flt->data()->nthInterval(0).begin();
    QDateTime dt = QDateTime::fromTime_t(uint(label_as_ptime.to_localtime_t()));
    QString format = dt.toString(Qt::ISODate) + ";HH:mm:ss";
    g->setLabelsDateTimeFormat(2, ScaleDraw::Date, format);
  } else {
    // Make the x-axis a numeric format, 0 decimals
    g->setLabelsNumericFormat(2, 1, 0, "");
  }

  // Set style #3 (HorizontalSteps) for curve iValueCurve
  g->setCurveStyle(iValueCurve, 3);
  QPen pn = QPen(Qt::black);
  g->setCurvePen(iValueCurve, pn);

  if (filter && flt->filter()) {
    int iFilterCurve = 1;
    QwtPlotCurve *c = g->curve(iFilterCurve);
    if (c) {
      // Set the right axis as Y axis for the filter curve.
      c->setAxis(2, 1);
      // Set style #3 (HorizontalSteps) for curve 1
      // Set scale of right Y-axis (#3) from 0 to 1
      g->setCurveStyle(iFilterCurve, 3);
      g->setScale(3, 0, 1);
      // Fill area under the curve with a pattern
      QBrush br = QBrush(Qt::gray, Qt::Dense5Pattern);
      g->setCurveBrush(iFilterCurve, br);
      // Set line colour
      QPen pn = QPen(Qt::gray);
      g->setCurvePen(iFilterCurve, pn);
    }
  }
  g->setTitle(label);
  g->setAutoScale();

  ml->showNormal();
}

/** Format a log name for a title bar
@param[out] label :: the QString that will hold the caption
@param[in] wsName :: the workspace name
*/
void MantidUI::formatLogName(QString &label, const QString &wsName) {
  label.replace("_", "-");
  if (!wsName.isEmpty()) {
    label = wsName + "-" + label;
  }
}

void MantidUI::configModified() {
  m_exploreMantid->enableDeletePrompt(
      appWindow()->isDeleteWorkspacePromptEnabled());
}

std::string MantidUI::extractLogTime(DateAndTime value, bool useAbsoluteDate,
                                     DateAndTime start) {
  std::string time_string;
  if (useAbsoluteDate) {
    // Convert time into string
    time_string = value.toSimpleString();
  } else {
    // How many seconds elapsed?
    time_duration elapsed = value - start;
    double seconds = DateAndTime::secondsFromDuration(elapsed);

    // Output with 6 decimal points
    std::ostringstream oss;
    oss.precision(6);
    oss << std::fixed << seconds;
    time_string = oss.str();
  }
  return time_string;
}

void MantidUI::showLogFileWindow() {
  // Need a new window to display entries
  MantidSampleLogDialog *dlg =
      new MantidSampleLogDialog(getSelectedWorkspaceName(), this);
  dlg->setModal(false);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->setFocus();
}

void MantidUI::showSampleMaterialWindow() {
  MantidSampleMaterialDialog *dlg =
      new MantidSampleMaterialDialog(getSelectedWorkspaceName(), this);
  dlg->setModal(false);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->setFocus();
  dlg->updateMaterial();
}

//  *****      Plotting Methods     *****  //

/** Create a Table form specified spectra in a MatrixWorkspace
@param tableName :: Table name
@param workspaceName :: Shared pointer to the workspace
@param indexList :: A list of spectra indices to go to the table
@param errs :: If true include the errors into the table
@param binCentres :: If true the X column will contain the bin centres, i.e.
(x_i+1 + x_i)/2.
If false the Y values will be in the same row with the left bin boundaries.
If the workspace is not a histogram the parameter is ignored.
*/
Table *MantidUI::createTableFromSpectraList(const QString &tableName,
                                            const QString &workspaceName,
                                            QList<int> indexList, bool errs,
                                            bool binCentres) {
  MatrixWorkspace_const_sptr workspace =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(
          getWorkspace(workspaceName));
  if (!workspace) {
    throw std::invalid_argument(workspaceName.toStdString() +
                                " is not a Matrix Workspace.");
  }

  int nspec = static_cast<int>(workspace->getNumberHistograms());
  // Loop through the list of index and remove all the indexes that are out of
  // range

  for (QList<int>::iterator it = indexList.begin(); it != indexList.end();) {
    if ((*it) > nspec || (*it) < 0) {
      // erase moves iterator to next position
      it = indexList.erase(it);
    } else {
      ++it;
    }
  }
  if (indexList.empty())
    return nullptr;

  int c = errs ? 2 : 1;
  int numRows = static_cast<int>(workspace->blocksize());
  bool isHistogram = workspace->isHistogramData();
  int no_cols = static_cast<int>(indexList.size());
  Table *t = new Table(appWindow()->scriptingEnv(), numRows, (1 + c) * no_cols,
                       "", appWindow(), nullptr);
  appWindow()->initTable(t, appWindow()->generateUniqueName(tableName + "-"));
  // t->askOnCloseEvent(false);

  for (int i = 0; i < no_cols; i++) {
    const auto &dataXPoints = workspace->points(indexList[i]);
    const auto &dataY = workspace->y(indexList[i]);
    const auto &dataE = workspace->e(indexList[i]);

    const int kY = (c + 1) * i + 1;
    const int kX = (c + 1) * i;
    int kErr = 0;
    t->setColName(kY, "YS" + QString::number(indexList[i]));
    t->setColName(kX, "XS" + QString::number(indexList[i]));
    t->setColPlotDesignation(kX, Table::X);
    if (errs) {
      kErr = (c + 1) * i + 2;
      t->setColPlotDesignation(kErr, Table::yErr);
      t->setColName(kErr, "ES" + QString::number(indexList[i]));
    }
    for (int j = 0; j < numRows; j++) {
      t->setCell(j, kX, dataXPoints[j]);
      t->setCell(j, kY, dataY[j]);

      if (errs)
        t->setCell(j, kErr, dataE[j]);
    }
    if (isHistogram && (!binCentres)) {
      int iRow = numRows;
      t->addRows(1);
      if (i == 0)
        t->setCell(iRow, 0, dataXPoints[iRow]);
      t->setCell(iRow, kY, 0);
      if (errs)
        t->setCell(iRow, kErr, 0);
    }
  }

  return t;
}

/** Creates a Qtiplot Table from selected spectra of MantidMatrix m.
The columns are: 1st column is x-values from the first selected spectrum,
2nd column is y-values of the first spectrum. Depending on value of errs
the 3rd column contains either first spectrum errors (errs == true) or
y-values of the second spectrum (errs == false). Consecutive columns have
y-values and errors (if errs is true) of the following spectra. If visible ==
true
the table is made visible in Qtiplot.

The name of a Y column is "Y"+QString\:\:number(i), where i is the row in the
MantidMatrix,
not the spectrum index in the workspace.

*/
Table *MantidUI::createTableFromSelectedRows(MantidMatrix *m, bool errs,
                                             bool binCentres) {
  const QList<int> &indexList = m->getSelectedRows();
  if (indexList.empty())
    return nullptr;

  return createTableFromSpectraList(
      m->name(), QString::fromStdString(m->workspace()->getName()), indexList,
      errs, binCentres);
}

/**  Create a 1d graph from a Table.
@param t :: Pointer to the Table.
@param type :: Type of the curve. Possible values are:
- Graph::Line
- Graph::Scatter
- Graph::LineSymbols
- Graph::HorizontalSteps
*/
MultiLayer *MantidUI::createGraphFromTable(Table *t, int type) {
  if (!t)
    return nullptr;
  QStringList lst = t->colNames();
  QStringList::const_iterator itr;
  for (itr = lst.begin(); itr != lst.end(); ++itr) {
    // remove the X names from the column list and pass the X removed list
    // to multilayerPlot
    QString str = (*itr);
    if (str.contains("XS", Qt::CaseInsensitive)) {
      int index = lst.indexOf(str);
      lst.removeAt(index);
    }
  }

  MultiLayer *ml = appWindow()->multilayerPlot(t, lst, GraphOptions::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g, type);
  for (int i = 0; i < g->curves(); i++)
    g->setCurveStyle(i, type);

  return ml;
}

/** Set properties of a 1d graph which plots bin data from a workspace.
@param ml :: MultiLayer plot with the graph
@param Name :: Name of the graph
@param workspace :: The workspace
*/
void MantidUI::setUpBinGraph(
    MultiLayer *ml, const QString &Name,
    Mantid::API::MatrixWorkspace_const_sptr workspace) {
  Graph *g = ml->activeGraph();
  g->setTitle(tr("Workspace ") + Name);
  QString xtitle;
  if (workspace->axes() >
      1) // Protection against calling this on 1D/single value workspaces
  {
    xtitle = MantidQt::API::PlotAxis(*workspace, 1).title();
  }
}

/**
Plots the spectra from the given workspaces
@param ws_names :: List of ws names to plot
@param indexList :: List of indices to plot for each workspace
@param spectrumPlot :: True if indices should be interpreted as row indices
@param distr :: if true, workspace plot as y data/bin width
@param errs :: If true include the errors on the graph
@param style :: Curve style for plot
@param plotWindow :: Window to plot to. If NULL a new one will be created
@param clearWindow :: Whether to clear specified plotWindow before plotting.
Ignored if plotWindow == NULL
@param waterfallPlot :: If true create a waterfall type plot
*/
MultiLayer *MantidUI::plot1D(const QStringList &ws_names,
                             const QList<int> &indexList, bool spectrumPlot,
                             MantidQt::DistributionFlag distr, bool errs,
                             GraphOptions::CurveType style,
                             MultiLayer *plotWindow, bool clearWindow,
                             bool waterfallPlot) {
  // Convert the list into a map (with the same workspace as key in each case)
  QMultiMap<QString, int> pairs;
  QListIterator<QString> ws_itr(ws_names);
  ws_itr.toBack();
  QListIterator<int> spec_itr(indexList);
  spec_itr.toBack();

  // Need to iterate through the set in reverse order to get the curves in the
  // correct order on the plot
  while (ws_itr.hasPrevious()) {
    QString workspace_name = ws_itr.previous();
    while (spec_itr.hasPrevious()) {
      pairs.insert(workspace_name, spec_itr.previous());
    }
    // Reset spectrum index pointer
    spec_itr.toBack();
  }

  // Pass over to the overloaded method
  return plot1D(pairs, spectrumPlot, distr, errs, style, plotWindow,
                clearWindow, waterfallPlot);
}

/** Create a 1D graph from the specified list of workspaces/spectra.
@param toPlot :: Map of form ws -> [spectra_list]
@param spectrumPlot :: True if indices should be interpreted as row indices
@param distr :: if true, workspace plot as y data/bin width
@param errs :: If true include the errors on the graph
@param plotWindow :: Window to plot to. If NULL a new one will be created
@param clearWindow :: Whether to clear specified plotWindow before plotting.
Ignored if plotWindow == NULL
@param waterfallPlot :: If true create a waterfall type plot
@param log :: log name for advanced plotting
@param customLogValues :: custom log values for advanced plotting
@return NULL if failure. Otherwise, if plotWindow == NULL - created window, if
not NULL - plotWindow
*/
MultiLayer *MantidUI::plot1D(const QMultiMap<QString, set<int>> &toPlot,
                             bool spectrumPlot,
                             MantidQt::DistributionFlag distr, bool errs,
                             MultiLayer *plotWindow, bool clearWindow,
                             bool waterfallPlot, const QString &log,
                             const std::set<double> &customLogValues) {
  // Convert the list into a map (with the same workspace as key in each case)
  bool multipleSpectra = false;
  QMultiMap<QString, int> pairs;
  // Need to iterate through the workspaces
  QMultiMap<QString, std::set<int>>::const_iterator it;
  for (it = toPlot.constBegin(); it != toPlot.constEnd(); ++it) {
    multipleSpectra = multipleSpectra || (it.value().size() > 1);
    std::set<int>::const_reverse_iterator itSet;
    for (itSet = it->rbegin(); itSet != it->rend(); ++itSet) {
      pairs.insert(it.key(), *itSet);
    }
  }

  // Pass over to the overloaded method
  return plot1D(pairs, spectrumPlot, distr, errs, GraphOptions::Unspecified,
                plotWindow, clearWindow, waterfallPlot, log, customLogValues,
                multipleSpectra);
}

/** Create a 1d graph from the specified spectra in a MatrixWorkspace
@param wsName :: Workspace name
@param indexList :: List of indices to plot for each workspace
@param spectrumPlot :: True if indices should be interpreted as row indices
@param distr :: if true, workspace plot as y data/bin width
@param errs :: If true include the errors on the graph
@param plotWindow :: Window to plot to. If NULL a new one will be created
@param clearWindow :: Whether to clear specified plotWindow before plotting.
Ignored if plotWindow == NULL
@param waterfallPlot :: If true create a waterfall type plot
@return NULL if failure. Otherwise, if plotWindow == NULL - created window, if
not NULL - plotWindow
*/
MultiLayer *MantidUI::plot1D(const QString &wsName,
                             const std::set<int> &indexList, bool spectrumPlot,
                             MantidQt::DistributionFlag distr, bool errs,
                             MultiLayer *plotWindow, bool clearWindow,
                             bool waterfallPlot) {
  // Convert the list into a map (with the same workspace as key in each case)
  QMultiMap<QString, int> pairs;
  // Need to iterate through the set in reverse order
  std::set<int>::const_reverse_iterator it;
  for (it = indexList.rbegin(); it != indexList.rend(); ++it) {
    pairs.insert(wsName, *it);
  }

  // Pass over to the overloaded method
  return plot1D(pairs, spectrumPlot, distr, errs, GraphOptions::Unspecified,
                plotWindow, clearWindow, waterfallPlot);
}

/** Create a 1d graph form a set of workspace-spectrum pairs
@param toPlot :: A list of workspace/spectra to be shown in the graph
@param spectrumPlot :: True if indices should be interpreted as row indices
@param distr :: if true, workspace plot as y data/bin width
@param errs :: If true include the errors to the graph
@param style :: curve style for plot
@param plotWindow :: Window to plot to. If NULL a new one will be created
@param clearWindow :: Whether to clear specified plotWindow before plotting.
Ignored if plotWindow == NULL
@param waterfallPlot :: If true create a waterfall type plot
@param log :: log name for advanced plotting
@param customLogValues :: custom log values for advanced plotting
@param multipleSpectra :: indicates that there are multiple spectra and
so spectrum numbers must always be shown in the plot legend.
@return NULL if failure. Otherwise, if plotWindow == NULL - created window, if
not NULL - plotWindow
*/
MultiLayer *MantidUI::plot1D(const QMultiMap<QString, int> &toPlot,
                             bool spectrumPlot,
                             MantidQt::DistributionFlag distr, bool errs,
                             GraphOptions::CurveType style,
                             MultiLayer *plotWindow, bool clearWindow,
                             bool waterfallPlot, const QString &log,
                             const std::set<double> &customLogValues,
                             bool multipleSpectra) {
  if (toPlot.size() == 0)
    return nullptr;

  if (toPlot.size() > 10) {
    QMessageBox ask(appWindow());
    QAbstractButton *confirmButton =
        ask.addButton(tr("Confirm"), QMessageBox::ActionRole);
    ask.addButton(tr("Cancel"), QMessageBox::ActionRole);
    ask.setText("You selected " + QString::number(toPlot.size()) +
                " spectra to plot. "
                "Are you sure you want to plot this many?");
    ask.setIcon(QMessageBox::Question);
    ask.exec();
    if (ask.clickedButton() != confirmButton)
      return nullptr;
  }
  // Force waterfall option to false if only 1 curve
  if ((nullptr == plotWindow || clearWindow) && toPlot.size() == 1)
    waterfallPlot = false;

  ScopedOverrideCursor waitCursor;

  // If the first workspace selected in the tree is a WorkspaceGroup,
  // use its name directly, rather than the first in the list 'toPlot'
  // (which will be the first workspace included in the group - not
  // the best title).
  QString plotTitle = getSelectedGroupName();
  if (plotTitle.isEmpty()) {
    plotTitle = toPlot.constBegin().key();
  }

  // Limit to 1 window for this type of plot -> reuse plot/graph window
  if (workspacesDockPlot1To1()) {
    if (m_lastShown1DPlotWin) {
      plotWindow = m_lastShown1DPlotWin;
      clearWindow = true;
    }
  }
  bool isGraphNew = false;
  MultiLayer *ml = appWindow()->prepareMultiLayer(isGraphNew, plotWindow,
                                                  plotTitle, clearWindow);
  m_lastShown1DPlotWin = ml;

  // Do we plot try to plot as distribution. If request and it is not already
  // one!
  bool plotAsDistribution(false);
  if (distr == MantidQt::DistributionDefault) {
    plotAsDistribution = appWindow()->autoDistribution1D;
  } else {
    plotAsDistribution = (distr == MantidQt::DistributionTrue);
  }

  vector<CurveSpec> curveSpecList;
  putLogsIntoCurveSpecs(curveSpecList, toPlot, log, customLogValues);

  // Add curves to the plot
  Graph *g = ml->activeGraph();
  MantidMatrixCurve::IndexDir indexType =
      (spectrumPlot) ? MantidMatrixCurve::Spectrum : MantidMatrixCurve::Bin;
  MantidMatrixCurve *firstCurve(nullptr);
  QString logValue("");
  for (const auto &curveSpec : curveSpecList) {

    if (!log.isEmpty()) { // Get log value from workspace
      logValue = logValue.number(curveSpec.logVal, 'g', 6);
    }

    auto *wsCurve = new MantidMatrixCurve(
        logValue, curveSpec.wsName, g, curveSpec.index, indexType, errs,
        plotAsDistribution, style, multipleSpectra);
    if (!firstCurve) {
      firstCurve = wsCurve;
      g->setNormalizable(firstCurve->isNormalizable());
      g->setDistribution(firstCurve->isDistribution());
    }
  }

  if (!isGraphNew) {
    // Replot graph is we've added curves to existing one
    g->replot();
  } else {
    if (!firstCurve) {
      return nullptr;
    }

    // Ensure plot encompasses all data points
    setInitialAutoscale(g);

    // This deals with the case where the X-values are not in order. In general,
    // this shouldn't
    // happen, but it does apparently with some muon analyses.
    g->checkValuesInAxisRange(firstCurve);
  }
  ml->toggleWaterfall(waterfallPlot);

  // Check if window does not contain any curves and should be closed
  ml->maybeNeedToClose();

  return ml;
}

/* Get the log values and put into a curve spec list in preparation of
 *  the creation of the curves
 *  @param curveSpecList :: list of curve specs to receive the logs
 *  @param toPlot :: workspaces to plot
 *  @param log :: log value
 *  @param customLogValues :: custom log values
 */
void MantidUI::putLogsIntoCurveSpecs(std::vector<CurveSpec> &curveSpecList,
                                     const QMultiMap<QString, int> &toPlot,
                                     const QString &log,
                                     const std::set<double> &customLogValues) {
  // Try to store log values, if needed, and prepare for sorting.
  int i = 0;
  for (QMultiMap<QString, int>::const_iterator it = toPlot.begin();
       it != toPlot.end(); ++it) {
    CurveSpec curveSpec;

    try {
      if (!log.isEmpty()) { // Get log value from workspace
        if (!customLogValues.empty()) {
          curveSpec.logVal = getSingleWorkspaceLogValue(i++, customLogValues);
        } else {
          MatrixWorkspace_const_sptr workspace =
              AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                  it.key().toStdString());
          curveSpec.logVal = getSingleWorkspaceLogValue(1, workspace, log);
        }
      } else {
        curveSpec.logVal = 0.1234; // This should not be used.
      }
      curveSpec.wsName = it.key();
      curveSpec.index = it.value();
      curveSpecList.push_back(curveSpec);

    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      g_log.warning() << "Workspace " << it.key().toStdString()
                      << " not found\n";
    } catch (std::exception &ex) {
      g_log.warning() << ex.what() << '\n';
    }
  }

  // Sort curves, if log values are used
  if (!log.isEmpty()) {
    sort(curveSpecList.begin(), curveSpecList.end(), byLogValue);
  }
}

/**
 * Draw a color fill plot for each of the listed workspaces. Unfortunately the
 * plotting is
 * initimately linked to MantidMatrix so that one of these needs to be created
 * first
 * @param ui :: the sequential fitting UI form
 * @param fitbrowser :: pointer to the fit property browser
 */
void MantidUI::showSequentialPlot(
    Ui::SequentialFitDialog *ui,
    MantidQt::MantidWidgets::FitPropertyBrowser *fitbrowser) {
  std::string wsName = fitbrowser->outputName();
  Mantid::API::ITableWorkspace_sptr ws =
      boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
  if (ws) {
    if ((ws->columnCount() - 1) / 2 !=
        fitbrowser->compositeFunction()->nParams())
      return;
    Table *t = importTableWorkspace(QString::fromStdString(wsName));
    if (!t)
      return;
    QString parName;
    if (fitbrowser->compositeFunction()->nFunctions() == 1) {
      size_t i = fitbrowser->compositeFunction()->parameterIndex(
          ui->cbParameter->currentText().toStdString());
      parName = QString::fromStdString(
          fitbrowser->compositeFunction()->getFunction(0)->parameterName(i));
    } else {
      parName = ui->cbParameter->currentText();
    }
    QStringList colNames;
    colNames << t->name() + "_" + parName << t->name() + "_" + parName + "_Err";
    MultiLayer *ml = appWindow()->multilayerPlot(
        t, colNames, ui->cbCurveType->currentIndex());
    // set plot titles
    Graph *g = ml->activeGraph();
    if (g) {
      if (ui->ckbLogPlot->isChecked()) {
        g->setXAxisTitle(ui->cbLogValue->currentText());
      } else {
        g->setXAxisTitle("Spectra");
      }
      g->setYAxisTitle(parName);
      g->setTitle("");
    }
  }
}

/**
 * Draw a color fill plot for each of the listed workspaces. Unfortunately the
 * plotting is
 * initimately linked to MantidMatrix so that one of these needs to be created
 * first
 * @param wsNames :: For each workspace listed create a 2D colorfill plot
 * @param curveType :: The curve type for each of the plots
 */
void MantidUI::drawColorFillPlots(const QStringList &wsNames,
                                  GraphOptions::CurveType curveType) {
  int nPlots = wsNames.size();
  if (nPlots > 1) {
    QList<MultiLayer *> plots;
    for (const auto &wsName : wsNames) {
      const bool hidden = true;
      MultiLayer *plot =
          this->drawSingleColorFillPlot(wsName, curveType, nullptr, hidden);
      if (plot)
        plots.append(plot);
    }

    if (!plots.isEmpty()) {
      nPlots = plots.size();

      int nCols = 1;
      if (nPlots >= 16)
        nCols = 4;
      else if (nPlots >= 9)
        nCols = 3;
      else if (nPlots >= 4)
        nCols = 2;
      else
        nCols = nPlots;

      int nRows = nPlots / nCols;
      if (nPlots % nCols != 0)
        ++nRows;

      auto tiledWindow = new TiledWindow(
          appWindow(), "", appWindow()->generateUniqueName("TiledWindow"),
          nRows, nCols);

      int row = 0;
      int col = 0;
      for (auto &plot : plots) {
        tiledWindow->addWidget(plot, row, col);
        ++col;
        if (col == nCols) {
          col = 0;
          ++row;
        }
      }

      appWindow()->addMdiSubWindow(tiledWindow);
    }
  } else if (nPlots == 1) {
    this->drawSingleColorFillPlot(wsNames.front(), curveType);
  }
}

/**
 * Draw a single ColorFill plot for the named workspace
 * @param wsName :: The name of the workspace which provides data for the plot
 * @param curveType :: The type of curve
 * @param window :: An optional pointer to a plot window. If not NULL the window
 * is cleared
 *                      and reused
 * @param hidden
 * @returns A pointer to the created plot
 */
MultiLayer *MantidUI::drawSingleColorFillPlot(const QString &wsName,
                                              GraphOptions::CurveType curveType,
                                              MultiLayer *window, bool hidden) {
  auto workspace =
      boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
          getWorkspace(wsName));
  if (!workspace)
    return nullptr;

  ScopedOverrideCursor waitCursor;

  bool reusePlots = workspacesDockPlot1To1();
  if ((!reusePlots && nullptr == window) ||
      (reusePlots && !m_lastShownColorFillWin)) // needs to create a new window
  {
    try {
      window = appWindow()->multilayerPlot(
          appWindow()->generateUniqueName(wsName + "-"));
      if (hidden) {
        window->hide();
      }
    } catch (std::runtime_error &e) {
      m_lastShownColorFillWin = nullptr;
      g_log.error() << "Could not create color fill plot: " << e.what() << "\n";
      throw std::runtime_error(e);
    }
    window->setCloseOnEmpty(true);
    m_lastShownColorFillWin = window;
  } else {
    if (nullptr == window) {
      if (nullptr == m_lastShownColorFillWin)
        return nullptr;
      window = m_lastShownColorFillWin;
    }
    // start fresh layer
    window->setName(appWindow()->generateUniqueName(wsName + "-"));
    window->setLayersNumber(0);
    window->addLayer();
  }

  Graph *plot = window->activeGraph();
  appWindow()->setPreferences(plot);

  plot->setTitle(wsName);

  Spectrogram *spgrm = new Spectrogram(wsName, workspace);
  plot->plotSpectrogram(spgrm, curveType);
  connect(spgrm, SIGNAL(removeMe(Spectrogram *)), plot,
          SLOT(removeSpectrogram(Spectrogram *)));
  connect(plot, SIGNAL(curveRemoved()), window, SLOT(maybeNeedToClose()),
          Qt::QueuedConnection);

  appWindow()->setSpectrogramTickStyle(plot);
  setInitialAutoscale(plot);

  return window;
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace
@param wsName :: Workspace name
@param i0 :: Starting index
@param i1 :: Last index
@param errs :: If true include the errors to the graph
@param distr :: if true, workspace is a distribution
*/
MultiLayer *MantidUI::plotSpectraRange(const QString &wsName, int i0, int i1,
                                       MantidQt::DistributionFlag distr,
                                       bool errs) {
  if (i0 < 0 || i1 < 0)
    return nullptr;
  /** For instrument with one to many spectra-detector mapping,
   * different pixels with correspond to the same specta so
   * we need to remove doublons in this case.
   */
  std::set<int> indexList;
  for (int i = i0; i <= i1; i++)
    indexList.insert(i);

  return plot1D(wsName, indexList, true, distr, errs);
}

/**  Create a graph and plot the selected rows of a MantidMatrix
@param m :: Mantid matrix
@param distr :: if true, workspace is a distribution
@param errs :: True if the errors to be plotted
*/
MultiLayer *MantidUI::plotSelectedRows(const MantidMatrix *const m,
                                       MantidQt::DistributionFlag distr,
                                       bool errs) {
  const QList<int> &rows = m->getSelectedRows();
  std::set<int> rowSet(rows.constBegin(), rows.constEnd());

  return plot1D(m->workspaceName(), rowSet, true, distr, errs);
}

/**
 * Create a graph and plot the selected columns of a MantidMatrix
 * @param m MantidMatrix
 * @param errs True if errors are required
 * @return
 */
MultiLayer *MantidUI::plotSelectedColumns(const MantidMatrix *const m,
                                          bool errs) {
  const QList<int> &cols = m->getSelectedColumns();
  std::set<int> colSet(cols.constBegin(), cols.constEnd());

  return plot1D(m->workspaceName(), colSet, false, MantidQt::DistributionFalse,
                errs);
}

/**
 * Plot a "tiled" plot (with subplots).
 * Ask user for confirmation if lots of plots are chosen.
 * If just one workspace, put each spectrum in its own subplot
 * If multiple workspaces, each ws gets its own subplot
 * @param toPlot :: Map of form ws -> [spectra_list]
 * @param distr :: if true, workspace plot as distribution (y data/bin width)
 * @param errs :: if true, plot the errors on the graph
 * @param plotWindow :: Window to plot in - if null, create a new one
 * @return created MultiLayer, or null on failure
 */
MultiLayer *MantidUI::plotSubplots(const QMultiMap<QString, set<int>> &toPlot,
                                   MantidQt::DistributionFlag distr, bool errs,
                                   MultiLayer *plotWindow) {
  // Check if nothing to plot
  if (toPlot.size() == 0)
    return nullptr;

  // If one workspace, each spectrum goes in its own subplot.
  // Otherwise, there is one subplot per workspace.
  const int nWorkspaces = toPlot.size();
  const int nSubplots = [&toPlot, &nWorkspaces]() {
    if (nWorkspaces == 1) {
      return static_cast<int>(toPlot.begin()->size()); // number of spectra
    } else {
      return nWorkspaces;
    }
  }();

  // If user has selected a large number, check if they want to plot that many
  if (nSubplots > REASONABLE_NUM_SUBPLOTS) {
    const auto &answer = QMessageBox::question(
        appWindow(), "MantidPlot",
        "Are you sure you want to plot " + QString::number(nSubplots) +
            " subplots?",
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
    if (answer != QMessageBox::Ok) {
      return nullptr;
    }
  }

  // Get title. If this is a wsGroup, use name of that
  QString plotTitle = getSelectedGroupName();
  if (plotTitle.isEmpty()) {
    plotTitle = toPlot.constBegin().key();
  }

  // Do we plot as distribution?
  const bool plotAsDistribution = distr == MantidQt::DistributionDefault
                                      ? appWindow()->autoDistribution1D
                                      : distr == MantidQt::DistributionTrue;

  // Set the wait cursor while we are plotting
  ScopedOverrideCursor waitCursor;

  // Create window with correct number of layers, or use existing
  MultiLayer *multi;
  if (plotWindow) {
    multi = plotWindow;
    multi->setLayersNumber(0); // remove any existing plots
    multi->setLayersNumber(nSubplots);
  } else {
    multi = appWindow()->multilayerPlot(plotTitle, nSubplots, 1, nSubplots);
  }
  assert(multi);
  multi->setCloseOnEmpty(true);
  multi->arrangeLayers(true, true);

  QStringList legends; // Legends for each plot
  legends.reserve(nSubplots);
  int row(0), col(0);
  if (nWorkspaces == 1) {
    // One workspace, each spectrum in its own subplot
    const auto &wsName = toPlot.begin().key();
    const auto &spectra = toPlot.begin().value();
    for (const auto &spec : spectra) {
      const std::set<int> spectraSet{spec};
      plotLayerOfMultilayer(multi, errs, plotAsDistribution, row, col, wsName,
                            spectraSet);
      legends.append(getLegendKeys(wsName, spectraSet));
    }
  } else {
    // Each workspace in its own subplot
    for (auto iter = toPlot.constBegin(); iter != toPlot.constEnd(); ++iter) {
      const auto &wsName = iter.key();
      const auto &spectra = iter.value();
      plotLayerOfMultilayer(multi, errs, plotAsDistribution, row, col, wsName,
                            spectra);
      legends.append(getLegendKeys(wsName, spectra));
    }
  }

  multi->setCommonAxisScales();
  multi->arrangeLayers(true, true);

  // add legends last of all, so they are in the correct place
  for (int index = 0; index < multi->layers(); ++index) {
    auto *layer = multi->layer(index + 1); // MultiLayer has 1-based indices
    layer->newLegend(legends[index]);
  }

  // Check if window does not contain any curves and should be closed
  multi->maybeNeedToClose();

  return multi;
}

/**
 * Plot a single layer of a multilayer plot.
 * Data comes from the specified spectra of the named workspace.
 *
 * @param multi :: [input, output] Multilayer to plot graph onto
 * @param plotErrors :: [input] Whether to plot errors
 * @param plotDist :: [input] Whether to plot as distribution
 * @param row :: [input, output] Row counter - will be incremented
 * @param col :: [input, output] Column counter - will be incremented
 * @param wsName :: [input] Workspace name to plot from
 * @param spectra :: [input] Spectra from workspace to plot in the layer
 */
void MantidUI::plotLayerOfMultilayer(MultiLayer *multi, const bool plotErrors,
                                     const bool plotDist, int &row, int &col,
                                     const QString &wsName,
                                     const std::set<int> &spectra) {
  const int nRows = multi->getRows();
  const int nCols = multi->getCols();
  const int nPlots = multi->layers();

  // Lambda to increment row, column counters
  const auto incrementCounters = [&nRows, &nCols](int &row, int &col) {
    if (col < nCols - 1) {
      ++col;
    } else if (row < nRows - 1) {
      col = 0;
      ++row;
    }
  };

  // Lambda to set axis label hiding
  const auto formatAxes = [&nRows, &nCols, &nPlots](Graph *layer, const int row,
                                                    const int col) {
    const bool drawYAxisLabel = col == 0;
    if (!drawXAxisLabel(row, col, nRows, nCols, nPlots)) {
      layer->setXAxisTitle(QString::null);
    }
    if (!drawYAxisLabel) {
      layer->setYAxisTitle(QString::null);
    }
  };

  const bool isFitResult = workspaceIsFitResult(wsName);

  const int layerIndex = row * nCols + col + 1; // layers numbered from 1
  auto *layer = multi->layer(layerIndex);
  for (const int spec : spectra) {
    const auto plotType = isFitResult ? getCurveTypeForFitResult(spec)
                                      : GraphOptions::Unspecified;
    layer->insertCurve(wsName, spec, plotErrors, plotType, plotDist);
  }
  m_appWindow->setPreferences(layer); // apply default style
  layer->removeTitle();
  setInitialAutoscale(layer);
  formatAxes(layer, row, col);
  incrementCounters(row, col);
}

/**
 * Plot a "tiled" plot (with subplots).
 * Ask user for confirmation if lots of plots are chosen.
 * If just one workspace, put each spectrum in its own subplot
 * If multiple workspaces, each ws gets its own subplot
 * @param toPlot :: A list of workspace/spectra to be shown in the graph
 * @param distr :: if true, workspace plot as distribution (y data/bin width)
 * @param errs :: if true, plot the errors on the graph
 * @param plotWindow :: Window to plot in - if null, create a new one
 * @return created MultiLayer, or null on failure
 */
MultiLayer *MantidUI::plotSubplots(const QMultiMap<QString, int> &toPlot,
                                   MantidQt::DistributionFlag distr, bool errs,
                                   MultiLayer *plotWindow) {

  // Convert the input map into a map of workspace->spectra
  QMultiMap<QString, std::set<int>> spectraByWorkspace;
  for (auto it = toPlot.constBegin(); it != toPlot.constEnd(); ++it) {
    if (spectraByWorkspace.contains(it.key())) { // append to existing entry
      auto entry = spectraByWorkspace.find(it.key());
      entry.value().insert(it.value());
    } else { // add a new entry
      spectraByWorkspace.insert(it.key(), std::set<int>{it.value()});
    }
  }

  // Pass over to the overloaded method
  return plotSubplots(spectraByWorkspace, distr, errs, plotWindow);
}

/**
 * Plot a "tiled" plot (with subplots).
 * Ask user for confirmation if lots of plots are chosen.
 * If just one workspace, put each spectrum in its own subplot
 * If multiple workspaces, each ws gets its own subplot
 *
 * This overload plots the same spectra for each workspace.
 *
 * @param wsNames :: A list of workspace names to be shown in the graph
 * @param indexList :: list of workspace indices
 * @param distr :: if true, workspace plot as distribution (y data/bin width)
 * @param errs :: if true, plot the errors on the graph
 * @param plotWindow :: Window to plot in - if null, create a new one
 * @return created MultiLayer, or null on failure
 */
MultiLayer *MantidUI::plotSubplots(const QStringList &wsNames,
                                   const QList<int> &indexList,
                                   MantidQt::DistributionFlag distr, bool errs,
                                   MultiLayer *plotWindow) {
  // convert input into map of workspace->spectra
  QMultiMap<QString, std::set<int>> spectraByWorkspace;
  const std::set<int> wsIndices = [&indexList]() {
    std::set<int> indexSet;
    for (const auto &index : indexList) {
      indexSet.insert(index);
    }
    return indexSet;
  }();
  for (const auto &wsName : wsNames) {
    spectraByWorkspace.insert(wsName, wsIndices);
  }

  // Pass to the overloaded method
  return plotSubplots(spectraByWorkspace, distr, errs, plotWindow);
}

Table *MantidUI::createTableFromBins(
    const QString &wsName, Mantid::API::MatrixWorkspace_const_sptr workspace,
    const QList<int> &bins, bool errs, int fromRow, int toRow) {
  if (bins.empty())
    return nullptr;

  int c = errs ? 2 : 1;
  int numRows = static_cast<int>(workspace->getNumberHistograms());

  int j0 = fromRow >= 0 ? fromRow : 0;
  int j1 = toRow >= 0 ? toRow : numRows - 1;

  if (j0 >= numRows || j1 >= numRows)
    return nullptr;

  Table *t = new Table(appWindow()->scriptingEnv(), numRows,
                       c * bins.size() + 1, "", appWindow(), nullptr);
  appWindow()->initTable(t, appWindow()->generateUniqueName(wsName + "-"));

  for (int i = 0; i < bins.size(); i++) {
    const int kY = c * i + 1;
    int kErr = 0;
    t->setColName(kY, "YB" + QString::number(bins[i]));
    if (errs) {
      kErr = 2 * i + 2;
      t->setColPlotDesignation(kErr, Table::yErr);
      t->setColName(kErr, "EB" + QString::number(bins[i]));
    }
    for (int j = j0; j <= j1; j++) {
      const auto &dataY = workspace->y(j);
      const auto &dataE = workspace->e(j);

      if (i == 0) {
        // Get the X axis values from the vertical axis of the workspace
        if (workspace->axes() > 1)
          t->setCell(j, 0, (*workspace->getAxis(1))(j));
        else
          t->setCell(j, 0, j);
      }
      t->setCell(j, kY, dataY[bins[i]]);
      if (errs)
        t->setCell(j, kErr, dataE[bins[i]]);
    }
  }
  return t;
}

Table *MantidUI::createTableFromSelectedColumns(MantidMatrix *m, bool errs) {
  const QList<int> &cols = m->getSelectedColumns();
  if (cols.empty())
    return nullptr;

  int j0 = m->workspaceIndex(0);
  int j1 = m->workspaceIndex(m->numRows() - 1);

  return createTableFromBins(m->name(), m->workspace(), cols, errs, j0, j1);
}

/** Saves data to  nexus file
 * @param wsName :: Name of the workspace to be saved
 * @param fileName :: name of the nexus file to created
 */
void MantidUI::savedatainNexusFormat(const std::string &fileName,
                                     const std::string &wsName) {
  auto inputWorkspace =
      AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);

  // Typically, we use SaveNexusProcessed to save a workspace
  QString algorithm = "SaveNexusProcessed";

  //...but if it's an MD workspace, we use SaveMD instead
  if (boost::dynamic_pointer_cast<const IMDEventWorkspace>(inputWorkspace) ||
      boost::dynamic_pointer_cast<const IMDHistoWorkspace>(inputWorkspace))
    algorithm = "SaveMD";

  try {
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm(algorithm);
    alg->setPropertyValue("Filename", fileName);
    alg->setPropertyValue("InputWorkspace", wsName);
    executeAlgorithmAsync(alg, true /* wait for completion */);
  } catch (...) {
  }
}
/** Loads data from nexus file
 * @param wsName :: Name of the workspace to be created
 * @param fileName :: name of the nexus file
 */
void MantidUI::loadWSFromFile(const std::string &wsName,
                              const std::string &fileName) {
  if (fileName.empty())
    return;
  try {
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm("Load");
    alg->setPropertyValue("Filename", fileName);
    alg->setPropertyValue("OutputWorkspace", wsName);
    executeAlgorithmAsync(alg, true /* wait for completion */);
  } catch (...) {
  }
}

bool MantidUI::workspacesDockPlot1To1() {
  return Mantid::Kernel::ConfigService::Instance()
      .getValue<bool>("MantidOptions.ReusePlotInstances")
      .get_value_or(false);
}

/**
 * If a workspace group is selected, return its name
 * @returns :: Name of selected workspace group, or empty if no group selected
 */
QString MantidUI::getSelectedGroupName() const {
  auto selNames = m_exploreMantid->getSelectedWorkspaceNames();
  QString sel;

  if (selNames.size() > 0)
    sel = QString::fromStdString(selNames[0]);

  WorkspaceGroup_const_sptr gWs;
  if (!sel.isEmpty() &&
      AnalysisDataService::Instance().doesExist(sel.toStdString())) {
    try {
      gWs = boost::dynamic_pointer_cast<const WorkspaceGroup>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              sel.toStdString()));
    } catch (std::exception &) {
      // can happen, nothing to worry about
      gWs = WorkspaceGroup_const_sptr(); // make sure, anyway
    }
  }
  if (gWs) {
    return sel;
  } else {
    return "";
  }
}

/**
 * Sets graph to encompass all data points, then resets user's plot preference
 *
 * The 'setAutoScale' method is used to make sure that the plot initially
 * encompasses all the data points. However, this has the side-effect suggested
 * by its name: all the axes become auto-scaling if the data changes. If, in the
 * plot preferences, autoscaling has been disabled, then the axes are re-fixed.
 *
 * @param graph :: [input, output] Graph to set initial autoscale for
 */
void MantidUI::setInitialAutoscale(Graph *graph) {
  // Set axes to include all data points
  graph->setAutoScale();
  // If option disabled, re-fix the axes
  if (!appWindow()->autoscale2DPlots)
    graph->enableAutoscaling(false);
}

//=========================================================================
//
// This section defines some stuff that is only used on Windows
//
//=========================================================================
#ifdef _WIN32

struct mem_block {
  SIZE_T size;
  int state;
};

///  Assess the virtual memory of the current process.
void countVirtual(vector<mem_block> &mem, int &total) {

  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memStatus);

  MEMORY_BASIC_INFORMATION info;

  char *addr = 0;
  size_t free = 0;      // total free space
  size_t reserved = 0;  // total reserved space
  size_t committed = 0; // total committed (used) space
  size_t size = 0;
  size_t free_max = 0;      // maximum contiguous block of free memory
  size_t reserved_max = 0;  // maximum contiguous block of reserved memory
  size_t committed_max = 0; // maximum contiguous block of committed memory

  size_t GB2 =
      memStatus.ullTotalVirtual; // Maximum memory available to the process
  total = static_cast<int>(GB2);

  // Loop over all virtual memory to find out the status of every block.
  do {
    VirtualQuery(addr, &info, sizeof(MEMORY_BASIC_INFORMATION));

    int state = 0;
    if (info.State == MEM_FREE) {
      free += info.RegionSize;
      if (info.RegionSize > free_max)
        free_max = info.RegionSize;
      state = 0;
    }
    if (info.State == MEM_RESERVE) {
      reserved += info.RegionSize;
      if (info.RegionSize > reserved_max)
        reserved_max = info.RegionSize;
      state = 500;
    }
    if (info.State == MEM_COMMIT) {
      committed += info.RegionSize;
      if (info.RegionSize > committed_max)
        committed_max = info.RegionSize;
      state = 1000;
    }

    addr += info.RegionSize;
    size += info.RegionSize;

    mem_block b = {info.RegionSize, state};
    mem.push_back(b);

    /*cerr<<"BaseAddress = "<< info.BaseAddress<<'\n';
    cerr<<"AllocationBase = "<< info.AllocationBase<<'\n';
    cerr<<"AllocationProtect = "<< info.AllocationProtect<<'\n';
    cerr<<"RegionSize = "<< hex << info.RegionSize<<'\n';
    cerr<<"State = "<< state_str(info.State)<<'\n';
    cerr<<"Protect = "<< hex << info.Protect <<' '<<
    protect_str(info.Protect)<<'\n';
    cerr<<"Type = "<< hex << info.Type<<'\n';*/

  } while (size < GB2);

  std::cerr << "count FREE = " << std::dec << double(free) / 1024 / 1024
            << '\n';
  std::cerr << "count RESERVED = " << double(reserved) / 1024 / 1024 << '\n';
  std::cerr << "count COMMITTED = " << double(committed) / 1024 / 1024 << '\n';

  std::cerr << "max FREE = " << std::dec << double(free_max) / 1024 / 1024
            << '\n';
  std::cerr << "max RESERVED = " << double(reserved_max) / 1024 / 1024 << '\n';
  std::cerr << "max COMMITTED = " << double(committed_max) / 1024 / 1024
            << '\n';
  std::cerr << '\n';
}

/// Shows 2D plot of current memory usage.
/// One point is 1K of memory. One row is 1M.
/// Red - used memory block, blue - free, green - reserved.
void MantidUI::memoryImage() {
  // ofstream ofil("memory.txt");
  vector<mem_block> mem;
  int total;
  countVirtual(mem, total);
  int colNum = 1024;
  int rowNum = total / 1024 / colNum;
  Matrix *m = appWindow()->newMatrix(rowNum, colNum);
  m->setCoordinates(0, colNum, 0, rowNum);
  int row = 0;
  int col = 0;
  QImage image(colNum, rowNum, QImage::Format_Mono);
  for (vector<mem_block>::iterator b = mem.begin(); b != mem.end(); ++b) {
    int n = b->size / 1024;
    for (int i = 0; i < n; i++) {
      m->setCell(row, col, b->state);
      // ofil<<b->state<<'\t';
      col++;
      if (col >= colNum) {
        col = 0;
        row++;
        // ofil<<'\n';
      }
    }
  }
  appWindow()->plotSpectrogram(m, GraphOptions::ColorMap);
}

void MantidUI::memoryImage2() {
  // ofstream ofil("memory.txt");
  vector<mem_block> mem;
  int total;
  countVirtual(mem, total);
  int colNum = 1024;
  int rowNum = total / 1024 / colNum;
  int row = 0;
  int col = 0;
  QImage image(colNum, rowNum, QImage::Format_Mono);
  for (vector<mem_block>::iterator b = mem.begin(); b != mem.end(); ++b) {
    int n = b->size / 1024;
    for (int i = 0; i < n; i++) {
      if (row < rowNum && col < colNum) {
        image.setPixel(col, row, b->state > 600);
      }
      // ofil<<b->state<<'\t';
      col++;
      if (col >= colNum) {
        col = 0;
        row++;
        // ofil<<'\n';
      }
    }
  }
  image.save("memory_image.jpg");
}

#endif
  //=======================================================================
  // End of Windows specific stuff
  //=======================================================================

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

void MantidUI::test() {
  std::cerr << "\nTest\n\n";

  MatrixWorkspace_const_sptr ws =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(
          getSelectedWorkspace());
  if (ws) {
    boost::shared_ptr<const Mantid::Geometry::Instrument> instr =
        ws->getInstrument()->baseInstrument();
    boost::shared_ptr<Mantid::Geometry::CompAssembly> both =
        boost::dynamic_pointer_cast<Mantid::Geometry::CompAssembly>(
            (*instr)[3]);
    if (both) {
      boost::shared_ptr<Mantid::Geometry::CompAssembly> first =
          boost::dynamic_pointer_cast<Mantid::Geometry::CompAssembly>(
              (*both)[0]);
      if (first) {
        static int i = 0;
        Mantid::Kernel::V3D u =
            i++ ? Mantid::Kernel::V3D(1, 0, 0) : Mantid::Kernel::V3D(0, 1, 0);
        Mantid::Kernel::Quat q(30, u);
        first->rotate(q);
        return;
      }
    }
  }
  std::cerr << "Failed...\n";
}

void MantidUI::updateRecentFilesList(const QString &fname) {
  m_appWindow->updateRecentFilesList(fname);
}

/**
 * Create a new MantidWSIndexDialog
 * @param flags :: [input] Qt::WindowFlags enum as an integer
 * @param wsNames :: [input] Names of workspaces
 * @param showWaterfall :: [input] Whether to show "plot as waterfall" option
 * @param showPlotAll :: [input] Whether to show "plot all" button
 * @param showTiledOpt :: [input] Whether to show "tiled plot" option
 * @param isAdvanced :: [input] Whether to do advanced plotting
 * @returns :: New dialog
 */
MantidWSIndexDialog *
MantidUI::createWorkspaceIndexDialog(int flags, const QStringList &wsNames,
                                     bool showWaterfall, bool showPlotAll,
                                     bool showTiledOpt, bool isAdvanced) {
  return new MantidWSIndexDialog(m_appWindow, static_cast<Qt::WFlags>(flags),
                                 wsNames, showWaterfall, showPlotAll,
                                 showTiledOpt, isAdvanced);
}

void MantidUI::plotContour(bool accepted, int plotIndex,
                           const QString &axisName, const QString &logName,
                           const std::set<double> &customLogValues,
                           const QList<QString> &workspaceNames) {
  auto workspaces = getWorkspacesFromAds(workspaceNames);
  auto plotter =
      Mantid::Kernel::make_unique<MantidSurfaceContourPlotGenerator>(this);
  plotter->plotContour(accepted, plotIndex, axisName, logName, customLogValues,
                       workspaces);
}

void MantidUI::plotSurface(bool accepted, int plotIndex,
                           const QString &axisName, const QString &logName,
                           const std::set<double> &customLogValues,
                           const QList<QString> &workspaceNames) {
  auto workspaces = getWorkspacesFromAds(workspaceNames);
  auto plotter =
      Mantid::Kernel::make_unique<MantidSurfaceContourPlotGenerator>(this);
  plotter->plotSurface(accepted, plotIndex, axisName, logName, customLogValues,
                       workspaces);
}

QWidget *MantidUI::getParent() { return m_appWindow; }

#ifdef MAKE_VATES
bool MantidUI::doesVatesSupportOpenGL() {
  return vtkPVDisplayInformation::SupportsOpenGLLocally();
}
#endif
