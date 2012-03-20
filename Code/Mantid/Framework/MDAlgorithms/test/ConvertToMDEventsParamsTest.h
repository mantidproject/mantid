#ifndef MANTID_MD_CONVEVENTS_PARAMS_TEST_H_
#define MANTID_MD_CONVEVENTS_PARAMS_TEST_H_


#include "MantidAPI/TextAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"

#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertToMDEventsParamsTest : public CxxTest::TestSuite
{
public:
static ConvertToMDEventsParamsTest *createSuite() { return new ConvertToMDEventsParamsTest(); }
static void destroySuite(ConvertToMDEventsParamsTest * suite) { delete suite; }    
// TEST QMode
void testParseQMode_WrongThrows()
{
     std::vector<std::string> ws_dim_names;
     std::vector<std::string> ws_dim_units;
     std::vector<std::string> out_dim_names,out_dim_units;
     ConvertToMDEventsParams params;
     int nQ_dims;    
     TS_ASSERT_THROWS(params.parseQMode("WrongMode",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims,false),std::invalid_argument);

}
void testParseQMode_NoQ()
{
     std::vector<std::string> ws_dim_names(2,"A");
     std::vector<std::string> ws_dim_units(2,"UnA");
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;
     std::string MODE;
     ConvertToMDEventsParams params;
     TS_ASSERT_THROWS_NOTHING(MODE=params.parseQMode("CopyToMD",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims));
     TS_ASSERT_EQUALS(2,nQ_dims);
     TS_ASSERT_EQUALS("CopyToMD",MODE);
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

     ConvertToMDEventsParams params;
     TS_ASSERT_THROWS_NOTHING(MODE=params.parseQMode("|Q|",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims));
     TS_ASSERT_EQUALS(1,nQ_dims);
     TS_ASSERT_EQUALS("|Q|Cryst",MODE);
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
     ConvertToMDEventsParams params;
     TS_ASSERT_THROWS_NOTHING(MODE=params.parseQMode("Q3D",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims,false));
     TS_ASSERT_EQUALS(3,nQ_dims);
     TS_ASSERT_EQUALS("Q3DCryst",MODE);
     TS_ASSERT_EQUALS("Q1",out_dim_names[0]);
     TS_ASSERT_EQUALS("Q2",out_dim_names[1]);
     TS_ASSERT_EQUALS("Q3",out_dim_names[2]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[0]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[1]);
     TS_ASSERT_EQUALS("Momentum",out_dim_units[2]);
}
void testParseQMode_Q3DPowd()
{
     std::vector<std::string> ws_dim_names(2,"A");
     std::vector<std::string> ws_dim_units(2,"UnA");
     std::vector<std::string> out_dim_names,out_dim_units;
     int nQ_dims;
     std::string MODE;
     ConvertToMDEventsParams params;
     TS_ASSERT_THROWS_NOTHING(MODE=params.parseQMode("Q3D",ws_dim_names,ws_dim_units,out_dim_names,out_dim_units, nQ_dims,true));
     TS_ASSERT_EQUALS(3,nQ_dims);
     TS_ASSERT_EQUALS("Q3DPowd",MODE);
     TS_ASSERT_EQUALS("Q1",out_dim_names[0]);
     TS_ASSERT_EQUALS("Q2",out_dim_names[1]);
     TS_ASSERT_EQUALS("Q3",out_dim_names[2]);
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS(params.parseDEMode("SOMEQMODE","WrongMode",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units),std::invalid_argument);
}
void testParseDEMode_NoQ()
{
     std::vector<std::string> ws_dim_units(1,"some");
     std::vector<std::string> out_dim_names,out_dim_units;
     int ndE_dims;    
     std::string natural_units;
     std::string EID;
    ConvertToMDEventsParams params;

     TS_ASSERT_THROWS_NOTHING(EID=params.parseDEMode("CopyToMD","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
     TS_ASSERT_EQUALS(0,ndE_dims);

     TSM_ASSERT_EQUALS("Regardless of the dE mode, if Q-mode is NoQ, should return Any_Mode (NoDE): ","NoDE",EID);
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS_NOTHING(EID=params.parseDEMode("DoesNotMatter","Direct",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS_NOTHING(EID=params.parseDEMode("DoesNotMatter","Indirect",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS_NOTHING(EID=params.parseDEMode("DoesNotMatter","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS_NOTHING(EID=params.parseDEMode("|Q|","Elastic",ws_dim_units,out_dim_names,out_dim_units,ndE_dims,natural_units));
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
     ConvertToMDEventsParams params;

     TS_ASSERT_THROWS(params.parseConvMode("AnyConversionMode",natural_units,ws_dim_units),std::invalid_argument);
}

void testParseConv_ElasticViaTOFNotThrowsAnyMore()
{
     std::vector<std::string> ws_dim_units(1,"DeltaE");
     std::string natural_units;
     std::string CONV_ID;
     ConvertToMDEventsParams params;

   // this is actually incorrect as wavelength can not be converted to/from DeltaE
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("Elastic","Wavelength",ws_dim_units));
     TS_ASSERT_EQUALS("CnvByTOF",CONV_ID);
}

void testParseConv_NoQ()
{
     std::vector<std::string> ws_dim_units(1,"Any");
     std::string CONV_ID;
     ConvertToMDEventsParams params;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("CopyToMD","AnyUnits",ws_dim_units));
     TS_ASSERT_EQUALS("CnvNo",CONV_ID);
}
void testParseConv_NaturalNoQ()
{
     std::vector<std::string> ws_dim_units(1,"dSpacing");
     std::string CONV_ID;
     ConvertToMDEventsParams params;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("CopyToMD","dSpacing",ws_dim_units));
     TS_ASSERT_EQUALS("CnvNo",CONV_ID);
}

void testParseConv_QuickConvertsion()
{
     std::vector<std::string> ws_dim_units(1,"dSpacing");
     std::string CONV_ID;
     ConvertToMDEventsParams params;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("AnyMode","MomentumTransfer",ws_dim_units));
     TS_ASSERT_EQUALS("CnvFast",CONV_ID);
}
void testParseConv_FromTOF()
{
     std::vector<std::string> ws_dim_units(1,"TOF");
     std::string CONV_ID;
     ConvertToMDEventsParams params;
    
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("AnyMode","MomentumTransfer",ws_dim_units));
     TS_ASSERT_EQUALS("CnvFromTOF",CONV_ID);
}
void testParseConv_ByTOF()
{
     std::vector<std::string> ws_dim_units(1,"DeltaE");
     std::string CONV_ID;
     ConvertToMDEventsParams params;
      
     TS_ASSERT_THROWS_NOTHING(CONV_ID=params.parseConvMode("AnyMode","Wavelength",ws_dim_units));
     TS_ASSERT_EQUALS("CnvByTOF",CONV_ID);
}


// --> GET DIMENSIONS FROM WS MATRIX:
void testNeedsNumericAxis()
{
    MDEvents::MDWSDescription TWS;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    ws2D->replaceAxis(0,new API::TextAxis(3));
    std::vector<std::string> dim_ID;
    std::vector<std::string> dim_units;
    ConvertToMDEventsParams params;

    TS_ASSERT_THROWS(params.identifyMatrixAlg(ws2D,"Q3D","CopyToMD",dim_ID,dim_units,TWS),std::invalid_argument);
}
void testGetWS4DimIDFine()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    MDEvents::MDWSDescription TWS;

    std::vector<std::string> dim_ID;
    std::vector<std::string> dim_units;
    std::string Alg_ID;
    ConvertToMDEventsParams params;

    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"Q3D","Direct",dim_ID,dim_units,TWS));

    TSM_ASSERT_EQUALS("Inelastic workspace will produce 4 dimensions",4,dim_ID.size());
    TSM_ASSERT_EQUALS("Last dimension of Inelastic transformation should be DeltaE","DeltaE",dim_units[3]);
    TSM_ASSERT_EQUALS("Alg ID would be: ","WS2DHistoQ3DCrystDirectCnvNo",Alg_ID);
    TS_ASSERT(!TWS.detInfoLost);
}
void testGetWS3DimIDFine()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title() = "Dim1";
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);
    MDEvents::MDWSDescription TWS;

    std::vector<std::string> dim_ID;
    std::vector<std::string> dim_units;
    std::string Alg_ID; 
    ConvertToMDEventsParams params;

    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"Q3D","Elastic",dim_ID,dim_units, TWS));

    TSM_ASSERT_EQUALS("Inelastic workspace will produce 3 dimensions",3,dim_ID.size());
    TSM_ASSERT_EQUALS("Last dimension of Elastic transformation should be ","Momentum",dim_units[2]);
    TSM_ASSERT_EQUALS("Alg ID would be: ","WS2DHistoQ3DCrystElasticCnvByTOF",Alg_ID);
    TS_ASSERT(!TWS.detInfoLost);
}
void testGetWSDimNames2AxisNoQ()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    MDEvents::MDWSDescription TWS;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title() = "Dim1";
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);

    pAx = new API::NumericAxis(3);
    pAx->title() = "Dim2";
    pAx->setUnit("QSquared");
    ws2D->replaceAxis(1,pAx);

    std::vector<std::string> dim_ID;
    std::vector<std::string> dim_units;
    std::string AlgID;
    ConvertToMDEventsParams params;

    TS_ASSERT_THROWS_NOTHING(AlgID=params.identifyMatrixAlg(ws2D,"CopyToMD","NoDE",dim_ID,dim_units,TWS));

    TSM_ASSERT("Det info should be undefined an an numeric axis is along axis 2",TWS.detInfoLost);

    TS_ASSERT_EQUALS(2,dim_ID.size());
    TS_ASSERT_EQUALS("Dim1",dim_ID[0]);
    TS_ASSERT_EQUALS("Dim2",dim_ID[1]);

    TS_ASSERT_EQUALS(2,dim_units.size());
    TS_ASSERT_EQUALS("dSpacing",dim_units[0]);
    TS_ASSERT_EQUALS("QSquared",dim_units[1]);
}
///------------------------------------------------------------------

void testIdentifyMatrixAlg_1()
{  
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    MDEvents::MDWSDescription TWS;
    
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
    ConvertToMDEventsParams params;

    TS_ASSERT_EQUALS("WS2DHistoCopyToMDNoDECnvNo",params.identifyMatrixAlg(ws2D,"CopyToMD","NoDE",dim_names,dim_units,TWS));
    TS_ASSERT_EQUALS(ws_dim_names[0],dim_names[0]);
    TS_ASSERT_EQUALS(ws_dim_names[1],dim_names[1]);
    TSM_ASSERT("Det info should be undefined an an numeric axis is along axis 2",TWS.detInfoLost);

}


void testIdentifyMatrixAlg_2()
{  
    ConvertToMDEventsParams params;

    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    std::vector<std::string> dim_names,dim_units;
    MDEvents::MDWSDescription TWS;
 
    API::NumericAxis *
    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);

    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units,TWS));
    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Wavelength");
    ws2D->replaceAxis(0,pAx);
    // This is probably bug in conversion --> does not work in elastic mode
   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units,TWS));
    //TSM_ASSERT_THROWS("Can not convert wavelength to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);
   TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);

    pAx = new API::NumericAxis(3);
    pAx->setUnit("Energy");
    ws2D->replaceAxis(0,pAx);
    // This is probably bug in conversion --> does not work in elastic mode
   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units,TWS));
   // TSM_ASSERT_THROWS("Can not convert Energy to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);
   TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);

    pAx = new API::NumericAxis(3);
    pAx->setUnit("dSpacing");
    ws2D->replaceAxis(0,pAx);
    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units,TWS));
    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);

    pAx = new API::NumericAxis(3);
    pAx->setUnit("TOF");
    ws2D->replaceAxis(0,pAx);
    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdElasticCnvFromTOF",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units,TWS));

    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be logically wrong)",1,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");

}

void testIdentifyMatrixAlg_3()
{  
    ConvertToMDEventsParams params;

    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    std::vector<std::string> dim_names,dim_units;
    MDEvents::MDWSDescription TWS;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdDirectCnvNo",params.identifyMatrixAlg(ws2D,"|Q|","Direct",dim_names,dim_units,TWS));
    TSM_ASSERT_EQUALS("One dimension comes from Q",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
     TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
}

void testIdentifyMatrixAlg_4()
{  
    ConvertToMDEventsParams params;

    std::vector<std::string> dim_names,dim_units;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    MDEvents::MDWSDescription TWS;


    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdIndirectCnvNo",params.identifyMatrixAlg(ws2D,"|Q|","Indirect",dim_names,dim_units,TWS));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",2,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"|Q|");
    TS_ASSERT_EQUALS(dim_names[1],"DeltaE");
    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
}
void testIdentifyMatrixAlg_5()
{  
    ConvertToMDEventsParams params;

    std::vector<std::string> dim_names,dim_units;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    MDEvents::MDWSDescription TWS;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2DHistoQ3DPowdIndirectCnvNo",params.identifyMatrixAlg(ws2D,"Q3D","Indirect",dim_names,dim_units,TWS));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",4,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"Q1");
    TS_ASSERT_EQUALS(dim_names[1],"Q2");
    TS_ASSERT_EQUALS(dim_names[2],"Q3");
    TS_ASSERT_EQUALS(dim_names[3],"DeltaE");
    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
}
void testIdentifyMatrixAlg_LatticeSet()
{  
    ConvertToMDEventsParams params;

    std::vector<std::string> dim_names,dim_units;
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
    ws2D->mutableSample().setOrientedLattice(new Mantid::Geometry::OrientedLattice());

    MDEvents::MDWSDescription TWS;

    API::NumericAxis *pAx = new API::NumericAxis(3);
    pAx->title()="A";
    pAx->setUnit("DeltaE");
    ws2D->replaceAxis(0,pAx);

    TS_ASSERT_EQUALS("WS2DHistoQ3DCrystIndirectCnvNo",params.identifyMatrixAlg(ws2D,"Q3D","Indirect",dim_names,dim_units,TWS));
    TSM_ASSERT_EQUALS("One dim name came from Q (this can be wrong)",4,dim_names.size());
    TS_ASSERT_EQUALS(dim_names[0],"Q1");
    TS_ASSERT_EQUALS(dim_names[1],"Q2");
    TS_ASSERT_EQUALS(dim_names[2],"Q3");
    TS_ASSERT_EQUALS(dim_names[3],"DeltaE");
    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
}


};

//
#endif
