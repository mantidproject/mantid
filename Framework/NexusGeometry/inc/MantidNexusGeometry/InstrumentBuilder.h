// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/DllConfig.h"
#include "MantidNexusGeometry/TubeBuilder.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {
class Instrument;
class IComponent;
class ICompAssembly;
} // namespace Geometry
namespace NexusGeometry {

/** InstrumentBuilder : Builder for wrapping the creating of a Mantid
  Instrument. Provides some useful abstractions over the full-blown Instrument
  interface
*/
class MANTID_NEXUSGEOMETRY_DLL InstrumentBuilder {
public:
  /// Constructor creates the instrument
  explicit InstrumentBuilder(const std::string &instrumentName);
  InstrumentBuilder(const InstrumentBuilder &) = delete;
  InstrumentBuilder &operator=(const InstrumentBuilder &) = delete;
  /// Adds component to instrument
  Geometry::IComponent *addComponent(const std::string &compName, const Eigen::Vector3d &position);
  /// Adds tubes (ObjComponentAssemblies) to the last registered bank
  void addTubes(const std::string &bankName, const std::vector<detail::TubeBuilder> &tubes,
                const std::shared_ptr<const Mantid::Geometry::IObject> &pixelShape);
  /// Adds detector to the root (instrument)
  void addDetectorToInstrument(const std::string &detName, detid_t detId, const Eigen::Vector3d &position,
                               std::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Adds detector to the last registered bank
  void addDetectorToLastBank(const std::string &detName, detid_t detId, const Eigen::Vector3d &relativeOffset,
                             std::shared_ptr<const Mantid::Geometry::IObject> shape);
  /// Adds detector to instrument
  void addMonitor(const std::string &detName, detid_t detId, const Eigen::Vector3d &position,
                  std::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Add sample
  void addSample(const std::string &sampleName, const Eigen::Vector3d &position);
  /// Add source
  void addSource(const std::string &sourceName, const Eigen::Vector3d &position);

  void addBank(const std::string &localName, const Eigen::Vector3d &position, const Eigen::Quaterniond &rotation);

  /// Returns underlying instrument
  std::unique_ptr<const Geometry::Instrument> createInstrument();

private:
  /// Add a single tube to the last registed bank
  void doAddTube(const std::string &compName, const detail::TubeBuilder &tube,
                 const std::shared_ptr<const Mantid::Geometry::IObject> &pixelShape);
  /// Sorts detectors
  void sortDetectors() const;
  /// product
  std::unique_ptr<Geometry::Instrument> m_instrument;
  /// Last bank added. The instrument is the owner of the bank.
  Geometry::ICompAssembly *m_lastBank = nullptr;
};
} // namespace NexusGeometry
} // namespace Mantid
