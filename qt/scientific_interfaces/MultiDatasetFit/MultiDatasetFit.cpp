// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiDatasetFit.h"
#include "MDFDataController.h"
#include "MDFPlotController.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include <QMessageBox>
#include <QSettings>
#include <QToolBar>

namespace {
// tool options pages
const int zoomToolPage = 0;
const int rangeToolPage = 1;
Mantid::Kernel::Logger g_log("MultiDatasetFit");

/// Copy parameter values into a table workspace that is ready for
/// plotting them against a dataset index.
void formatParametersForPlotting(const Mantid::API::IFunction &function,
                                 const std::string &parametersPropertyName) {

  const auto nDomains = function.getNumberDomains();

  if (nDomains < 2) {
    // Single domain fit: nothing to plot.
    return;
  }

  // function must be MultiDomainFunction by the logic of the interface.
  // If it's not the cast error will tell us about it.
  const auto &mdFunction =
      dynamic_cast<const Mantid::API::MultiDomainFunction &>(function);

  if (mdFunction.nFunctions() == 0) {
    // Fit button was hit by mistake? Nothing to do here.
    // A warning will be shown elsewhere.
    return;
  }

  assert(nDomains == mdFunction.nFunctions());

  auto table =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  auto col = table->addColumn("double", "Dataset");
  col->setPlotType(1); // X-values inplots

  // Add columns for parameters and their errors
  const auto &firstFun = *mdFunction.getFunction(0);
  for (size_t iPar = 0; iPar < firstFun.nParams(); ++iPar) {
    table->addColumn("double", firstFun.parameterName(iPar));
    table->addColumn("double", firstFun.parameterName(iPar) + "_Err");
  }

  // Fill in the columns
  table->setRowCount(nDomains);
  for (size_t iData = 0; iData < nDomains; ++iData) {
    Mantid::API::TableRow row = table->getRow(iData);
    row << static_cast<double>(iData);
    const auto &fun = *mdFunction.getFunction(iData);
    for (size_t iPar = 0; iPar < fun.nParams(); ++iPar) {
      row << fun.getParameter(iPar) << fun.getError(iPar);
    }
  }

  // Store the table in the ADS
  Mantid::API::AnalysisDataService::Instance().addOrReplace(
      parametersPropertyName + "_vs_dataset", table);
}

// Class to implement a scoped "enabler"
class FinallyEnable {
  QPushButton *m_button;

public:
  explicit FinallyEnable(QPushButton *control) : m_button(control) {}
  ~FinallyEnable() { m_button->setEnabled(true); }
};

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

// Register the class with the factory
DECLARE_SUBWINDOW(MultiDatasetFit)

/// Constructor
/// @param parent :: The parent widget
MultiDatasetFit::MultiDatasetFit(QWidget *parent)
    : UserSubWindow(parent), m_plotController(nullptr),
      m_dataController(nullptr), m_functionBrowser(nullptr),
      m_fitOptionsBrowser(nullptr), m_fitAllSettings(QMessageBox::No) {}

MultiDatasetFit::~MultiDatasetFit() {
  saveSettings();
  bool clearGuess = true;
  m_plotController->clear(clearGuess);
}

/// Initilize the layout.
void MultiDatasetFit::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.hSplitter->setStretchFactor(0, 0);
  m_uiForm.hSplitter->setStretchFactor(1, 1);
  m_uiForm.vSplitter->setStretchFactor(0, 0);
  m_uiForm.vSplitter->setStretchFactor(1, 1);

  QHeaderView *header = m_uiForm.dataTable->horizontalHeader();
  header->setResizeMode(0, QHeaderView::Stretch);
  header->setResizeMode(1, QHeaderView::Fixed);

  m_uiForm.btnRemove->setEnabled(false);

  connect(m_uiForm.btnFit, SIGNAL(clicked()), this, SLOT(fit()));

  m_dataController = new MDF::DataController(this, m_uiForm.dataTable);
  connect(m_dataController, SIGNAL(hasSelection(bool)), m_uiForm.btnRemove,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.btnAddWorkspace, SIGNAL(clicked()), m_dataController,
          SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove, SIGNAL(clicked()), m_dataController,
          SLOT(removeSelectedSpectra()));
  connect(m_uiForm.cbApplyRangeToAll, SIGNAL(toggled(bool)), m_dataController,
          SLOT(setFittingRangeGlobal(bool)));

  m_plotController = new MDF::PlotController(
      this, m_uiForm.plot, m_uiForm.dataTable, m_uiForm.cbPlotSelector,
      m_uiForm.btnPrev, m_uiForm.btnNext);
  connect(m_dataController, SIGNAL(dataTableUpdated()), m_plotController,
          SLOT(tableUpdated()));
  connect(m_dataController, SIGNAL(dataSetUpdated(int)), m_plotController,
          SLOT(updateRange(int)));
  connect(m_dataController, SIGNAL(dataTableUpdated()), this,
          SLOT(setLogNames()));
  connect(m_dataController, SIGNAL(dataTableUpdated()), this,
          SLOT(invalidateOutput()));
  connect(m_plotController, SIGNAL(fittingRangeChanged(int, double, double)),
          m_dataController, SLOT(setFittingRange(int, double, double)));
  connect(m_uiForm.cbShowDataErrors, SIGNAL(toggled(bool)), m_plotController,
          SLOT(showDataErrors(bool)));
  connect(m_uiForm.btnToVisibleRange, SIGNAL(clicked()), m_plotController,
          SLOT(resetRange()));
  connect(m_uiForm.btnToFittingRange, SIGNAL(clicked()), m_plotController,
          SLOT(zoomToRange()));
  connect(m_uiForm.cbPlotGuess, SIGNAL(toggled(bool)), m_plotController,
          SLOT(showGuessFunction(bool)));

  QSplitter *splitter = new QSplitter(Qt::Vertical, this);

  m_functionBrowser =
      new MantidQt::MantidWidgets::FunctionBrowser(nullptr, true);
  m_functionBrowser->setColumnSizes(100, 100, 45);
  splitter->addWidget(m_functionBrowser);
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this,
          SLOT(reset()));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this,
          SLOT(checkFittingType()));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this,
          SLOT(setParameterNamesForPlotting()));
  connect(m_functionBrowser,
          SIGNAL(parameterChanged(const QString &, const QString &)), this,
          SLOT(updateGuessFunction(const QString &, const QString &)));
  connect(m_plotController, SIGNAL(currentIndexChanged(int)), m_functionBrowser,
          SLOT(setCurrentDataset(int)));
  connect(m_dataController, SIGNAL(spectraRemoved(QList<int>)),
          m_functionBrowser, SLOT(removeDatasets(QList<int>)));
  connect(m_dataController, SIGNAL(spectraAdded(const QStringList &)),
          m_functionBrowser, SLOT(addDatasets(const QStringList &)));

  m_fitOptionsBrowser = new MantidQt::MantidWidgets::FitOptionsBrowser(
      nullptr,
      MantidQt::MantidWidgets::FitOptionsBrowser::SimultaneousAndSequential);
  connect(m_fitOptionsBrowser, SIGNAL(changedToSequentialFitting()), this,
          SLOT(setLogNames()));
  splitter->addWidget(m_fitOptionsBrowser);

  m_uiForm.browserLayout->addWidget(splitter);

  createPlotToolbar();

  // filters
  m_functionBrowser->installEventFilter(this);
  m_fitOptionsBrowser->installEventFilter(this);
  m_uiForm.plot->installEventFilter(this);
  m_uiForm.dataTable->installEventFilter(this);

  m_plotController->enableZoom();
  showInfo("Add some data, define fitting function");

  loadSettings();
}

/// Create the tool bar for the plot widget.
void MultiDatasetFit::createPlotToolbar() {
  // ----- Main tool bar --------
  auto toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16, 16));
  auto group = new QActionGroup(this);

  auto action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/zoom.png"));
  action->setCheckable(true);
  action->setChecked(true);
  action->setToolTip("Zooming tool");
  connect(action, SIGNAL(triggered()), this, SLOT(enableZoom()));
  group->addAction(action);

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/panning.png"));
  action->setCheckable(true);
  action->setToolTip("Panning tool");
  connect(action, SIGNAL(triggered()), this, SLOT(enablePan()));
  group->addAction(action);

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/range.png"));
  action->setCheckable(true);
  action->setToolTip("Set fitting range");
  connect(action, SIGNAL(triggered()), this, SLOT(enableRange()));
  group->addAction(action);

  toolBar->addActions(group->actions());
  toolBar->addSeparator();

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/export-plot.png"));
  action->setToolTip("Export current plot");
  connect(action, SIGNAL(triggered()), this, SLOT(exportCurrentPlot()));
  toolBar->addAction(action);

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/export-all-plots.png"));
  action->setToolTip("Export all plots");
  connect(action, SIGNAL(triggered()), this, SLOT(exportAllPlots()));
  toolBar->addAction(action);

  m_uiForm.horizontalLayout->insertWidget(3, toolBar);
}

/// Create a multi-domain function to fit all the spectra in the data table.
boost::shared_ptr<Mantid::API::IFunction>
MultiDatasetFit::createFunction() const {
  return m_functionBrowser->getGlobalFunction();
}

/// Fit the data sets sequentially if there are no global parameters.
void MultiDatasetFit::fitSequential() {
  try {

    /// disable button to avoid multiple fit click
    m_uiForm.btnFit->setEnabled(false);

    std::ostringstream input;

    int n = getNumberOfSpectra();
    for (int ispec = 0; ispec < n; ++ispec) {
      input << getWorkspaceName(ispec).toStdString() << ",i"
            << getWorkspaceIndex(ispec) << ";";
    }

    auto fun = m_functionBrowser->getFunction();
    auto fit =
        Mantid::API::AlgorithmManager::Instance().create("PlotPeakByLogValue");
    fit->initialize();
    fit->setPropertyValue("Function", fun->asString());
    fit->setPropertyValue("Input", input.str());
    auto range = getFittingRange(0);
    fit->setProperty("StartX", range.first);
    fit->setProperty("EndX", range.second);

    m_fitOptionsBrowser->copyPropertiesToAlgorithm(*fit);

    m_outputWorkspaceName =
        m_fitOptionsBrowser->getProperty("OutputWorkspace") + "_Workspaces";

    removeOldOutput();

    m_fitRunner.reset(new API::AlgorithmRunner());
    connect(m_fitRunner.get(), SIGNAL(algorithmComplete(bool)), this,
            SLOT(finishFit(bool)), Qt::QueuedConnection);

    m_fitRunner->startAlgorithm(fit);

  } catch (std::exception &e) {
    QString mess(e.what());
    const int maxSize = 500;
    if (mess.size() > maxSize) {
      mess = mess.mid(0, maxSize);
      mess += "...";
    }
    QMessageBox::critical(
        this, "Mantid - Error",
        QString("PlotPeakByLogValue failed:\n\n  %1").arg(mess));
    m_uiForm.btnFit->setEnabled(true);
  }
}

/// Fit the data simultaneously.
void MultiDatasetFit::fitSimultaneous() {
  try {
    m_uiForm.btnFit->setEnabled(false);
    auto fun = createFunction();
    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setProperty("Function", fun);
    fit->setPropertyValue("InputWorkspace", getWorkspaceName(0).toStdString());
    fit->setProperty("WorkspaceIndex", getWorkspaceIndex(0));
    auto range = getFittingRange(0);
    fit->setProperty("StartX", range.first);
    fit->setProperty("EndX", range.second);

    int n = getNumberOfSpectra();
    for (int ispec = 1; ispec < n; ++ispec) {
      std::string suffix = boost::lexical_cast<std::string>(ispec);
      fit->setPropertyValue("InputWorkspace_" + suffix,
                            getWorkspaceName(ispec).toStdString());
      fit->setProperty("WorkspaceIndex_" + suffix, getWorkspaceIndex(ispec));
      range = getFittingRange(ispec);
      fit->setProperty("StartX_" + suffix, range.first);
      fit->setProperty("EndX_" + suffix, range.second);
    }

    m_fitOptionsBrowser->copyPropertiesToAlgorithm(*fit);

    m_outputWorkspaceName = m_fitOptionsBrowser->getProperty("Output");
    if (m_outputWorkspaceName.isEmpty()) {
      m_outputWorkspaceName = "out";
      fit->setPropertyValue("Output", m_outputWorkspaceName.toStdString());
      m_fitOptionsBrowser->setProperty("Output", "out");
    }
    if (n == 1) {
      m_outputWorkspaceName += "_Workspace";
    } else {
      m_outputWorkspaceName += "_Workspaces";
    }

    removeOldOutput();

    m_fitRunner.reset(new API::AlgorithmRunner());
    connect(m_fitRunner.get(), SIGNAL(algorithmComplete(bool)), this,
            SLOT(finishFit(bool)), Qt::QueuedConnection);

    m_fitRunner->startAlgorithm(fit);

  } catch (std::exception &e) {
    QString mess(e.what());
    const int maxSize = 500;
    if (mess.size() > maxSize) {
      mess = mess.mid(0, maxSize);
      mess += "...";
    }
    QMessageBox::critical(this, "Mantid - Error",
                          QString("Fit failed:\n\n  %1").arg(mess));
    m_uiForm.btnFit->setEnabled(true);
  }
}

/// Run the fitting algorithm.
void MultiDatasetFit::fit() {
  if (!m_functionBrowser->hasFunction()) {
    QMessageBox::warning(this, "Mantid - Warning", "Function wasn't set.");
    return;
  }

  auto fittingType = m_fitOptionsBrowser->getCurrentFittingType();
  auto n = getNumberOfSpectra();
  if (n == 0) {
    QMessageBox::warning(this, "Mantid - Warning", "Data wasn't set.");
    return;
  }
  int fitAll = QMessageBox::Yes;

  if (fittingType == MantidWidgets::FitOptionsBrowser::Simultaneous || n == 1) {
    if (n > 20 && m_fitAllSettings == QMessageBox::No) {
      fitAll = QMessageBox::question(this, "Fit All?",
                                     "Are you sure you would like to fit " +
                                         QString::number(n) +
                                         " spectrum simultaneously?",
                                     QMessageBox::Yes, QMessageBox::No);

      if (fitAll == QMessageBox::Yes)
        m_fitAllSettings = QMessageBox::Yes;
    }
    if (fitAll == QMessageBox::Yes) {
      fitSimultaneous();
    }

  } else if (fittingType == MantidWidgets::FitOptionsBrowser::Sequential) {
    if (n > 100 && m_fitAllSettings == QMessageBox::No) {
      fitAll = QMessageBox::question(this, "Fit All?",
                                     "Are you sure you would like to fit " +
                                         QString::number(n) +
                                         " spectrum sequentially?",
                                     QMessageBox::Yes, QMessageBox::No);

      if (fitAll == QMessageBox::Yes)
        m_fitAllSettings = QMessageBox::Yes;
    }
    if (fitAll == QMessageBox::Yes) {
      fitSequential();
    }
  } else {
    throw std::logic_error(
        "Unrecognised fitting type. Only Normal and Sequential are accepted.");
  }
}

/// Get the workspace name of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
QString MultiDatasetFit::getWorkspaceName(int i) const {
  return m_dataController->getWorkspaceName(i);
}

/// Get the workspace index of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
int MultiDatasetFit::getWorkspaceIndex(int i) const {
  return m_dataController->getWorkspaceIndex(i);
}

/// Get the name of the output workspace
/// @param i :: Index of a spectrum in the data table.
QString MultiDatasetFit::getOutputWorkspaceName(int i) const {
  auto wsName = m_outputWorkspaceName.toStdString();
  if (!wsName.empty() &&
      Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
    if (auto group =
            boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws)) {
      wsName = group->getItem(i)->getName();
    }
  }
  return QString::fromStdString(wsName);
}

/// Get the fitting range for the i-th spectrum
/// @param i :: Index of a spectrum in the data table.
std::pair<double, double> MultiDatasetFit::getFittingRange(int i) const {
  return m_dataController->getFittingRange(i);
}

/// Get the number of spectra to fit to.
int MultiDatasetFit::getNumberOfSpectra() const {
  return m_dataController->getNumberOfSpectra();
}

/// Set the fit status info string after a fit is finished.
/// @param status :: The Fit's status property.
/// @param chiSquared :: The chi squared value returned by Fit.
void MultiDatasetFit::setFitStatusInfo(const QString &status,
                                       const QString &chiSquared) {
  auto text(status);
  text.replace("\n", "<br>");
  QString color("green");
  if (status != "success") {
    color = "red";
  }
  m_fitStatus = QString("Status: <span style='color:%2'>%1</span>"
                        "<br>Chi Squared: %4")
                    .arg(text, color, chiSquared);
  showInfo("");
}

/// Clear the fit status info string.
void MultiDatasetFit::clearFitStatusInfo() {
  m_fitStatus.clear();
  showInfo("");
}

/// Slot called on completion of the Fit algorithm.
/// @param error :: Set to true if Fit finishes with an error.
void MultiDatasetFit::finishFit(bool error) {
  FinallyEnable ensureEnabled(m_uiForm.btnFit);
  if (!error) {
    m_plotController->clear();
    m_plotController->update();
    Mantid::API::IFunction_sptr fun;
    auto algorithm = m_fitRunner->getAlgorithm();
    if (m_fitOptionsBrowser->getCurrentFittingType() ==
        MantidWidgets::FitOptionsBrowser::Simultaneous) {
      // After a simultaneous fit
      fun = algorithm->getProperty("Function");
      updateParameters(*fun);
      auto status =
          QString::fromStdString(algorithm->getPropertyValue("OutputStatus"));
      auto chiSquared = QString::fromStdString(
          algorithm->getPropertyValue("OutputChi2overDoF"));
      setFitStatusInfo(status, chiSquared);
      formatParametersForPlotting(
          *fun, algorithm->getPropertyValue("OutputParameters"));
    } else {
      // After a sequential fit
      auto paramsWSName =
          m_fitOptionsBrowser->getProperty("OutputWorkspace").toStdString();
      if (!Mantid::API::AnalysisDataService::Instance().doesExist(paramsWSName))
        return;
      size_t nSpectra = getNumberOfSpectra();
      if (nSpectra == 0)
        return;
      fun = m_functionBrowser->getGlobalFunction();
      auto nParams = fun->nParams() / nSpectra;
      auto params = Mantid::API::AnalysisDataService::Instance()
                        .retrieveWS<Mantid::API::ITableWorkspace>(paramsWSName);
      if (nParams * 2 + 2 != params->columnCount()) {
        throw std::logic_error(
            "Output table workspace has unexpected number of columns.");
      }
      for (size_t index = 0; index < nSpectra; ++index) {
        std::string prefix =
            "f" + boost::lexical_cast<std::string>(index) + ".";
        for (size_t ip = 0; ip < nParams; ++ip) {
          auto colIndex = ip * 2 + 1;
          auto column = params->getColumn(colIndex);
          fun->setParameter(prefix + column->name(), column->toDouble(index));
        }
      }
      updateParameters(*fun);
      showParameterPlot();
      clearFitStatusInfo();
    }
  }
  m_functionBrowser->setErrorsEnabled(!error);
}

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function from which to take the parameters.
void MultiDatasetFit::updateParameters(const Mantid::API::IFunction &fun) {
  m_functionBrowser->updateMultiDatasetParameters(fun);
}

/// Show a message in the info bar at the bottom of the interface.
void MultiDatasetFit::showInfo(const QString &text) {
  QString info(text);
  if (!m_fitStatus.isEmpty()) {
    info += "<br>" + m_fitStatus;
  }
  m_uiForm.infoBar->setText(info);
}

/// Intersept mouse-enter events to display context-specific info
/// in the "status bar".
bool MultiDatasetFit::eventFilter(QObject *widget, QEvent *evn) {
  if (evn->type() == QEvent::Enter) {
    if (qobject_cast<QObject *>(m_functionBrowser) == widget) {
      showFunctionBrowserInfo();
    } else if (qobject_cast<QObject *>(m_fitOptionsBrowser) == widget) {
      showFitOptionsBrowserInfo();
    } else if (qobject_cast<QObject *>(m_uiForm.plot) == widget) {
      showPlotInfo();
    } else if (qobject_cast<QObject *>(m_uiForm.dataTable) == widget) {
      showTableInfo();
    } else {
      showInfo("");
    }
  }
  return false;
}

/// Show info about the function browser.
void MultiDatasetFit::showFunctionBrowserInfo() {
  if (m_functionBrowser->hasFunction()) {
    showInfo("Use context menu to add more functions. Set parameters and "
             "attributes.");
  } else {
    showInfo("Use context menu to add a function.");
  }
}

/// Show info about the Fit options browser.
void MultiDatasetFit::showFitOptionsBrowserInfo() {
  showInfo("Set Fit properties.");
}

/// Show info / tips on the plot widget.
void MultiDatasetFit::showPlotInfo() {
  QString text = "Use Alt+. and Alt+, to change the data set. ";

  if (m_plotController->isZoomEnabled()) {
    text += "Click and drag to zoom in. Use middle or right button to zoom out";
  } else if (m_plotController->isPanEnabled()) {
    text += "Click and drag to move. Use mouse wheel to zoom in and out.";
  } else if (m_plotController->isRangeSelectorEnabled()) {
    text += "Drag the vertical dashed lines to adjust the fitting range.";
  }

  showInfo(text);
}

/// Show info / tips on the dataset table.
void MultiDatasetFit::showTableInfo() {
  if (getNumberOfSpectra() > 0) {
    showInfo("Select spectra by selecting rows. For multiple selection use "
             "Shift or Ctrl keys.");
  } else {
    showInfo("Add some data sets. Click \"Add Workspace\" button.");
  }
}

/// Check that the data sets in the table are valid and remove invalid ones.
void MultiDatasetFit::checkSpectra() { m_dataController->checkSpectra(); }

/// Enable the zoom tool.
void MultiDatasetFit::enableZoom() {
  m_plotController->enableZoom();
  m_uiForm.toolOptions->setCurrentIndex(zoomToolPage);
}

/// Enable the panning tool.
void MultiDatasetFit::enablePan() {
  m_plotController->enablePan();
  m_uiForm.toolOptions->setCurrentIndex(zoomToolPage);
}

/// Enable the fitting range selection tool.
void MultiDatasetFit::enableRange() {
  m_plotController->enableRange();
  m_uiForm.toolOptions->setCurrentIndex(rangeToolPage);
}

/// Export current plot
void MultiDatasetFit::exportCurrentPlot() {
  m_plotController->exportCurrentPlot();
}

/// Export all plots
void MultiDatasetFit::exportAllPlots() { m_plotController->exportAllPlots(); }

/// Set value of a local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
/// @param value :: New value for the parameter.
void MultiDatasetFit::setLocalParameterValue(const QString &parName, int i,
                                             double value) {
  m_functionBrowser->setLocalParameterValue(parName, i, value);
}

/// Get value of a local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
double MultiDatasetFit::getLocalParameterValue(const QString &parName,
                                               int i) const {
  return m_functionBrowser->getLocalParameterValue(parName, i);
}

/// Reset the caches. Prepare to fill them in lazily.
void MultiDatasetFit::reset() {
  m_functionBrowser->setNumberOfDatasets(getNumberOfSpectra());
  setParameterNamesForPlotting();
  try {
    m_plotController->setGuessFunction(m_functionBrowser->getFunctionString());
  } catch (const std::runtime_error &e) {
    m_plotController->setGuessFunction("");
    m_functionBrowser->clear();
    QMessageBox::warning(this, "Mantid - Warning", e.what());
  }
}

/// Check if a local parameter is fixed
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
bool MultiDatasetFit::isLocalParameterFixed(const QString &parName,
                                            int i) const {
  return m_functionBrowser->isLocalParameterFixed(parName, i);
}

/// Fix/unfix local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
/// @param fixed :: Should the parameter be fixed (true) or unfixed (false).
void MultiDatasetFit::setLocalParameterFixed(const QString &parName, int i,
                                             bool fixed) {
  m_functionBrowser->setLocalParameterFixed(parName, i, fixed);
}

/// Get the tie for a local parameter.
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
QString MultiDatasetFit::getLocalParameterTie(const QString &parName,
                                              int i) const {
  return m_functionBrowser->getLocalParameterTie(parName, i);
}

/// Set a tie for a local parameter.
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
/// @param tie :: A tie string to set.
void MultiDatasetFit::setLocalParameterTie(const QString &parName, int i,
                                           QString tie) {
  m_functionBrowser->setLocalParameterTie(parName, i, tie);
}

/// Load settings
void MultiDatasetFit::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->loadSettings(settings);
  bool option = settings.value("ShowDataErrors", false).toBool();
  m_uiForm.cbShowDataErrors->setChecked(option);
  option = settings.value("ApplyRangeToAll", false).toBool();
  m_uiForm.cbApplyRangeToAll->setChecked(option);
  option = settings.value("PlotGuess", false).toBool();
  m_uiForm.cbPlotGuess->setChecked(option);
}

/// Save settings
void MultiDatasetFit::saveSettings() const {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->saveSettings(settings);
  settings.setValue("ShowDataErrors", m_uiForm.cbShowDataErrors->isChecked());
  settings.setValue("ApplyRangeToAll", m_uiForm.cbApplyRangeToAll->isChecked());
  settings.setValue("PlotGuess", m_uiForm.cbPlotGuess->isChecked());
}

/// Make sure that simultaneous fitting is on
/// when the function has at least one global parameter.
void MultiDatasetFit::checkFittingType() {
  auto globals = m_functionBrowser->getGlobalParameters();
  if (globals.isEmpty()) {
    m_fitOptionsBrowser->unlockCurrentFittingType();
  } else {
    m_fitOptionsBrowser->lockCurrentFittingType(
        MantidWidgets::FitOptionsBrowser::Simultaneous);
  }
}

/**
 * Collect names of the logs in the data workspaces and pass them on to
 * m_fitOptionsBrowser.
 */
void MultiDatasetFit::setLogNames() {
  if (getNumberOfSpectra() > 0) {
    try {
      auto ws = Mantid::API::AnalysisDataService::Instance()
                    .retrieveWS<Mantid::API::MatrixWorkspace>(
                        getWorkspaceName(0).toStdString());
      const std::vector<Mantid::Kernel::Property *> logs =
          ws->run().getLogData();
      QStringList logNames;
      for (auto log : logs) {
        if (dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(log)) {
          logNames << QString::fromStdString(log->name());
        }
      }
      if (!logNames.isEmpty()) {
        m_fitOptionsBrowser->setLogNames(logNames);
      }
    } catch (...) { /*Maybe the data table hasn't updated yet*/
    }
  }
}

/// Collect names of local parameters and pass them to m_fitOptionsBrowser.
void MultiDatasetFit::setParameterNamesForPlotting() {
  m_fitOptionsBrowser->setParameterNamesForPlotting(
      m_functionBrowser->getLocalParameters());
}

/// Remove old output from Fit.
void MultiDatasetFit::removeOldOutput() {
  auto outWS = m_outputWorkspaceName.toStdString();
  auto &ADS = Mantid::API::AnalysisDataService::Instance();
  boost::shared_ptr<Mantid::API::WorkspaceGroup> group;
  auto nSpectra = static_cast<size_t>(getNumberOfSpectra());
  if (ADS.doesExist(outWS) &&
      (group = ADS.retrieveWS<Mantid::API::WorkspaceGroup>(outWS)) &&
      group->size() > nSpectra) {
    // If size of output group decreases the extra workspaces will pop out
    // to the top level. Remove them.
    outWS.erase(outWS.size() - 1);
    for (auto i = nSpectra; i < group->size(); ++i) {
      auto wsName = outWS + "_" + std::to_string(i);
      ADS.remove(wsName);
    }
  }
}

/// Invalidate the previous fit output
void MultiDatasetFit::invalidateOutput() {
  m_outputWorkspaceName = "";
  m_plotController->clear();
  m_plotController->update();
}

/// Open a new graph window and plot a fitting parameter
/// against a log value. The name of the parameter to plot
/// and the log name must be selected in m_fitOptionsBrowser.
void MultiDatasetFit::showParameterPlot() {
  auto table = m_fitOptionsBrowser->getProperty("OutputWorkspace");
  auto parName = m_fitOptionsBrowser->getParameterToPlot();
  if (table.isEmpty() || parName.isEmpty())
    return;

  auto pyInput = QString("table = importTableWorkspace('%1')\n"
                         "plotTableColumns(table, ('%2','%3_Err'))\n")
                     .arg(table, parName, parName);
  runPythonCode(pyInput);
}

void MultiDatasetFit::updateGuessFunction(const QString & /*unused*/,
                                          const QString & /*unused*/) {
  auto fun = m_functionBrowser->getFunction();
  auto composite =
      boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (composite && composite->nFunctions() == 1) {
    fun = composite->getFunction(0);
  }
  m_plotController->updateGuessFunction(*fun);
}

/// Log a warning
/// @param msg :: A warning message to log.
void MultiDatasetFit::logWarning(const std::string &msg) { g_log.warning(msg); }

} // namespace CustomInterfaces
} // namespace MantidQt
