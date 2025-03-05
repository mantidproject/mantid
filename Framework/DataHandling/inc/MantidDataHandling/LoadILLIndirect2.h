// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"

namespace Mantid {
namespace DataHandling {

/**
  Loads an ILL IN16B nexus file into a Mantid workspace.
*/
class MANTID_DATAHANDLING_DLL LoadILLIndirect2 : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLIndirect2();
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a ILL/IN16B nexus file."; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  void loadDataDetails(const NeXus::NXEntry &entry);
  void initWorkSpace();
  void setInstrumentName(const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath);
  std::string getDataPath(const NeXus::NXEntry &entry);
  void loadNexusEntriesIntoProperties(const std::string &nexusfilename);
  void loadDataIntoWorkspace(const NeXus::NXEntry &entry);
  void loadDiffractionData(NeXus::NXEntry &entry);
  void moveComponent(const std::string &, double);
  void moveSingleDetectors(const NeXus::NXEntry &entry);
  void rotateTubes();
  std::string getInstrumentFileName();

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument

  // Variables describing the data in the detector
  size_t m_numberOfTubes;           // number of tubes - X
  size_t m_numberOfPixelsPerTube;   // number of pixels per tube - Y
  size_t m_numberOfChannels;        // time channels - Z
  size_t m_numberOfSimpleDetectors; // number of simple detector
  size_t m_numberOfMonitors;        // number of monitors
  std::set<int> m_activeSDIndices;  // set of Single Detector indices,
                                    // that were actually active
  bool m_bats;                      // A flag marking the BATS mode
  size_t m_firstTubeAngleRounded;   // A flag holding the rounded angle of the first tube

  std::vector<std::string> m_supportedInstruments;
  std::string m_loadOption;
};

} // namespace DataHandling
} // namespace Mantid
