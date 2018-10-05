// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADILLINDIRECT2_H_
#define MANTID_DATAHANDLING_LOADILLINDIRECT2_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/**
  Loads an ILL IN16B nexus file into a Mantid workspace.
*/
class DLLExport LoadILLIndirect2
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLIndirect2();
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a ILL/IN16B nexus file.";
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  void loadDataDetails(NeXus::NXEntry &entry);
  void initWorkSpace(NeXus::NXEntry &entry,
                     std::vector<std::vector<int>> monitorsData);
  void setInstrumentName(const NeXus::NXEntry &firstEntry,
                         const std::string &instrumentNamePath);
  void loadNexusEntriesIntoProperties(std::string nexusfilename);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                                std::vector<std::vector<int>> monitorsData);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }

  void moveComponent(const std::string &, double);
  void moveSingleDetectors(NeXus::NXEntry &entry);

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument

  // Variables describing the data in the detector
  size_t m_numberOfTubes;           // number of tubes - X
  size_t m_numberOfPixelsPerTube;   // number of pixels per tube - Y
  size_t m_numberOfChannels;        // time channels - Z
  size_t m_numberOfSimpleDetectors; // number of simple detector
  size_t m_numberOfHistograms;      // number of spectra
  size_t m_numberOfMonitors;        // number of monitor spectra
  std::set<int> m_activeSDIndices;  // set of Single Detector indices,
                                    // that were actually active

  std::vector<std::string> m_supportedInstruments;
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLINDIRECT2_H_ */
