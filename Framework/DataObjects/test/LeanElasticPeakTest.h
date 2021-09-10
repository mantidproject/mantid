// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/V3D.h"

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/LeanElasticPeak.h"

#include "MantidDataObjects/Peak.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LeanElasticPeakTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeanElasticPeakTest *createSuite() { return new LeanElasticPeakTest(); }
  static void destroySuite(LeanElasticPeakTest *suite) { delete suite; }

  void test_default_constructor() {
    LeanElasticPeak p;
    TS_ASSERT_EQUALS(p.getH(), 0.0)
    TS_ASSERT_EQUALS(p.getK(), 0.0)
    TS_ASSERT_EQUALS(p.getL(), 0.0)
    TS_ASSERT(std::isinf(p.getInitialEnergy()))
    TS_ASSERT(std::isinf(p.getFinalEnergy()))
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(0, 0, 0))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D())
    TS_ASSERT_EQUALS(p.getSamplePos(), V3D(0, 0, 0))
    TS_ASSERT_THROWS(p.getTOF(), const Exception::NotImplementedError &)
    TS_ASSERT_EQUALS(p.getScattering(), 0.)
    TS_ASSERT_EQUALS(p.getAzimuthal(), -M_PI)
    TS_ASSERT_THROWS(p.getRow(), const Exception::NotImplementedError &)
    TS_ASSERT_THROWS(p.getCol(), const Exception::NotImplementedError &)
    TS_ASSERT_THROWS(p.getL1(), const Exception::NotImplementedError &)
    TS_ASSERT_THROWS(p.getL2(), const Exception::NotImplementedError &)
  }

  void test_Qsample_constructor() {
    LeanElasticPeak p(V3D(1, 2, 3));
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(1, 2, 3))

    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;
    p.setGoniometerMatrix(gon);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(2, 1, 3))

    p.setWavelength(1.);
    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getFinalEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getWavelength(), 1., 1e-9)
  }

  void test_Qsample_gon_constructor() {

    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanElasticPeak p(V3D(1, 2, 3), gon);

    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(2, 1, 3))
    TS_ASSERT_DELTA(p.getWavelength(), M_PI * 6 / 7, 1e-9)
    TS_ASSERT_DELTA(p.getDSpacing(), 1.679251908362714, 1e-9)
    TS_ASSERT_DELTA(p.getScattering(), 1.860548028230944, 1e-9)
    TS_ASSERT_DELTA(p.getAzimuthal(), -2.6779450449, 1e-9)

    // calculate Q_lab from scattering and azimuthal to check values
    const auto k = 2 * M_PI / p.getWavelength();
    V3D qLab(-sin(p.getScattering()) * cos(p.getAzimuthal()), -sin(p.getScattering()) * sin(p.getAzimuthal()),
             1 - cos(p.getScattering()));
    qLab *= k;
    TS_ASSERT_DELTA(qLab.X(), 2, 1e-9)
    TS_ASSERT_DELTA(qLab.Y(), 1, 1e-9)
    TS_ASSERT_DELTA(qLab.Z(), 3, 1e-9)
  }

  void test_Qsample_gon_constructor_refFrame() {

    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    // different reference frame should cause different wavelength to be
    // calculated
    auto refFrame = std::make_shared<const ReferenceFrame>(Mantid::Geometry::Y /*up*/, Mantid::Geometry::X /*along*/,
                                                           Left, "0,0,0");

    LeanElasticPeak p(V3D(1, 2, 3), gon, refFrame);

    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(2, 1, 3))

    TS_ASSERT_EQUALS(p.getReferenceFrame()->vecPointingAlongBeam(), V3D(1, 0, 0))
    TS_ASSERT_EQUALS(p.getReferenceFrame()->pointingAlongBeam(), 0)
    TS_ASSERT_DELTA(p.getWavelength(), M_PI * 4 / 7, 1e-9)
    TS_ASSERT_DELTA(p.getDSpacing(), 1.679251908362714, 1e-9)
    TS_ASSERT_DELTA(p.getScattering(), 1.1278852827212578, 1e-9)

    // calculate Q_lab from scattering and azimuthal to check values
    const auto k = 2 * M_PI / p.getWavelength();
    V3D qLab(1 - cos(p.getScattering()), -sin(p.getScattering()) * sin(p.getAzimuthal()),
             -sin(p.getScattering()) * cos(p.getAzimuthal()));
    qLab *= k;
    TS_ASSERT_DELTA(qLab.X(), 2, 1e-9)
    TS_ASSERT_DELTA(qLab.Y(), 1, 1e-9)
    TS_ASSERT_DELTA(qLab.Z(), 3, 1e-9)
  }

  void test_Qsample_gon_constructor_wavelength_fail() {
    // Identity transform
    Mantid::Kernel::Matrix<double> gon(3, 3, true);

    LeanElasticPeak p(V3D(0, 1, 0), gon);

    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(0, 1, 0))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(0, 1, 0))
    TS_ASSERT_DELTA(p.getWavelength(), 0, 1e-9)
    TS_ASSERT_DELTA(p.getDSpacing(), 2 * M_PI, 1e-9)
    TS_ASSERT_DELTA(p.getScattering(), 0, 1e-9)
  }

  void test_Qsample_wavelength_constructor() {
    LeanElasticPeak p(V3D(1, 2, 3), 1.);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(1, 2, 3))

    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getFinalEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getWavelength(), 1., 1e-9)
    TS_ASSERT_DELTA(p.getDSpacing(), 1.679251908362714, 1e-9)
    TS_ASSERT_DELTA(p.getScattering(), 0.6046731932, 1e-9)
  }

  void test_copyConstructor() {
    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanElasticPeak p(V3D(1, 2, 3), gon);
    // Default (not-explicit) copy constructor
    LeanElasticPeak p2(p);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), p2.getQSampleFrame());
    TS_ASSERT_EQUALS(p.getQLabFrame(), p2.getQLabFrame());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
    TS_ASSERT_EQUALS(p.getWavelength(), p2.getWavelength())
  }

  void test_ConstructorFromIPeakInterface() {
    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanElasticPeak p(V3D(1, 2, 3), gon);

    const Mantid::Geometry::IPeak &ipeak = p;
    LeanElasticPeak p2(ipeak);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), p2.getQSampleFrame());
    TS_ASSERT_EQUALS(p.getQLabFrame(), p2.getQLabFrame());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
    TS_ASSERT_EQUALS(p.getWavelength(), p2.getWavelength())
  }

  void test_HKL() {
    LeanElasticPeak p;
    p.setHKL(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS(p.getH(), 1.0);
    TS_ASSERT_EQUALS(p.getK(), 2.0);
    TS_ASSERT_EQUALS(p.getL(), 3.0);
    p.setH(5);
    p.setK(6);
    p.setL(7);
    TS_ASSERT_EQUALS(p.getH(), 5.0);
    TS_ASSERT_EQUALS(p.getK(), 6.0);
    TS_ASSERT_EQUALS(p.getL(), 7.0);
    p.setHKL(V3D(1.0, 2.0, 3.0));
    TS_ASSERT_EQUALS(p.getH(), 1.0);
    TS_ASSERT_EQUALS(p.getK(), 2.0);
    TS_ASSERT_EQUALS(p.getL(), 3.0);
    TS_ASSERT_EQUALS(p.getHKL(), V3D(1.0, 2.0, 3.0));
  }

  void test_isIndexed() {
    LeanElasticPeak p;
    TS_ASSERT_EQUALS(false, p.isIndexed());
    p.setHKL(1, 2, 3);
    TS_ASSERT_EQUALS(true, p.isIndexed());
  }

  void test_get_intensity_over_sigma() {
    const double intensity{100};
    const double sigma{10};
    LeanElasticPeak p;

    p.setIntensity(intensity);
    p.setSigmaIntensity(sigma);

    TS_ASSERT_EQUALS(p.getIntensityOverSigma(), intensity / sigma);
  }

  void test_get_intensity_over_sigma_empty_sigma() {
    const double intensity{10};
    const double sigma{0};
    LeanElasticPeak p;

    p.setIntensity(intensity);
    p.setSigmaIntensity(sigma);

    const double expectedResult{0.0};
    const double tolerance{1e-10};
    TS_ASSERT_DELTA(p.getIntensityOverSigma(), expectedResult, tolerance);
  }

  void test_get_energy() {
    LeanElasticPeak p;
    p.setWavelength(1.);

    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.8042024359, 1e-7);
    TS_ASSERT_DELTA(p.getFinalEnergy(), 81.8042024359, 1e-7);

    TS_ASSERT_EQUALS(p.getEnergyTransfer(), 0.);
  }

  void test_Peak_to_LeanElasticPeak_through_IPeak() {
    Instrument_sptr inst(ComponentCreationHelper::createTestInstrumentRectangular(5, 100));

    // Peak 3 is phi,chi,omega of 90,0,0; giving this matrix:
    Matrix<double> r(3, 3, false);
    r[0][2] = 1;
    r[1][1] = 1;
    r[2][0] = -1;

    Peak peak(inst, 19999, 2.0, V3D(1, 2, 3), r);
    peak.setRunNumber(1234);
    peak.setPeakNumber(42);
    peak.setIntensity(900);
    peak.setSigmaIntensity(30);
    peak.setBinCount(90);

    const IPeak &ipeak = peak;

    LeanElasticPeak leanpeak(ipeak);

    TS_ASSERT_EQUALS(leanpeak.getQSampleFrame(), peak.getQSampleFrame());
    V3D qsample = leanpeak.getQSampleFrame();
    TS_ASSERT_DELTA(qsample[0], -0.0759765444, 1e-7);
    TS_ASSERT_DELTA(qsample[1], -0.4855935910, 1e-7);
    TS_ASSERT_DELTA(qsample[2], -0.4855935910, 1e-7);

    TS_ASSERT_EQUALS(leanpeak.getQLabFrame(), peak.getQLabFrame());
    V3D qlab = leanpeak.getQLabFrame();
    TS_ASSERT_DELTA(qlab[0], -0.4855935910, 1e-7);
    TS_ASSERT_DELTA(qlab[1], -0.4855935910, 1e-7);
    TS_ASSERT_DELTA(qlab[2], 0.0759765444, 1e-7);

    TS_ASSERT_EQUALS(leanpeak.getHKL(), peak.getHKL());
    TS_ASSERT_EQUALS(leanpeak.getH(), 1);
    TS_ASSERT_EQUALS(leanpeak.getK(), 2);
    TS_ASSERT_EQUALS(leanpeak.getL(), 3);

    TS_ASSERT_EQUALS(leanpeak.getGoniometerMatrix(), peak.getGoniometerMatrix());
    TS_ASSERT_EQUALS(leanpeak.getGoniometerMatrix(), r);

    TS_ASSERT_DELTA(leanpeak.getInitialEnergy(), peak.getInitialEnergy(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getInitialEnergy(), 20.4510506207, 1e-7);

    TS_ASSERT_DELTA(leanpeak.getFinalEnergy(), peak.getFinalEnergy(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getFinalEnergy(), 20.4510506207, 1e-7);

    TS_ASSERT_DELTA(leanpeak.getWavelength(), peak.getWavelength(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getWavelength(), 2.0, 1e-7);

    TS_ASSERT_DELTA(leanpeak.getDSpacing(), peak.getDSpacing(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getDSpacing(), 9.0938998166, 1e-7);

    TS_ASSERT_DELTA(leanpeak.getScattering(), peak.getScattering(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getScattering(), 0.2203733065, 1e-7);

    TS_ASSERT_DELTA(leanpeak.getAzimuthal(), peak.getAzimuthal(), 1e-7);
    TS_ASSERT_DELTA(leanpeak.getAzimuthal(), 0.7853981637, 1e-7);

    TS_ASSERT_EQUALS(leanpeak.getRunNumber(), peak.getRunNumber());
    TS_ASSERT_EQUALS(leanpeak.getRunNumber(), 1234);

    TS_ASSERT_EQUALS(leanpeak.getPeakNumber(), peak.getPeakNumber());
    TS_ASSERT_EQUALS(leanpeak.getPeakNumber(), 42);

    TS_ASSERT_EQUALS(leanpeak.getIntensity(), peak.getIntensity());
    TS_ASSERT_EQUALS(leanpeak.getIntensity(), 900);

    TS_ASSERT_EQUALS(leanpeak.getSigmaIntensity(), peak.getSigmaIntensity());
    TS_ASSERT_EQUALS(leanpeak.getSigmaIntensity(), 30);
    TS_ASSERT_EQUALS(leanpeak.getIntensityOverSigma(), 30);

    TS_ASSERT_EQUALS(leanpeak.getBinCount(), peak.getBinCount());
    TS_ASSERT_EQUALS(leanpeak.getBinCount(), 90);
  }
};
