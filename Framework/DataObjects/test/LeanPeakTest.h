// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/V3D.h"

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/LeanPeak.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LeanPeakTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeanPeakTest *createSuite() { return new LeanPeakTest(); }
  static void destroySuite(LeanPeakTest *suite) { delete suite; }

  void test_default_constructor() {
    LeanPeak p;
    TS_ASSERT_EQUALS(p.getH(), 0.0)
    TS_ASSERT_EQUALS(p.getK(), 0.0)
    TS_ASSERT_EQUALS(p.getL(), 0.0)
    TS_ASSERT_EQUALS(p.getInitialEnergy(), 0.0)
    TS_ASSERT_EQUALS(p.getFinalEnergy(), 0.0)
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(0, 0, 0))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D())

    TS_ASSERT_EQUALS(p.getDetectorID(), -1)
    TS_ASSERT_THROWS(p.getDetector(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.getInstrument(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.findDetector(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.getDetectorPosition(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.getDetectorPositionNoCheck(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.getDetPos(), const std::runtime_error &)
    TS_ASSERT_THROWS(p.getSamplePos(), const std::runtime_error &)
    TS_ASSERT(std::isnan(p.getTOF()))
    TS_ASSERT(std::isnan(p.getScattering()))
    TS_ASSERT(std::isnan(p.getAzimuthal()))
    TS_ASSERT(std::isnan(p.getL1()))
    TS_ASSERT(std::isnan(p.getL2()))
  }

  void test_Qsample_constructor() {
    LeanPeak p(V3D(1, 2, 3));
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

    LeanPeak p(V3D(1, 2, 3), gon);

    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(2, 1, 3))
  }

  void test_Qsample_wavelength_constructor() {
    LeanPeak p(V3D(1, 2, 3), 1.);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(1, 2, 3))

    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getFinalEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getWavelength(), 1., 1e-9)
    TS_ASSERT_DELTA(p.getDSpacing(), 1.679251908362714, 1e-9)
    TS_ASSERT_DELTA(p.getScattering(), 0.6046731932, 1e-9)
  }

  void test_Qsample_gon_wavelength_constructor() {

    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanPeak p(V3D(1, 2, 3), gon, 1.);

    TS_ASSERT_EQUALS(p.getQSampleFrame(), V3D(1, 2, 3))
    TS_ASSERT_EQUALS(p.getQLabFrame(), V3D(2, 1, 3))
    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getFinalEnergy(), 81.8042024359, 1e-5)
    TS_ASSERT_DELTA(p.getWavelength(), 1., 1e-9)
  }

  void test_copyConstructor() {
    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanPeak p(V3D(1, 2, 3), gon, 1.);
    // Default (not-explicit) copy constructor
    LeanPeak p2(p);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), p2.getQSampleFrame());
    TS_ASSERT_EQUALS(p.getQLabFrame(), p2.getQLabFrame());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
  }

  void test_ConstructorFromIPeakInterface() {
    // This goniometer should just swap x and y of q
    Mantid::Kernel::Matrix<double> gon(3, 3);
    gon[0][1] = 1;
    gon[1][0] = 1;
    gon[2][2] = 1;

    LeanPeak p(V3D(1, 2, 3), gon, 1.);

    const Mantid::Geometry::IPeak &ipeak = p;
    LeanPeak p2(ipeak);
    TS_ASSERT_EQUALS(p.getQSampleFrame(), p2.getQSampleFrame());
    TS_ASSERT_EQUALS(p.getQLabFrame(), p2.getQLabFrame());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
  }
};
