#ifndef MANTID_REMOTEALGORITHMS_QUERYREMOTEFILETEST_H_
#define MANTID_REMOTEALGORITHMS_QUERYREMOTEFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/QueryRemoteFile.h"

using namespace Mantid::RemoteAlgorithms;

class QueryRemoteFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryRemoteFileTest *createSuite() {
    return new QueryRemoteFileTest();
  }
  static void destroySuite(QueryRemoteFileTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "QueryRemoteFile" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "QueryRemoteFile");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<QueryRemoteFile> a;
    TS_ASSERT(a = boost::make_shared<QueryRemoteFile>());

    // can cast to inherited interfaces and base classes
    QueryRemoteFile alg;
    TS_ASSERT(dynamic_cast<Mantid::RemoteAlgorithms::QueryRemoteFile *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(&alg));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    QueryRemoteFile qrf;
    TS_ASSERT_THROWS_NOTHING(qrf.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    QueryRemoteFile alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // Transaction id missing
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    QueryRemoteFile alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("TransactionID", "trans0001"));

    TS_ASSERT_THROWS(alg2.execute(), std::runtime_error);
    TS_ASSERT(!alg2.isExecuted());
  }

  void test_wronProperty() {
    QueryRemoteFile qrf;
    TS_ASSERT_THROWS_NOTHING(qrf.initialize();)
    TS_ASSERT_THROWS(qrf.setPropertyValue("Compute", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qrf.setPropertyValue("TransID", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qrf.setPropertyValue("ComputeResourc", "anything"),
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
      QueryRemoteFile qrf;
      TS_ASSERT_THROWS_NOTHING(qrf.initialize());
      TS_ASSERT_THROWS_NOTHING(
          qrf.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(
          qrf.setPropertyValue("TransactionID", "anything001"));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(qrf.execute(), std::exception);
      TS_ASSERT(!qrf.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_QUERYREMOTEFILETEST_H_
