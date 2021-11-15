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
  MOCK_METHOD(void, updateWorkspace, (Mantid::API::MatrixWorkspace_sptr &), (override));
  MOCK_METHOD(MantidWidgets::InstrumentActor *, getInstrumentViewActor, (), (const, override));
  MOCK_METHOD(Mantid::Kernel::V3D, getSamplePos, (), (const, override));
  MOCK_METHOD(Mantid::Kernel::V3D, getAxis, (), (const, override));
  MOCK_METHOD(std::vector<Mantid::detid_t>, detIndicesToDetIDs, (std::vector<size_t> const &), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
