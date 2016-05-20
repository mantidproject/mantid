#ifndef MANTID_API_INCREASINGAXISVALIDATORTEST_H_
#define MANTID_API_INCREASINGAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidAPI/NumericAxis.h"

#include <boost/shared_ptr.hpp>

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

    points = {2.0, 1.0, 0.0};
    m_wrong_ws->setPoints(0, points);
  }

  void testRight() { TS_ASSERT_EQUALS(validator.isValid(m_right_ws), ""); }

  void testWrong() { TS_ASSERT_DIFFERS(validator.isValid(m_wrong_ws), ""); }

private:
  Mantid::API::MatrixWorkspace_sptr m_wrong_ws; // Workspace with the wrong axis
  Mantid::API::MatrixWorkspace_sptr m_right_ws; // Workspace with the right axis

  Mantid::API::IncreasingAxisValidator validator;
};

#endif /* MANTID_API_INCREASINGAXISVALIDATORTEST_H_ */
