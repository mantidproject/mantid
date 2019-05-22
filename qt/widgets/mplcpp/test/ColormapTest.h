// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORMAPTEST_H
#define MPLCPP_COLORMAPTEST_H

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"

#include <cxxtest/TestSuite.h>

using Mantid::PythonInterface::PythonException;
using MantidQt::Widgets::MplCpp::Colormap;
using MantidQt::Widgets::MplCpp::cmapExists;
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

  void testcmapExistsForKnownCMapReturnsTrue() { TS_ASSERT(cmapExists("jet")); }

  void testcmapExistsForUnknownCMapReturnsFalse() {
    TS_ASSERT(!cmapExists("NotAKnownCMap"));
  }

  void testConstructionColorMapInstanceIsSuccessful() {
    auto jet = getCMap("jet");
    TS_ASSERT_EQUALS("jet", jet.pyobj().attr("name"));
  }

  // ----------------------- Failure tests ------------------------
  void testgetCMapWithUnknownCMapThrowsException() {
    TS_ASSERT_THROWS(getCMap("AnUnknownName"), PythonException);
  }

  void testConstructionWithNonColorMapObjectThrows() {
    TS_ASSERT_THROWS(Colormap cmap(MantidQt::Widgets::Common::Python::Object{}),
                     const std::invalid_argument &);
  }
};

#endif // MPLCPP_COLORMAPTEST_H
