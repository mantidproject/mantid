#include "MantidAlgorithms/BasicInstrumentInfo.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

BasicInstrumentInfo::BasicInstrumentInfo(const API::MatrixWorkspace &workspace)
    : m_instrument(workspace.getInstrument()) {
  // TODO: Create these only when needed (thread-safe via atomics)
  m_source = m_instrument->getSource();
  m_sample = m_instrument->getSample();
}

const Geometry::Instrument &BasicInstrumentInfo::getInstrument() const {
  return *m_instrument;
}

const Geometry::IComponent &BasicInstrumentInfo::getSource() const {
  return *m_source;
}

const Geometry::IComponent &BasicInstrumentInfo::getSample() const {
  return *m_sample;
}
}
}
