#ifndef MANTID_REMOTEALGORITHMS_STOPREMOTETRANSACTIONTEST_H_
#define MANTID_REMOTEALGORITHMS_STOPREMOTETRANSACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/StopRemoteTransaction.h"

using namespace Mantid::RemoteAlgorithms;

class StopRemoteTransactionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StopRemoteTransactionTest *createSuite() {
    return new StopRemoteTransactionTest();
  }
  static void destroySuite(StopRemoteTransactionTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "StopRemoteTransaction" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "StopRemoteTransaction");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<StopRemoteTransaction> a;
    TS_ASSERT(a = boost::make_shared<StopRemoteTransaction>());

    // can cast to inherited interfaces and base classes
    StopRemoteTransaction alg;
    TS_ASSERT(
        dynamic_cast<Mantid::RemoteAlgorithms::StopRemoteTransaction *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(&alg));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    StopRemoteTransaction stop;
    TS_ASSERT_THROWS_NOTHING(stop.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    StopRemoteTransaction alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // transaction id missing
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    StopRemoteTransaction alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("TransactionID", "john_missing"));

    TS_ASSERT_THROWS(alg2.execute(), std::runtime_error);
    TS_ASSERT(!alg2.isExecuted());
  }

  void test_wrongProperty() {
    StopRemoteTransaction stop;
    TS_ASSERT_THROWS_NOTHING(stop.initialize();)
    TS_ASSERT_THROWS(stop.setPropertyValue("Compute", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(stop.setPropertyValue("Transaction", "whatever"),
                     std::runtime_error);
    TS_ASSERT_THROWS(stop.setPropertyValue("JobID", "whichever"),
                     std::runtime_error);
    TS_ASSERT_THROWS(stop.setPropertyValue("ID", "whichever"),
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
      StopRemoteTransaction stop;
      TS_ASSERT_THROWS_NOTHING(stop.initialize());
      TS_ASSERT_THROWS_NOTHING(
          stop.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(
          stop.setPropertyValue("TransactionID", "000001"));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(stop.execute(), std::exception);
      TS_ASSERT(!stop.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_STOPREMOTETRANSACTIONTEST_H_
