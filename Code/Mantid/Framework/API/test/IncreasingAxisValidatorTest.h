#ifndef MANTID_API_INCREASINGAXISVALIDATORTEST_H_
#define MANTID_API_INCREASINGAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"

#include <boost/shared_ptr.hpp>

class IncreasingAxisValidatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IncreasingAxisValidatorTest *createSuite() { return new IncreasingAxisValidatorTest(); }
  static void destroySuite( IncreasingAxisValidatorTest *suite ) { delete suite; }

  IncreasingAxisValidatorTest()
  {
    m_right_ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,2);

    boost::shared_ptr<Mantid::MantidVec> right_x(new Mantid::MantidVec);
    right_x->push_back(0);
    right_x->push_back(1);
    right_x->push_back(2);

    m_right_ws->setX(0, *right_x.get());

    m_wrong_ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,2);

    boost::shared_ptr<Mantid::MantidVec> wrong_x(new Mantid::MantidVec);
    wrong_x->push_back(2);
    wrong_x->push_back(1);
    wrong_x->push_back(0);

    m_wrong_ws->setX(0, *wrong_x.get());
  }

  void testRight()
  {
    TS_ASSERT_EQUALS(validator.isValid(m_right_ws), "");
  }

  void testWrong()
  {
    TS_ASSERT_DIFFERS(validator.isValid(m_wrong_ws), "");
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_wrong_ws; // Workspace with the wrong axis
  Mantid::API::MatrixWorkspace_sptr m_right_ws; // Workspace with the right axis

  Mantid::API::IncreasingAxisValidator validator;
};


#endif /* MANTID_API_INCREASINGAXISVALIDATORTEST_H_ */