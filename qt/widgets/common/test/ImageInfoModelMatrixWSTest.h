// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::DataObjects;

class ImageInfoModelMatrixWSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoModelMatrixWSTest *createSuite() {
    return new ImageInfoModelMatrixWSTest();
  }
  static void destroySuite(ImageInfoModelMatrixWSTest *suite) { delete suite; }

  void test_construct() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    TS_ASSERT_THROWS_NOTHING(ImageInfoModelMatrixWS model(workspace))
  }

  void test_getInfoList() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model(workspace);

    auto list = model.getInfoList(2, 4, 7);

    const std::string expectList[]{
        "Value",     "7",      "Spec Num",  "5",         "Time-of-flight",
        "2",         "Det ID", "5",         "L2",        "5.016",
        "TwoTheta",  "4.6",    "Azimuthal", "90",        "Wavelength",
        "0.0003163", "Energy", "8.178e+08", "d-Spacing", "0.003963",
        "|Q|",       "1585"};

    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_with_no_instrument() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, false);
    workspace->getAxis(0)->setUnit("TOF");
    ImageInfoModelMatrixWS model(workspace);

    auto list = model.getInfoList(2, 4, 7);

    const std::string expectList[]{"Value",          "7", "Spec Num", "5",
                                   "Time-of-flight", "2", "Det ID",   "5"};
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_ws_return_nothing_if_x_out_of_ws_range() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model(workspace);

    auto list1 = model.getInfoList(-1, 4, 7);
    auto list2 = model.getInfoList(10, 4, 7);

    TS_ASSERT_EQUALS(0, list1.size())
    TS_ASSERT_EQUALS(0, list2.size())
  }

  void test_getInfoList_return_nothing_if_y_out_of_range() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model(workspace);
    auto list1 = model.getInfoList(2, -1, 7);
    auto list2 = model.getInfoList(2, 10, 7);
    TS_ASSERT_EQUALS(0, list1.size())
    TS_ASSERT_EQUALS(0, list2.size())
  }
};
