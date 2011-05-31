#ifndef MANTID_CRYSTAL_LOADISAWUBTEST_H_
#define MANTID_CRYSTAL_LOADISAWUBTEST_H_

#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
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

//    Matrix<double> UB = latt.getUB();
//    TS_ASSERT_EQUALS( UB.numRows(), 3);
//    TS_ASSERT_EQUALS( UB.numCols(), 3);
//    TS_ASSERT_DELTA( UB[0][0],  0.0574, 1e-4);
//    TS_ASSERT_DELTA( UB[1][0], -0.0454, 1e-4);
//    TS_ASSERT_DELTA( UB[2][2],  0.1169, 1e-4);

    AnalysisDataService::Instance().remove("LoadIsawUBTest_ws");
  }

  /**
# Raw data
LoadEventNexus(Filename="/home/8oz/data/TOPAZ_3007_event.nxs",OutputWorkspace="TOPAZ_3007",FilterByTime_Stop="1500",SingleBankPixelsOnly="0",CompressTolerance="0.050000000000000003")

# The found peaks
LoadPeaksFile(Filename="/home/8oz/Code/Mantid/Test/AutoTestData/TOPAZ_3007.peaks",OutputWorkspace="TOPAZ_3007_peaks")

SortEvents(InputWorkspace="TOPAZ_3007")
LoadIsawUB(InputWorkspace="TOPAZ_3007",Filename="/home/8oz/Code/Mantid/Test/AutoTestData/TOPAZ_3007.mat")
PredictPeaks(InputWorkspace="TOPAZ_3007",HKLPeaksWorkspace="TOPAZ_3007_peaks",OutputWorkspace="peaks")
MaskPeaksWorkspace("TOPAZ_3007", "peaks")
   *
   */
  void xtest_integration()
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 20);
    AnalysisDataService::Instance().addOrReplace("TOPAZ_3007", ws);
    AlgorithmHelper::runAlgorithm("LoadInstrument", 4,
        "Workspace", "TOPAZ_3007",
        "Filename", "TOPAZ_Definition_2011-01-01.xml");

    // Match the goniometer angles
    WorkspaceCreationHelper::SetGoniometer(ws, 86.92, 135.00, -105.66);
    //WorkspaceCreationHelper::SetGoniometer(ws, 0, 0, 0);

    // Load the .mat file into it
    AlgorithmHelper::runAlgorithm("LoadIsawUB", 4,
        "Filename", "TOPAZ_3007.mat",
        "InputWorkspace", "TOPAZ_3007");

    // Get a reference list of HKLs
    AlgorithmHelper::runAlgorithm("LoadPeaksFile", 4,
        "Filename", "TOPAZ_3007.peaks",
        "OutputWorkspace", "TOPAZ_3007_peaks");

    // Load the .mat file into it
    AlgorithmHelper::runAlgorithm("PredictPeaks", 6,
        "HKLPeaksWorkspace", "TOPAZ_3007_peaks",
        "InputWorkspace", "TOPAZ_3007",
        "OutputWorkspace", "peaks_predicted" );

    PeaksWorkspace_sptr pw = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("peaks_predicted") );

    TS_ASSERT(pw);
    if (!pw) return;

    TS_ASSERT_EQUALS( pw->getNumberPeaks(), 43);


  }
};


#endif /* MANTID_CRYSTAL_LOADISAWUBTEST_H_ */

