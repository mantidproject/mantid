// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "StepScan.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidJson/Json.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"
#include "boost/python.hpp"
#include <QtGlobal>

#include <QFileInfo>
#include <QUrl>

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

#include <json/reader.h>

namespace MantidQt {
using API::HelpWindow;

namespace CustomInterfaces {

// Register the class with the factory
DECLARE_SUBWINDOW(StepScan)

using namespace Mantid::Kernel;
using namespace Mantid::API;

/// Constructor
StepScan::StepScan(QWidget *parent)
    : UserSubWindow(parent), m_instrument(ConfigService::Instance().getInstrument().name()),
      m_algRunner(new API::QtAlgorithmRunner(this)), m_addObserver(*this, &StepScan::handleAddEvent),
      m_replObserver(*this, &StepScan::handleReplEvent), m_replaceObserverAdded(false) {}

StepScan::~StepScan() {
  // Stop any async algorithm
  m_algRunner->cancelRunningAlgorithm();
  // Stop live data collection, if running
  m_uiForm.mWRunFiles->stopLiveAlgorithm();
  // Disconnect the observers for the mask workspace combobox
  AnalysisDataService::Instance().notificationCenter.removeObserver(m_addObserver);
  AnalysisDataService::Instance().notificationCenter.removeObserver(m_replObserver);
  // Clean up any hidden workspaces created
  cleanupWorkspaces();
}

/// Set up the dialog layout
void StepScan::initLayout() {
  m_uiForm.setupUi(this);

  // I couldn't see a way to set a validator on a qlineedit in designer
  m_uiForm.xmin->setValidator(new QDoubleValidator(m_uiForm.xmin));
  m_uiForm.xmax->setValidator(new QDoubleValidator(m_uiForm.xmax));

  setWindowTitle(windowTitle() + " - " + QString::fromStdString(m_instrument));

  connect(m_uiForm.mWRunFiles, SIGNAL(liveButtonPressed(bool)), SLOT(triggerLiveListener(bool)), Qt::QueuedConnection);

  connect(m_uiForm.launchInstView, SIGNAL(clicked()), SLOT(launchInstrumentWindow()));

  connect(m_uiForm.mWRunFiles, SIGNAL(filesFound()), SLOT(loadFile()));
  connect(this, SIGNAL(logsAvailable(const Mantid::API::MatrixWorkspace_const_sptr &)),
          SLOT(fillPlotVarCombobox(const Mantid::API::MatrixWorkspace_const_sptr &)));

  connect(m_uiForm.helpButton, SIGNAL(clicked()), SLOT(helpClicked()));
  connect(m_uiForm.startButton, SIGNAL(clicked()), SLOT(runStepScanAlg()));
  if (this->parent()) {
    // note connection to the parent window, otherwise an empty frame window
    // may remain open and visible after this close.
    connect(m_uiForm.closeButton, SIGNAL(released()), this->parent(), SLOT(close()));
  } else {
    // In MantidWorkbench this->parent() returns NULL. Connecting to
    // this->close() appears to work.
    connect(m_uiForm.closeButton, SIGNAL(released()), this, SLOT(close()));
  }
}

void StepScan::cleanupWorkspaces() {
  if (!m_inputWSName.empty()) {
    // Get a reference to the analysis data service
    auto &ADS = AnalysisDataService::Instance();
    // Clean up, checking first that those that may not exist do (to avoid a
    // warning in the log)
    ADS.remove(m_inputWSName);
    const std::string monitorWSName = m_inputWSName + "_monitors";
    if (ADS.doesExist(monitorWSName))
      ADS.remove(monitorWSName);
    m_inputWSName.clear();
    if (ADS.doesExist(m_plotWSName))
      ADS.remove(m_plotWSName);
    m_plotWSName.clear();
    disconnect(SIGNAL(logsUpdated(const Mantid::API::MatrixWorkspace_const_sptr &)));
  }

  m_uiForm.startButton->setEnabled(false);
  m_uiForm.launchInstView->setEnabled(false);
  m_uiForm.plotVariable->setEnabled(false);
  // Disconnect anything listening to the comboboxes
  m_uiForm.plotVariable->disconnect(SIGNAL(currentIndexChanged(const QString &)));
  m_uiForm.normalization->disconnect(SIGNAL(currentIndexChanged(const QString &)));
}

/** Slot that is called when the live data button is clicked
 *  @param checked Whether the button is being enabled (true) or disabled
 */
void StepScan::triggerLiveListener(bool checked) {
  if (checked) {
    startLiveListener();
  } else {
    m_uiForm.mWRunFiles->stopLiveAlgorithm();
    cleanupWorkspaces();
  }
}

void StepScan::startLiveListener() {
  if (!LiveListenerFactory::Instance().create(m_instrument, false)->buffersEvents()) {
    QMessageBox::critical(this, "Invalid live stream",
                          "This interface requires event data.\nThe live data for " +
                              QString::fromStdString(m_instrument) + " is in histogram form");
    m_uiForm.mWRunFiles->liveButtonSetChecked(false);
    return;
  }

  // Remove any previously-loaded workspaces
  cleanupWorkspaces();

  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), SLOT(startLiveListenerComplete(bool)));

  IAlgorithm_sptr startLiveData = AlgorithmManager::Instance().create("StartLiveData");
  startLiveData->setProperty("UpdateEvery", 5.0);
  startLiveData->setProperty("FromNow", false);
  startLiveData->setProperty("FromStartOfRun", true);
  startLiveData->setProperty("Instrument", m_instrument);
  m_inputWSName = "__live";
  startLiveData->setProperty("OutputWorkspace", m_inputWSName);
  if (!startLiveData->validateInputs().empty()) {
    QMessageBox::critical(this, "StartLiveData failed", "Unable to start live data collection");
    m_uiForm.mWRunFiles->liveButtonSetChecked(false);
    return;
  }
  m_uiForm.mWRunFiles->setLiveAlgorithm(startLiveData);
  m_algRunner->startAlgorithm(startLiveData);
}

void StepScan::startLiveListenerComplete(bool error) {
  disconnect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(startLiveListenerComplete(bool)));
  if (!error) {
    // Keep track of the algorithm that's pulling in the live data
    m_uiForm.mWRunFiles->setLiveAlgorithm(m_algRunner->getAlgorithm()->getProperty("MonitorLiveData"));

    setupOptionControls();

    addReplaceObserverOnce();
    connect(this, SIGNAL(logsUpdated(const Mantid::API::MatrixWorkspace_const_sptr &)),
            SLOT(expandPlotVarCombobox(const Mantid::API::MatrixWorkspace_const_sptr &)));
  } else {
    QMessageBox::critical(this, "StartLiveData failed", "Unable to start live data collection");
    m_uiForm.mWRunFiles->liveButtonSetChecked(false);
  }
}

void StepScan::loadFile(bool async) {
  const QString filename = m_uiForm.mWRunFiles->getUserInput().toString();
  // This handles the fact that mwRunFiles emits the filesFound signal more than
  // we want (on some platforms). TODO: Consider dealing with this up in
  // mwRunFiles.
  if (filename != m_inputFilename && m_uiForm.mWRunFiles->isValid()) {
    m_inputFilename = filename;

    // Remove any previously-loaded workspaces
    cleanupWorkspaces();

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Load");
    try {
      alg->setPropertyValue("Filename", filename.toStdString());
      if (m_uiForm.mWRunFiles->getFilenames().size() == 1) {
        m_inputWSName = "__" + QFileInfo(filename).baseName().toStdString();
      } else {
        m_inputWSName = "__multifiles";
      }
      alg->setPropertyValue("OutputWorkspace", m_inputWSName);
      alg->setProperty("LoadMonitors", true);
    } catch (std::exception &) // Have to catch at this level as different
                               // exception types can happen
    {
      QMessageBox::warning(this, "File loading failed", "Is this an event nexus file?");
      return;
    }

    m_uiForm.statusText->setText("<i><font color='darkblue'>Loading data...</font></i>");

    if (async) {
      connect(m_algRunner, SIGNAL(algorithmComplete(bool)), SLOT(loadFileComplete(bool)));
      m_algRunner->startAlgorithm(alg);
    } else {
      alg->execute();
      loadFileComplete(!alg->isExecuted());
    }
  }
}

void StepScan::loadFileComplete(bool error) {
  m_uiForm.statusText->clear();
  disconnect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(loadFileComplete(bool)));

  if (m_inputWSName == "__multifiles" && !error)
    error = mergeRuns();

  if (!error) {
    setupOptionControls();
  } else {
    QMessageBox::warning(this, "File loading failed", "Is this an event nexus file?");
  }
}

namespace {
class ScopedStatusText {
public:
  ScopedStatusText(QLabel *label, const QString &labelText) : status_label(label) {
    status_label->setText("<i><font color='darkblue'>" + labelText + "</font></i>");
  }

  ~ScopedStatusText() { status_label->clear(); }

private:
  QLabel *const status_label;
};

// Small class to handle disabling mouse clicks and showing the busy cursor in
// an RAII manner.
// Used in the runStepScanAlg below to ensure these things are unset when the
// method is exited.
class DisableGUI_RAII {
public:
  explicit DisableGUI_RAII(StepScan *gui) : the_gui(gui) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    the_gui->setAttribute(Qt::WA_TransparentForMouseEvents);
  }

  ~DisableGUI_RAII() {
    QApplication::restoreOverrideCursor();
    the_gui->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  }

private:
  StepScan *const the_gui;
};
} // namespace

bool StepScan::mergeRuns() {
  ScopedStatusText _merging(this->m_uiForm.statusText, "Merging runs...");
  // This can be slow and will lock the GUI, but will probably be so rarely used
  // that it's
  // not worth making it asynchronous
  // Block mouse clicks while the algorithm runs. Also set the busy cursor.
  DisableGUI_RAII _blockclicks(this);

  // Get hold of the group workspace and go through the entries adding an
  // incrementing scan_index variable
  WorkspaceGroup_const_sptr wsGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(m_inputWSName);
  if (!wsGroup)
    return true; // Shouldn't be possible, but be defensive

  for (size_t i = 0; i < wsGroup->size(); ++i) {
    // Add a scan_index variable to each workspace, counting from 1
    MatrixWorkspace_sptr ws = std::static_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));
    if (!ws)
      return true; // Again, shouldn't be possible (unless there's a group
                   // within a group?)
    IAlgorithm_sptr addScanIndex = AlgorithmManager::Instance().create("AddSampleLog");
    addScanIndex->setPropertyValue("Workspace", ws->getName());
    addScanIndex->setProperty("LogName", "scan_index");
    addScanIndex->setProperty("LogType", "Number Series");
    addScanIndex->setProperty("LogText", Strings::toString(i + 1));
    auto result = addScanIndex->executeAsync();
    while (!result.available()) {
      QApplication::processEvents();
    }
    if (!addScanIndex->isExecuted())
      return true;

    // Add a scan_index = 0 to the end time for each workspace
    try {
      ws->run().getTimeSeriesProperty<int>("scan_index")->addValue(ws->run().endTime(), 0);
    } catch (std::runtime_error &) {
      /* Swallow the error if there's no run end time. It shouldn't happen for
       * real data. */
    }
  }

  IAlgorithm_sptr merge = AlgorithmManager::Instance().create("MergeRuns");
  merge->setPropertyValue("InputWorkspaces", m_inputWSName);
  const std::string summedWSName = "__summed_multifiles";
  merge->setPropertyValue("OutputWorkspace", summedWSName);
  auto result = merge->executeAsync();
  while (!result.available()) {
    QApplication::processEvents();
  }
  if (!merge->isExecuted())
    return true;
  m_inputWSName = summedWSName;

  return false;
}

void StepScan::setupOptionControls() {
  MatrixWorkspace_const_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWSName);
  // Trigger population of the logs combobox
  emit logsAvailable(outWS);
  fillNormalizationCombobox();
  // Enable the button to launch the instrument view (for defining a mask)
  m_uiForm.launchInstView->setEnabled(true);
}

void StepScan::launchInstrumentWindow() {
  // Gotta do this in python
  std::string pyCode = "from mantidqt.widgets.instrumentview.api import get_instrumentview\n"
                       "instrument_view = get_instrumentview('" +
                       m_inputWSName +
                       "')\n"
                       "instrument_view.select_tab(2)\n"
                       "instrument_view.show_view()";
  Mantid::PythonInterface::GlobalInterpreterLock lock;
  PyRun_SimpleString(pyCode.c_str());

  // Attach the observers so that if a mask workspace is generated over in the
  // instrument view,
  // it is automatically selected by the combobox over here
  AnalysisDataService::Instance().notificationCenter.addObserver(m_addObserver);
  addReplaceObserverOnce();
}

void StepScan::fillPlotVarCombobox(const MatrixWorkspace_const_sptr &ws) {
  // Hold the name of the scan index log in a common place
  const std::string scan_index("scan_index");
  // If this has already been set to something, keep track of what
  auto currentSetting = m_uiForm.plotVariable->currentText();
  // Clear the combobox and immediately re-insert 'scan_index' (so it's the
  // first entry)
  m_uiForm.plotVariable->clear();
  m_uiForm.plotVariable->addItem(QString::fromStdString(scan_index));

  // First check that the provided workspace has the scan_index - complain if it
  // doesn't
  try {
    auto scan_index_prop = ws->run().getTimeSeriesProperty<int>(scan_index);
    if (!m_uiForm.mWRunFiles->liveButtonIsChecked() && scan_index_prop->realSize() < 2) {
      QMessageBox::warning(this, "scan_index log empty", "This data does not appear to be an alignment scan");
      return;
    }
  } catch (std::exception &) {
    QMessageBox::warning(this, "scan_index log not found", "Is this an ADARA-style dataset?");
    return;
  }

  expandPlotVarCombobox(ws);

  // Set back to whatever it was set to before
  m_uiForm.plotVariable->setCurrentIndex(m_uiForm.plotVariable->findText(currentSetting));
  // Now that this has been populated, allow the user to select from it
  m_uiForm.plotVariable->setEnabled(true);
  // Now's the time to enable the start button as well
  m_uiForm.startButton->setEnabled(true);
}

void StepScan::expandPlotVarCombobox(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
  // This is unfortunately more or less a copy of
  // SumEventsByLogValue::getNumberSeriesLogs
  // but I want to populate the box before running the algorithm
  const auto &logs = ws->run().getLogData();
  for (auto log : logs) {
    const QString logName = QString::fromStdString(log->name());
    // Don't add scan_index - that's already there
    if (logName == "scan_index")
      continue;
    // Try to cast to an ITimeSeriesProperty
    auto tsp = dynamic_cast<const ITimeSeriesProperty *>(log);
    // Move on to the next one if this is not a TSP
    if (tsp == nullptr)
      continue;
    // Don't keep ones with only one entry
    if (tsp->realSize() < 2)
      continue;
    // Now make sure it's either an int or double tsp
    if (dynamic_cast<TimeSeriesProperty<double> *>(log) || dynamic_cast<TimeSeriesProperty<int> *>(log)) {
      // Add it to the list if it isn't already there
      if (m_uiForm.plotVariable->findText(logName) == -1) {
        m_uiForm.plotVariable->addItem(logName);
      }
    }
  }
}

void StepScan::fillNormalizationCombobox() {
  clearNormalizationCombobox();

  // Add the monitors to the normalization combobox
  const auto inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWSName);

  try {
    const auto monWS = inputWS->monitorWorkspace();
    const auto &monitorSpectrumInfo = monWS->spectrumInfo();

    if (monWS) {
      for (std::size_t i = 0; i < monWS->getNumberHistograms(); ++i) {
        const std::string monitorName = monitorSpectrumInfo.detector(i).getName();
        m_uiForm.normalization->addItem(QString::fromStdString(monitorName));
      }
    }
  } catch (Exception::NotFoundError &) {
    // No monitors workspace....carry on
  }
}

void StepScan::clearNormalizationCombobox() {
  // If there are more than 3 entries in the combobox (nothing, time,
  // proton_charge) then
  // remove any stale ones
  while (m_uiForm.normalization->count() > 3) {
    m_uiForm.normalization->removeItem(m_uiForm.normalization->count() - 1);
  }
}

IAlgorithm_sptr StepScan::setupStepScanAlg() {
  IAlgorithm_sptr stepScan = AlgorithmManager::Instance().create("StepScan");
  // The table should not be hidden, so leave off the prefix
  m_tableWSName = m_inputWSName.substr(2) + "_StepScan";
  stepScan->setPropertyValue("OutputWorkspace", m_tableWSName);

  // ROI masking
  const QString maskWS = m_uiForm.maskWorkspace->currentText();
  stepScan->setPropertyValue("MaskWorkspace", maskWS.toStdString());

  // Filtering on time (or other unit)
  const QString xminStr = m_uiForm.xmin->text();
  const QString xmaxStr = m_uiForm.xmax->text();
  const double xmin = xminStr.toDouble();
  const double xmax = xmaxStr.toDouble();
  // If both set, check that xmax > xmin
  if (!xminStr.isEmpty() && !xmaxStr.isEmpty() && xmin >= xmax) {
    QMessageBox::critical(this, "Invalid filtering range set", "For the filtering range, min has to be less than max");
    return IAlgorithm_sptr();
  }
  if (!xminStr.isEmpty())
    stepScan->setProperty("XMin", xmin);
  if (!xmaxStr.isEmpty())
    stepScan->setProperty("XMax", xmax);
  switch (m_uiForm.rangeUnit->currentIndex()) {
  case 1:
    stepScan->setProperty("RangeUnit", "dSpacing");
    break;
  default:
    // The default value for the property is TOF (which is index 0 in the
    // combobox)
    break;
  }

  return stepScan;
}

void StepScan::runStepScanAlg() {
  IAlgorithm_sptr stepScan = setupStepScanAlg();
  if (!stepScan)
    return;

  // Block mouse clicks while the algorithm runs. Also set the busy cursor.
  DisableGUI_RAII _blockclicks(this);

  bool algSuccessful;
  if (m_uiForm.mWRunFiles->liveButtonIsChecked()) // Live data
  {
    algSuccessful = runStepScanAlgLive(stepScan->toString());
  } else // Offline data
  {
    // Check just in case the user has deleted the loaded workspace
    if (!AnalysisDataService::Instance().doesExist(m_inputWSName)) {
      m_inputFilename.clear();
      loadFile(false);
    }
    stepScan->setPropertyValue("InputWorkspace", m_inputWSName);
    ScopedStatusText _merging(this->m_uiForm.statusText, "Analyzing scan...");
    auto result = stepScan->executeAsync();
    while (!result.available()) {
      QApplication::processEvents();
    }
    algSuccessful = stepScan->isExecuted();
  }

  if (!algSuccessful) {
    return;
  }

  // Now that the algorithm's been run, connect up the signal to change the plot
  // variable
  connect(m_uiForm.plotVariable, SIGNAL(currentIndexChanged(const QString &)), SLOT(generateCurve(const QString &)));
  // and the one if the normalisation's been changed
  connect(m_uiForm.normalization, SIGNAL(currentIndexChanged(const QString &)), SLOT(updateForNormalizationChange()));
  // Create the plot for the first time
  generateCurve(m_uiForm.plotVariable->currentText());
}

bool StepScan::runStepScanAlgLive(const std::string &stepScanProperties) {
  // First stop the currently running live algorithm
  IAlgorithm_const_sptr oldMonitorLiveData = m_uiForm.mWRunFiles->stopLiveAlgorithm();

  Json::Value root;

  bool parsingSuccessful = Mantid::JsonHelpers::parse(stepScanProperties, &root);
  if (!parsingSuccessful) {
    throw std::runtime_error("Parsing parameters failed for StepScan.");
  }
  Json::Value &prop = root["properties"];
  if (prop.isNull()) {
    throw std::runtime_error("Parsing parameters failed for StepScan.");
  }
  std::string ssp = prop.toStyledString();

  IAlgorithm_sptr startLiveData = AlgorithmManager::Instance().create("StartLiveData");
  startLiveData->setProperty("Instrument", m_instrument);
  startLiveData->setProperty("FromNow", false);
  startLiveData->setProperty("FromStartOfRun", true);
  startLiveData->setProperty("UpdateEvery", 10.0);
  startLiveData->setProperty("PreserveEvents", true);
  startLiveData->setProperty("PostProcessingAlgorithm", "StepScan");
  startLiveData->setProperty("PostProcessingProperties", ssp);
  startLiveData->setProperty("RunTransitionBehavior", "Stop");
  startLiveData->setProperty("AccumulationWorkspace", m_inputWSName);
  startLiveData->setProperty("OutputWorkspace", m_tableWSName);
  // The previous listener needs to finish before this one can start
  while (oldMonitorLiveData->isRunning()) {
    Poco::Thread::sleep(100);
  }
  auto result = startLiveData->executeAsync();
  while (!result.available()) {
    QApplication::processEvents();
  }
  if (!startLiveData->isExecuted())
    return false;

  // Keep track of the algorithm that's pulling in the live data
  m_uiForm.mWRunFiles->setLiveAlgorithm(startLiveData->getProperty("MonitorLiveData"));

  connect(this, SIGNAL(updatePlot(const QString &)), SLOT(generateCurve(const QString &)));
  return true;
}

void StepScan::updateForNormalizationChange() { generateCurve(m_uiForm.plotVariable->currentText()); }

void StepScan::generateCurve(const QString &var) {
  if (!AnalysisDataService::Instance().doesExist(m_tableWSName)) {
    QMessageBox::critical(this, "Unable to generate plot",
                          "Table workspace " + QString::fromStdString(m_tableWSName) + "\nhas been deleted!");
    return;
  }

  // Create a matrix workspace out of the variable that's asked for
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ConvertTableToMatrixWorkspace");
  alg->setLogging(false); // Don't log this algorithm
  alg->setPropertyValue("InputWorkspace", m_tableWSName);
  m_plotWSName = m_tableWSName + "_plot";
  alg->setPropertyValue("OutputWorkspace", m_plotWSName);
  alg->setPropertyValue("ColumnX", var.toStdString());
  alg->setPropertyValue("ColumnY", "Counts");
  alg->setPropertyValue("ColumnE", "Error");
  if (!alg->execute())
    return;

  // Now create one for the normalisation, if required
  if (m_uiForm.normalization->currentIndex() != 0) {
    IAlgorithm_sptr norm = AlgorithmManager::Instance().create("ConvertTableToMatrixWorkspace");
    norm->setChild(true);
    norm->setLogging(false); // Don't log this algorithm
    norm->setPropertyValue("InputWorkspace", m_tableWSName);
    norm->setPropertyValue("OutputWorkspace", "dummyName");
    norm->setPropertyValue("ColumnX", var.toStdString());
    // TODO: Protect against column being missing (e.g. if monitor not found in
    // data)
    norm->setPropertyValue("ColumnY", m_uiForm.normalization->currentText().toStdString());
    if (!norm->execute())
      return;

    MatrixWorkspace_sptr top = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_plotWSName);
    MatrixWorkspace_sptr bottom = norm->getProperty("OutputWorkspace");
    top /= bottom;
    AnalysisDataService::Instance().addOrReplace(m_plotWSName, top);
  }

  plotCurve();
}

namespace {
auto get_fig_ax(std::optional<int> fignum) {
  std::string pyCode = "import matplotlib.pyplot as plt\n"
                       "from mantid import plots\n"
                       "from workbench.plotting.globalfiguremanager import GlobalFigureManager\n"
                       "if GlobalFigureManager.has_fignum(fig_num):\n"
                       "    fig = plt.figure(fig_num)\n"
                       "    ax = plt.gca()\n"
                       "    ax.clear()\n"
                       "else:\n"
                       "    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})";
  Mantid::PythonInterface::GlobalInterpreterLock lock;
  using namespace MantidQt::Widgets::Common;
  using namespace MantidQt::Widgets::MplCpp;
  Python::Object main_module = boost::python::import("__main__");
  Python::Object main_namespace = main_module.attr("__dict__");
  if (fignum) {
    main_namespace["fig_num"] = fignum.value();
  } else {
    main_namespace["fig_num"] = Python::Object();
  }
  boost::python::exec(pyCode.c_str(), main_namespace);
  auto fig = Figure(boost::python::extract<Python::Object>(main_namespace["fig"]));
  auto ax = MantidAxes(boost::python::extract<Python::Object>(main_namespace["ax"]));
  return std::make_tuple(fig, ax);
}
} // namespace

void StepScan::plotCurve() {
  // Get the name of the dataset to produce the plot title
  std::string title = m_inputWSName.substr(2);
  // qtiplot may unhelpfully change '_' to '-' so I need to as well
  std::replace(title.begin(), title.end(), '_', '-');

  // Figure out the axis titles
  const std::string xAxisTitle = m_uiForm.plotVariable->currentText().toStdString();
  std::string yAxisTitle = "Counts";
  const std::string normalization = m_uiForm.normalization->currentText().toStdString();
  if (normalization == "nothing") /* Do nothing */
    ;
  else if (normalization == "time")
    yAxisTitle += " / second";
  else if (normalization == "proton_charge")
    yAxisTitle += " / picocoulomb";
  else
    yAxisTitle += " / " + normalization;

  auto [fig, ax] = get_fig_ax(m_fignum);
  m_fignum = fig.number();
  auto ws = AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(m_plotWSName);
  title += " - Step Scan";
  fig.setWindowTitle(title.c_str());
  QHash<QString, QVariant> hash;
  hash.insert("linestyle", "");
  hash.insert("marker", ".");
  ax.plot(ws, 0, "black", "", hash);
  ax.setXLabel(xAxisTitle.c_str());
  ax.setYLabel(yAxisTitle.c_str());
  fig.show();
  this->activateWindow();
  this->raise();
}

void StepScan::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf) {
  checkForMaskWorkspace(pNf->objectName());
}

void StepScan::handleReplEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf) {
  checkForMaskWorkspace(pNf->objectName());
  checkForResultTableUpdate(pNf->objectName());
  checkForVaryingLogs(pNf->objectName());
}

void StepScan::addReplaceObserverOnce() {
  if (!m_replaceObserverAdded) {
    AnalysisDataService::Instance().notificationCenter.addObserver(m_replObserver);
    m_replaceObserverAdded = true;
  }
}

void StepScan::checkForMaskWorkspace(const std::string &wsName) {
  if (wsName == "MaskWorkspace") {
    // Make sure the combobox has picked up the new workspace
    m_uiForm.maskWorkspace->refresh();
    // Now set it to point at the mask workspace
    const int index = m_uiForm.maskWorkspace->findText("MaskWorkspace");
    if (index != -1)
      m_uiForm.maskWorkspace->setCurrentIndex(index);
  }
}

void StepScan::checkForResultTableUpdate(const std::string &wsName) {
  if (wsName == m_tableWSName) {
    emit updatePlot(m_uiForm.plotVariable->currentText());
  }
}

void StepScan::checkForVaryingLogs(const std::string &wsName) {
  if (wsName == m_inputWSName) {
    MatrixWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWSName);
    emit logsUpdated(ws);
  }
}

void StepScan::helpClicked() {
  HelpWindow::showCustomInterface(QStringLiteral("Step Scan Analysis"), QString("general"));
}

} // namespace CustomInterfaces
} // namespace MantidQt
