// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/ConvertHFIRSCDtoMDE.h"
#include "MantidMDAlgorithms/LoadMD.h"

#include <string>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

class ConvertHFIRSCDtoMDETest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertHFIRSCDtoMDETest *createSuite() { return new ConvertHFIRSCDtoMDETest(); }
  static void destroySuite(ConvertHFIRSCDtoMDETest *suite) { delete suite; }

  void test_Init() {
    ConvertHFIRSCDtoMDE alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Create test input if necessary
    LoadMD loader;
    loader.initialize();
    loader.setPropertyValue("Filename", Mantid::API::FileFinder::Instance().getFullPath("HB3A_data.nxs").string());
    loader.setPropertyValue("OutputWorkspace", "ConvertHFIRSCDtoMDETest_data");
    loader.setProperty("FileBackEnd", false);
    loader.execute();
    auto inputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDHistoWorkspace>(
        "ConvertHFIRSCDtoMDETest_data");

    auto setGoniometer = AlgorithmManager::Instance().create("SetGoniometer");
    setGoniometer->initialize();
    setGoniometer->setProperty("Workspace", inputWS);
    setGoniometer->setPropertyValue("Axis0", "omega,0,1,0,-1");
    setGoniometer->setPropertyValue("Axis1", "chi,0,0,1,-1");
    setGoniometer->setPropertyValue("Axis2", "phi,0,1,0,-1");
    setGoniometer->setProperty("Average", false);
    setGoniometer->execute();

    ConvertHFIRSCDtoMDE alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "ConvertHFIRSCDtoMDETest_data"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Wavelength", "1.008"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably
    // need to change. It should be the type using in declareProperty for the
    // "OutputWorkspace" type. We can't use auto as it's an implicit conversion.
    IMDEventWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);

    // check dimensions
    TS_ASSERT_EQUALS(3, outWS->getNumDims());
    TS_ASSERT_EQUALS(Mantid::Kernel::QSample, outWS->getSpecialCoordinateSystem());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(0)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(0)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(0)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(0)->getMaximum());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(1)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(1)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(1)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(1)->getMaximum());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(2)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(2)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(2)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(2)->getMaximum());

    // check other things
    const Mantid::coord_t coords[3] = {-0.42f, 1.71f, 2.3f}; // roughly the location of maximum instenity
    TS_ASSERT_EQUALS(1, outWS->getNumExperimentInfo());
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 568, 1e-5);

    // check coeff behavior
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "ConvertHFIRSCDtoMDETest_data"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Wavelength", "1.008"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ObliquityParallaxCoefficient", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(1, outWS->getNumExperimentInfo());
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 453, 1e-5);
  }

  void test_wavelength_from_sample_log_without_property() {
    // HB3A_data.nxs stores the wavelength log as the string "1.008000"
    auto outWS = convert(loadData(), "");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_sample_log_takes_precedence_over_property() {
    auto outWS = convert(loadData(), "2.0");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_numeric_sample_log() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().addProperty("wavelength", 1.008, true);
    auto outWS = convert(inputWS, "2.0");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_property_used_when_sample_log_missing() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    auto outWS = convert(inputWS, "1.008");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_missing_wavelength() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    assertWavelengthError(inputWS, "", "no 'wavelength' sample log");
  }

  void test_non_numeric_sample_log() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().addProperty("wavelength", std::string("unknown"), true);
    assertWavelengthError(inputWS, "1.008", "cannot be converted to a number");
  }

  void test_several_wavelengths_for_single_input() { assertWavelengthError(loadData(), "1.008,1.5", "Only one value"); }

  void test_non_positive_wavelength_property() {
    ConvertHFIRSCDtoMDE alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("Wavelength", "0.0"), const std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setProperty("Wavelength", "-1.0"), const std::invalid_argument &);
  }

private:
  IMDHistoWorkspace_sptr m_data;

  /// Load HB3A_data.nxs once, and return a copy that a test may modify
  IMDHistoWorkspace_sptr loadData() {
    if (!m_data) {
      LoadMD loader;
      loader.initialize();
      loader.setChild(true);
      loader.setPropertyValue("Filename", "HB3A_data.nxs");
      loader.setPropertyValue("OutputWorkspace", "_unused_for_child");
      loader.setProperty("FileBackEnd", false);
      loader.execute();
      Mantid::API::IMDWorkspace_sptr loaded = loader.getProperty("OutputWorkspace");
      m_data = std::dynamic_pointer_cast<IMDHistoWorkspace>(loaded);

      auto setGoniometer = AlgorithmManager::Instance().createUnmanaged("SetGoniometer");
      setGoniometer->initialize();
      setGoniometer->setChild(true);
      setGoniometer->setProperty("Workspace", m_data);
      setGoniometer->setPropertyValue("Axis0", "omega,0,1,0,-1");
      setGoniometer->setPropertyValue("Axis1", "chi,0,0,1,-1");
      setGoniometer->setPropertyValue("Axis2", "phi,0,1,0,-1");
      setGoniometer->setProperty("Average", false);
      setGoniometer->execute();
    }
    return m_data->clone();
  }

  /// Run the algorithm; an empty wavelength leaves the Wavelength property unset
  IMDEventWorkspace_sptr convert(const IMDHistoWorkspace_sptr &inputWS, const std::string &wavelength) {
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    if (!wavelength.empty())
      alg.setProperty("Wavelength", wavelength);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    return alg.getProperty("OutputWorkspace");
  }

  /// Check the output against the reference values of test_exec, obtained with the given wavelength
  void assertConvertedWithWavelength(const IMDEventWorkspace_sptr &outWS, const double wavelength) {
    TS_ASSERT(outWS);
    if (!outWS)
      return;
    const auto &run = outWS->getExperimentInfo(0)->run();
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("wavelength"), wavelength, 1e-9);
    const Mantid::coord_t coords[3] = {-0.42f, 1.71f, 2.3f};
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 568, 1e-5);
  }

  /// Check that validation reports a Wavelength error containing the given text
  void assertWavelengthError(const IMDHistoWorkspace_sptr &inputWS, const std::string &wavelength,
                             const std::string &expected) {
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    if (!wavelength.empty())
      alg.setProperty("Wavelength", wavelength);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    const auto errors = alg.validateInputs();
    TS_ASSERT_EQUALS(errors.count("Wavelength"), 1);
    if (errors.count("Wavelength") == 1)
      TS_ASSERT(errors.at("Wavelength").find(expected) != std::string::npos);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
};
