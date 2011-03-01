#ifndef MANTID_MDEVENTS_MDEVENTFACTORYTEST_H_
#define MANTID_MDEVENTS_MDEVENTFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidMDEvents/MDEventFactory.h>

using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MDEventFactoryTest : public CxxTest::TestSuite
{
public:

  /** Create MDEW's with various number of dimensions */
  void test_factory()
  {
    IMDEventWorkspace_sptr ew;
    ew = MDEventFactory::CreateMDEventWorkspace(4, "MDEvent");
    TS_ASSERT_EQUALS( ew->getNumDims(), 4);

    size_t n = 9;
    ew = MDEventFactory::CreateMDEventWorkspace(n);
    TS_ASSERT_EQUALS( ew->getNumDims(), n);

    TS_ASSERT_THROWS( ew = MDEventFactory::CreateMDEventWorkspace(0), std::invalid_argument);
  }


  // Templated function that will be called for a specific MDEW
  template<typename MDE, size_t nd>
  void functionTest(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    test_value = ws->getNumDims();
  }

  void test_CALL_MDEVENT_FUNCTION_macro()
  {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDEvent<1>, 1>() );
    TS_ASSERT_EQUALS( ew->getNumDims(), 1);
    TS_ASSERT_EQUALS( ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION( functionTest, ew );
    TS_ASSERT_EQUALS( test_value, 1 );
  }

  void test_CALL_MDEVENT_FUNCTION_macro_2()
  {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDEvent<8>, 8>() );
    TS_ASSERT_EQUALS( ew->getNumDims(), 8);
    TS_ASSERT_EQUALS( ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION( functionTest, ew );
    TS_ASSERT_EQUALS( test_value, 8 );
  }


  int test_value;

};


#endif /* MANTID_MDEVENTS_MDEVENTFACTORYTEST_H_ */

