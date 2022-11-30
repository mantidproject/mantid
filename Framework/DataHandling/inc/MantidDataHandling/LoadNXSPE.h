// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid {
namespace DataHandling {

/** LoadNXSPE : Algorithm to load an NXSPE file into a workspace2D. It will
  create a "new" instrument, that can be overwritten later by the LoadInstrument
  algorithm
  Properties:
  <ul>
  <li>Filename  - the name of the file to read from.</li>
  <li>Workspace - the workspace name that will be created and hold the loaded
  data.</li>
  </ul>
  @author Andrei Savici, ORNL
  @date 2011-08-14
*/
class MANTID_DATAHANDLING_DLL LoadNXSPE : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadNXSPE"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return " Algorithm to load an NXSPE file into a workspace2D."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveNXSPE", "LoadSPE"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(DataHandling\Nexus;DataHandling\SPE;Inelastic\DataHandling)";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Confidence in identifier.
  static int identiferConfidence(const std::string &value);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
