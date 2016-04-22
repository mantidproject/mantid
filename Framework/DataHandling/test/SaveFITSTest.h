#ifndef MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_
#define MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveFITS.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataHandling::SaveFITS;

// This algorithm just saves a file. This test just saves a toy
// workspace to avoid slow I/O in unit tests. The doc test checks a
// load / save / load cycle with more realistic data/images.
class SaveFITSTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveFITSTest *createSuite() { return new SaveFITSTest(); }

  static void destroySuite(SaveFITSTest *suite) { delete suite; }

  void test_init() {
    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int bits TS_ASSERT_THROWS_NOTHING(numProjs = alg.getProperty("BitDepth"));
    TS_ASSERT_EQUALS(bits, 16);
  }

  void test_errors_options() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveFITS");

    TS_ASSERT_THROWS(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"),
        Mantid::Kernel::Exception::NotFoundError);

    TS_ASSERT_THROWS(
        alg->setPropertyValue("BitDepth", "this_is_wrong_you_must_fail"),
        std::invalid_argument);
  }

  void test_exec_fail() {
    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "foo.fits"));
    TS_ASSERT_THROWS(
        alg.setPropertyValue("InputWorkspace", "inexistent_workspace_fails"),
        std::invalid_argument);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_runs_ok() {
    const std::string filename = "savefits_simple_test.fits";

    auto ws = WorkspaceCreationHelper::Create2DWorkspace(2, 2);

    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS(alg.setPropertyValue("InputWorkspace", ws),
                     std::invalid_argument);

    TSM_ASSERT_THROWS_NOTHING("The algorithm should execute and save a file "
                              "without any error/exceptions",
                              alg.execute());
    TS_ASSERT(alg.isExecuted());

    Poco::File saved(filename);
    TSM_ASSERT("The saved file should be found on disk", saved.exists());
    TSM_ASSERT("The saved file should be readable", saved.canRead());
    TSM_ASSERT_EQUALS("The size of the file should be as expected",
                      saved.getSize(), 256);
    TSM_ASSERT(
        "It should be possible to remove the file saved by the algorithm",
        saved.remove());
  }
};

#endif /* MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_ */