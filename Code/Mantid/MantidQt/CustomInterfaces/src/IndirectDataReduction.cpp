//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDataReduction.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/IndirectCalibration.h"
#include "MantidQtCustomInterfaces/IndirectConvertToEnergy.h"
#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"
#include "MantidQtCustomInterfaces/IndirectMoments.h"
#include "MantidQtCustomInterfaces/IndirectSqw.h"
#include "MantidQtCustomInterfaces/IndirectSymmetrise.h"
#include "MantidQtCustomInterfaces/IndirectTransmission.h"

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

  QString tabName = m_uiForm.tabWidget->tabText(
      m_uiForm.tabWidget->currentIndex());

  QString url = "http://www.mantidproject.org/Indirect:";

  if ( tabName == "Energy Transfer" )
    url += "EnergyTransfer";
  else if ( tabName == "Calibration" )
    url += "Calibration";
  else if ( tabName == "Diagnostics" )
    url += "Diagnostics";
  else if (tabName == "Symmetrise")
    url += "Symmetrise";
  else if ( tabName == "S(Q, w)" )
    url += "SofQW";
  else if (tabName == "Transmission")
    url += "Transmission";
  else if (tabName == "Moments")
    url += "Moments";

  QDesktopServices::openUrl(QUrl(url));
}


/**
 * Called when the user clicks the Python export button.
 */
void IndirectDataReduction::exportTabPython()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  m_tabs[tabName]->exportPythonScript();
}


/**
 * This is the function called when the "Run" button is clicked. It will call the relevent function
 * in the subclass.
 */
void IndirectDataReduction::runClicked()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
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
  m_tabs["Energy Transfer"] = new IndirectConvertToEnergy(m_uiForm, this);
  m_tabs["Calibration"] = new IndirectCalibration(m_uiForm, this);
  m_tabs["Diagnostics"] = new IndirectDiagnostics(m_uiForm, this);
  m_tabs["Transmission"] = new IndirectTransmission(m_uiForm, this);
  m_tabs["Symmetrise"] = new IndirectSymmetrise(m_uiForm, this);
  m_tabs["S(Q, w)"] = new IndirectSqw(m_uiForm, this);
  m_tabs["Moments"] = new IndirectMoments(m_uiForm, this);

  // Handle instrument configuration changes
  connect(m_uiForm.iicInstrumentConfiguration, SIGNAL(instrumentConfigurationUpdated(const QString &, const QString &, const QString &)),
          this, SLOT(instrumentSetupChanged(const QString &, const QString &, const QString &)));

  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  // Connect the Python export buton
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Run" button
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));

  // Reset the Run button state when the tab is changed
  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateRunButton()));

  // Connect tab signals and run any setup code
  for(auto it = m_tabs.begin(); it != m_tabs.end(); ++it)
  {
    connect(it->second, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
    connect(it->second, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
    connect(it->second, SIGNAL(updateRunButton(bool, QString, QString)), this, SLOT(updateRunButton(bool, QString, QString)));
    connect(this, SIGNAL(newInstrumentConfiguration()), it->second, SIGNAL(newInstrumentConfiguration())),
    it->second->setupTab();
  }

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
  MatrixWorkspace_sptr ws = loadInstrumentIfNotExist(instrumentName.toStdString(), analyser.toStdString(), reflection.toStdString());
  instrumentLoadingDone(ws == NULL);
  emit newInstrumentConfiguration();
}


/**
 * Loads an empty instrument into a workspace (__empty_INST) unless the workspace already exists.
 *
 * If an analyser and reflection are supplied then the corresponding IPF is also loaded.
 *
 * @param instrumentName Name of the instrument to load
 * @param analyser Analyser being used (optional)
 * @param reflection Relection being used (optional)
 * @returns Pointer to instrument workspace
 */
Mantid::API::MatrixWorkspace_sptr IndirectDataReduction::loadInstrumentIfNotExist(std::string instrumentName,
    std::string analyser, std::string reflection)
{
  std::string instWorkspaceName = "__empty_" + instrumentName;
  std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

  // If the workspace does not exist in ADS then load an empty instrument
  if(!AnalysisDataService::Instance().doesExist(instWorkspaceName))
  {
    std::string parameterFilename = idfDirectory + instrumentName + "_Definition.xml";
    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", parameterFilename);
    loadAlg->setProperty("OutputWorkspace", instWorkspaceName);
    loadAlg->execute();
  }

  // Load the IPF if given an analyser and reflection
  if(!analyser.empty() && !reflection.empty())
  {
    std::string ipfFilename = idfDirectory + instrumentName + "_" + analyser + "_" + reflection + "_Parameters.xml";
    IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Filename", ipfFilename);
    loadParamAlg->setProperty("Workspace", instWorkspaceName);
    loadParamAlg->execute();
  }

  // Get the workspace, which should exist now
  MatrixWorkspace_sptr instWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(instWorkspaceName);

  return instWorkspace;
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
  settings.beginGroup(m_settingsGroup + "DataFiles");
  settings.setValue("last_directory", m_dataDir);
  m_uiForm.ind_runFiles->readSettings(settings.group());
  m_uiForm.cal_leRunNo->readSettings(settings.group());
  m_uiForm.slice_inputFile->readSettings(settings.group());
  settings.endGroup();

  settings.beginGroup(m_settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", m_saveDir);
  m_uiForm.ind_calibFile->readSettings(settings.group());
  m_uiForm.ind_mapFile->readSettings(settings.group());
  m_uiForm.slice_dsCalibFile->readSettings(settings.group());
  m_uiForm.moment_dsInput->readSettings(settings.group());
  m_uiForm.sqw_dsSampleInput->readSettings(settings.group());
  settings.endGroup();

  // Load the last used instrument
  settings.beginGroup(m_settingsGroup);
  QString instName = settings.value("instrument-name", "").toString();
  settings.endGroup();
}


/**
 * Save settings to a persistent storage.
 */
void IndirectDataReduction::saveSettings()
{
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  QString instrName;

  instrName = m_uiForm.iicInstrumentConfiguration->getInstrumentName();

  settings.setValue("instrument-name", instrName);
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
