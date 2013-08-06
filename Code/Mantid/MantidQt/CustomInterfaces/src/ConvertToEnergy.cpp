//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ConvertToEnergy.h"
#include "MantidQtCustomInterfaces/Homer.h" // user interface for Direct instruments
#include "MantidQtCustomInterfaces/Indirect.h" // user interface for Indirect instruments

#include "MantidQtAPI/ManageUserDirectories.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QMessageBox>
#include <QDir>

#include <QDesktopServices>
#include <QUrl>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(ConvertToEnergy);
  }
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------

/**
 * Default constructor for class. Initialises interface pointers to NULL values.
 * @param parent :: This is a pointer to the "parent" object in Qt, most likely the main MantidPlot window.
 */
ConvertToEnergy::ConvertToEnergy(QWidget *parent) :
  UserSubWindow(parent), m_directInstruments(NULL), m_indirectInstruments(NULL), 
  m_curInterfaceSetup(""), m_curEmodeType(ConvertToEnergy::Undefined), m_settingsGroup("CustomInterfaces/ConvertToEnergy")
{
}

/**
 * Destructor
 */
ConvertToEnergy::~ConvertToEnergy()
{
  saveSettings();
}

/**
 * On user clicking the "help" button on the interface, directs their request to the relevant
 * interface's helpClicked() function.
 */
void ConvertToEnergy::helpClicked()
{
  switch ( m_curEmodeType )
  {
  case Direct:
    m_directInstruments->helpClicked();
    break;
  case InDirect:
    m_indirectInstruments->helpClicked();
    break;
  default:
    QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
				   "ConvertToEnergy"));
  }
}

/**
 * This is the function called when the "Run" button is clicked. It will call the relevent function
 * in the subclass.
 */
void ConvertToEnergy::runClicked()
{
  switch ( m_curEmodeType )
  {
  case Direct:
    m_directInstruments->runClicked();
    break;
  case InDirect:
    m_indirectInstruments->runClicked();
    break;
  case Undefined:
  default:
    showInformationBox("This interface is not configured to use the instrument you have selected.\nPlease check your instrument selection.");
  }
}

/**
 * Sets up Qt UI file and connects signals, slots. 
 */
void ConvertToEnergy::initLayout()
{
  m_uiForm.setupUi(this);
  m_curInterfaceSetup = "";
  m_curEmodeType = Undefined;

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

}

/**
 * This function is ran after initLayout(), and runPythonCode is unavailable before this function
 * has run (because of the setup of the base class). For this reason, "setup" functions that require
 * Python scripts are located here.
 */
void ConvertToEnergy::initLocalPython()
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
void ConvertToEnergy::readSettings()
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
void ConvertToEnergy::saveSettings()
{
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  QString instrName;
  if( m_curEmodeType == Undefined )
  {
    instrName = "";
  }
  else
  {
    instrName = m_uiForm.cbInst->currentText();
  }

  settings.setValue("instrument-name", instrName);
  settings.endGroup();
}

/**
 * Sets up the initial instrument for the interface. This value is taken from the users'
 * settings in the menu View -> Preferences -> Mantid -> Instrument
 * @param name :: The name of the default instrument
 */
void ConvertToEnergy::setDefaultInstrument(const QString & name)
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
void ConvertToEnergy::instrumentSelectChanged(const QString& name)
{
  if ( ! m_uiForm.cbInst->isVisible() )
  {
    return;
  }

  QString defFile = (Mantid::API::ExperimentInfo::getInstrumentFilename(name.toStdString())).c_str();

  if ( defFile == "" )
  {
    m_curEmodeType = Undefined;
    return;
  }

  DeltaEMode desired = instrumentDeltaEMode(defFile);

  if ( desired == Undefined )
  {
    m_curEmodeType = Undefined;
    QMessageBox::warning(this, "MantidPlot", "Selected instrument (" + name + ") does not have a parameter to signify it's deltaE-mode");
    m_uiForm.cbInst->blockSignals(true);
    m_uiForm.cbInst->setCurrentIndex(m_uiForm.cbInst->findText(m_curInterfaceSetup));
    m_uiForm.cbInst->blockSignals(false);
    return;
  }

  DeltaEMode current;

  if ( m_curInterfaceSetup == "" )
  {
    current = Undefined;
  }
  else
  {
    current = DeltaEMode(m_uiForm.swInstrument->currentIndex());
  }

  if ( desired != current || m_curInterfaceSetup != name )
  {
    changeInterface(desired);
  }

  m_curInterfaceSetup = name;
  m_curEmodeType = desired;
  m_uiForm.pbRun->setEnabled(true);

}

/**
 * Runs a Python script to discover whether the selected instrument is direct or indirect.
 * @param defFile :: path to instrument definition file.
 * @return 'Undefined' deltaE-mode not found, otherwise the relevant value ('Direct' or 'InDirect')
 */
ConvertToEnergy::DeltaEMode ConvertToEnergy::instrumentDeltaEMode(const QString& defFile)
{
  QString pyInput =
    "from mantid.simpleapi import LoadEmptyInstrument,mtd\n"
    "ws_name = '__empty_%2'\n"
    "if not mtd.doesExist(ws_name):\n"
    "  LoadEmptyInstrument(Filename=r'%1', OutputWorkspace=ws_name)\n"
    "instrument = mtd[ws_name].getInstrument()\n"
    "try:\n"
    "    print instrument.getStringParameter('deltaE-mode')[0]\n"
    "except IndexError, message:\n" // the above line will raise an IndexError in Python
    "    print ''\n"; // if the instrument doesn't have this parameter.

  pyInput = pyInput.arg(defFile,m_uiForm.cbInst->currentText());

  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput == "direct" )
  {
    return Direct;
  }
  else if ( pyOutput == "indirect" )
  {
    return InDirect;
  }
  else
  {
    return Undefined;
  }
}

/**
 * Makes the changes necessary for switching between Direct and Indirect interfaces.
 * @param desired :: The interface format that is to be changed to.
 */
void ConvertToEnergy::changeInterface(DeltaEMode desired)
{
  QString curInstPrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();;
  switch ( desired )
  {
  case Direct:
    m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabCalibration));
    m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabSofQW));
    m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabTimeSlice));
    m_uiForm.tabWidget->addTab(m_uiForm.tabDiagnoseDetectors, "Diagnose Detectors");
    m_uiForm.tabWidget->addTab(m_uiForm.tabAbsoluteUnits, "Absolute Units");
    if ( m_directInstruments == NULL )
    {
      m_directInstruments = new Homer(qobject_cast<QWidget*>(this->parent()), m_uiForm);
      m_directInstruments->initLayout();
      connect(m_directInstruments, SIGNAL(runAsPythonScript(const QString&, bool)),
	      this, SIGNAL(runAsPythonScript(const QString&, bool)));
      m_directInstruments->initializeLocalPython();
    }
    m_directInstruments->setIDFValues(curInstPrefix);
    break;
  case InDirect:
    m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabDiagnoseDetectors));
    m_uiForm.tabWidget->removeTab(m_uiForm.tabWidget->indexOf(m_uiForm.tabAbsoluteUnits));
    m_uiForm.tabWidget->addTab(m_uiForm.tabCalibration, "Calibration");
    m_uiForm.tabWidget->addTab(m_uiForm.tabTimeSlice, "Diagnostics");
    m_uiForm.tabWidget->addTab(m_uiForm.tabSofQW, "S(Q, w)");
    if ( m_indirectInstruments == NULL )
    {
      m_indirectInstruments = new Indirect(qobject_cast<QWidget*>(this->parent()), m_uiForm);
      m_indirectInstruments->initLayout();
      connect(m_indirectInstruments, SIGNAL(runAsPythonScript(const QString&, bool)),
	      this, SIGNAL(runAsPythonScript(const QString&, bool)));
      m_indirectInstruments->initializeLocalPython();
    }
    m_indirectInstruments->performInstSpecific();
    m_indirectInstruments->setIDFValues(curInstPrefix);
    break;
  default:
    QMessageBox::information(this, "MantidPlot", "Undefined interface type detected.");
    return;
  }
  m_uiForm.swInstrument->setCurrentIndex(desired);
  m_uiForm.swInputFiles->setCurrentIndex(desired);
  m_uiForm.swAnalysis->setCurrentIndex(desired);
  m_uiForm.swConvertToEnergy->setCurrentIndex(desired);
  m_uiForm.swRebin->setCurrentIndex(desired);
  m_uiForm.swSave->setCurrentIndex(desired);
}

/**
 * If the instrument selection has changed, calls instrumentSelectChanged
 * @param prefix :: instrument name from QComboBox object
 */
void ConvertToEnergy::userSelectInstrument(const QString& prefix) 
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
    m_uiForm.pbRun->setEnabled(true);
    m_uiForm.cbInst->setEnabled(true);
  }
  if( m_curEmodeType != InDirect )
  {
    m_uiForm.pbRun->setEnabled(true);
  }
}

void ConvertToEnergy::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}
