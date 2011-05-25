#ifndef MANTID_CRYSTAL_LOADISAWUBTEST_H_
#define MANTID_CRYSTAL_LOADISAWUBTEST_H_

#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadIsawUBTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("LoadIsawUBTest_ws", ws);
  
    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "TOPAZ_3007.mat") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    TS_ASSERT(ws);
    if (!ws) return;
    
    // Check the results
    OrientedLattice latt;
    TS_ASSERT_THROWS_NOTHING( latt = ws->mutableSample().getOrientedLattice() );
    TS_ASSERT_DELTA( latt.a(), 14.1526, 1e-4 );
    TS_ASSERT_DELTA( latt.b(), 19.2903, 1e-4 );
    TS_ASSERT_DELTA( latt.c(), 8.5813 , 1e-4 );
    TS_ASSERT_DELTA( latt.alpha(), 90.0000, 1e-4 );
    TS_ASSERT_DELTA( latt.beta(), 105.0738, 1e-4 );
    TS_ASSERT_DELTA( latt.gamma(), 90.0000, 1e-4 );

    Matrix<double> UB = latt.getUB();
    TS_ASSERT_EQUALS( UB.numRows(), 3);
    TS_ASSERT_EQUALS( UB.numCols(), 3);
    TS_ASSERT_DELTA( UB[0][0],  0.0574, 1e-4);
    TS_ASSERT_DELTA( UB[1][0], -0.0454, 1e-4);
    TS_ASSERT_DELTA( UB[2][2],  0.1169, 1e-4);

    AnalysisDataService::Instance().remove("LoadIsawUBTest_ws");
  }


};


#endif /* MANTID_CRYSTAL_LOADISAWUBTEST_H_ */

