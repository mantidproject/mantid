// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL LoadMcStas : public API::NexusFileLoader {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a McStas NeXus file into an workspace."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadMcStasNexus", "LoadNexus"}; }
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

private:
  void init() override;
  void execLoader() override;

  API::WorkspaceGroup_sptr groupWorkspaces(const std::vector<std::string> &workspaces) const;

  std::vector<std::string> readEventData(const std::map<std::string, std::string> &eventEntries, ::NeXus::File &nxFile);
  std::vector<std::string> readHistogramData(const std::map<std::string, std::string> &histogramEntries,
                                             ::NeXus::File &nxFile);
};

} // namespace DataHandling
} // namespace Mantid
