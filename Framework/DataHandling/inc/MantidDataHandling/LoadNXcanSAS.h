// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADNXCANSAS_H_
#define MANTID_DATAHANDLING_LOADNXCANSAS_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"

namespace H5 {
class Group;
}

namespace Mantid {
namespace DataHandling {

/** LoadNXcanSAS : Tries to load an NXcanSAS file type into a Workspace2D.
 *  This can load either 1D or 2D data
 */
class MANTID_DATAHANDLING_DLL LoadNXcanSAS
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Constructor
  LoadNXcanSAS();
  /// Virtual dtor
  ~LoadNXcanSAS() override {}
  const std::string name() const override { return "LoadNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an HDF5 NXcanSAS file into a MatrixWorkspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadCanSAS1D", "SaveNXcanSAS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Loads the transmission runs
  void loadTransmission(H5::Group &entry, const std::string &name);
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADNXCANSAS_H_ */
