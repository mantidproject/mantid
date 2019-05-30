// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEFITSTEST_H_
#define MANTID_DATAHANDLING_SAVEFITSTEST_H_

#include "MantidDataHandling/SaveFITS.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

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

    int bits;
    TS_ASSERT_THROWS_NOTHING(bits = alg.getProperty("BitDepth"));
    TS_ASSERT_EQUALS(bits, 16);
  }

  void test_errors_options() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveFITS");

    TS_ASSERT_THROWS(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"),
        const Mantid::Kernel::Exception::NotFoundError &);

    TS_ASSERT_THROWS(
        alg->setPropertyValue("BitDepth", "this_is_wrong_you_must_fail"),
        const std::invalid_argument &);

    TS_ASSERT_THROWS(alg->setProperty("BitDepth", 10),
                     const std::invalid_argument &);

    TS_ASSERT_THROWS(alg->setProperty("BitDepth", 64),
                     const std::invalid_argument &);
  }

  void test_exec_fail() {
    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "foo.fits"));
    TS_ASSERT_THROWS(
        alg.setPropertyValue("InputWorkspace", "inexistent_workspace_fails"),
        const std::invalid_argument &);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_fails_units() {
    const std::string filename = "./savefits_wont_work.fits";

    auto ws = WorkspaceCreationHelper::create2DWorkspace(2, 2);

    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));

    TSM_ASSERT_THROWS(
        "The algorithm should not accept workspaces if the units are wrong",
        alg.setProperty("InputWorkspace", ws), const std::invalid_argument &);
  }

  void test_exec_fails_empty() {
    const std::string filename = "./savefits_wont_work.fits";

    // auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();

    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));

    TSM_ASSERT_THROWS(
        "The algorithm should not accept empty / uninitialized workspaces",
        alg.setProperty("InputWorkspace", ws), const std::invalid_argument &);
  }

  void test_exec_runs_ok() {
    const std::string filename = "./savefits_simple_test.fits";

    // create with appropriate units
    auto ws = WorkspaceCreationHelper::create2DWorkspace(2, 2);
    auto lbl = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
        Mantid::Kernel::UnitFactory::Instance().create("Label"));
    lbl->setLabel("width", "cm");
    ws->getAxis(0)->unit() = lbl;

    lbl = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
        Mantid::Kernel::UnitFactory::Instance().create("Label"));
    lbl->setLabel("height", "cm");
    ws->getAxis(1)->unit() = lbl;

    SaveFITS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));

    TSM_ASSERT_THROWS_NOTHING("The algorithm should execute and save a file "
                              "without any error/exceptions",
                              alg.execute());
    TS_ASSERT(alg.isExecuted());

    Poco::File saved(filename);
    TSM_ASSERT("The saved file should be found on disk", saved.exists());
    TSM_ASSERT("The saved file should be a regular file", saved.isFile());
    TSM_ASSERT("The saved file should be readable", saved.canRead());
    TSM_ASSERT_EQUALS("The size of the file should be as expected",
                      saved.getSize(), 2888);
    TSM_ASSERT_THROWS_NOTHING(
        "It should be possible to remove the file saved by the algorithm",
        saved.remove());
  }
};

#endif /* MANTID_DATAHANDLING_SAVEFITSTEST_H_ */
