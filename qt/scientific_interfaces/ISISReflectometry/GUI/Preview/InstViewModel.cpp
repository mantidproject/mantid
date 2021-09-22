// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "InstViewModel.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
std::unique_ptr<MantidWidgets::InstrumentActor>
InstViewModel::createInstrumentViewActor(Mantid::API::MatrixWorkspace_sptr &workspace) const {
  bool autoscaling = true;
  // TODO remove hard-coded scale when we add colorbar
  auto scaleMin = 0.0;
  auto scaleMax = 1.0;

  return std::make_unique<MantidWidgets::InstrumentActor>(workspace, autoscaling, scaleMin, scaleMax);
}

void InstViewModel::notifyWorkspaceUpdated(Mantid::API::MatrixWorkspace_sptr &workspace) {
  // TODO refactor the component info stuff into the surface constructor so we don't need to get it here
  m_actor = createInstrumentViewActor(workspace);
  const auto &componentInfo = m_actor->componentInfo();
  auto sample_pos = componentInfo.samplePosition();
  auto axis = Mantid::Kernel::V3D(0, 1, 0); // CYLINDRICAL_Y

  m_surface = std::make_shared<MantidWidgets::UnwrappedCylinder>(m_actor.get(), sample_pos, axis);
}

std::shared_ptr<MantidWidgets::RotationSurface> InstViewModel::getInstrumentViewSurface() const { return m_surface; }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry