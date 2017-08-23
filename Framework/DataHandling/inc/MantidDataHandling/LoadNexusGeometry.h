#ifndef LOAD_NEXUS_GEOMETRY_H_
#define LOAD_NEXUS_GEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument_fwd.h"

#include "Eigen/Core"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadNexusGeometry
    : public API::IFileLoader<Kernel::NexusDescriptor> {

public:
  /// Default constructor
  LoadNexusGeometry() = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexusGeometry"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads Instrument Geometry from a NeXus file.";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Add component to instrument
  Geometry::IComponent *addComponent(const std::string &name,
                                     const Eigen::Vector3d &position,
                                     Geometry::Instrument_sptr instrument);
  /// Add source to instrument
  void addSource(const std::string &name, const Eigen::Vector3d &position,
                 Geometry::Instrument_sptr instrument);
  /// Add sample to instrument
  void addSample(const std::string &name, const Eigen::Vector3d &position,
                 Geometry::Instrument_sptr instrument);
  /// Add detector to instrument
  void addDetector(const std::string &name, const Eigen::Vector3d &position, const int detId,
                   Geometry::Instrument_sptr instrument);

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  // Instrument pointer
  std::string defaultName = "defaultInstrumentName";
};
}
}

#endif // LOAD_NEXUS_GEOMETRY_H_
