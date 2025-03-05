// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::Poldi;
using namespace std::placeholders;

class PoldiPeakTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiPeakTest *createSuite() { return new PoldiPeakTest(); }
  static void destroySuite(PoldiPeakTest *suite) { delete suite; }

  void testCreateQOnly() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0);

    UncertainValue dValue = peak->d();
    TS_ASSERT_EQUALS(dValue.value(), 2.0 * M_PI);
    TS_ASSERT_EQUALS(dValue.error(), 0.0);

    double doubleDValue = peak->d();
    TS_ASSERT_EQUALS(doubleDValue, 2.0 * M_PI);

    UncertainValue qValue = peak->q();
    TS_ASSERT_EQUALS(qValue.value(), 1.0);

    TS_ASSERT_EQUALS(peak->fwhm(), 0.0);
    TS_ASSERT_EQUALS(peak->intensity(), 0.0);

    TS_ASSERT_THROWS(PoldiPeak::create(0.0), const std::domain_error &)
  }

  void testCreateQIntensity() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0, 2000.0);

    UncertainValue intensity = peak->intensity();
    TS_ASSERT_EQUALS(intensity.value(), 2000.0);
    TS_ASSERT_EQUALS(intensity.error(), 0.0);
    TS_ASSERT_EQUALS(peak->d(), 2.0 * M_PI);

    TS_ASSERT_THROWS(PoldiPeak::create(0.0, 23.0), const std::domain_error &);
  }

  void testQDConversion() {
    PoldiPeak_sptr one = PoldiPeak::create(2.0);

    TS_ASSERT_EQUALS(one->d(), M_PI);
    TS_ASSERT_EQUALS(one->q(), 2.0);

    TS_ASSERT_EQUALS(one->twoTheta(1.5), 0.48212062546814725648);
  }

  void testSetD() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0);

    peak->setD(UncertainValue(0.5, 0.0001));

    UncertainValue d = peak->d();
    TS_ASSERT_EQUALS(d.value(), 0.5);
    TS_ASSERT_EQUALS(d.error(), 0.0001);

    UncertainValue q = peak->q();
    TS_ASSERT_EQUALS(q.value(), 4.0 * M_PI);
    TS_ASSERT_EQUALS(q.error(), 0.00251327412287183459);
  }

  void testSetQ() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0);

    peak->setQ(UncertainValue(2.0, 0.1));

    UncertainValue d = peak->d();
    TS_ASSERT_EQUALS(d.value(), M_PI);
    TS_ASSERT_EQUALS(d.error(), 0.15707963267948966);

    UncertainValue q = peak->q();
    TS_ASSERT_EQUALS(q.value(), 2.0);
    TS_ASSERT_EQUALS(q.error(), 0.1);
  }

  void testSetIntensity() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0, 23.0);

    TS_ASSERT_EQUALS(peak->intensity(), 23.0);

    peak->setIntensity(UncertainValue(24.0, 2.0));

    UncertainValue newIntensity = peak->intensity();
    TS_ASSERT_EQUALS(newIntensity.value(), 24.0);
    TS_ASSERT_EQUALS(newIntensity.error(), 2.0);

    double doubleIntensity = peak->intensity();
    TS_ASSERT_EQUALS(doubleIntensity, 24.0);
  }

  void testSetFwhm() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0);

    peak->setFwhm(UncertainValue(0.01, 0.001));
    UncertainValue fwhm = peak->fwhm();
    std::cout << fwhm.value() << " " << fwhm.error() << '\n';
    TS_ASSERT_EQUALS(fwhm.value(), 0.01);
    TS_ASSERT_EQUALS(fwhm.error(), 0.001);

    double doubleFwhm = peak->fwhm();
    TS_ASSERT_EQUALS(doubleFwhm, 0.01);

    UncertainValue fwhmD = peak->fwhm(PoldiPeak::AbsoluteD);
    TS_ASSERT_EQUALS(fwhmD.value(), 0.02 * M_PI);
    TS_ASSERT_EQUALS(fwhmD.error(), 0.002 * M_PI);

    UncertainValue fwhmRel = peak->fwhm(PoldiPeak::Relative);
    TS_ASSERT_EQUALS(fwhmRel.value(), 0.01);
    TS_ASSERT_EQUALS(fwhmRel.error(), 0.001);
  }

  void testSetHKL() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0);
    MillerIndices hkl(2, 0, 3);

    peak->setHKL(hkl);

    MillerIndices newHkl = peak->hkl();
    TS_ASSERT_EQUALS(newHkl.h(), 2);
    TS_ASSERT_EQUALS(newHkl.k(), 0);
    TS_ASSERT_EQUALS(newHkl.l(), 3);
  }

  void testSortingGreater() {
    std::vector<PoldiPeak_sptr> peaks;
    peaks.emplace_back(PoldiPeak::create(1.0, 200.0));
    peaks.emplace_back(PoldiPeak::create(2.0, 20.0));
    peaks.emplace_back(PoldiPeak::create(3.0, 800.0));

    std::sort(peaks.begin(), peaks.end(), std::bind(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::q));
    TS_ASSERT_EQUALS(peaks[0]->q(), 3.0);
    TS_ASSERT_EQUALS(peaks[1]->q(), 2.0);
    TS_ASSERT_EQUALS(peaks[2]->q(), 1.0);

    std::sort(peaks.begin(), peaks.end(), std::bind(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::intensity));
    TS_ASSERT_EQUALS(peaks[0]->q(), 3.0);
    TS_ASSERT_EQUALS(peaks[1]->q(), 1.0);
    TS_ASSERT_EQUALS(peaks[2]->q(), 2.0);
  }

  void testSortingLess() {
    std::vector<PoldiPeak_sptr> peaks;
    peaks.emplace_back(PoldiPeak::create(1.0, 200.0));
    peaks.emplace_back(PoldiPeak::create(2.0, 20.0));
    peaks.emplace_back(PoldiPeak::create(3.0, 800.0));

    std::sort(peaks.begin(), peaks.end(), std::bind(&PoldiPeak::lessThan, _1, _2, &PoldiPeak::q));
    TS_ASSERT_EQUALS(peaks[0]->q(), 1.0);
    TS_ASSERT_EQUALS(peaks[1]->q(), 2.0);
    TS_ASSERT_EQUALS(peaks[2]->q(), 3.0);

    std::sort(peaks.begin(), peaks.end(), std::bind(&PoldiPeak::lessThan, _1, _2, &PoldiPeak::intensity));
    TS_ASSERT_EQUALS(peaks[0]->q(), 2.0);
    TS_ASSERT_EQUALS(peaks[1]->q(), 1.0);
    TS_ASSERT_EQUALS(peaks[2]->q(), 3.0);
  }

  void testClone() {
    PoldiPeak_sptr peak = PoldiPeak::create(1.0, 200.00);
    PoldiPeak_sptr clone = peak->clonePeak();

    TS_ASSERT_EQUALS(peak->d(), clone->d());
    TS_ASSERT_EQUALS(peak->fwhm(), clone->fwhm());
    TS_ASSERT_EQUALS(peak->intensity(), clone->intensity());
    TS_ASSERT_EQUALS(peak->hkl(), clone->hkl());
  }
};
