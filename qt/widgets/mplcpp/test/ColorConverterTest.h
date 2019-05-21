// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORCONVERTERTEST_H
#define MPLCPP_COLORCONVERTERTEST_H

#include "MantidQtWidgets/MplCpp/ColorConverter.h"
#include <QRgb>
#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::ColorConverter;
using namespace MantidQt::Widgets::Common;

class ColorConverterTest : public CxxTest::TestSuite {
public:
  static ColorConverterTest *createSuite() { return new ColorConverterTest; }
  static void destroySuite(ColorConverterTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testKnownColorIsTranslated() {
    // matplotlib cyan=(0,0.75,0.75). See matplotlib.colors
    auto color = ColorConverter::toRGB(Python::NewRef(Py_BuildValue("s", "c")));
    TS_ASSERT_EQUALS(0, color.red());
    TS_ASSERT_EQUALS(191, color.blue());
    TS_ASSERT_EQUALS(191, color.green());
  }
};

#endif // MPLCPP_COLORCONVERTERTEST_H
