// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexus/NexusDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL IN4/5/6/Panther NeXus file into a Mantid workspace.
 */
class MANTID_DATAHANDLING_DLL LoadILLTOF3 : public API::IFileLoader<Nexus::NexusDescriptor> {
public:
  /// Constructor
  LoadILLTOF3();
  /// Algorithm's name
  const std::string name() const override { return "LoadILLTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an ILL TOF NeXus file."; }

  /// Algorithm's version
  int version() const override { return 3; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus;ILL\\Direct"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Nexus::NexusDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  void addAllNexusFieldsAsProperties(const std::string &filename);
  void addEnergyToRun();
  void addFacility();
  void addPulseInterval();

  void fillStaticWorkspace(const Nexus::NXEntry &entry, const std::vector<std::string> &monitorList, bool convertToTOF);
  void fillScanWorkspace(const Nexus::NXEntry &entry, const std::vector<std::string> &monitorList);

  std::vector<std::string> getMonitorInfo(const Nexus::NXEntry &firstEntry);
  void initWorkspace(const Nexus::NXEntry &entry);

  void loadInstrumentDetails(const Nexus::NXEntry &);
  void loadTimeDetails(const Nexus::NXEntry &entry);

  std::vector<double> prepareAxis(const Nexus::NXEntry &entry, bool convertToTOF);

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName;    ///< Name of the instrument
  std::string m_instrumentAddress; ///< Name of the instrument address

  // Variables describing the data in the detector
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;    // number of histograms (individual detectors)
  size_t m_numberOfMonitors;      // number of monitors

  // Values parsed from the nexus file
  double m_wavelength;
  double m_channelWidth;
  double m_timeOfFlightDelay;
  std::string m_monitorName;
  bool m_isScan; // whether the loaded data is a scan measurement
};

} // namespace DataHandling
} // namespace Mantid
