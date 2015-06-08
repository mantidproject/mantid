#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"

#include <boost/make_shared.hpp>
#include <Poco/Net/HTTPResponse.h>

using namespace Mantid::RemoteJobManagers;

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

    MantidWebServiceAPIHelper h;
  }

  void test_defaultValues() {
    MantidWebServiceAPIHelper h;

    Poco::Net::HTTPResponse::HTTPStatus sts;
    TS_ASSERT_THROWS_NOTHING(sts = h.lastStatus());
    TS_ASSERT_EQUALS(sts, Poco::Net::HTTPResponse::HTTP_OK);

    std::string reason;
    TS_ASSERT_THROWS_NOTHING(reason = h.lastStatusReason());
    TS_ASSERT_EQUALS(reason, h.lastStatusReason());
  }
};

#endif // MANTID_REMOTEJOGMANAGERS_MANTIDWEBSERVICEAPIHELPERTEST_H_
