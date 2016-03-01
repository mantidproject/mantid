#include "MantidAPI/GeometryInfoFactory.h"

#include "MantidGeometry/Instrument.h"
#include "MantidAPI/GeometryInfo.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

GeometryInfoFactory::GeometryInfoFactory(const MatrixWorkspace &workspace)
    : m_workspace(workspace), m_instrument(workspace.getInstrument()) {
  if (!m_instrument)
    throw std::runtime_error("Workspace " + workspace.getName() +
                             " does not contain an instrument!");

  // TODO: Create these only when needed (thread-safe via atomics)
  m_source = m_instrument->getSource();
  m_sample = m_instrument->getSample();

  if (!m_source)
    throw std::runtime_error("Instrument in workspace " + workspace.getName() +
                             " does not contain source!");
  if (!m_sample)
    throw std::runtime_error("Instrument in workspace " + workspace.getName() +
                             "  does not contain sample!");

  m_sourcePos = m_source->getPos();
  m_samplePos = m_sample->getPos();
  m_L1 = getSource().getDistance(getSample());
}

GeometryInfo GeometryInfoFactory::create(const size_t index) const {
  // Note: Why return by value? We want to avoid memory allocation, since this
  // is used in a loop over all histograms in a workspace. Obviously, with the
  // current (2016) instrument code there are memory allocations when getting
  // the detector, so this additional allocation may be negligible, but we want
  // this class to be future proof. GeometryInfo should be kept small.
  return {*this, *(m_workspace.getSpectrum(index))};
}

const Geometry::Instrument &GeometryInfoFactory::getInstrument() const {
  return *m_instrument;
}

const Geometry::IComponent &GeometryInfoFactory::getSource() const {
  return *m_source;
}

const Geometry::IComponent &GeometryInfoFactory::getSample() const {
  return *m_sample;
}

Kernel::V3D GeometryInfoFactory::getSourcePos() const { return m_sourcePos; }

Kernel::V3D GeometryInfoFactory::getSamplePos() const { return m_samplePos; }

double GeometryInfoFactory::getL1() const { return m_L1; }
}
}
