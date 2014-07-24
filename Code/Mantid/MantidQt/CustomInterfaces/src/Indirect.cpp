#include "MantidQtCustomInterfaces/Indirect.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/IndirectCalibration.h"
#include "MantidQtCustomInterfaces/IndirectConvertToEnergy.h"
#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"
#include "MantidQtCustomInterfaces/IndirectMoments.h"
#include "MantidQtCustomInterfaces/IndirectSqw.h"
#include "MantidQtCustomInterfaces/Transmission.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <cmath>

#include <Poco/NObserver.h>

#include <QtCheckBoxFactory>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::IndirectDataReduction & uiForm) : 
  UserSubWindow(parent), m_uiForm(uiForm),
  m_changeObserver(*this, &Indirect::handleDirectoryChange),
  // Additional tab interfaces
  m_tab_convert_to_energy(new IndirectConvertToEnergy(m_uiForm, this)),
  m_tab_sqw(new IndirectSqw(m_uiForm, this)),
  m_tab_diagnostics(new IndirectDiagnostics(m_uiForm, this)),
  m_tab_calibration(new IndirectCalibration(m_uiForm, this)),
  m_tab_trans(new Transmission(m_uiForm, this)),
  m_tab_moments(new IndirectMoments(m_uiForm, this))
{
}

/**
* This function performs any one-time actions needed when the Inelastic interface
* is first selected, such as connecting signals to slots.
*/
void Indirect::initLayout()
{
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  m_settingsGroup = "CustomInterfaces/ConvertToEnergy/Indirect/";

  //All tab signals
  connect(m_tab_convert_to_energy, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_sqw, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_calibration, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_diagnostics, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_trans, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_moments, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));

  connect(m_tab_convert_to_energy, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_sqw, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_calibration, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_diagnostics, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_trans, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_moments, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));

  // set default values for save formats
  m_uiForm.save_ckSPE->setChecked(false);
  m_uiForm.save_ckNexus->setChecked(true);

  loadSettings();
}

/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
}

/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
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
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
*/
void Indirect::runClicked()
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
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix :: The selected instruments prefix in Mantid.
*/
void Indirect::setIDFValues(const QString & prefix)
{
  dynamic_cast<IndirectConvertToEnergy *>(m_tab_convert_to_energy)->setIDFValues(prefix);
}

/**
* This function holds any steps that must be performed on the layout that are specific
* to the currently selected instrument.
*/
void Indirect::performInstSpecific()
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
void Indirect::setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState)
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

void Indirect::closeEvent(QCloseEvent* close)
{
  (void) close;
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void Indirect::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();

  if ( key == "datasearch.directories" || key == "defaultsave.directory" )
  {
    loadSettings();
  }
}

void Indirect::loadSettings()
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
 * Called when a user starts to type / edit the runs to load.
 */
void Indirect::pbRunEditing()
{
  m_uiForm.pbRun->setEnabled(false);
  m_uiForm.pbRun->setText("Editing...");
}

/**
 * Called when the FileFinder starts finding the files.
 */
void Indirect::pbRunFinding()
{
  m_uiForm.pbRun->setText("Finding files...");
  m_uiForm.ind_runFiles->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void Indirect::pbRunFinished()
{
  m_uiForm.pbRun->setEnabled(true);
  m_uiForm.ind_runFiles->setEnabled(true);
}

void Indirect::intensityScaleMultiplierCheck(bool state)
{
  m_uiForm.cal_leIntensityScaleMultiplier->setEnabled(state);
}

void Indirect::calibValidateIntensity(const QString & text)
{
  if(!text.isEmpty())
  {
    m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");
  }
  else
  {
    m_uiForm.cal_valIntensityScaleMultiplier->setText("*");
  }
}

void Indirect::useCalib(bool state)
{
  m_uiForm.ind_calibFile->isOptional(!state);
  m_uiForm.ind_calibFile->setEnabled(state);
}

/**
* Controls the ckUseCalib checkbox to automatically check it when a user inputs a file from clicking on 'browse'.
* @param calib :: path to calib file
*/
void Indirect::calibFileChanged(const QString & calib)
{
  if ( calib.isEmpty() )
  {
    m_uiForm.ckUseCalib->setChecked(false);
  }
  else
  {
    m_uiForm.ckUseCalib->setChecked(true);
  }
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 * 
 * @param message :: The message to display in the message box
 */
void Indirect::showMessageBox(const QString& message)
{
  showInformationBox(message);
}
