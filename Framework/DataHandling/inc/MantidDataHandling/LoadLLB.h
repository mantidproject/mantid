// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADLLB_H_
#define MANTID_DATAHANDLING_LOADLLB_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadLLB : Loads an LLB MIBEMOL TOF NeXus file into a Workspace2D with the
  given name.
*/
class DLLExport LoadLLB : public API::IFileLoader<Kernel::NexusDescriptor> {
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
  void setInstrumentName(NeXus::NXEntry &entry);
  void initWorkSpace(NeXus::NXEntry &);
  void loadTimeDetails(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &);
  int getDetectorElasticPeakPosition(const NeXus::NXFloat &);
  void setTimeBinning(HistogramData::HistogramX &histX, int, double);
  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  void loadExperimentDetails(NeXus::NXEntry &);
  void loadRunDetails(NeXus::NXEntry &);
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

  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADLLB_H_ */
