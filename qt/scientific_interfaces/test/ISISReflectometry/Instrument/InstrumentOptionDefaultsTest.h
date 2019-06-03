// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTOPTIONDEFAULTSTEST_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTOPTIONDEFAULTSTEST_H_

#include "../../../ISISReflectometry/GUI/Instrument/InstrumentOptionDefaults.h"
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

class InstrumentOptionDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentOptionDefaultsTest *createSuite() {
    return new InstrumentOptionDefaultsTest();
  }
  static void destroySuite(InstrumentOptionDefaultsTest *suite) {
    delete suite;
  }

  InstrumentOptionDefaultsTest() { Mantid::API::FrameworkManager::Instance(); }

  void testValidMonitorOptions() {
    auto result = getDefaults("Instrument");
    auto expected = MonitorCorrections(2, true, RangeInLambda(17.0, 18.0),
                                       RangeInLambda(4.0, 10.0));
    TS_ASSERT_EQUALS(result.monitorCorrections(), expected);
  }

  void testInvalidMonitorIndex() { getDefaultsThrows("MonitorIndex_Invalid"); }

  void testInvalidMonitorBackground() {
    getDefaultsThrows("MonitorBackground_Invalid");
  }

  void testInvalidMonitorIntegral() {
    getDefaultsThrows("MonitorIntegral_Invalid");
  }

  void testValidWavelengthRange() {
    auto result = getDefaults("Instrument");
    auto expected = RangeInLambda(1.5, 17.0);
    TS_ASSERT_EQUALS(result.wavelengthRange(), expected);
  }

  void testInvalidWavelengthRange() {
    getDefaultsThrows("WavelengthRange_Invalid");
  }

  void testValidDetectorOptions() {
    auto result = getDefaults("Instrument");
    auto expected =
        DetectorCorrections(true, DetectorCorrectionType::RotateAroundSample);
    TS_ASSERT_EQUALS(result.detectorCorrections(), expected);
  }

  void testInvalidDetectorCorrection() {
    getDefaultsThrows("DetectorCorrection_Invalid");
  }

private:
  Instrument getDefaults(std::string const &paramsType) {
    // Get a dummy reflectometry instrument with the given parameters file type.
    // paramsType is appended to "REFL_Parameters_" to form the name for the
    // file
    // to load. See ReflectometryHelper.h for details.
    auto workspace = Mantid::TestHelpers::createREFL_WS(
        5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    InstrumentOptionDefaults instrumentDefaults;
    return instrumentDefaults.get(instrument);
  }

  void getDefaultsThrows(std::string const &paramsType) {
    auto workspace = Mantid::TestHelpers::createREFL_WS(
        5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, paramsType);
    auto instrument = workspace->getInstrument();
    InstrumentOptionDefaults instrumentDefaults;
    TS_ASSERT_THROWS(instrumentDefaults.get(instrument), std::invalid_argument);
  }
};

#endif // MANTID_CUSTOMINTERFACES_INSTRUMENTOPTIONDEFAULTSTEST_H_
