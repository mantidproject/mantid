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
   // private (PROTECTED) methods, exposed for testing:
     void getDimensionNamesFromWSMatrix(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &ws_dimensions,std::vector<std::string> &ws_units)const{
        return ConvertToMDEvents::getDimensionNamesFromWSMatrix(inMatrixWS,ws_dimensions,ws_units);
    }
     //
    std::vector<std::string> get_dimension_names(MatrixWorkspace_const_sptr inMatrixWS){      
       std::vector<std::string> ws_dim_names,ws_dim_units;
       return ConvertToMDEvents::getDimensionNames(inMatrixWS,ws_dim_names,ws_dim_units);
    }
    std::string identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,std::vector<std::string> &outws_dim_names){
        return ConvertToMDEvents::identifyMatrixAlg(inMatrixWS,Q_mode_req, dE_mode_req,outws_dim_names);
    }

   //std::string identify_requested_alg(const std::vector<std::string> &dim_names_availible, const std::string &QOption,const std::vector<std::string> &dim_selected,size_t &nDims)
   //{ 
   //    return   ConvertToMDEvents::identifyTheAlg(dim_names_availible,QOption,dim_selected,nDims);
   //}

   //void run_algo(const std::string &algo_id){   
   //// call selected algorithm
   //     pMethod algo =  alg_selector[algo_id];
   //     if(algo){
   ////         algo(this);
   //     }else{
   //         g_log.error()<<"requested undefined subalgorithm :"<<algo_id<<std::endl;
   //         throw(std::invalid_argument("undefined subalgoritm requested "));
   //     }      
   //}
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

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 8 propeties",8,(size_t)(pAlg->getProperties().size()));
}
// --> GET DIMENSIONS FROM WS MATRIX:
void testGetWSDimNamesWsNoAxisThrows(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    ws2D->replaceAxis(0,new API::TextAxis(3));
    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    TS_ASSERT_THROWS(pAlg->getDimensionNamesFromWSMatrix(ws2D,dim_names,dim_units),std::invalid_argument);
}
void testGetWSDimNamesFine(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);

    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    TS_ASSERT_THROWS_NOTHING(pAlg->getDimensionNamesFromWSMatrix(ws2D,dim_names,dim_units));

    TSM_ASSERT_EQUALS("Inelastic workspace can produce 3 dimension types",3,dim_names.size());
    TSM_ASSERT_EQUALS("Simpe Inelastic workspace has units along X axis only: ",1,dim_units.size());
    TSM_ASSERT_EQUALS("Inelastic workspace units are DeltaE only","DeltaE",dim_units[0]);

}
void testGetWSDimNames2AxisNoQ(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title() = "Dim1";
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);

    pAx = new API::NumericAxis(3);
    pAx->title() = "Dim2";
    pAx->setUnit("QSquared");
    ws2D->replaceAxis(1,pAx);

    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    TS_ASSERT_THROWS_NOTHING(pAlg->getDimensionNamesFromWSMatrix(ws2D,dim_names,dim_units));

    TS_ASSERT_EQUALS(2,dim_names.size());
    TS_ASSERT_EQUALS("Dim1",dim_names[0]);
    TS_ASSERT_EQUALS("Dim2",dim_names[1]);

    TS_ASSERT_EQUALS(2,dim_units.size());
    TS_ASSERT_EQUALS("dSpacing",dim_units[0]);
    TS_ASSERT_EQUALS("QSquared",dim_units[1]);

}
// --> GET ALL DIMENSION NAMES:
void testGetDimNames(){
    // get ws from the DS    
    Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
    // check the private function
    std::vector<std::string> dim_names = pAlg->get_dimension_names(ws2D);

   TSM_ASSERT_EQUALS("the algorithm for this workpace can choose from 7 properties/dimensions",7,dim_names.size());

   std::vector<std::string> basic_properties(7);
   basic_properties[0]="|Q|";
   basic_properties[1]="QxQyQz";
   basic_properties[2]="DeltaE";
   basic_properties[3]="phi";
   basic_properties[4]="chi";
   basic_properties[5]="omega";
   basic_properties[6]="Ei";
   for(size_t i=0;i<basic_properties.size();i++){
        TSM_ASSERT_EQUALS("The workspace property have to be specific",basic_properties[i],dim_names[i]);
   }
}
void testIdentifyMatrixAlg_1()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    
    std::vector<std::string> ws_dim_names(2);
    std::vector<std::string> dim_names;
    ws_dim_names[0]="A";
    ws_dim_names[1]="B";

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title() = ws_dim_names[0];
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);
    pAx = new API::NumericAxis(3);
    pAx->title() = ws_dim_names[1];
    pAx->setUnit("QSquared");
    ws2D->replaceAxis(1,pAx);


    TS_ASSERT_EQUALS("",pAlg->identifyMatrixAlg(ws2D,"","",dim_names));
    TS_ASSERT_EQUALS(ws_dim_names[0],dim_names[0]);
    TS_ASSERT_EQUALS(ws_dim_names[1],dim_names[1]);

}
void testIdentifyMatrixAlg_2InvalidArg()
{  
    std::vector<std::string> dim_names;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    TSM_ASSERT_THROWS("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names),std::invalid_argument);
}
void testIdentifyMatrixAlg_2()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
     size_t nDims;
    std::vector<std::string> dim_names;
 
    API::NumericAxis *
    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names));

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Wavelength");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names));

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Energy");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names));

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Energy_inWavenumber");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names));


    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);
    TS_ASSERT_EQUALS("|Q|Elastic",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names));

    TSM_ASSERT_EQUALS("One dim name came from Q (this can be logically wrong)",1,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");

}
void testIdentifyMatrixAlg_3Throws()
{  
    std::vector<std::string> dim_names;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    TSM_ASSERT_THROWS("Inelastic conversion needs X-axis to be in an Energy-transfer related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Direct",dim_names),std::invalid_argument);
}
void testIdentifyMatrixAlg_3()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    std::vector<std::string> dim_names;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("|Q|Direct",pAlg->identifyMatrixAlg(ws2D,"|Q|","Direct",dim_names));
    TSM_ASSERT_EQUALS("One dimension comes from Q",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
}
void testIdentifyMatrixAlg_4throws()
{  
    std::vector<std::string> dim_names;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    TSM_ASSERT_THROWS("Inelastic conversion needs X-axis to be in an Energy-transfer related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Indirect",dim_names),std::invalid_argument);

}
void testIdentifyMatrixAlg_4()
{  
     std::vector<std::string> dim_names;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("|Q|Indirect",pAlg->identifyMatrixAlg(ws2D,"|Q|","Indirect",dim_names));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
}
void testIdentifyMatrixAlg_5()
{  
    std::vector<std::string> dim_names;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("QxQyQzIndirect",pAlg->identifyMatrixAlg(ws2D,"QxQyQz","Indirect",dim_names));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",4,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"Q_h");
    TS_ASSERT_EQUALS(dim_names[1],"Q_k");
    TS_ASSERT_EQUALS(dim_names[2],"Q_l");
    TS_ASSERT_EQUALS(dim_names[3],"DeltaE");
}

void testSetUpThrow()
{
    //TODO: check if wrong WS throws (should on validator)

     // get ws from the DS    
     Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
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
void xtestAlgoSelectorThrowsWrongNDim()
{
    std::vector<std::string> data_names_in_WS = dim_availible();

    std::vector<std::string> dim_requested(1);
    dim_requested[0]="AA";
    size_t nDims;
   // TSM_ASSERT_THROWS("AA property is unavalible among ws parameters ",pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims),std::invalid_argument);
}

void xtestAlgoSelectorThrowsInalidQ()
{
   std::vector<std::string> data_names_in_WS = dim_availible();

    std::vector<std::string> dim_requested(2);
    dim_requested[0]="DeltaE";
    dim_requested[1]="alpha";
    size_t nDims;
    //TSM_ASSERT_THROWS("Invalid Q argument ",pAlg->identify_requested_alg(data_names_in_WS,"wrong",dim_requested,nDims),std::invalid_argument);
}
     
void xtestAlgoSelector0()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(2);
    size_t nDims;

    dim_requested[0]="T";
    dim_requested[1]="alpha";
  //  TS_ASSERT_EQUALS("NoQND2",pAlg->identify_requested_alg(data_names_in_WS,"",dim_requested,nDims));
    TS_ASSERT_EQUALS(2,nDims);
}
void xtestAlgoSelector1()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(1);
    size_t nDims;

    dim_requested[0]="DeltaE";
  //  TS_ASSERT_EQUALS("modQdE2",pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(2,nDims);
}
void xtestAlgoSelector2()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(2);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
  //  TS_ASSERT_EQUALS("modQND3",pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(3,nDims);
}
void xtestAlgoSelector3()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(3);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    dim_requested[2]="DeltaE";
  //  TS_ASSERT_EQUALS("modQdEND4",pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(4,nDims);
}
void xtestAlgoSelector4()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested;
    size_t nDims;

   // TS_ASSERT_EQUALS("Q3D3",pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(3,nDims);
  
}
void xtestAlgoSelector5()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(1);
    size_t nDims;
    dim_requested[0]="DeltaE";
  //  TS_ASSERT_EQUALS("Q3DdE4",pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(4,nDims);
  
}
void xtestAlgoSelector6()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(2);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
  //  TS_ASSERT_EQUALS("Q3DND5",pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(5,nDims);  
}
void xtestAlgoSelector7()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(3);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    dim_requested[2]="DeltaE";
   // TS_ASSERT_EQUALS("Q3DdEND6",pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(6,nDims);
}


void testExecModQ()
{
    
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("QDimensions","|Q|");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    //TODO: wrong -- q should generate 2 dimensions -- currently 1
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS(pAlg->execute(),Kernel::Exception::NotImplementedError);
    AnalysisDataService::Instance().remove("OutputWorkspace");

}
void testExecQ3D()
{
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
     
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "QxQyQz"));
    pAlg->setPropertyValue("MinValues","-10,-10,-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10, 10,10,20,40");


    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Shoud finish succesfully",pAlg->isExecuted());
 
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

