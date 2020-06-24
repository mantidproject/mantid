// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;

class ImageInfoModelMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoModelMDTest *createSuite() {
    return new ImageInfoModelMDTest();
  }
  static void destroySuite(ImageInfoModelMDTest *suite) { delete suite; }

  void test_getInfoList_with_md_ws() {
    ImageInfoModelMD model;

    auto list = model.getInfoList(2, 4, 7);

    const std::array<QString, 6> expectList{"x", "2", "y", "4", "Signal", "7"};
    TS_ASSERT_EQUALS(expectList.size(), list.size())
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_returns_dashes_when_given_double_max() {
    ImageInfoModelMD model;

    auto list = model.getInfoList(2, std::numeric_limits<double>::max(), 7);

    const std::array<QString, 6> expectList{"x", "2", "y", "-", "Signal", "7"};
    TS_ASSERT_EQUALS(expectList.size(), list.size())
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }
};
