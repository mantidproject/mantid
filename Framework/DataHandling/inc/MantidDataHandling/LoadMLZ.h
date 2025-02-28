// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"

namespace Mantid {
namespace DataHandling {
/**
    LoadMLZ : Loads MLZ nexus or hdf file into a Mantid workspace.
 */
class MANTID_DATAHANDLING_DLL LoadMLZ : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadMLZ();

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a nexus file from MLZ facility."; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;

  void loadInstrumentDetails(const NeXus::NXEntry &);
  void loadTimeDetails(const NeXus::NXEntry &entry);

  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);

  void initWorkspace(const NeXus::NXEntry &entry);
  void initInstrumentSpecific();
  void loadRunDetails(NeXus::NXEntry &entry);
  void loadExperimentDetails(const NeXus::NXEntry &entry);

  NeXus::NXData loadNexusFileData(NeXus::NXEntry &entry);
  void maskDetectors(const NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(const NeXus::NXEntry &entry); //, int ElasticPeakPosition = -1);

  void runLoadInstrument();

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument
  std::string m_instrumentPath; ///< Name of the instrument path

  // Variables describing the data in the detector
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;

  /* Values parsed from the nexus file */
  int m_monitorElasticPeakPosition;
  double m_wavelength;
  double m_channelWidth;
  double m_timeOfFlightDelay;
  int m_monitorCounts;
  double m_chopper_speed;
  int m_chopper_ratio;

  double m_l1;
  double m_l2;

  double m_t1; // time of flight from source to sample

  std::vector<std::string> m_supportedInstruments;
};

} // namespace DataHandling
} // namespace Mantid
