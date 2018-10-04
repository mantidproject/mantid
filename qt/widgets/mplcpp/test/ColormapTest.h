#ifndef MPLCPP_COLORMAPTEST_H
#define MPLCPP_COLORMAPTEST_H

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"

#include <cxxtest/TestSuite.h>

using Mantid::PythonInterface::PythonRuntimeError;
using MantidQt::Widgets::MplCpp::Colormap;
using MantidQt::Widgets::MplCpp::Python::Object;
using MantidQt::Widgets::MplCpp::getCMap;

class ColormapTest : public CxxTest::TestSuite {
public:
  static ColormapTest *createSuite() { return new ColormapTest; }
  static void destroySuite(ColormapTest *suite) { delete suite; }

public:
  // ----------------------- Success tests ------------------------
  void testgetCMapKnownCMapIsSuccesful() {
    TS_ASSERT_THROWS_NOTHING(getCMap("jet"));
  }

  void testConstructionColorMapInstanceIsSuccessful() {
    auto jet = getCMap("jet");
    TS_ASSERT_EQUALS("jet", jet.pyobj().attr("name"));
  }

  // ----------------------- Failure tests ------------------------
  void testgetCMapWithUnknownCMapThrowsException() {
    TS_ASSERT_THROWS(getCMap("AnUnknownName"), PythonRuntimeError);
  }

  void testConstructionWithNonColorMapObjectThrows() {
    TS_ASSERT_THROWS(Colormap cmap(Object{}), std::invalid_argument);
  }
};

#endif // MPLCPP_COLORMAPTEST_H
