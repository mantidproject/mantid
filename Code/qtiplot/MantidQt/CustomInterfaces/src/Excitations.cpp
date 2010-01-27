#include "MantidQtCustomInterfaces/Excitations.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QGridLayout>
#include <QStringList>
#include <QWidget>
#include <QUrl>
#include <QSignalMapper>
#include <QDesktopServices>
#include <QHeaderView>
#include <QFileDialog>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(Excitations);
  }
}
using namespace MantidQt::CustomInterfaces;

//these two defaults will be removed when other instruments are supported
static const QString G_INSTRUMENT("MAR");
static const QString G_DEFAULT_MAP_FILE("mari_res.map");

//default values
static const int G_NUM_NORM_SCHEMES = 4;
static const QString G_NORM_SCHEMES[G_NUM_NORM_SCHEMES] = 
  {"protons (uAh)", "no normalization", "monitor-monitor peak1",
  "monitor-peak2 area"};
static const QString G_DEFAULT_NORM = "monitor-monitor peak1";
static const QString G_BACK_REMOVE("bg removal: none");
static const double G_START_WINDOW_TOF(18000);
static const double G_END_WINDOW_TOF(19500);
static const bool G_USE_FIXED_EI(false);
static const bool G_SUM_SPECS(true);

// number of extensions for input files allowed
static const int G_NUM_INPUT_EXTS = 4;
// a list of the extensions for input files allowed
static const std::string G_INPUT_EXTS[G_NUM_INPUT_EXTS] =
  {"raw", "RAW", "NXS", "nxs"};
//----------------------
// Public member functions
//----------------------
///Constructor
Excitations::Excitations(QWidget *parent) : UserSubWindow(parent),
  m_diagPage(NULL), m_saveChanged(false), m_inFiles(NULL), m_busy(NULL)
{}
/// Set up the dialog layout
void Excitations::initLayout()
{
  // standard setting up of all widgets
  m_uiForm.setupUi(this);
   
  // the next statments add default vales, toolTips, etc.
  setSettingsGroup(setUpInstru());
  // don't change the order of these setUpPage*() statments
  setUpPage1();
  // they do the custom setting up like setting initial values tool tips on each of the three tab pages 
  setUpPage2();
  // but the initial values on each page can depend on the values in previous pages
  setUpPage3();
  
    // a helper widget for loading the second white beam vanadium run used to find bad detectors
//  RunNumbers *WBV2Widget = new RunNumbers;
//  QLayout *diagTest2Layout = m_uiForm.gbVariation->layout();
//  QGridLayout *diagTest2 = qobject_cast<QGridLayout*>(diagTest2Layout);
//  diagTest2->addWidget(WBV2Widget, 0, 0, 1, -1);
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_uiForm.pbWBV0, "pbWBV0");
  signalMapper->setMapping(m_uiForm.map_fileInput_pbBrowse, QString("map_fileInput_pbBrowse"));
  signalMapper->setMapping(m_uiForm.pbAddMono, QString("pbAddMono"));
  signalMapper->setMapping(m_uiForm.pbAddWhite, QString("pbAddWhite"));
  signalMapper->setMapping(m_uiForm.pbAddMap, QString("pbAddMap"));
  signalMapper->setMapping(m_uiForm.pbBrowseSPE, QString("pbBrowseSPE"));
  connect(m_uiForm.pbWBV0, SIGNAL(clicked()), signalMapper, SLOT(map()));

  connect(m_uiForm.map_fileInput_pbBrowse, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddMono, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddWhite, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddMap, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbBrowseSPE, SIGNAL(clicked()), signalMapper, SLOT(map()));
  
  connect(signalMapper, SIGNAL(mapped(const QString)),
         this, SLOT(browseClicked(const QString)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked())); 
  m_uiForm.pbRun->setToolTip("Process run files");
  m_uiForm.pbHelp->setToolTip("Online documentation (loads in a browser)");
}
/** Disables the form when passed the information that Python is running
*  and enables it when instructed that Pythons scripts have stopped
*  @param running if set to false only controls disabled by a previous call to this function will be re-enabled
*/
void Excitations::pythonIsRunning(bool running)
{// the run button was disabled when the results form was shown, as we can only do one analysis at a time, we can enable it now
  m_busy = running;
  m_uiForm.tabWidget->setEnabled( ! running );
  m_uiForm.pbRun->setEnabled( ! running );
}
/** Fill the instrument selection dialog box with the list of instruments
*  and set the current text to the one that was passed
*
*/
QString Excitations::setUpInstru()
{ // if there were no previously used instruments the "" below adds a blank entry. The empty string entry will always be there, even as more instruments are added
  QStringList prevInstrus =
    m_prev.value("CustomInterfaces/Excitations/instrusList","").toStringList();

	/*??STEVES ?? get rid of this when more instruments are supported*/if ( ! prevInstrus.contains(G_INSTRUMENT) ) prevInstrus.prepend(G_INSTRUMENT);
  QStringList::const_iterator instru = prevInstrus.begin();
  for ( ; instru != prevInstrus.end(); ++instru )
  {
    m_uiForm.loadRun_cbInst->addItem(*instru);
  }
  
  QString curInstru
    = m_prev.value("CustomInterfaces/Excitations/instrument", G_INSTRUMENT).toString();
  m_uiForm.loadRun_cbInst->setEditText(curInstru);
  
 // insert the file loader helper widget
  //??STEVES?? implement the file widget
  m_inFiles = new deltaECalc::FileInput(*m_uiForm.loadRun_lenumber, *m_uiForm.loadRun_cbInst);
/*  FileInput *m_inFiles = new FileInput;
  QLayout *mapLayout = m_uiForm.gbrunMap->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout);
  mapLay->addWidget(m_inFiles, 0, 0, 1, -1);*/
  connect(m_uiForm.loadRun_pbBrowse, SIGNAL(clicked()), this, SLOT(addRunFile()));

  return curInstru;
}
/// For each widgets in the first tab this adds custom widgets, fills in combination boxes and runs setToolTip()
void Excitations::setUpPage1()
{
  page1setUpNormCom();
  page1Defaults();
  page1Validators();
  page1Tooltips();
  
  connect(m_uiForm.pbBack, SIGNAL(clicked()), this, SLOT(bgRemoveClick()));

  // SIGNALS and SLOTS that deal with coping the text from one edit box to another 
  connect(m_uiForm.loadRun_lenumber, SIGNAL(editingFinished()), this, SLOT(runFilesChanged()));
  connect(m_uiForm.ckSumSpecs, SIGNAL(stateChanged(int)), this, SLOT(updateSaveName()));
  connect(m_uiForm.leNameSPE, SIGNAL(editingFinished()), this, SLOT(saveNameUpd()));
  connect(m_uiForm.leWBV0, SIGNAL(editingFinished()), this, SLOT(updateWBV()));
      
  m_uiForm.lbPrefix->setToolTip("For example MAR, MAP, ...");
  m_uiForm.loadRun_cbInst->setToolTip("For example MAR, MAP, ...");
}
/// put default values into the controls in the first tab
void Excitations::page1setUpNormCom()
{
  m_uiForm.cbNormal->addItem("monitor");
  for ( int i = 0; i < G_NUM_NORM_SCHEMES; ++i )
  {
    QString displayName = removeStrMonitor(G_NORM_SCHEMES[i]);
	if (displayName == G_NORM_SCHEMES[i])
	{// these means that the normalisation scheme doesn't include the word monitor and so we don't need the second combobox
	  m_uiForm.cbNormal->addItem(G_NORM_SCHEMES[i]);
	}
	else // this is a monitor based normalisation scheme add the name to the second combobox
	{
	  m_uiForm.cbMonitors->addItem(displayName);
	}
  }
  
  connect(m_uiForm.cbNormal, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setupNormBoxes(const QString &)));
}
/** Removes the string "monitor-" from the startt of the string that is passed. If the string
*  doesn't start with "monitor-" a copy of the string that was originally passed is returned
*  @param the string in which to test the presence of the string monitor
*  @return the string that was passed with any initial string equal to "monitor-" removed
*/
QString Excitations::removeStrMonitor(const QString &check)
{
  if (check.startsWith("monitor-"))
  {
    QStringRef end = check.rightRef(check.count()-QString("monitor-").count());
	return end.toString();
  }
  return check;
}

/// put default values into the controls in the first tab
void Excitations::page1Defaults()
{
  // unchanging defaults
  m_uiForm.leScale->setText("0");

  QString normalise = m_prev.value("normalise", G_DEFAULT_NORM).toString();
  // if the normalisation scheme is monitor based then it will contain the name monitor
  QString displayName = removeStrMonitor(normalise);  
  if ( displayName == normalise )
  {// this is not a monitor based normalisation scheme, the simple case
    enableSecondBox(false);
    int blankInd = m_uiForm.cbNormal->findText(displayName);
	m_uiForm.cbNormal->setCurrentIndex(blankInd);
  }
  else
  {// the normalisation scheme requires the second comobox box
    enableSecondBox(true);
    int blankInd = m_uiForm.cbNormal->findText("monitor");
	m_uiForm.cbNormal->setCurrentIndex(blankInd);
    blankInd = m_uiForm.cbMonitors->findText(displayName);
	m_uiForm.cbMonitors->setCurrentIndex(blankInd);
  }
 
  // the value that is used when the form is loaded for the first time is included below on later loadings a saved setting is used
  m_uiForm.ckFixEi->setChecked(m_prev.value("fixei", G_USE_FIXED_EI).toBool());
  m_uiForm.ckSumSpecs->setChecked(m_prev.value("sumsps", G_SUM_SPECS).toBool());

  m_uiForm.pbBack->setText(
    m_prev.value("bgremove", G_BACK_REMOVE).toString()); 

  // the statements below only do something the first time a user runs this form on an instrument, it inserts the default values
  m_prev.setValue("bgremove",
    m_prev.value("bgremove", G_BACK_REMOVE).toString());
  m_prev.setValue("TOFstart",
    m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble());
  m_prev.setValue("TOFend",
    m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());

  m_uiForm.map_fileInput_leName->setText(
    m_prev.value("map", G_DEFAULT_MAP_FILE).toString());
}
/// make validator labels and associate them with the controls that need them in the first tab
void Excitations::page1Validators()
{
  m_validators.clear();
  m_validators[m_uiForm.loadRun_lenumber] = newStar(m_uiForm.gbExperiment, 1, 5);

  setupValidator(m_uiForm.valBg);
  m_validators[m_uiForm.pbBack] = m_uiForm.valBg;

  setupValidator(m_uiForm.valWBV);
  setupValidator(m_uiForm.valMap);
  m_validators[m_uiForm.leWBV0] = m_uiForm.valWBV;
  m_validators[m_uiForm.map_fileInput_leName] = m_uiForm.valMap;
  
  m_validators[m_uiForm.leWBV0Low] = newStar(m_uiForm.gbExperiment, 6, 4);
  m_validators[m_uiForm.leWBV0High] = m_validators[m_uiForm.leWBV0Low];
  
  setupValidator(m_uiForm.valGuess);
  m_validators[m_uiForm.leEGuess] = m_uiForm.valGuess;
  
  m_validators[m_uiForm.leEHigh] = newStar(m_uiForm.gbConvUnits, 2, 8);
  setupValidator(m_uiForm.valSPE);
  m_validators[m_uiForm.leNameSPE] = m_uiForm.valSPE;
  
  hideValidators();
}
void Excitations::setupValidator(QLabel *star)
{
  QPalette pal = star->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  star->setPalette(pal);
}
QLabel* Excitations::newStar(const QGroupBox * const UI, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  QGridLayout *grid = qobject_cast<QGridLayout*>(UI->layout());
  grid->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
void Excitations::hideValidators()
{// loop through all the validators in the map
  QHash<const QWidget * const, QLabel *>::iterator vali = m_validators.begin();
  for ( ; vali != m_validators.end(); ++vali)
  {
    vali.value()->hide();    
  }
}
/// set all the tooltips for the first tab
void Excitations::page1Tooltips()
{  
  m_uiForm.gbExperiment->setToolTip("Files to process");
  m_uiForm.loadRun_lbDiscrip->setToolTip("List of runs to load"), m_uiForm.loadRun_lenumber->setToolTip("List of runs to load"), m_uiForm.loadRun_pbBrowse->setToolTip("List of runs to load");
  m_uiForm.ckSumSpecs->setToolTip("If this box is not ticked there will be one output file for each input, otherwise the output will be summed into one file");
  m_uiForm.ckSumSpecs->setToolTip("If this box is not ticked there will be one output file for each input, otherwise the output will be summed into one file");
  m_uiForm.ckFixEi->setToolTip("Leave unticked for the algorithm GetEi to calculate a the incident neutron energy based on the monitor signals and the guess below");
//??STEVES?? this code will go to the run number to file name widget
//  m_uiForm.loadRun_tvRuns->setToolTip("List of runs to load");
//  m_uiForm.loadRun_tvRuns->setColumnCount(3);
//  m_uiForm.loadRun_tvRuns->setRowCount(1);
//  m_uiForm.loadRun_tvRuns->horizontalHeader()->hide();
//  m_uiForm.loadRun_tvRuns->verticalHeader()->hide();
  m_uiForm.lbNorm->setToolTip("Select the type of normalization for the runs"), m_uiForm.cbNormal->setToolTip("Select the type of normalization for the runs");
  m_uiForm.cbMonitors->setToolTip("If normalization to monitor was selected");
  m_uiForm.lbScale->setToolTip("Multiply numbers of counts by this power of 10");
  m_uiForm.leScale->setToolTip("Multipling numbers by a large constant can make plotting easier");
  m_uiForm.lbWBV0->setToolTip("The white beam vanadium run selected here will set the default for finding bad detectors and absolute units conversion"), m_uiForm.leWBV0->setToolTip("The white beam vanadium run picked here will set the default for finding bad detectors and absolute units conversion"), m_uiForm.pbWBV0->setToolTip("The white beam vanadium run picked here will set the default for finding bad detectors and absolute units conversion");
  m_uiForm.lbWBV0Low1->setToolTip("Energy range for the white beam normalisation"); m_uiForm.lbWBV0Low2->setToolTip("Energy range for the white beam normalisation");
  m_uiForm.lbWBV0High1->setToolTip("Energy range for the white beam normalisation"); m_uiForm.lbWBV0High2->setToolTip("Energy range for the white beam normalisation");
  m_uiForm.map_fileInput_lbName->setToolTip("Sum spectra into groups defined by this file (passed to GroupDetectors)"), m_uiForm.map_fileInput_leName->setToolTip("Sum spectra into groups defined by this file (passed to GroupDetectors)"), m_uiForm.map_fileInput_pbBrowse->setToolTip("Sum spectra into groups defined by this file (passed to GroupDetectors)");

  m_uiForm.gbConvUnits->setToolTip("Settings for units conversion to energy transfer");
  m_uiForm.lbEGuess1->setToolTip("Approximate initial neutron energy, is passed to GetEi"), m_uiForm.leEGuess->setToolTip("Approximate initial neutron energy, is passed to GetEi");
  m_uiForm.lbEBins->setToolTip("Settings for units conversion to energy transfer (passed to ReBin)");
  m_uiForm.lbELow->setToolTip("Exclude neutrons with less than this energy (meV)"), m_uiForm.leELow->setToolTip("Exclude neutrons with less than this energy (meV)");
  m_uiForm.lbEHigh->setToolTip("Exclude neutrons with more than this energy (meV)"), m_uiForm.leEHigh->setToolTip("Exclude neutrons with more than this energy (meV)");
  m_uiForm.lbEWidth->setToolTip("Width of the energy bins (meV)"), m_uiForm.leEWidth->setToolTip("Width of the energy bins (meV)");

  m_uiForm.lbSPE->setToolTip("File name for the converted data"), m_uiForm.leNameSPE->setToolTip("File name for the converted data"), m_uiForm.pbBrowseSPE->setToolTip("File name for the converted data");
}
/// Adds the diag custom widgets and a check box to allow users to enable or disable the widget
void Excitations::setUpPage2()
{/* The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
 this second page is largely filled with the diag widget*/
  // previous settings, second argument, depends on the instrument and the detector diagnostic settings are kept separate in "diag/"
  m_diagPage = new MantidWidgets::MWDiag(this, m_prev.group()+"/diag");

  // set the default background region to the same as the default on this form
  emit MWDiag_updateTOFs(m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble(),
    m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());
	
  // insert the widgets on to the second page (index = 1)
  QLayout *mapLayout = m_uiForm.tabWidget->widget(1)->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout); 
  if ( mapLay )
  { // this should always be true because we setup a grid layout in the designer
    mapLay->addWidget(m_diagPage, 1, 0, 6, 5);
  }

  m_uiForm.ckRunDiag->setToolTip("Enable or disable all the controls on this page");
  // either enables or disables the detector diagnostics page depending on if the check box is clicked or not
  disenableDiag();
  connect(m_uiForm.ckRunDiag, SIGNAL(clicked()), this, SLOT(disenableDiag()));
}

void Excitations::setUpPage3()
{
  m_uiForm.leVanMap->setText("mari_res.map");
  m_uiForm.leVanMass->setText("32.58");
  m_uiForm.leSamMass->setText("1");
  m_uiForm.leRMMMass->setText("1");
  m_uiForm.leVanELow->setText("-1");
  m_uiForm.leVanEHigh->setText("1");

  m_uiForm.ckRunAbsol->setToolTip("Convert to absolute units");

  m_uiForm.gbCalRuns->setToolTip("Load Calibration Runs");
  QString runWSMap = "Sum spectra in groups defined by this file (passed to GroupDetectors)";
  m_uiForm.leVanMap->setToolTip(runWSMap);
  m_uiForm.pbAddMap->setToolTip(runWSMap);
    
  m_uiForm.leVanELow->setToolTip("Lowest energy to include in the integration");
  m_uiForm.lbVanELow1->setToolTip("Lowest energy to include in the integration");
  m_uiForm.lbVanELow2->setToolTip("Lowest energy to include in the integration");
    
  m_uiForm.leVanEHigh->setToolTip("Highest energy to include in the integration");
  m_uiForm.lbVanEHigh1->setToolTip("Highest energy to include in the integration");
  m_uiForm.lbVanEHigh2->setToolTip("Highest energy to include in the integration");
  // disables or enable this page depending on the clicked state of the check at the top of the form
  disenableAbsolute();
  connect(m_uiForm.ckRunAbsol, SIGNAL(clicked()), this, SLOT(disenableAbsolute()));
}
/** is run if the Python scripts complete successfully, enters a slection of the values
*  entered on the form into the QSettings database (Window's registry, Linux .file, etc.)
*/
void Excitations::saveSettings()
{  
  m_prev.endGroup();

  QString instrument = m_uiForm.loadRun_cbInst->currentText();
  m_prev.setValue("CustomInterfaces/Excitations/instrument", instrument); 
  
  QStringList prevInstrus =
    m_prev.value("CustomInterfaces/Excitations/instrusList","").toStringList();
  if ( ! prevInstrus.contains(instrument) )
  {
    prevInstrus.append(instrument);
    // put the instrument list alphabetic order to make it easier to use
    prevInstrus.sort();
  }
  m_prev.setValue("CustomInterfaces/Excitations/instrumsList", prevInstrus);
  
  // where settings are stored (except the list of previously used instruments) is dependent on the instrument selected
  setSettingsGroup(instrument);

  if (m_uiForm.cbNormal->currentText() == "monitor")
  {
    m_prev.setValue("normalise","monitor-"+m_uiForm.cbMonitors->currentText());
  }
  else
  {
    m_prev.setValue("normalise", m_uiForm.cbNormal->currentText());
  }

  m_prev.setValue("fixei", m_uiForm.ckFixEi->isChecked());
  m_prev.setValue("sumsps", m_uiForm.ckSumSpecs->isChecked());
	
  m_prev.setValue("map", m_uiForm.map_fileInput_leName->text());
}
// ??STEVES?? move this function to the file widget?
QString Excitations::openFileDia(const bool save, const QStringList &exts)
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
    filename = QFileDialog::getSaveFileName(this, "Save file",
	  m_prev.value("save file dir", "").toString(), filter);
	if( ! filename.isEmpty() )
	{
	  m_prev.setValue("save file dir", QFileInfo(filename).absoluteDir().path());
	}
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file",
	  m_prev.value("load file dir", "").toString(), filter);
	if( ! filename.isEmpty() )
	{
	  m_prev.setValue("load file dir", QFileInfo(filename).absoluteDir().path());
	}
  }
  return filename;
} 
/** the form entries that are saved are stored under a directory like string
*  in QSettings tht is dependent on the instrument, this is set up here
*/
void Excitations::setSettingsGroup(const QString &instrument)
{
  m_prev.beginGroup("CustomInterfaces/Excitations/in instrument "+instrument);
}
/** this runs after the run button was clicked. It runs runScripts()
*  ans saves the settings on the form
*/
void Excitations::runClicked()
{
  hideValidators();
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
    QMessageBox::critical(this, "", QString(e.what()));
  }
  catch (std::runtime_error &e)
  {// possibly a Python run time error
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
  }
  catch (std::exception &e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered"));
  }
  
  pythonIsRunning(false);
}
/** Runnings everything, depending on what was entered on to the form
*  @throw out_of_range if there was an error reading user input but no validator could be displayed
*  @throw invalid_argument if some of the user entries are invalid
*  @throws runtime_error if there was a problem during execution of a Python script
*/
bool Excitations::runScripts()
{
  // constructing this builds the Python script, it is executed below
  deltaECalc unitsConv( this, m_uiForm, *m_inFiles,
    m_prev.value("bgremove", G_BACK_REMOVE).toString() == "bg removal: on",
	m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble(),
	m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());
    
  QString errors("");
  
  try
  {// if this function finds a control with an invalid entry the control is marked with a star and some information is returned here
    errors = unitsConv.checkNoErrors(m_validators);
	if ( ! errors.isEmpty() )
	{
      throw std::invalid_argument(errors.toStdString());
    }
  }
  catch(std::exception)
  {// it's likely the problem come from somewhere on this page
    m_uiForm.tabWidget->setCurrentIndex(0);
    throw;
  }

  try
  {
    // The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
    if (m_uiForm.ckRunDiag->isChecked())
    {
      QString maskOutWS =
	    "mask_"+MantidWidgets::MantidWidget::removePath(m_inFiles->getFile1());
	  // mostly important to stop the run button being clicked twice, prevents any change to the form until the run has completed
      pythonIsRunning(true);
	  errors = m_diagPage->run(maskOutWS, true);
	  if ( ! errors.isEmpty() )
	  {
        throw std::invalid_argument(errors.toStdString());
      }
	  // pass the bad detector list to the conversion script to enable masking
	  unitsConv.maskDetects(maskOutWS);
    }
  }
  catch(std::exception)
  {// it's likely the problem come from somewhere on this page
    m_uiForm.tabWidget->setCurrentIndex(1);
    throw;
  }
  
  try
  {
    pythonIsRunning(true);
    //unitsConv is always executed, the user can't switch this off, unless there's an error on the form. To examine the script that is executed uncomment QMessageBox::critical(this, "", unitsConv.python());
    errors = unitsConv.run();
	
	if ( ! errors.isEmpty() )
	{
      throw std::runtime_error(errors.toStdString());
	}
  }
  catch (std::exception)
  {// it's likely the problem come from somewhere on this page
    m_uiForm.tabWidget->setCurrentIndex(0);
    pythonIsRunning(false); 
    throw;
  }
  
  pythonIsRunning(false); 
  return errors.isEmpty();
}
//this function will be replaced a function in a widget
void Excitations::browseClicked(const QString buttonDis)
{
  QLineEdit *editBox = NULL;
  QStringList extensions;
  bool toSave = false;

  if (buttonDis == "pbWBV0")
  {
    editBox = m_uiForm.leWBV0;
	for ( int i = 0; i < G_NUM_INPUT_EXTS; i++)
	{
	  extensions << QString::fromStdString(G_INPUT_EXTS[i]);
	}
  }
  if ( buttonDis == "map_fileInput_pbBrowse" )
  {
    editBox = m_uiForm.map_fileInput_leName;
    extensions << "MAP"<< "map";
  }
  if ( buttonDis == "pbAddMono" )
  {
    editBox = m_uiForm.leMonoVan;
	for ( int i = 0; i < G_NUM_INPUT_EXTS; i++)
	{
	  extensions << QString::fromStdString(G_INPUT_EXTS[i]);
	}
  }
  if ( buttonDis == "pbAddWhite")
  {
    editBox = m_uiForm.leWhiteVan;
	for ( int i = 0; i < G_NUM_INPUT_EXTS; i++)
	{
	  extensions << QString::fromStdString(G_INPUT_EXTS[i]);
	}
  }
  if ( buttonDis == "pbAddMap" )
  {
    editBox = m_uiForm.leVanMap;
    extensions << "MAP"<< "map";
  }
  if ( buttonDis == "pbBrowseSPE")
  {
    editBox = m_uiForm.leNameSPE;
    extensions << "spe";
    toSave = true;
  }
  
  if (! editBox ) throw std::invalid_argument("unlinked browse button");
  
  QString filepath = this->openFileDia(toSave, extensions);
  if( filepath.isEmpty() ) return;
  editBox->setText(filepath);
  
  // the diag widget wants to know if a white beam vanadium file was loaded as its algorithm needs one too
  if ( buttonDis == "pbWBV0" )
  {
    emit MWDiag_updateWBV(m_uiForm.leWBV0->text());
  }
}
//function will be replaced a function in a widget
void Excitations::addRunFile()
{
  QStringList extensions;
  for ( int i = 0; i < G_NUM_INPUT_EXTS; i++)
  {
	extensions << QString::fromStdString(G_INPUT_EXTS[i]);
  }

  if( ! m_uiForm.loadRun_lenumber->text().isEmpty() )
  {
    QString dir =
	  QFileInfo(m_uiForm.loadRun_lenumber->text()).absoluteDir().path();
	m_prev.setValue("directories/runfile", dir);
  }

  QString uFile = this->openFileDia(false, extensions);
  if( uFile.isEmpty() ) return;

  if ( ! m_uiForm.loadRun_lenumber->text().isEmpty() )
  {
    m_uiForm.loadRun_lenumber->setText(
	  m_uiForm.loadRun_lenumber->text()+", " + uFile);
  }
  else m_uiForm.loadRun_lenumber->setText(uFile);
  
  runFilesChanged();
}
/**
 * A slot to handle the help button click
 */
void Excitations::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
    "Detector Efficiency Tests"));
}
/** Enables or disables the absoulte unit conversion cocntrols based
*  on whether or not the check box has been checked
*/
void Excitations::disenableAbsolute()
{
  const bool enabled = m_uiForm.ckRunAbsol->isChecked();
  m_uiForm.gbCalRuns->setEnabled(enabled);
  m_uiForm.gbMasses->setEnabled(enabled);
  m_uiForm.gbInteg->setEnabled(enabled);
 }

/** Enables or disables the find bad detectors controls based
*  on whether or not the check box has been checked
*/
void Excitations::disenableDiag()
{
  m_diagPage->setEnabled(m_uiForm.ckRunDiag->isChecked());
}
void Excitations::runFilesChanged()
{// this signal to the diag GUI allows the run files we choose here to be the default for its background correction
  emit MWDiag_sendRuns(m_inFiles->getRunString());
  // the output file's default name is based on the input file names
  updateSaveName();
}
/** Check if the user has specified a name for the output SPE file,
* if not insert a name based on the name of the input files
*/
void Excitations::updateSaveName()
{// if the user added their own value prevent it from being changed
  if ( ! m_saveChanged ) 
  {
    m_uiForm.leNameSPE->setText(defaultName());
  }
}
/** update m_saveChanged with whether the user has changed the name away from the
*  default in this instance of the dialog box
*/
void Excitations::saveNameUpd()
{// if the user had already altered the contents of the box it has been noted that the save name is under user control so do nothing
  if (m_saveChanged) return;
  m_saveChanged = m_uiForm.leNameSPE->text() != defaultName();
}
void Excitations::updateWBV()
{
  emit MWDiag_updateWBV(m_uiForm.leWBV0->text());
}
/** enables or disables the list of monitors depending on the whether
* the monitor was set in the normalisation combobox
*  @param the text in the normalisastion combobox
*  throw invalid_argument if neither the chosen normalisation scheme or the default can be found
*/
void Excitations::setupNormBoxes(const QString &newText)
{
  enableSecondBox( newText == "monitor" );
}
/** When true is passed to this function the second dialog box is enabled
*  _without_ a blank entry or other it is disabled and left blank
*  @param toEnable true if the second box will be used
*/
void Excitations::enableSecondBox(bool toEnable)
{
  int blankInd = m_uiForm.cbMonitors->findText("");
  if (toEnable && blankInd > -1)
  {// we are going to enable the combobox so we can't have an empty string in its contents
    m_uiForm.cbMonitors->removeItem(blankInd);
  }
  if ( ! toEnable )
  {// when the comobox is disabled, it should show blank
    if ( blankInd < 0 )
	{
      m_uiForm.cbMonitors->addItem("");
	}
    blankInd = m_uiForm.cbMonitors->findText("");
	m_uiForm.cbMonitors->setCurrentIndex(blankInd);
  }
  m_uiForm.cbMonitors->setEnabled(toEnable);
}

/** Create a suggested output filename based on the supplied input
*  file names
*/
QString Excitations::defaultName()
{
  const std::vector<std::string> &fileList = m_inFiles->getRunFiles();
  if ( fileList.size() == 0 )
  {// no input files we can't say anything about the output files
    return "";
  }
  if ( fileList.size() > 1 && ! m_uiForm.ckSumSpecs->isChecked() )
  {// multiple input files that are not summed give rise to multiple output files. Prepare to give the output files names that corrospond to the input filenames
    return "";
  }
  // maybe normal operation: the output file name is based on the first input file
  return deltaECalc::SPEFileName(fileList.front());
}
/** creates and shows the background removal time of flight form
*/
void Excitations::bgRemoveClick()
{
  Background *bgRemovDialog = new Background(this, m_prev.group());
  connect(bgRemovDialog, SIGNAL(formClosed()),
          this, SLOT(bgRemoveReadSets()));
  m_uiForm.pbBack->setEnabled(false);
  m_uiForm.pbRun->setEnabled(false);
  bgRemovDialog->show();
}
/** runs when the background removal time of flight form is run
*/
void Excitations::bgRemoveReadSets()
{ // the user can press these buttons again, they were disabled before while the dialog box was up
  m_uiForm.pbBack->setEnabled(true);
  m_uiForm.pbRun->setEnabled(true);
  
  m_uiForm.pbBack->setText(
    m_prev.value("bgremove", G_BACK_REMOVE).toString());

  // send the values to the detector diagnostics form, they are used as suggested values
  emit MWDiag_updateTOFs(m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble(),
    m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());
}