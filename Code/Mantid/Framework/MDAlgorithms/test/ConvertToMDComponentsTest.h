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
      return ConvertToMD::preprocessDetectorsPositions(InWS2D,dEModeRequested,updateMasks);
    }
    void setSourceWS(Mantid::API::MatrixWorkspace_sptr InWS2D)
    {
      m_InWS2D = InWS2D;
    }
    Convert2MDComponentsTestHelper()
    {
      ConvertToMD::initialize();
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

     TS_ASSERT_EQUALS("ServiceTableWS",TableWS3->getName());
     TSM_ASSERT("Should not add service WS to the data service",!AnalysisDataService::Instance().doesExist("ServiceTableWS"));

     // now it does not calulates new workspace and takes old from data service
     pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
     auto TableWS4= pAlg->preprocessDetectorsPositions(ws2Dp);
     TS_ASSERT_EQUALS(TableWS4.get(),TableWSs.get());

     // and now it does not take old and caluclated new
     pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS2");
     auto TableWS5= pAlg->preprocessDetectorsPositions(ws2Dp);
     TS_ASSERT(TableWS5.get()!=TableWS4.get());

     // workspace with different number of detectors calculated into different workspace, replacing the previous one into dataservice
      Mantid::API::MatrixWorkspace_sptr ws2DNew =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(9,10,true);
      // this is the probems with alg, as ws has to be added to data service to be avail to algorithm.
      pAlg->setSourceWS(ws2DNew);

      TSM_ASSERT_THROWS("WS has to have input energy for indirect methods",pAlg->preprocessDetectorsPositions(ws2DNew),std::invalid_argument);

      ws2DNew->mutableRun().addProperty("Ei",130,"meV",true);
      auto TableWS6= pAlg->preprocessDetectorsPositions(ws2DNew);
      TS_ASSERT(TableWS6.get()!=TableWS5.get());
      TS_ASSERT_EQUALS(9,TableWS6->rowCount());
      TS_ASSERT_EQUALS(4,TableWS5->rowCount());


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
      
      MDTransfDEHelper DeH;
      auto TableWS7= pAlg->preprocessDetectorsPositions(ws2D,DeH.getEmode(MDEvents::CnvrtToMD::Indir));

      TS_ASSERT_EQUALS(4,TableWS7->rowCount());

      float *pDataArray=TableWS7->getColDataArray<float>("eFixed");
      TS_ASSERT(pDataArray);
      if(!pDataArray)return;

      for(size_t i=0;i<TableWS7->rowCount();i++)
      {
          TS_ASSERT_DELTA(13.f,*(pDataArray+i),1.e-6);
      }


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

#ifdef WIN32
// load Algoritm library to register CloneWorkspace with framoworkManager
     Mantid::Kernel::LibraryWrapper Lw;
     Lw.OpenLibrary("MantidAlgorithms");
#endif
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