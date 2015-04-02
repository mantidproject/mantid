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

class MockMantidAPIStatusOK : public MantidWebServiceAPIJobManager {
  std::istream &httpGet(const std::string &path,
                        const std::string &query_str = "",
                        const std::string &username = "",
                        const std::string &password = "") const {
    UNUSED_ARG(path);
    UNUSED_ARG(query_str);
    UNUSED_ARG(username);
    UNUSED_ARG(password);
    return is;
  }

  std::istream &
  httpPost(const std::string &path,
           const MantidWebServiceAPIHelper::PostDataMap &postData,
           const MantidWebServiceAPIHelper::PostDataMap &fileData =
               MantidWebServiceAPIHelper::PostDataMap(),
           const std::string &username = "",
           const std::string &password = "") const {
    UNUSED_ARG(path);
    UNUSED_ARG(postData);
    UNUSED_ARG(fileData);
    UNUSED_ARG(username);
    UNUSED_ARG(password);
    return is;
  }

  Poco::Net::HTTPResponse::HTTPStatus lastStatus() const {
    return Poco::Net::HTTPResponse::HTTP_OK;
  }

private:
  mutable std::istringstream is;
};

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

    Mantid::Kernel::ConfigService::Instance().setFacility(SNSFac);
    TSM_ASSERT_THROWS_NOTHING(
        "create() with " + FermiName + " in the facility " + SNSFac +
            " should not throw",
        IRemoteJobManager_sptr jobManager =
            RemoteJobManagerFactory::Instance().create(FermiName));
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual cluster and will only work
    // within ISIS.

    // it should not be available here...
    Mantid::Kernel::ConfigService::Instance().setFacility(ISISFac);
    TSM_ASSERT_THROWS("create() with " + FermiName +
                          " in a facility other than " + SNSFac +
                          " should fail",
                      IRemoteJobManager_sptr jobManager =
                          RemoteJobManagerFactory::Instance().create(FermiName),
                      std::runtime_error);

    // restore facility to what it was before test
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  void test_defaultValues() {
    MantidWebServiceAPIJobManager h;

    Poco::Net::HTTPResponse::HTTPStatus sts;
    // TS_ASSERT_THROWS_NOTHING(sts = h.lastStatus());
    TSM_ASSERT_EQUALS("Wrong status returned", sts,
                      0 /*Poco::Net::HTTPResponse::HTTP_OK */);

    std::string reason;
    // TS_ASSERT_THROWS_NOTHING(reason = h.lastStatusReason());
    TSM_ASSERT_EQUALS("Last status reason string shold be empty", reason, "");
  }

private:
  std::string goodUsername;
  std::string goodPassword;

  static const std::string SNSFac;
  static const std::string ISISFac;
  static const std::string FermiName;
  static const std::string SCARFName;
};

const std::string MantidWebServiceAPIJobManagerTest::SNSFac = "SNS";
const std::string MantidWebServiceAPIJobManagerTest::ISISFac = "ISIS";
const std::string MantidWebServiceAPIJobManagerTest::FermiName = "Fermi";
const std::string MantidWebServiceAPIJobManagerTest::SCARFName = "SCARF@STFC";

#endif // MANTID_REMOTEJOGMANAGERS_MANTIDWEBSERVICEJOBMANAGERTEST_H_
