// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "IndirectDataReduction.h"

#include "ILLEnergyTransfer.h"
#include "ISISCalibration.h"
#include "ISISDiagnostics.h"
#include "ISISEnergyTransfer.h"
#include "IndirectMoments.h"
#include "IndirectSqw.h"
#include "IndirectSymmetrise.h"
#include "IndirectTransmission.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <QDir>
#include <QMessageBox>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt;

namespace {
Mantid::Kernel::Logger g_log("IndirectDataReduction");
}

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectDataReduction)

IndirectDataReduction::IndirectDataReduction(QWidget *parent)
    : IndirectInterface(parent), m_settingsGroup("CustomInterfaces/IndirectDataReduction"),
      m_algRunner(new MantidQt::API::AlgorithmRunner(this)),
      m_changeObserver(*this, &IndirectDataReduction::handleConfigChange), m_ipfFilename(""), m_instDetails() {
  // Signals to report load instrument algo result
  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));

  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);
}

IndirectDataReduction::~IndirectDataReduction() {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);

  // Make sure no algos are running after the window has been closed
  m_algRunner->cancelRunningAlgorithm();

  saveSettings();
}

std::string IndirectDataReduction::documentationPage() const { return "Indirect Data Reduction"; }

/**
 * Called when the user clicks the Python export button.
 */
void IndirectDataReduction::exportTabPython() {
  QString tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex());
  m_tabs[tabName].second->exportPythonScript();
}

/**
 * Sets up Qt UI file and connects signals, slots.
 */
void IndirectDataReduction::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(IndirectSettings::icon());

  // Create the tabs
  addTab<ISISEnergyTransfer>("ISIS Energy Transfer");
  addTab<ISISCalibration>("ISIS Calibration");
  addTab<ISISDiagnostics>("ISIS Diagnostics");
  addTab<IndirectTransmission>("Transmission");
  addTab<IndirectSymmetrise>("Symmetrise");
  addTab<IndirectSqw>("S(Q, w)");
  addTab<IndirectMoments>("Moments");
  addTab<ILLEnergyTransfer>("ILL Energy Transfer");

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  // Connect the Python export buton
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  // Handle instrument configuration changes
  connect(m_uiForm.iicInstrumentConfiguration,
          SIGNAL(instrumentConfigurationUpdated(const QString &, const QString &, const QString &)), this,
          SLOT(instrumentSetupChanged(const QString &, const QString &, const QString &)));

  auto const facility = Mantid::Kernel::ConfigService::Instance().getFacility();
  filterUiForFacility(QString::fromStdString(facility.name()));

  // Update the instrument configuration across the UI
  readSettings();
  m_uiForm.iicInstrumentConfiguration->updateInstrumentConfigurations(
      m_uiForm.iicInstrumentConfiguration->getInstrumentName());

  // Needed to initially apply the settings loaded on the settings GUI
  applySettings(getInterfaceSettings());
}

void IndirectDataReduction::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

/**
 * This function is ran after initLayout(), and runPythonCode is unavailable
 * before this function
 * has run (because of the setup of the base class). For this reason, "setup"
 * functions that require
 * Python scripts are located here.
 */
void IndirectDataReduction::initLocalPython() {
  // select starting instrument
  readSettings();
}

/**
 * Called when any of the instrument configuration options are changed.
 *
 * Used to notify tabs that rely on the instrument config when the config
 *changes.
 *
 * @param instrumentName Name of selected instrument
 * @param analyser Name of selected analyser bank
 * @param reflection Name of selected reflection mode
 */
void IndirectDataReduction::instrumentSetupChanged(const QString &instrumentName, const QString &analyser,
                                                   const QString &reflection) {
  loadInstrumentIfNotExist(instrumentName.toStdString(), analyser.toStdString(), reflection.toStdString());
  instrumentLoadingDone(m_instWorkspace == nullptr);

  if (m_instWorkspace != nullptr)
    emit newInstrumentConfiguration();
}

/**
 * Gets the current instrument workspace. It will load the instrument workspace
 * if there is currently nothing loaded
 *
 * @returns Pointer to instrument workspace
 */
MatrixWorkspace_sptr IndirectDataReduction::instrumentWorkspace() {
  if (!m_instWorkspace)
    loadInstrumentIfNotExist(m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString(),
                             m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString(),
                             m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString());
  return m_instWorkspace;
}

/**
 * Loads an empty instrument into a workspace and returns a pointer to it.
 *
 * If an analyser and reflection are supplied then the corresponding IPF is also
 *loaded.
 * The workspace is not stored in ADS.
 *
 * @param instrumentName Name of the instrument to load
 * @param analyser Analyser being used (optional)
 * @param reflection Relection being used (optional)
 */
void IndirectDataReduction::loadInstrumentIfNotExist(const std::string &instrumentName, const std::string &analyser,
                                                     const std::string &reflection) {
  auto const idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  auto const ipfFilename = idfDirectory + instrumentName + "_" + analyser + "_" + reflection + "_Parameters.xml";

  if (ipfFilename != m_ipfFilename) {
    try {
      auto const dateRange = instrumentName == "BASIS" ? "_2014-2018" : "";
      auto const parameterFilename = idfDirectory + instrumentName + "_Definition" + dateRange + ".xml";
      auto loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
      loadAlg->setChild(true);
      loadAlg->setLogging(false);
      loadAlg->initialize();
      loadAlg->setProperty("Filename", parameterFilename);
      loadAlg->setProperty("OutputWorkspace", "__IDR_Inst");
      loadAlg->execute();
      MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");

      // Load the IPF if given an analyser and reflection
      if (!analyser.empty() && !reflection.empty()) {
        m_ipfFilename = ipfFilename;
        auto loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
        loadParamAlg->setChild(true);
        loadParamAlg->setLogging(false);
        loadParamAlg->initialize();
        loadParamAlg->setProperty("Filename", m_ipfFilename);
        loadParamAlg->setProperty("Workspace", instWorkspace);
        loadParamAlg->execute();
      }

      m_instWorkspace = instWorkspace;
      loadInstrumentDetails();

    } catch (std::exception const &ex) {
      g_log.warning() << "Failed to load instrument with error: " << ex.what()
                      << ". The current facility may not be fully "
                         "supported.\n";
      m_instWorkspace = MatrixWorkspace_sptr();
    }
  }
}

/**
 * Gets the details for the current instrument configuration.
 *
 * @return Map of information ID to value
 */
QMap<QString, QString> IndirectDataReduction::getInstrumentDetails() {
  if (m_instDetails.isEmpty())
    loadInstrumentDetails();
  return m_instDetails;
}

/**
 * Loads the details for the current instrument configuration.
 *
 */
void IndirectDataReduction::loadInstrumentDetails() {
  m_instDetails.clear();

  std::string instrumentName = m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
  std::string analyser = m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString();
  std::string reflection = m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString();

  m_instDetails["instrument"] = QString::fromStdString(instrumentName);
  m_instDetails["analyser"] = QString::fromStdString(analyser);
  m_instDetails["reflection"] = QString::fromStdString(reflection);

  // List of values to get from IPF
  std::vector<std::string> ipfElements{
      "analysis-type",     "spectra-min",       "spectra-max",        "Efixed",        "peak-start",
      "peak-end",          "back-start",        "back-end",           "rebin-default", "cm-1-convert-choice",
      "save-nexus-choice", "save-ascii-choice", "fold-frames-choice", "resolution"};

  // In the IRIS IPF there is no fmica component
  if (instrumentName == "IRIS" && analyser == "fmica")
    analyser = "mica";

  // Get the instrument
  auto const instrument = instrumentWorkspace()->getInstrument();
  if (instrument == nullptr)
    g_log.warning("Instrument workspace has no instrument");

  // Get the analyser component
  auto component = instrument->getComponentByName(analyser);

  // For each parameter we want to get
  for (auto &ipfElement : ipfElements) {
    try {
      std::string key = ipfElement;

      QString value = getInstrumentParameterFrom(instrument, key);

      if (value.isEmpty() && component != nullptr)
        value = getInstrumentParameterFrom(component, key);

      m_instDetails[QString::fromStdString(key)] = value;
    }
    // In the case that the parameter does not exist
    catch (Mantid::Kernel::Exception::NotFoundError &nfe) {
      UNUSED_ARG(nfe);
      g_log.warning() << "Could not find parameter " << ipfElement << " in instrument " << instrumentName << '\n';
    }
  }
}

/**
 * Gets a parameter from an instrument component as a string.
 *
 * @param comp Instrument component
 * @param param Parameter name
 * @return Value as QString
 */
QString IndirectDataReduction::getInstrumentParameterFrom(const Mantid::Geometry::IComponent_const_sptr &comp,
                                                          const std::string &param) {
  QString value;

  if (!comp->hasParameter(param)) {
    g_log.debug() << "Component " << comp->getName() << " has no parameter " << param << '\n';
    return "";
  }

  // Determine it's type and call the corresponding get function
  std::string paramType = comp->getParameterType(param);

  if (paramType == "string")
    value = QString::fromStdString(comp->getStringParameter(param)[0]);

  if (paramType == "double")
    value = QString::number(comp->getNumberParameter(param)[0]);

  return value;
}

/**
 * Tasks to be carried out after an empty instument has finished loading
 */
void IndirectDataReduction::instrumentLoadingDone(bool error) {
  if (error) {
    g_log.warning("Instument loading failed! This instrument (or "
                  "analyser/reflection configuration) may not be supported by "
                  "the interface.");
    return;
  }
}

/**
 * Remove the Poco observer on the config service when the interfaces is closed.
 *
 * @param close Close event (unused)
 */
void IndirectDataReduction::closeEvent(QCloseEvent *close) {
  UNUSED_ARG(close);
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles configuration values being changed.
 *
 * Currently checks for data search paths and default facility.
 *
 * @param pNf Poco notification
 */
void IndirectDataReduction::handleConfigChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();
  std::string value = pNf->curValue();

  if (key == "datasearch.directories" || key == "defaultsave.directory") {
    readSettings();
  } else if (key == "default.facility") {
    QString facility = QString::fromStdString(value);

    filterUiForFacility(facility);
    m_uiForm.iicInstrumentConfiguration->setFacility(facility);
  }
  // Update the instrument configuration across the UI
  m_uiForm.iicInstrumentConfiguration->updateInstrumentConfigurations(
      m_uiForm.iicInstrumentConfiguration->getInstrumentName());
}

/**
 * Read Qt settings for the interface.
 */
void IndirectDataReduction::readSettings() {
  // Set values of m_dataDir and m_saveDir
  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_dataDir.replace(" ", "");
  if (m_dataDir.length() > 0)
    m_dataDir = m_dataDir.split(";", QString::SkipEmptyParts)[0];
  m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  QSettings settings;

  // Load the last used instrument
  settings.beginGroup(m_settingsGroup);

  auto const instrumentName = settings.value("instrument-name", "").toString();
  if (!instrumentName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setInstrument(instrumentName);

  auto const analyserName = settings.value("analyser-name", "").toString();
  if (!analyserName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setAnalyser(analyserName);

  auto const reflectionName = settings.value("reflection-name", "").toString();
  if (!reflectionName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setReflection(reflectionName);

  settings.endGroup();
}

/**
 * Save settings to a persistent storage.
 */
void IndirectDataReduction::saveSettings() {
  QSettings settings;
  settings.beginGroup(m_settingsGroup);

  QString instrumentName = m_uiForm.iicInstrumentConfiguration->getInstrumentName();
  settings.setValue("instrument-name", instrumentName);

  QString analyserName = m_uiForm.iicInstrumentConfiguration->getAnalyserName();
  settings.setValue("analyser-name", analyserName);

  QString reflectionName = m_uiForm.iicInstrumentConfiguration->getReflectionName();
  settings.setValue("reflection-name", reflectionName);

  settings.endGroup();
}

/**
 * Filters the displayed tabs based on the current facility.
 *
 * @param facility Name of facility
 */
void IndirectDataReduction::filterUiForFacility(const QString &facility) {
  g_log.information() << "Facility selected: " << facility.toStdString() << '\n';
  QStringList enabledTabs;
  QStringList disabledInstruments;

  // Add facility specific tabs and disable instruments
  if (facility == "ISIS") {
    enabledTabs << "ISIS Energy Transfer"
                << "ISIS Calibration"
                << "ISIS Diagnostics";
  } else if (facility == "ILL") {
    enabledTabs << "ILL Energy Transfer";
    disabledInstruments << "IN10"
                        << "IN13"
                        << "IN16";
  }

  // These tabs work at any facility (always at end of tabs)
  enabledTabs << "Transmission"
              << "Symmetrise"
              << "S(Q, w)"
              << "Moments";

  // First remove all tabs
  while (m_uiForm.twIDRTabs->count() > 0) {
    // Disconnect the instrument changed signal
    QString tabName = m_uiForm.twIDRTabs->tabText(0);
    disconnect(this, SIGNAL(newInstrumentConfiguration()), m_tabs[tabName].second,
               SIGNAL(newInstrumentConfiguration()));

    // Remove the tab
    m_uiForm.twIDRTabs->removeTab(0);

    g_log.debug() << "Removing tab " << tabName.toStdString() << '\n';
  }

  // Add the required tabs
  for (auto &enabledTab : enabledTabs) {
    // Connect the insturment changed signal
    connect(this, SIGNAL(newInstrumentConfiguration()), m_tabs[enabledTab].second,
            SIGNAL(newInstrumentConfiguration()));

    // Add the tab
    m_uiForm.twIDRTabs->addTab(m_tabs[enabledTab].first, enabledTab);

    g_log.debug() << "Adding tab " << enabledTab.toStdString() << '\n';
  }

  // Disable instruments as required
  m_uiForm.iicInstrumentConfiguration->setDisabledInstruments(disabledInstruments);
}

} // namespace CustomInterfaces
} // namespace MantidQt
