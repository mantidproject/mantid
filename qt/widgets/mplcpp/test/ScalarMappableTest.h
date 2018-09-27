#ifndef MPLCPP_SCALARMAPPABLETEST_H
#define MPLCPP_SCALARMAPPABLETEST_H

#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QRgb>

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::ScalarMappable;
using MantidQt::Widgets::MplCpp::getCMap;

class ScalarMappableTest : public CxxTest::TestSuite {
public:
  static ScalarMappableTest *createSuite() { return new ScalarMappableTest; }
  static void destroySuite(ScalarMappableTest *suite) { delete suite; }

public:
  // ----------------------- Success tests ------------------------
  void testConstructionWithValidCMapAndNormalize() {
    TS_ASSERT_THROWS_NOTHING(
        ScalarMappable mappable(Normalize(-1, 1), getCMap("jet")));
  }

  void testConstructionWithValidCMapAsStringAndNormalize() {
    TS_ASSERT_THROWS_NOTHING(ScalarMappable mappable(Normalize(-1, 1), "jet"));
  }

  void testtoRGBAWithNoAlphaGivesDefault() {
    ScalarMappable mappable(Normalize(-1, 1), getCMap("jet"));
    auto rgba = mappable.toRGBA(0.0);
    TS_ASSERT_EQUALS(124, qRed(rgba));
    TS_ASSERT_EQUALS(255, qGreen(rgba));
    TS_ASSERT_EQUALS(121, qBlue(rgba));
    TS_ASSERT_EQUALS(255, qAlpha(rgba));
  }

  void testtoRGBAWithAlpha() {
    ScalarMappable mappable(Normalize(-1, 1), getCMap("jet"));
    auto rgba = mappable.toRGBA(0.0, 0.5);
    TS_ASSERT_EQUALS(124, qRed(rgba));
    TS_ASSERT_EQUALS(255, qGreen(rgba));
    TS_ASSERT_EQUALS(121, qBlue(rgba));
    TS_ASSERT_EQUALS(127, qAlpha(rgba));
  }
};

#endif // MPLCPP_SCALARMAPPABLETEST_H
