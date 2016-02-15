#include "MantidAlgorithms/BasicInstrumentInfo.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

BasicInstrumentInfo::BasicInstrumentInfo(const API::MatrixWorkspace &workspace)
    : m_instrument(workspace.getInstrument()) {
  // TODO: Create these only when needed (thread-safe via atomics)
  m_source = m_instrument->getSource();
  m_sample = m_instrument->getSample();
  m_sourcePos = m_source->getPos();
  m_samplePos = m_sample->getPos();
  m_L1 = getSource().getDistance(getSample());
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

Kernel::V3D BasicInstrumentInfo::getSourcePos() const { return m_sourcePos; }

Kernel::V3D BasicInstrumentInfo::getSamplePos() const { return m_samplePos; }

double BasicInstrumentInfo::getL1() const { return m_L1; }
}
}
