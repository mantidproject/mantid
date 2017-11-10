#ifndef MANTID_API_INCREASINGAXISVALIDATORTEST_H_
#define MANTID_API_INCREASINGAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <boost/shared_ptr.hpp>

#include <algorithm>

using namespace Mantid;

class IncreasingAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IncreasingAxisValidatorTest *createSuite() {
    return new IncreasingAxisValidatorTest();
  }
  static void destroySuite(IncreasingAxisValidatorTest *suite) { delete suite; }

  IncreasingAxisValidatorTest() {
    m_right_ws = boost::make_shared<WorkspaceTester>();
    m_right_ws->initialize(1, 3, 3);

    auto points = {0.0, 1.0, 2.0};
    m_right_ws->setPoints(0, points);

    m_wrong_ws = boost::make_shared<WorkspaceTester>();
    m_wrong_ws->initialize(1, 3, 3);

    auto bad_points = {2.0, 1.0, 0.0};
    std::copy(bad_points.begin(), bad_points.end(),
              m_wrong_ws->mutableX(0).begin());
  }

  void testRight() { TS_ASSERT_EQUALS(validator.isValid(m_right_ws), ""); }

  void testWrong() { TS_ASSERT_DIFFERS(validator.isValid(m_wrong_ws), ""); }

  void testSingleValuedWorkspace() {
    auto testWS = boost::make_shared<AxeslessWorkspaceTester>();
    testWS->initialize(1, 3, 3);
    std::string s;
    TS_ASSERT_THROWS_NOTHING(s = validator.isValid(testWS))
    TS_ASSERT_DIFFERS(s, "")
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_wrong_ws; // Workspace with the wrong axis
  Mantid::API::MatrixWorkspace_sptr m_right_ws; // Workspace with the right axis

  Mantid::API::IncreasingAxisValidator validator;
};

#endif /* MANTID_API_INCREASINGAXISVALIDATORTEST_H_ */
