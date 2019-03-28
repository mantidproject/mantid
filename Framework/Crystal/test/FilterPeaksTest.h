// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FILTERPEAKSTEST_H_
#define MANTID_CRYSTAL_FILTERPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/FilterPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Crystal::FilterPeaks;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

//-------------------------------------------------------------
// Functional Tests
//-------------------------------------------------------------
class FilterPeaksTest : public CxxTest::TestSuite {
private:
  /*
   * Helper method to create a peaks workspace with a single peak.
   */
  PeaksWorkspace_sptr createInputWorkspace(const double h, const double k,
                                           const double l,
                                           const double intensity = 0,
                                           const double sigIntensity = 0) {
    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    ws->getPeak(0).setHKL(h, k, l); // First peak is already indexed now.
    ws->getPeak(0).setIntensity(intensity);
    ws->getPeak(0).setSigmaIntensity(sigIntensity);
    return ws;
  }

  /*
   * Helper method to run the algorithm and return the output workspace.
   */
  IPeaksWorkspace_sptr runAlgorithm(PeaksWorkspace_sptr inWS,
                                    const std::string &filterVariable,
                                    const double filterValue,
                                    const std::string &filterOperator) {
    const std::string outputWorkspace = "FilteredPeaks";

    FilterPeaks alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", outputWorkspace);
    alg.setProperty("FilterVariable", filterVariable);
    alg.setProperty("FilterValue", filterValue);
    alg.setProperty("Operator", filterOperator);
    alg.execute();

    IPeaksWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outputWorkspace);

    return outWS;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterPeaksTest *createSuite() { return new FilterPeaksTest(); }
  static void destroySuite(FilterPeaksTest *suite) { delete suite; }

  void test_Init() {
    FilterPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createPeaksWorkspace();

    // Name of the output workspace.
    std::string outWSName("FilterPeaksTest_OutputWS");

    FilterPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterVariable", "h+k+l"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterValue", 0.0))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Operator", ">"))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Will be empty as indices not set
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_filter_by_hkl() {
    const double h = 1;
    const double k = 1;
    const double l = 1;

    auto inWS = createInputWorkspace(h, k, l);

    auto outWS = runAlgorithm(inWS, "h+k+l", h + k + l, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", h + k + l, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", h + k + l, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", h + k + l, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", h + k + l, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", h + k + l, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_hkl_sq_sum() {
    const double h = 1;
    const double k = 1;
    const double l = 1;

    auto inWS = createInputWorkspace(h, k, l);

    auto outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", h * h + k * k + l * l, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_intensity() {
    const double h = 1;
    const double k = 1;
    const double l = 1;
    const double intensity = 1;

    auto inWS = createInputWorkspace(h, k, l, intensity);

    auto outWS = runAlgorithm(inWS, "Intensity", intensity, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", intensity, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", intensity, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", intensity, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", intensity, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", intensity, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_wavelength() {
    const double h = 1;
    const double k = 1;
    const double l = 1;

    auto inWS = createInputWorkspace(h, k, l);
    const auto wavelength = inWS->getPeak(0).getWavelength();

    auto outWS = runAlgorithm(inWS, "Wavelength", wavelength, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Wavelength", wavelength, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Wavelength", wavelength, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Wavelength", wavelength, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Wavelength", wavelength, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Wavelength", wavelength, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_tof() {
    const double h = 1;
    const double k = 1;
    const double l = 1;

    auto inWS = createInputWorkspace(h, k, l);
    const auto tof = inWS->getPeak(0).getTOF();

    auto outWS = runAlgorithm(inWS, "TOF", tof, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "TOF", tof, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "TOF", tof, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "TOF", tof, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "TOF", tof, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "TOF", tof, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_d_spacing() {
    const double h = 1;
    const double k = 1;
    const double l = 1;

    auto inWS = createInputWorkspace(h, k, l);
    const auto dspacing = inWS->getPeak(0).getDSpacing();

    auto outWS = runAlgorithm(inWS, "DSpacing", dspacing, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", dspacing, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", dspacing, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", dspacing, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", dspacing, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", dspacing, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }

  void test_filter_by_signal_to_noise() {
    const double h = 1;
    const double k = 1;
    const double l = 1;
    const double intensity = 1;
    const double sigIntensity = 0.5;
    const double ratio = intensity / sigIntensity;

    auto inWS = createInputWorkspace(h, k, l, intensity, sigIntensity);

    auto outWS = runAlgorithm(inWS, "Signal/Noise", ratio, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Signal/Noise", ratio, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Signal/Noise", ratio, "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Signal/Noise", ratio, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Signal/Noise", ratio, ">=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Signal/Noise", ratio, "<=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    AnalysisDataService::Instance().remove(outWS->getName());
    AnalysisDataService::Instance().remove(inWS->getName());
  }
};

//-------------------------------------------------------------
// Performance Tests
//-------------------------------------------------------------
class FilterPeaksTestPerformance : public CxxTest::TestSuite {
private:
  Mantid::DataObjects::PeaksWorkspace_sptr testWorkspace;

public:
  static FilterPeaksTestPerformance *createSuite() {
    return new FilterPeaksTestPerformance();
  }
  static void destroySuite(FilterPeaksTestPerformance *suite) { delete suite; }

  FilterPeaksTestPerformance() {
    const std::string outputWorkspace = "TOPAZ_3007.peaks";

    Mantid::API::FrameworkManagerImpl &manager =
        Mantid::API::FrameworkManager::Instance();
    auto load = manager.createAlgorithm("LoadIsawPeaks");
    load->initialize();
    load->setProperty("Filename", "TOPAZ_3007.peaks");
    load->setPropertyValue("OutputWorkspace", outputWorkspace);
    load->execute();
    testWorkspace =
        AnalysisDataService::Instance()
            .retrieveWS<Mantid::DataObjects::PeaksWorkspace>(outputWorkspace);
  }

  void testPerformance() {
    const std::string outputWorkspace = "FilteredPeaks";
    FilterPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWorkspace);
    alg.setPropertyValue("OutputWorkspace", outputWorkspace);
    alg.setProperty("FilterVariable", "h+k+l");
    alg.setProperty("FilterValue", 50.0);
    alg.execute();
    IPeaksWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outputWorkspace);
    TS_ASSERT(outWS->getNumberPeaks() <= testWorkspace->getNumberPeaks());
  }
};

#endif /* MANTID_CRYSTAL_FILTERPEAKSTEST_H_ */
