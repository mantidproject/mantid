#include "MantidQtCustomInterfaces/Excitations.h"
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
  m_saveChanged(false),
  m_inFiles(NULL)
{
}
/// Set up the dialog layout
void Excitations::initLayout()
{
  m_uiForm.setupUi(this);
  
  setUpPage1();
  setUpPage2(); //  Replace this line with the following m_uiForm.diagSec->initLayout(this);
  setUpPage3();
  
    // a helper widget for loading the second white beam vanadium run used to find bad detectors
//  RunNumbers *WBV2Widget = new RunNumbers;
//  QLayout *diagTest2Layout = m_uiForm.gbVariation->layout();
//  QGridLayout *diagTest2 = qobject_cast<QGridLayout*>(diagTest2Layout);
//  diagTest2->addWidget(WBV2Widget, 0, 0, 1, -1);

  /*****replace the code below with the widget****/
  // connect all the open file buttons to an open file dialog connected to it's line edit box
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_uiForm.pbIFile, QString("InputFile"));
  signalMapper->setMapping(m_uiForm.pbOFile, QString("OutputFile"));
  signalMapper->setMapping(m_uiForm.pbWBV1, QString("WBVanadium1"));
  signalMapper->setMapping(m_uiForm.pbWBV2, QString("WBVanadium2"));
  connect(m_uiForm.pbIFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbOFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV0, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV1, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV2, SIGNAL(clicked()), signalMapper, SLOT(map()));
  
  signalMapper->setMapping(m_uiForm.pbWBV0, QString("pbWBV0"));
  signalMapper->setMapping(m_uiForm.map_fileInput_pbBrowse, QString("map_fileInput_pbBrowse"));
  signalMapper->setMapping(m_uiForm.pbAddMono, QString("pbAddMono"));
  signalMapper->setMapping(m_uiForm.pbAddWhite, QString("pbAddWhite"));
  signalMapper->setMapping(m_uiForm.pbAddMap, QString("pbAddMap"));
  signalMapper->setMapping(m_uiForm.pbBrowseSPE, QString("pbBrowseSPE"));
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
/// Adds custom widgets, fills in combination boxes and runs setToolTip() on widgets
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
  connect(m_uiForm.loadRun_lenumber, SIGNAL(editingFinished()), this, SLOT(updateSaveName()));

  connect(m_uiForm.leNameSPE, SIGNAL(editingFinished()), this, SLOT(saveNameUpd()));
  
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
  
  m_uiForm.gbconvUnits->setToolTip("Settings for units conversion to energy transfer");
  m_uiForm.lbEGuess1->setToolTip("Approximate initial neutron energy, is passed to GetEi"), m_uiForm.leEGuess->setToolTip("Approximate initial neutron energy, is passed to GetEi");
  m_uiForm.lbEBins->setToolTip("Settings for units conversion to energy transfer (passed to ReBin)");
  m_uiForm.lbELow->setToolTip("Exclude neutrons with less than this energy (meV)"), m_uiForm.leELow->setToolTip("Exclude neutrons with less than this energy (meV)");
  m_uiForm.lbEHigh->setToolTip("Exclude neutrons with more than this energy (meV)"), m_uiForm.leEHigh->setToolTip("Exclude neutrons with more than this energy (meV)");
  m_uiForm.lbEWidth->setToolTip("Width of the energy bins (meV)"), m_uiForm.leEWidth->setToolTip("Width of the energy bins (meV)");

  m_uiForm.lbSPE->setToolTip("File name for the converted data"), m_uiForm.leNameSPE->setToolTip("File name for the converted data"), m_uiForm.pbBrowseSPE->setToolTip("File name for the converted data");
}
/// Adds custom widgets, fills in combination boxes and runs setToolTip() on widgets
void Excitations::setUpPage2()
{
// default parameters that are writen to the GUI
const char defHighAbsolute[5] = "1e10";
const char defLowAbsolute[2] = "0";
const char defSignificanceTest[4] = "3.3";
const char defHighMedian[4] = "3.0";
const char defLowMedian[4] = "0.1";
const char defVariation[4] = "1.1";
const char defBackground[4] = "5.0";

  m_uiForm.leStartTime->setText("18000");
  m_uiForm.leEndTime->setText("19500");
  m_uiForm.ckZeroCounts->setChecked(true);
  
  m_uiForm.ckRunDiag->setToolTip("Mask bad detectors");
  
  // deal with each input control in turn doing,
  //   go through each control and add (??previous?? or) default values
  //   add tool tips
  //   store any relation to algorthim properties as this will be used for validation
  QString iFileToolTip = "A file containing a list of spectra numbers which we aleady know should be masked";
  m_uiForm.lbIFile->setToolTip(iFileToolTip);
  m_uiForm.leIFile->setToolTip(iFileToolTip);
  m_uiForm.pbIFile->setToolTip(iFileToolTip);
  
  QString oFileToolTip =
    "The name of a file to write the spectra numbers of those that fail a test";
  m_uiForm.lbOFile->setToolTip(oFileToolTip);
  m_uiForm.leOFile->setToolTip(oFileToolTip);
  m_uiForm.pbOFile->setToolTip(oFileToolTip);
  
  m_uiForm.leSignificance->setText(defSignificanceTest);
  QString significanceToolTip =
    "Spectra with integrated counts within this number of standard deviations from\n"
    "the median will not be labelled bad (sets property SignificanceTest when\n"
    "MedianDetectorTest is run)";
  m_uiForm.leSignificance->setToolTip(significanceToolTip);
  m_uiForm.lbError->setToolTip(significanceToolTip);
//-------------------------------------------------------------------------------------------------
  QString WBV1ToolTip =
    "The name of a white beam vanadium run from the instrument of interest";
  m_uiForm.lbWBV1->setToolTip(WBV1ToolTip);
  m_uiForm.leWBV1->setToolTip(WBV1ToolTip);
  m_uiForm.pbWBV1->setToolTip(WBV1ToolTip);

  m_uiForm.leHighAbs->setText(defHighAbsolute);
  QString highAbsSetTool =
    "Reject any spectrum that contains more than this number of counts in total\n"
    "(sets property HighThreshold when FindDetectorsOutsideLimits is run)";
  m_uiForm.leHighAbs->setToolTip(highAbsSetTool);
  m_uiForm.lbHighAbs->setToolTip(highAbsSetTool);
  
  m_uiForm.leLowAbs->setText(defLowAbsolute);
  QString lowAbsSetTool =
    "Reject any spectrum that contains less than this number of counts in total\n"
    "(sets property LowThreshold when FindDetectorsOutsideLimits is run)";
  m_uiForm.leLowAbs->setToolTip(lowAbsSetTool);
  m_uiForm.lbLowAbs->setToolTip(lowAbsSetTool);

  m_uiForm.leHighMed->setText(defHighMedian);
  QString highMedToolTip =
    "Reject any spectrum whose total number of counts is more than this number of\n"
    "times the median total for spectra (sets property HighThreshold when\n"
    "MedianDetectorTest is run)";
  m_uiForm.leHighMed->setToolTip(highMedToolTip);
  m_uiForm.lbHighMed->setToolTip(highMedToolTip);

  m_uiForm.leLowMed->setText(defLowMedian);
  QString lowMedToolTip =
    "Reject any spectrum whose total number of counts is less than this number of\n"
    "times the median total for spectra (sets property LowThreshold when\n"
    "MedianDetectorTest is run)";
  m_uiForm.leLowMed->setToolTip(lowMedToolTip);
  m_uiForm.lbLowMed->setToolTip(lowMedToolTip);
//-------------------------------------------------------------------------------------------------
  QString WBV2ToolTip =
    "The name of a white beam vanadium run from the same instrument as the first";
  m_uiForm.lbWBV2->setToolTip(WBV2ToolTip);
  m_uiForm.leWBV2->setToolTip(WBV2ToolTip);
  m_uiForm.pbWBV2->setToolTip(WBV2ToolTip);

  m_uiForm.leVariation->setText(defVariation);
  QString variationToolTip = 
    "When comparing equilivient spectra in the two white beam vanadiums reject any\n"
    "whose the total number of counts varies by more than this multiple of the\n"
    "medain variation (sets property Variation when DetectorEfficiencyVariation is\n"
    "is run)";
  m_uiForm.leVariation->setToolTip(variationToolTip);
  m_uiForm.lbVariation->setToolTip(variationToolTip);

  m_uiForm.leAcceptance->setText(defBackground);
  QString acceptToolTip =
    "Spectra whose total number of counts in the background region is this number\n"
    "of times the median number of counts would be marked bad (sets property\n"
    "HighThreshold when MedianDetectorTest is run)";
  m_uiForm.lbAcceptance->setToolTip(acceptToolTip);
  m_uiForm.leAcceptance->setToolTip(acceptToolTip);

  QString startTToolTip =
    "An x-value in the bin marking the start of the background region, the\n"
    "selection is exclusive (RangeLower in MedianDetectorTest)";
  m_uiForm.lbStartTime->setToolTip(startTToolTip);
  m_uiForm.leStartTime->setToolTip(startTToolTip);
  QString endTToolTip =
    "An x-value in the bin marking the the background region's end, the selection\n"
    "is exclusive (RangeUpper in MedianDetectorTest)";
  m_uiForm.lbEndTime->setToolTip(endTToolTip);
  m_uiForm.leEndTime->setToolTip(endTToolTip);
  m_uiForm.ckZeroCounts->setToolTip(
    "Check this and spectra with zero counts in the background region will be"
    "considered bad");

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
  try
  {
    if ( ! parseInput() )
    {// one or more of the user values is unacceptable return to the dialog box a validator star should already be there
      return;
    }

	if (m_uiForm.ckRunDiag->isChecked())
	{// this part of the form is a separate widget, all the work is coded in over there
	  // m_uiForm.diagSec->run();
	}

    try
	{
	  deltaECalc unitsConv( m_uiForm, *m_inFiles );
	//m_userSettingsMap.find("RunFiles")->second,
	//                      m_userSettingsMap.find("badDetects,
	//					  m_userSettingsMap.find("binSpecif,
	//					  m_userSettingsMap.find("normFile,
	//					  m_userSettingsMap.find("mapFile,
	//					  m_userSettingsMap.find("outFile") );
	  QMessageBox::information(this, "", unitsConv.python());

	  runPythonCode(unitsConv.python());
	}
	catch(std::invalid_argument &e)
	{
	  QMessageBox::critical(this, "", 
        QString("Invalid value in control \""+QString(e.what())+"\""));
	}
  }
  catch (std::runtime_error e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
  }
  m_saveChanged = false;
//  Mantid::API::FrameworkManager::Instance().deleteWorkspace(test1.inputWS.toStdString());
}

//this function will be replaced a function in a widget
void Excitations::browseClicked(const QString &buttonDis)
{
  QLineEdit *editBox;
  QStringList extensions;
  bool toSave = false;
  if ( buttonDis == "InputFile")
  {
    editBox = m_uiForm.leIFile;
  }
  if ( buttonDis == "OutputFile")
  {
    editBox = m_uiForm.leOFile;
    extensions << "msk";
    toSave = true;
  }
  if ( buttonDis == "WBVanadium1")
  {
    editBox = m_uiForm.leWBV1;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
	}
  }
  if ( buttonDis == "WBVanadium2")
  {
    editBox = m_uiForm.leWBV2;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
	}
  }
  if ( buttonDis == "loadRun_pbBrowse")
  {
    editBox = m_uiForm.loadRun_lenumber;
	for ( int i = 0; i < numInputExts; i++)
	{
	  extensions << QString::fromStdString(inputExts[i]);
	}
  }
  if ( buttonDis == "pbWBV0")
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
  const bool enabled = m_uiForm.ckRunDiag->isChecked();
  m_uiForm.lbIFile->setEnabled(enabled);
  m_uiForm.leIFile->setEnabled(enabled);
  m_uiForm.pbIFile->setEnabled(enabled);
  m_uiForm.lbOFile->setEnabled(enabled);
  m_uiForm.leOFile->setEnabled(enabled);
  m_uiForm.pbOFile->setEnabled(enabled);
  m_uiForm.lbError->setEnabled(enabled);
  m_uiForm.leSignificance->setEnabled(enabled);
  m_uiForm.ckAngles->setEnabled(enabled);

  m_uiForm.gbIndividual->setEnabled(enabled);
  m_uiForm.gbVariation->setEnabled(enabled);
  m_uiForm.gbExperiment->setEnabled(enabled);
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
/// Parse input when the Run button is pressed
bool Excitations::parseInput()
{
  try
  {    
    readTheDialog();

    return true;
  }
  catch (Mantid::Kernel::Exception::NotFoundError e)
  {
    QMessageBox::critical(this, "", 
      QString("Error ") + QString(e.what()) + QString(". Make sure that the Mantid (including diagnostic) algorithms libraries are available"));
    return false;
  }
}

/** copies values from the form a map that is passed to the form and a sometimes a map
that is used in validation
*/
void Excitations::readTheDialog()
{
}


