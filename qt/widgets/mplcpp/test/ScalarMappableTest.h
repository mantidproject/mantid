// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_SCALARMAPPABLETEST_H
#define MPLCPP_SCALARMAPPABLETEST_H

#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QRgb>

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::PowerNorm;
using MantidQt::Widgets::MplCpp::ScalarMappable;
using MantidQt::Widgets::MplCpp::getCMap;
namespace Python = MantidQt::Widgets::MplCpp::Python;

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

  void testSetCMapAsStringResetsColormap() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setCmap("coolwarm");

    TS_ASSERT_EQUALS("coolwarm", mappable.pyobj().attr("cmap").attr("name"));
  }

  void testSetCMapResetsColormap() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setCmap(getCMap("coolwarm"));

    TS_ASSERT_EQUALS("coolwarm", mappable.pyobj().attr("cmap").attr("name"));
  }

  void testSetNormResetsNormalizeInstance() {
    ScalarMappable mappable(Normalize(-1, 1), "jet");
    mappable.setNorm(PowerNorm(2, 0, 1));

    auto norm = Python::Object(mappable.pyobj().attr("norm"));
    TS_ASSERT(PyObject_HasAttrString(norm.ptr(), "gamma"));
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
