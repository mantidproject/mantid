//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/Indirect/IndirectDataReduction.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectCalibration.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectConvertToEnergy.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectDataReductionTab.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectDiagnostics.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectMoments.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSqw.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSymmetrise.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectTransmission.h"

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QUrl>


//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(IndirectDataReduction);
  }
}


namespace
{
  Mantid::Kernel::Logger g_log("IndirectDataReduction");
}


using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt;


/**
 * Default constructor for class. Initialises interface pointers to NULL values.
 * @param parent :: This is a pointer to the "parent" object in Qt, most likely the main MantidPlot window.
 */
IndirectDataReduction::IndirectDataReduction(QWidget *parent) :
  UserSubWindow(parent),
  m_instrument(""),
  m_settingsGroup("CustomInterfaces/IndirectDataReduction"),
  m_algRunner(new MantidQt::API::AlgorithmRunner(this)),
  m_changeObserver(*this, &IndirectDataReduction::handleDirectoryChange)
{
  // Signals to report load instrument algo result
  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));
}


/**
 * Destructor
 */
IndirectDataReduction::~IndirectDataReduction()
{
  // Make sure no algos are running after the window has been closed
  m_algRunner->cancelRunningAlgorithm();

  saveSettings();
}


/**
 * On user clicking the "help" button on the interface, directs their request to the relevant
 * interface's helpClicked() function.
 */
void IndirectDataReduction::helpClicked()
{
  MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Indirect_DataReduction"));
}


/**
 * Called when the user clicks the Python export button.
 */
void IndirectDataReduction::exportTabPython()
{
  QString tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex());
  m_tabs[tabName]->exportPythonScript();
}


/**
 * This is the function called when the "Run" button is clicked. It will call the relevent function
 * in the subclass.
 */
void IndirectDataReduction::runClicked()
{
  QString tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex());
  m_tabs[tabName]->runTab();
}


/**
 * Sets up Qt UI file and connects signals, slots.
 */
void IndirectDataReduction::initLayout()
{
  m_uiForm.setupUi(this);

  // Do not allow running until setup  and instrument laoding are done
  updateRunButton(false, "Loading UI", "Initialising user interface components...");

  // Create the tabs
  m_tabs["Energy Transfer"] = new IndirectConvertToEnergy(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loEnergyTransfer"));
  m_tabs["Calibration"] = new IndirectCalibration(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loCalibration"));
  m_tabs["Diagnostics"] = new IndirectDiagnostics(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loDiagnostics"));
  m_tabs["Transmission"] = new IndirectTransmission(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loTransmission"));
  m_tabs["Symmetrise"] = new IndirectSymmetrise(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loSymmetrise"));
  m_tabs["S(Q, w)"] = new IndirectSqw(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loSofQW"));
  m_tabs["Moments"] = new IndirectMoments(this, m_uiForm.twIDRTabs->findChild<QWidget *>("loMoments"));

  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  // Connect the Python export buton
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Run" button
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));

  // Reset the Run button state when the tab is changed
  connect(m_uiForm.twIDRTabs, SIGNAL(currentChanged(int)), this, SLOT(updateRunButton()));

  // Connect tab signals and run any setup code
  for(auto it = m_tabs.begin(); it != m_tabs.end(); ++it)
  {
    connect(it->second, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
    connect(it->second, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
    connect(it->second, SIGNAL(updateRunButton(bool, QString, QString)), this, SLOT(updateRunButton(bool, QString, QString)));
    connect(this, SIGNAL(newInstrumentConfiguration()), it->second, SIGNAL(newInstrumentConfiguration())),
    it->second->setupTab();
  }

  // Handle instrument configuration changes
  connect(m_uiForm.iicInstrumentConfiguration, SIGNAL(instrumentConfigurationUpdated(const QString &, const QString &, const QString &)),
          this, SLOT(instrumentSetupChanged(const QString &, const QString &, const QString &)));

  // Update the instrument configuration across the UI
  m_uiForm.iicInstrumentConfiguration->newInstrumentConfiguration();
}


/**
 * This function is ran after initLayout(), and runPythonCode is unavailable before this function
 * has run (because of the setup of the base class). For this reason, "setup" functions that require
 * Python scripts are located here.
 */
void IndirectDataReduction::initLocalPython()
{
  // select starting instrument
  readSettings();
}


/**
 * Called when any of the instrument configuration options are changed.
 *
 * Used to notify tabs that rely on the instrument config when the config changes.
 *
 * @param instrumentName Name of selected instrument
 * @param analyser Name of selected analyser bank
 * @param reflection Name of selected reflection mode
 */
void IndirectDataReduction::instrumentSetupChanged(const QString & instrumentName, const QString & analyser,
                                                   const QString & reflection)
{
  m_instWorkspace = loadInstrumentIfNotExist(instrumentName.toStdString(), analyser.toStdString(), reflection.toStdString());
  instrumentLoadingDone(m_instWorkspace == NULL);

  if(m_instWorkspace != NULL)
    emit newInstrumentConfiguration();
}


/**
 * Loads an empty instrument into a workspace and returns a pointer to it.
 *
 * If an analyser and reflection are supplied then the corresponding IPF is also loaded.
 * The workspace is not stored in ADS.
 *
 * @param instrumentName Name of the instrument to load
 * @param analyser Analyser being used (optional)
 * @param reflection Relection being used (optional)
 * @returns Pointer to instrument workspace
 */
Mantid::API::MatrixWorkspace_sptr IndirectDataReduction::loadInstrumentIfNotExist(std::string instrumentName,
    std::string analyser, std::string reflection)
{
  std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

  std::string parameterFilename = idfDirectory + instrumentName + "_Definition.xml";
  IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->initialize();
  loadAlg->setProperty("Filename", parameterFilename);
  loadAlg->setProperty("OutputWorkspace", "__IDR_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");

  // Load the IPF if given an analyser and reflection
  if(!analyser.empty() && !reflection.empty())
  {
    std::string ipfFilename = idfDirectory + instrumentName + "_" + analyser + "_" + reflection + "_Parameters.xml";
    IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->setChild(true);
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Filename", ipfFilename);
    loadParamAlg->setProperty("Workspace", instWorkspace);
    loadParamAlg->execute();
  }

  return instWorkspace;
}


/**
 * Gets details for the current instrument configuration.
 *
 * @return Map of information ID to value
 */
std::map<QString, QString> IndirectDataReduction::getInstrumentDetails()
{
  std::map<QString, QString> instDetails;

  std::string instrumentName = m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
  std::string analyser = m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString();
  std::string reflection = m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString();

  instDetails["instrument"] = QString::fromStdString(instrumentName);
  instDetails["analyser"] = QString::fromStdString(analyser);
  instDetails["reflection"] = QString::fromStdString(reflection);

  // List of values to get from IPF
  std::vector<std::string> ipfElements;
  ipfElements.push_back("analysis-type");
  ipfElements.push_back("spectra-min");
  ipfElements.push_back("spectra-max");
  ipfElements.push_back("efixed-val");
  ipfElements.push_back("peak-start");
  ipfElements.push_back("peak-end");
  ipfElements.push_back("back-start");
  ipfElements.push_back("back-end");
  ipfElements.push_back("rebin-default");
  ipfElements.push_back("cm-1-convert-choice");
  ipfElements.push_back("save-ascii-choice");
  ipfElements.push_back("fold-frames-choice");

  // In the IRIS IPF there is no fmica component
  if(instrumentName == "IRIS" && analyser == "fmica")
    analyser = "mica";

  if(m_instWorkspace == NULL)
    return instDetails;

  // Get the instrument
  auto instrument = m_instWorkspace->getInstrument();
  if(instrument == NULL)
    return instDetails;

  // Get the analyser component
  auto component = instrument->getComponentByName(analyser);

  // For each parameter we want to get
  for(auto it = ipfElements.begin(); it != ipfElements.end(); ++it)
  {
    try
    {
      std::string key = *it;

      QString value = getInstrumentParameterFrom(instrument, key);

      if(value.isEmpty() && component != NULL)
        QString value = getInstrumentParameterFrom(component, key);

      instDetails[QString::fromStdString(key)] = value;
    }
    // In the case that the parameter does not exist
    catch(Mantid::Kernel::Exception::NotFoundError &nfe)
    {
      UNUSED_ARG(nfe);
      g_log.warning() << "Could not find parameter " << *it << " in instrument " << instrumentName << std::endl;
    }
  }

  return instDetails;
}


/**
 * Gets a parameter from an instrument component as a string.
 *
 * @param comp Instrument component
 * @param param Parameter name
 * @return Value as QString
 */
QString IndirectDataReduction::getInstrumentParameterFrom(Mantid::Geometry::IComponent_const_sptr comp, std::string param)
{
  QString value;

  if(!comp->hasParameter(param))
    return "";

  // Determine it's type and call the corresponding get function
  std::string paramType = comp->getParameterType(param);

  if(paramType == "string")
    value = QString::fromStdString(comp->getStringParameter(param)[0]);

  if(paramType == "double")
    value = QString::number(comp->getNumberParameter(param)[0]);

  return value;
}


/**
 * Tasks to be carried out after an empty instument has finished loading
 */
void IndirectDataReduction::instrumentLoadingDone(bool error)
{
  if(error)
  {
    g_log.error("Instument loading failed! (this can be caused by having both direct and indirect interfaces open)");
    updateRunButton(false, "No Instrument", "No instrument is currently loaded.");
    return;
  }

  updateRunButton();
}


/**
 * Remove the Poco observer on the config service when the interfaces is closed.
 *
 * @param close CLose event (unused)
 */
void IndirectDataReduction::closeEvent(QCloseEvent* close)
{
  UNUSED_ARG(close);
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}


/**
 * Reloads settings if the default sata search or save directories have been changed.
 */
void IndirectDataReduction::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();

  if(key == "datasearch.directories" || key == "defaultsave.directory")
    readSettings();
}


/**
 * Read Qt settings for the interface.
 */
void IndirectDataReduction::readSettings()
{
  // Set values of m_dataDir and m_saveDir
  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_dataDir.replace(" ", "");
  if(m_dataDir.length() > 0)
    m_dataDir = m_dataDir.split(";", QString::SkipEmptyParts)[0];
  m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  QSettings settings;

  // Load settings for MWRunFile widgets
  //TODO
  /* settings.beginGroup(m_settingsGroup + "DataFiles"); */
  /* settings.setValue("last_directory", m_dataDir); */
  /* m_uiForm.ind_runFiles->readSettings(settings.group()); */
  /* m_uiForm.cal_leRunNo->readSettings(settings.group()); */
  /* m_uiForm.slice_inputFile->readSettings(settings.group()); */
  /* settings.endGroup(); */

  /* settings.beginGroup(m_settingsGroup + "ProcessedFiles"); */
  /* settings.setValue("last_directory", m_saveDir); */
  /* m_uiForm.ind_calibFile->readSettings(settings.group()); */
  /* m_uiForm.ind_mapFile->readSettings(settings.group()); */
  /* m_uiForm.slice_dsCalibFile->readSettings(settings.group()); */
  /* m_uiForm.moment_dsInput->readSettings(settings.group()); */
  /* m_uiForm.sqw_dsSampleInput->readSettings(settings.group()); */
  /* settings.endGroup(); */

  // Load the last used instrument
  settings.beginGroup(m_settingsGroup);

  QString instrumentName = settings.value("instrument-name", "").toString();
  if(!instrumentName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setInstrument(instrumentName);

  QString analyserName = settings.value("analyser-name", "").toString();
  if(!analyserName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setAnalyser(analyserName);

  QString reflectionName = settings.value("reflection-name", "").toString();
  if(!reflectionName.isEmpty())
    m_uiForm.iicInstrumentConfiguration->setReflection(reflectionName);

  settings.endGroup();
}


/**
 * Save settings to a persistent storage.
 */
void IndirectDataReduction::saveSettings()
{
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
 * Handles showing the manage directory dialog box.
 */
void IndirectDataReduction::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}


/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 *
 * @param message The message to display in the message box
 */
void IndirectDataReduction::showMessageBox(const QString& message)
{
  showInformationBox(message);
}


/**
 * Slot to allow setting the state of the Run button.
 *
 * @param enabled If the button is clickable
 * @param message Message shown on the button
 * @param tooltip Tooltip shown when hovering over button
 */
void IndirectDataReduction::updateRunButton(bool enabled, QString message, QString tooltip)
{
  m_uiForm.pbRun->setEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
}
