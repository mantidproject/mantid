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

using namespace MantidQt::MantidWidgets;
using namespace Mantid::DataObjects;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockImageInfoView : public IImageInfoWidget {
public:
  MockImageInfoView() {
    m_presenter = std::make_shared<ImageInfoPresenter>(this);
  }

  MOCK_METHOD3(updateTable,
               void(const double x, const double y, const double z));
  MOCK_METHOD1(setWorkspace, void(const Mantid::API::Workspace_sptr &ws));

  std::shared_ptr<ImageInfoPresenter> getPresenter() { return m_presenter; }

private:
  std::shared_ptr<ImageInfoPresenter> m_presenter;
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ImageInfoPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoPresenterTest *createSuite() {
    return new ImageInfoPresenterTest();
  }
  static void destroySuite(ImageInfoPresenterTest *suite) { delete suite; }

  void test_createImageInfoModel_creates_matrix_ws_model_with_matrix_ws() {
    auto mockView = std::make_shared<NiceMock<MockImageInfoView>>();
    auto presenter = mockView->getPresenter();
    presenter->createImageInfoModel("MATRIX");
    auto model = presenter->getModel();
    TS_ASSERT(std::dynamic_pointer_cast<ImageInfoModelMatrixWS>(model) != NULL);
  }

  void test_createImageInfoModel_creates_MD_model_with_md_ws() {
    auto mockView = std::make_shared<NiceMock<MockImageInfoView>>();
    auto presenter = mockView->getPresenter();
    presenter->createImageInfoModel("MDH");
    auto model = presenter->getModel();
    TS_ASSERT(std::dynamic_pointer_cast<ImageInfoModelMD>(model) != NULL);
  }
};
