// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"
#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
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
  MOCK_METHOD3(cursorAt, void(const double x, const double y, const double z));
  MOCK_METHOD1(showInfo, void(const ImageInfoModel::ImageInfo &info));
  MOCK_METHOD1(setWorkspace, void(const Mantid::API::Workspace_sptr &ws));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ImageInfoPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoPresenterTest *createSuite() { return new ImageInfoPresenterTest(); }
  static void destroySuite(ImageInfoPresenterTest *suite) { delete suite; }

  void test_cursorAt_calls_view_showInfo() {
    auto mockView = std::make_unique<StrictMock<MockImageInfoView>>();
    ImageInfoPresenter presenter(mockView.get());
    presenter.setWorkspace(WorkspaceCreationHelper::create2DWorkspace123(10, 10));

    EXPECT_CALL(*mockView, showInfo(_)).Times(1);
    presenter.cursorAt(1, 2, 1);

    TS_ASSERT(Mock::VerifyAndClear(mockView.get()));
  }

  void test_setWorkspace_creates_matrix_ws_model_with_matrix_ws() {
    auto mockView = std::make_unique<MockImageInfoView>();
    ImageInfoPresenter presenter(mockView.get());
    auto matrixWS = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);
    EXPECT_CALL(*mockView, showInfo(_)).Times(1);

    presenter.setWorkspace(matrixWS);

    const auto &model = presenter.model();
    TS_ASSERT(dynamic_cast<const ImageInfoModelMatrixWS *>(&model));
    TS_ASSERT(Mock::VerifyAndClear(mockView.get()));
  }

  void test_setWorkspace_creates_MD_model_with_md_ws() {
    auto mockView = std::make_unique<MockImageInfoView>();
    ImageInfoPresenter presenter(mockView.get());
    const auto mdWS = makeFakeMDEventWorkspace("dummyName", 100);
    EXPECT_CALL(*mockView, showInfo(_)).Times(1);

    presenter.setWorkspace(mdWS);

    const auto &model = presenter.model();
    TS_ASSERT(dynamic_cast<const ImageInfoModelMD *>(&model));
    TS_ASSERT(Mock::VerifyAndClear(mockView.get()));
  }
};
