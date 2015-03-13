#ifndef MANTID_REMOTEALGORITHMS_QUERYALLREMOTEJOBSTEST_H_
#define MANTID_REMOTEALGORITHMS_QUERYALLREMOTEJOBSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/QueryAllRemoteJobs.h"

using namespace Mantid::RemoteAlgorithms;

class QueryAllRemoteJobsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryAllRemoteJobsTest *createSuite() {
    return new QueryAllRemoteJobsTest();
  }
  static void destroySuite(QueryAllRemoteJobsTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "QueryAllRemoteJobs" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "QueryAllRemoteJobs");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<QueryAllRemoteJobs> a;
    TS_ASSERT(a = boost::make_shared<QueryAllRemoteJobs>());

    // can cast to inherited interfaces and base classes
    QueryAllRemoteJobs alg;
    TS_ASSERT(
        dynamic_cast<Mantid::RemoteAlgorithms::QueryAllRemoteJobs *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(&alg));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    QueryAllRemoteJobs qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    QueryAllRemoteJobs qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(qar.setPropertyValue("JobID", "john_missing"));

    TS_ASSERT_THROWS(qar.execute(), std::runtime_error);
    TS_ASSERT(!qar.isExecuted());
  }

  void test_wronProperty() {
    QueryAllRemoteJobs qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize();)
    TS_ASSERT_THROWS(qar.setPropertyValue("ComputeRes", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qar.setPropertyValue("TransactionID", "whatever"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qar.setPropertyValue("ID", "whichever"),
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
      QueryAllRemoteJobs qar;
      TS_ASSERT_THROWS_NOTHING(qar.initialize());
      TS_ASSERT_THROWS_NOTHING(
          qar.setPropertyValue("ComputeResource", compName));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(qar.execute(), std::exception);
      TS_ASSERT(!qar.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_QUERYALLREMOTEJOBSTEST_H_
