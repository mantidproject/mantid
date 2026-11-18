// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectrumUnitConversion.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include <memory>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SpectrumUnitConversionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumUnitConversionTest *createSuite() { return new SpectrumUnitConversionTest(); }
  static void destroySuite(SpectrumUnitConversionTest *suite) { delete suite; }

  SpectrumUnitConversionTest() {
    // The detectors of spectra 0 and 2 are on opposite sides of the beam
    m_workspace.initialize(3, 2, 1);
    InstrumentCreationHelper::addFullInstrumentToWorkspace(m_workspace, false, true, "SimpleFakeInstrument");
  }

  void test_units_are_uninitialised_copies() {
    const auto setup = makeSetup();

    const SpectrumUnitConversion conversion(setup);

    TS_ASSERT_DIFFERS(conversion.from.get(), &m_tof);
    TS_ASSERT_DIFFERS(conversion.to.get(), &m_wavelength);
    TS_ASSERT_EQUALS(conversion.from->unitID(), "TOF");
    TS_ASSERT_EQUALS(conversion.to->unitID(), "Wavelength");
    TS_ASSERT(!conversion.from->isInitialized());
    TS_ASSERT(!conversion.to->isInitialized());
  }

  void test_only_initial_parameters_are_present_before_reading_detector_values() {
    const auto setup = makeSetup(DeltaEMode::Elastic, false, {{UnitParams::delta, 0.5}});

    const SpectrumUnitConversion conversion(setup);

    TS_ASSERT_EQUALS(conversion.params.size(), 1);
    TS_ASSERT_EQUALS(conversion.params.at(UnitParams::delta), 0.5);
  }

  void test_readDetectorValues_adds_values_for_the_spectrum() {
    const auto setup = makeSetup(DeltaEMode::Elastic, false, {{UnitParams::delta, 0.5}});
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    SpectrumUnitConversion conversion(setup);

    conversion.readDetectorValues(2);

    TS_ASSERT_EQUALS(conversion.params.at(UnitParams::l2), spectrumInfo.l2(2));
    TS_ASSERT_EQUALS(conversion.params.at(UnitParams::twoTheta), spectrumInfo.twoTheta(2));
    TS_ASSERT_EQUALS(conversion.params.at(UnitParams::delta), 0.5);
  }

  void test_initial_efixed_is_not_overwritten() {
    const auto setup = makeSetup(DeltaEMode::Direct, false, {{UnitParams::efixed, 12.5}});
    SpectrumUnitConversion conversion(setup);

    conversion.readDetectorValues(0);

    TS_ASSERT_EQUALS(conversion.params.at(UnitParams::efixed), 12.5);
  }

  void test_signedTheta_gives_signed_angles() {
    const auto setup = makeSetup(DeltaEMode::Elastic, true);
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    SpectrumUnitConversion first(setup);
    SpectrumUnitConversion last(setup);

    first.readDetectorValues(0);
    last.readDetectorValues(2);

    const double firstAngle = first.params.at(UnitParams::twoTheta);
    const double lastAngle = last.params.at(UnitParams::twoTheta);
    TS_ASSERT_EQUALS(firstAngle, spectrumInfo.signedTwoTheta(0));
    TS_ASSERT_EQUALS(lastAngle, spectrumInfo.signedTwoTheta(2));
    TS_ASSERT_LESS_THAN(std::min(firstAngle, lastAngle), 0.0);
  }

  void test_conversions_do_not_share_unit_state() {
    const auto setup = makeSetup();
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    SpectrumUnitConversion first(setup);
    const SpectrumUnitConversion second(setup);
    first.readDetectorValues(0);

    first.to->initialize(spectrumInfo.l1(), 0, first.params);

    TS_ASSERT(first.to->isInitialized());
    TS_ASSERT(!second.to->isInitialized());
    TS_ASSERT(!m_wavelength.isInitialized());
  }

private:
  UnitConversionSetup makeSetup(const DeltaEMode::Type emode = DeltaEMode::Elastic, const bool signedTheta = false,
                                const UnitParametersMap &initialParams = {}) const {
    return {m_workspace.spectrumInfo(), m_tof, m_wavelength, emode, signedTheta, initialParams};
  }

  WorkspaceTester m_workspace;
  Units::TOF m_tof;
  Units::Wavelength m_wavelength;
};
