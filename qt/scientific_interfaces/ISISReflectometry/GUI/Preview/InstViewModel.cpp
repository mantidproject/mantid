// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "InstViewModel.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/Common/MessageHandler.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

InstViewModel::InstViewModel(std::unique_ptr<MantidWidgets::IMessageHandler> messageHandler)
    : m_messageHandler(std::move(messageHandler)) {
  if (!m_messageHandler)
    m_messageHandler = std::make_unique<MantidWidgets::MessageHandler>();
}

std::unique_ptr<MantidWidgets::InstrumentActor>
InstViewModel::createInstrumentViewActor(Mantid::API::MatrixWorkspace_sptr &workspace) const {
  bool autoscaling = true;
  // TODO remove hard-coded scale when we add colorbar
  auto scaleMin = 0.0;
  auto scaleMax = 1.0;

  return std::make_unique<MantidWidgets::InstrumentActor>(workspace, *m_messageHandler, autoscaling, scaleMin,
                                                          scaleMax);
}

void InstViewModel::updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) {
  m_actor = createInstrumentViewActor(workspace);
  m_actor->initialize(true, true);
}

// TODO refactor getting the sample pos and axis into the surface constructor so we don't need to get it here
Mantid::Kernel::V3D InstViewModel::getSamplePos() const {
  const auto &componentInfo = m_actor->componentInfo();
  return componentInfo.samplePosition();
}

Mantid::Kernel::V3D InstViewModel::getAxis() const {
  return Mantid::Kernel::V3D(0, 1, 0); // CYLINDRICAL_Y
}

MantidWidgets::InstrumentActor *InstViewModel::getInstrumentViewActor() const { return m_actor.get(); }

std::vector<Mantid::detid_t> InstViewModel::detIndicesToDetIDs(std::vector<size_t> const &detIndices) const {
  return m_actor->getDetIDs(detIndices);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
