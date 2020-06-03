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

  void test_getInfoList() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model;
    model.setWorkspace(workspace);

    auto list = model.getInfoList(2, 4, 7);
    const std::array<std::string, 22> expectList{
        "Signal",    "7",      "Spec Num",  "4",         "Time-of-flight",
        "2",         "Det ID", "4",         "L2",        "5.009",
        "TwoTheta",  "3.4",    "Azimuthal", "90",        "Wavelength",
        "0.0003164", "Energy", "8.173e+08", "d-Spacing", "0.00528",
        "|Q|",       "1190"};

    TS_ASSERT_EQUALS(expectList.size(), list.size())
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_with_no_instrument() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, false);
    workspace->getAxis(0)->setUnit("TOF");
    ImageInfoModelMatrixWS model;
    model.setWorkspace(workspace);

    auto list = model.getInfoList(2, 4, 7);

    const std::array<std::string, 6> expectList{
        "Signal", "7", "Spec Num", "4", "Time-of-flight", "2"};

    TS_ASSERT_EQUALS(expectList.size(), list.size())
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_ws_return_nothing_if_x_out_of_ws_range() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model;
    model.setWorkspace(workspace);

    auto list1 = model.getInfoList(-1, 4, 7);
    auto list2 = model.getInfoList(10, 4, 7);

    TS_ASSERT_EQUALS(0, list1.size())
    TS_ASSERT_EQUALS(0, list2.size())
  }

  void test_getInfoList_return_nothing_if_y_out_of_range() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model;
    model.setWorkspace(workspace);
    auto list1 = model.getInfoList(2, -1, 7);
    auto list2 = model.getInfoList(2, 11, 7);
    TS_ASSERT_EQUALS(0, list1.size())
    TS_ASSERT_EQUALS(0, list2.size())
  }

  void test_getInfoList_returns_dashes_if_includeValues_is_false() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model;
    model.setWorkspace(workspace);

    auto list = model.getInfoList(2, 4, 7, false);

    const std::array<std::string, 22> expectList{
        "Signal",    "-", "Spec Num",   "-", "Time-of-flight", "-",
        "Det ID",    "-", "L2",         "-", "TwoTheta",       "-",
        "Azimuthal", "-", "Wavelength", "-", "Energy",         "-",
        "d-Spacing", "-", "|Q|",        "-"};

    TS_ASSERT_EQUALS(expectList.size(), list.size())
    for (size_t i = 0; i < list.size(); ++i) {
      TS_ASSERT_EQUALS(expectList[i], list[i]);
    }
  }

  void test_getInfoList_returns_nothing_if_no_workspace_has_been_set() {
    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            10, 10, true, false, true, "workspace", false);
    ImageInfoModelMatrixWS model;

    auto list = model.getInfoList(2, 4, 7);

    TS_ASSERT_EQUALS(0, list.size())
  }
};
