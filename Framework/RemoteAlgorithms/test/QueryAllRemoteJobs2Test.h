// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REMOTEALGORITHMS_QUERYALLREMOTEJOBS2TEST_H_
#define MANTID_REMOTEALGORITHMS_QUERYALLREMOTEJOBS2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/QueryAllRemoteJobs2.h"

using namespace Mantid::RemoteAlgorithms;

class QueryAllRemoteJobs2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryAllRemoteJobs2Test *createSuite() {
    return new QueryAllRemoteJobs2Test();
  }
  static void destroySuite(QueryAllRemoteJobs2Test *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "QueryAllRemoteJobs" /*, 2*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "QueryAllRemoteJobs");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<QueryAllRemoteJobs2> a;
    TS_ASSERT(a = boost::make_shared<QueryAllRemoteJobs2>());

    // can cast to inherited interfaces and base classes
    TS_ASSERT(
        dynamic_cast<Mantid::RemoteAlgorithms::QueryAllRemoteJobs2 *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(a.get()));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    QueryAllRemoteJobs2 qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    QueryAllRemoteJobs2 qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(qar.setPropertyValue("JobID", "john_missing"));

    TS_ASSERT_THROWS(qar.execute(), std::runtime_error);
    TS_ASSERT(!qar.isExecuted());
  }

  void test_wrongProperty() {
    QueryAllRemoteJobs2 qar;
    TS_ASSERT_THROWS_NOTHING(qar.initialize();)
    TS_ASSERT_THROWS(qar.setPropertyValue("ComputeRes", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qar.setPropertyValue("TransactionID", "whatever"),
                     std::runtime_error);
    TS_ASSERT_THROWS(qar.setPropertyValue("ID", "whichever"),
                     std::runtime_error);
  }

  void test_propertiesOK() {
    testFacilities.emplace_back("SNS", "Fermi");
    testFacilities.emplace_back("ISIS", "SCARF@STFC");

    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();
    for (auto &testFacility : testFacilities) {
      const auto facName = testFacility.first;
      const auto compName = testFacility.second;

      Mantid::Kernel::ConfigService::Instance().setFacility(facName);
      QueryAllRemoteJobs2 qar;
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
