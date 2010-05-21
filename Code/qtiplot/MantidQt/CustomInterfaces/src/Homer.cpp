#include "MantidQtCustomInterfaces/Homer.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "Poco/Path.h"
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
    DECLARE_SUBWINDOW(Homer);
  }
}
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;

//default values
static const int G_NUM_NORM_SCHEMES = 3;
static const QString G_NORM_SCHEMES[G_NUM_NORM_SCHEMES] = 
{"protons (uAh)", "no normalization", "monitor-monitor 1"};
//  , "monitor-monitor 2"};
static const QString G_DEFAULT_NORM = "monitor-monitor 1";
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
Homer::Homer(QWidget *pare) : UserSubWindow(pare),
  m_mantidplot(pare), m_runFilesWid(NULL),m_diagPage(NULL),m_saveChanged(false)
{}
/// Set up the dialog layout
void Homer::initLayout()
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
  
  // the signal mapper is used to link both browse buttons on the form on to a load file dialog
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_uiForm.map_fileInput_pbBrowse, QString("map_fileInput_pbBrowse"));
  signalMapper->setMapping(m_uiForm.pbBrowseSPE, QString("pbBrowseSPE"));
  connect(m_uiForm.map_fileInput_pbBrowse, SIGNAL(clicked()), signalMapper, SLOT(map()));
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
void Homer::pythonIsRunning(bool running)
{// the run button was disabled when the results form was shown, as we can only do one analysis at a time, we can enable it now
  m_uiForm.tabWidget->setEnabled( ! running );
  m_uiForm.pbRun->setEnabled( ! running );
  m_diagPage->blockPython(running);
}
/** Fill the instrument selection dialog box with the list of instruments
*  and set the current text to the one that was passed
*/
QString Homer::setUpInstru()
{ 
  // Populate the prefix box with the known instruments and set the default
  Mantid::Kernel::ConfigServiceImpl & mtd_config = Mantid::Kernel::ConfigService::Instance();
  // It's easier here to populate the combobox with a QStringList which can be formed using the split method
  // than using getInstrumentPrefixes on the ConfigService
  std::string key = std::string("instrument.prefixes.") + mtd_config.getString("default.facility");
  QString prefixes = QString::fromStdString(mtd_config.getString(key));
  QStringList pref_list = prefixes.split(";", QString::SkipEmptyParts);
  m_uiForm.loadRun_cbInst->clear();
  m_uiForm.loadRun_cbInst->addItems(pref_list);

  QString curInstru = m_prev.value("CustomInterfaces/Homer/instrument", "").toString();
  int index = m_uiForm.loadRun_cbInst->findText(curInstru);
  if( index < 0 )
  {
    curInstru = QString::fromStdString(mtd_config.getString("default.instrument"));
    index =  m_uiForm.loadRun_cbInst->findText(curInstru);
    if( index < 0 )
    {
      index = 0;
    }
  }
  m_uiForm.loadRun_cbInst->setCurrentIndex(index);
  return curInstru;
}
/// For each widgets in the first tab this adds custom widgets, fills in combination boxes and runs setToolTip()
void Homer::setUpPage1()
{
  page1FileWidgs();
  page1setUpNormCom();
  page1Defaults();
  page1Validators();
  page1Tooltips();

  // Force a check of the instrument
  instrSelectionChanged(m_uiForm.loadRun_cbInst->currentText());
  connect(m_uiForm.loadRun_cbInst, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(instrSelectionChanged(const QString&)));
  
  connect(m_uiForm.pbBack, SIGNAL(clicked()), this, SLOT(bgRemoveClick()));

  // SIGNALS and SLOTS that deal with coping the text from one edit box to another 
  connect(m_uiForm.ckSumSpecs, SIGNAL(stateChanged(int)), this, SLOT(updateSaveName()));
  connect(m_uiForm.leNameSPE, SIGNAL(editingFinished()), this, SLOT(saveNameUpd()));
}
/// put default values into the controls in the first tab
void Homer::page1FileWidgs()
{
  // we create and place some new widgets on the form using it's grid layout
  QLayout *mapLayout = m_uiForm.gbExperiment->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout); 
  // this requires that the form was setup with a grid layout
  if ( ! mapLay )
  { // if you see the following exception check that the group box gbExperiment has a grid layout
    throw Exception::NullPointerException("Problem with the layout in the first tab", "mapLay");
  }

  m_runFilesWid = new MWRunFiles(this, m_prev.group()+"/runs", false,
	m_uiForm.loadRun_cbInst, "Run Files", "List of runs to load");
  //??STEVES?? code for if the selected files will be listed in a table
//  m_uiForm.loadRun_tvRuns->setColumnCount(3);
//  m_uiForm.loadRun_tvRuns->setRowCount(1);
//  m_uiForm.loadRun_tvRuns->horizontalHeader()->hide();
//  m_uiForm.loadRun_tvRuns->verticalHeader()->hide();
  mapLay->addWidget(m_runFilesWid, 0, 0, 1, 7);
  connect(m_runFilesWid, SIGNAL(fileChanged()), this, SLOT(runFilesChanged()));

  m_WBVWid = new MWRunFile(this, m_prev.group()+"/WBV", false,
	m_uiForm.loadRun_cbInst, "White Beam Van",
	"This white beam vanadium run also sets the defaults\n"
	"in Diagnose Detectors and Absolute Units");
  mapLay->addWidget(m_WBVWid, 4, 0, 1, 7);
  connect(m_WBVWid, SIGNAL(fileChanged()), this, SLOT(updateWBV()));
}

/// put default values into the controls in the first tab
void Homer::page1setUpNormCom()
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
  
  connect(m_uiForm.cbNormal, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(setupNormBoxes(const QString &)));
}
/** Removes the string "monitor-" from the startt of the string that is passed. If the string
*  doesn't start with "monitor-" a copy of the string that was originally passed is returned
*  @param the string in which to test the presence of the string monitor
*  @return the string that was passed with any initial string equal to "monitor-" removed
*/
QString Homer::removeStrMonitor(const QString &check)
{
  if (check.startsWith("monitor-"))
  {
    QStringRef end = check.rightRef(check.count()-QString("monitor-").count());
	return end.toString();
  }
  return check;
}
/// put default values into the controls in the first tab
void Homer::page1Defaults()
{
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

  //m_uiForm.map_fileInput_leName->setText(m_prev.value("map", G_DEFAULT_MAP_FILE).toString());
}
/// make validator labels and associate them with the controls that need them in the first tab
void Homer::page1Validators()
{
  m_validators.clear();

  setupValidator(m_uiForm.valBg);
  m_validators[m_uiForm.pbBack] = m_uiForm.valBg;

  setupValidator(m_uiForm.valMap);
  m_validators[m_uiForm.map_fileInput_leName] = m_uiForm.valMap;
  
  setupValidator(m_uiForm.valGuess);
  m_validators[m_uiForm.leEGuess] = m_uiForm.valGuess;
  
  m_validators[m_uiForm.leEHigh] = newStar(m_uiForm.gbConvUnits, 2, 9);
  setupValidator(m_uiForm.valSPE);
  m_validators[m_uiForm.leNameSPE] = m_uiForm.valSPE;
  
  hideValidators();
}
void Homer::setupValidator(QLabel *star)
{
  QPalette pal = star->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  star->setPalette(pal);
}
QLabel* Homer::newStar(const QGroupBox * const UI, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  QGridLayout *grid = qobject_cast<QGridLayout*>(UI->layout());
  grid->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
void Homer::hideValidators()
{// loop through all the validators in the map
  QHash<const QWidget * const, QLabel *>::iterator vali = m_validators.begin();
  for ( ; vali != m_validators.end(); ++vali)
  {
    vali.value()->hide();    
  }
}
/// set all the tooltips for the first tab
void Homer::page1Tooltips()
{  
  m_uiForm.lbPrefix->setToolTip("Default instrument prefix");
  m_uiForm.loadRun_cbInst->setToolTip("Default instrument prefix");

  m_uiForm.gbExperiment->setToolTip("Files to process");
  m_uiForm.pbBack->setToolTip("Enabling this removes the mean number of counts per bin in the background region\n"
                               "of spectra from the all bins. Negative values are replaced with zeros (uses\n"
							   "FlatBackground)");
  m_uiForm.ckSumSpecs->setToolTip("If this box is not ticked there will be one output file for each input, otherwise\n"
                                   "the output will be summed into one file");
  m_uiForm.ckFixEi->setToolTip("Leave unticked for the algorithm GetEi to calculate a the incident neutron\nenergy based on the monitor signals and the guess below");

  m_uiForm.lbNorm->setToolTip("Select the type of normalization for the runs"), m_uiForm.cbNormal->setToolTip("Select the type of normalization for the runs");
  m_uiForm.cbMonitors->setToolTip("If normalization to monitor was selected");

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
void Homer::setUpPage2()
{/* The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
 this second page is largely filled with the diag widget, previous settings, second argument, depends on the instrument and the detector diagnostic settings are kept separate in "diag/"*/
  m_diagPage = new MWDiag(this, m_prev.group()+"/diag", m_uiForm.loadRun_cbInst);

  // set the default background region to the same as the default on this form
  emit MWDiag_updateTOFs(m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble(),
    m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());
	
  // insert the widget on to the second tab page (index = 1) of the form
  QLayout *mapLayout = m_uiForm.tabWidget->widget(1)->layout();
  QVBoxLayout *mapLay = qobject_cast<QVBoxLayout*>(mapLayout); 
  if (mapLay)
  {
    mapLay->addWidget(m_diagPage);
  }

  int height = m_uiForm.ckRunDiag->height() + m_diagPage->height();
  this->resize(this->width(), height);

  m_uiForm.ckRunDiag->setToolTip("Enable or disable all the controls on this page");
  // disenableDiag() enables or disables the detector diagnostics page depending on if the check box is clicked or not
  connect(m_uiForm.ckRunDiag, SIGNAL(clicked()), this, SLOT(disenableDiag()));

}

void Homer::setUpPage3()
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
void Homer::saveSettings()
{  
  m_prev.endGroup();

  QString instrument = m_uiForm.loadRun_cbInst->currentText();
  m_prev.setValue("CustomInterfaces/Homer/instrument", instrument); 
  
  QStringList prevInstrus =
    m_prev.value("CustomInterfaces/Homer/instrusList","").toStringList();
  if ( ! prevInstrus.contains(instrument) )
  {
    prevInstrus.append(instrument);
    // put the instrument list alphabetic order to make it easier to use
    prevInstrus.sort();
  }
  m_prev.setValue("CustomInterfaces/Homer/instrumsList", prevInstrus);
  
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
// ??STEVES?? remove this function?
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
void Homer::setSettingsGroup(const QString &instrument)
{
  m_prev.beginGroup("CustomInterfaces/Homer/in instrument "+instrument);
}
/** this runs after the run button was clicked. It runs runScripts()
*  ans saves the settings on the form
*/
void Homer::runClicked()
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
  deltaECalc unitsConv( this, m_uiForm, m_runFilesWid->getFileNames(),
    m_prev.value("bgremove").toString() == "bg removal: on",
	m_prev.value("TOFstart").toDouble(), m_prev.value("TOFend").toDouble(),
	m_WBVWid->getFileName());
    
  // if this function finds a control with an invalid entry the control is marked with a star and some information is returned here
  QString errors = unitsConv.checkNoErrors(m_validators);
  if ( ! errors.isEmpty() )
  {
    throw std::invalid_argument(errors.toStdString());
  }
  
  // The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
  if (m_uiForm.ckRunDiag->isChecked())
  {
    // mostly important to stop the run button being clicked twice, prevents any change to the form until the run has completed
    pythonIsRunning(true);
    // display the second page in case errors occur in processing the user settings here
    m_uiForm.tabWidget->setCurrentIndex(1);
    QString maskOutWS = "mask_"+QString::fromStdString(
	    Poco::Path(m_runFilesWid->getFile1().toStdString()).getBaseName());
    errors = m_diagPage->run(maskOutWS, true);
    if ( ! errors.isEmpty() )
    {
      pythonIsRunning(false); 
      throw std::invalid_argument(errors.toStdString());
    }
	// pass the bad detector list to the conversion script to enable masking
	unitsConv.maskDetects(maskOutWS);
  }

  pythonIsRunning(true);
  // we're back to processing the settings on the first page
  m_uiForm.tabWidget->setCurrentIndex(0);
  //unitsConv is always executed, the user can't switch this off, unless there's an error on the form. To examine the script that is executed uncomment 
  // use the following code to see the script
  //QStringList list1 = unitsConv.python().split("\n"); QMessageBox::critical(this, "", *(list1.end()-6)+'\n'+*(list1.end()-5)+'\n'+*(list1.end()-4)+'\n'+*(list1.end()-3)+'\n'+*(list1.end()-2));
  errors = unitsConv.run();
  pythonIsRunning(false); 

  if ( ! errors.isEmpty() )
  {
    throw std::runtime_error(errors.toStdString());
  }
  
  return errors.isEmpty();
}
//this function will be replaced a function in a widget
void Homer::browseClicked(const QString buttonDis)
{
  QLineEdit *editBox = NULL;
  QStringList extensions;
  bool toSave = false;

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
}
/**
 * A slot to handle the help button click
 */
void Homer::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
    "Homer"));
}
/** Enables or disables the absoulte unit conversion cocntrols based
*  on whether or not the check box has been checked
*/
void Homer::disenableAbsolute()
{
  const bool enabled = m_uiForm.ckRunAbsol->isChecked();
  m_uiForm.gbCalRuns->setEnabled(enabled);
  m_uiForm.gbMasses->setEnabled(enabled);
  m_uiForm.gbInteg->setEnabled(enabled);
 }
/** Enables or disables the find bad detectors controls based
*  on whether or not the check box has been checked
*/
void Homer::disenableDiag()
{
  m_diagPage->setEnabled(m_uiForm.ckRunDiag->isChecked());
}
/** This slot updates the MWDiag and SPE filename suggestor with the
* names of the files the user has just chosen
*/
void Homer::runFilesChanged()
{// this signal to the diag GUI allows the run files we choose here to be the default for its background correction
  try
  {// there might be an invalid file name in the box
    const std::vector<std::string> &names = m_runFilesWid->getFileNames();
    emit MWDiag_sendRuns(names);
    // the output file's default name is based on the input file names
    updateSaveName();
  }
  catch (std::invalid_argument)
  {// nothing is sent if there is an invalid filename
  }//the problem is displayed by the file widget's validator
}
/** Check if the user has specified a name for the output SPE file,
* if not insert a name based on the name of the input files
*/
void Homer::updateSaveName()
{// if the user added their own value prevent it from being changed
  if ( ! m_saveChanged ) 
  {
    m_uiForm.leNameSPE->setText(defaultName());
  }
}
/** update m_saveChanged with whether the user has changed the name away from the
*  default in this instance of the dialog box
*/
void Homer::saveNameUpd()
{// if the user had already altered the contents of the box it has been noted that the save name is under user control so do nothing
  if (m_saveChanged) return;
  m_saveChanged = m_uiForm.leNameSPE->text() != defaultName();
}
/** This slot passes the name of the white beam vanadium file to the MWDiag
*/
void Homer::updateWBV()
{
  try
  {  
    emit MWDiag_updateWBV(m_WBVWid->getFileName());
  }
  catch (std::invalid_argument)
  {// nothing is sent if there is an invalid filename
  }//the problem is displayed by the file widget's validator
}
/** enables or disables the list of monitors depending on the whether
* the monitor was set in the normalisation combobox
*  @param the text in the normalisastion combobox
*  @throw invalid_argument if neither the chosen normalisation scheme or the default can be found
*/
void Homer::setupNormBoxes(const QString &newText)
{
  enableSecondBox( newText == "monitor" );
}
/** When true is passed to this function the second dialog box is enabled
*  _without_ a blank entry or other it is disabled and left blank
*  @param toEnable true if the second box will be used
*/
void Homer::enableSecondBox(bool toEnable)
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
QString Homer::defaultName()
{
  try
  {//this will trhow if there is an invalid filename
    const std::vector<std::string> &fileList = m_runFilesWid->getFileNames();
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
  catch (std::invalid_argument)
  {// if there is an invalid filename
    return "";
  }//the error is also displayed by the file widget's validator
}
/** creates and shows the background removal time of flight form
*/
void Homer::bgRemoveClick()
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
void Homer::bgRemoveReadSets()
{ // the user can press these buttons again, they were disabled before while the dialog box was up
  m_uiForm.pbBack->setEnabled(true);
  m_uiForm.pbRun->setEnabled(true);
  
  m_uiForm.pbBack->setText(
    m_prev.value("bgremove", G_BACK_REMOVE).toString());

  // send the values to the detector diagnostics form, they are used as suggested values
  emit MWDiag_updateTOFs(m_prev.value("TOFstart", G_START_WINDOW_TOF).toDouble(),
    m_prev.value("TOFend", G_END_WINDOW_TOF).toDouble());
}

/**
 * Called when a new selection is made in the instrument box
 */
void Homer::instrSelectionChanged(const QString& prefix)
{
  // Need to check that there is a valid parameter file for the instrument else the
  // analysis won't work
  QString paramfile_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("parameterDefinition.directory"));
  QDir paramdir(paramfile_dir);
  paramdir.setFilter(QDir::Files);
  QStringList filters;
  filters << prefix + "*_Parameters.xml";
  paramdir.setNameFilters(filters);

  QStringList entries = paramdir.entryList();
  if( entries.isEmpty() )
  {
    QMessageBox::warning(this, "MantidPlot", "Selected instrument does have a parameter file.\nCannot run analysis");
    m_uiForm.pbRun->setEnabled(false);
  }
  else
  {
    m_uiForm.pbRun->setEnabled(true);
  }
  
}
