#ifndef NEXUSGEOMETRY_INSTRUMENTBUILDER_H
#define NEXUSGEOMETRY_INSTRUMENTBUILDER_H

//----------------------
// Includes
//----------------------

#include "MantidNexusGeometry/DllConfig.h"
#include "MantidGeometry/Objects/IObject.h"
#include "Eigen/Core"
#include <string>
#include <memory>

namespace Mantid {
namespace Geometry {
class Instrument;
class IComponent;
}
namespace NexusGeometry {

class MANTID_NEXUSGEOMETRY_DLL InstrumentBuilder {
public:
  /// Constructor creates the instrument
  InstrumentBuilder(const std::string &instrumentName);
  /// Adds component to instrument
  Geometry::IComponent *addComponent(const std::string &compName,
                                     const Eigen::Vector3d &position);
  /// Adds detector to instrument
  void addDetector(const std::string &detName, int detId,
                   const Eigen::Vector3d &position,
                   boost::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Adds detector to instrument
  void addMonitor(const std::string &detName, int detId,
                  const Eigen::Vector3d &position,
                  boost::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Add sample
  void addSample(const std::string &sampleName,
                 const Eigen::Vector3d &position);
  /// Add source
  void addSource(const std::string &sourceName,
                 const Eigen::Vector3d &position);
  /// Returns underlying instrument
  std::unique_ptr<const Geometry::Instrument> createInstrument() const;

private:
  /// Sorts detectors
  void sortDetectors() const;
  /// product
  mutable std::unique_ptr<Geometry::Instrument> m_instrument;
};
}
}
#endif // NEXUSGEOMETRY_INSTRUMENTBUILDER_H
