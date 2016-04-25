#ifndef MANTID_DATAHANDLING_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_
#define MANTID_DATAHANDLING_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ImggTomographicReconstruction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>

using Mantid::Algorithms::ImggTomographicReconstruction;

class ImggTomographicReconstructionTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImggTomographicReconstructionTest *createSuite() {
    return new ImggTomographicReconstructionTest();
  }

  static void destroySuite(ImggTomographicReconstructionTest *suite) {
    delete suite;
  }

  void test_init() {
    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    double relax;
    TS_ASSERT_THROWS_NOTHING(relax = alg.getProperty("RelaxationParameter"));
    TS_ASSERT_EQUALS(relax, 0.5);
  }

  void test_errors_options() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create(
        "ImggTomographicReconstruction");

    TS_ASSERT_THROWS(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"),
        Mantid::Kernel::Exception::NotFoundError);

    TS_ASSERT_THROWS(
        alg->setPropertyValue("BitDepth", "this_is_wrong_you_must_fail"),
        std::invalid_argument);
  }

  void test_exec_fail() {
    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS(
        alg.setPropertyValue("InputWorkspace", "inexistent_workspace_fails"),
        std::invalid_argument);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_fails_some_condition() {}

  void test_exec_runs_ok() {}
};

#endif /* MANTID_DATAHANDLING_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_ */