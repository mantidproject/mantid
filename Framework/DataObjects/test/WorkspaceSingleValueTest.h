// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TESTWORKSPACESINGLEVALUE_
#define TESTWORKSPACESINGLEVALUE_

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "PropertyManagerHelper.h"

using Mantid::DataObjects::WorkspaceSingleValue;
using Mantid::DataObjects::WorkspaceSingleValue_const_sptr;
using Mantid::DataObjects::WorkspaceSingleValue_sptr;

class WorkspaceSingleValueTest : public CxxTest::TestSuite {

public:
  void testConstructorDefaults() {
    WorkspaceSingleValue ws;
    TS_ASSERT_DELTA(0.0, ws.x(0)[0], 1e-6);
    TS_ASSERT_DELTA(0.0, ws.y(0)[0], 1e-6);
    TS_ASSERT_DELTA(0.0, ws.e(0)[0], 1e-6);
  }
  void testConstructor() {
    WorkspaceSingleValue ws(1, 2);
    TS_ASSERT_DELTA(0.0, ws.x(0)[0], 1e-6);
    TS_ASSERT_DELTA(1, ws.y(0)[0], 1e-6);
    TS_ASSERT_DELTA(2, ws.e(0)[0], 1e-6);
  }

  void testClone() {
    WorkspaceSingleValue ws(2.0, 0.1);
    auto cloned = ws.clone();

    TS_ASSERT_EQUALS(ws.x(0)[0], cloned->x(0)[0]);
    TS_ASSERT_EQUALS(ws.y(0)[0], cloned->y(0)[0]);
    TS_ASSERT_EQUALS(ws.e(0)[0], cloned->e(0)[0]);
  }

  void testsetgetXvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.mutableX(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.x(0).rawData());
  }
  void testsetgetYvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.mutableY(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.y(0).rawData());
  }

  void testsetgetEvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.mutableE(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.e(0).rawData());
  }

  void testgetNumDims() {
    WorkspaceSingleValue ws;
    TS_ASSERT_EQUALS(0, ws.getNumDims());
  }

  /**
   * Test declaring an input WorkspaceSingleValue and retrieving it as
   * const_sptr or sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    WorkspaceSingleValue_sptr wsInput(new WorkspaceSingleValue());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    WorkspaceSingleValue_const_sptr wsConst;
    WorkspaceSingleValue_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<WorkspaceSingleValue_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(
        wsNonConst = manager.getValue<WorkspaceSingleValue_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    WorkspaceSingleValue_const_sptr wsCastConst;
    WorkspaceSingleValue_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst =
                                 (WorkspaceSingleValue_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (WorkspaceSingleValue_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};
#endif /*TESTWORKSPACESINGLEVALUE_*/
