// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class IInstViewModel {
public:
  virtual ~IInstViewModel() = default;
  virtual void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) = 0;
  virtual MantidWidgets::InstrumentActor *getInstrumentViewActor() const = 0;
  virtual Mantid::Kernel::V3D getSamplePos() const = 0;
  virtual Mantid::Kernel::V3D getAxis() const = 0;
  virtual std::vector<Mantid::detid_t> detIndicesToDetIDs(std::vector<size_t> const &detIndices) const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
