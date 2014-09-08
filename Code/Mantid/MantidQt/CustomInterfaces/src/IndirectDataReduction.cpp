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

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt;
//----------------------
// Public member functions
//----------------------

/**
 * Default constructor for class. Initialises interface pointers to NULL values.
 * @param parent :: This is a pointer to the "parent" object in Qt, most likely the main MantidPlot window.
 */
IndirectDataReduction::IndirectDataReduction(QWidget *parent) :
  UserSubWindow(parent),
  m_curInterfaceSetup(""),
  m_settingsGroup("CustomInterfaces/IndirectDataReduction"),
  m_algRunner(new MantidQt::API::AlgorithmRunner(this)),
  m_changeObserver(*this, &IndirectDataReduction::handleDirectoryChange)
{
  //Signals to report load instrument algo result
  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));
}

/**
 * Destructor
 */
IndirectDataReduction::~IndirectDataReduction()
{
  //Make sure no algos are sunning after the window has been closed
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
  else if ( tabName == "S(Q, w)" )
    url += "SofQW";
  else if (tabName == "Transmission")
    url += "Transmission";
  else if (tabName == "Moments")
    url += "Moments";
  QDesktopServices::openUrl(QUrl(url));
}

/**
 * This is the function called when the "Run" button is clicked. It will call the relevent function
 * in the subclass.
 */
void IndirectDataReduction::runClicked()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  if ( tabName == "Energy Transfer" )
    m_tab_convert_to_energy->runTab();
  else if ( tabName == "Calibration" )
    m_tab_calibration->runTab();
  else if ( tabName == "Diagnostics" )
    m_tab_diagnostics->runTab();
  else if ( tabName == "S(Q, w)" )
    m_tab_sqw->runTab();
  else if (tabName == "Transmission")
    m_tab_trans->runTab();
  else if(tabName == "Moments")
    m_tab_moments->runTab();
}

/**
 * Sets up Qt UI file and connects signals, slots. 
 */
void IndirectDataReduction::initLayout()
{
  m_uiForm.setupUi(this);

  m_tab_convert_to_energy = new IndirectConvertToEnergy(m_uiForm, this);
  m_tab_sqw = new IndirectSqw(m_uiForm, this);
  m_tab_diagnostics = new IndirectDiagnostics(m_uiForm, this);
  m_tab_calibration = new IndirectCalibration(m_uiForm, this);
  m_tab_trans = new IndirectTransmission(m_uiForm, this);
  m_tab_moments = new IndirectMoments(m_uiForm, this);

  // Assume we get a incompatiable instrument to start with
  m_uiForm.pbRun->setEnabled(false);

  // Signal / Slot Connections Set Up Here

  // signal/slot connections to respond to changes in instrument selection combo boxes
  connect(m_uiForm.cbInst, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(userSelectInstrument(const QString&)));

  // connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  // connect the "Run" button
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  // connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));

  // ignals for tabs running Python
  connect(m_tab_convert_to_energy, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_sqw, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_calibration, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_diagnostics, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_trans, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_moments, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));

  // ignals for tabs showing mesage boxes
  connect(m_tab_convert_to_energy, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_sqw, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_calibration, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_diagnostics, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_trans, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_moments, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));

  // Run any tab setup code
  m_tab_convert_to_energy->setupTab();
  m_tab_sqw->setupTab();
  m_tab_diagnostics->setupTab();
  m_tab_calibration->setupTab();
  m_tab_trans->setupTab();
  m_tab_moments->setupTab();
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

  if ( m_curInterfaceSetup == "" )
  {
    userSelectInstrument(m_uiForm.cbInst->currentText());
  }
}

/**
 * Read settings from the persistent store
 */
void IndirectDataReduction::readSettings()
{
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  QString instrName = settings.value("instrument-name", "").toString();
  settings.endGroup();

  setDefaultInstrument(instrName);
}

/**
 * Save settings to a persistent storage
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
 * Sets up the initial instrument for the interface. This value is taken from the users'
 * settings in the menu View -> Preferences -> Mantid -> Instrument
 * @param name :: The name of the default instrument
 */
void IndirectDataReduction::setDefaultInstrument(const QString & name)
{
  if( name.isEmpty() ) return;

  int index = m_uiForm.cbInst->findText(name);
  if( index >= 0 )
  {
    m_uiForm.cbInst->setCurrentIndex(index);
  }
}

/**
 * This function: 1. loads the instrument and gets the value of deltaE-mode parameter
 *				 2. Based on this value, makes the necessary changes to the form setup (direct or indirect).
 * @param name :: name of the instrument from the QComboBox
 */
void IndirectDataReduction::instrumentSelectChanged(const QString& name)
{
  QString defFile = (Mantid::API::ExperimentInfo::getInstrumentFilename(name.toStdString())).c_str();
  if((defFile == "") || !m_uiForm.cbInst->isVisible())
  {
    g_log.error("Instument loading failed!");
    m_uiForm.cbInst->setEnabled(true);
    m_uiForm.pbRun->setEnabled(true);
    return;
  }

  QString outWS = "__empty_" + m_uiForm.cbInst->currentText();

  m_curInterfaceSetup = name;

  //Load the empty instrument into the workspace __empty_[name]
  //This used to be done in Python
  Mantid::API::IAlgorithm_sptr instLoader = Mantid::API::AlgorithmManager::Instance().create("LoadEmptyInstrument", -1);
  instLoader->initialize();
  instLoader->setProperty("Filename", defFile.toStdString());
  instLoader->setProperty("OutputWorkspace", outWS.toStdString());

  //Ensure no other algorithm is running
  m_algRunner->cancelRunningAlgorithm();
  m_algRunner->startAlgorithm(instLoader);
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
    m_uiForm.pbRun->setEnabled(true);
    return;
  }

  performInstSpecific();
  setIDFValues(curInstPrefix);

  m_uiForm.pbRun->setEnabled(true);
  m_uiForm.cbInst->setEnabled(true);
}

/**
 * If the instrument selection has changed, calls instrumentSelectChanged
 * @param prefix :: instrument name from QComboBox object
 */
void IndirectDataReduction::userSelectInstrument(const QString& prefix) 
{
  if ( prefix != m_curInterfaceSetup )
  {
    // Remove the old empty instrument workspace if it is there
    std::string ws_name = "__empty_" + m_curInterfaceSetup.toStdString();
    Mantid::API::AnalysisDataServiceImpl& dataStore = Mantid::API::AnalysisDataService::Instance();
    if( dataStore.doesExist(ws_name) )
    {
      dataStore.remove(ws_name);
    }

    m_uiForm.pbRun->setEnabled(false);
    m_uiForm.cbInst->setEnabled(false);
    instrumentSelectChanged(prefix);
  }
}

void IndirectDataReduction::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix :: The selected instruments prefix in Mantid.
*/
void IndirectDataReduction::setIDFValues(const QString & prefix)
{
  dynamic_cast<IndirectConvertToEnergy *>(m_tab_convert_to_energy)->setIDFValues(prefix);
}

/**
* This function holds any steps that must be performed on the layout that are specific
* to the currently selected instrument.
*/
void IndirectDataReduction::performInstSpecific()
{
  setInstSpecificWidget("cm-1-convert-choice", m_uiForm.ckCm1Units, QCheckBox::Off);
  setInstSpecificWidget("save-aclimax-choice", m_uiForm.save_ckAclimax, QCheckBox::Off);
}

/**
* This function either shows or hides the given QCheckBox, based on the named property
* inside the instrument param file.  When hidden, the default state will be used to
* reset to the "unused" state of the checkbox.
*
* @param parameterName :: The name of the property to look for inside the current inst param file.
* @param checkBox :: The checkbox to set the state of, and to either hide or show based on the current inst.
* @param defaultState :: The state to which the checkbox will be set upon hiding it.
*/
void IndirectDataReduction::setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState)
{
  // Get access to instrument specific parameters via the loaded empty workspace.
  std::string instName = m_uiForm.cbInst->currentText().toStdString();
  Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("__empty_" + instName));
  if(input == NULL)
    return;

  Mantid::Geometry::Instrument_const_sptr instr = input->getInstrument();

  // See if the instrument params file requests that the checkbox be shown to the user.
  std::vector<std::string> showParams = instr->getStringParameter(parameterName);
  
  std::string show = "";
  if(!showParams.empty())
    show = showParams[0];
  
  if(show == "Show")
    checkBox->setHidden(false);
  else
  {
    checkBox->setHidden(true);
    checkBox->setState(defaultState);
  }
}

void IndirectDataReduction::closeEvent(QCloseEvent* close)
{
  (void) close;
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void IndirectDataReduction::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();

  if ( key == "datasearch.directories" || key == "defaultsave.directory" )
  {
    loadSettings();
  }
}

void IndirectDataReduction::loadSettings()
{  
  // set values of m_dataDir and m_saveDir
  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_dataDir.replace(" ","");
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
  m_uiForm.slice_calibFile->readSettings(settings.group());
  m_uiForm.moment_dsInput->readSettings(settings.group());
  m_uiForm.transInputFile->readSettings(settings.group());
  m_uiForm.transCanFile->readSettings(settings.group());
  m_uiForm.sqw_dsSampleInput->readSettings(settings.group());
  settings.endGroup();
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 * 
 * @param message :: The message to display in the message box
 */
void IndirectDataReduction::showMessageBox(const QString& message)
{
  showInformationBox(message);
}
