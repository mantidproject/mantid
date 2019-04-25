// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_MANTIDCOLORMAPTEST_H_
#define MANTIDQT_API_MANTIDCOLORMAPTEST_H_

#include "MantidQtWidgets/Plotting/Qwt/MantidColorMap.h"
#include <QRgb>
#include <cxxtest/TestSuite.h>
#include <limits>

class MantidColorMapTest : public CxxTest::TestSuite {
public:
  /// Check default color map
  void test_constructor() {
    MantidColorMap map;
    QRgb col;
    col = map.rgb(QwtDoubleInterval(0.0, 1.0), 0.0);
    TSM_ASSERT_EQUALS("Default min color.", col, qRgb(0, 170, 252));
    col = map.rgb(QwtDoubleInterval(0.0, 1.0), 1.0);
    TSM_ASSERT_EQUALS("Default max color.", col, qRgb(255, 255, 255));
    TSM_ASSERT_EQUALS("Default map is linear", map.getScaleType(),
                      MantidColorMap::ScaleType::Log10);
  }

  void test_normalize_linear() {
    MantidColorMap map;
    QwtDoubleInterval range(10.0, 20.0);
    map.changeScaleType(MantidColorMap::ScaleType::Linear);
    TS_ASSERT_DELTA(map.normalize(range, 15.), 0.5, 1e-5);
  }

  void test_normalize_log() {
    MantidColorMap map;
    QwtDoubleInterval range(1.0, 10000.0);
    map.changeScaleType(MantidColorMap::ScaleType::Log10);
    TS_ASSERT_DELTA(map.normalize(range, 1000.), 0.75, 1e-5);
  }

  void test_normalize_power() {
    MantidColorMap map;
    QwtDoubleInterval range(10.0, 20.0);
    map.changeScaleType(MantidColorMap::ScaleType::Power);
    map.setNthPower(2.0);
    TS_ASSERT_DELTA(map.normalize(range, 16.), 0.52, 1e-5);
  }

  /// Setting a NAN color
  void test_nan_color() {
    MantidColorMap map;
    map.setNanColor(123, 23, 34);
    QRgb col;
    QwtDoubleInterval range(10.0, 20.0);
    double nan = std::numeric_limits<double>::quiet_NaN();
    col = map.rgb(range, nan);
    TSM_ASSERT_EQUALS("Passing NAN to rgb returns the set color.", col,
                      qRgb(123, 23, 34));
  }

  void test_colorIndex() {
    MantidColorMap map;
    QwtDoubleInterval range(10.0, 20.0);
    double nan = std::numeric_limits<double>::quiet_NaN();
    TSM_ASSERT_EQUALS("Color index is 0 for NAN", map.colorIndex(range, nan),
                      0);
    TSM_ASSERT_EQUALS("Color index is 1 for small numbers",
                      map.colorIndex(range, -123.0), 1);
    TSM_ASSERT_EQUALS("Color index is 255 for large numbers",
                      map.colorIndex(range, +123.0), 255);
  }
};

#endif /* MANTIDQT_API_MANTIDCOLORMAPTEST_H_ */
