#ifndef MANTID_MD_CONVEVENTS_PARAMS_TEST_H_
#define MANTID_MD_CONVEVENTS_PARAMS_TEST_H_


#include "MantidAPI/TextAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDEvents/MDWSDescription.h"

#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace Mantid::MDEvents;

class ConvertToMDEventsParamsTest : public CxxTest::TestSuite
{
public:
static ConvertToMDEventsParamsTest *createSuite() { return new ConvertToMDEventsParamsTest(); }
static void destroySuite(ConvertToMDEventsParamsTest * suite) { delete suite; }    

// --> GET DIMENSIONS FROM WS MATRIX:
//--> this test  should go on unit conversion
//void testNeedsNumericAxis()
//{
//    MDEvents::MDWSDescription TWS;
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    ws2D->replaceAxis(0,new API::TextAxis(3));   
//    std::vector<std::string> add_dim_names;  
//    std::vector<double> min(3,-10),max(3,10);
//    TWS.setMinMax(min,max);
//
//    TS_ASSERT_THROWS(TWS.buildFromMatrixWS(ws2D,"Q3D","Elastic",add_dim_names),std::invalid_argument);
//}
void testGetWS4DimIDFine()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    ws2D->mutableRun().addProperty("Ei",12.,"meV",true);

    MDEvents::MDWSDescription TWS;
    std::vector<double> min(4,-10),max(4,10);
    TWS.setMinMax(min,max);

    std::vector<std::string> other_dim_names;
 

    TS_ASSERT_THROWS_NOTHING(TWS.buildFromMatrixWS(ws2D,"Q3D","Direct",other_dim_names));

    TSM_ASSERT_EQUALS("Inelastic workspace will produce 4 dimensions",4,TWS.nDimensions());
    std::vector<std::string> dim_units = TWS.getDimUnits();
    TSM_ASSERT_EQUALS("Last dimension of Inelastic transformation should be DeltaE","DeltaE",dim_units[3]);
    TSM_ASSERT_EQUALS("Alg ID would be: ","Q3D",TWS.AlgID);
    TSM_ASSERT("detector infromation should be present in the workspace ",!TWS.isDetInfoLost());

    TS_ASSERT_THROWS_NOTHING(TWS.buildFromMatrixWS(ws2D,TWS.AlgID,"Indirect",other_dim_names));



    //std::vector<std::string> dimID= TWS.getDefaultDimIDQ3D(1);
    //for(size_t i=0;i<4;i++)
    //{
    //    TS_ASSERT_EQUALS(dimID[i],TWS.dimIDs[i]);
    //    TS_ASSERT_EQUALS(dimID[i],TWS.dimNames[i]);
    //}


}

//void testGetWSDimNames2AxisNoQ()
//{
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    MDEvents::MDWSDescription TWS;
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title() = "Dim1";
//    pAx->setUnit("dSpacing");
//    ws2D->replaceAxis(0,pAx);
//
//    pAx = new API::NumericAxis(3);
//    pAx->title() = "Dim2";
//    pAx->setUnit("QSquared");
//    ws2D->replaceAxis(1,pAx);
//
//    std::vector<std::string> dim_units;
//    std::string AlgID;
//    ConvertToMDEventsParams params;
//
//    TS_ASSERT_THROWS_NOTHING(AlgID=params.identifyMatrixAlg(ws2D,"CopyToMD","NoDE",dim_units,TWS));
//
//    TS_ASSERT_EQUALS(2,dim_units.size());
//    TS_ASSERT_EQUALS("dSpacing",dim_units[0]);
//    TS_ASSERT_EQUALS("QSquared",dim_units[1]);
//
//    TSM_ASSERT("Det info should be undefined an an numeric axis is along axis 2",TWS.detInfoLost);
//
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,AlgID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("This NoQ workspace will produce 2 dimensions",2,TWS.nDims);
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    TS_ASSERT_EQUALS("Dim1",TWS.dimIDs[0]);
//    TS_ASSERT_EQUALS("Dim2",TWS.dimIDs[1]);
//    TS_ASSERT_EQUALS("Dim1",TWS.dimNames[0]);
//    TS_ASSERT_EQUALS("Dim2",TWS.dimNames[1]);
//
//}
/////------------------------------------------------------------------
//
//void testIdentifyMatrixAlg_1()
//{  
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    MDEvents::MDWSDescription TWS;
//    
//    std::vector<std::string> ws_dim_names(2);
//    std::vector<std::string> dim_names,dim_units;
//    ws_dim_names[0]="A";
//    ws_dim_names[1]="B";
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title() = ws_dim_names[0];
//    pAx->setUnit("dSpacing");
//    ws2D->replaceAxis(0,pAx);
//    pAx = new API::NumericAxis(3);
//    pAx->title() = ws_dim_names[1];
//    pAx->setUnit("QSquared");
//    ws2D->replaceAxis(1,pAx);
//    ConvertToMDEventsParams params;
//    std::string Alg_ID;
//
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"CopyToMD","NoDE",dim_units,TWS));
//    TS_ASSERT_EQUALS("WS2DHistoCopyToMDNoDECnvNo",Alg_ID);
//    TSM_ASSERT("Det info should be undefined an an numeric axis is along axis 2",TWS.detInfoLost);
//
//    //
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("This NoQ workspace should produce 2 dimensions",2,TWS.nDims);
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//
//
//}
//
//
//void testIdentifyMatrixAlg_2()
//{  
//    ConvertToMDEventsParams params;
//
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    std::vector<std::string> dim_names,dim_units;
//    MDEvents::MDWSDescription TWS;
// 
//    API::NumericAxis *
//    pAx = new API::NumericAxis(3);
//    pAx->setUnit("TOF");
//    ws2D->replaceAxis(0,pAx);
//
//    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_units,TWS));
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    pAx = new API::NumericAxis(3);
//    pAx->setUnit("Wavelength");
//    ws2D->replaceAxis(0,pAx);
//    // This is probably bug in conversion --> does not work in elastic mode
//   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_units,TWS));
//    //TSM_ASSERT_THROWS("Can not convert wavelength to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);
//   TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    pAx = new API::NumericAxis(3);
//    pAx->setUnit("Energy");
//    ws2D->replaceAxis(0,pAx);
//    // This is probably bug in conversion --> does not work in elastic mode
//   TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_units,TWS));
//   // TSM_ASSERT_THROWS("Can not convert Energy to momentum transfer in elastic mode ",pAlg->identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_names,dim_units),std::invalid_argument);
//   TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    pAx = new API::NumericAxis(3);
//    pAx->setUnit("dSpacing");
//    ws2D->replaceAxis(0,pAx);
//    TSM_ASSERT_THROWS_NOTHING("Elastic conversion needs X-axis to be in an Energy-related units",params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_units,TWS));
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    pAx = new API::NumericAxis(3);
//    pAx->setUnit("TOF");
//    ws2D->replaceAxis(0,pAx);
//
//    std::string Alg_ID;
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"|Q|","Elastic",dim_units,TWS));
//    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdElasticCnvFromTOF",Alg_ID);
//
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//
//    //
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("This ModQ workspace should produce 1 dimensions",1,TWS.nDims);
//
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    std::vector<std::string> ws_dim_names = TWS.getDefaultDimIDModQ(0);
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//}
//
//void testIdentifyMatrixAlg_3()
//{  
//    ConvertToMDEventsParams params;
//
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    std::vector<std::string> dim_names,dim_units;
//    MDEvents::MDWSDescription TWS;
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title()="A";
//    pAx->setUnit("DeltaE");
//    ws2D->replaceAxis(0,pAx);
//
//    std::string Alg_ID;
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"|Q|","Direct",dim_units,TWS));
//
//
//    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdDirectCnvNo",Alg_ID);
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    // Dim ID &Names
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("Inelastic ModQ workspace should produce 2 dimensions",2,TWS.nDims);
//
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    std::vector<std::string> ws_dim_names = TWS.getDefaultDimIDModQ(2);
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//}
//
//void testIdentifyMatrixAlg_4()
//{  
//    ConvertToMDEventsParams params;
//
//    std::vector<std::string> dim_names,dim_units;
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    MDEvents::MDWSDescription TWS;
//
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title()="A";
//    pAx->setUnit("DeltaE");
//    ws2D->replaceAxis(0,pAx);
//
//    std::string Alg_ID;
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"|Q|","Indirect",dim_units,TWS));
//
//
//    TS_ASSERT_EQUALS("WS2DHisto|Q|PowdIndirectCnvNo",Alg_ID);
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//    // Dim ID &Names
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("Inelastic ModQ workspace should produce 2 dimensions",2,TWS.nDims);
//
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    std::vector<std::string> ws_dim_names = TWS.getDefaultDimIDModQ(2);
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//}
//void testIdentifyMatrixAlg_5()
//{  
//    ConvertToMDEventsParams params;
//
//    std::vector<std::string> dim_names,dim_units;
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    MDEvents::MDWSDescription TWS;
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title()="A";
//    pAx->setUnit("DeltaE");
//    ws2D->replaceAxis(0,pAx);
//
//    std::string Alg_ID;
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"Q3D","Indirect",dim_units,TWS));
//
//    TS_ASSERT_EQUALS("WS2DHistoQ3DPowdIndirectCnvNo",Alg_ID);
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//
//    // Dim ID &Names
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("Inelastic Q3D workspace should produce 4 dimensions",4,TWS.nDims);
//
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    std::vector<std::string> ws_dim_names = TWS.getDefaultDimIDQ3D(2);
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//}
//void testIdentifyMatrixAlg_LatticeSet()
//{  
//    ConvertToMDEventsParams params;
//
//    std::vector<std::string> dim_names,dim_units;
//    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::Create2DWorkspace(4,10);
//    ws2D->mutableSample().setOrientedLattice(new Mantid::Geometry::OrientedLattice());
//
//    MDEvents::MDWSDescription TWS;
//
//    API::NumericAxis *pAx = new API::NumericAxis(3);
//    pAx->title()="A";
//    pAx->setUnit("DeltaE");
//    ws2D->replaceAxis(0,pAx);
//
//    std::string Alg_ID;
//    TS_ASSERT_THROWS_NOTHING(Alg_ID=params.identifyMatrixAlg(ws2D,"Q3D","Indirect",dim_units,TWS));
//
//    TS_ASSERT_EQUALS("WS2DHistoQ3DCrystIndirectCnvNo",Alg_ID);
//    TSM_ASSERT("Det info should be defined for conversion",!TWS.detInfoLost);
//
//  // Dim ID &Names
//    std::vector<std::string> other_dim_names;
//    TS_ASSERT_THROWS_NOTHING(params.buildMDDimDescription(ws2D,Alg_ID,other_dim_names,TWS));
//    TSM_ASSERT_EQUALS("Inelastic Q3D workspace should produce 4 dimensions",4,TWS.nDims);
//
//    TS_ASSERT_EQUALS(TWS.dimIDs.size(),TWS.dimNames.size());
//
//    std::vector<std::string> ws_dim_names = TWS.getDefaultDimIDQ3D(1);
//    for(size_t i=0;i<TWS.dimNames.size();i++)
//    {
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimIDs[i]);
//        TS_ASSERT_EQUALS(ws_dim_names[i],TWS.dimNames[i]);
//    }
//
//}


};

//
#endif
