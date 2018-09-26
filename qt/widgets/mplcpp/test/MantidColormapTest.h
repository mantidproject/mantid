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
    TS_ASSERT_THROWS(MantidColorMap::exists("NotAColormap"),
                     std::runtime_error);
  }
};

#endif // MANTIDCOLORMAPTEST_H
