// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"

namespace Mantid {
namespace DataHandling {

/**
 Loads an PSI nexus file into a Mantid workspace.

 Required properties:
 <UL>
 <LI> Filename - The ILL nexus file to be read </LI>
 <LI> Workspace - The name to give to the output workspace </LI>
 </UL>
 */
class MANTID_DATAHANDLING_DLL LoadSINQFocus : public API::IFileLoader<Kernel::NexusDescriptor>,
                                              public API::DeprecatedAlgorithm {
public:
  LoadSINQFocus();
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a FOCUS nexus file from the PSI"; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadSINQ", "LoadSINQFile"}; }
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  void setInstrumentName(const NeXus::NXEntry &entry);
  void initWorkSpace(const NeXus::NXEntry &);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &);
  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  void loadExperimentDetails(const NeXus::NXEntry &);
  void loadRunDetails(const NeXus::NXEntry &);
  void runLoadInstrument();

  std::vector<std::string> m_supportedInstruments;
  std::string m_instrumentName;
  std::string m_instrumentPath;
  API::MatrixWorkspace_sptr m_localWorkspace;
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;
};

} // namespace DataHandling
} // namespace Mantid
