#include "MantidQtCustomInterfaces/Excitations.h"
//??STEVES implement this
//#include "MantidQtMantidWidgets/FileInput.h"
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

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(Excitations);
  }
}
using namespace MantidQt::CustomInterfaces;
//using namespace MantidQt::MantidWidgets;

const std::string Excitations::inputExts[] = {"raw", "RAW", "NXS", "nxs"};
//----------------------
// Public member functions
//----------------------
///Constructor
Excitations::Excitations(QWidget *parent) : UserSubWindow(parent),
  m_diagPage(NULL), m_saveChanged(false), m_inFiles(NULL), m_busy(NULL)
{
}
/// Set up the dialog layout
void Excitations::initLayout()
{
  // standard setting up of all widgets
  m_uiForm.setupUi(this);
  
  // do the custom setting up like tool tips on each of the three tab pages 
  setUpPage1();
  setUpPage2();
  setUpPage3();
  
    // a helper widget for loading the second white beam vanadium run used to find bad detectors
//  RunNumbers *WBV2Widget = new RunNumbers;
//  QLayout *diagTest2Layout = m_uiForm.gbVariation->layout();
//  QGridLayout *diagTest2 = qobject_cast<QGridLayout*>(diagTest2Layout);
//  diagTest2->addWidget(WBV2Widget, 0, 0, 1, -1);
  
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_uiForm.pbWBV0, QString("pbWBV0"));
  signalMapper->setMapping(m_uiForm.map_fileInput_pbBrowse, QString("map_fileInput_pbBrowse"));
  signalMapper->setMapping(m_uiForm.pbAddMono, QString("pbAddMono"));
  signalMapper->setMapping(m_uiForm.pbAddWhite, QString("pbAddWhite"));
  signalMapper->setMapping(m_uiForm.pbAddMap, QString("pbAddMap"));
  signalMapper->setMapping(m_uiForm.pbBrowseSPE, QString("pbBrowseSPE"));
  connect(m_uiForm.pbWBV0, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.loadRun_pbBrowse, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.map_fileInput_pbBrowse, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddMono, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddWhite, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbAddMap, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbBrowseSPE, SIGNAL(clicked()), signalMapper, SLOT(map()));
  
  connect(signalMapper, SIGNAL(mapped(const QString &)),
         this, SLOT(browseClicked(const QString &)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));

  connect(m_uiForm.ckRunDiag, SIGNAL(clicked()), this, SLOT(disenableDiag()));
  connect(m_uiForm.ckRunAbsol, SIGNAL(clicked()), this, SLOT(disenableAbsolute()));

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
/// For each widgets in the first tab this adds custom widgets, fills in combination boxes and runs setToolTip()
void Excitations::setUpPage1()
{
  // insert the file loader helper widget
  m_inFiles = new deltaECalc::FileInput(*m_uiForm.loadRun_lenumber, *m_uiForm.loadRun_cbInst);
/*  FileInput *m_inFiles = new FileInput;
  QLayout *mapLayout = m_uiForm.gbrunMap->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout);
  mapLay->addWidget(m_inFiles, 0, 0, 1, -1);*/
  m_uiForm.map_fileInput_leName->setText("mari_res.map");
  m_uiForm.leScale->setText("0");
  m_uiForm.ckFixEi->setChecked(false);
  m_uiForm.ckSumSpecs->setChecked(true);
  m_uiForm.cbNoBack->setChecked(true);
  
  connect(m_uiForm.loadRun_pbBrowse, SIGNAL(clicked()), this, SLOT(addRunFile()));

  // SIGNALS and SLOTS that deal with coping the text from one edit box to another 
  connect(m_uiForm.loadRun_lenumber, SIGNAL(editingFinished()), this, SLOT(updateSaveName()));
  connect(m_uiForm.ckSumSpecs, SIGNAL(stateChanged(int)), this, SLOT(updateSaveName()));
  connect(m_uiForm.leNameSPE, SIGNAL(editingFinished()), this, SLOT(saveNameUpd()));
  connect(m_uiForm.leWBV0, SIGNAL(editingFinished()), this, SLOT(updateWBV()));
  
  QString runWSMap = "Sum spectra before saving in groups defined by this file (passed to GroupDetectors)";
/*  m_uiForm.gbrunMap->setToolTip(runWSMap);*/m_uiForm.map_fileInput_leName->setToolTip(runWSMap);
/*  m_inFiles->setToolTip(runWSMap);*/m_uiForm.map_fileInput_pbBrowse->setToolTip(runWSMap);
  
  m_uiForm.cbNormal->addItem("monitor");
  m_uiForm.cbNormal->addItem("protons (uAh)");
  m_uiForm.cbNormal->addItem("no normalization");
    
  // the first entry in this comobox should be an empty string
  m_uiForm.cbMonitors->addItem("");
  m_uiForm.cbMonitors->addItem("first monitor");
  m_uiForm.cbMonitors->addItem("second monitor peak area");
  m_uiForm.cbMonitors->setCurrentIndex(1);
    
  m_uiForm.loadRun_cbInst->addItem("MAR");
  m_uiForm.loadRun_cbInst->addItem("MAP");
  m_uiForm.loadRun_cbInst->addItem("");
  connect(m_uiForm.cbNormal, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(normalise(const QString &)));

  m_uiForm.lbScale->setToolTip("Multiply numbers of counts by this power of 10");
  m_uiForm.leScale->setToolTip("Multiply numbers of counts by this power of 10");
  
  m_uiForm.lbPrefix->setToolTip("For example MAR, MAP, ...");
  m_uiForm.loadRun_cbInst->setToolTip("For example MAR, MAP, ...");
  
  m_uiForm.gbExperiment->setToolTip("Files to process");
  m_uiForm.loadRun_lbDiscrip->setToolTip("List of runs to load"), m_uiForm.loadRun_lenumber->setToolTip("List of runs to load"), m_uiForm.loadRun_pbBrowse->setToolTip("List of runs to load");
//??STEVES?? this code will go to the run number to file name widget
//  m_uiForm.loadRun_tvRuns->setToolTip("List of runs to load");
//  m_uiForm.loadRun_tvRuns->setColumnCount(3);
//  m_uiForm.loadRun_tvRuns->setRowCount(1);
//  m_uiForm.loadRun_tvRuns->horizontalHeader()->hide();
//  m_uiForm.loadRun_tvRuns->verticalHeader()->hide();
  m_uiForm.lbNorm->setToolTip("Select the type of normalization for the runs"), m_uiForm.cbNormal->setToolTip("Select the type of normalization for the runs");
  m_uiForm.cbMonitors->setToolTip("If normalization to monitor was selected");
  m_uiForm.lbScale->setToolTip("Multipling numbers by a large constant can make plotting easier"), m_uiForm.leScale->setToolTip("Multipling numbers by a large constant can make plotting easier");
  m_uiForm.lbWBV0->setToolTip("The white beam vanadium run picked here will set the default for finding bad detectors and absolute units conversion"), m_uiForm.leWBV0->setToolTip("The white beam vanadium run picked here will set the default for finding bad detectors and absolute units conversion"), m_uiForm.pbWBV0->setToolTip("The white beam vanadium run picked here will set the default for finding bad detectors and absolute units conversion");
  m_uiForm.lbWBV0Low1->setToolTip("Energy range for the white beam normalisation"); m_uiForm.lbWBV0Low2->setToolTip("Energy range for the white beam normalisation");
  m_uiForm.lbWBV0High1->setToolTip("Energy range for the white beam normalisation"); m_uiForm.lbWBV0High2->setToolTip("Energy range for the white beam normalisation");
  
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
{// The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
  // this second page is largely filled with the diag widget
  m_diagPage = new MantidWidgets::MWDiag(this);
  // insert the widgets on to the second page (index = 1)
  QLayout *mapLayout = m_uiForm.tabWidget->widget(1)->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout); 
  if ( mapLay )
  { // this should always be true because we setup a grid layout in the designer
    mapLay->addWidget(m_diagPage, 1, 0, 6, 5);
  }
  connect(m_diagPage, SIGNAL(runAsPythonScr(const QString&)),
                this, SIGNAL(runAsPythonScript(const QString&)));

  m_uiForm.ckRunDiag->setToolTip("Enable or disable all the controls on this page");
  // either enables or disables the detector diagnostics page depending on if the check box is clicked or not
  disenableDiag();
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
}
/** Runnings everything, depending on what was entered on to the form
*/
void Excitations::run()
{
  bool noErrorFound = true;
  
  try
  {
    // constructing this builds the Python script, it is executed below
    deltaECalc unitsConv( m_uiForm, *m_inFiles );
	if ( unitsConv.invalid().size() > 0 )
	{// errors were found, a discription of them should be left on the form, leave the user to correct them
	  throw std::invalid_argument(unitsConv.invalid().begin()->second);
	}
    connect(&unitsConv, SIGNAL(runAsPythonScript(const QString&)),
                this, SIGNAL(runAsPythonScript(const QString&)));

    // mostly important to stop the run button being clicked twice, prevents any change to the form until the run has completed
    pythonIsRunning(true);
    // The diag -detector diagnositics part of the form is a separate widget, all the work is coded in over there
 	if (m_uiForm.ckRunDiag->isChecked())
	{
      if ( ! m_diagPage->run() )
	  {
        noErrorFound = false;
		//??STEVES?? ensure an error is given at this point
      }
	  else
	  {// pass the bad detector list to the conversion script to enable masking
	    unitsConv.maskDetects(m_diagPage->getOutputWS());
	  }
	}

    if ( unitsConv.python().count('\n') == 0 )
	{// this means that a script wasn't produced, only a one line error message
      noErrorFound = false;
    }
    //unitsConv always executed, the user can't switch this off, unless there's an error on the form	
	if (noErrorFound)
	{
    QMessageBox::critical(this, "", unitsConv.python());
	  unitsConv.run();
	}
  }
  catch (std::runtime_error &e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
  }
  catch (std::exception &e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
  }
  pythonIsRunning(false);
  m_saveChanged = false;
//  Mantid::API::FrameworkManager::Instance().deleteWorkspace(test1.inputWS.toStdString());
}
//this function will be replaced a function in a widget
void Excitations::browseClicked(const QString &buttonDis)
{
  QLineEdit *editBox;
  QStringList extensions;
  bool toSave = false;

  if ( buttonDis == "loadRun_pbBrowse")
  {
    editBox = m_uiForm.loadRun_lenumber;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
	}
  }
  if (buttonDis == "pbWBV0")
  {
    editBox = m_uiForm.leWBV0;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
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
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
	}
  }
  if ( buttonDis == "pbAddWhite")
  {
    editBox = m_uiForm.leWhiteVan;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
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
  
  if( ! editBox->text().isEmpty() )
  {
    QString dir = QFileInfo(editBox->text()).absoluteDir().path();
    //STEVES use QSettings to store the last entry instead of the line below
//  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = this->openFileDialog(toSave, extensions);
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
  for ( int i = 0; i < numInputExts; i++)
  {
	extensions << QString::fromStdString(inputExts[i]);
  }

  if( ! m_uiForm.loadRun_lenumber->text().isEmpty() )
  {
    QString dir =
	  QFileInfo(m_uiForm.loadRun_lenumber->text()).absoluteDir().path();
    //STEVES use QSettings to store the last entry instead of the line below
//  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = this->openFileDialog(false, extensions);
  if( filepath.isEmpty() ) return;

  if ( ! m_uiForm.loadRun_lenumber->text().isEmpty() )
  {
    m_uiForm.loadRun_lenumber->setText(
	  m_uiForm.loadRun_lenumber->text()+", " + filepath);
  }
  else m_uiForm.loadRun_lenumber->setText(filepath);
  
  updateSaveName();
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
/** Check if the user has specified a name for the output SPE file,
* if not insert a name based on the name of the input files
*/
void Excitations::updateSaveName()
{// if the user added their own value don't change it
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
* normalise by monitor was selected
*/
void Excitations::normalise(const QString &newText)
{
  if ( newText == "monitor" )
  {
    m_uiForm.cbMonitors->setEnabled(true);
	if (m_uiForm.cbMonitors->currentText().isEmpty())
	{// the first entry in the comobox is an empty string, the second entry shouldn't be
      m_uiForm.cbMonitors->setCurrentIndex(1);
	}	
  }
  else
  { 
    // the first entry in the comobox should be an empty string
	m_uiForm.cbMonitors->setCurrentIndex(0);
	m_uiForm.cbMonitors->setEnabled(false);
  }
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

