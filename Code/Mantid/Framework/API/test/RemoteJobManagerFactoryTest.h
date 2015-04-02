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

class RemoteJobManagerFactoryTest : public CxxTest::TestSuite {
public:
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

  // a simple positive test
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

  const std::string SNSFac = "SNS";
  const std::string ISISFac = "ISIS";
  const std::string FermiName = "Fermi";
  const std::string SCARFName = "SCARF@STFC";
};

#endif /* REMOTEJOBMANAGERFACTORYTEST_H_ */
