// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENTOPTIONDEFAULTSTEST_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENTOPTIONDEFAULTSTEST_H_

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentOptionDefaults.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ReflectometryHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class ExperimentOptionDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentOptionDefaultsTest *createSuite() {
    return new ExperimentOptionDefaultsTest();
  }
  static void destroySuite(ExperimentOptionDefaultsTest *suite) {
    delete suite;
  }

  ExperimentOptionDefaultsTest() { Mantid::API::FrameworkManager::Instance(); }

  void testValidAnalysisMode() {
    auto result = getDefaults("Experiment");
    TS_ASSERT_EQUALS(result.analysisMode(), AnalysisMode::MultiDetector);
  }

  void testInvalidAnalysisMode() { getDefaultsThrows("Analysis_Invalid"); }

  void testValidReductionOptions() {
    auto result = getDefaults("Experiment");
    TS_ASSERT_EQUALS(result.summationType(), SummationType::SumInQ);
    TS_ASSERT_EQUALS(result.reductionType(), ReductionType::NonFlatSample);
    TS_ASSERT_EQUALS(result.includePartialBins(), true);
  }

  void testInvalidReductionOptions() { getDefaultsThrows("Reduction_Invalid"); }

  void testValidDebugOptions() {
    auto result = getDefaults("Experiment");
    TS_ASSERT_EQUALS(result.debug(), true);
  }

  void testValidPerThetaOptions() {
    auto result = getDefaults("Experiment");
    auto expected = PerThetaDefaults(boost::none, TransmissionRunPair(),
                                     RangeInQ(0.01, 0.03, 0.2), 0.7,
                                     std::string("390-415"));
    TS_ASSERT_EQUALS(result.perThetaDefaults().size(), 1);
    TS_ASSERT_EQUALS(result.perThetaDefaults().front(), expected);
  }

  void testInvalidPerThetaOptions() { getDefaultsThrows("PerTheta_Invalid"); }

  void testValidTransmissionRunRange() {
    auto result = getDefaults("Experiment");
    auto const expected = RangeInLambda{10.0, 12.0};
    TS_ASSERT_EQUALS(result.transmissionRunRange(), expected);
  }

  void testInvalidTransmissionRunRange() {
    getDefaultsThrows("TransmissionRunRange_Invalid");
  }

  void testValidCorrectionOptions() {
    auto result = getDefaults("Experiment");
    TS_ASSERT_EQUALS(result.polarizationCorrections().correctionType(),
                     PolarizationCorrectionType::ParameterFile);
    TS_ASSERT_EQUALS(result.floodCorrections().correctionType(),
                     FloodCorrectionType::ParameterFile);
  }

  void testInvalidCorrectionOptions() {
    getDefaultsThrows("Correction_Invalid");
  }

private:
  Experiment getDefaults(std::string const &paramsType) {
    // Get a dummy reflectometry instrument with the given parameters file type.
    // paramsType is appended to "REFL_Parameters_" to form the name for the
    // file to load. See ReflectometryHelper.h for details.
    auto workspace = Mantid::TestHelpers::createREFL_WS(
        5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    ExperimentOptionDefaults experimentDefaults;
    return experimentDefaults.get(instrument);
  }

  void getDefaultsThrows(std::string const &paramsType) {
    auto workspace = Mantid::TestHelpers::createREFL_WS(
        5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    ExperimentOptionDefaults experimentDefaults;
    TS_ASSERT_THROWS(experimentDefaults.get(instrument), std::invalid_argument);
  }
};

#endif // MANTID_CUSTOMINTERFACES_EXPERIMENTOPTIONDEFAULTSTEST_H_
