// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMD_MATRIX_WORKSPACETEST_H_
#define IMD_MATRIX_WORKSPACETEST_H_

// Tests the MatrixWorkspace as an IMDWorkspace.

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "PropertyManagerHelper.h"
#include <cxxtest/TestSuite.h>

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Test the MatrixWorkspace as an IMDWorkspace.
class IMDWorkspaceTest : public CxxTest::TestSuite {
private:
  WorkspaceTester workspace;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IMDWorkspaceTest *createSuite() { return new IMDWorkspaceTest(); }
  static void destroySuite(IMDWorkspaceTest *suite) { delete suite; }

  IMDWorkspaceTest() {
    workspace.setTitle("workspace");
    workspace.initialize(2, 4, 3);
    for (int i = 0; i < 4; ++i) {
      workspace.dataX(0)[i] = i;
      workspace.dataX(1)[i] = i + 4;
    }

    for (int i = 0; i < 3; ++i) {
      workspace.dataY(0)[i] = i * 10;
      workspace.dataE(0)[i] = sqrt(workspace.dataY(0)[i]);
      workspace.dataY(1)[i] = i * 100;
      workspace.dataE(1)[i] = sqrt(workspace.dataY(1)[i]);
    }
  }

  void testGetXDimension() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension =
        matrixWS.getXDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-X does not have the expected dimension id.",
                      "xDimension", id);
  }

  void testGetYDimension() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension =
        matrixWS.getYDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-Y does not have the expected dimension id.",
                      "yDimension", id);
  }

  void testGetZDimension() {
    WorkspaceTester matrixWS;
    TSM_ASSERT_THROWS_ANYTHING(
        "Current implementation should throw runtime error.",
        matrixWS.getZDimension());
  }

  void testGettDimension() {
    WorkspaceTester matrixWS;
    TSM_ASSERT_THROWS_ANYTHING(
        "Current implementation should throw runtime error.",
        matrixWS.getTDimension());
  }

  void testGetDimensionThrows() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(1, 1, 1);
    TSM_ASSERT_THROWS("Id doesn't exist. Should throw during find routine.",
                      matrixWS.getDimensionWithId("3"),
                      const std::overflow_error &);
  }

  void testGetDimension() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim =
        matrixWS.getDimensionWithId("yDimension");
    TSM_ASSERT_EQUALS(
        "The dimension id found is not the same as that searched for.",
        "yDimension", dim->getDimensionId());
  }

  void testGetDimensionOverflow() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(1, 1, 1);
    TSM_ASSERT_THROWS(
        "The dimension does not exist. Attempting to get it should throw",
        matrixWS.getDimensionWithId("1"), const std::overflow_error &);
  }

  void testGetNPoints() {
    WorkspaceTester matrixWS;
    matrixWS.initialize(5, 5, 5);
    TSM_ASSERT_EQUALS("The expected number of points have not been returned.",
                      25, matrixWS.getNPoints());
  }

  /**
   * Test declaring an input workspace and retrieving as const_sptr or sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    IMDWorkspace_sptr wsInput(new WorkspaceTester());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    IMDWorkspace_const_sptr wsConst;
    IMDWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<IMDWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst =
                                 manager.getValue<IMDWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    IMDWorkspace_const_sptr wsCastConst;
    IMDWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (IMDWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (IMDWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};

#endif /*IMD_MATRIX_WORKSPACETEST_H_*/
