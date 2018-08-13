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
    TS_ASSERT_DELTA(0.0, ws.dataX(0)[0], 1e-6);
    TS_ASSERT_DELTA(0.0, ws.dataY(0)[0], 1e-6);
    TS_ASSERT_DELTA(0.0, ws.dataE(0)[0], 1e-6);
  }
  void testConstructor() {
    WorkspaceSingleValue ws(1, 2);
    TS_ASSERT_DELTA(0.0, ws.dataX(0)[0], 1e-6);
    TS_ASSERT_DELTA(1, ws.dataY(0)[0], 1e-6);
    TS_ASSERT_DELTA(2, ws.dataE(0)[0], 1e-6);
  }

  void testClone() {
    WorkspaceSingleValue ws(2.0, 0.1);
    auto cloned = ws.clone();

    TS_ASSERT_EQUALS(ws.dataX(0)[0], cloned->dataX(0)[0]);
    TS_ASSERT_EQUALS(ws.dataY(0)[0], cloned->dataY(0)[0]);
    TS_ASSERT_EQUALS(ws.dataE(0)[0], cloned->dataE(0)[0]);
  }

  void testsetgetXvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.dataX(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.dataX(0));
  }
  void testsetgetYvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.dataY(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.dataY(0));
  }

  void testsetgetEvector() {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1, 1.1);
    ws.dataE(0) = v1;
    TS_ASSERT_EQUALS(v1, ws.dataE(0));
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
