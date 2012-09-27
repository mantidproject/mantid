#ifndef MANTID_MDALGORITHM_CONVERT2MD_COMPONENTS_TEST_H
#define MANTID_MDALGORITHM_CONVERT2MD_COMPONENTS_TEST_H
// tests for different parts of ConvertToMD exec functions

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
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
    TableWorkspace_const_sptr preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D,const std::string dEModeRequested="Direct")
    {
      return ConvertToMD::preprocessDetectorsPositions(InWS2D,dEModeRequested);
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
     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     // if workspace name is specified, it has been preprocessed and added to analysis data service:

     pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
     auto TableWS= pAlg->preprocessDetectorsPositions(ws2D);
     auto TableWSs = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("PreprDetWS");
     TS_ASSERT_EQUALS(TableWS.get(),TableWSs.get());
     // does not calculate ws second time:
     auto TableWS2= pAlg->preprocessDetectorsPositions(ws2D);
     TS_ASSERT_EQUALS(TableWS2.get(),TableWSs.get());

     // but now it does calculate a new workspace
     pAlg->setPropertyValue("PreprocDetectorsWS","");
     auto TableWS3= pAlg->preprocessDetectorsPositions(ws2D);
     TS_ASSERT(TableWSs.get()!=TableWS3.get());

     TS_ASSERT_EQUALS("ServiceTableWS",TableWS3->getName());
     TSM_ASSERT("Should not add service WS to the data service",!AnalysisDataService::Instance().doesExist("ServiceTableWS"));

     // now it does not calulates new workspace and takes old from data service
     pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS");
     auto TableWS4= pAlg->preprocessDetectorsPositions(ws2D);
     TS_ASSERT_EQUALS(TableWS4.get(),TableWSs.get());

     // and now it does not take old and caluclated new
     pAlg->setPropertyValue("PreprocDetectorsWS","PreprDetWS2");
     auto TableWS5= pAlg->preprocessDetectorsPositions(ws2D);
     TS_ASSERT(TableWS5.get()!=TableWS4.get());

     // workspace with different number of detectors calculated into different workspace, replacing the previous one into dataservice
      Mantid::API::MatrixWorkspace_sptr ws2DNew =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(9,10,true);
      // this is the probems with alg, as ws has to be added to data service to be avail to algorithm.
      pAlg->setSourceWS(ws2DNew);

      auto TableWS6= pAlg->preprocessDetectorsPositions(ws2DNew);
      TS_ASSERT(TableWS6.get()!=TableWS5.get());
      TS_ASSERT_EQUALS(9,TableWS6->rowCount());
      TS_ASSERT_EQUALS(4,TableWS5->rowCount());


}

void testCalcDECol()
{
      
      MDTransfDEHelper DeH;
      auto TableWS7= pAlg->preprocessDetectorsPositions(ws2D,DeH.getEmode(MDEvents::CnvrtToMD::Indir));

      TS_ASSERT_EQUALS(4,TableWS7->rowCount());

      float *pDataArray=TableWS7->getColDataArray<float>("eFixed");
      TS_ASSERT(pDataArray);

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
}
~ConvertToMDComponentsTest()
{
    AnalysisDataService::Instance().remove("testWSProcessed");
}
};
#endif