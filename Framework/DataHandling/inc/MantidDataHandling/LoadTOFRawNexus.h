// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
#include "MantidNexus/NexusClasses.h"

#include <mutex>

namespace Mantid {

namespace DataHandling {
/**
 Loads a NeXus file that conforms to the TOFRaw instrument definition format and
 stores it in a 2D workspace.
 */
class MANTID_DATAHANDLING_DLL LoadTOFRawNexus : public API::IFileLoader<Kernel::NexusHDF5Descriptor> {
public:
  /// Default Constructor
  LoadTOFRawNexus();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadTOFRawNexus"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a NeXus file confirming to the TOFRaw format"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  static std::string getEntryName(const std::string &filename);

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

  void countPixels(const std::string &nexusfilename, const std::string &entry_name,
                   std::vector<std::string> &bankNames);

  /// Number of pixels
  size_t m_numPixels;

  /// Signal # to load. Default 1
  int m_signalNo;

protected:
  void init() override;
  void exec() override;

  /// Validate the optional input properties
  void checkOptionalProperties();

  /// Run LoadInstrument as a ChildAlgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr);

  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr, Mantid::NeXus::NXEntry &entry);

  void loadBank(const std::string &nexusfilename, const std::string &entry_name, const std::string &bankName,
                const API::MatrixWorkspace_sptr &WS, const detid2index_map &id_to_wi);

  /// List of the absolute time of each pulse
  std::vector<Types::Core::DateAndTime> pulseTimes;

  /// Number of bins
  size_t m_numBins;

  /// Interval of chunk
  specnum_t m_spec_min, m_spec_max;

  /// Name of the 'data' field to load (depending on Signal)
  std::string m_dataField;

  /// Name of the 'axis' field to load (depending on Signal)
  std::string m_axisField;

  /// Units of the X axis found
  std::string m_xUnits;

  /// Mutex to avoid simultaneous file access
  std::mutex m_fileMutex;

  /// Flag for whether or not to assume the data is old SNS raw files;
  bool m_assumeOldFile;
};

} // namespace DataHandling
} // namespace Mantid
