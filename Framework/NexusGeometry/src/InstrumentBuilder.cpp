// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include "MantidNexusGeometry/NexusShapeFactory.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace NexusGeometry {

/// Constructor
InstrumentBuilder::InstrumentBuilder(const std::string &instrumentName)
    : m_instrument(std::make_unique<Geometry::Instrument>(instrumentName)) {
  // Default view
  std::string defaultViewAxis = "z";
  Geometry::PointingAlong pointingUp(Geometry::Y), alongBeam(Geometry::Z),
      thetaSign(Geometry::X);
  Geometry::Handedness handedness(Geometry::Right);
  std::string origin;
  m_instrument->setDefaultViewAxis(defaultViewAxis);
  // Reference frame definitely does not need to be shared, and the copy
  // operations in instrument make a new one anyway, but at present i'm not
  // changing the instrument API, particularly since getReferenceFrame on
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

/** Add a set of tubes to the last registered bank
@param bankName Bank name
@param tubes Tubes to be added to bank
@param pixelShape Shape of each detector within the tube
*/
void InstrumentBuilder::addTubes(
    const std::string &bankName, const std::vector<detail::TubeBuilder> &tubes,
    boost::shared_ptr<const Mantid::Geometry::IObject> pixelShape) {
  for (size_t i = 0; i < tubes.size(); i++)
    doAddTube(bankName + "_tube_" + std::to_string(i), tubes[i], pixelShape);
}

/** Add a tube to the last registered bank
@param compName Tube name
@param tube Tube to be added to bank
@param pixelShape Shape of each detector within the tube
*/
void InstrumentBuilder::doAddTube(
    const std::string &compName, const detail::TubeBuilder &tube,
    boost::shared_ptr<const Mantid::Geometry::IObject> pixelShape) {
  auto *objComp(new Geometry::ObjCompAssembly(compName));
  const auto &pos = tube.tubePosition();
  objComp->setPos(pos(0), pos(1), pos(2));
  objComp->setOutline(tube.shape());
  auto baseName = compName + "_";
  for (size_t i = 0; i < tube.detPositions().size(); ++i) {
    auto *detector = new Geometry::Detector(baseName + std::to_string(i),
                                            tube.detIDs()[i], objComp);
    detector->translate(
        Mantid::Kernel::toV3D(tube.detPositions()[i] - tube.tubePosition()));
    detector->setShape(pixelShape);
    objComp->add(detector);
    m_instrument->markAsDetectorIncomplete(detector);
  }
  m_lastBank->add(objComp);
}

void InstrumentBuilder::addDetectorToLastBank(
    const std::string &detName, detid_t detId,
    const Eigen::Vector3d &relativeOffset,
    boost::shared_ptr<const Geometry::IObject> shape) {
  if (!m_lastBank)
    throw std::runtime_error("No bank to add the detector to");
  auto *detector = new Geometry::Detector(
      detName, detId,
      const_cast<Geometry::IComponent *>(m_lastBank->getBaseComponent()));
  detector->translate(Mantid::Kernel::toV3D(relativeOffset));
  // No rotation set for detector pixels of a bank. This is not possible in the
  // Nexus Geometry specification.
  if (shape.get() == nullptr)
    detector->setShape(shape);
  m_lastBank->add(detector);
  m_instrument->markAsDetectorIncomplete(detector);
}

/// Adds detector to instrument
void InstrumentBuilder::addDetectorToInstrument(
    const std::string &detName, detid_t detId, const Eigen::Vector3d &position,
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
    const std::string &detName, detid_t detId, const Eigen::Vector3d &position,
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
InstrumentBuilder::createInstrument() {
  sortDetectors();
  // Create the new replacement first incase it throws
  auto temp = std::make_unique<Geometry::Instrument>(m_instrument->getName());
  auto *product = m_instrument.release();
  m_instrument = std::move(temp);
  product->parseTreeAndCacheBeamline();
  // Some older compilers (Apple clang 7) don't support copy construction
  // std::unique_ptr<const T>(const std::ptr<T>&)
  return std::unique_ptr<const Geometry::Instrument>(product);
}
} // namespace NexusGeometry
} // namespace Mantid
