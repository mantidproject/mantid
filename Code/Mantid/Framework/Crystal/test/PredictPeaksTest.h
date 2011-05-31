#ifndef MANTID_CRYSTAL_PREDICTPEAKSTEST_H_
#define MANTID_CRYSTAL_PREDICTPEAKSTEST_H_

#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IDTypes.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class PredictPeaksTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  /** Make a HKL peaks workspace */
  PeaksWorkspace_sptr getHKLpw(IInstrument_sptr inst, std::vector<V3D> hkls, detid_t detid)
  {
    PeaksWorkspace_sptr hklPW;
    if (hkls.size() > 0)
    {
      hklPW = PeaksWorkspace_sptr(new PeaksWorkspace());
      for (size_t i=0; i<hkls.size(); i++)
      {
        Peak p(inst, detid, 1.0);
        p.setHKL( hkls[i] );
        hklPW->addPeak( p );
      }
    }
    return hklPW;
  }


  void do_test_exec(std::string reflectionCondition, size_t expectedNumber, std::vector<V3D> hkls)
  {
    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace(10000, 1);
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    inWS->setInstrument(inst);

    //Set ub and Goniometer rotation
    WorkspaceCreationHelper::SetOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::SetGoniometer(inWS, 0., 0., 0.);

    PeaksWorkspace_sptr hklPW = getHKLpw(inst, hkls, 10000);
  
    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(inWS) ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WavelengthMin", "0.1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WavelengthMax", "10.0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MinDSpacing", "1.0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ReflectionCondition", reflectionCondition) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("HKLPeaksWorkspace", hklPW) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), expectedNumber);
    //std::cout << ws->getPeak(0).getHKL() << " hkl\n";
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_exec()
  {
    do_test_exec("Primitive", 10, std::vector<V3D>() );
  }

  /** Fewer HKLs if they are not allowed */
  void test_exec_withReflectionCondition()
  {
    do_test_exec("C-face centred", 6, std::vector<V3D>() );
  }

  void test_exec_withInputHKLList()
  {
    std::vector<V3D> hkls;
    hkls.push_back(V3D(6,9,-1));
    hkls.push_back(V3D(7,7,-1));
    do_test_exec("Primitive", 2, hkls);
  }

  /** More manual test of predict peaks where we build a simple UB
   * and see that the peak falls where it should.
   * In this case, hkl 1,0,0 on a crystal rotated 45 deg. relative to +Y
   * should fall on a detector towards (+1.0, 0.0, 0.0)
   */
  void test_manual()
  {
    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::Create2DWorkspace(10000, 1);
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    inWS->setInstrument(inst);

    //Set ub and Goniometer rotation
    WorkspaceCreationHelper::SetOrientedLattice(inWS, 10.0, 10.0, 10.0);
    // Rotate the crystal 45 degrees so that hkl 1,0,0 goes to +X
    WorkspaceCreationHelper::SetGoniometer(inWS, 45., 0., 0.);

    MantidMat ub = inWS->sample().getOrientedLattice().getUB();

    std::vector<V3D> hkls;
    hkls.push_back(V3D(1,0,0));
    PeaksWorkspace_sptr hklPW = getHKLpw(inst, hkls, 0);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(inWS) ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("HKLPeaksWorkspace", hklPW) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 1);
    // Center of the panel
    TS_ASSERT_EQUALS( ws->getPeak(0).getDetectorID(), 5050);
    // Expected wavelength
    TS_ASSERT_DELTA( ws->getPeak(0).getWavelength(), 14.14, 0.01);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }


};


#endif /* MANTID_CRYSTAL_PREDICTPEAKSTEST_H_ */

