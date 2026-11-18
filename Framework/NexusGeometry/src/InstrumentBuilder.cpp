// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/BeamlineRef.h"
#include "MantidGeometry/Instrument/InstrumentMetadata.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace Mantid::NexusGeometry {

using Kernel::Quat;
using Kernel::toV3D;

/// Constructor
InstrumentBuilder::InstrumentBuilder(const std::string &instrumentName)
    : m_root(m_builder.addAssembly(Geometry::BeamlineBuilder::noParent, instrumentName)) {
  Geometry::PointingAlong pointingUp(Geometry::Y), alongBeam(Geometry::Z), thetaSign(Geometry::X);
  Geometry::Handedness handedness(Geometry::Right);
  std::string origin;
  m_builder.setReferenceFrame(
      std::make_shared<Geometry::ReferenceFrame>(pointingUp, alongBeam, thetaSign, handedness, origin));
  // Default view, with the instrument's other defaults
  m_builder.setMetadata(std::make_shared<Geometry::InstrumentMetadata>(Types::Core::DateAndTime(),
                                                                       Types::Core::DateAndTime(), "", "", "3D", "z"));
}

/// Adds component to instrument
void InstrumentBuilder::addComponent(const std::string &compName, const Eigen::Vector3d &position) {
  m_builder.addAssembly(m_root, compName, toV3D(position));
}

/** Add a set of tubes to the last registered bank
@param bankName Bank name
@param tubes Tubes to be added to bank
@param pixelShape Shape of each detector within the tube
*/
void InstrumentBuilder::addTubes(const std::string &bankName, const std::vector<detail::TubeBuilder> &tubes,
                                 const std::shared_ptr<const Mantid::Geometry::IObject> &pixelShape) {
  for (size_t i = 0; i < tubes.size(); i++)
    doAddTube(bankName + "_tube_" + std::to_string(i), tubes[i], pixelShape);
}

/** Add a tube to the last registered bank
@param compName Tube name
@param tube Tube to be added to bank
@param pixelShape Shape of each detector within the tube
*/
void InstrumentBuilder::doAddTube(const std::string &compName, const detail::TubeBuilder &tube,
                                  const std::shared_ptr<const Mantid::Geometry::IObject> &pixelShape) {
  auto const tubeNode =
      m_builder.addOutlineAssembly(lastBank("tube"), compName, toV3D(tube.tubePosition()), Quat(), tube.shape());
  auto const baseName = compName + "_";
  for (size_t i = 0; i < tube.detPositions().size(); ++i) {
    m_builder.addDetector(tubeNode, baseName + std::to_string(i), tube.detIDs()[i],
                          toV3D(tube.detPositions()[i] - tube.tubePosition()), Quat(), pixelShape);
  }
}

void InstrumentBuilder::addDetectorToLastBank(const std::string &detName, detid_t detId,
                                              const Eigen::Vector3d &relativeOffset,
                                              std::shared_ptr<const Geometry::IObject> shape) {
  // No rotation set for detector pixels of a bank. This is not possible in the
  // Nexus Geometry specification.
  m_builder.addDetector(lastBank("detector"), detName, detId, toV3D(relativeOffset), Quat(), std::move(shape));
}

/// Adds detector to instrument
void InstrumentBuilder::addDetectorToInstrument(const std::string &detName, detid_t detId,
                                                const Eigen::Vector3d &position,
                                                std::shared_ptr<const Geometry::IObject> const &shape) {
  m_builder.addDetector(m_root, detName, detId, toV3D(position), Quat(), shape);
}

void InstrumentBuilder::addMonitor(const std::string &detName, detid_t detId, const Eigen::Vector3d &position,
                                   std::shared_ptr<const Geometry::IObject> const &shape) {
  m_builder.addDetector(m_root, detName, detId, toV3D(position), Quat(), shape, /*isMonitor*/ true);
}

/// Add sample. As in the legacy instrument, a later sample is only a component.
void InstrumentBuilder::addSample(const std::string &sampleName, const Eigen::Vector3d &position) {
  m_builder.setSample(m_builder.addAssembly(m_root, sampleName, toV3D(position)));
}
/// Add source. As in the legacy instrument, a later source is only a component.
void InstrumentBuilder::addSource(const std::string &sourceName, const Eigen::Vector3d &position) {
  m_builder.setSource(m_builder.addAssembly(m_root, sourceName, toV3D(position)));
}

void InstrumentBuilder::addBank(const std::string &localName, const Eigen::Vector3d &position,
                                const Eigen::Quaterniond &rotation) {
  m_lastBank = m_builder.addAssembly(m_root, localName, toV3D(position), Kernel::toQuat(rotation));
}

std::unique_ptr<const Geometry::Instrument> InstrumentBuilder::createInstrument() const {
  return Geometry::Instrument::fromBeamline(m_builder.build());
}

InstrumentBuilder::Node InstrumentBuilder::lastBank(std::string const &adding) const {
  if (!m_lastBank)
    throw std::runtime_error("No bank to add the " + adding + " to");
  return *m_lastBank;
}
} // namespace Mantid::NexusGeometry
