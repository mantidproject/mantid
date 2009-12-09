#include "MantidQtCustomInterfaces/Excitations.h"
//#include "MantidQtMantidWidgets/FileInput.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include <string>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QGridLayout>
#include <QString>
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

//----------------------
// Public member functions
//----------------------
///Constructor
Excitations::Excitations(QWidget *parent) : UserSubWindow(parent)
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
  connect(m_uiForm.pbWBV1, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_uiForm.pbWBV2, SIGNAL(clicked()), signalMapper, SLOT(map()));
  
  signalMapper->setMapping(m_uiForm.loadRun_pbBrowse, QString("loadRun_pbBrowse"));
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

//  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));

  connect(m_uiForm.ckRunDiag, SIGNAL(clicked()), this, SLOT(disenableDiag()));
  connect(m_uiForm.ckRunAbsol, SIGNAL(clicked()), this, SLOT(disenableAbsolute()));

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  
  m_uiForm.pbRun->setToolTip("Process run files");
  m_uiForm.pbHelp->setToolTip("Documentation");
}
/// Adds custom widgets, fills in combination boxes and runs setToolTip() on widgets
void Excitations::setUpPage1()
{
  m_algorPropList["RunFiles"] = "LoadRaw.Filename";

  // insert the file loader helper widget
/*  FileInput *mapWidget = new FileInput;
  QLayout *mapLayout = m_uiForm.gbrunMap->layout();
  QGridLayout *mapLay = qobject_cast<QGridLayout*>(mapLayout);
  mapLay->addWidget(mapWidget, 0, 0, 1, -1);*/
m_uiForm.map_fileInput_leName->setText("mari_res.map");
  
  QString runWSMap = "Sum spectra before saving in groups defined by this file (passed to GroupDetectors)";
/*  m_uiForm.gbrunMap->setToolTip(runWSMap);*/m_uiForm.map_fileInput_leName->setToolTip(runWSMap);
/*  mapWidget->setToolTip(runWSMap);*/m_uiForm.map_fileInput_pbBrowse->setToolTip(runWSMap);
  
  m_uiForm.cbNormal->addItem("??post-chopper peak area??");
  m_uiForm.cbNormal->addItem("pre-chopper monitor");
  m_uiForm.cbNormal->addItem("protons (uAh)");
  m_uiForm.cbNormal->addItem("\"no normalization\"");
    
  m_uiForm.loadRun_cbInst->addItem("MAR");
  m_uiForm.loadRun_cbInst->addItem("MAP");
  m_uiForm.loadRun_cbInst->addItem("");
  
  m_uiForm.lbPrefix->setToolTip("For example MAR, MAP, ...");
  m_uiForm.loadRun_cbInst->setToolTip("For example MAR, MAP, ...");
  
  m_uiForm.gbExperiment->setToolTip("Experimental runs to process");
  m_uiForm.loadRun_lbDiscrip->setToolTip("List of runs to load");
  m_uiForm.loadRun_lenumber->setToolTip("List of runs to load");
  m_uiForm.loadRun_pbBrowse->setToolTip("List of runs to load");
  m_uiForm.loadRun_tvRuns->setToolTip("List of runs to load");
  m_uiForm.loadRun_tvRuns->setColumnCount(3);
  m_uiForm.loadRun_tvRuns->setRowCount(1);
  m_uiForm.loadRun_tvRuns->horizontalHeader()->hide();
  m_uiForm.loadRun_tvRuns->verticalHeader()->hide();
  
  m_uiForm.lbNorm->setToolTip("Select the type of normalization for the runs");
  m_uiForm.cbNormal->setToolTip("Select the type of normalization for the runs");
  
  m_uiForm.gbconvUnits->setToolTip("Settings for units conversion to energy transfer");
  m_uiForm.lbEGuess1->setToolTip("Approximate initial neutron energy, is passed to GetEi");
  m_uiForm.leEGuess2->setToolTip("Approximate initial neutron energy, is passed to GetEi");
  m_uiForm.lbEBins->setToolTip("Settings for units conversion to energy transfer (passed to ReBin)");
  m_uiForm.lbELow->setToolTip("Exclude neutrons with less than this energy (meV)");
  m_uiForm.leELow->setToolTip("Exclude neutrons with less than this energy (meV)");
  m_uiForm.lbEHigh->setToolTip("Exclude neutrons with more than this energy (meV)");
  m_uiForm.leEHigh->setToolTip("Exclude neutrons with more than this energy (meV)");
  m_uiForm.lbEWidth->setToolTip("Width of the energy bins (meV)");
  m_uiForm.leEWidth->setToolTip("Width of the energy bins (meV)");
  
  m_uiForm.gbSPE->setToolTip("File name for the converted data");
}
/// Adds custom widgets, fills in combination boxes and runs setToolTip() on widgets
void Excitations::setUpPage2()
{
// default parameters that are writen to the GUI
const char defHighAbsolute[6] = "1e10";
const char defLowAbsolute[2] = "0";
const char defSignificanceTest[4] = "3.3";
const char defHighMedian[4] = "3.0";
const char defLowMedian[4] = "0.1";
const char defVariation[4] = "1.1";
const char defBackground[5] = "5.0";

  m_uiForm.leStartTime->setText("18000");
  m_uiForm.leEndTime->setText("19500");
  
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
  m_algorPropList["OutputFile"] = "MedianDetectorTest.OutputFile";
  
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
  
  // setup the stars that show when a property is invalid and link the stars to the property
  loadAlgorDummies();

  getAlgProperties();   

  createValidatorLabels();

  placeValidatorLabels();

  readTheDialog();
  setPropertyValues();  
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

/*	if setupUi.ckRunDiag->checked()
	{// this part of the form is a separate widget, all the work is coded in over there
	  m_uiForm.diagSec->run();
	}*/

    deltaECalc conversion("setting");
	conversion.run();
  }
  catch (std::exception e)
  {// any exception that works its way passed here would cause QTiplot to suggest that it's shutdown, which I think would be uneccessary
    QMessageBox::critical(this, "", 
      QString("Exception \"") + QString(e.what()) + QString("\" encountered during execution"));
  }
//  Mantid::API::FrameworkManager::Instance().deleteWorkspace(test1.inputWS.toStdString());
}

//replace this function
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
    extensions << "RAW"<< "raw" << "NXS" << "nxs";
  }
  if ( buttonDis == "WBVanadium2")
  {
    editBox = m_uiForm.leWBV2;
    extensions << "RAW"<< "raw" << "NXS" << "nxs";
  }
  if ( buttonDis == "loadRun_pbBrowse")
  {
    editBox = m_uiForm.loadRun_lenumber;
    extensions << "RAW"<< "raw" << "NXS" << "nxs";
  }
  if ( buttonDis == "map_fileInput_pbBrowse" )
  {
    editBox = m_uiForm.map_fileInput_leName;
    extensions << "MAP"<< "map";
  }
  if ( buttonDis == "pbAddMono" )
  {
    editBox = m_uiForm.leMonoVan;
    extensions << "RAW"<< "raw" << "NXS" << "nxs";
  }
  if ( buttonDis == "pbAddWhite")
  {
    editBox = m_uiForm.leWhiteVan;
    extensions << "RAW"<< "raw" << "NXS" << "nxs";
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
  //??STEVES?? fix the hack
  if ( extensions[0] == "RAW" &&  editBox->text().isEmpty() ) editBox->setText(editBox->text()+", "+filepath);
  else editBox->setText(filepath);
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
  m_uiForm.gbUniversal->setEnabled(enabled);
  m_uiForm.gbIndividual->setEnabled(enabled);
  m_uiForm.gbVariation->setEnabled(enabled);
  m_uiForm.gbExperiment->setEnabled(enabled);
 }

/** Creates instances of the algorithms used by the scripts so that we have some properties
*  available for input parameter validation
*/
void Excitations::loadAlgorDummies()
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

  for ( size_t i = 0; i < m_algorDummies.size(); i ++ )
  {
    m_algorDummies[i]->initialize();
  }
}
/// Parse input when the Run button is pressed
bool Excitations::parseInput()
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
void Excitations::readTheDialog()
{
  m_propertyValueMap.clear();
  m_userSettingsMap.clear();

  //  copy the values from each input control into the storage map
  storeUserSetting("RunFiles", m_uiForm.loadRun_lenumber->text());

  // DIAG STUFF MOVE
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
}
/** Link the propery names that we are using here and are all of the form
*  algorithm_name.property_name with a pointer to the property
* @param algList pointers instances of the algorithm used by the scripts
*/
void Excitations::getAlgProperties()
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
    for ( size_t j = 0; j < m_algorDummies.size(); ++j)
    {
      if ( algName == m_algorDummies[j]->name() )
      {
        std::vector<Mantid::Kernel::Property*> propsToAdd =
          m_algorDummies[j]->getProperties();
        // we've found the algorithm now get the property
        for ( size_t k = 0 ; k < propsToAdd.size() ; ++k )
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
void Excitations::placeValidatorLabels()
{
  QLayout *curLayout = m_uiForm.gbExperiment->layout();
  QGridLayout *exp = qobject_cast<QGridLayout*>(curLayout);
  QLabel *validlb = getValidatorMarker("LoadRaw.Filename");
  exp->addWidget(validlb, 0, 2);

  QLayout *currentLayout = m_uiForm.gbUniversal->layout();
  QGridLayout *uni = qobject_cast<QGridLayout*>(currentLayout);
  QLabel *validlbl = getValidatorMarker("MedianDetectorTest.SignificanceTest");
  uni->addWidget(validlbl, 2, 2);

  validlbl = getValidatorMarker("MedianDetectorTest.OutputFile");
  uni->addWidget(validlbl, 1, 3);

  // work on the Individual White Beam Tests groupbox
  currentLayout = m_uiForm.gbIndividual->layout();
  QGridLayout *individGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("LoadRaw.Filename");
  individGrid->addWidget(validlbl, 0, 7);

  // work on the efficency variation test groupbox
  currentLayout = m_uiForm.gbVariation->layout();
  QGridLayout *efficiencyGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("DetectorEfficiencyVariation.Variation");
  efficiencyGrid->addWidget(validlbl, 1, 2);  
}

/** Saves a values into a map for passing to the scripts. Usually these are values the
*  user entered into the form. Some values used by algorithms are copied in to m_propertyValueMap
*  @param variableName the internal name for the setting, not seen by users
*  @param value the value that setting is going to take
*/
void Excitations::storeUserSetting(const std::string &varibleName, const QString &value)
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
/**
 * Set the properties that have been parsed from the dialog.
 * @returns A boolean that indicates if the validation was successful.
 */
bool Excitations::setPropertyValues()
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
void Excitations::createValidatorLabels()
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
// is saving old input values important for interfaces
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
QLabel* Excitations::getValidatorMarker(const QString & propname) const
{
  return m_validators.value(propname);
}
/**
 * Adds a property (name,value) pair to the stored map
 */
void Excitations::storePropertyValue(const QString & name, const QString & value)
{
  if( name.isEmpty() ) return;
  
  m_propertyValueMap.insert(name, value);
}
