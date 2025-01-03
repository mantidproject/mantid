// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/SetGoniometer.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::Goniometer;
using Mantid::Kernel::V3D;

class SetGoniometerTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_fail() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", "angle1, 1.0,2.0,3.0, 1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis1", "angle2  , 4.0, 5.0,6.0, -1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // catch for no log values
  }

  /** Create an "empty" goniometer by NOT giving any axes. */
  void test_exec_emptyGoniometer() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", ""));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // no log values

    // Check the results
    const Goniometer &gon = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS(gon.getNumberAxes(), 0);
    DblMatrix rot = ws->mutableRun().getGoniometerMatrix();
    TSM_ASSERT_EQUALS("Goniometer Rotation matrix is 3x3 identity", rot, DblMatrix(3, 3, true));
  }

  void test_exec() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);
    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetGoniometerTest_ws", "LogName", "angle1",
                                      "LogType", "Number Series", "LogText", "1.234");

    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetGoniometerTest_ws", "LogName", "angle2",
                                      "LogType", "Number Series", "LogText", "1.234");

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", "angle1, 1.0,2.0,3.0, 1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis1", "angle2  , 4.0, 5.0,6.0, -1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis2", "45, 1.0, 0.0,0.0, 1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // no log values

    // Check the results
    const Goniometer &gon = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS(gon.getNumberAxes(), 3);

    TS_ASSERT_EQUALS(gon.getAxis(0).name, "angle1");
    TS_ASSERT_EQUALS(gon.getAxis(0).rotationaxis, V3D(1.0, 2.0, 3.0));
    TS_ASSERT_EQUALS(gon.getAxis(0).sense, 1);

    TS_ASSERT_EQUALS(gon.getAxis(1).name, "angle2");
    TS_ASSERT_EQUALS(gon.getAxis(1).rotationaxis, V3D(4.0, 5.0, 6.0));
    TS_ASSERT_EQUALS(gon.getAxis(1).sense, -1);

    TS_ASSERT_EQUALS(gon.getAxis(2).name, "GoniometerAxis2_FixedValue");
    TS_ASSERT_EQUALS(gon.getAxis(2).rotationaxis, V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(gon.getAxis(2).sense, 1);
    TS_ASSERT_EQUALS(gon.getAxis(2).angle, 45);
    AnalysisDataService::Instance().remove("SetGoniometerTest_ws");
  }

  void test_multiple_goniometers() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetMutipleGoniometerTest_ws", ws);
    FrameworkManager::Instance().exec("AddTimeSeriesLog", 8, "Workspace", "SetMutipleGoniometerTest_ws", "Name",
                                      "angle1", "Time", "2010-01-01T00:00:00", "Value", "0.0");
    FrameworkManager::Instance().exec("AddTimeSeriesLog", 8, "Workspace", "SetMutipleGoniometerTest_ws", "Name",
                                      "angle1", "Time", "2010-01-01T00:01:00", "Value", "90.0");

    FrameworkManager::Instance().exec("AddTimeSeriesLog", 8, "Workspace", "SetMutipleGoniometerTest_ws", "Name",
                                      "angle2", "Time", "2010-01-01T00:00:00", "Value", "90.0");

    FrameworkManager::Instance().exec("AddTimeSeriesLog", 8, "Workspace", "SetMutipleGoniometerTest_ws", "Name",
                                      "angle2", "Time", "2010-01-01T00:01:00", "Value", "0.0");

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetMutipleGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", "angle1, 1,0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis1", "angle2, 0,1,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Average", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // no log values

    // Check the results
    TS_ASSERT_EQUALS(ws->run().getNumGoniometers(), 2);

    const Goniometer &gon = ws->mutableRun().getGoniometer(0);
    TS_ASSERT_EQUALS(gon.getNumberAxes(), 2);

    TS_ASSERT_EQUALS(gon.getAxis(0).name, "angle1");
    TS_ASSERT_EQUALS(gon.getAxis(0).rotationaxis, V3D(1, 0, 0));
    TS_ASSERT_EQUALS(gon.getAxis(0).sense, 1);
    TS_ASSERT_EQUALS(gon.getAxis(0).angle, 0.0);

    TS_ASSERT_EQUALS(gon.getAxis(1).name, "angle2");
    TS_ASSERT_EQUALS(gon.getAxis(1).rotationaxis, V3D(0, 1, 0));
    TS_ASSERT_EQUALS(gon.getAxis(1).sense, 1);
    TS_ASSERT_EQUALS(gon.getAxis(1).angle, 90.0);

    const Goniometer &gon2 = ws->mutableRun().getGoniometer(1);
    TS_ASSERT_EQUALS(gon2.getNumberAxes(), 2);

    TS_ASSERT_EQUALS(gon2.getAxis(0).name, "angle1");
    TS_ASSERT_EQUALS(gon2.getAxis(0).rotationaxis, V3D(1, 0, 0));
    TS_ASSERT_EQUALS(gon2.getAxis(0).sense, 1);
    TS_ASSERT_EQUALS(gon2.getAxis(0).angle, 90.0);

    TS_ASSERT_EQUALS(gon2.getAxis(1).name, "angle2");
    TS_ASSERT_EQUALS(gon2.getAxis(1).rotationaxis, V3D(0, 1, 0));
    TS_ASSERT_EQUALS(gon2.getAxis(1).sense, 1);
    TS_ASSERT_EQUALS(gon2.getAxis(1).angle, 0.0);
    AnalysisDataService::Instance().remove("SetMultipleGoniometerTest_ws");
  }

  void test_universal() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetUnivGoniometerTest_ws", ws);
    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetUnivGoniometerTest_ws", "LogName", "phi",
                                      "LogType", "Number Series", "LogText", "1.234");

    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetUnivGoniometerTest_ws", "LogName", "chi",
                                      "LogType", "Number Series", "LogText", "1.234");

    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetUnivGoniometerTest_ws", "LogName", "omega",
                                      "LogType", "Number Series", "LogText", "1.234");

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetUnivGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Goniometers", "Universal"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // no log values

    // Check the results
    const Goniometer &G = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(2).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(0).name, "omega");

    AnalysisDataService::Instance().remove("SetUnivGoniometerTest_ws");
  }

  void test_PeaksWorkspace() {
    PeaksWorkspace_sptr ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    AnalysisDataService::Instance().addOrReplace("SetPeaksWorkspaceGoniometerTest_ws", ws);
    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetPeaksWorkspaceGoniometerTest_ws", "LogName",
                                      "phi", "LogType", "Number Series", "LogText", "1.234");

    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetPeaksWorkspaceGoniometerTest_ws", "LogName",
                                      "chi", "LogType", "Number Series", "LogText", "1.234");

    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetPeaksWorkspaceGoniometerTest_ws", "LogName",
                                      "omega", "LogType", "Number Series", "LogText", "1.234");

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetPeaksWorkspaceGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Goniometers", "Universal"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted()); // no log values

    // Check the results
    const Goniometer &G = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(2).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(0).name, "omega");

    AnalysisDataService::Instance().remove("SetPeaksWorkspaceGoniometerTest_ws");
  }

  /** Do a test with a single param
   *
   * @param axis0 :: string to pass
   * @param numExpected :: how many axes should be created (0 or 1)
   */
  void do_test_param(const std::string &axis0, size_t numExpected = 0) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("SetGoniometerTest_ws", ws);
    FrameworkManager::Instance().exec("AddSampleLog", 8, "Workspace", "SetGoniometerTest_ws", "LogName", "name",
                                      "LogType", "Number Series", "LogText", "1.234");

    SetGoniometer alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "SetGoniometerTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", axis0));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    if (numExpected == 0) {
      TS_ASSERT(!alg.isExecuted());
    } else {
      // Check the results
      const Goniometer &gon = ws->mutableRun().getGoniometer();
      TS_ASSERT_EQUALS(gon.getNumberAxes(), numExpected);
    }
    AnalysisDataService::Instance().remove("SetGoniometerTest_ws");
  }

  void test_param_NotEnough() { do_test_param("name, 1.0, 2.0"); }

  void test_param_TooMany() { do_test_param("name, 1.0, 2.0, 3.0, 1, 12345"); }

  void test_param_WrongCCW() {
    do_test_param("name, 1.0, 2.0, 3.0, 0");
    do_test_param("name, 1.0, 2.0, 3.0, 2");
  }

  void test_param_NotANumber() { do_test_param("name, One, Two, Three, 1"); }

  void test_param_EmptyName() { do_test_param(", 1.0, 2.0, 3.0, 1"); }

  void test_param_ZeroVector() { do_test_param("name, 0.0, 0.0, 0.0, 1"); }

  void test_ok() { do_test_param("name, 1.0, 2.0, 3.0, 1", 1); }
};
