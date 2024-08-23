// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"
#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using Mantid::DataObjects::MDEventsTestHelper::makeFakeMDEventWorkspace;
using MantidQt::MantidWidgets::IImageInfoWidget;
using MantidQt::MantidWidgets::ImageInfoModel;
using MantidQt::MantidWidgets::ImageInfoModelMatrixWS;
using MantidQt::MantidWidgets::ImageInfoModelMD;
using MantidQt::MantidWidgets::ImageInfoPresenter;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockImageInfoView : public IImageInfoWidget {
public:
  MOCK_METHOD4(cursorAt,
               void(const double x, const double y, const double signal, const QMap<QString, QString> &extraValues));
  MOCK_METHOD1(showInfo, void(const ImageInfoModel::ImageInfo &info));
  MOCK_METHOD1(setWorkspace, void(const Mantid::API::Workspace_sptr &ws));
  MOCK_METHOD1(setRowCount, void(const int count));
  MOCK_METHOD1(setColumnCount, void(const int count));
  MOCK_METHOD3(setItem, void(const int rowIndex, const int columnIndex, QTableWidgetItem *item));
  MOCK_METHOD1(hideColumn, void(const int index));
  MOCK_METHOD1(showColumn, void(const int index));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ImageInfoPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoPresenterTest *createSuite() { return new ImageInfoPresenterTest(); }
  static void destroySuite(ImageInfoPresenterTest *suite) { delete suite; }

  void test_constructor_calls_view_set_row_count() {
    auto mockView = std::make_unique<StrictMock<MockImageInfoView>>();

    EXPECT_CALL(*mockView, setRowCount(2)).Times(1);

    ImageInfoPresenter presenter(mockView.get());
  }

  void test_cursorAt_calls_view_showInfo() {
    auto mockView = std::make_unique<StrictMock<MockImageInfoView>>();

    EXPECT_CALL(*mockView, setRowCount(2)).Times(1);
    EXPECT_CALL(*mockView, showInfo(_)).Times(1);

    ImageInfoPresenter presenter(mockView.get());
    presenter.setWorkspace(WorkspaceCreationHelper::create2DWorkspace123(10, 10));
    presenter.cursorAt(1, 2, 1, {});
  }

  void test_setWorkspace_creates_matrix_ws_model_with_matrix_ws() {
    auto mockView = std::make_unique<MockImageInfoView>();
    ImageInfoPresenter presenter(mockView.get());
    auto matrixWS = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);

    presenter.setWorkspace(matrixWS);

    const auto &model = presenter.model();
    TS_ASSERT(dynamic_cast<const ImageInfoModelMatrixWS *>(&model));
    TS_ASSERT(Mock::VerifyAndClear(mockView.get()));
  }

  void test_setWorkspace_creates_MD_model_with_md_ws() {
    auto mockView = std::make_unique<MockImageInfoView>();
    ImageInfoPresenter presenter(mockView.get());
    const auto mdWS = makeFakeMDEventWorkspace("dummyName", 100);

    presenter.setWorkspace(mdWS);

    const auto &model = presenter.model();
    TS_ASSERT(dynamic_cast<const ImageInfoModelMD *>(&model));
    TS_ASSERT(Mock::VerifyAndClear(mockView.get()));
  }
};
