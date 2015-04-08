#ifndef REMOTEJOBMANAGERFACTORYTEST_H_
#define REMOTEJOBMANAGERFACTORYTEST_H_

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::API;

// Just a minimal implementation of IRemoteJobManager, sufficient for the
// factory
// TODO: use gmock for this
class FakeJM : public IRemoteJobManager {
public:
  virtual void authenticate(const std::string & /*username*/,
                            const std::string & /*password*/) {}

  virtual std::string submitRemoteJob(const std::string & /*transactionID*/,
                                      const std::string & /*runnable*/,
                                      const std::string & /*param*/,
                                      const std::string & /*taskName*/ = "",
                                      const int /*numNodes*/ = 1,
                                      const int /*coresPerNode*/ = 1) {
    return "";
  }

  virtual void downloadRemoteFile(const std::string & /*transactionID*/,
                                  const std::string & /*remoteFileName*/,
                                  const std::string & /*localFileName*/) {}

  virtual std::vector<RemoteJobInfo> queryAllRemoteJobs() const {
    return std::vector<RemoteJobInfo>();
  }

  virtual std::vector<std::string>
  queryRemoteFile(const std::string & /*transactionID*/) const {
    return std::vector<std::string>();
  }

  virtual RemoteJobInfo queryRemoteJob(const std::string & /*jobID*/) const {
    return RemoteJobInfo();
  }

  virtual std::string startRemoteTransaction() { return ""; }

  virtual void stopRemoteTransaction(const std::string & /*transactionID*/) {}

  virtual void abortRemoteJob(const std::string & /*jobID*/) {}

  virtual void uploadRemoteFile(const std::string & /*transactionID*/,
                                const std::string & /*remoteFileName*/,
                                const std::string & /*localFileName*/) {}
};

class FakeJMDeriv : public FakeJM {};

class FakeJM3 : public FakeJMDeriv {};

DECLARE_REMOTEJOBMANAGER(FakeJM)
DECLARE_REMOTEJOBMANAGER(FakeJMDeriv)
DECLARE_REMOTEJOBMANAGER(FakeJM3)

class RemoteJobManagerFactoryTest : public CxxTest::TestSuite {
public:
  void test_unsubscribeDeclared() {
    // subscribed with DECLARE_...
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "FakeJM3"));
  }

  void test_unsubscribed() {
    IRemoteJobManager_sptr jobManager;
    TSM_ASSERT_THROWS(
        "create() with inexistent and unsubscribed class should "
        "throw",
        jobManager = RemoteJobManagerFactory::Instance().create("Inexistent"),
        std::runtime_error);

    TSM_ASSERT_THROWS("create() with unsubscribed class should throw",
                      jobManager =
                          RemoteJobManagerFactory::Instance().create("FakeJM"),
                      std::runtime_error);
  }

  // minimal positive test
  void test_createFakeJM() {
    RemoteJobManagerFactory::Instance().subscribe<FakeJM>("FakeJM");
    // throws not found cause it is not in facilities.xml, but otherwise fine
    TSM_ASSERT_THROWS(
        "create() with class name that is not defined in facilities should "
        "throw",
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create("FakeJM"),
        Mantid::Kernel::Exception::NotFoundError);
  }

  void test_exists() {
    // a bit of stress, unsubscribe after being subscribed with DECLARE_...,
    // then subscribe and the unsubscribe again
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>(
            "FakeJM"));

    std::vector<std::string> keys =
        Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t count = keys.size();

    TS_ASSERT_THROWS(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>(
            "FakeJM"),
        std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>(
            "FakeJM"));

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJMDeriv>(
            "FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJMDeriv>(
            "FakeJM3"));

    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJM"));
    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJMDeriv"));
    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJM3"));

    // these are not in the facilities file
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create("FakeJM"),
        std::runtime_error);
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create(
            "FakeJMDeriv"),
        std::runtime_error);

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t after = keys.size();

    TS_ASSERT_EQUALS(count + 2, after);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "FakeJM3"));

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t newCount = keys.size();

    TS_ASSERT_EQUALS(after - 3, newCount);
  }

  // this must fail, resource not found in the current facility
  void test_createAlienResource() {
    // save facility, do this before any changes
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility(ISISFac);
    TSM_ASSERT_THROWS(
        "create() with " + FermiName + "in a facility other than " + SNSFac +
            " should fail",
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create(FermiName),
        Mantid::Kernel::Exception::NotFoundError);

    Mantid::Kernel::ConfigService::Instance().setFacility(SNSFac);
    TSM_ASSERT_THROWS(
        "create() with " + SCARFName + "in a facility other than " + ISISFac +
            " should fail",
        Mantid::API::IRemoteJobManager_sptr jobManager =
            Mantid::API::RemoteJobManagerFactory::Instance().create(SCARFName),
        Mantid::Kernel::Exception::NotFoundError);

    // restore facility, always do this at the end
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  // a simple positive test, meant to be moved to the job managers tests as
  // these are added
  void test_createRemoteManagers() {
    // save facility, do this before any changes
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");

    // These two create should throw a NotFoundError because the
    // RemoteJobManager classes are missing and have not done a
    // DECLARE_REMOTEJOBMANAGER. A positive test is done in the tests
    // of the job managers
    TSM_ASSERT_THROWS(
        "create() with " + FermiName +
            " should throw because its job manager is not declared",
        Mantid::API::IRemoteJobManager_sptr jobManager =
            Mantid::API::RemoteJobManagerFactory::Instance().create("Fermi"),
        Mantid::Kernel::Exception::NotFoundError);

    Mantid::Kernel::ConfigService::Instance().setFacility("ISIS");
    TSM_ASSERT_THROWS(
        "create() with " + SCARFName +
            " should throw because its job manager is not declared",
        Mantid::API::IRemoteJobManager_sptr jobManager =
            Mantid::API::RemoteJobManagerFactory::Instance().create(
                "SCARF@STFC"),
        Mantid::Kernel::Exception::NotFoundError);

    // restore facility, always do this at the end
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

private:
  Mantid::API::IRemoteJobManager_sptr jm;

  static const std::string SNSFac;
  static const std::string ISISFac;
  static const std::string FermiName;
  static const std::string SCARFName;
};

const std::string RemoteJobManagerFactoryTest::SNSFac = "SNS";
const std::string RemoteJobManagerFactoryTest::ISISFac = "ISIS";
const std::string RemoteJobManagerFactoryTest::FermiName = "Fermi";
const std::string RemoteJobManagerFactoryTest::SCARFName = "SCARF@STFC";

#endif /* REMOTEJOBMANAGERFACTORYTEST_H_ */
