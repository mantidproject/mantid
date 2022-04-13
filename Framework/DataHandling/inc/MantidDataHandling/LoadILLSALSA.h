// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

#include <memory>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL SALSA NeXus file into a Mantid workspace.
 */
class DLLExport LoadILLSALSA : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadILLSALSA"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an ILL SALSA NeXus file."; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus;ILL\\Diffraction"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;
  // set the instrument
  void setInstrument(double distance, double angle);
  // load data from old single point nexus
  void loadOldNexus(const std::string &filename);
  // fill workspace with detector and monitor counts
  void fillWorkspaceData(const Mantid::NeXus::NXInt &detectorData, const std::vector<std::string> &scanVariableNames,
                         const Mantid::NeXus::NXDouble &scanVariables);
  // fill workspace with metadata
  void fillWorkspaceMetadata(const std::string &filename);
  // Output workspace
  std::shared_ptr<DataObjects::Workspace2D> m_outputWorkspace;
  // number of scans in data
  int m_numberOfScans;
  // number of rows in data
  int m_numberOfRows;
  // number of columns in data
  int m_numberOfColumns;
};

} // namespace DataHandling
} // namespace Mantid
