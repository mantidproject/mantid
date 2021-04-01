// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidCrystal/FilterPeaks.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
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
  PeaksWorkspace_sptr createInputWorkspace(const double h, const double k, const double l, const double intensity = 0,
                                           const double sigIntensity = 0) {
    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    ws->getPeak(0).setHKL(h, k, l); // First peak is already indexed now.
    ws->getPeak(0).setIntensity(intensity);
    ws->getPeak(0).setSigmaIntensity(sigIntensity);
    ws->getPeak(0).setBankName("bank1");
    return ws;
  }

  /*
   * Helper method to run the algorithm and return the output workspace.
   * -- filter value
   */
  IPeaksWorkspace_sptr runAlgorithm(const IPeaksWorkspace_sptr &inWS, const std::string &filterVariable,
                                    const double filterValue, const std::string &filterOperator) {
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

    IPeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outputWorkspace);

    return outWS;
  }

  /*
   * Helper method to run the algorithm and return the output workspace.
   * -- bank selection
   */
  IPeaksWorkspace_sptr runAlgorithm(const PeaksWorkspace_sptr &inWS, const std::string &bankname,
                                    const std::string &criterion) {
    const std::string outputWorkspace = "FilteredPeaks";

    FilterPeaks alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", outputWorkspace);
    alg.setProperty("BankName", bankname);
    alg.setProperty("Criterion", criterion);
    alg.execute();

    IPeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outputWorkspace);

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

    PeaksWorkspace_sptr inputWS = WorkspaceCreationHelper::createPeaksWorkspace(2);

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
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outWSName));
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

  void test_filter_by_bank() {
    const double h = 1;
    const double k = 1;
    const double l = 1;
    const double intensity = 1;
    const double sigIntensity = 0.5;
    const std::string bankname = "bank1";

    auto inWS = createInputWorkspace(h, k, l, intensity, sigIntensity);
    auto outWS = runAlgorithm(inWS, "bank1", "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "bank1", "!=");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());
  }

  void test_filter_LeanElasticPeaksWorkspace() {
    auto inWS = std::make_shared<LeanElasticPeaksWorkspace>();

    LeanElasticPeak peak(Mantid::Kernel::V3D(1, 1, 0), 1.0);
    peak.setIntensity(100.0);
    peak.setHKL(1, 1, 0);
    TS_ASSERT_DELTA(peak.getDSpacing(), M_PI * M_SQRT2, 1e-9)
    inWS->addPeak(peak);

    LeanElasticPeak peak2(Mantid::Kernel::V3D(1, 0, 0), 2.0);
    peak2.setIntensity(10.0);
    peak2.setHKL(1, 0, 0);
    TS_ASSERT_DELTA(peak2.getDSpacing(), 2 * M_PI, 1e-9)
    inWS->addPeak(peak2);

    auto outWS = runAlgorithm(inWS, "Wavelength", 1.0, "<");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());
    outWS = runAlgorithm(inWS, "Wavelength", 1.0, ">");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "DSpacing", 5, "<");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());
    outWS = runAlgorithm(inWS, "DSpacing", 0, ">");
    TS_ASSERT_EQUALS(2, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h+k+l", 2, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());
    outWS = runAlgorithm(inWS, "h+k+l", 3, "<");
    TS_ASSERT_EQUALS(2, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", 2, "=");
    TS_ASSERT_EQUALS(1, outWS->getNumberPeaks());
    outWS = runAlgorithm(inWS, "h^2+k^2+l^2", 2, ">");
    TS_ASSERT_EQUALS(0, outWS->getNumberPeaks());

    outWS = runAlgorithm(inWS, "Intensity", 1000, "<");
    TS_ASSERT_EQUALS(2, outWS->getNumberPeaks());
    outWS = runAlgorithm(inWS, "Intensity", 20, ">");
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
  static FilterPeaksTestPerformance *createSuite() { return new FilterPeaksTestPerformance(); }
  static void destroySuite(FilterPeaksTestPerformance *suite) { delete suite; }

  FilterPeaksTestPerformance() {
    const std::string outputWorkspace = "TOPAZ_3007.peaks";

    auto &manager = Mantid::API::AlgorithmManager::Instance();
    auto load = manager.create("LoadIsawPeaks");
    load->initialize();
    load->setProperty("Filename", "TOPAZ_3007.peaks");
    load->setPropertyValue("OutputWorkspace", outputWorkspace);
    load->execute();
    testWorkspace = AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::PeaksWorkspace>(outputWorkspace);
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
    IPeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outputWorkspace);
    TS_ASSERT(outWS->getNumberPeaks() <= testWorkspace->getNumberPeaks());
  }
};
