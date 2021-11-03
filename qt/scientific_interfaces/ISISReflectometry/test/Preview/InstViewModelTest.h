// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InstViewModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "test/ReflMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using ::testing::_;

class InstViewModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstViewModelTest *createSuite() { return new InstViewModelTest(); }
  static void destroySuite(InstViewModelTest *suite) { delete suite; }

  // Init framework because the model uses an instrument actor which needs to call algorithms
  InstViewModelTest() { Mantid::API::FrameworkManager::Instance(); }

  void test_update_workspace_updates_actor() {
    auto model = makeInstViewModel();

    auto previousActor = model.getInstrumentViewActor();
    auto ws = createWorkspace();
    model.updateWorkspace(ws);

    const auto result = model.getInstrumentViewActor();
    TS_ASSERT(result);
    TS_ASSERT_DIFFERS(previousActor, result)
    TS_ASSERT_EQUALS(result->getWorkspace(), ws)
  }

  void test_update_workspace_initializes_actor() {
    auto model = makeInstViewModel();
    auto ws = createWorkspace();
    model.updateWorkspace(ws);

    const auto result = model.getInstrumentViewActor();
    TS_ASSERT(result->isInitialized());
  }

  void test_get_sample_pos() {
    auto model = makeInstViewModel();
    auto ws = createWorkspace();
    model.updateWorkspace(ws);

    auto samplePos = model.getSamplePos();
    // The sample is at 15, 0, 0 in the workspace created by the helper
    TS_ASSERT_EQUALS(samplePos, Mantid::Kernel::V3D(15, 0, 0));
  }

  void test_get_axis() {
    auto model = makeInstViewModel();
    auto axis = model.getAxis();
    // Currently this just returns a hard-coded value but we may change it to be configurable in future
    TS_ASSERT_EQUALS(axis, Mantid::Kernel::V3D(0, 1, 0));
  }

  void test_convert_det_ids_to_ws_indices() {
    auto model = makeInstViewModel();
    auto ws = createWorkspaceMultiDetector();
    model.updateWorkspace(ws);

    auto const detIndices = std::vector<size_t>{1, 2, 3};
    auto const expected = std::vector<Mantid::detid_t>{2, 3, 4};
    auto const workspaceIndices = model.detIndicesToDetIDs(detIndices);
    TS_ASSERT_EQUALS(workspaceIndices, expected);
  }

private:
  InstViewModel makeInstViewModel() { return InstViewModel(makeMessageHandler()); }

  Mantid::API::MatrixWorkspace_sptr createWorkspace() {
    return WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
  }

  Mantid::API::MatrixWorkspace_sptr createWorkspaceMultiDetector() {
    return WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrumentMultiDetector();
  }

  std::unique_ptr<MockMessageHandler> makeMessageHandler() { return std::make_unique<MockMessageHandler>(); }
};
