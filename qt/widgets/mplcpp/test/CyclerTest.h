// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPPTEST_CYCLERTEST_H
#define MPLCPPTEST_CYCLERTEST_H

#include "MantidQtWidgets/MplCpp/Cycler.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Cycler;
using MantidQt::Widgets::MplCpp::cycler;
using namespace MantidQt::Widgets::Common;

class CyclerTest : public CxxTest::TestSuite {
public:
  static CyclerTest *createSuite() { return new CyclerTest; }
  static void destroySuite(CyclerTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testCyclerFactoryFunctionReturnsExpectedCycler() {
    const std::string label("colors");
    auto colors = cycler(label.c_str(), "rgb");

    auto toDict = [&label](const char *value) {
      return Python::NewRef(Py_BuildValue("{ss}", label.c_str(), value));
    };
    TS_ASSERT_EQUALS(toDict("r"), colors());
    TS_ASSERT_EQUALS(toDict("g"), colors());
    TS_ASSERT_EQUALS(toDict("b"), colors());
    TS_ASSERT_EQUALS(toDict("r"), colors());
  }
  // ----------------- failure tests ---------------------

  void testConstructWithNonCyclerThrowsInvalidArgument() {
    Python::Object none;
    TS_ASSERT_THROWS(Cycler cycler(none), std::invalid_argument);
  }
};

#endif // MPLCPPTEST_CYCLERTEST_H
