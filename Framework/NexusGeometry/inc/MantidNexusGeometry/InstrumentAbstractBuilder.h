#ifndef INSTRUMENT_ABSTRACT_BUILDER_H_
#define INSTRUMENT_ABSTRACT_BUILDER_H_

//-----------------------
// Includes
//-----------------------

#include "Eigen/Core"

namespace Mantid {
namespace NexusGeometry {

// Instrument Abstract Builder crpt class
template <typename iAbstraction> class InstrumentAbstractBuilder {
public:
  /// Add component to instrument
  void addComponent(std::string &compName, Eigen::Vector3d &position) {
    this->iImp().addComponent(compName, position);
  }
  /// Add detector to instrument
  void addDetector(std::string &detName, int detId, Eigen::Vector3d &position) {
    this->iImp().addDetector(detName, detId, position);
  }
  /// Sort detectors
  void sortDetectors() { this->iImp().sortDetectors(); }
  /// Add sample
  void addSample(std::string &sampleName, Eigen::Vector3d &position) {
    this->iImp().addSample(sampleName, position);
  }
  /// Add source
  void addSource(std::string &sourceName, Eigen::Vector3d &position) {
    this->iImp().addSource(sourceName, position);
  }

private:
  /// Factor out the static_cast
  iAbstraction &iImp() { return static_cast<iAbstraction &>(*this); }
};
}
}

#endif // INSTRUMENT_ABSTRACT_BUILDER_H_
