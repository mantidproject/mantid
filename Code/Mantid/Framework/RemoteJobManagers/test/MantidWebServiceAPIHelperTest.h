#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"

#include <boost/make_shared.hpp>

/// This is just an overly simple test that objects can be
/// created. Not bothering to improve this, as this
/// MantidWebServiceAPIHelper should be replaced/merged into the more
/// generic Kernel::InternetHelper.
class MantidWebServiceAPIHelperTest : public CxxTest::TestSuite {
 public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MantidWebServiceAPIHelperTest *createSuite() { return new MantidWebServiceAPIHelperTest(); }
  static void destroySuite(MantidWebServiceAPIHelperTest *suite) { delete suite; }

  void test_construct() {
    // can create
    boost::shared_ptr<MantidWebServiceAPIHelper> help;
    TS_ASSERT(help = boost::make_shared<MantidWebServiceAPIHelper>());
    // can cast to inherited interfaces and base classes

    TS_THROWS_NOTHING(MantidWebServiceAPIHelper h);
  }
};

#endif // MANTID_REMOTEJOGMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_
