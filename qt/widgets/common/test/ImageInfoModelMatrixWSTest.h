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
#include "MantidQtWidgets/Common/QStringUtils.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::DataObjects;
using MantidQt::API::toQStringInternal;

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
    const std::array<QString, 22> expectList{
        "Signal",
        "7",
        "Spec Num",
        "4",
        "Time-of-flight" + toQStringInternal(L"(\u03bcs)"),
        "2",
        "Det ID",
        "4",
        "L2(m)",
        "5.009",
        "TwoTheta(Deg)",
        "3.43",
        "Azimuthal(Deg)",
        "90",
        "Wavelength" + toQStringInternal(L"(\u212b)"),
        "0.0003",
        "Energy(meV)",
        "817312177.3087",
        "d-Spacing" + toQStringInternal(L"(\u212b)"),
        "0.0053",
        "|Q|" + toQStringInternal(L"(\u212b\u207b\u00b9)"),
        "1190.0137"};

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

    const std::array<QString, 6> expectList{"Signal",
                                            "7",
                                            "Spec Num",
                                            "4",
                                            "Time-of-flight" +
                                                toQStringInternal(L"(\u03bcs)"),
                                            "2"};

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

    const std::array<QString, 22> expectList{
        "Signal",
        "-",
        "Spec Num",
        "-",
        "Time-of-flight" + toQStringInternal(L"(\u03bcs)"),
        "-",
        "Det ID",
        "-",
        "L2(m)",
        "-",
        "TwoTheta(Deg)",
        "-",
        "Azimuthal(Deg)",
        "-",
        "Wavelength" + toQStringInternal(L"(\u212b)"),
        "-",
        "Energy(meV)",
        "-",
        "d-Spacing" + toQStringInternal(L"(\u212b)"),
        "-",
        "|Q|" + toQStringInternal(L"(\u212b\u207b\u00b9)"),
        "-"};

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
