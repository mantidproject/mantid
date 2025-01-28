// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadLLB : Loads an LLB MIBEMOL TOF NeXus file into a Workspace2D with the
  given name.
*/
class MANTID_DATAHANDLING_DLL LoadLLB : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadLLB();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads LLB nexus file."; }

  int version() const override;
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  void setInstrumentName(const NeXus::NXEntry &entry);
  void initWorkSpace(const NeXus::NXEntry &entry);
  void loadTimeDetails(const NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(const NeXus::NXEntry &entry);
  int getDetectorElasticPeakPosition(const NeXus::NXFloat &);
  void setTimeBinning(HistogramData::HistogramX &histX, int, double);
  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  void loadExperimentDetails(const NeXus::NXEntry &entry);
  void loadRunDetails(const NeXus::NXEntry &);
  void runLoadInstrument();

  std::vector<std::string> m_supportedInstruments;
  std::string m_instrumentName;
  std::string m_instrumentPath; ///< Name of the instrument path

  API::MatrixWorkspace_sptr m_localWorkspace;
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;
  double m_wavelength;
  double m_channelWidth;
};

} // namespace DataHandling
} // namespace Mantid
