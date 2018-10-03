// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADMLZ_H_
#define MANTID_DATAHANDLING_LOADMLZ_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/**
    LoadMLZ : Loads MLZ nexus or hdf file into a Mantid workspace.
 */
class DLLExport LoadMLZ : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadMLZ();

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a nexus file from MLZ facility.";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;

  void loadInstrumentDetails(NeXus::NXEntry &);
  void loadTimeDetails(NeXus::NXEntry &entry);

  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);

  void initWorkSpace(NeXus::NXEntry &entry);
  void initInstrumentSpecific();
  void loadRunDetails(NeXus::NXEntry &entry);
  void loadExperimentDetails(NeXus::NXEntry &entry);

  NeXus::NXData loadNexusFileData(NeXus::NXEntry &entry);
  void maskDetectors(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(
      NeXus::NXEntry &entry); //, int ElasticPeakPosition = -1);

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
  LoadHelper m_mlzloader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADMLZ_H_ */
