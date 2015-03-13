#ifndef MANTID_REMOTEALGORITHMS_UPLOADREMOTEFILETEST_H_
#define MANTID_REMOTEALGORITHMS_UPLOADREMOTEFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/UploadRemoteFile.h"

using namespace Mantid::RemoteAlgorithms;

class UploadRemoteFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UploadRemoteFileTest *createSuite() {
    return new UploadRemoteFileTest();
  }
  static void destroySuite(UploadRemoteFileTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "UploadRemoteFile" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "UploadRemoteFile");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<UploadRemoteFile> a;
    TS_ASSERT(a = boost::make_shared<UploadRemoteFile>());

    // can cast to inherited interfaces and base classes
    UploadRemoteFile alg;
    TS_ASSERT(dynamic_cast<Mantid::RemoteAlgorithms::UploadRemoteFile *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(&alg));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    UploadRemoteFile ul;
    TS_ASSERT_THROWS_NOTHING(ul.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    UploadRemoteFile alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // Transaction id missing
    TS_ASSERT_THROWS_NOTHING(
        alg1.setPropertyValue("RemoteFileName", "file name"));
    TS_ASSERT_THROWS_NOTHING(
        alg1.setPropertyValue("LocalFileName", "local file name"));
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    UploadRemoteFile alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    // remote file name missing
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("LocalFileName", "local file name"));
    TS_ASSERT_THROWS(alg2.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg2.execute(), std::runtime_error);
    TS_ASSERT(!alg2.isExecuted());

    UploadRemoteFile alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    // local file name missing
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("RemoteFileName", "remote file name"));
    TS_ASSERT_THROWS(alg3.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg3.execute(), std::runtime_error);
    TS_ASSERT(!alg3.isExecuted());

    UploadRemoteFile alg4;
    TS_ASSERT_THROWS_NOTHING(alg4.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(
        alg4.setPropertyValue("RemoteFileName", "file name"));
    TS_ASSERT_THROWS_NOTHING(alg4.setPropertyValue("TransactionID", "id001"));

    TS_ASSERT_THROWS(alg4.execute(), std::runtime_error);
    TS_ASSERT(!alg4.isExecuted());
  }

  void test_wrongProperty() {
    UploadRemoteFile ul;
    TS_ASSERT_THROWS_NOTHING(ul.initialize();)
    TS_ASSERT_THROWS(ul.setPropertyValue("Compute", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ul.setPropertyValue("TransID", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ul.setPropertyValue("RemoteFile", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ul.setPropertyValue("FileName", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ul.setPropertyValue("LocalFile", "anything"),
                     std::runtime_error);
  }

  void test_propertiesOK() {
    testFacilities.push_back(std::make_pair("SNS", "Fermi"));
    testFacilities.push_back(std::make_pair("ISIS", "SCARF@STFC"));

    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();
    for (size_t fi = 0; fi < testFacilities.size(); fi++) {
      const std::string facName = testFacilities[fi].first;
      const std::string compName = testFacilities[fi].second;

      Mantid::Kernel::ConfigService::Instance().setFacility(facName);

      UploadRemoteFile ul;
      TS_ASSERT_THROWS_NOTHING(ul.initialize());
      TS_ASSERT_THROWS_NOTHING(
          ul.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(
          ul.setPropertyValue("TransactionID", "anything001"));
      TS_ASSERT_THROWS_NOTHING(
          ul.setPropertyValue("RemoteFileName", "any name"));
      TS_ASSERT_THROWS_NOTHING(
          ul.setPropertyValue("LocalFileName", "any local path"));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(ul.execute(), std::exception);
      TS_ASSERT(!ul.isExecuted());
    }
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  // TODO: void test_runOK() - with a mock when we can add it.
  // ideally, with different compute resources to check the remote job
  // manager factory, etc.

private:
  Mantid::API::IAlgorithm_sptr testAlg;
  std::vector<std::pair<std::string, std::string>> testFacilities;
};

#endif // MANTID_REMOTEALGORITHMS_UPLOADREMOTEFILETEST_H_
