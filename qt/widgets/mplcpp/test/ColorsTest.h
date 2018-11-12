// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORSTEST_H
#define MPLCPP_COLORSTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/Colors.h"

using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::NormalizeBase;
using MantidQt::Widgets::MplCpp::PowerNorm;
using MantidQt::Widgets::MplCpp::SymLogNorm;

class ColorsTest : public CxxTest::TestSuite {
public:
  static ColorsTest *createSuite() { return new ColorsTest; }
  static void destroySuite(ColorsTest *suite) { delete suite; }

public:
  void testDefaultNormalizeAndAutoscale() {
    Normalize norm;
    const double vmin(-1), vmax(1);
    auto range = norm.autoscale(std::make_tuple(vmin, vmax));

    TS_ASSERT_EQUALS(vmin, std::get<0>(range));
    TS_ASSERT_EQUALS(vmax, std::get<1>(range));
    assertColorLimits(norm, vmin, vmax);
  }

  void testNormalizeWithLimits() {
    const double vmin(-1), vmax(1);
    Normalize norm(vmin, vmax);
    assertColorLimits(norm, vmin, vmax);
  }

  void testDefaultSymLogNormAndAutoscale() {
    SymLogNorm norm(0.001, 2);
    const double vmin(-1), vmax(1);
    auto range = norm.autoscale(std::make_tuple(vmin, vmax));

    TS_ASSERT_EQUALS(vmin, std::get<0>(range));
    TS_ASSERT_EQUALS(vmax, std::get<1>(range));
    assertColorLimits(norm, vmin, vmax);
  }

  void testSymLogNorm() {
    const double vmin(-1), vmax(1);
    SymLogNorm norm(0.001, 2, vmin, vmax);
    // No public api method for access linscale
    TS_ASSERT_EQUALS(0.001, norm.pyobj().attr("linthresh"));
    assertColorLimits(norm, vmin, vmax);
  }

  void testDefaultPowerNormAndAutoscale() {
    PowerNorm norm(2);
    // vmin will get rescaled as -1 is not valid
    const double vmin(-1), vmax(1), validMin(0.);
    auto range = norm.autoscale(std::make_tuple(vmin, vmax));

    TS_ASSERT_EQUALS(validMin, std::get<0>(range));
    TS_ASSERT_EQUALS(vmax, std::get<1>(range));
    assertColorLimits(norm, validMin, vmax);
  }

  void testPowerNorm() {
    const double vmin(-1), vmax(1);
    PowerNorm norm(2, vmin, vmax);
    TS_ASSERT_EQUALS(2, norm.pyobj().attr("gamma"));
    assertColorLimits(norm, vmin, vmax);
  }

private:
  void assertColorLimits(const NormalizeBase &norm, double vmin, double vmax) {
    TS_ASSERT_EQUALS(vmin, norm.pyobj().attr("vmin"));
    TS_ASSERT_EQUALS(vmax, norm.pyobj().attr("vmax"));
  }
};

#endif // MPLCPP_COLORSTEST_H
