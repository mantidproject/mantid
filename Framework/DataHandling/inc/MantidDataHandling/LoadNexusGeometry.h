#ifndef LOAD_NEXUS_GEOMETRY_H_
#define LOAD_NEXUS_GEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"

namespace Mantid{
namespace DataHandling{

class DLLExport LoadNexusGeometry
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Default constructor
  LoadNexusGeometry();
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

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

};


}
}


#endif // LOAD_NEXUS_GEOMETRY_H_

