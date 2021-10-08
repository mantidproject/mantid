// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IInstViewModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <gmock/gmock.h>

#include <string>

using Mantid::API::MatrixWorkspace_sptr;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockInstViewModel : public IInstViewModel {
public:
  MOCK_METHOD(void, notifyWorkspaceUpdated, (Mantid::API::MatrixWorkspace_sptr &), (override));
  MOCK_METHOD(std::shared_ptr<MantidWidgets::RotationSurface>, getInstrumentViewSurface, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
