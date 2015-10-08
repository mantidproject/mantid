#ifndef MANTID_REMOTEALGORITHMS_AUTHENTICATETEST_H_
#define MANTID_REMOTEALGORITHMS_AUTHENTICATETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/Authenticate.h"

using namespace Mantid::RemoteAlgorithms;

class AuthenticateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AuthenticateTest *createSuite() { return new AuthenticateTest(); }
  static void destroySuite(AuthenticateTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("Authenticate", 1);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "Authenticate");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<Authenticate> a;
    TS_ASSERT(a = boost::make_shared<Authenticate>());
    // can cast to inherited interfaces and base classes

    TS_ASSERT(dynamic_cast<Mantid::RemoteAlgorithms::Authenticate *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(a.get()));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    Authenticate auth;
    TS_ASSERT_THROWS_NOTHING(auth.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    Authenticate alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // password missing
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("UserName", "john_missing"));
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    Authenticate alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    // username missing
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Password", "LogIn"));
    TS_ASSERT_THROWS(alg2.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg2.execute(), std::runtime_error);
    TS_ASSERT(!alg2.isExecuted());

    Authenticate alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("UserName", "john_missing"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("Password", "LogIn"));

    TS_ASSERT_THROWS(alg3.execute(), std::runtime_error);
    TS_ASSERT(!alg3.isExecuted());
  }

  void test_wrongProperty() {
    Authenticate auth;
    TS_ASSERT_THROWS_NOTHING(auth.initialize());
    TS_ASSERT_THROWS(auth.setPropertyValue("usernam", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(auth.setPropertyValue("sername", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(auth.setPropertyValue("Passwo", "anything"),
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
      Authenticate auth;
      TS_ASSERT_THROWS_NOTHING(auth.initialize());
      TS_ASSERT_THROWS_NOTHING(
          auth.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(
          auth.setPropertyValue("UserName", "john_missing"));
      TS_ASSERT_THROWS_NOTHING(auth.setPropertyValue("Password", "LogIn"));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(auth.execute(), std::exception);
      TS_ASSERT(!auth.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_AUTHENTICATETEST_H_
