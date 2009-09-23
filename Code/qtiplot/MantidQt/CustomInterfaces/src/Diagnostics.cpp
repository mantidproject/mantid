//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/Diagnostics.h"
#include "MantidQtCustomInterfaces/ExcitationsDiagResults.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QSignalMapper>
#include <QUrl>
#include <QDesktopServices>
#include <QHash>
#include <boost/lexical_cast.hpp>
#include <exception>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(Diagnostics);
}
}

using namespace MantidQt::CustomInterfaces;
// default parameters that are writen to the GUI
const char Diagnostics::defHighAbsolute[5] = "1e10";
const char Diagnostics::defLowAbsolute[2] = "0";
const char Diagnostics::defSignificanceTest[4] = "3.3";
const char Diagnostics::defHighMedian[4] = "1.5";
const char Diagnostics::defLowMedian[4] = "0.1";
const char Diagnostics::defVariation[4] = "1.1";
const char Diagnostics::defBackground[4] = "0.1";

//----------------------
// Public member functions
//----------------------
///Constructor
Diagnostics::Diagnostics(QWidget *parent) : UserSubWindow(parent),
  m_dispDialog(0), m_busy(false)
{
}
/// slot called run by the results form (a ExcitationsDiagResults) to say that it has finished and we can start processing again
void Diagnostics::childFormDied()
{
  // label the dialog as dead so that we don't try to call it's properties
  m_dispDialog = 0;
  // allowing running the scripts again, if running is true this will be done later on
  if (!m_busy)
  {
    m_uiForm.pbRun->setEnabled(true);
  }
}
/// Set up the dialog layout
void Diagnostics::initLayout()
{
  m_uiForm.setupUi(this);

  // deal with each input control in turn doing,
  //   go through each control and add (??previous?? or) default values
  //   add tool tips
  //   store any relation to algorthim properties as this will be used for validation
  QString iFileToolTip = "NOT IMPLEMENTED YET A file containing a list of spectra numbers which we aleady know should be masked";
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
  m_algorPropList["Significance"] = "MedianDetectorTest.SignificanceTest";
//-------------------------------------------------------------------------------------------------
  QString WBV1ToolTip =
    "The name of a white beam vanadium run from the instrument of interest";
  m_uiForm.lbWBV1->setToolTip(WBV1ToolTip);
  m_uiForm.leWBV1->setToolTip(WBV1ToolTip);
  m_uiForm.pbWBV1->setToolTip(WBV1ToolTip);
  m_algorPropList["WBVanadium1"] = "LoadRaw.Filename";

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
  m_algorPropList["Variation"] = "DetectorEfficiencyVariation.Variation";

  m_uiForm.pbAddRun->setToolTip("Add another experimental run file for analysis");
  m_uiForm.pbRemoveRun->setToolTip("Remove the selected run");
  m_uiForm.lwRunFiles->setToolTip("List experimental run files to be analysed");

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

  // connect all the open file buttons to an open file dialog connected to it's line edit box
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_uiForm.pbIFile, QString("InputFile"));
  signalMapper->setMapping(m_uiForm.pbOFile, QString("OutputFile"));
  signalMapper->setMapping(m_uiForm.pbWBV1, QString("WBVanadium1"));
  signalMapper->setMapping(m_uiForm.pbWBV2, QString("WBVanadium2"));
  connect(m_uiForm.pbIFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbOFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV1, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV2, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(signalMapper, SIGNAL(mapped(const QString &)),
         this, SLOT(browseClicked(const QString &)));

  // open experiment files is different
  connect(m_uiForm.pbAddRun, SIGNAL(clicked()), this, SLOT(addFile()));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));

  connect(m_uiForm.lwRunFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
    SLOT(removeName(QListWidgetItem*)));
  connect(m_uiForm.pbRemoveRun, SIGNAL(clicked()), this, SLOT(removeName()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  
  // setup the stars that show when a property is invalid and link the stars to the property
  loadAlgorDummies();

  getAlgProperties();   

  createValidatorLabels();

  placeValidatorLabels();

  readTheDialog();
  setPropertyValues();  
}

void Diagnostics::run()
{
  // these structures are used to report progress and pass results from one test to another, at the moment there is no progress
  ExcitationsDiagResults::TestSummary test1;
  test1.test = "First white beam test";
  test1.numBad = ExcitationsDiagResults::TestSummary::NORESULTS;

  ExcitationsDiagResults::TestSummary test2;
  test2.test = "Second white beam test";
  test2.numBad = ExcitationsDiagResults::TestSummary::NORESULTS;

  try
  {
    if ( ! parseInput() )
    {// one or more of the user values is unacceptable return to the dialog box a validator star should already be there
      return;
    }

    // the input is good bring up the status window
    m_dispDialog = raiseDialog();

    // report to the dialog what's happening
    test1.status = "Analysing white beam vanadium 1";

    if ( ! m_dispDialog ) return;
    m_dispDialog->notifyDialog(test1);

    // run the first test and then report again
    test1 = runWhite1();
    if ( ! m_dispDialog ) return;
    m_dispDialog->notifyDialog(test1);
    if ( test1.status != "White beam vanadium 1 complete" )
    {// return to the dialog box, runWhite calls a function that some displays errors, some other errors will be in the log, we hope this covers everything, but there's not guarantee
      return;
    }
    // the other two test below are optional dependent on the information supplied by the user
    if ( ! m_userSettingsMap["WBVanadium2"].isEmpty() )
    {
      test2.status = "Analysing white beam vanadium 2 and comparing";
      if ( ! m_dispDialog ) return;
      m_dispDialog->notifyDialog(test2);

      test2 = runWhite2(test1);
      if ( ! m_dispDialog ) return;
      m_dispDialog->notifyDialog(test2);
      if ( test2.status != "White beam vanadium comparison complete" )
      {
        return;
      }
    }
    
    if ( ! m_userSettingsMap["expFileNames"].isEmpty() )
    {
      ExcitationsDiagResults::TestSummary test3;
      test3.test = "Background test";
      test3.status = "Analysing the background regions of experimental runs";
      test3.outputWS = "";
      test3.numBad = ExcitationsDiagResults::TestSummary::NORESULTS;
      test3.inputWS = "";
      if ( ! m_dispDialog ) return;
      m_dispDialog->notifyDialog(test3);

      test3 = runBack(test1, test2);
      if ( ! m_dispDialog ) return;
      m_dispDialog->notifyDialog(test3);
    }
  }
  catch (std::exception e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
    // the following commands 
  }
  Mantid::API::FrameworkManager::Instance().deleteWorkspace(test1.inputWS.toStdString());
  Mantid::API::FrameworkManager::Instance().deleteWorkspace(test2.inputWS.toStdString());
}
/** A slot that is called by the "browse" buttons that are associated
* with QLineEdits
* @param buttonDis The name of the property associated with the QLineEdit
*/
void Diagnostics::browseClicked(const QString &buttonDis)
{
  QLineEdit *editBox;
  QStringList extensions;
  if ( buttonDis == "InputFile") editBox = m_uiForm.leIFile;
  if ( buttonDis == "OutputFile") editBox = m_uiForm.leOFile;
  if ( buttonDis == "WBVanadium1")
  {
    editBox = m_uiForm.leWBV1;
    extensions << "RAW"<< "raw";
  }
  if ( buttonDis == "WBVanadium2")
  {
    editBox = m_uiForm.leWBV2;
    extensions << "RAW"<< "raw";
  }

  if( ! editBox->text().isEmpty() )
  {
    QString dir = QFileInfo(editBox->text()).absoluteDir().path();
    //STEVES I want the line below but I haven't got an algorithm name and so I guess this does make sense
//  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = this->openFileDialog(false, extensions);
  if( !filepath.isEmpty() ) editBox->setText(filepath);
}
/**
 * A slot to handle the help button click
 */
void Diagnostics::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
    "Detector Efficiency Tests"));
}
/** Remove as file from the list, the file is specified using the QListWidgetItem
* agruement or if no agruements are passed the file in the current row
*  @param item an item in the lwRunFiles to remove
*/
void Diagnostics::removeName(QListWidgetItem *item)
{
  int row;
  if (item) row = m_uiForm.lwRunFiles->row(item);
  else row = m_uiForm.lwRunFiles->currentRow();

  delete m_uiForm.lwRunFiles->takeItem(row);
}
/** A slot called by the add file button that adds an experimental file to the list 
*/
void Diagnostics::addFile()
{
  if( m_uiForm.lwRunFiles->count() > 0 )
  {
    QString dir = QFileInfo(m_uiForm.lwRunFiles->item(0)->text()).absoluteDir().path();
    //STEVES I want the line below but I haven't got an algorithm name and so I guess this does make sense
//  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = this->openFileDialog(false, QStringList()<< "RAW"<<"raw");
  if( !filepath.isEmpty() ) m_uiForm.lwRunFiles->insertItem( 0, filepath);
}
/** Creates instances of the algorithms used by the scripts so that we have some properties
*  available for input parameter validation
*/
void Diagnostics::loadAlgorDummies()
{
  if ( m_algorDummies.size() > 0 ) return;
  m_algorDummies.push_back( Mantid::API::
    AlgorithmManager::Instance().createUnmanaged("LoadRaw"));
  m_algorDummies.push_back( Mantid::API::
    AlgorithmManager::Instance().createUnmanaged("FindDetectorsOutsideLimits"));
  m_algorDummies.push_back( Mantid::API::
    AlgorithmManager::Instance().createUnmanaged("MedianDetectorTest"));
  m_algorDummies.push_back( Mantid::API::
    AlgorithmManager::Instance().createUnmanaged("DetectorEfficiencyVariation"));

  for ( int i = 0; i < m_algorDummies.size(); i ++ )
  {
    m_algorDummies[i]->initialize();
  }
}

/// Parse input when the Run button is pressed
bool Diagnostics::parseInput()
{
  try
  {    
    readTheDialog();

    return setPropertyValues();
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
void Diagnostics::readTheDialog()
{
  m_propertyValueMap.clear();
  m_userSettingsMap.clear();
  //  copy the values from each input control into the storage map
  storeUserSetting("InputFile", m_uiForm.leIFile->text());
  storeUserSetting("OutputFile", m_uiForm.leOFile->text());

  storeUserSetting("Significance", m_uiForm.leSignificance->text());

  storeUserSetting("WBVanadium1", m_uiForm.leWBV1->text());

  storeUserSetting("HighAbsolute", m_uiForm.leHighAbs->text());
  storeUserSetting("LowAbsolute", m_uiForm.leLowAbs->text());
  storeUserSetting("HighMedian", m_uiForm.leHighMed->text());
  storeUserSetting("LowMedian", m_uiForm.leLowMed->text());

  storeUserSetting("WBVanadium2", m_uiForm.leWBV2->text());
  if ( ! m_userSettingsMap["WBVanadium2"].isEmpty() )
  {
    storeUserSetting("Variation", m_uiForm.leVariation->text());
  }
  else
  {// with no "WBVanadium2 Variation isn't going to get used, set it to dummy value that passes validation, although only just
    storeUserSetting("Variation", "1e-200");
  }

  QString expFiles = "";
  if ( m_uiForm.lwRunFiles->count() > 0 )
  {
    expFiles += m_uiForm.lwRunFiles->item(0)->text();
  }
  for( int i = 1; i < m_uiForm.lwRunFiles->count(); i++ )
  {
    expFiles += ", " + m_uiForm.lwRunFiles->item(i)->text();
  }
  storeUserSetting("expFileNames", expFiles);

  if ( ! expFiles.isEmpty() )
  {
    QString checked = m_uiForm.ckZeroCounts->isChecked() ? "true" : "false";
    storeUserSetting("removeZero", checked);
    storeUserSetting("backgroundAccept", m_uiForm.leAcceptance->text());
        
    storeUserSetting("TOFStart", m_uiForm.leStartTime->text());
    storeUserSetting("TOFEnd", m_uiForm.leEndTime->text());
  }
}
/** Link the propery names that we are using here and are all of the form
*  algorithm_name.property_name with a pointer to the property
* @param algList pointers instances of the algorithm used by the scripts
*/
void Diagnostics::getAlgProperties()
{
  // m_algorPropList contains, as the indexabled value, the list of all the properties used. The map is small and we only do this once so it should not be noticably slow. If we upgrade boost then use bimap instead
  std::map<std::string, std::string>::const_iterator it;
  for ( it = m_algorPropList.begin(); it != m_algorPropList.end(); ++it )
  {
    // now loop through all the algoriths to find the one that the property is in ( on the left of the .)
    std::string algDotProp = it->second;
    int dotPos = algDotProp.find_first_of(".");
    std::string algName = algDotProp.substr(0, dotPos);
    std::string propName = algDotProp.substr(dotPos+1 ,std::string::npos);
    for ( int j = 0; j < m_algorDummies.size(); ++j)
    {
      if ( algName == m_algorDummies[j]->name() )
      {
        std::vector<Mantid::Kernel::Property*> propsToAdd =
          m_algorDummies[j]->getProperties();
        // we've found the algorithm now get the property
        for ( int k = 0 ; k < propsToAdd.size() ; ++k )
        {
          if ( propName == propsToAdd[k]->name() )
          {//we have a match, a boost::bimap would be a much shorter way of doing this
            m_algProperties.insert(QString::fromStdString(algDotProp), propsToAdd[k]);
          }// this is the last time we have to search through the back of a map like this
        }
      }
    }
  }
}
/// place the stars that are used as validators on the form
void Diagnostics::placeValidatorLabels()
{
  QLayout *currentLayout = m_uiForm.gbUniversal->layout();
  QGridLayout *uni = qobject_cast<QGridLayout*>(currentLayout);
  QLabel *validlbl = getValidatorMarker("MedianDetectorTest.SignificanceTest");
  uni->addWidget(validlbl, 2, 2);

  // work on the Individual White Beam Tests groupbox
  currentLayout = m_uiForm.gbIndividual->layout();
  QGridLayout *individGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("LoadRaw.Filename");
  individGrid->addWidget(validlbl, 0, 7);
// delete the following in 2010 or uncomment out if validators are added to any of the following properties 
  //validlbl = getValidatorMarker("HighAbsolute");
  //individGrid->addWidget(validlbl, 1, 3);
  //validlbl = getValidatorMarker("LowAbsolute");
  //individGrid->addWidget(validlbl, 1, 9);
  //validlbl = getValidatorMarker("HighMedian");
  //individGrid->addWidget(validlbl, 2, 3);
  //validlbl = getValidatorMarker("LowMedian");
  //individGrid->addWidget(validlbl, 2, 9);

  // work on the efficency variation test groupbox
  currentLayout = m_uiForm.gbVariation->layout();
  QGridLayout *efficiencyGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("DetectorEfficiencyVariation.Variation");
  efficiencyGrid->addWidget(validlbl, 1, 3);  
}

/** Runs python code that implements the tests on the first white beam vanadium
*  @return a structure containing the status of running the tests, the number of bad detectors found the output workspace, etc.
*/
ExcitationsDiagResults::TestSummary Diagnostics::runWhite1()
{  
  //run the tests on the first white beam vanadium run
  QString code = constructScript();
  pythonIsRunning(true);
  QString result = runPythonCode(code);
  pythonIsRunning(false);
  // data from python is all passed back as a string, read in the string and see if it contains what we want
  return readRes(result);
}
/** Runs python code that implements the tests on the second white beam vanadium and the comparison with the first
*  @param lastResults results from the first white beam vanadium test
*  @param whiteBeam2 user settings for the comparison between white beams
*  @return a structure containing the status of running the tests, the number of bad detectors found the output workspace, etc.
*/
ExcitationsDiagResults::TestSummary Diagnostics::runWhite2(
  const ExcitationsDiagResults::TestSummary &lastResults )
{  
  //run the tests on the first white beam vanadium run
  QString code = constructScript(lastResults);
  pythonIsRunning(true);
  QString result = runPythonCode(code);
  pythonIsRunning(false);
  // data from python is all passed back as a string, read in the string and see if it contains what we want
  return readRes(result);
}
/** Runs python code that implements the tests on the second white beam vanadium and the comparison with the first
*  @param test1 results from the first white beam vanadium test
*  @param test2 results of comparing white beam runs, test2.numBad=ExcitationsDiagResults::TestSummary::NORESULTS to ignore test
*  @return a structure containing the status of running the tests, the number of bad detectors found the output workspace, etc.
*/
ExcitationsDiagResults::TestSummary Diagnostics::runBack(
  const ExcitationsDiagResults::TestSummary &test1,
  const ExcitationsDiagResults::TestSummary &test2 )
{
  //run the tests on the first white beam vanadium run
  QString code = constructScript(test1, test2);
  pythonIsRunning(true);
  QString result = runPythonCode(code);
  pythonIsRunning(false);
  // data from python is all passed back as a string, read in the string and see if it contains what we want
  return readRes(result);
}
/** Loads the python code to find bad detectors in the first
*  white beam vanadium and inserts the user entered values into that code
*  @return executable python code
*/
QString Diagnostics::constructScript() const
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  // generate an array with the all the file names that contain the python code we are going to use
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/whitebeam1test.py");
  QString pythonScript;
  readFile(pythonFileName, pythonScript);
   
  pythonScript.replace("|WBVANADIUM1|", 
    m_userSettingsMap.find("WBVanadium1")->second);
  pythonScript.replace("|HIGHABSOLUTE|", 
    m_userSettingsMap.find("HighAbsolute")->second);
  pythonScript.replace("|LOWABSOLUTE|", 
    m_userSettingsMap.find("LowAbsolute")->second);
  pythonScript.replace("|HIGHMEDIAN|", 
    m_userSettingsMap.find("HighMedian")->second);
  pythonScript.replace("|LOWMEDIAN|", 
    m_userSettingsMap.find("LowMedian")->second);
  pythonScript.replace("|SIGNIFICANCETEST|",
    m_userSettingsMap.find("Significance")->second);
  pythonScript.replace("|OUTPUTFILE|",
    m_userSettingsMap.find("OutputFile")->second);
  pythonScript.replace("|INPUTFILE|",
    m_userSettingsMap.find("InputFile")->second);

  return pythonScript;
}

/** Loads the python code to find bad detectors using two
*  white beam vanadiums and inserts the user entered values into that code
*  @param foundBad the results of the tests on the first white beam run
*  @return executable python code
*/
QString Diagnostics::constructScript(
    const ExcitationsDiagResults::TestSummary &foundBad) const
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  // this file contains all python script to run, expect the user settings and the results of previous tests
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/whitebeam2test.py");
  QString pythonScript;
  readFile(pythonFileName, pythonScript);
   
  pythonScript.replace("|SIGNIFICANCETEST|",
    m_userSettingsMap.find("Significance")->second);
  pythonScript.replace("|OUTPUTFILE|",
    m_userSettingsMap.find("OutputFile")->second);
  pythonScript.replace("|HIGHABSOLUTE|", 
    m_userSettingsMap.find("HighAbsolute")->second);
  pythonScript.replace("|LOWABSOLUTE|",
    m_userSettingsMap.find("LowAbsolute")->second);
  pythonScript.replace("|HIGHMEDIAN|", 
    m_userSettingsMap.find("HighMedian")->second);
  pythonScript.replace("|LOWMEDIAN|", 
    m_userSettingsMap.find("LowMedian")->second);
  pythonScript.replace("|INPUTMASK|", foundBad.outputWS);
  pythonScript.replace("|WBV1|", foundBad.inputWS);
  pythonScript.replace("|WBVANADIUM2|",
    m_userSettingsMap.find("WBVanadium2")->second);
  pythonScript.replace("|CHANGEBETWEEN|",
    m_userSettingsMap.find("Variation")->second);

  return pythonScript;
}

/** Loads the python code that runs the background tests for badly performing
*  detectors
*  @param test1 results from the first white beam vanadium test
*  @param test2 results of comparing white beam runs, test2.numBad=ExcitationsDiagResults::TestSummary::NORESULTS to ignore test
*  @return executable python code
*/
QString Diagnostics::constructScript(
  const ExcitationsDiagResults::TestSummary &test1,
  const ExcitationsDiagResults::TestSummary &test2) const
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  // this file contains all python script to run, expect the user settings and the results of previous tests
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/backgroundtest.py");
  QString pythonScript;
  readFile(pythonFileName, pythonScript);
  // read in user settings
  pythonScript.replace("|ERRORBARS|",
    m_userSettingsMap.find("Significance")->second);
  pythonScript.replace("|OUTPUTFILE|",
    m_userSettingsMap.find("OutputFile")->second);
  // use results from the first test
  pythonScript.replace("|WBV1|", test1.inputWS);
  pythonScript.replace("|MASK1|", test1.outputWS);
  // results from the second test, if any
  if ( test2.numBad != ExcitationsDiagResults::TestSummary::NORESULTS )
  {
    pythonScript.replace("|WBV2|", test2.inputWS);
  pythonScript.replace("|MASK2|", test2.outputWS);
  }
  else
  {
    pythonScript.replace("|WBV2|", "");
    pythonScript.replace("|MASK2|", "");
  }
  // user settings for our last test
  pythonScript.replace("|EXPFILES|",
    m_userSettingsMap.find("expFileNames")->second);
  pythonScript.replace("|BACKGROUNDACCEPT|",
    m_userSettingsMap.find("backgroundAccept")->second);

  // we need to check if the TOF arguments are empty because python doesn't accept an empty string as an argument
  QString TOFWindowBlock;
  if ( ! m_userSettingsMap.find("TOFStart")->second.isEmpty() )
  {
    TOFWindowBlock = QString(", RangeLower = ") +
      m_userSettingsMap.find("TOFStart")->second;
  }
  if ( ! m_userSettingsMap.find("TOFEnd")->second.isEmpty() )
  {
    TOFWindowBlock += QString(", RangeHigher = ") +
      m_userSettingsMap.find("TOFEnd")->second;
  }
  pythonScript.replace("|TOFWINDOWBLOCK|", TOFWindowBlock);


  pythonScript.replace("|REMOVEZEROS|",
    m_userSettingsMap.find("removeZero")->second);
  
  return pythonScript;
}

void Diagnostics::readFile(const QString &pythonFile, QString &scriptText) const
{
  QFile py_script(pythonFile);
  try
  {
    if ( !py_script.open(QIODevice::ReadOnly) )
    {
      throw Mantid::Kernel::Exception::FileError(std::string("Couldn't open python file "), pythonFile.toStdString());
    }
    QTextStream stream(&py_script);
    while( !stream.atEnd() )
    {
      scriptText.append(stream.readLine() + "\n");
    }
    py_script.close();
  }
  catch( ... )
  {
    py_script.close();
    throw;
  }
}
/** Turns the multi-line string returned from python scripts into a struct
*  @pythonOut string as returned by runPythonCode()
*  @return either data (or if numBad is set to NORESULTS then diagnositic output, or possibly nothing at all)
*/
ExcitationsDiagResults::TestSummary Diagnostics::readRes(QString pyhtonOut)
{
  QStringList results = pyhtonOut.split("\n");
  if ( results.count() < 2 )
  {// there was an error in the python, disregard these results
    QString Error = "Error \"" + pyhtonOut + "\" found, while executing scripts, more details can be found in the Mantid and python log files.";
    QMessageBox::critical(this, this->windowTitle(),
      Error);
    ExcitationsDiagResults::TestSummary temp = { Error, "", "", ExcitationsDiagResults::TestSummary::NORESULTS, "" };
    return temp;
  }
  if ( results.count() < 6 || results[0] != "success" )
  {// there was an error in the python, disregard these results
    QString Error = "Error \"" + results[1] + "\" found executing scripts.  More details can be found in the Mantid and python log files.";
    QMessageBox::critical(this, this->windowTitle(),
      Error);
    ExcitationsDiagResults::TestSummary temp = { Error, "", "", ExcitationsDiagResults::TestSummary::NORESULTS, "" };
    return temp;
  }
  std::string theBad = results[4].toStdString();
  
  int numBad = boost::lexical_cast<int>(theBad);
      // the format of the struct TestSummary { QString test; QString status; QString outputWS;  int numBad; QString inputWS;};
  ExcitationsDiagResults::TestSummary temp = { results[1], results[2], results[3], numBad, results[5] };
  return temp;
}

/** create and show a dialog box that reports the number of bad
* detectors.
* @param summaryInfo the results from running each the test
*/
ExcitationsDiagResults* Diagnostics::raiseDialog()
{
  ExcitationsDiagResults *dialog = 0;
  try
  {
    dialog = new ExcitationsDiagResults(this);
    connect(dialog, SIGNAL(pythonCodeConstructed(const QString&)), this, SIGNAL(runAsPythonScript(const QString&)));
    m_uiForm.pbRun->setEnabled(false);
    connect(dialog, SIGNAL(releaseParentWindow()), this, SLOT(childFormDied()));
    dialog->show();
  }
  catch (std::exception& e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which may be excessive
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString::fromStdString(e.what()) + QString("\" found displaying the data"));
    m_uiForm.pbRun->setEnabled(true);
  }
  return dialog;
}

/** Saves a values into a map for passing to the scripts. Usually these are values the
*  user entered into the form. Some values used by algorithms are copied in to m_propertyValueMap
*  @param variableName the internal name for the setting, not seen by users
*  @param value the value that setting is going to take
*/
void Diagnostics::storeUserSetting(const std::string &varibleName, const QString &value)
{
  // some varibles will be copied over to algorithm properties and copy them over to the map in that situation 
  std::map<std::string, std::string>::const_iterator it
    =  m_algorPropList.find(varibleName);
  if ( it != m_algorPropList.end() )
  {// get the property name from m_algorPropList and copy the information over
    storePropertyValue(
      QString::fromStdString(m_algorPropList[varibleName]), value);
  }
  m_userSettingsMap[varibleName] = value;
}
/// enable the run button if the results dialog has been closed and the python has stopped
void Diagnostics::pythonIsRunning(bool running)
{// the run button was disabled when the results form was shown, as we can only do one analysis at a time, we can enable it now
  m_busy = running;
  if (!m_dispDialog && !running)
  {
    m_uiForm.pbRun->setEnabled(true);
  }
}
//----------------------
// Protected member functions
//----------------------
/**
 * Set the properties that have been parsed from the dialog.
 * @returns A boolean that indicates if the validation was successful.
 */
bool Diagnostics::setPropertyValues()
{
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  bool allValid(true);
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    Mantid::Kernel::Property *prop = pitr.value();
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    QLabel *validator = getValidatorMarker(pitr.key());

    std::string error = "";
    if ( !value.isEmpty() )
    {//if there something in the box then use it
      error = prop->setValue(value.toStdString());
    }
    else
    {//else use the default which may or may not be a valid property value
      error = prop->setValue(prop->getDefault());
    }

    if( error.empty() )
    {//no error
      if( validator ) validator->hide();
      //Store value for future input if it is not default
    }
    else
    {//the property could not be set
      allValid = false;
      if( validator && validator->parent() )
      {
        //a description of the problem will be visible to users if they their mouse pointer lingers over validator star mark
        validator->setToolTip(  QString::fromStdString(error) );
        validator->show();
      }
    }
  }
  return allValid;
}
/**
 * This sets up the labels that are to be used to mark whether a property is valid. It has
 * a default implmentation but can be overridden if some other marker is required
 */ 
void Diagnostics::createValidatorLabels()
{
  m_validators.clear();
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QLabel *validLbl = new QLabel("*");
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    m_validators[pitr.key()] = validLbl;
  }
}
///**
// * Save the property values to the input history
// */
//void Diagnostics::saveInput()
//{
//  AlgorithmInputHistory::Instance().clearAlgorithmInput(m_algName);
//  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
//  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
//       pitr != pend; ++pitr )
//  {
//    QString pName = pitr.key();
//    QString value = m_propertyValueMap.value(pName);
//    AlgorithmInputHistory::Instance().storeNewValue(m_algName, QPair<QString, QString>(pName, value));
//  }
//}
/**
 * Get a property validator label
 */
QLabel* Diagnostics::getValidatorMarker(const QString & propname) const
{
  return m_validators.value(propname);
}
/**
 * Adds a property (name,value) pair to the stored map
 */
void Diagnostics::storePropertyValue(const QString & name, const QString & value)
{
  if( name.isEmpty() ) return;
  
  m_propertyValueMap.insert(name, value);
}