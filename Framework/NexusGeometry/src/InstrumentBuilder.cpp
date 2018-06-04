#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/EigenConversionHelpers.h"
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

  m_instrument->setPos(0, 0, 0);
  m_instrument->setRot(Kernel::Quat());
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

void InstrumentBuilder::addDetectorToLastBank(
    const std::string &detName, int detId,
    const Eigen::Vector3d &relativeOffset,
    boost::shared_ptr<const Geometry::IObject> shape) {
  if (!m_lastBank)
    throw std::runtime_error("No bank to add the detector to");
  auto *detector = new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(m_lastBank->getBaseComponent()));
  detector->translate(Mantid::Kernel::toV3D(relativeOffset));
  // detector->setPos(relativeOffset[0], relativeOffset[1], relativeOffset[2]);
  // No rotation set for detector pixels of a bank. This is not possible in the
  // Nexus Geometry specification.
  detector->setShape(shape);
  m_lastBank->add(detector);
  m_instrument->markAsDetectorIncomplete(detector);
}

/// Adds detector to instrument
void InstrumentBuilder::addDetectorToInstrument(
    const std::string &detName, int detId, const Eigen::Vector3d &position,
    boost::shared_ptr<const Geometry::IObject> &shape) {
  auto *detector(new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(m_instrument->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));

  detector->setShape(shape);

  m_instrument->add(detector);
  m_instrument->markAsDetectorIncomplete(detector);
}

void InstrumentBuilder::addMonitor(
    const std::string &detName, int detId, const Eigen::Vector3d &position,
    boost::shared_ptr<const Geometry::IObject> &shape) {
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

void InstrumentBuilder::addBank(const std::string &localName,
                                const Eigen::Vector3d &position,
                                const Eigen::Quaterniond &rotation) {
  auto *assembly =
      new Geometry::CompAssembly(m_instrument->getBaseComponent(), nullptr);
  assembly->setName(localName);
  assembly->setPos(position[0], position[1], position[2]);
  assembly->setRot(Kernel::toQuat(rotation));
  m_lastBank = assembly;
  m_instrument->add(assembly);
}

std::unique_ptr<const Geometry::Instrument>
InstrumentBuilder::createInstrument() const {
  sortDetectors();
  return std::unique_ptr<const Geometry::Instrument>(m_instrument->clone());
}
}
}
