#ifndef MANTID_CONV2MD_PARAMS_TEST_H_
#define MANTID_CONV2MD_PARAMS_TEST_H_


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

class ConvertToMDParamsTest : public CxxTest::TestSuite
{
public:
static ConvertToMDParamsTest *createSuite() { return new ConvertToMDParamsTest(); }
static void destroySuite(ConvertToMDParamsTest * suite) { delete suite; }    

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



};

//
#endif
