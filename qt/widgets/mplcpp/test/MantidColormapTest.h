// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDCOLORMAPTEST_H
#define MANTIDCOLORMAPTEST_H

#include "MantidQtWidgets/MplCpp/MantidColorMap.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::MantidColorMap;

class MantidColormapTest : public CxxTest::TestSuite {
public:
  static MantidColormapTest *createSuite() { return new MantidColormapTest; }
  static void destroySuite(MantidColormapTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testExistsReturnsEmptyStringIfMapExists() {
    TS_ASSERT_EQUALS("jet", MantidColorMap::exists("jet"));
  }

  // ----------------- failure tests ---------------------

  void testExistsThrowsIfMapDoesNotExist() {
    using Mantid::PythonInterface::PythonException;
    TS_ASSERT_THROWS(MantidColorMap::exists("NotAColormap"),
                     const PythonException &);
  }
};

#endif // MANTIDCOLORMAPTEST_H
