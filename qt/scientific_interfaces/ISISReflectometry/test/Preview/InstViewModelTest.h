// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InstViewModel.h"
#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "test/ReflMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class InstViewModelTest : public CxxTest::TestSuite {
public:
  void test_notify_workspace_updated_updates_surface() {
    auto model = InstViewModel(makeMessageHandler());
    auto previousSurface = model.getInstrumentViewSurface();
    auto ws = createWorkspace();
    model.updateWorkspace(ws);
    const auto result = model.getInstrumentViewSurface();
    TS_ASSERT(result);
    TS_ASSERT_DIFFERS(previousSurface, result)
  }

private:
  Mantid::API::MatrixWorkspace_sptr createWorkspace() {
    return WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
  }

  std::unique_ptr<MantidQt::MantidWidgets::IMessageHandler> makeMessageHandler() {
    return std::make_unique<MockMessageHandler>();
  }
};
