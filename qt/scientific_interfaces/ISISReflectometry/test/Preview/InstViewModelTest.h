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
  void test_notify_workspace_updated_updates_actor() {
    auto model = InstViewModel(makeMessageHandler());
    auto previousActor = model.getInstrumentViewActor();
    auto ws = createWorkspace();
    model.updateWorkspace(ws);
    const auto result = model.getInstrumentViewActor();
    TS_ASSERT(result);
    TS_ASSERT_DIFFERS(previousActor, result)
    TS_ASSERT_EQUALS(result->getWorkspace(), ws)
  }

private:
  Mantid::API::MatrixWorkspace_sptr createWorkspace() {
    return WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
  }

  std::unique_ptr<MantidQt::MantidWidgets::IMessageHandler> makeMessageHandler() {
    return std::make_unique<MockMessageHandler>();
  }
};
