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
   // private (PROTECTED) methods, exposed for testing:
     void getAddDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &add_dimensions,std::vector<std::string> &add_units)const{
              ConvertToMDEvents::getAddDimensionNames(inMatrixWS,add_dimensions,add_units);
    }
     //
    std::string identifyTheAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                              const std::vector<std::string> &other_dim_names,MDEvents::MDWSDescription &TWSD)
    {
       
                              return ConvertToMDEvents::identifyTheAlg(inMatrixWS,Q_mode_req, dE_mode_req,other_dim_names,TWSD);
    }
    std::string identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                                  std::vector<std::string> &outws_dim_names,std::vector<std::string> &outws_dim_units){
        return ConvertToMDEvents::identifyMatrixAlg(inMatrixWS,Q_mode_req, dE_mode_req,outws_dim_names,outws_dim_units);
    }
    //
    std::string parseQMode(const std::string &Q_mode_req,const std::vector<std::string> &ws_dim_names,const std::vector<std::string> &ws_dim_units,
                           std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units, int &nQ_dims)
    {

      return ConvertToMDEvents::parseQMode(Q_mode_req,ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims);
    }
    //
    std::string parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const std::vector<std::string> &ws_dim_units,
                            std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units, 
                               int &ndE_dims,std::string &natural_units)
   {
       return ConvertToMDEvents::parseDEMode(Q_MODE_ID,dE_mode_req,ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units);
    
   }
   std::string parseConvMode(const std::string &Q_MODE_ID,const std::string &natural_units,const std::vector<std::string> &ws_dim_units){
       return ConvertToMDEvents::parseConvMode(Q_MODE_ID,natural_units,ws_dim_units);
   }
  
   void setAlgoID(const std::string &newID){
       ConvertToMDEvents::setAlgoID(newID);
   }
   void setAlgoUnits(int emode){
          ConvertToMDEvents::setAlgoUnits(emode);
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

    TSM_ASSERT_EQUALS("algortithm should have 13 propeties",13,(size_t)(pAlg->getProperties().size()));
}
// TEST QMode
void testParseQMode_WrongThrows()
{
     std::vector<std::string> ws_dim_names;
     std::vector<std::string> ws_dim_units;
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;    
     TS_ASSERT_THROWS(pAlg->parseQMode("WrongMode",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims),std::invalid_argument);

}
void testParseQMode_NoQ()
{
     std::vector<std::string> ws_dim_names(2,"A");
     std::vector<std::string> ws_dim_units(2,"UnA");
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;
     std::string MODE;
     TS_ASSERT_THROWS_NOTHING(MODE=pAlg->parseQMode("",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims));
     TS_ASSERT_EQUALS(2,nQ_dims);
     TS_ASSERT_EQUALS("",MODE);
     TS_ASSERT_EQUALS(ws_dim_names[0],out_dim_names[0]);
     TS_ASSERT_EQUALS(ws_dim_names[1],out_dim_names[1]);
     TS_ASSERT_EQUALS(ws_dim_units[0],out_dim_units[0]);
     TS_ASSERT_EQUALS(ws_dim_units[1],out_dim_units[1]);
}
void testParseQMode_modQ()
{
     std::vector<std::string> ws_dim_names(2,"A");
     std::vector<std::string> ws_dim_units(2,"UnA");
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;
     std::string MODE;
     TS_ASSERT_THROWS_NOTHING(MODE=pAlg->parseQMode("|Q|",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims));
     TS_ASSERT_EQUALS(1,nQ_dims);
     TS_ASSERT_EQUALS("|Q|",MODE);
     TS_ASSERT_EQUALS("|Q|",out_dim_names[0]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[0]);
}
void testParseQMode_Q3D()
{
     std::vector<std::string> ws_dim_names(2,"A");
     std::vector<std::string> ws_dim_units(2,"UnA");
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;
     std::string MODE;
     TS_ASSERT_THROWS_NOTHING(MODE=pAlg->parseQMode("QxQyQz",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims));
     TS_ASSERT_EQUALS(3,nQ_dims);
     TS_ASSERT_EQUALS("QxQyQz",MODE);
     TS_ASSERT_EQUALS("Q_x",out_dim_names[0]);
     TS_ASSERT_EQUALS("Q_y",out_dim_names[1]);
     TS_ASSERT_EQUALS("Q_z",out_dim_names[2]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[0]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[1]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[2]);
}

// TEST dE mode
void testParseDEMode_WrongThrows()
{

     std::vector<std::string> ws_dim_units;
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;

     TS_ASSERT_THROWS(pAlg->parseDEMode("SOMEQMODE","WrongMode",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units),std::invalid_argument);
}
void testParseDEMode_NoQ()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;

     TS_ASSERT_THROWS_NOTHING(EID=pAlg->parseDEMode("","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(0,ndE_dims);

     TSM_ASSERT_EQUALS("Regardless of the dE mode, if Q-mode is NoQ, should return Any_Mode: ","",EID);
     TS_ASSERT(out_dim_names.empty());
     TS_ASSERT(out_dim_units.empty());
     TS_ASSERT_EQUALS(ws_dim_units[0],natural_units);
}
void testParseDEMode_InelasticDirect()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;

     TS_ASSERT_THROWS_NOTHING(EID=pAlg->parseDEMode("DoesNotMatter","Direct",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(1,ndE_dims);
 
     TS_ASSERT_EQUALS("Direct",EID);
     TS_ASSERT_EQUALS("DeltaE",out_dim_names[0]);
     TS_ASSERT_EQUALS("DeltaE",out_dim_units[0]);
     TS_ASSERT_EQUALS("DeltaE",natural_units);
}
void testParseDEMode_InelasticInDir()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;

     TS_ASSERT_THROWS_NOTHING(EID=pAlg->parseDEMode("DoesNotMatter","Indirect",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(1,ndE_dims);
     TS_ASSERT_EQUALS("Indirect",EID);
     TS_ASSERT_EQUALS("DeltaE",out_dim_names[0]);
     TS_ASSERT_EQUALS("DeltaE",out_dim_units[0]);
     TS_ASSERT_EQUALS("DeltaE",natural_units);
}
void testParseDEMode_Elastic()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;

     TS_ASSERT_THROWS_NOTHING(EID=pAlg->parseDEMode("DoesNotMatter","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(0,ndE_dims);
     TS_ASSERT_EQUALS("Elastic",EID);
     TS_ASSERT(out_dim_names.empty());
     TS_ASSERT(out_dim_units.empty());
     TS_ASSERT_EQUALS("Momentum",natural_units);
}
void testParseDEMode_ElasticPowd()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;

     TS_ASSERT_THROWS_NOTHING(EID=pAlg->parseDEMode("|Q|","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(0,ndE_dims);
     TS_ASSERT_EQUALS("Elastic",EID);
     TS_ASSERT(out_dim_names.empty());
     TS_ASSERT(out_dim_units.empty());
     TS_ASSERT_EQUALS("Momentum",natural_units);
}
// TEST ConvertMode
void testParseConv_NonConvertUnitThrows()
{
     std::vector<std::string> ws_dim_units(1,"wrong");
     std::string natural_units;

     TS_ASSERT_THROWS(pAlg->parseConvMode("AnyConversionMode",natural_units,ws_dim_units),std::invalid_argument);
}
void testParseConv_ElasticViaTOFNotThrowsAnyMore()
{
     std::vector<std::string> ws_dim_units(1,"DeltaE");
     std::string natural_units;
     std::string CONV_ID;

     // satisfy internal dependancies (debug only!!!)
     pAlg->setAlgoID("blaBla-Elastic-BlaBlaBla");
     pAlg->setAlgoUnits(0);

     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("Elastic",natural_units,ws_dim_units));
     TS_ASSERT_EQUALS("CnvByTOF",CONV_ID);
}
void testParseConv_NoQ()
{
     std::vector<std::string> ws_dim_units(1,"Any");
     std::string CONV_ID;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("","AnyUnits",ws_dim_units));
     TS_ASSERT_EQUALS("CnvNo",CONV_ID);
}
void testParseConv_NaturalNoQ()
{
     std::vector<std::string> ws_dim_units(1,"dSpacing");
     std::string CONV_ID;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("","dSpacing",ws_dim_units));
     TS_ASSERT_EQUALS("CnvNo",CONV_ID);
}
void testParseConv_QuickConvertsion()
{
     std::vector<std::string> ws_dim_units(1,"dSpacing");
     std::string CONV_ID;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("AnyMode","MomentumTransfer",ws_dim_units));
     TS_ASSERT_EQUALS("CnvFast",CONV_ID);
}
void testParseConv_FromTOF()
{
     std::vector<std::string> ws_dim_units(1,"TOF");
     std::string CONV_ID;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("AnyMode","MomentumTransfer",ws_dim_units));
     TS_ASSERT_EQUALS("CnvFromTOF",CONV_ID);
}
void testParseConv_ByTOF()
{
     std::vector<std::string> ws_dim_units(1,"DeltaE");
     std::string CONV_ID;
     // satisfy internal dependancies
     pAlg->setAlgoID("blaBla-Direct-BlaBlaBla");
     pAlg->setAlgoUnits(1);

    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=pAlg->parseConvMode("AnyMode","Wavelength",ws_dim_units));
     TS_ASSERT_EQUALS("CnvByTOF",CONV_ID);
}


// --> GET DIMENSIONS FROM WS MATRIX:
void testNeedsNumericAxis(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    ws2D->replaceAxis(0,new API::TextAxis(3));
    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    TS_ASSERT_THROWS(pAlg->identifyMatrixAlg(ws2D,"QxQyQz","",dim_names,dim_units),std::invalid_argument);
}
void testGetWS4DimIDFine(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);

    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    std::string Alg_ID;
    TS_ASSERT_THROWS_NOTHING(Alg_ID=pAlg->identifyMatrixAlg(ws2D,"QxQyQz","Direct",dim_names,dim_units));

    TSM_ASSERT_EQUALS("Inelastic workspace will produce 4 dimensions",4,dim_names.size());
    TSM_ASSERT_EQUALS("Last dimension of Inelastic transformation should be DeltaE","DeltaE",dim_units[3]);
    TSM_ASSERT_EQUALS("Alg ID would be: ","WS2DQxQyQzDirectCnvNo",Alg_ID);
}
void testGetWS3DimIDFine(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title() = "Dim1";
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);


    std::vector<std::string> dim_names;
    std::vector<std::string> dim_units;
    std::string Alg_ID;
    TS_ASSERT_THROWS_NOTHING(Alg_ID=pAlg->identifyMatrixAlg(ws2D,"QxQyQz","Elastic",dim_names,dim_units));

    TSM_ASSERT_EQUALS("Inelastic workspace will produce 3 dimensions",3,dim_names.size());
    TSM_ASSERT_EQUALS("Last dimension of Elastic transformation should be ","Momentum",dim_units[2]);
    TSM_ASSERT_EQUALS("Alg ID would be: ","WS2DQxQyQzElasticCnvByTOF",Alg_ID);

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
    std::string AlgID;
    TS_ASSERT_THROWS_NOTHING(AlgID=pAlg->identifyMatrixAlg(ws2D,"","",dim_names,dim_units));

    TS_ASSERT_EQUALS(2,dim_names.size());
    TS_ASSERT_EQUALS("Dim1",dim_names[0]);
    TS_ASSERT_EQUALS("Dim2",dim_names[1]);

    TS_ASSERT_EQUALS(2,dim_units.size());
    TS_ASSERT_EQUALS("dSpacing",dim_units[0]);
    TS_ASSERT_EQUALS("QSquared",dim_units[1]);

}
// --> GET ALL DIMENSION NAMES:
//void xtestGetDimNames(){
//    // get ws from the DS    
//    Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
//    // check the private function
//    std::vector<std::string> dim_names = pAlg->get_dimension_names(ws2D);
//
//   TSM_ASSERT_EQUALS("the algorithm for this workpace can choose from 7 properties/dimensions",7,dim_names.size());
//
//   std::vector<std::string> basic_properties(7);
//   basic_properties[0]="|Q|";
//   basic_properties[1]="QxQyQz";
//   basic_properties[2]="DeltaE";
//   basic_properties[3]="phi";
//   basic_properties[4]="chi";
//   basic_properties[5]="omega";
//   basic_properties[6]="Ei";
//   for(size_t i=0;i<basic_properties.size();i++){
//        TSM_ASSERT_EQUALS("The workspace property have to be specific",basic_properties[i],dim_names[i]);
//   }
//}
void testIdentifyMatrixAlg_1()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    
    std::vector<std::string> ws_dim_names(2);
    std::vector<std::string> dim_names,dim_units;
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


    TS_ASSERT_EQUALS("WS2DCnvNo",pAlg->identifyMatrixAlg(ws2D,"","",dim_names,dim_units));
    TS_ASSERT_EQUALS(ws_dim_names[0],dim_names[0]);
    TS_ASSERT_EQUALS(ws_dim_names[1],dim_names[1]);

}


void testIdentifyMatrixAlg_2()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    std::vector<std::string> dim_names,dim_units;
 
    API::NumericAxis *
    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units));

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Wavelength");
    ws2D->replaceAxis(0,pAx);
    // This is probably bug in conversion --> does not work in elastic mode
   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units));
    //TSM_ASSERT_THROWS("Can not convert wavelength to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Energy");
    ws2D->replaceAxis(0,pAx);
    // This is probably bug in conversion --> does not work in elastic mode
   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units));
   // TSM_ASSERT_THROWS("Can not convert Energy to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);


    pAx = new API::NumericAxis(3);
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units));


    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);
    TS_ASSERT_EQUALS("WS2D|Q|ElasticCnvFromTOF",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units));

    TSM_ASSERT_EQUALS("One dim name came from Q (this can be logically wrong)",1,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");

}

void testIdentifyMatrixAlg_3()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    std::vector<std::string> dim_names,dim_units;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2D|Q|DirectCnvNo",pAlg->identifyMatrixAlg(ws2D,"|Q|","Direct",dim_names,dim_units));
    TSM_ASSERT_EQUALS("One dimension comes from Q",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
}

void testIdentifyMatrixAlg_4()
{  
     std::vector<std::string> dim_names,dim_units;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2D|Q|IndirectCnvNo",pAlg->identifyMatrixAlg(ws2D,"|Q|","Indirect",dim_names,dim_units));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
}
void testIdentifyMatrixAlg_5()
{  
    std::vector<std::string> dim_names,dim_units;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2DQxQyQzIndirectCnvNo",pAlg->identifyMatrixAlg(ws2D,"QxQyQz","Indirect",dim_names,dim_units));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",4,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"Q_x");
    TS_ASSERT_EQUALS(dim_names[1],"Q_y");
    TS_ASSERT_EQUALS(dim_names[2],"Q_z");
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


void testExecNoQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OutputWorkspace","WS3DNoQ");
    pAlg->setPropertyValue("UsePreprocessedDetectors","0");
    pAlg->setPropertyValue("QDimensions","");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", ""));
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    AnalysisDataService::Instance().remove("OutputWorkspace");

}

void testExecModQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
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
     
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "QxQyQz"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues","-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues"," 10, 10, 10, 20, 40, 20");


    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Shoud finish succesfully",pAlg->isExecuted());
    AnalysisDataService::Instance().remove("OutputWorkspace"); 
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

