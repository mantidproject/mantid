// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::DataObjects;

class ImageInfoModelMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoModelMDTest *createSuite() {
    return new ImageInfoModelMDTest();
  }
  static void destroySuite(ImageInfoModelMDTest *suite) { delete suite; }

  void test_construct_with_md_workspace() {
    IMDWorkspace_sptr workspace =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    TS_ASSERT_THROWS_NOTHING(ImageInfoModelMD model(workspace))
  }

  void test_getInfoList_with_md_ws() {
    IMDWorkspace_sptr workspace =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    ImageInfoModelMD model(workspace);

    auto list = model.getInfoList(2, 4, 7);

    const std::string expectList[]{"x", "2", "y", "4", "Value", "7"};
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }
};
