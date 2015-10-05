#ifndef MANTID_REMOTEALGORITHMS_ABORTREMOTEJOB2TEST_H_
#define MANTID_REMOTEALGORITHMS_ABORTREMOTEJOB2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/AbortRemoteJob2.h"

using namespace Mantid::RemoteAlgorithms;

class AbortRemoteJob2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AbortRemoteJob2Test *createSuite() {
    return new AbortRemoteJob2Test();
  }
  static void destroySuite(AbortRemoteJob2Test *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create("AbortRemoteJob"
                                                               /*, 2*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "AbortRemoteJob");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<AbortRemoteJob2> a;
    TS_ASSERT(a = boost::make_shared<AbortRemoteJob2>());
    // can cast to inherited interfaces and base classes

    TS_ASSERT(
        dynamic_cast<Mantid::RemoteAlgorithms::AbortRemoteJob2 *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(a.get()));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    AbortRemoteJob2 auth;
    TS_ASSERT_THROWS_NOTHING(auth.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    AbortRemoteJob2 alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // id missing
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    AbortRemoteJob2 alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("JobID", "john_missing"));

    TS_ASSERT_THROWS(alg3.execute(), std::runtime_error);
    TS_ASSERT(!alg3.isExecuted());
  }

  void test_wrongProperty() {
    AbortRemoteJob2 ab;
    TS_ASSERT_THROWS_NOTHING(ab.initialize();)
    TS_ASSERT_THROWS(ab.setPropertyValue("ComputeRes", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ab.setPropertyValue("username", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(ab.setPropertyValue("sername", "anything"),
                     std::runtime_error);
  }

  void test_wrongResource() {
    AbortRemoteJob2 ab;
    TS_ASSERT_THROWS_NOTHING(ab.initialize());
    // the compute resource given  does not exist:
    TS_ASSERT_THROWS(ab.setPropertyValue("ComputeResource", "missing c r!"),
                     std::invalid_argument);
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
      AbortRemoteJob2 ab;
      TS_ASSERT_THROWS_NOTHING(ab.initialize());
      TS_ASSERT_THROWS_NOTHING(
          ab.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(ab.setPropertyValue("JobID", "000001"));
      // TODO: this will run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(ab.execute(), std::exception);
      TS_ASSERT(!ab.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_ABORTREMOTEJOB2TEST_H_
