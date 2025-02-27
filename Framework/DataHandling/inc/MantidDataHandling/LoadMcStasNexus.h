// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid {
namespace DataHandling {

/** LoadMcStasNexus : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL LoadMcStasNexus : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an McStas NeXus file into a group workspace."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadMcStas"}; }
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
