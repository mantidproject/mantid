#ifndef MANTID_MD_CONVERT2_Q_NDANY_TEST_H_
#define MANTID_MD_CONVERT2_Q_NDANY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
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

class Convert2AnyTestHelper: public ConvertToMDEvents
{
public:
    Convert2AnyTestHelper(){};
    //
   void buildDimNames(MDEvents::MDWSDescription &TargWSDescription){
       this->ConvertToMDEvents::buildDimensions(TargWSDescription);
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
class ConvertToMDEventsTest : public CxxTest::TestSuite
{
 std::auto_ptr<Convert2AnyTestHelper> pAlg;
public:
static ConvertToMDEventsTest *createSuite() { return new ConvertToMDEventsTest(); }
static void destroySuite(ConvertToMDEventsTest * suite) { delete suite; }    

void testSpecialConversionTOF()
{
    double factor,power;

    const Kernel::Unit_sptr pThisUnit=Kernel::UnitFactory::Instance().create("Wavelength");
    TS_ASSERT(!pThisUnit->quickConversion("MomentumTransfer",factor,power));
}
void testTOFConversionFails()
{ 

    Kernel::Unit_sptr pSourceWSUnit     = Kernel::UnitFactory::Instance().create("Wavelength");
    Kernel::Unit_sptr pWSUnit           = Kernel::UnitFactory::Instance().create("MomentumTransfer");
    double delta;
    double L1(10),L2(10),TwoTheta(0.1),efix(10);
    int emode(0);
    TS_ASSERT_THROWS_NOTHING(pWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
    TS_ASSERT_THROWS_NOTHING(pSourceWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
     
    double X0(5);
    double tof(0) ,k_tr(0);
    TS_ASSERT_THROWS_NOTHING(tof  = pSourceWSUnit->singleToTOF(X0));
    TS_ASSERT_THROWS_NOTHING(k_tr = pWSUnit->singleFromTOF(tof));
}

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 16 propeties",16,(size_t)(pAlg->getProperties().size()));
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


void testExecNoQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OutputWorkspace","WS3DNoQ");
    pAlg->setPropertyValue("UsePreprocessedDetectors","0");
    pAlg->setPropertyValue("QDimensions","CopyToMD");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "NoDE"));
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    AnalysisDataService::Instance().remove("OutputWorkspace");

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
    pAlg->setPropertyValue("UsePreprocessedDetectors","0");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    AnalysisDataService::Instance().remove("OutputWorkspace");

}

void testExecQ3D()
{
    pAlg->setPropertyValue("OutputWorkspace","WS5DQ3D");
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    pAlg->setPropertyValue("UsePreprocessedDetectors","0");
     
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues","-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues"," 10, 10, 10, 20, 40, 20");


    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Shoud finish succesfully",pAlg->isExecuted());
    AnalysisDataService::Instance().remove("OutputWorkspace"); 
}

void test_buildDimNames(){

    MDEvents::MDWSDescription TargWSDescription(4);

    TargWSDescription.u=Kernel::V3D(1,0,0);
    TargWSDescription.v=Kernel::V3D(0,1,0);
    TargWSDescription.emode=1;
    TargWSDescription.AlgID = "WS2DHistoQ3DElasticCnvNo";
    TargWSDescription.convert_to_hkl=true;
    TargWSDescription.rotMatrix.assign(9,0);

    pAlg->buildDimNames(TargWSDescription);
    TS_ASSERT_EQUALS("[Q1,0,0]",TargWSDescription.dimNames[0]);
    TS_ASSERT_EQUALS("[0,Q2,0]",TargWSDescription.dimNames[1]);
    TS_ASSERT_EQUALS("[0,0,Q3]",TargWSDescription.dimNames[2]);
    

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

  ConvertToMDEvents alg;
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
  TSM_ASSERT_EQUALS("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", 4, dEAnalysisModeValues.size());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  dEAnalysisModeValues.find("NoDE") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Direct") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Indirect") != dEAnalysisModeValues.end());
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", dEAnalysisModeValues.find("Elastic") != dEAnalysisModeValues.end());
}


ConvertToMDEventsTest(){
     pAlg = std::auto_ptr<Convert2AnyTestHelper>(new Convert2AnyTestHelper());
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

