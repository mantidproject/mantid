#ifndef MANTID_CRYSTAL_SETGONIOMETERTEST_H_
#define MANTID_CRYSTAL_SETGONIOMETERTEST_H_

#include "MantidCrystal/SetGoniometer.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidGeometry/V3D.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::Goniometer;

class SetGoniometerTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis0", "angle1, 1.0,2.0,3.0, 1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis1", "angle2  , 4.0, 5.0,6.0, -1") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Check the results
    Goniometer & gon = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS( gon.getNumberAxes(), 2);

    TS_ASSERT_EQUALS( gon.getAxis(0).name, "angle1");
    TS_ASSERT_EQUALS( gon.getAxis(0).rotationaxis, V3D(1.0,2.0,3.0));
    TS_ASSERT_EQUALS( gon.getAxis(0).sense, 1);

    TS_ASSERT_EQUALS( gon.getAxis(1).name, "angle2");
    TS_ASSERT_EQUALS( gon.getAxis(1).rotationaxis, V3D(4.0,5.0,6.0));
    TS_ASSERT_EQUALS( gon.getAxis(1).sense, -1);

    AnalysisDataService::Instance().remove("SetGoniometerTest_ws");
  }
  

  /** Do a test with a single param
   *
   * @param axis0 :: string to pass
   * @param numExpected :: how many axes should be created (0 or 1)
   */
  void do_test_param(std::string axis0, size_t numExpected=0)
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);
    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis0", axis0) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    if (numExpected == 0)
    {
      TS_ASSERT( !alg.isExecuted() );
    }
    else
    {
      // Check the results
      Goniometer & gon = ws->mutableRun().getGoniometer();
      TS_ASSERT_EQUALS( gon.getNumberAxes(), numExpected);
    }
    AnalysisDataService::Instance().remove("SetGoniometerTest_ws");
  }

  void test_param_NotEnough()
  { do_test_param("name, 1.0, 2.0"); }

  void test_param_TooMany()
  { do_test_param("name, 1.0, 2.0, 3.0, 1, 12345"); }

  void test_param_WrongCCW()
  { do_test_param("name, 1.0, 2.0, 3.0, 0");
    do_test_param("name, 1.0, 2.0, 3.0, 2"); }

  void test_param_NotANumber()
  { do_test_param("name, One, Two, Three, 1"); }

  void test_param_EmptyName()
  { do_test_param(", 1.0, 2.0, 3.0, 1"); }

  void test_param_ZeroVector()
  { do_test_param("name, 0.0, 0.0, 0.0, 1"); }

  void test_param_Empty()
  { do_test_param(""); }


};


#endif /* MANTID_CRYSTAL_SETGONIOMETERTEST_H_ */

