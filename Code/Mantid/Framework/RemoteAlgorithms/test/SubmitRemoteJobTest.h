#ifndef MANTID_REMOTEALGORITHMS_SUBMITREMOTEJOBTEST_H_
#define MANTID_REMOTEALGORITHMS_SUBMITREMOTEJOBTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteAlgorithms/SubmitRemoteJob.h"

using namespace Mantid::RemoteAlgorithms;

class SubmitRemoteJobTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SubmitRemoteJobTest *createSuite() {
    return new SubmitRemoteJobTest();
  }
  static void destroySuite(SubmitRemoteJobTest *suite) { delete suite; }

  void test_algorithm() {
    testAlg = Mantid::API::AlgorithmManager::Instance().create(
        "SubmitRemoteJob" /*, 1*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), "SubmitRemoteJob");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<SubmitRemoteJob> a;
    TS_ASSERT(a = boost::make_shared<SubmitRemoteJob>());

    // can cast to inherited interfaces and base classes
    SubmitRemoteJob alg;
    TS_ASSERT(dynamic_cast<Mantid::RemoteAlgorithms::SubmitRemoteJob *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(&alg));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(&alg));
  }

  void test_init() {
    if (!testAlg->isInitialized())
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());

    TS_ASSERT(testAlg->isInitialized());

    SubmitRemoteJob s;
    TS_ASSERT_THROWS_NOTHING(s.initialize());
  }

  // TODO: when we have a RemoteJobManager capable of creating
  // algorithms for different types of compute resources (example:
  // Fermi@SNS and SCARF@STFC), create different algorithms for them
  void test_propertiesMissing() {
    SubmitRemoteJob alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    // Transaction id missing
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("NumNodes", "1"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("CoresPerNode", "4"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("TaskName", "unit test"));
    TS_ASSERT_THROWS_NOTHING(
        alg1.setPropertyValue("ScriptName", "test script"));
    TS_ASSERT_THROWS_NOTHING(
        alg1.setPropertyValue("PythonScript", "print 'hello world'"));
    TS_ASSERT_THROWS(alg1.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg1.execute(), std::runtime_error);
    TS_ASSERT(!alg1.isExecuted());

    SubmitRemoteJob alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    // task name name missing
    TS_ASSERT_THROWS(alg2.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("ScriptName", "test script"));
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("PythonScript", "print 'hello world'"));

    TS_ASSERT_THROWS(alg2.execute(), std::runtime_error);
    TS_ASSERT(!alg2.isExecuted());

    SubmitRemoteJob alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    // script name name missing
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("TaskName", "unit test"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("PythonScript", "print 'hello world'"));
    TS_ASSERT_THROWS(alg3.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg3.execute(), std::runtime_error);
    TS_ASSERT(!alg3.isExecuted());

    SubmitRemoteJob alg4;
    TS_ASSERT_THROWS_NOTHING(alg4.initialize());
    // compute resource missing
    TS_ASSERT_THROWS_NOTHING(alg4.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(alg4.setPropertyValue("TaskName", "unit test"));
    TS_ASSERT_THROWS_NOTHING(
        alg4.setPropertyValue("ScriptName", "test script"));
    TS_ASSERT_THROWS_NOTHING(
        alg4.setPropertyValue("PythonScript", "print 'hello world'"));

    TS_ASSERT_THROWS(alg4.execute(), std::runtime_error);
    TS_ASSERT(!alg4.isExecuted());

    SubmitRemoteJob alg5;
    TS_ASSERT_THROWS_NOTHING(alg5.initialize());
    // py script missing
    TS_ASSERT_THROWS_NOTHING(alg5.setPropertyValue("TransactionID", "id001"));
    TS_ASSERT_THROWS_NOTHING(alg5.setPropertyValue("TaskName", "unit test"));
    TS_ASSERT_THROWS_NOTHING(
        alg5.setPropertyValue("ScriptName", "test script"));
    TS_ASSERT_THROWS(alg5.setPropertyValue("ComputeResource", "missing!"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg5.execute(), std::runtime_error);
    TS_ASSERT(!alg5.isExecuted());
  }

  void test_wrongProperty() {
    SubmitRemoteJob s;
    TS_ASSERT_THROWS_NOTHING(s.initialize();)
    TS_ASSERT_THROWS(s.setPropertyValue("Compute", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("NumNodes", "anything"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(s.setPropertyValue("NumNodes", "-3"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(s.setPropertyValue("CoresPerNode", "anything"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(s.setPropertyValue("Task", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("Name", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("Transaction", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("ID", "anything"), std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("ScriptName", ""),
                     std::invalid_argument);
    TS_ASSERT_THROWS(s.setPropertyValue("Scrip", "any name"),
                     std::runtime_error);
    TS_ASSERT_THROWS(s.setPropertyValue("PythonScript", ""),
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

      SubmitRemoteJob s;
      TS_ASSERT_THROWS_NOTHING(s.initialize());
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("ComputeResource", compName));
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("NumNodes", "1"));
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("CoresPerNode", "4"));
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("TaskName", "unit test"));
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("TransactionID", "tr001"));
      TS_ASSERT_THROWS_NOTHING(s.setPropertyValue("ScriptName", "test script"));
      TS_ASSERT_THROWS_NOTHING(
          s.setPropertyValue("PythonScript", "print 'hello world'"));
      // TODO: this would run the algorithm and do a remote
      // connection. uncomment only when/if we have a mock up for this
      // TS_ASSERT_THROWS(s.execute(), std::exception);
      TS_ASSERT(!s.isExecuted());
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

#endif // MANTID_REMOTEALGORITHMS_SUBMITREMOTEJOBTEST_H_
