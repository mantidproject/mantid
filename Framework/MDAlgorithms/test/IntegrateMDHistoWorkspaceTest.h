#ifndef MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_
#define MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/IntegrateMDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
using Mantid::MDAlgorithms::IntegrateMDHistoWorkspace;
using namespace Mantid::API;

namespace {

// This helper sets the signal values to the linear index to have some
// variety
void resetSignalsToLinearIndexValue(IMDHistoWorkspace_sptr ws) {
  auto numberOfIndices = static_cast<size_t>(ws->getNPoints());
  for (size_t i = 0; i < numberOfIndices; ++i) {
    auto &signal = ws->signalAt(i);
    signal = static_cast<Mantid::signal_t>(i);
  }
}
} // namespace

//=====================================================================================
// Functional Tests
//=====================================================================================
class IntegrateMDHistoWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateMDHistoWorkspaceTest *createSuite() {
    return new IntegrateMDHistoWorkspaceTest();
  }
  static void destroySuite(IntegrateMDHistoWorkspaceTest *suite) {
    delete suite;
  }

  void test_Init() {
    IntegrateMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_throw_if_new_steps_in_binning() {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double step = 0.1;
    std::vector<double> p1BinVec = {(0.0), (step), (1.0)};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("No new steps allowed", alg.execute(),
                      std::runtime_error &);
  }

  void test_throw_if_incorrect_binning_limits_when_integrating() {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");

    const double min = 3;

    // Test equal to
    double max = min;
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error &);

    // Test less than
    max = min - 0.01;
    p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error &);
  }

  void test_throw_if_incorrect_binning_limits_when_similar() {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");

    const double min = 3;
    double step = 0;
    // Test equal to
    double max = min;
    std::vector<double> p1BinVec = {min, step, max};
    alg.setProperty("P1Bin", p1BinVec);
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error &);

    // Test less than
    max = min - 0.01;
    p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error &);

    // Test non-zero step. ZERO means copy!
    max = min - 0.01;
    p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Step has been specified", alg.execute(),
                      std::runtime_error &);
  }

  // Users may set all binning parameter to [] i.e. direct copy, no integration.
  void test_exec_do_nothing_but_clone() {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);
    auto histNorm = Mantid::API::MDNormalization::NumEventsNormalization;
    ws->setDisplayNormalization(histNorm);
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to be a copy of input.
    TS_ASSERT_EQUALS(outWS->getNPoints(), ws->getNPoints());
    TS_ASSERT_EQUALS(outWS->getNumDims(), ws->getNumDims());
    TS_ASSERT_EQUALS(outWS->getSignalAt(0), ws->getSignalAt(0));
    TS_ASSERT_EQUALS(outWS->getSignalAt(1), ws->getSignalAt(1));
    TSM_ASSERT_EQUALS("Should have a num events normalization",
                      outWS->displayNormalization(), histNorm);
  }

  void test_1D_integration_exact_binning() {

    /*

                         input
    (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
              1  1  1  1  1  1  1  1  1  1

                output requested

    (x = 0) *|--------------|* (x = 5)
              1 + 1 + 1 + 1 + 1 = 5 counts

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
    auto histNorm = Mantid::API::MDNormalization::NumEventsNormalization;
    ws->setDisplayNormalization(histNorm);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 0;
    const double max = 5;
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
    auto dim = outWS->getDimension(0);
    TS_ASSERT_EQUALS(min, dim->getMinimum());
    TS_ASSERT_EQUALS(max, dim->getMaximum());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong integrated value", 5.0, outWS->getSignalAt(0),
                     1e-4);
    TSM_ASSERT_DELTA("Wrong error value",
                     std::sqrt(5 * (ws->getErrorAt(0) * ws->getErrorAt(0))),
                     outWS->getErrorAt(0), 1e-4);
    TSM_ASSERT_EQUALS("Should have a num events normalization",
                      outWS->displayNormalization(), histNorm);
  }

  void test_1D_integration_partial_binning_complex() {

    /*

                         input
    (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
              1  1  1  1  1  1  1  1  1  1

                output requested

    (x = 0.75) *|--------------|* (x = 4.25)
              1/4 + 1 + 1 + 1 + 1/4 = 3.5 counts

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 0.75;
    const double max = 4.25;
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
    auto dim = outWS->getDimension(0);
    TS_ASSERT_EQUALS(min, dim->getMinimum());
    TS_ASSERT_EQUALS(max, dim->getMaximum());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong integrated value", 3.5, outWS->getSignalAt(0),
                     1e-4);
    TSM_ASSERT_DELTA("Wrong error value",
                     std::sqrt(3.5 * (ws->getErrorAt(0) * ws->getErrorAt(0))),
                     outWS->getErrorAt(0), 1e-4);
  }

  void test_1D_integration_with_original_step_and_forbidden_partial_binning() {

    /*

                         input
    (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
              1  1  1  1  1  1  1  1  1  1

                output requested, but partial bins are forbidden so round to the
    nearest bin edges

    (x = 0.75) *|--------------|* (x = 4.25)
              1/4 , 1 , 1 , 1 , 1/4

                output with rounding (maintain closest possible bin boundaries.
    no partial binning)

    (x = 0) *|--------------------|* (x = 5)
               1 , 1 , 1 , 1 , 1, 1

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 0.75;
    const double max = 4.25;
    std::vector<double> p1BinVec = {min, 0.0, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("Should have rounded to whole widths.", 5,
                      outWS->getNPoints());
    auto outDim = outWS->getDimension(0);
    auto inDim = ws->getDimension(0);
    TS_ASSERT_EQUALS(0.0f, outDim->getMinimum());
    TS_ASSERT_EQUALS(5.0f, outDim->getMaximum());
    TSM_ASSERT_EQUALS("Bin width should be unchanged", inDim->getBinWidth(),
                      outDim->getBinWidth());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(0), 1e-4);
    TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(1), 1e-4);
    TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(2), 1e-4);
    TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(3), 1e-4);
    TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(4), 1e-4);
  }

  void test_1D_integration_exact_binning_with_mask() {

    /*
     * Test that masked values do not contribute to integral

                         input
    (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
              1  1  1  1  1  1  1  1  1  1

                output requested

    (x = 0) *|--------------|* (x = 5)
              1 + 1 + masked + masked + 1 = 3 counts

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
    auto histNorm = Mantid::API::MDNormalization::NumEventsNormalization;
    ws->setDisplayNormalization(histNorm);

    ws->setMDMaskAt(2, true);
    ws->setMDMaskAt(3, true);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 0;
    const double max = 5;
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
    auto dim = outWS->getDimension(0);
    TS_ASSERT_EQUALS(min, dim->getMinimum());
    TS_ASSERT_EQUALS(max, dim->getMaximum());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong integrated value", 3.0, outWS->getSignalAt(0),
                     1e-4);
    TSM_ASSERT_DELTA("Wrong error value",
                     std::sqrt(3 * (ws->getErrorAt(0) * ws->getErrorAt(0))),
                     outWS->getErrorAt(0), 1e-4);
    TSM_ASSERT_EQUALS("Should have a num events normalization",
                      outWS->displayNormalization(), histNorm);
  }

  void test_2d_partial_binning() {

    /*

      Input filled with 1's binning = 1 in each dimension
      ----------------------------- (10, 10)
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      -----------------------------
    (0, 0)


      Slice. Two vertical columns. Each 1 in width.

      ----------------------------- (10, 10)
      |                           |
      |                           |
      |__________________________ | (10, 7.1)
      |    |    |   ...           |
      |    |    |                 |
      |    |    |                 |
      |    |    |                 |
      |    |    |                 |
      |__________________________ | (10, 1.1)
      |                           |
      -----------------------------
    (0, 0)

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 2 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 1.1;
    const double max = 7.1; // 7.1 - 1.1 = 6
    alg.setProperty("P1Bin", std::vector<double>(
                                 0)); // Pass through. Do not change binning.
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P2Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS(
        "All integrated", 10,
        outWS->getNPoints()); // one dimension unchanged the other integrated
    auto intdim = outWS->getDimension(1);
    TS_ASSERT_DELTA(min, intdim->getMinimum(), 1e-4);
    TS_ASSERT_DELTA(max, intdim->getMaximum(), 1e-4);
    TS_ASSERT_EQUALS(1, intdim->getNBins());
    auto dim = outWS->getDimension(0);
    TSM_ASSERT_DELTA(
        "Not integrated binning should be the same as the original dimension",
        0, dim->getMinimum(), 1e-4);
    TSM_ASSERT_DELTA(
        "Not integrated binning should be the same as the original dimension",
        10, dim->getMaximum(), 1e-4);
    TSM_ASSERT_EQUALS(
        "Not integrated binning should be the same as the original dimension",
        10, dim->getNBins());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong integrated value", 6.0, outWS->getSignalAt(0),
                     1e-4);
    TSM_ASSERT_DELTA("Wrong error value",
                     std::sqrt(6.0 * (ws->getErrorAt(0) * ws->getErrorAt(0))),
                     outWS->getErrorAt(0), 1e-4);
  }

  void test_update_n_events_for_normalization() {

    /*
                  input
    (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
              1  2  3  4  5  6  7  8  9  10    (signal values in bins)
              1  2  3  4  5  6  7  8  9  10    (n_events in bins)

                output requested

    (x = 0.75) *|--------------|* (x = 4.25)
              1/4 , 1 , 1 , 1 , 1/4 = weights based on fraction overlap
              1/4 + 2 + 3 + 4 + 5/4  (signal values in bins)
              1/4 + 2 + 3 + 4 + 5/4  (n_events in bins)


    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
    // Fill signal and n-events as above
    for (size_t i = 0; i < ws->getNPoints(); ++i) {
      ws->setSignalAt(i, Mantid::signal_t(i + 1));
      ws->setNumEventsAt(i, Mantid::signal_t(i + 1));
      std::cout << "signal " << i + 1 << "\t"
                << "nevents"
                << "\t"
                << "at"
                << "\t" << i << '\n';
    }

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 0.75;
    const double max = 4.25;
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
    auto dim = outWS->getDimension(0);
    TS_ASSERT_EQUALS(min, dim->getMinimum());
    TS_ASSERT_EQUALS(max, dim->getMaximum());

    // Check the data. No accounting for normalization.
    TSM_ASSERT_DELTA("Wrong integrated value", 1.0 / 4 + 2 + 3 + 4 + 5.0 / 4,
                     outWS->getSignalAt(0), 1e-4);

    Mantid::coord_t point[1] = {3.0}; // Roughly centre of the single output bin
    TSM_ASSERT_DELTA(
        "Number of events normalization. Weights for n-events "
        "used incorrectly.",
        1.0,
        outWS->getSignalAtCoord(point, Mantid::API::NumEventsNormalization),
        1e-4);
  }

  //-----------------------------------------------
  // Snapping Tests
  //------------------------------------------------
  void
  test_that_PBin_on_boundaries_are_detected_for_asymmetric_workspace_with_no_bin_boundary_at_origin() {
    /**
     Input workspace: Is asymmetric about the origina and the origin does not
    lie on a bin boundary, but
     the PMIN and PMAX boundaries lie on bin boundaries.
     Bins: 6
     XMin: -2.4359
     XMAX: 2.1001

    XMIN                          XMAX
     |                             |
     |    |    |     |    |    |   |
          |            |       |
         PMIN          0      PMAX
    **/
    // Arrange
    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 3.4;
    const double errorSquared = 1.3;
    size_t numBins[static_cast<int>(numDims)] = {6};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-2.4359f};
    Mantid::coord_t max[static_cast<int>(numDims)] = {2.1001f};
    const std::string name("test");
    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    const double pMin = -1.6799;
    const double pMax = 1.3441;
    resetSignalsToLinearIndexValue(ws);

    // Act
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    std::vector<double> p1BinVec = {pMin, 0.0, pMax};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Assert
    // At this point we need to take into account the precision of a float value
    TSM_ASSERT_DELTA("Should have a lower bound set to exactly PMIN",
                     outWS->getDimension(0)->getMinimum(),
                     static_cast<Mantid::coord_t>(pMin), 1e-6);
    TSM_ASSERT_DELTA("Should have an upper bound set to exactly PMAX",
                     outWS->getDimension(0)->getMaximum(),
                     static_cast<Mantid::coord_t>(pMax), 1e-6);

    // Confirm that the old workspace has signals of 0,1,2,3,4,5
    TSM_ASSERT_DELTA("Should have a signal value of 0", ws->getSignalAt(0),
                     static_cast<Mantid::signal_t>(0.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 1", ws->getSignalAt(1),
                     static_cast<Mantid::signal_t>(1.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 2", ws->getSignalAt(2),
                     static_cast<Mantid::signal_t>(2.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 3", ws->getSignalAt(3),
                     static_cast<Mantid::signal_t>(3.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 4", ws->getSignalAt(4),
                     static_cast<Mantid::signal_t>(4.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 5", ws->getSignalAt(5),
                     static_cast<Mantid::signal_t>(5.0), 1e-4);

    // We expect that the signals have not chagned. The first bin and the last
    // bin of the original workspace were trimmed, hence
    // we expect to have signal values of 1, 2, 3, 4 for the new workspace
    TSM_ASSERT_DELTA("Should have a signal value of 1", outWS->getSignalAt(0),
                     static_cast<Mantid::signal_t>(1.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 2", outWS->getSignalAt(1),
                     static_cast<Mantid::signal_t>(2.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 3", outWS->getSignalAt(2),
                     static_cast<Mantid::signal_t>(3.0), 1e-4);
    TSM_ASSERT_DELTA("Should have a signal value of 4", outWS->getSignalAt(3),
                     static_cast<Mantid::signal_t>(4.0), 1e-4);
  }

  void
  test_that_PBin_between_extent_and_bin_boundary_floors_when_pmin_not_contained_in_threshold() {
    /*
    Input workspace: Is asymmetric about the origina and the origin does not lie
   on a bin boundary and
    the PMIN and PMAX do not lie on bin boundaries.PMin is defined to be at a
   greater distance than 1e-06 of a bin boundary
    Bins: 3
    XMin: -1
    XMAX: 1

   XMIN                             XMAX
    |                                |
    |          |          |          |
             |    |            |
            PMIN  0         PMAX

   where 0-PMin > threshold (1e-06)
   */
    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 3.5;
    const double errorSquared = 1.2;
    size_t numBins[static_cast<int>(numDims)] = {3};
    Mantid::coord_t min[static_cast<int>(numDims)];
    min[0] = -1.0f;
    Mantid::coord_t max[static_cast<int>(numDims)];
    max[0] = 1.0f;
    const std::string name("test");
    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    const double secondBinBoundary = -(1.0f) / 3;
    const double pMin = secondBinBoundary - 0.1; // outside of threshold
    const double pMax = 1.0;

    // Act
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    std::vector<double> p1BinVec = {pMin, 0.0, pMax};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Assert
    Mantid::coord_t minimumDim = outWS->getDimension(0)->getMinimum();
    Mantid::coord_t maximumDim = outWS->getDimension(0)->getMaximum();

    std::cerr << std::setprecision(10) << minimumDim << "   "
              << std::setprecision(10) << static_cast<Mantid::coord_t>(min[0])
              << "    " << std::setprecision(10) << min[0] << '\n';
    std::cerr << std::setprecision(10) << maximumDim << "   "
              << std::setprecision(10) << static_cast<Mantid::coord_t>(max[0])
              << "    " << std::setprecision(10) << max[0] << '\n';

    TSM_ASSERT_DELTA("Should snap to the second bin boundary.", minimumDim,
                     static_cast<Mantid::coord_t>(min[0]), 1e-6);
    TSM_ASSERT_DELTA("Should snap to the last bin boundary", maximumDim,
                     static_cast<Mantid::coord_t>(max[0]), 1e-6);
  }
  void
  test_that_PBin_between_extent_and_bin_boundary_does_snap_to_boundary_within_threshold() {
    /*
      Input workspace: Is asymmetric about the origina and the origin does not
     lie on a bin boundary and
      the PMIN and PMAX do not lie on bin boundaries. PMin is defined to be
     within 1e-06 of a bin boundary
      Bins: 3
      XMin: -1
      XMAX: 1

     XMIN                             XMAX
      |                                |
      |          |          |          |
              |    |            |
             PMIN  0         PMAX
      where 0-PMin < threshold (1e-06)
     */
    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 3.5;
    const double errorSquared = 1.2;
    size_t numBins[static_cast<int>(numDims)] = {3};
    Mantid::coord_t min[static_cast<int>(numDims)];
    min[0] = -1.0f;
    Mantid::coord_t max[static_cast<int>(numDims)];
    max[0] = 1.0f;
    const std::string name("test");
    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    const double secondBinBoundary = -(1.0f) / 3;
    const double pMin = secondBinBoundary - (1e-7); // within threshold
    const double pMax = 1.0;

    // Act
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    std::vector<double> p1BinVec = {pMin, 0.0, pMax};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Assert
    Mantid::coord_t minimumDim = outWS->getDimension(0)->getMinimum();
    Mantid::coord_t maximumDim = outWS->getDimension(0)->getMaximum();
    std::cerr << std::setprecision(10) << minimumDim << "   "
              << std::setprecision(10)
              << static_cast<Mantid::coord_t>(secondBinBoundary) << "    "
              << std::setprecision(10) << secondBinBoundary << '\n';
    std::cerr << std::setprecision(10) << maximumDim << "   "
              << std::setprecision(10) << static_cast<Mantid::coord_t>(max[0])
              << "    " << std::setprecision(10) << max[0] << '\n';

    TSM_ASSERT_DELTA("Should snap to the second bin boundary.", minimumDim,
                     static_cast<Mantid::coord_t>(secondBinBoundary), 1e-6);
    TSM_ASSERT_DELTA("Should snap to the last bin boundary", maximumDim,
                     static_cast<Mantid::coord_t>(max[0]), 1e-6);
  }

  void
  test_that_PBin_NOT_on_boundary_are_detected_for_asymmetric_workspace_with_no_bin_boundary_at_origin() {
    /**
     Input workspace: Is asymmetric about the origina and the origin does not
    lie on a bin boundary and
     the PMIN and PMAX do not lie on bin boundaries.
     Bins: 6
     XMin: -2.4359
     XMAX: 2.1001

    XMIN                          XMAX
     |                             |
     |    |    |     |    |    |   |
           |            |        |
          PMIN          0       PMAX
    **/
    // Arrange
    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 3.4;
    const double errorSquared = 1.3;
    size_t numBins[static_cast<int>(numDims)] = {6};
    Mantid::coord_t min[static_cast<int>(numDims)];
    min[0] = -2.4359f;
    Mantid::coord_t max[static_cast<int>(numDims)];
    max[0] = 2.1001f;
    const std::string name("test");
    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    const double secondBinBoundary = -1.6799;
    const double pMin = secondBinBoundary + 0.16729;
    const double pMax = 1.3441 + 0.08792;

    // Act
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    std::vector<double> p1BinVec = {pMin, 0.0, pMax};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Assert
    Mantid::coord_t minimumDim = outWS->getDimension(0)->getMinimum();
    Mantid::coord_t maximumDim = outWS->getDimension(0)->getMaximum();

    TSM_ASSERT_DELTA("Should snap to the second bin boundary.", minimumDim,
                     static_cast<Mantid::coord_t>(secondBinBoundary), 1e-6);
    TSM_ASSERT_DELTA("Should snap to the last bin boundary", maximumDim,
                     static_cast<Mantid::coord_t>(max[0]), 1e-6);
  }

  void test_that_signals_of_workspace_shifted_by_half_a_bin_width_is_correct() {

  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
using namespace Mantid::DataObjects;
class IntegrateMDHistoWorkspaceTestPerformance : public CxxTest::TestSuite {

private:
  MDHistoWorkspace_sptr m_ws;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateMDHistoWorkspaceTestPerformance *createSuite() {
    return new IntegrateMDHistoWorkspaceTestPerformance();
  }
  static void destroySuite(IntegrateMDHistoWorkspaceTestPerformance *suite) {
    delete suite;
  }

  IntegrateMDHistoWorkspaceTestPerformance() {
    // Create a 4D workspace.
    m_ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal*/, 4 /*nd*/, 100 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
  }

  void test_execute_4d() {
    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    const double min = 0;
    const double max = 1;
    alg.setProperty("InputWorkspace", m_ws);
    std::vector<double> p1BinVec = {min, max};
    alg.setProperty("P1Bin", p1BinVec);
    alg.setProperty("P2Bin", p1BinVec);
    alg.setProperty("P3Bin", p1BinVec);
    alg.setProperty("P4Bin", p1BinVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
  }
};

#endif /* MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_ */
