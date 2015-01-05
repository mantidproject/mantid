//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDataReduction.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/HelpWindow.h"
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
  MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Indirect_DataReduction"));
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

  if(m_instrument == "")
    instrumentSelected(m_uiForm.cbInst->currentText());

  // Create the tabs
  m_tabs["Energy Transfer"] = new IndirectConvertToEnergy(m_uiForm, this);
  m_tabs["Calibration"] = new IndirectCalibration(m_uiForm, this);
  m_tabs["Diagnostics"] = new IndirectDiagnostics(m_uiForm, this);
  m_tabs["Transmission"] = new IndirectTransmission(m_uiForm, this);
  m_tabs["Symmetrise"] = new IndirectSymmetrise(m_uiForm, this);
  m_tabs["S(Q, w)"] = new IndirectSqw(m_uiForm, this);
  m_tabs["Moments"] = new IndirectMoments(m_uiForm, this);

  // Handle the instrument being changed
  connect(m_uiForm.cbInst, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(instrumentSelected(const QString&)));
  // Handle the analyser being changed
  connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(int)), this, SLOT(analyserSelected(int)));
  // Handle the reflection being changed
  connect(m_uiForm.cbReflection, SIGNAL(currentIndexChanged(int)), this, SLOT(instrumentSetupChanged()));

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
 */
void IndirectDataReduction::instrumentSetupChanged()
{
  QString instrumentName = m_uiForm.cbInst->currentText();
  QString analyser = m_uiForm.cbAnalyser->currentText();
  QString reflection = m_uiForm.cbReflection->currentText();

  if(instrumentName != "" && analyser != "" && reflection != "")
  {
    loadInstrumentIfNotExist(instrumentName.toStdString(), analyser.toStdString(), reflection.toStdString());
    emit newInstrumentConfiguration();
  }
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
 * Gets the operation modes for the current instrument as defined in it's parameter file.
 *
 * @returns A list of analysers and a vector of reflections that can be used with each
 */
std::vector<std::pair<std::string, std::vector<std::string> > > IndirectDataReduction::getInstrumentModes()
{
  std::vector<std::pair<std::string, std::vector<std::string> > > modes;
  MatrixWorkspace_sptr instWorkspace = loadInstrumentIfNotExist(m_instrument.toStdString());
  Instrument_const_sptr instrument = instWorkspace->getInstrument();

  std::vector<std::string> analysers;
  boost::split(analysers, instrument->getStringParameter("analysers")[0], boost::is_any_of(","));

  for(auto it = analysers.begin(); it != analysers.end(); ++it)
  {
    std::string analyser = *it;
    std::string ipfReflections = instrument->getStringParameter("refl-" + analyser)[0];

    std::vector<std::string> reflections;
    boost::split(reflections, ipfReflections, boost::is_any_of(","), boost::token_compress_on);

    std::pair<std::string, std::vector<std::string> > data(analyser, reflections);
    modes.push_back(data);
  }

  return modes;
}


/**
 * Updated the list of analysers based on the current instrument.
 */
void IndirectDataReduction::updateAnalyserList()
{
  auto instModes = getInstrumentModes();

  m_uiForm.cbAnalyser->clear();

  for(auto modesIt = instModes.begin(); modesIt != instModes.end(); ++modesIt)
  {
    QString analyser = QString::fromStdString(modesIt->first);
    std::vector<std::string> reflections = modesIt->second;

    if(analyser != "diffraction") // Do not put diffraction into the analyser list
    {
      if(reflections.size() > 0)
      {
        QStringList reflectionsList;
        for(auto reflIt = reflections.begin(); reflIt != reflections.end(); ++reflIt)
          reflectionsList.push_back(QString::fromStdString(*reflIt));
        QVariant data = QVariant(reflectionsList);
        m_uiForm.cbAnalyser->addItem(analyser, data);
      }
      else
      {
        m_uiForm.cbAnalyser->addItem(analyser);
      }
    }
  }

  analyserSelected(m_uiForm.cbAnalyser->currentIndex());
}


/**
 * Tasks to be carried out after an empty instument has finished loading
 */
void IndirectDataReduction::instrumentLoadingDone(bool error)
{
  QString curInstPrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();
  if((curInstPrefix == "") || error)
  {
    g_log.error("Instument loading failed! (this can be caused by having both direct and indirect interfaces open)");
    m_uiForm.cbInst->setEnabled(true);
    updateRunButton(false, "No Instrument", "No instrument is currently loaded.");
    return;
  }

  updateAnalyserList();
  updateRunButton();
  m_uiForm.cbInst->setEnabled(true);
}


/**
 * Handled loading thebase instrument when it is selected form the instrument combo box.
 *
 * @param instName Instrument name from QComboBox object
 */
void IndirectDataReduction::instrumentSelected(const QString& instName)
{
  if(instName != m_instrument)
  {
    // Remove the old empty instrument workspace if it is there
    std::string wsName = "__empty_" + m_instrument.toStdString();
    Mantid::API::AnalysisDataServiceImpl& dataStore = Mantid::API::AnalysisDataService::Instance();
    if(dataStore.doesExist(wsName))
      dataStore.remove(wsName);

    updateRunButton(false, "Loading Inst.", "Loading the selected instrument...");
    m_uiForm.cbInst->setEnabled(false);
    loadInstrumentIfNotExist(instName.toStdString());
    m_instrument = instName;

    //TODO
    instrumentLoadingDone(false);
  }
}


/**
 * Updates the list of reflections in the reflection combo box when the analyser is changed.
 *
 * @param index Index of analyser in combo box
 */
void IndirectDataReduction::analyserSelected(int index)
{
  // Populate Reflection combobox with correct values for Analyser selected.
  m_uiForm.cbReflection->clear();

  QVariant currentData = m_uiForm.cbAnalyser->itemData(index);
  if ( currentData == QVariant::Invalid )
  {
    m_uiForm.lbReflection->setEnabled(false);
    m_uiForm.cbReflection->setEnabled(false);
    return;
  }
  else
  {
    m_uiForm.lbReflection->setEnabled(true);
    m_uiForm.cbReflection->setEnabled(true);
    QStringList reflections = currentData.toStringList();
    for ( int i = 0; i < reflections.count(); i++ )
    {
      m_uiForm.cbReflection->addItem(reflections[i]);
    }
  }

  emit instrumentSetupChanged();
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

  if(instName.isEmpty())
    return;

  int index = m_uiForm.cbInst->findText(instName);
  if(index >= 0)
    m_uiForm.cbInst->setCurrentIndex(index);
}


/**
 * Save settings to a persistent storage.
 */
void IndirectDataReduction::saveSettings()
{
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  QString instrName;

  instrName = m_uiForm.cbInst->currentText();

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
