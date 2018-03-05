//--------------------
// Includes
//--------------------

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"
#include "MantidNexusGeometry/ShapeGeometryAbstraction.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidKernel/make_unique.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace NexusGeometry {

/// Constructor
InstrumentBuilder::InstrumentBuilder(const std::string &instrumentName)
    : m_instrument(
          Mantid::Kernel::make_unique<Geometry::Instrument>(instrumentName)) {
  // Default view
  std::string defaultViewAxis = "z";
  Geometry::PointingAlong pointingUp(Geometry::Y), alongBeam(Geometry::Z),
      thetaSign(Geometry::X);
  Geometry::Handedness handedness(Geometry::Right);
  std::string origin;
  m_instrument->setDefaultViewAxis(defaultViewAxis);
  // Reference frame definitely does not need to be shared, and the copy
  // operations in instrument make a new one anyway, but at present i'm not
  // chaning the instruemnt API, particularly since getReferenceFrame on
  // instrument returns the shared pointer
  m_instrument->setReferenceFrame(boost::make_shared<Geometry::ReferenceFrame>(
      pointingUp, alongBeam, thetaSign, handedness, origin));
}

/// Adds component to instrument
Geometry::IComponent *
InstrumentBuilder::addComponent(const std::string &compName,
                                const Eigen::Vector3d &position) {
  Geometry::IComponent *component(new Geometry::ObjCompAssembly(compName));
  component->setPos(position(0), position(1), position(2));
  m_instrument->add(component);
  return component;
}

/// Adds detector to instrument
void InstrumentBuilder::addDetector(const std::string &detName, int detId,
                                    const Eigen::Vector3d &position,
                                    objectHolder &shape) {
  auto *detector(new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(m_instrument->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));

  detector->setShape(shape);

  m_instrument->add(detector);
  m_instrument->markAsDetectorIncomplete(detector);
}

void InstrumentBuilder::addMonitor(const std::string &detName, int detId,
                                   const Eigen::Vector3d &position,
                                   objectHolder &shape) {
  auto *detector(new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(m_instrument->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));

  detector->setShape(shape);

  m_instrument->add(detector);
  m_instrument->markAsMonitor(detector);
}

/// Sorts detectors
void InstrumentBuilder::sortDetectors() const {
  m_instrument->markAsDetectorFinalize();
}

/// Add sample
void InstrumentBuilder::addSample(const std::string &sampleName,
                                  const Eigen::Vector3d &position) {
  auto *sample(this->addComponent(sampleName, position));
  m_instrument->markAsSamplePos(sample);
}
/// Add source
void InstrumentBuilder::addSource(const std::string &sourceName,
                                  const Eigen::Vector3d &position) {
  auto *source(this->addComponent(sourceName, position));
  m_instrument->markAsSource(source);
}

std::unique_ptr<const Geometry::Instrument>
InstrumentBuilder::createInstrument() const {
  sortDetectors();
  return std::unique_ptr<const Geometry::Instrument>(m_instrument->clone());
}
}
}
