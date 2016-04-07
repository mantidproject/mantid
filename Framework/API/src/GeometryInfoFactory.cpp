#include "MantidAPI/GeometryInfoFactory.h"

#include "MantidGeometry/Instrument.h"
#include "MantidAPI/GeometryInfo.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

GeometryInfoFactory::GeometryInfoFactory(const MatrixWorkspace &workspace)
    : m_workspace(workspace), m_instrument(workspace.getInstrument()),
      m_L1(-1.0) {
  // Note: This does not seem possible currently (the instrument objects is
  // always allocated, even if it is empty), so this will not fail.
  if (!m_instrument)
    throw std::runtime_error("Workspace " + workspace.getName() +
                             " does not contain an instrument!");
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
  std::call_once(m_sourceCached, &GeometryInfoFactory::cacheSource, this);
  return *m_source;
}

const Geometry::IComponent &GeometryInfoFactory::getSample() const {
  std::call_once(m_sampleCached, &GeometryInfoFactory::cacheSample, this);
  return *m_sample;
}

Kernel::V3D GeometryInfoFactory::getSourcePos() const {
  std::call_once(m_sourceCached, &GeometryInfoFactory::cacheSource, this);
  return m_sourcePos;
}

Kernel::V3D GeometryInfoFactory::getSamplePos() const {
  std::call_once(m_sampleCached, &GeometryInfoFactory::cacheSample, this);
  return m_samplePos;
}

double GeometryInfoFactory::getL1() const {
  std::call_once(m_L1Cached, &GeometryInfoFactory::cacheL1, this);
  return m_L1;
}

void GeometryInfoFactory::cacheSource() const {
  m_source = m_instrument->getSource();
  if (!m_source)
    throw std::runtime_error("Instrument in workspace " +
                             m_workspace.getName() +
                             " does not contain source!");
  m_sourcePos = m_source->getPos();
}

void GeometryInfoFactory::cacheSample() const {
  m_sample = m_instrument->getSample();
  if (!m_sample)
    throw std::runtime_error("Instrument in workspace " +
                             m_workspace.getName() +
                             "  does not contain sample!");
  m_samplePos = m_sample->getPos();
}

void GeometryInfoFactory::cacheL1() const {
  std::call_once(m_sourceCached, &GeometryInfoFactory::cacheSource, this);
  std::call_once(m_sampleCached, &GeometryInfoFactory::cacheSample, this);
  m_L1 = m_source->getDistance(*m_sample);
}
}
}
