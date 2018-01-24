//--------------------
// Includes
//--------------------

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"
#include "MantidNexusGeometry/ShapeGeometryAbstraction.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace NexusGeometry {

/// Constructor
InstrumentGeometryAbstraction::InstrumentGeometryAbstraction(
    const std::string &instrumentName) {
  Geometry::Instrument_sptr inst_sptr(new Geometry::Instrument(instrumentName));
  this->instrument_sptr = inst_sptr;

  // Default view
  std::string defaultViewAxis = "z";
  Geometry::PointingAlong pointingUp(Geometry::Y), alongBeam(Geometry::Z),
      thetaSign(Geometry::X);
  Geometry::Handedness handedness(Geometry::Right);
  std::string origin;
  this->instrument_sptr->setDefaultViewAxis(defaultViewAxis);
  this->instrument_sptr->setReferenceFrame(
      boost::make_shared<Geometry::ReferenceFrame>(
          pointingUp, alongBeam, thetaSign, handedness, origin));
}

/// Adds component to instrument
Geometry::IComponent *
InstrumentGeometryAbstraction::addComponent(const std::string &compName,
                                            const Eigen::Vector3d &position) {
  Geometry::IComponent *component(new Geometry::ObjCompAssembly(compName));
  component->setPos(position(0), position(1), position(2));
  this->instrument_sptr->add(component);
  return component;
}

/// Adds detector to instrument
void InstrumentGeometryAbstraction::addDetector(const std::string &detName,
                                                int detId,
                                                const Eigen::Vector3d &position,
                                                objectHolder &shape) {
  auto *detector(new Geometry::Detector(
      detName, detId, const_cast<Geometry::IComponent *>(
                          this->instrument_sptr->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));

  detector->setShape(shape);

  this->instrument_sptr->add(detector);
  this->instrument_sptr->markAsDetectorIncomplete(detector);
}

void InstrumentGeometryAbstraction::addMonitor(const std::string &detName,
                                               int detId,
                                               const Eigen::Vector3d &position,
                                               objectHolder &shape) {
  auto *detector(new Geometry::Detector(
      detName, detId, const_cast<Geometry::IComponent *>(
                          this->instrument_sptr->getBaseComponent())));
  detector->setPos(position(0), position(1), position(2));

  detector->setShape(shape);

  this->instrument_sptr->add(detector);
  this->instrument_sptr->markAsMonitor(detector);
}

/// Sorts detectors
void InstrumentGeometryAbstraction::sortDetectors() {
  this->instrument_sptr->markAsDetectorFinalize();
}

/// Add sample
void InstrumentGeometryAbstraction::addSample(const std::string &sampleName,
                                              const Eigen::Vector3d &position) {
  auto *sample(this->addComponent(sampleName, position));
  this->instrument_sptr->markAsSamplePos(sample);
}
/// Add source
void InstrumentGeometryAbstraction::addSource(const std::string &sourceName,
                                              const Eigen::Vector3d &position) {
  auto *source(this->addComponent(sourceName, position));
  this->instrument_sptr->markAsSource(source);
}
}
}
