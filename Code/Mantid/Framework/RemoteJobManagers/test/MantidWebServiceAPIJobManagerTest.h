#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIJobManager.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::RemoteJobManagers;

class MantidWebServiceAPIJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MantidWebServiceAPIJobManagerTest *createSuite() {
    return new MantidWebServiceAPIJobManagerTest();
  }
  static void destroySuite(MantidWebServiceAPIJobManagerTest *suite) {
    delete suite;
  }

  void test_construct() {
    // can create
    boost::shared_ptr<MantidWebServiceAPIJobManager> help;
    TS_ASSERT(help = boost::make_shared<MantidWebServiceAPIJobManager>());
    // can cast to inherited interfaces and base classes

    MantidWebServiceAPIJobManager h;
  }

  void test_createWithFactory() {
    // The factory is tested in its own unit test, but here we can specifically
    // test the creation of Mantid WS API objects.

    // save facility before test
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");
    TS_ASSERT_THROWS_NOTHING(
        IRemoteJobManager_sptr jobManager =
            RemoteJobManagerFactory::Instance().create(FermiName));
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual cluster and will only work
    // within ISIS.

    // it should not be available here...
    Mantid::Kernel::ConfigService::Instance().setFacility("ISIS");
    TS_ASSERT_THROWS(IRemoteJobManager_sptr jobManager =
                         RemoteJobManagerFactory::Instance().create(FermiName),
                     std::runtime_error);

    // restore facility to what it was before test
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  void test_defaultValues() {
    MantidWebServiceAPIJobManager h;

    Poco::Net::HTTPResponse::HTTPStatus sts;
    // TS_ASSERT_THROWS_NOTHING(sts = h.lastStatus());
    TS_ASSERT_EQUALS(sts, 0 /*Poco::Net::HTTPResponse::HTTP_OK */);

    std::string reason;
    // TS_ASSERT_THROWS_NOTHING(reason = h.lastStatusReason());
    TS_ASSERT_EQUALS(reason, "");
  }

private:
  std::string goodUsername;
  std::string goodPassword;
  static const std::string FermiName;
};

const std::string MantidWebServiceAPIJobManagerTest::FermiName = "Fermi";

#endif // MANTID_REMOTEJOGMANAGERS_MANTIDWEBSERVICEJOBMANAGERTEST_H_
