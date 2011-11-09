#ifndef MANTIDQT_API_MANTIDCOLORMAPTEST_H_
#define MANTIDQT_API_MANTIDCOLORMAPTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include "MantidQtAPI/MantidColorMap.h"
#include <limits>
#include <QRgb>

class MantidColorMapTest : public CxxTest::TestSuite
{
public:

  /// Check default color map
  void test_constructor()
  {
    MantidColorMap map;
    QRgb col;
    col = map.rgb( QwtDoubleInterval( 0.0, 1.0 ), 0.0);
    TSM_ASSERT_EQUALS("Default min color.", col, qRgb(0, 172, 252) );
    col = map.rgb( QwtDoubleInterval( 0.0, 1.0 ), 1.0);
    TSM_ASSERT_EQUALS("Default max color.", col, qRgb(255,255,255) );
  }

  void test_nan_color()
  {
    MantidColorMap map;
    map.setNanColor(123, 23, 34);
    QRgb col;
    double nan = std::numeric_limits<double>::quiet_NaN();
    col = map.rgb( QwtDoubleInterval( 0.0, 1.0 ), nan);
    TSM_ASSERT_EQUALS("Passing NAN to rgb returns the set color.", col, qRgb(123, 23, 34) );
  }
};


#endif /* MANTIDQT_API_MANTIDCOLORMAPTEST_H_ */
