// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class IInstViewModel {
public:
  virtual ~IInstViewModel() = default;
  virtual void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) = 0;
  virtual std::shared_ptr<MantidWidgets::RotationSurface> getInstrumentViewSurface() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
