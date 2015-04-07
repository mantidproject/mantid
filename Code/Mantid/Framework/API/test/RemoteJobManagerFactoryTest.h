#ifndef REMOTEJOBMANAGERFACTORYTEST_H_
#define REMOTEJOBMANAGERFACTORYTEST_H_

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::API;

// Just a minimal implementation of IRemoteJobManager, sufficient for the
// factory
class TestJM : public IRemoteJobManager {
public:
  virtual void authenticate(const std::string &username,
                            const std::string &password) {
    UNUSED_ARG(username);
    UNUSED_ARG(password);
  }

  virtual std::string
  submitRemoteJob(const std::string &transactionID, const std::string &runnable,
                  const std::string &param, const std::string &taskName = "",
                  const int numNodes = 1, const int coresPerNode = 1) {
    UNUSED_ARG(transactionID);
    UNUSED_ARG(runnable);
    UNUSED_ARG(param);
    UNUSED_ARG(taskName);
    UNUSED_ARG(numNodes);
    UNUSED_ARG(coresPerNode);
    return "";
  }

  virtual void downloadRemoteFile(const std::string &transactionID,
                                  const std::string &remoteFileName,
                                  const std::string &localFileName) {
    UNUSED_ARG(transactionID);
    UNUSED_ARG(remoteFileName);
    UNUSED_ARG(localFileName);
  }

  virtual std::vector<RemoteJobInfo> queryAllRemoteJobs() const {
    return std::vector<RemoteJobInfo>();
  }

  virtual std::vector<std::string>
  queryRemoteFile(const std::string &transactionID) const {
    UNUSED_ARG(transactionID);
    return std::vector<std::string>();
  }

  virtual RemoteJobInfo queryRemoteJob(const std::string &jobID) const {
    UNUSED_ARG(jobID);
    return RemoteJobInfo();
  }

  virtual std::string startRemoteTransaction() { return ""; }

  virtual void stopRemoteTransaction(const std::string &transactionID) {
    UNUSED_ARG(transactionID);
  }

  virtual void abortRemoteJob(const std::string &jobID) { UNUSED_ARG(jobID); }

  virtual void uploadRemoteFile(const std::string &transactionID,
                                const std::string &remoteFileName,
                                const std::string &localFileName) {
    UNUSED_ARG(transactionID);
    UNUSED_ARG(remoteFileName);
    UNUSED_ARG(localFileName);
  }
};

class TestJMDeriv : public TestJM {};

class TestJM3 : public TestJMDeriv {};

DECLARE_REMOTEJOBMANAGER(TestJM)
DECLARE_REMOTEJOBMANAGER(TestJMDeriv)
DECLARE_REMOTEJOBMANAGER(TestJM3)

class RemoteJobManagerFactoryTest : public CxxTest::TestSuite {
public:
  void test_unsubscribeDeclared() {
    // subscribed with DECLARE_...
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("TestJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "TestJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "TestJM3"));
  }

  void test_unsubscribed() {
    IRemoteJobManager_sptr jobManager;
    TS_ASSERT_THROWS(
        jobManager = RemoteJobManagerFactory::Instance().create("Inexistent"),
        std::runtime_error);

    TS_ASSERT_THROWS(jobManager =
                         RemoteJobManagerFactory::Instance().create("TestJM"),
                     std::runtime_error);
  }

  // minimal positive test
  void test_createTestJM() {
    TS_ASSERT_THROWS_NOTHING(
        RemoteJobManagerFactory::Instance().subscribe<TestJM>("TestJM"));
    // throws not found cause it is not in facilities.xml, but otherwise fine
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create("TestJM"),
        Mantid::Kernel::Exception::NotFoundError);
  }

  void test_exists() {
    // a bit of stress, unsubscribe after being subscribed with DECLARE_...,
    // then subscribe and the unsubscribe again
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("TestJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<TestJM>(
            "TestJM"));

    std::vector<std::string> keys =
        Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t count = keys.size();

    TS_ASSERT_THROWS(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<TestJM>(
            "TestJM"),
        std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("TestJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<TestJM>(
            "TestJM"));

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<TestJMDeriv>(
            "TestJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().subscribe<TestJMDeriv>(
            "TestJM3"));

    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("TestJM"));
    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("TestJMDeriv"));
    TS_ASSERT(
        Mantid::API::RemoteJobManagerFactory::Instance().exists("TestJM3"));

    // these are not in the facilities file
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create("TestJM"),
        std::runtime_error);
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create(
            "TestJMDeriv"),
        std::runtime_error);

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t after = keys.size();

    TS_ASSERT_EQUALS(count + 2, after);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("TestJM"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "TestJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe(
            "TestJM3"));

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t newCount = keys.size();

    TS_ASSERT_EQUALS(after - 3, newCount);
  }

  // this must fail, resource not found in the current facility
  void test_createAlienResource() {
    // save facility, do this before any changes
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("ISIS");
    TS_ASSERT_THROWS(
        jm = Mantid::API::RemoteJobManagerFactory::Instance().create("Fermi"),
        Mantid::Kernel::Exception::NotFoundError);

    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");
    TS_ASSERT_THROWS(
        Mantid::API::IRemoteJobManager_sptr jobManager =
            Mantid::API::RemoteJobManagerFactory::Instance().create(
                "SCARF@STFC"),
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
    // TODO: at the moment this throws a NotFoundError for Fermi
    // because the RemoteJobManager class for the Mantid web service
    // API is missing and there is no DECLARE_REMOTEJOBMANAGER for
    // it. Change this (move to the MantidAPIWebServiceJobManager unit test)
    // when that is done (ticket #11391 etc.)
    TS_ASSERT_THROWS(
        Mantid::API::IRemoteJobManager_sptr jobManager =
            Mantid::API::RemoteJobManagerFactory::Instance().create("Fermi"),
        Mantid::Kernel::Exception::NotFoundError);
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual cluster and will only work
    // within SNS.

    // restore facility, always do this at the end
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

private:
  Mantid::API::IRemoteJobManager_sptr jm;
};

#endif /* REMOTEJOBMANAGERFACTORYTEST_H_ */
