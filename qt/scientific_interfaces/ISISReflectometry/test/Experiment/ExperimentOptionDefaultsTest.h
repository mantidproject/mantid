// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentOptionDefaults.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidGeometry/Instrument.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class ExperimentOptionDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentOptionDefaultsTest *createSuite() { return new ExperimentOptionDefaultsTest(); }
  static void destroySuite(ExperimentOptionDefaultsTest *suite) { delete suite; }

  ExperimentOptionDefaultsTest() { Mantid::API::FrameworkManager::Instance(); }

  void testDefaultAnalysisMode() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.analysisMode(), AnalysisMode::PointDetector);
  }

  void testValidAnalysisModeFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    TS_ASSERT_EQUALS(result.analysisMode(), AnalysisMode::MultiDetector);
  }

  void testInvalidAnalysisModeFromParamsFile() { getDefaultsFromParamsFileThrows("Analysis_Invalid"); }

  void testDefaultReductionOptions() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.summationType(), SummationType::SumInLambda);
    TS_ASSERT_EQUALS(result.reductionType(), ReductionType::Normal);
    TS_ASSERT_EQUALS(result.includePartialBins(), false);
  }

  void testValidReductionOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    TS_ASSERT_EQUALS(result.summationType(), SummationType::SumInQ);
    TS_ASSERT_EQUALS(result.reductionType(), ReductionType::NonFlatSample);
    TS_ASSERT_EQUALS(result.includePartialBins(), true);
  }

  void testInvalidReductionOptionsFromParamsFile() { getDefaultsFromParamsFileThrows("Reduction_Invalid"); }

  void testDefaultDebugOptions() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.debug(), false);
  }

  void testValidDebugOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    TS_ASSERT_EQUALS(result.debug(), true);
  }

  void testDefaultLookupRowOptions() {
    auto result = getDefaults();
    auto expected = LookupRow(boost::none, boost::none, TransmissionRunPair(), boost::none,
                              RangeInQ(std::nullopt, std::nullopt, std::nullopt), boost::none, boost::none, boost::none,
                              boost::none);
    auto foundLookupRows = result.lookupTableRows();
    TS_ASSERT_EQUALS(foundLookupRows.size(), 1);
    if (!foundLookupRows.empty())
      TS_ASSERT_EQUALS(foundLookupRows.front(), expected);
  }

  void testValidLookupRowOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    auto expected = LookupRow(boost::none, boost::none, TransmissionRunPair(), boost::none, RangeInQ(0.01, 0.03, 0.2),
                              0.7, std::string("390-415"), std::string("370-389,416-430"), boost::none);
    auto foundLookupRows = result.lookupTableRows();
    TS_ASSERT_EQUALS(foundLookupRows.size(), 1);
    if (!foundLookupRows.empty())
      TS_ASSERT_EQUALS(foundLookupRows.front(), expected);
  }

  void testInvalidLookupRowOptionsFromParamsFile() { getDefaultsFromParamsFileThrows("LookupRow_Invalid"); }

  void testDefaultTransmissionRunRange() {
    auto result = getDefaults();
    auto const expected = RangeInLambda{0.0, 0.0};
    TS_ASSERT_EQUALS(result.transmissionStitchOptions().overlapRange(), expected);
  }

  void testValidTransmissionRunRangeFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    auto const expected = RangeInLambda{10.0, 12.0};
    TS_ASSERT_EQUALS(result.transmissionStitchOptions().overlapRange(), expected);
  }

  void testInvalidTransmissionRunRangeFromParamsFile() {
    getDefaultsFromParamsFileThrows("TransmissionRunRange_Invalid");
  }

  void testDefaultSubtractionOptions() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.backgroundSubtraction().subtractBackground(), false);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().subtractionType(), BackgroundSubtractionType::PerDetectorAverage);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().degreeOfPolynomial(), 0);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().costFunction(), CostFunctionType::LeastSquares);
  }

  void testValidSubtractionOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    TS_ASSERT_EQUALS(result.backgroundSubtraction().subtractBackground(), true);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().subtractionType(), BackgroundSubtractionType::Polynomial);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().degreeOfPolynomial(), 2);
    TS_ASSERT_EQUALS(result.backgroundSubtraction().costFunction(), CostFunctionType::UnweightedLeastSquares);
  }

  void testInvalidSubtractionOptionsFromParamsFile() { getDefaultsFromParamsFileThrows("Subtraction_Invalid"); }

  void testDefaultCorrectionOptions() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.polarizationCorrections().correctionType(), PolarizationCorrectionType::None);
    TS_ASSERT_EQUALS(result.floodCorrections().correctionType(), FloodCorrectionType::Workspace);
  }

  void testValidCorrectionOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    TS_ASSERT_EQUALS(result.polarizationCorrections().correctionType(), PolarizationCorrectionType::ParameterFile);
    TS_ASSERT_EQUALS(result.floodCorrections().correctionType(), FloodCorrectionType::ParameterFile);
  }

  void testInvalidCorrectionOptionsFromParamsFile() { getDefaultsFromParamsFileThrows("Correction_Invalid"); }

  void testDefaultStitchParamsOptions() {
    auto result = getDefaults();
    TS_ASSERT_EQUALS(result.stitchParameters().empty(), true);
  }

  void testValidStitchParamsOptionsFromParamsFile() {
    auto result = getDefaultsFromParamsFile("Experiment");
    auto stitchResults = result.stitchParameters();
    TS_ASSERT_EQUALS(stitchResults.size(), 1);
    TS_ASSERT_EQUALS(stitchResults.begin()->first, "ManualScaleFactors");
    TS_ASSERT_EQUALS(stitchResults.begin()->second, "1");
  }

private:
  Experiment getDefaults() {
    // Provide the mandatory params file so that we don't throw. Other params
    // will be unset so the hard-coded defaults will be used instead.

    // Note that we use an instrument suffix here because otherwise
    // the workspace instrument can pick up settings from a previously-loaded
    // parameters file for the same instrument for another test!
    auto workspace =
        Mantid::FrameworkTestHelpers::createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "", "MANDATORY");
    auto instrument = workspace->getInstrument();
    ExperimentOptionDefaults experimentDefaults;
    return experimentDefaults.get(instrument);
  }

  Experiment getDefaultsFromParamsFile(std::string const &paramsType) {
    // Get a dummy reflectometry instrument with the given parameters file type.
    // paramsType is appended to "REFL_Parameters_" to form the name for the
    // file to load. See ReflectometryHelper.h for details.
    auto workspace =
        Mantid::FrameworkTestHelpers::createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    ExperimentOptionDefaults experimentDefaults;
    return experimentDefaults.get(instrument);
  }

  void getDefaultsFromParamsFileThrows(std::string const &paramsType) {
    auto workspace =
        Mantid::FrameworkTestHelpers::createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    ExperimentOptionDefaults experimentDefaults;
    TS_ASSERT_THROWS(experimentDefaults.get(instrument), const std::invalid_argument &);
  }
};
