#ifndef MANTID_MDALGORITHM_CONVERT2MD_COMPONENTS_TEST_H
#define MANTID_MDALGORITHM_CONVERT2MD_COMPONENTS_TEST_H
// tests for different parts of ConvertToMD exec functions

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidMDEvents/MDWSTransform.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class Convert2MDComponentsTestHelper: public ConvertToMD
{
public:
  TableWorkspace_const_sptr preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D,const std::string dEModeRequested="Direct",bool updateMasks=true)
  {
    std::string OutWSName(this->getProperty("PreprocDetectorsWS"));
    return ConvertToMD::preprocessDetectorsPositions(InWS2D,dEModeRequested,updateMasks,OutWSName);
  }
  void setSourceWS(Mantid::API::MatrixWorkspace_sptr InWS2D)
  {
    this->m_InWS2D = InWS2D;
    // and create the class, which will deal with the target workspace
    if(!this->m_OutWSWrapper) this->m_OutWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
  }
  Convert2MDComponentsTestHelper()
  {
    ConvertToMD::initialize();
  }
  bool buildTargetWSDescription(API::IMDEventWorkspace_sptr spws,const std::string &Q_mod_req,const std::string &dEModeRequested,const std::vector<std::string> &other_dim_names,
    const std::string &QFrame,const std::string &convert_to_,MDEvents::MDWSDescription &targWSDescr)
  {
    std::vector<double> dimMin = this->getProperty("MinValues");
    std::vector<double> dimMax = this->getProperty("MaxValues");
    return ConvertToMD::buildTargetWSDescription(spws,Q_mod_req,dEModeRequested,other_dim_names,dimMin,dimMax,QFrame,convert_to_,targWSDescr);
  }
  void copyMetaData(API::IMDEventWorkspace_sptr mdEventWS) const
  {
    ConvertToMD::copyMetaData(mdEventWS);
  }
  void addExperimentInfo(API::IMDEventWorkspace_sptr mdEventWS, MDEvents::MDWSDescription &targWSDescr) const
  {
     ConvertToMD::addExperimentInfo(mdEventWS,targWSDescr);

  };


  API::IMDEventWorkspace_sptr createNewMDWorkspace(const MDEvents::MDWSDescription &NewMDWSDescription)
  {
    return ConvertToMD::createNewMDWorkspace(NewMDWSDescription);
  }

};


//
class ConvertToMDComponentsTest : public CxxTest::TestSuite
{
  std::auto_ptr<Convert2MDComponentsTestHelper> pAlg;
  Mantid::API::MatrixWorkspace_sptr ws2D;
public:
  static ConvertToMDComponentsTest *createSuite() { return new ConvertToMDComponentsTest(); }
  static void destroySuite(ConvertToMDComponentsTest * suite) { delete suite; }    


  void testPreprocDetLogic()
  {
    Mantid::API::MatrixWorkspace_sptr ws2Dp = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    // if workspace name is specified, it has been preprocessed and added to analysis data service:

    pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
    auto TableWS= pAlg->preprocessDetectorsPositions(ws2Dp);
    auto TableWSs = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("PreprDetWS");
    TS_ASSERT_EQUALS(TableWS.get(),TableWSs.get());
    // does not calculate ws second time:
    auto TableWS2= pAlg->preprocessDetectorsPositions(ws2Dp);
    TS_ASSERT_EQUALS(TableWS2.get(),TableWSs.get());

    // but now it does calculate a new workspace
    pAlg->setPropertyValue("PreprocDetectorsWS","-");
    auto TableWS3= pAlg->preprocessDetectorsPositions(ws2Dp);
    TS_ASSERT(TableWSs.get()!=TableWS3.get());

    TS_ASSERT_EQUALS("",TableWS3->getName()); // if WS isn't in the ADS it doesn't have a name
    TSM_ASSERT("Should not add service WS to the data service",!AnalysisDataService::Instance().doesExist("ServiceTableWS"));

    // now it does not calculates new workspace and takes old from data service
    pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
    auto TableWS4= pAlg->preprocessDetectorsPositions(ws2Dp);
    TS_ASSERT_EQUALS(TableWS4.get(),TableWSs.get());

    // and now it does not take old and calculated new
    pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS2");
    auto TableWS5= pAlg->preprocessDetectorsPositions(ws2Dp);
    TS_ASSERT(TableWS5.get()!=TableWS4.get());

    // workspace with different number of detectors calculated into different workspace, replacing the previous one into dataservice
    Mantid::API::MatrixWorkspace_sptr ws2DNew =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(9,10,true);
    // this is the problems with alg, as ws has to be added to data service to be avail to algorithm.
    pAlg->setSourceWS(ws2DNew);

    // Ei is not defined
    TSM_ASSERT_THROWS("WS has to have input energy for indirect methods",pAlg->preprocessDetectorsPositions(ws2DNew),std::invalid_argument);
    ws2DNew->mutableRun().addProperty("Ei",130.,"meV",true);

    auto TableWS6= pAlg->preprocessDetectorsPositions(ws2DNew);
    TS_ASSERT(TableWS6.get()!=TableWS5.get());
    TS_ASSERT_EQUALS(9,TableWS6->rowCount());
    TS_ASSERT_EQUALS(4,TableWS5->rowCount());

    // Trow on  running the test again if the workspace does not have energy attached.
    ws2DNew->mutableRun().removeProperty("Ei");
    TSM_ASSERT_THROWS("WS has to have input energy for indirect methods despite the table workspace is already calculated",pAlg->preprocessDetectorsPositions(ws2DNew),std::invalid_argument);



  }
  void testUpdateMasksSkipped()
  {
    Mantid::API::MatrixWorkspace_sptr ws2Dp = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");

    auto clVs= Mantid::API::FrameworkManager::Instance().createAlgorithm("CloneWorkspace");
    TS_ASSERT(clVs);
    if(!clVs)return;

    clVs->initialize();
    clVs->setProperty("InputWorkspace",ws2Dp);
    clVs->setProperty("OutputWorkspace","InWSCopy");
    clVs->execute();

    Mantid::API::MatrixWorkspace_sptr ws2DCopy = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("InWSCopy");
    // if workspace name is specified, it has been preprocessed and added to analysis data service:


    pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
    // the workspace has t
    auto TableWS= pAlg->preprocessDetectorsPositions(ws2DCopy,"Direct",false);

    auto &maskCol     = TableWS->getColVector<int>("detMask"); 
    for(size_t i=0;i<maskCol.size();i++)
    {
      TS_ASSERT_EQUALS(0,maskCol[i]);
    }
    // now mask a detector and check if masks are updated;
    maskAllDetectors("InWSCopy");
    // skip recalculating the detectors masks so the workspace should stay the same (untouched return from DS)
    auto TableWS1= pAlg->preprocessDetectorsPositions(ws2DCopy,"Direct",false);

    TS_ASSERT(TableWS.get()==TableWS1.get());
    for(size_t i=0;i<maskCol.size();i++)
    {
      TS_ASSERT_EQUALS(0,maskCol[i]);
    }
    AnalysisDataService::Instance().remove("InWSCopy");
  }


  void testUpdateMasksWorked()
  {
    Mantid::API::MatrixWorkspace_sptr ws2Dp = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    // if workspace name is specified, it has been preprocessed and added to analysis data service:

    pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
    // the workspace has t
    auto TableWS= pAlg->preprocessDetectorsPositions(ws2Dp);

    auto &maskCol     = TableWS->getColVector<int>("detMask"); 
    for(size_t i=0;i<maskCol.size();i++)
    {
      TS_ASSERT_EQUALS(0,maskCol[i]);
    }
    // now mask a detector and check if masks are updated;
    maskAllDetectors("testWSProcessed");
    // recalculate the detectors masks but the workspace should stay the same
    auto TableWS1= pAlg->preprocessDetectorsPositions(ws2Dp);

    TS_ASSERT(TableWS.get()==TableWS1.get());
    for(size_t i=0;i<maskCol.size();i++)
    {
      TS_ASSERT_EQUALS(1,maskCol[i]);
    }
  }

  void testCalcDECol()
  {


    auto TableWS7= pAlg->preprocessDetectorsPositions(ws2D,Kernel::DeltaEMode::asString(Kernel::DeltaEMode::Indirect));

    TS_ASSERT_EQUALS(4,TableWS7->rowCount());

    float *pDataArray=TableWS7->getColDataArray<float>("eFixed");
    TS_ASSERT(pDataArray);
    if(!pDataArray)return;

    for(size_t i=0;i<TableWS7->rowCount();i++)
    {
      TS_ASSERT_DELTA(13.f,*(pDataArray+i),1.e-6);
    }


  }

  void testAddExperimentInfo()
  {
    Mantid::API::MatrixWorkspace_sptr ws2Dp = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");

    API::IMDEventWorkspace_sptr spws;
    // create testing part of the algorithm
    Convert2MDComponentsTestHelper subAlgo;
    // set source workspace as it would be used by the algorithm itself;
    subAlgo.setSourceWS(ws2Dp);
    // and min-max values (they are still needed by the algorithm)
    subAlgo.setPropertyValue("MinValues","-10");
    subAlgo.setPropertyValue("MaxValues","10");

    bool createNewTargetWs(false);
    std::vector<std::string> Q_modes = MDEvents::MDTransfFactory::Instance().getKeys();
    std::string dE_mode = Kernel::DeltaEMode().asString(Kernel::DeltaEMode::Elastic);
    MDWSTransform QScl;
    std::vector<std::string> QScales = QScl.getQScalings();
    std::vector<std::string> Frames = QScl.getTargetFrames();

    MDEvents::MDWSDescription targWSDescr;
    TS_ASSERT_THROWS_NOTHING(createNewTargetWs=subAlgo.buildTargetWSDescription(spws,Q_modes[0],dE_mode,std::vector<std::string>(),
      Frames[CnvrtToMD::AutoSelect],QScales[CnvrtToMD::NoScaling],targWSDescr));

    TSM_ASSERT("as spws is null pointer, this should request creating new workspace ",createNewTargetWs)

      TS_ASSERT_THROWS_NOTHING(spws = subAlgo.createNewMDWorkspace(targWSDescr));
    TS_ASSERT(spws);
    if(!spws)return;

    // copy the experiment info and get the unique number, that identifies the run, the source workspace came from.
    TS_ASSERT_THROWS_NOTHING(subAlgo.addExperimentInfo(spws,targWSDescr));

    uint16_t runIndex(1000);
    TS_ASSERT_THROWS_NOTHING(runIndex=targWSDescr.getPropertyValueAsType<uint16_t>("RUN_INDEX"));
    TS_ASSERT_EQUALS(0,runIndex);

    // target workspace has W-matrix, which should be unit matrix
    TS_ASSERT(spws->getExperimentInfo(0)->run().hasProperty("W_MATRIX"));
    // it also has transformation matrix    
    TS_ASSERT(spws->getExperimentInfo(0)->run().hasProperty("RUBW_MATRIX"));

    if(!spws->getExperimentInfo(0)->run().hasProperty("W_MATRIX"))return;

    Kernel::DblMatrix UnitMatr(3,3,true);
    std::vector<double> libWMatr;

    TS_ASSERT_THROWS_NOTHING(libWMatr=spws->getExperimentInfo(0)->run().getPropertyValueAsType<std::vector<double> >("W_MATRIX"));

    Kernel::DblMatrix wMatr(libWMatr);
    TSM_ASSERT("We have not set up anything so it should be unit matrix",wMatr.equals(UnitMatr));
  }
  void xestCopyMetadata()
  {
    // this test should be enabled in some form
    //TS_ASSERT_THROWS_NOTHING(subAlgo.copyMetaData(spws));

  }

  ConvertToMDComponentsTest()
  {
    pAlg = std::auto_ptr<Convert2MDComponentsTestHelper>(new Convert2MDComponentsTestHelper());
    ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
    // add workspace energy
    ws2D->mutableRun().addProperty("eFixed",13.,"meV",true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    Mantid::API::FrameworkManager::Instance();
  }
  ~ConvertToMDComponentsTest()
  {
    AnalysisDataService::Instance().remove("testWSProcessed");
  }

  void maskAllDetectors(const std::string &wsName)
  {
    auto inputWS  =boost::dynamic_pointer_cast<API::MatrixWorkspace >(API::AnalysisDataService::Instance().retrieve(wsName));
    const size_t nRows = inputWS->getNumberHistograms();

    // build detectors ID list to mask
    std::vector<detid_t> detectorList;  detectorList.reserve(nRows);
    std::vector<size_t> indexLis;    indexLis.reserve(nRows);
    for (size_t i = 0; i < nRows; i++)
    {   
      // get detector or detector group which corresponds to the spectra i
      Geometry::IDetector_const_sptr spDet;
      try
      {
        spDet= inputWS->getDetector(i);      
      }
      catch(Kernel::Exception::NotFoundError &)
      {
        continue;
      }

      // Check that we aren't dealing with monitor...
      if (spDet->isMonitor())continue;   

      indexLis.push_back(i);
      //detectorList.push_back(spDet->getID());
    }

    std::vector<size_t>::const_iterator wit;
    for (wit = indexLis.begin(); wit != indexLis.end(); ++wit)
    {
      inputWS->maskWorkspaceIndex(*wit);  
    }

  }

};
#endif
