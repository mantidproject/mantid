// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/NexusDescriptor.h"

#include <H5Cpp.h>

#include <memory>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL SALSA NeXus file into a Mantid workspace.
 */
class MANTID_DATAHANDLING_DLL LoadILLSALSA : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadILLSALSA"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an ILL SALSA NeXus file."; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus;ILL\\Strain"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  // Number of pixel on the detector in the vertical dimension
  static const size_t VERTICAL_NUMBER_PIXELS;
  // Number of pixel on the detector in the horizontal dimension
  static const size_t HORIZONTAL_NUMBER_PIXELS;
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;
  // set the instrument
  void setInstrument(double distance, double angle);
  // load data from v1 Nexus
  void loadNexusV1(const H5::H5File &h5file);
  // load data from V2 Nexus
  void loadNexusV2(const H5::H5File &h5file);
  // fill workspace with metadata
  void fillWorkspaceMetadata(const std::string &filename);
  // Output workspace
  std::shared_ptr<DataObjects::Workspace2D> m_outputWorkspace;
};

} // namespace DataHandling
} // namespace Mantid
