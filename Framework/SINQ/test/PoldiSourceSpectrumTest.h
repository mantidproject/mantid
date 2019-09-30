// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef POLDISOURCESPECTRUMTEST_H
#define POLDISOURCESPECTRUMTEST_H

#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Interpolation.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::Poldi;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class PoldiSourceSpectrumTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiSourceSpectrumTest *createSuite() {
    return new PoldiSourceSpectrumTest();
  }
  static void destroySuite(PoldiSourceSpectrumTest *suite) { delete suite; }

  void testInterpolationConstructor() {
    Interpolation interpolation;

    TS_ASSERT_THROWS_NOTHING(PoldiSourceSpectrum spectrum(interpolation));
  }

  void testInterpolation() {
    Interpolation interpolation;
    interpolation.addPoint(0.0, 2.0);
    interpolation.addPoint(1.0, 4.0);
    interpolation.addPoint(2.0, 6.0);

    TS_ASSERT_EQUALS(interpolation.value(0.0), 2.0);
    TS_ASSERT_EQUALS(interpolation.value(2.0), 6.0);

    PoldiSourceSpectrum spectrum(interpolation);
    TS_ASSERT_EQUALS(spectrum.intensity(0.0), 2.0);
    TS_ASSERT_EQUALS(spectrum.intensity(1.0), 4.0);
  }

  void testGetSourceComponent() {
    TestablePoldiSourceSpectrum spectrum;

    boost::shared_ptr<const PoldiAbstractFakeInstrument> goodInstrument(
        new PoldiValidSourceFakeInstrument);
    TS_ASSERT_THROWS_NOTHING(spectrum.getSourceComponent(goodInstrument));
    IComponent_const_sptr source = spectrum.getSourceComponent(goodInstrument);
    TS_ASSERT_EQUALS(source->getFullName(), "FakePoldiSource");

    boost::shared_ptr<const PoldiAbstractFakeInstrument> badInstrument(
        new PoldiInvalidSourceFakeInstrument);
    TS_ASSERT_THROWS(spectrum.getSourceComponent(badInstrument),
                     const std::runtime_error &);
  }

  void testGetSpectrumParameter() {
    TestablePoldiSourceSpectrum spectrum;

    boost::shared_ptr<const IComponent> source =
        boost::make_shared<PoldiFakeSourceComponent>();
    ParameterMap_sptr goodParameterMap(
        new PoldiValidFakeParameterMap(source.get()));

    TS_ASSERT_THROWS_NOTHING(
        spectrum.getSpectrumParameter(source, goodParameterMap));

    ParameterMap_sptr badParameterMap(new PoldiInvalidFakeParameterMap);
    TS_ASSERT_THROWS(spectrum.getSpectrumParameter(source, badParameterMap),
                     const std::runtime_error &);
  }

  void testSetSpectrum() {
    TestablePoldiSourceSpectrum spectrum;

    boost::shared_ptr<const IComponent> source =
        boost::make_shared<PoldiFakeSourceComponent>();
    ParameterMap_sptr goodParameterMap(
        new PoldiValidFakeParameterMap(source.get()));
    Parameter_sptr goodParameter =
        spectrum.getSpectrumParameter(source, goodParameterMap);
    TS_ASSERT_THROWS_NOTHING(spectrum.setSpectrum(goodParameter));

    TS_ASSERT_THROWS(spectrum.setSpectrum(Parameter_sptr()),
                     const std::runtime_error &);
  }

private:
  class TestablePoldiSourceSpectrum : public PoldiSourceSpectrum {
    friend class PoldiSourceSpectrumTest;

  public:
    TestablePoldiSourceSpectrum(Interpolation spectrum = Interpolation())
        : PoldiSourceSpectrum(spectrum) {}

    TestablePoldiSourceSpectrum(Instrument_const_sptr poldiInstrument)
        : PoldiSourceSpectrum(poldiInstrument) {}
  };
};

#endif // POLDISOURCESPECTRUMTEST_H
