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

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

class ExperimentOptionDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentOptionDefaultsTest *createSuite() {
    return new ExperimentOptionDefaultsTest();
  }
  static void destroySuite(ExperimentOptionDefaultsTest *suite) { delete suite; }

  ExperimentOptionDefaultsTest() : m_view() {
    Mantid::API::FrameworkManager::Instance();
  }

  void testValidAnalysisMode() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Analysis");
    presenter.notifyRestoreDefaultsRequested();
    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(),
                     AnalysisMode::MultiDetector);
    verifyAndClear();
  }

  void testInvalidAnalysisMode() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Analysis_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

  void testValidReductionOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Reduction");
    presenter.notifyRestoreDefaultsRequested();
    TS_ASSERT_EQUALS(presenter.experiment().summationType(),
                     SummationType::SumInQ);
    TS_ASSERT_EQUALS(presenter.experiment().reductionType(),
                     ReductionType::NonFlatSample);
    TS_ASSERT_EQUALS(presenter.experiment().includePartialBins(), true);
    verifyAndClear();
  }

  void testInvalidReductionOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Reduction_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

  void testValidDebugOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Debug");
    presenter.notifyRestoreDefaultsRequested();
    TS_ASSERT_EQUALS(presenter.experiment().debug(), true);
    verifyAndClear();
  }

  void testInvalidDebugOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Debug_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

  void testValidPerThetaOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("PerTheta");
    presenter.notifyRestoreDefaultsRequested();
    auto expected = PerThetaDefaults(boost::none, TransmissionRunPair(),
                                     RangeInQ(0.01, 0.03, 0.2), 0.7,
                                     std::string("390-415"));
    TS_ASSERT_EQUALS(presenter.experiment().perThetaDefaults().size(), 1);
    TS_ASSERT_EQUALS(presenter.experiment().perThetaDefaults().front(),
                     expected);
    verifyAndClear();
  }

  void testInvalidPerThetaOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("PerTheta_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

  void testValidTransmissionRunRange() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("TransmissionRunRange");
    presenter.notifyRestoreDefaultsRequested();
    auto const expected = RangeInLambda{10.0, 12.0};
    TS_ASSERT_EQUALS(presenter.experiment().transmissionRunRange(), expected);
    verifyAndClear();
  }

  void testInvalidTransmissionRunRange() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("TransmissionRunRange_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

  void testValidCorrectionOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Correction");
    presenter.notifyRestoreDefaultsRequested();
    TS_ASSERT_EQUALS(
        presenter.experiment().polarizationCorrections().correctionType(),
        PolarizationCorrectionType::ParameterFile);
    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections().correctionType(),
                     FloodCorrectionType::ParameterFile);
    verifyAndClear();
  }

  void testInvalidCorrectionOptions() {
    auto presenter = makePresenter();
    expectInstrumentWithParameters("Correction_Invalid");
    TS_ASSERT_THROWS(presenter.notifyRestoreDefaultsRequested(), std::invalid_argument);
  }

private:
  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
  }
 
  // Get a dummy reflectometry instrument with the given parameters file type.
  // paramsType is appended to "REFL_Parameters_" to form the name for the file
  // to load. See ReflectometryHelper.h for details.
  void Mantid::Geometry::Instrument_const_sptr
  getInstrumentWithParameters(std::string const &paramsType) {
    auto workspace = Mantid::TestHelpers::createREFL_WS(
        5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    return workspace->getInstrument();
  }

  void expectInstrumentWithDefaultParameters() {
    // Use the default REFL_Parameters.xml file, which is empty
    expectInstrumentWithParameters("");
  }

  void expectInstrumentWithParameters(std::string const &paramsType) {
    // Use the REFL_Parameters_<paramsType> file
    auto instrument = getInstrumentWithParameters(paramsType);
    EXPECT_CALL(m_mainPresenter, instrument())
        .Times(1)
        .WillOnce(Return(instrument));
  }

GNU_DIAG_ON("missing-braces")

#endif // MANTID_CUSTOMINTERFACES_EXPERIMENTOPTIONDEFAULTSTEST_H_
