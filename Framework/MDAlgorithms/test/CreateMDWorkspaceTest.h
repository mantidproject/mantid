// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;

class CreateMDWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CreateMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_default_properties() {
    TS_ASSERT(FrameworkManager::Instance()
                  .exec("CreateMDWorkspace", 10, "OutputWorkspace", "simple_md",
                        "Dimensions", "3", "Extents", "-1,1,-2,2,3,3", "Names",
                        "One,Two,Three", "Units", "One,Two,Three")
                  ->isExecuted());
  }

  /** Validate bad inputs. */
  void test_validation() {
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 4, "OutputWorkspace",
                         "failed_output", "Dimensions", "0")
                   ->isExecuted());
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 6, "OutputWorkspace",
                         "failed_output", "Dimensions", "3", "Extents",
                         "-1,1,-2,2")
                   ->isExecuted());
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 6, "OutputWorkspace",
                         "failed_output", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3,4,4")
                   ->isExecuted());
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 8, "OutputWorkspace",
                         "failed_output", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3", "Names", "One,Two")
                   ->isExecuted());
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 12, "OutputWorkspace",
                         "failed_output", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3", "Names", "One,Two,Three",
                         "MinRecursionDepth", "5", "MaxRecursionDepth", "4")
                   ->isExecuted());

    // Wrong Frame-type input
    TS_ASSERT_THROWS(FrameworkManager::Instance().exec(
                         "CreateMDWorkspace", 12, "OutputWorkspace",
                         "simple_md", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3", "Names", "One,Two,Three", "Units",
                         "One,Two,Three", "Frames", "QSample, QTest, QSample"),
                     const std::runtime_error &);

    // Wrong number of frames
    TS_ASSERT_THROWS(!FrameworkManager::Instance().exec(
                         "CreateMDWorkspace", 12, "OutputWorkspace",
                         "simple_md", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3", "Names", "One,Two,Three", "Units",
                         "One,Two,Three", "Frames", "QSample, QSample"),
                     const std::runtime_error &);

    // Uses too much memory
    TS_ASSERT(!FrameworkManager::Instance()
                   .exec("CreateMDWorkspace", 14, "OutputWorkspace",
                         "failed_output", "Dimensions", "3", "Extents",
                         "-1,1,-2,2,3,3", "Names", "One,Two,Three", "Units",
                         "One,Two,Three", "SplitInto", "10",
                         "MinRecursionDepth", "5", "MaxRecursionDepth", "5")
                   ->isExecuted());
  }

  void do_test_exec(std::string Filename, bool lean, int MinRecursionDepth = 0,
                    int expectedNumMDBoxes = 216, bool withFrames = false) {

    std::string wsName = "CreateMDWorkspaceTest_out";
    CreateMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Dimensions", "3");
    alg.setPropertyValue("EventType", lean ? "MDLeanEvent" : "MDEvent");
    alg.setPropertyValue("Extents", "-1,1,-2,2,-3,3");
    alg.setPropertyValue("Names", "x,y,z");
    alg.setPropertyValue("Units", "m,mm,um");
    alg.setPropertyValue("SplitInto", "6");
    alg.setPropertyValue("SplitThreshold", "500");
    alg.setProperty("MinRecursionDepth", MinRecursionDepth);
    alg.setProperty("MaxRecursionDepth", 7);
    alg.setPropertyValue("OutputWorkspace", wsName);
    alg.setPropertyValue("Filename", Filename);
    alg.setPropertyValue("Memory", "1");
    if (withFrames) {
      alg.setPropertyValue("Frames", "QSample, QSample, QSample");
    }

    std::string fullName = alg.getPropertyValue("Filename");
    if (fullName != "")
      if (Poco::File(fullName).exists())
        Poco::File(fullName).remove();

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get it from data service
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve(wsName)));
    TS_ASSERT(ws);

    // Correct info?
    TS_ASSERT_EQUALS(ws->getNumDims(), 3);
    TS_ASSERT_EQUALS(ws->getNPoints(), 0);

    IMDDimension_const_sptr dim;
    dim = ws->getDimension(0);
    TS_ASSERT_DELTA(dim->getMaximum(), 1.0, 1e-6);
    TS_ASSERT_EQUALS(dim->getName(), "x");
    TS_ASSERT_EQUALS(dim->getUnits(), "m");
    dim = ws->getDimension(1);
    TS_ASSERT_DELTA(dim->getMaximum(), 2.0, 1e-6);
    TS_ASSERT_EQUALS(dim->getName(), "y");
    TS_ASSERT_EQUALS(dim->getUnits(), "mm");
    dim = ws->getDimension(2);
    TS_ASSERT_DELTA(dim->getMaximum(), 3.0, 1e-6);
    TS_ASSERT_EQUALS(dim->getName(), "z");
    TS_ASSERT_EQUALS(dim->getUnits(), "um");

    // What about the box controller
    BoxController_sptr bc;

    if (lean) {
      MDEventWorkspace3Lean::sptr ews =
          boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(ws);
      TS_ASSERT(ews);
      if (!ews)
        return;
      bc = ews->getBoxController();
    } else {
      MDEventWorkspace3::sptr ews =
          boost::dynamic_pointer_cast<MDEventWorkspace3>(ws);
      TS_ASSERT(ews);
      if (!ews)
        return;
      bc = ews->getBoxController();
    }

    TS_ASSERT(bc);
    if (!bc)
      return;
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 6);
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 500);
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 7);

    TS_ASSERT_EQUALS(bc->getTotalNumMDBoxes(), expectedNumMDBoxes);

    if (Filename != "") {
      std::string s = alg.getPropertyValue("Filename");
      TSM_ASSERT("File for the back-end was created.", Poco::File(s).exists());
      std::cout << "Closing the file.\n";

      ws->clearFileBacked(false);
      if (Poco::File(s).exists())
        Poco::File(s).remove();
    }

    // Test the frame type
    if (withFrames) {
      for (size_t dim = 0; dim < ws->getNumDims(); ++dim) {
        const auto &frame = ws->getDimension(dim)->getMDFrame();
        TSM_ASSERT_EQUALS("Should be convertible to a QSample frame",
                          Mantid::Geometry::QSample::QSampleName, frame.name());
      }
    } else {
      for (size_t dim = 0; dim < ws->getNumDims(); ++dim) {
        const auto &frame = ws->getDimension(dim)->getMDFrame();
        TSM_ASSERT_EQUALS("Should be convertible to a General frame",
                          Mantid::Geometry::GeneralFrame::GeneralFrameName,
                          frame.name());
      }
    }
  }

  void test_exec_MDEvent() { do_test_exec("", false); }

  void test_exec_MDEvent_fileBacked() {
    do_test_exec("CreateMDWorkspaceTest.nxs", false);
  }

  void test_exec_MDLeanEvent() { do_test_exec("", true); }

  void test_exec_MDLeanEvent_fileBacked() {
    do_test_exec("CreateMDWorkspaceTest.nxs", true);
  }

  void test_exec_MinRecursionDepth() { do_test_exec("", true, 2, 216 * 216); }
};

#endif /* MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_ */
