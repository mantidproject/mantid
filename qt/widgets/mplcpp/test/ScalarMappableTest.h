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

  void testSetCLimSetsMinAndMaxWhenProvided() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setClim(-10, 10);
    auto norm = mappable.pyobj().attr("norm");
    TS_ASSERT_EQUALS(-10, norm.attr("vmin"));
    TS_ASSERT_EQUALS(10, norm.attr("vmax"));
  }

  void testSetCLimSetsMinOnlyWhenMaxNotProvided() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setClim(-10);
    auto norm = mappable.pyobj().attr("norm");
    TS_ASSERT_EQUALS(-10, norm.attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.attr("vmax"));
  }

  void testSetCLimSetsMaxOnlyWhenMinNotProvided() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setClim(boost::none, 10);
    auto norm = mappable.pyobj().attr("norm");
    TS_ASSERT_EQUALS(-1, norm.attr("vmin"));
    TS_ASSERT_EQUALS(10, norm.attr("vmax"));
  }

  void testSetCLimSetsNothingWhenNothingProvided() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setClim(boost::none, boost::none);
    auto norm = mappable.pyobj().attr("norm");
    TS_ASSERT_EQUALS(-1, norm.attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.attr("vmax"));

    mappable.setClim();
    TS_ASSERT_EQUALS(-1, norm.attr("vmin"));
    TS_ASSERT_EQUALS(1, norm.attr("vmax"));
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
