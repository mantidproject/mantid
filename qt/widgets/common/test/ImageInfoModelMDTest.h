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
  static ImageInfoModelMDTest *createSuite() { return new ImageInfoModelMDTest(); }
  static void destroySuite(ImageInfoModelMDTest *suite) { delete suite; }

  void test_info_with_md_ws() {
    ImageInfoModelMD model;

    auto info = model.info(2, 4, 7.5);

    assertInfoMatches(info, {"2.0000", "4.0000", "7.5000"});
  }

  void test_info_returns_dashes_when_given_double_max() {
    ImageInfoModelMD model;

    auto info = model.info(2, std::numeric_limits<double>::max(), 7);

    assertInfoMatches(info, {"2.0000", "-", "7.0000"});
  }

private:
  void assertInfoMatches(const ImageInfoModel::ImageInfo &info, const std::vector<std::string> &expectedValues) {
    constexpr std::array<const char *, 3> expectedHeaders{"x", "y", "Signal"};
    TS_ASSERT_EQUALS(expectedHeaders.size(), info.size())
    for (int i = 0; i < info.size(); ++i) {
      TS_ASSERT_EQUALS(expectedHeaders[i], info.name(i).toStdString());
      TS_ASSERT_EQUALS(expectedValues[i], info.value(i).toLatin1().constData());
    }
  }
};
