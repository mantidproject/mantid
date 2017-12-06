#ifndef INSTRUMENT_ABSTRACT_BUILDER_H
#define INSTRUMENT_ABSTRACT_BUILDER_H

//----------------------
// Includes
//----------------------

#include "MantidGeometry/Instrument_fwd.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidNexusGeometry/InstrumentAbstractBuilder.h"
#include "MantidNexusGeometry/ShapeGeometryAbstraction.h"

#include "Eigen/Core"
#include <string>

namespace Mantid {
namespace NexusGeometry {

class DLLExport InstrumentGeometryAbstraction
    : public NexusGeometry::InstrumentAbstractBuilder<
          InstrumentGeometryAbstraction> {
public:
  /// Constructor creates the instrument
  InstrumentGeometryAbstraction(const std::string &instrumentName);
  /// Adds component to instrument
  Geometry::IComponent *addComponent(std::string &compName,
                                     Eigen::Vector3d &position);
  /// Adds detector to instrument
  void addDetector(std::string &detName, int detId, Eigen::Vector3d &position,
                   objectHolder &shape);
  /// Sorts detectors
  void sortDetectors();
  /// Add sample
  void addSample(std::string &sampleName, Eigen::Vector3d &position);
  /// Add source
  void addSource(std::string &sourceName, Eigen::Vector3d &position);
  /// Returns underlying instrument
  Geometry::Instrument_sptr _unAbstractInstrument() {
    return this->instrument_sptr;
  }

private:
  Geometry::Instrument_sptr instrument_sptr;
};
}
}
#endif // INSTRUMENT_ABSTRACT_BUILDER_H
