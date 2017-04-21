#ifndef LOADVTK_TEST_H_
#define LOADVTK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/LoadVTK.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;

class LoadVTKTest : public CxxTest::TestSuite {
public:
  void test_catagory() {
    LoadVTK loadVTK;
    Algorithm &alg = loadVTK;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  void test_version() {
    LoadVTK loadVTK;
    TS_ASSERT_EQUALS(1, loadVTK.version());
  }

  void test_Properties() {
    LoadVTK loadVTK;
    loadVTK.initialize();
    TS_ASSERT_THROWS_NOTHING(
        loadVTK.setPropertyValue("Filename", "iron_protein.vtk"));
    TS_ASSERT_THROWS_NOTHING(
        loadVTK.setPropertyValue("OutputWorkspace", "OutWS"));
    TS_ASSERT_THROWS_NOTHING(
        loadVTK.setPropertyValue("SignalArrayName", "scalars"));
    TS_ASSERT(loadVTK.isInitialized())
  }

  void do_check_dimension(IMDDimension_const_sptr dimension,
                          const std::string &expectedName,
                          const double expectedMin, const double expectedMax,
                          const int expectedNBins) {
    TSM_ASSERT_EQUALS("Name is wrong.", dimension->getName(), expectedName);
    TSM_ASSERT_EQUALS("Id is wrong.", dimension->getDimensionId(),
                      expectedName);
    TSM_ASSERT_DELTA("Minimum is wrong.", dimension->getMinimum(), expectedMin,
                     0.01);
    TSM_ASSERT_DELTA("Maximum is wrong.", dimension->getMaximum(), expectedMax,
                     0.01);
    TSM_ASSERT_EQUALS("Number of bins is wrong.", dimension->getNBins(),
                      expectedNBins);
  }

  void do_test_bad_arrays(const std::string &signalArrayName,
                          const std::string &errorSQArrayName = "") {
    LoadVTK loadVTK;
    loadVTK.setRethrows(true);
    loadVTK.initialize();
    loadVTK.setPropertyValue("Filename", "iron_protein.vtk");
    loadVTK.setPropertyValue("OutputWorkspace", "OutWS");
    loadVTK.setPropertyValue("SignalArrayName", signalArrayName);
    loadVTK.setPropertyValue("ErrorSQArrayName", errorSQArrayName);
    TS_ASSERT_THROWS(loadVTK.execute(), std::invalid_argument &);
  }

  void test_bad_signal_array() {
    const std::string signalArray = "?!"; // Not a name that exists.
    do_test_bad_arrays(signalArray);
  }

  void test_bad_errorSQ_array() {
    const std::string signalArray = "scalar_array"; // Does exist
    const std::string errorSQArray = "?!";          // Not a name that exists.
    do_test_bad_arrays(signalArray, errorSQArray);
  }

  void test_load_VTK_file_as_histo() {
    const std::string outWSName = "OutWS";

    LoadVTK loadVTK;
    loadVTK.setRethrows(true);
    loadVTK.initialize();
    loadVTK.setPropertyValue("Filename", "iron_protein.vtk");
    loadVTK.setPropertyValue("OutputWorkspace", outWSName);
    loadVTK.setPropertyValue("SignalArrayName", "scalar_array");
    loadVTK.setPropertyValue("ErrorSQArrayName", "scalar_array");
    loadVTK.setProperty("AdaptiveBinned", false);
    loadVTK.execute();

    IMDHistoWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
            outWSName);

    TS_ASSERT_EQUALS(3, outWS->getNumDims());
    do_check_dimension(outWS->getDimension(0), "X", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim x.
    do_check_dimension(outWS->getDimension(1), "Y", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim y.
    do_check_dimension(outWS->getDimension(2), "Z", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim z.

    // Quick check of loaded data.
    TS_ASSERT_EQUALS(0, outWS->getSignalAt(0));
    TS_ASSERT_EQUALS(1, outWS->getSignalAt(1));
    TS_ASSERT_EQUALS(2, outWS->getSignalAt(2));
    TS_ASSERT_EQUALS(3, outWS->getSignalAt(3));
  }

  void test_KeepTopPercent_bounds() {
    LoadVTK loadVTK;
    loadVTK.initialize();
    loadVTK.setRethrows(true);

    TSM_ASSERT_THROWS_NOTHING("Within bounds",
                              loadVTK.setProperty("KeepTopPercent", 1.0));
    TSM_ASSERT_THROWS("Too low", loadVTK.setProperty("KeepTopPercent", -0.01),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Too high", loadVTK.setProperty("KeepTopPercent", 100.01),
                      std::invalid_argument &);
  }

  void test_load_vtk_file_as_mdevent() {
    const std::string outWSName = "OutWS";

    LoadVTK loadVTK;
    loadVTK.setRethrows(true);
    loadVTK.initialize();
    loadVTK.setProperty("AdaptiveBinned", true);
    loadVTK.setPropertyValue("Filename", "iron_protein.vtk");
    loadVTK.setPropertyValue("OutputWorkspace", outWSName);
    loadVTK.setPropertyValue("SignalArrayName", "scalar_array");
    loadVTK.setPropertyValue("ErrorSQArrayName", "scalar_array");
    loadVTK.execute();

    IMDEventWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName);

    TS_ASSERT_EQUALS(3, outWS->getNumDims());
    do_check_dimension(outWS->getDimension(0), "X", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim x.
    do_check_dimension(outWS->getDimension(1), "Y", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim y.
    do_check_dimension(outWS->getDimension(2), "Z", 0, 67,
                       68); // These numbers are expected min, max, and nbins
                            // known from the input file for dim z.
    TSM_ASSERT_EQUALS("Should be an UnknownFrame",
                      Mantid::Geometry::UnknownFrame::UnknownFrameName,
                      outWS->getDimension(0)->getMDFrame().name());
    TSM_ASSERT_EQUALS("Should be an UnknownFrame",
                      Mantid::Geometry::UnknownFrame::UnknownFrameName,
                      outWS->getDimension(1)->getMDFrame().name());
    TSM_ASSERT_EQUALS("Should be an UnknownFrame",
                      Mantid::Geometry::UnknownFrame::UnknownFrameName,
                      outWS->getDimension(2)->getMDFrame().name());

    double topPercent = loadVTK.getProperty("KeepTopPercent");
    TSM_ASSERT_EQUALS("Should default to 25%", 25, topPercent);

    const int expectedSignalMax = 9999; // Known from file
    const int expectedSignalMin = 0;    // Known from file
    const int expectedSignalThreshold =
        static_cast<int>((1 - 0.25) * (expectedSignalMax - expectedSignalMin) +
                         expectedSignalMin);
    const int actualSignalMin = loadVTK.getProperty("SignalMinimum");
    const int actualSignalMax = loadVTK.getProperty("SignalMaximum");
    const int actualSignalThreshold = loadVTK.getProperty("SignalThreshold");
    TS_ASSERT_EQUALS(expectedSignalMin, actualSignalMin);
    TS_ASSERT_EQUALS(expectedSignalMax, actualSignalMax);
    TS_ASSERT_EQUALS(expectedSignalThreshold, actualSignalThreshold);

    TS_ASSERT(outWS->getNEvents() > 0);
  }

  void test_dynamic_load() {
    const std::string outWSName = "OutWS";
    auto alg = AlgorithmManager::Instance().create("Load");
    alg->setRethrows(true);
    alg->initialize();
    alg->setPropertyValue("Filename", "iron_protein.vtk");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setPropertyValue("SignalArrayName", "scalar_array");
    alg->setPropertyValue("ErrorSQArrayName", "scalar_array");
    alg->setProperty("AdaptiveBinned", false);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
            outWSName));
  }
};

#endif
