#ifndef MANTID_MD_CONVERT2_Q_NDANY_TEST_H_
#define MANTID_MD_CONVERT2_Q_NDANY_TEST_H_

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

class Convert2AnyTestHelper: public ConvertToMD
{
public:
    Convert2AnyTestHelper(){};
    TableWorkspace_const_sptr preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D)
    {
      return ConvertToMD::preprocessDetectorsPositions(InWS2D);
    }
    void setSourceWS(Mantid::API::MatrixWorkspace_sptr InWS2D)
    {
      m_InWS2D = InWS2D;
    }

};
// helper function to provide list of names to test:
std::vector<std::string> dim_availible()
{
    std::string dns_ws[]={"DeltaE","T","alpha","beta","gamma"};
    std::vector<std::string> data_names_in_WS;
    for(size_t i=0;i<5;i++){
        data_names_in_WS.push_back(dns_ws[i]);
    }
    return data_names_in_WS;
}
//
class ConvertToMDTest : public CxxTest::TestSuite
{
 std::auto_ptr<Convert2AnyTestHelper> pAlg;
public:
static ConvertToMDTest *createSuite() { return new ConvertToMDTest(); }
static void destroySuite(ConvertToMDTest * suite) { delete suite; }    


void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 18 propeties",18,(size_t)(pAlg->getProperties().size()));
}


void testSetUpThrow()
{
    //TODO: check if wrong WS throws (should on validator)

     // get ws from the DS    
     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     // give it to algorithm
    TSM_ASSERT_THROWS_NOTHING("the inital ws is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    // target ws fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransferND"));
    // unknown Q-dimension trows
    TS_ASSERT_THROWS(pAlg->setPropertyValue("QDimensions", "unknownQ"),std::invalid_argument);
    // correct Q-dimension fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "|Q|"));
    // additional dimensions requested -- fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", "DeltaE,omega"));

}

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

void testExecNoQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OutputWorkspace","WS3DNoQ");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
    pAlg->setPropertyValue("QDimensions","CopyToMD");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
//    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "NoDE"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode","Elastic")); // dE mode will be ignored
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DNoQ");
    AnalysisDataService::Instance().remove("WS3DNoQ");

}

void testExecModQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("OutputWorkspace","WS3DmodQ");
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("QDimensions","|Q|");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DmodQ");
    AnalysisDataService::Instance().remove("WS3DmodQ");

}

void testExecQ3D()
{
    pAlg->setPropertyValue("OutputWorkspace","WS5DQ3D");
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
     
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues","-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues"," 10, 10, 10, 20, 40, 20");


    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Shoud finish succesfully",pAlg->isExecuted());
    checkHistogramsHaveBeenStored("WS5DQ3D");
    AnalysisDataService::Instance().remove("WS5DQ3D");
}

//DO NOT DISABLE THIS TEST
void testAlgorithmProperties()
{
  /*
  The Create MD Workspace GUI runs this algorithm internally.
  If property names and property allowed values here change, that interface will break.

  This unit test is designed to flag up changes here. If property values and names here do need to be changed, 
  1) They must also be updated in CreateMDWorkspaceAlgDialog.cpp. 
  2) It should then be confirmed that that the Create MD Workspace custom interface still works!
  3) Finally this unit test should be updated so that the tests pass.
  */

  ConvertToMD alg;
  alg.initialize();

  Mantid::Kernel::Property *QDimProperty;
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("QDimensions"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("dEAnalysisMode"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("OtherDimensions"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("MinValues"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("MaxValues"));

  typedef std::set<std::string> PropertyAllowedValues;
  QDimProperty =alg.getProperty("QDimensions");
  PropertyAllowedValues QDimValues = QDimProperty->allowedValues();
  TSM_ASSERT_EQUALS("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", 3, QDimValues.size());
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  QDimValues.find("CopyToMD") != QDimValues.end());
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimValues.find("|Q|") != QDimValues.end());
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimValues.find("Q3D") != QDimValues.end());

  Mantid::Kernel::Property *dEAnalysisMode =alg.getProperty("dEAnalysisMode");
  PropertyAllowedValues dEAnalysisModeValues = dEAnalysisMode->allowedValues();
  TSM_ASSERT_EQUALS("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", 3, dEAnalysisModeValues.size());
//  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  dEAnalysisModeValues.find("NoDE") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Direct") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Indirect") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Elastic") != dEAnalysisModeValues.end());
}


ConvertToMDTest(){
     pAlg = std::auto_ptr<Convert2AnyTestHelper>(new Convert2AnyTestHelper());
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
}
private:

  void checkHistogramsHaveBeenStored(const std::string & wsName)
  {
    IMDEventWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    uint16_t nexpts = outputWS->getNumExperimentInfo();
    for(uint16_t i = 0; i < nexpts; ++i)
    {
      ExperimentInfo_const_sptr expt = outputWS->getExperimentInfo(i);
      std::pair<double,double> bin = expt->run().histogramBinBoundaries(0.34);
      TS_ASSERT_DELTA(bin.first, 0.3, 1e-10);
      TS_ASSERT_DELTA(bin.second, 0.4, 1e-10);
    }
  }
};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

