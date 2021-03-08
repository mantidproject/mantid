// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LeanElasticPeaksWorkspaceTest : public CxxTest::TestSuite {
private:
  class TestableLeanElasticPeaksWorkspace : public LeanElasticPeaksWorkspace {
  public:
    TestableLeanElasticPeaksWorkspace(const LeanElasticPeaksWorkspace &other)
        : LeanElasticPeaksWorkspace(other) {}
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeanElasticPeaksWorkspaceTest *createSuite() {
    return new LeanElasticPeaksWorkspaceTest();
  }
  static void destroySuite(LeanElasticPeaksWorkspaceTest *suite) { delete suite; }

  void test_defaultConstructor() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    TS_ASSERT_EQUALS(pw->columnCount(), 20);
    TS_ASSERT_EQUALS(pw->rowCount(), 1);
    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_copyConstructor() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    auto pw2 = std::make_shared<TestableLeanElasticPeaksWorkspace>(*pw);
    TS_ASSERT_EQUALS(pw2->rowCount(), 1);
    TS_ASSERT_EQUALS(pw2->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw2->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_clone() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    auto pw2 = pw->clone();
    TS_ASSERT_EQUALS(pw2->rowCount(), 1);
    TS_ASSERT_EQUALS(pw2->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw2->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_addRemovePeaks() {
    // build peaksworkspace (note number of peaks = 1)
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();

    // add peaks
    LeanElasticPeak p(V3D(1, 0, 0));
    LeanElasticPeak p2(V3D(0, 1, 0));
    LeanElasticPeak p3(V3D(0, 0, 1));
    pw->addPeak(p);
    pw->addPeak(p2);
    pw->addPeak(p3);

    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 3);

    // number of peaks = 4, now remove 3
    std::vector<int> badPeaks{0, 2, 3};
    pw->removePeaks(std::move(badPeaks));
    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 1);
  }

  void test_sort() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p0(V3D(1, 0, 0), 3.0);
    LeanElasticPeak p1(V3D(1, 0, 0), 4.0);
    LeanElasticPeak p2(V3D(1, 0, 0), 5.0);
    LeanElasticPeak p3(V3D(1, 1, 0), 3.0);
    LeanElasticPeak p4(V3D(2, 0, 0), 3.0);
    pw->addPeak(p0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector<std::pair<std::string, bool>> criteria;
    // Sort by ascending wavelength then decending dspacing
    criteria.emplace_back(std::pair<std::string, bool>("wavelength", false));
    criteria.emplace_back(std::pair<std::string, bool>("dspacing", false));
    pw->sort(criteria);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(0).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getDSpacing(), M_PI * M_SQRT2, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getDSpacing(), M_PI, 1e-5);

    // Sort by wavelength ascending then decending dspacing
    criteria.clear();
    criteria.emplace_back(std::pair<std::string, bool>("wavelength", true));
    criteria.emplace_back(std::pair<std::string, bool>("dspacing", false));
    pw->sort(criteria);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(0).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getDSpacing(), M_PI * M_SQRT2, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getDSpacing(), M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getDSpacing(), 2 * M_PI, 1e-5);
  }
};
