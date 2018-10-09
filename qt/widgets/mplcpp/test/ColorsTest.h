#ifndef MPLCPP_COLORSTEST_H
#define MPLCPP_COLORSTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/Colors.h"

using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::PowerNorm;
using MantidQt::Widgets::MplCpp::SymLogNorm;

class ColorsTest : public CxxTest::TestSuite {
public:
  static ColorsTest *createSuite() { return new ColorsTest; }
  static void destroySuite(ColorsTest *suite) { delete suite; }

public:
  void testNormalize() {
    Normalize norm(-1, 1);
    TS_ASSERT_EQUALS(-1, norm.pyobj().attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.pyobj().attr("vmax"));
  }

  void testSymLogNorm() {
    SymLogNorm norm(0.001, 2, -1, 1);
    // No public api method for access linscale
    TS_ASSERT_EQUALS(0.001, norm.pyobj().attr("linthresh"));
    TS_ASSERT_EQUALS(-1, norm.pyobj().attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.pyobj().attr("vmax"));
  }

  void testPowerNorm() {
    PowerNorm norm(2, -1, 1);
    TS_ASSERT_EQUALS(2, norm.pyobj().attr("gamma"));
    TS_ASSERT_EQUALS(-1, norm.pyobj().attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.pyobj().attr("vmax"));
  }
};

#endif // MPLCPP_COLORSTEST_H
