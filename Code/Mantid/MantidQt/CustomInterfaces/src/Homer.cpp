#include "MantidQtCustomInterfaces/Homer.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/deltaECalc.h"
#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidQtAPI/FileDialogHandler.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

#include "Poco/Path.h"

#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QSignalMapper>
#include <QDesktopServices>
#include <QFileDialog>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QCloseEvent>
#include <QHideEvent>
#include <QShowEvent>
#include <QMessageBox>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
Homer::Homer(QWidget *parent, Ui::ConvertToEnergy & uiForm) : 
  UserSubWindow(parent), m_uiForm(uiForm),
  m_backgroundDialog(NULL), m_diagPage(NULL),m_saveChanged(false),
  m_backgroundWasVisible(false), m_absEiDirty(false), m_topSettingsGroup("CustomInterfaces/Homer")
{}

/// Set up the dialog layout
void Homer::initLayout()
{
  setUpPage1();
  setUpPage2();
  setUpPage3();
  
  m_uiForm.pbRun->setToolTip("Process run files");
  m_uiForm.pbHelp->setToolTip("Online documentation (loads in a browser)");

  readSettings();
}

/**
 * Called when the form is asked to show
 */
void Homer::showEvent(QShowEvent *event)
{
  if( m_backgroundWasVisible )
  {
    m_backgroundDialog->show();
  }
  event->accept();
}

/**
 * Called when the form is asked to hide
 */
void Homer::hideEvent(QHideEvent *event)
{
  if( m_backgroundDialog->isVisible() )
  {
    m_backgroundDialog->hide();
    m_backgroundWasVisible = true;
  }
  else
  {
    m_backgroundWasVisible = false;
  }
  event->accept();
}

/**
 * Called when the form is asked to close
 */
void Homer::closeEvent(QCloseEvent *event)
{
  if( m_backgroundDialog->isVisible() )
  {
    m_backgroundDialog->close();
    m_backgroundWasVisible = false;
  }
  event->accept();
}

/** Disables the form when passed the information that Python is running
*  and enables it when instructed that Pythons scripts have stopped
*  @param running if set to false only controls disabled by a previous call to this function will be re-enabled
*/
void Homer::pythonIsRunning(bool running)
{// the run button was disabled when the results form was shown, as we can only do one analysis at a time, we can enable it now
  m_uiForm.tabWidget->setEnabled( ! running );
  m_uiForm.pbRun->setEnabled( ! running );
}

/// For each widgets in the first tab this adds custom widgets, fills in combination boxes and runs setToolTip()
void Homer::setUpPage1()
{
  page1FileWidgs();
  page1Validators();

  m_backgroundDialog = new Background(this);
  
  connect(m_uiForm.pbBack, SIGNAL(clicked()), this, SLOT(bgRemoveClick()));

  // SIGNALS and SLOTS that deal with copying the text from one edit box to another 
  connect(m_uiForm.ckSumSpecs, SIGNAL(stateChanged(int)), this, SLOT(updateSaveName()));
  connect(m_uiForm.leNameSPE, SIGNAL(editingFinished()), this, SLOT(saveNameUpd()));
  connect(m_uiForm.pbBrowseSPE, SIGNAL(clicked()), this, SLOT(browseSaveFile()));
}

/// put default values into the controls in the first tab
void Homer::page1FileWidgs()
{
  connect(m_uiForm.runFiles, SIGNAL(fileEditingFinished()), this, SLOT(runFilesChanged()));
  connect(m_uiForm.whiteBeamFile, SIGNAL(fileEditingFinished()), this, SLOT(updateWBV()));

  // Add the save buttons to a button group
  m_saveChecksGroup = new QButtonGroup();
  m_saveChecksGroup->addButton(m_uiForm.save_ckSPE);
  m_saveChecksGroup->addButton(m_uiForm.save_ckNexus);
  m_saveChecksGroup->addButton(m_uiForm.save_ckNxSPE);
  m_saveChecksGroup->setExclusive(false);

  connect(m_saveChecksGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(saveFormatOptionClicked(QAbstractButton*)));
}

/// make validator labels and associate them with the controls that need them in the first tab
void Homer::page1Validators()
{
  m_validators.clear();

  //Ensure that only numbers can be typed into the boxes
  m_uiForm.leEGuess->setValidator(new QDoubleValidator(m_uiForm.leEGuess));
  m_uiForm.leELow->setValidator(new QDoubleValidator(m_uiForm.leELow));
  m_uiForm.leEWidth->setValidator(new QDoubleValidator(m_uiForm.leEWidth));
  m_uiForm.leEHigh->setValidator(new QDoubleValidator(m_uiForm.leEHigh));

  //Remember which labels refer to these boxes
  m_validators.insert(m_uiForm.leELow, m_uiForm.validRebinLow);
  m_validators.insert(m_uiForm.leEWidth, m_uiForm.validRebinWidth);
  m_validators.insert(m_uiForm.leEHigh, m_uiForm.validRebinHigh);

  // Validate the input when something is typed
  connect(m_uiForm.leEGuess, SIGNAL(textChanged(const QString&)), this, SLOT(validateRunEi(const QString &)));
  connect(m_uiForm.leELow, SIGNAL(textChanged(const QString&)), this, SLOT(validateRebinBox(const QString &)));
  connect(m_uiForm.leEWidth, SIGNAL(textChanged(const QString&)), this, SLOT(validateRebinBox(const QString &)));
  connect(m_uiForm.leEHigh, SIGNAL(textChanged(const QString&)), this, SLOT(validateRebinBox(const QString &)));

}

/// Adds the diag custom widgets and a check box to allow users to enable or disable the widget
void Homer::setUpPage2()
{
  /* The diag -detector diagnositics part of the form is a separate widget, all the work is 
     coded in over there
     this second page is largely filled with the diag widget, previous settings, 
     second argument, depends on the instrument and the detector diagnostic settings are 
     kept separate in "diag/"*/

  m_diagPage = new MWDiag(this, getInstrumentSettingsGroup() + "/diag", m_uiForm.cbInst);
	
  QLayout *diagLayout = m_uiForm.tabDiagnoseDetectors->layout();
  diagLayout->addWidget(m_diagPage);

  connect(m_uiForm.ckRunDiag, SIGNAL(toggled(bool)), m_diagPage, SLOT(setEnabled(bool)));
  connect(m_diagPage, SIGNAL(runAsPythonScript(const QString&)), this, SIGNAL(runAsPythonScript(const QString&)));
  m_uiForm.ckRunDiag->setChecked(true);
}

void Homer::setUpPage3()
{
  m_uiForm.ckRunAbsol->setToolTip("Normalise to calibration run(s)");

  // Update values on absolute tab with those from vanadium tab
  connect(m_uiForm.mapFile, SIGNAL(fileTextChanged(const QString&)), m_uiForm.absMapFile, SLOT(setFileText(const QString &)));

  connect(m_uiForm.leEGuess, SIGNAL(textChanged(const QString &)), this, SLOT(updateAbsEi(const QString &)));
  connect(m_uiForm.leVanEi, SIGNAL(textChanged(const QString&)), this, SLOT(validateAbsEi(const QString &)));
  connect(m_uiForm.leVanEi, SIGNAL(textEdited(const QString&)), this, SLOT(markAbsEiDirty()));

  connect(m_uiForm.ckRunAbsol, SIGNAL(toggled(bool)), m_uiForm.gbCalRuns, SLOT(setEnabled(bool)));
  connect(m_uiForm.ckRunAbsol, SIGNAL(toggled(bool)), m_uiForm.gbMasses, SLOT(setEnabled(bool)));
  connect(m_uiForm.ckRunAbsol, SIGNAL(toggled(bool)), m_uiForm.gbInteg, SLOT(setEnabled(bool)));
  m_uiForm.ckRunAbsol->setChecked(true);
}

/**
 * Validate the input to the form
 * @returns True if all input on the form is valid, false otherwise
 */
bool Homer::isInputValid() const
{
  bool valid = isFileInputValid();
  valid &= isParamInputValid();
  return valid;
}

/**
 * Validate the file input on the form
 * @returns True if all input on the form is valid, false otherwise
 */
bool Homer::isFileInputValid() const
{
  bool valid = m_uiForm.runFiles->isValid();
  int error_index(-1);
  valid &= m_uiForm.whiteBeamFile->isValid();
  valid &= m_uiForm.mapFile->isValid();
  if( !valid )
  {
    error_index = 0;
  }
  if( m_uiForm.ckRunAbsol->isChecked() )
  {
    valid &= m_uiForm.absRunFiles->isValid();
    valid &= m_uiForm.absWhiteFile->isValid();
    valid &= m_uiForm.absMapFile->isValid();
    if( !valid && error_index < 0 )
    {
      error_index = 2;
    }
  }
  if( error_index >= 0 )
  {
    m_uiForm.tabWidget->setCurrentIndex(error_index);
  }
  return valid;
}

/**
 *
 */
bool Homer::isParamInputValid() const
{
  bool valid = isRebinStringValid();
  int error_index(-1);

  if( m_uiForm.valGuess->isVisible() )
  {
    valid &= false;
  }
  else
  {
    valid &= true;
    error_index = 0;
  }
  
  if( m_uiForm.lbValAbsEi->isVisible() )
  {
    valid &= false;
    if( error_index < 0 ) error_index = 2;
  }
  else
  {
    valid &= true;
  }

  if( !valid ) 
  {
    m_uiForm.tabWidget->setCurrentIndex(error_index);
  }
  return valid;
}

/**
* Validate rebin parameters as a whole
*/
bool Homer::isRebinStringValid() const
{
  bool valid(false);
  QString rbParams("%1,%2,%3");
  rbParams = rbParams.arg(m_uiForm.leELow->text(), m_uiForm.leEWidth->text(), m_uiForm.leEHigh->text());
  Mantid::API::IAlgorithm_sptr rebin = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin");
  if( rebin )
  {
    rebin->initialize();
    try
    {
      rebin->setPropertyValue("Params", rbParams.toStdString());
      valid = true;
    }
    catch(...)
    {
      valid = false;
    }
    if( valid )
    {
      m_uiForm.gbRebin->setStyleSheet("QLineEdit {background-color: white}");
    }
    else
    {
      m_uiForm.gbRebin->setStyleSheet("QLineEdit {background-color: red}");
    }
  }
  else
  {
    valid = false;
    QMessageBox::critical(this->parentWidget(), "Homer", "Error creating Rebin algorithm, check algorithms have been loaded.");
  }
  return valid;
}

/**
 * Read the stored settings 
 */
void Homer::readSettings()
{
  QSettings settings;

  // Instrument specific
  QString currentGroup = getInstrumentSettingsGroup();
  settings.beginGroup(currentGroup);
 
  m_uiForm.ckFixEi->setChecked(settings.value("fixei", false).toBool());
  m_uiForm.ckSumSpecs->setChecked(settings.value("sumsps", false).toBool());
  m_uiForm.mapFile->setFileText(settings.value("map", "").toString()); 
    
  m_lastSaveDir = settings.value("save file dir", "").toString();
  m_lastLoadDir = settings.value("load file dir", "").toString();

  //File widget settings
  m_uiForm.runFiles->readSettings(currentGroup  + "/RunFilesFinder");
  m_uiForm.whiteBeamFile->readSettings(currentGroup  + "/WhiteBeamFileFinder");
  m_uiForm.absRunFiles->readSettings(currentGroup  + "/AbsRunFilesFinder");
  m_uiForm.absWhiteFile->readSettings(currentGroup  + "/AbsWhiteBeamFileFinder");
  
  settings.endGroup();
}

/** 
 * Save the form settings to the persistent store 
 */
void Homer::saveSettings()
{
  QSettings settings;

  // Instrument specific
  QString currentGroup = getInstrumentSettingsGroup();
  settings.beginGroup(currentGroup);
  settings.setValue("fixei", m_uiForm.ckFixEi->isChecked());
  settings.setValue("sumsps", m_uiForm.ckSumSpecs->isChecked());
  settings.setValue("map", m_uiForm.mapFile->getFirstFilename());

  settings.setValue("save file dir", m_lastSaveDir);
  settings.setValue("load file dir", m_lastLoadDir);

  //File widget settings
  m_uiForm.runFiles->saveSettings(currentGroup  + "/RunFilesFinder");
  m_uiForm.whiteBeamFile->saveSettings(currentGroup  + "/WhiteBeamFileFinder");
  m_uiForm.absRunFiles->saveSettings(currentGroup  + "/AbsRunFilesFinder");
  m_uiForm.absWhiteFile->saveSettings(currentGroup  + "/AbsWhiteBeamFileFinder");
  
  settings.endGroup();
}

/**
 * Return the general settings group
 */
QString Homer::getGeneralSettingsGroup() const
{
  return m_topSettingsGroup;
}

/**
 * Return the current instrument settings group
 */
QString Homer::getInstrumentSettingsGroup() const
{
  //The original Homer stored its settings in "CustomInterfaces/Homer/in instrument [PRE]"
  //where [PRE] is the instrument prefix so we will continue with this
  QString currentGroup = m_topSettingsGroup;
  if( !currentGroup.endsWith("/") )
  {
    currentGroup += "/";
  }
  QString prefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();
  currentGroup += QString("in instrument %1").arg(prefix);
  return currentGroup;
}

/**
 * Open a file dialog with extensions
 * @param save If true, then the dialog is a save dialog
 * @param exts A list of file extensions for the file filter
 */
QString Homer::openFileDia(const bool save, const QStringList &exts)
{
  QString filter;
  if ( !exts.empty() )
  {
    filter = "Files (";
    for ( int i = 0; i < exts.size(); i ++ )
    {
      filter.append("*." + exts[i] + " ");
    }
    filter.trimmed();
    filter.append(")");
  }
  filter.append(";;All Files (*.*)");

  QString filename;
  if(save)
  {
    filename = API::FileDialogHandler::getSaveFileName(this, "Save file", m_lastSaveDir, filter);
    if( !filename.isEmpty() )
    {
      m_lastSaveDir = QFileInfo(filename).absoluteDir().path();
    }
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file", m_lastLoadDir, filter);
    if( !filename.isEmpty() )
    {
      m_lastLoadDir = QFileInfo(filename).absoluteDir().path();
    }
  }
  return filename;
}

/**
 * Update the form settings when new background settings have been set
 */
void Homer::syncBackgroundSettings()
{
  if( m_backgroundDialog->removeBackground() )
  {
    m_uiForm.pbBack->setText("bg removal: on");
  }
  else
  {
    m_uiForm.pbBack->setText("bg removal: none");
  }
  // send the values to the detector diagnostics form, they are used as suggested values
  QPair<double,double> bgRange = m_backgroundDialog->getRange();
  emit MWDiag_updateTOFs(bgRange.first, bgRange.second);
}


/**
 * Validate the run file Ei on page 1
 * @param text The Ei value as a string
 */
void Homer::validateRunEi(const QString & text)
{
  if( checkEi(text) )
  {
    m_uiForm.valGuess->hide();
  }
  else
  {
    m_uiForm.valGuess->show();
  }
}

/**
 * Validate the abs run file Ei on page 3
 * @param text The Ei value as a string
 */
void Homer::validateAbsEi(const QString & text)
{
  if( checkEi(text) )
  {
    m_uiForm.lbValAbsEi->hide();
  }
  else
  {
    m_uiForm.lbValAbsEi->show();
  }
}

/**
 * Check the Ei input
 * @param text The text to validate as an ei guess
 */
bool Homer::checkEi(const QString & text) const
{
  bool valid(false);
  if( text.isEmpty() )
  {
    valid = false;
  }
  else
  {
    Mantid::API::IAlgorithm_sptr getei = Mantid::API::AlgorithmManager::Instance().createUnmanaged("GetEi");
    if( getei )
    {
      try
      {
        getei->initialize();
        getei->setProperty<double>("EnergyEstimate", text.toDouble());
        valid = true;
      }
      catch(...)
      {
        valid = false;
      }
    }
    else
    {
      QMessageBox::critical(this->parentWidget(), "Homer", "An error occurred creating the GetEi algorithm, check the algorithms have been loaded.");
      valid = false;
      m_uiForm.pbRun->setEnabled(false);
    }
  }
  return valid;
}

/**
 * Validate the rebin parameter boxes
 */
void Homer::validateRebinBox(const QString & text)
{
  QObject *origin = this->sender();
  QLineEdit *editor = qobject_cast<QLineEdit*>(origin);
  if( !editor ) return;
  QLabel *validLbl = m_validators.value(editor);
  if( !validLbl ) return;

  if( text.isEmpty() )
  {
    validLbl->show();
  }
  else
  {
    validLbl->hide();
  }

  if( !m_uiForm.leELow->text().isEmpty() && 
      !m_uiForm.leEWidth->text().isEmpty() && 
      !m_uiForm.leEHigh->text().isEmpty() )
  {
    isRebinStringValid();
  }
}

/** this runs after the run button was clicked. It runs runScripts()
*  and saves the settings on the form
*/
void Homer::runClicked()
{
  if( !isInputValid() )
  {
    return;
  }
  try
  {
    if (runScripts())
	  { 
      m_saveChanged = false;
      saveSettings();
	  }
  }
  catch (std::invalid_argument &e)
  {// can be caused by an invalid user entry that was detected
    QMessageBox::critical(this, "", QString::fromStdString(e.what()));
  }
  catch (std::runtime_error &e)
  {// possibly a Python run time error
    QMessageBox::critical(this, "", 
      QString::fromStdString(e.what()) + QString("  Exception encountered during execution"));
  }
  catch (std::exception &e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", QString::fromStdString(e.what()) +
      QString("  Exception encountered"));
  }
  
  pythonIsRunning(false);
}
/** Runnings everything, depending on what was entered on to the form
*  @throw out_of_range if there was an error reading user input but no validator could be displayed
*  @throw invalid_argument if some of the user entries are invalid
*  @throws runtime_error if there was a problem during execution of a Python script
*/
bool Homer::runScripts()
{
  // display the first page because it's likely any problems occur now relate to problems with settings here
  m_uiForm.tabWidget->setCurrentIndex(0);
  // constructing this builds the Python script, it is executed below
  deltaECalc unitsConv( this, m_uiForm, m_backgroundDialog->removeBackground(),
			m_backgroundDialog->getRange().first, m_backgroundDialog->getRange().second);
  connect(&unitsConv, SIGNAL(runAsPythonScript(const QString&)), this, SIGNAL(runAsPythonScript(const QString&)));
  
  // The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
  if (m_uiForm.ckRunDiag->isChecked())
  {
    // mostly important to stop the run button being clicked twice, prevents any change to the form until the run has completed
    pythonIsRunning(true);
    // display the second page in case errors occur in processing the user settings here
    m_uiForm.tabWidget->setCurrentIndex(1);
    QString errors = m_diagPage->run("diag_total_mask", true);

    if ( ! errors.isEmpty() )
    {
      pythonIsRunning(false); 
      throw std::invalid_argument(errors.toStdString());
    }
    // pass the bad detector list to the conversion script to enable masking
    unitsConv.setDiagnosedWorkspaceName("diag_total_mask");
  }
  else
  {
    unitsConv.setDiagnosedWorkspaceName("");
  }

  QStringList absRunFiles;
  QString absWhiteFile;
  if( m_uiForm.ckRunAbsol->isChecked() )
  {
    absRunFiles = m_uiForm.absRunFiles->getFilenames();
    absWhiteFile = m_uiForm.absWhiteFile->getFirstFilename();
  }
  unitsConv.createProcessingScript(m_uiForm.runFiles->getFilenames(), m_uiForm.whiteBeamFile->getFirstFilename(),
      absRunFiles, absWhiteFile, m_uiForm.leNameSPE->text());

  pythonIsRunning(true);
  // we're back to processing the settings on the first page
  m_uiForm.tabWidget->setCurrentIndex(0);
  QString errors = unitsConv.run();
  pythonIsRunning(false); 

  if ( !errors.isEmpty() )
  {
    throw std::runtime_error(errors.toStdString());
  }
  
  return errors.isEmpty();
}
//this function will be replaced a function in a widget
void Homer::browseSaveFile()
{
  QStringList extensions;
  extensions << "spe";

  QString filepath = this->openFileDia(true, extensions);
  if( filepath.isEmpty() ) return;
  QWidget *focus = QApplication::focusWidget();
  m_uiForm.leNameSPE->setFocus();
  m_uiForm.leNameSPE->setText(filepath);
  if( focus )
  {
    focus->setFocus();
  }
  else
  {
    m_uiForm.tabWidget->widget(0)->setFocus();
  }
}
/**
 * A slot to handle the help button click
 */
void Homer::helpClicked()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Homer"));
}

/** This slot updates the MWDiag and SPE filename suggestor with the
* names of the files the user has just chosen
*/
void Homer::runFilesChanged()
{
  if( !m_uiForm.runFiles->isValid() ) return;
  emit MWDiag_sendRuns(m_uiForm.runFiles->getFilenames());
  m_saveChanged = false;
  // the output file's default name is based on the input file names
  updateSaveName();
}

/** Check if the user has specified a name for the output SPE file,
* if not insert a name based on the name of the input files
*/
void Homer::updateSaveName()
{
  if ( !m_saveChanged ) 
  {
    m_uiForm.leNameSPE->setText(defaultName());
  }
}
/** update m_saveChanged with whether the user has changed the name away from the
*  default in this instance of the dialog box
*/
void Homer::saveNameUpd()
{
  if (m_saveChanged) return;
  m_saveChanged = m_uiForm.leNameSPE->text() != defaultName();
}

/** 
 * This slot passes the name of the white beam vanadium file to the MWDiag
 */
void Homer::updateWBV()
{
  if( m_uiForm.whiteBeamFile->isValid() )
  {
    emit MWDiag_updateWBV(m_uiForm.whiteBeamFile->getFirstFilename());
  }
}

/** Create a suggested output filename based on the supplied input
*  file names
*/
QString Homer::defaultName()
{
  try
  {//this will trhow if there is an invalid filename
    QStringList fileList = m_uiForm.runFiles->getFilenames();
    if ( fileList.size() == 0 )
    {
      return "";
    }
    if ( fileList.size() > 1 && ! m_uiForm.ckSumSpecs->isChecked() )
    {
      return "multiple-output-files";
    }
    // maybe normal operation: the output file name is based on the first input file
    return deltaECalc::SPEFileName(fileList.front());
  }
  catch (std::invalid_argument&)
  {
    return "";
  }
}

/** 
 * Creates and shows the background removal time of flight dialog
 */
void Homer::bgRemoveClick()
{
  connect(m_backgroundDialog, SIGNAL(rejected()), this, SLOT(bgRemoveReadSets()));
  connect(m_backgroundDialog, SIGNAL(accepted()), this, SLOT(bgRemoveReadSets()));
  m_uiForm.pbBack->setEnabled(false);
  m_uiForm.pbRun->setEnabled(false);
  m_backgroundDialog->show();
}

/** 
 * Runs when the background removal time of flight form is run
 */
void Homer::bgRemoveReadSets()
{
  // the user can press these buttons again, they were disabled before while the dialog box was up
  m_uiForm.pbBack->setEnabled(true);
  m_uiForm.pbRun->setEnabled(true);
  syncBackgroundSettings();
}

/**
 * Set the default parameters for the given instrument
 */
void Homer::setIDFValues(const QString &)
{
  if( !isPyInitialized() ) 
  {
    QMessageBox::information(this, "MantidPlot", "Error: Python not connected, cannot continue.");
    return;
  }

  QString prefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();

  // Fill in default values for tab
  QString param_defs = 
    "import DirectEnergyConversion as direct\n"
    "mono = direct.DirectEnergyConversion('%1')\n";
  param_defs = param_defs.arg(prefix);

  param_defs += 
    "print mono.monovan_integr_range[0]\n"
    "print mono.monovan_integr_range[1]\n"
    "print mono.van_mass\n"
    "print mono.background_range[0]\n"
    "print mono.background_range[1]\n"
    "print str(mono.background)\n";
  
  QString pyOutput = runPythonCode(param_defs).trimmed();
  QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);
  if( values.count() != 6 )
  {
    QMessageBox::critical(this->parentWidget(), "Homer", 
      "Error setting default parameter values.\n"
		  "Check instrument parameter file");
    return;
  }

  m_uiForm.leVanELow->setText(values[0]);
  m_uiForm.leVanEHigh->setText(values[1]);
  m_uiForm.leVanMass->setText(values[2]);

  m_backgroundDialog->setRange(values[3].toDouble(), values[4].toDouble());
  if( values[5] == "True" )
  {
    m_backgroundDialog->removeBackground(true);
  }
  else
  {
    m_backgroundDialog->removeBackground(false);
  }
  syncBackgroundSettings();

  m_uiForm.leSamMass->setText("1");
  m_uiForm.leRMMMass->setText("1");
  readSettings();
  
  // Also diag interface
  m_diagPage->loadSettings();
}

void Homer::saveFormatOptionClicked(QAbstractButton*)
{
  bool enabled(false);
  if( m_saveChecksGroup->checkedButton() )
  {
    enabled = true;
  }
  m_uiForm.leNameSPE->setEnabled(enabled);
  m_uiForm.pbBrowseSPE->setEnabled(enabled);
}

/**
 * If the user has not touched the absolute Ei entry they update it with the run value
 * @param text The Ei value
 */
void Homer::updateAbsEi(const QString & text)
{
  if( !m_absEiDirty )
  {
    m_uiForm.leVanEi->setText(text);
  }
}
/**
 * Mark the absolute Ei flag as dirty
 * @param dirty Boolean indicating the user has changed the field not the program
*/
void Homer::markAbsEiDirty(bool dirty)
{
  m_absEiDirty = dirty;

}
