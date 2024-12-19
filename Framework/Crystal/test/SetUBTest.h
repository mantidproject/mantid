// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidCrystal/SetUB.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::OrientedLattice;

class SetUBTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetUBTest *createSuite() { return new SetUBTest(); }
  static void destroySuite(SetUBTest *suite) { delete suite; }

  /// test to check initialization
  void test_Init() {
    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /// test to check the default settings
  void test_defaultexec() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    //    TS_ASSERT_THROWS_NOTHING(
    //    alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws1;
    TS_ASSERT_THROWS_NOTHING(ws1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    TS_ASSERT(ws1);
    if (!ws1)
      return;

    OrientedLattice latt;
    TS_ASSERT_THROWS_NOTHING(latt = ws1->mutableSample().getOrientedLattice());
    TS_ASSERT_DELTA(latt.a(), 1., 1e-4);
    TS_ASSERT_DELTA(latt.b(), 1., 1e-4);
    TS_ASSERT_DELTA(latt.c(), 1., 1e-4);
    TS_ASSERT_DELTA(latt.alpha(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.beta(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 1e-4);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  /// test to check if setting UB works
  void test_settingUB() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS(alg.setPropertyValue("UB", "1,1"),
                     const std::invalid_argument &); // should fail to initialize UB,
                                                     // since 9 elements are required
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UB", "0,0,2,0,4,0,-8,0,0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws1;
    TS_ASSERT_THROWS_NOTHING(ws1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    TS_ASSERT(ws1);
    if (!ws1)
      return;

    OrientedLattice latt;
    TS_ASSERT_THROWS_NOTHING(latt = ws1->mutableSample().getOrientedLattice());
    TS_ASSERT_DELTA(latt.a(), 0.125, 1e-4);
    TS_ASSERT_DELTA(latt.b(), 0.25, 1e-4);
    TS_ASSERT_DELTA(latt.c(), 0.5, 1e-4);
    TS_ASSERT_DELTA(latt.alpha(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.beta(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 1e-4);
    DblMatrix u(3, 3);
    u[0][2] = 1.;
    u[1][1] = 1.;
    u[2][0] = -1.;
    TS_ASSERT(u.equals(latt.getU(), 1e-7));
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  /// test to check if it fails when |UB|=0
  void test_settingUB_fail() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS(alg.setPropertyValue("UB", "1,1"),
                     const std::invalid_argument &); // should fail to initialize UB,
                                                     // since 9 elements are required
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UB", "1,1,1,1,1,1,1,1,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(!alg.isExecuted());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  /// test to check if it fails when |Bu|=0
  void test_settingLattice_failBu() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("u", "0,0,0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(!alg.isExecuted());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  /// test to check if it fails when u||v
  void test_settingLattice_failuv() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("u", "1,0,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("v", "2,0,0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(!alg.isExecuted());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  /// test setting lattice parameters + u and v
  void test_settingLattice() {
    // Name of the output workspace.
    std::string wsName("SetUBTest_WS");
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    SetUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS(alg.setPropertyValue("alpha", "1"),
                     const std::invalid_argument &); // should fail to initialize alpha,
                                                     // since angle is too small
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("alpha", "90."));
    TS_ASSERT_THROWS(alg.setPropertyValue("u", "0,0,2,0,4,0,-8,0,0"),
                     const std::invalid_argument &); // should fail, only 3 numbers allowed
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("u", "0,2,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("v", "2,0,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("a", "4"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("b", "4"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("c", "4"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws1;
    TS_ASSERT_THROWS_NOTHING(ws1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    TS_ASSERT(ws1);
    if (!ws1)
      return;

    OrientedLattice latt;
    TS_ASSERT_THROWS_NOTHING(latt = ws1->mutableSample().getOrientedLattice());
    TS_ASSERT_DELTA(latt.a(), 4, 1e-4);
    TS_ASSERT_DELTA(latt.b(), 4, 1e-4);
    TS_ASSERT_DELTA(latt.c(), 4, 1e-4);
    TS_ASSERT_DELTA(latt.alpha(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.beta(), 90.0, 1e-4);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 1e-4);
    DblMatrix u(3, 3);
    u[0][0] = 1.;
    u[1][2] = -1.;
    u[2][1] = 1.;
    TS_ASSERT(u.equals(latt.getU(), 1e-7));
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }
};
