// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REMOTEALGORITHMS_LOGOUT2TEST_H_
#define MANTID_REMOTEALGORITHMS_LOGOUT2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/Logout2.h"

using namespace Mantid::RemoteAlgorithms;

class Logout2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Logout2Test *createSuite() { return new Logout2Test(); }
  static void destroySuite(Logout2Test *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create("Logout", 2);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "Logout");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<Logout2> a;
    TS_ASSERT(a = boost::make_shared<Logout2>());
    // can cast to inherited interfaces and base classes

    TS_ASSERT(dynamic_cast<Mantid::RemoteAlgorithms::Logout2 *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(a.get()));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    Logout2 lo;
    TS_ASSERT_THROWS_NOTHING(lo.initialize());
  }

  void test_propertiesMissing() {
    Logout2 lo;
    TS_ASSERT_THROWS_NOTHING(lo.initialize());
    // username missing
    TS_ASSERT_THROWS(lo.setPropertyValue("ComputeResource", "anything"),
                     const std::invalid_argument &);

    TS_ASSERT_THROWS(lo.execute(), const std::runtime_error &);
    TS_ASSERT(!lo.isExecuted());
  }

  void test_wrongProperty() {
    Logout2 lo;
    TS_ASSERT_THROWS_NOTHING(lo.initialize());
    TS_ASSERT_THROWS(lo.setPropertyValue("usernam", "anything"),
                     const std::runtime_error &);
  }

  void test_runOK() {
    testFacilities.emplace_back("SNS", "Fermi");

    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();
    // test that job managers are created correctly for different facilities
    for (auto &testFacility : testFacilities) {
      const std::string facName = testFacility.first;
      const std::string compName = testFacility.second;

      Mantid::Kernel::ConfigService::Instance().setFacility(facName);
      Logout2 lo;
      TS_ASSERT_THROWS_NOTHING(lo.initialize());
      TS_ASSERT_THROWS_NOTHING(
          lo.setPropertyValue("ComputeResource", compName));
      // Note: this would run the algorithm and could do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(lo.execute(), std::exception);
      TS_ASSERT(!lo.isExecuted());
    }
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

private:
  Mantid::API::IAlgorithm_sptr testAlg;
  std::vector<std::pair<std::string, std::string>> testFacilities;
};

#endif // MANTID_REMOTEALGORITHMS_LOGOUT2TEST_H_
