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
#include "MantidAPI/ISISRunLogs.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataObjects/Workspace2D_fwd.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <boost/scoped_ptr.hpp>

#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace Mantid {
namespace DataHandling {

/**

Loads a file in a NeXus format and stores it in a 2D workspace. LoadISISNexus2
is an algorithm and
as such inherits  from the Algorithm class, via DataHandlingCommand, and
overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data
(a multi-period file will store higher periods in workspaces called
OutputWorkspace_PeriodNo)</LI>
</UL>

Optional Properties: (note that these options are not available if reading a
multi-period file)
<UL>
<LI> SpectrumMin  - The  starting spectrum number</LI>
<LI> SpectrumMax  - The  final spectrum number (inclusive)</LI>
<LI> SpectrumList - An ArrayProperty of spectra to load</LI>
</UL>

@author Roman Tolchenov, Tessella plc
*/
class MANTID_DATAHANDLING_DLL LoadISISNexus2 : public API::IFileLoader<Kernel::NexusHDF5Descriptor> {
public:
  /// Default constructor
  LoadISISNexus2();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadISISNexus"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadEventNexus", "SaveISISNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a file in ISIS NeXus format."; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

  /// Spectra block descriptor
  struct SpectraBlock {
    /// Constructor - initialize the block
    SpectraBlock(int64_t f, int64_t l, bool is_mon, const std::string &monname)
        : first(f), last(l), isMonitor(is_mon), monName(monname) {}

    int64_t first;  ///< first spectrum number of the block
    int64_t last;   ///< last spectrum number of the block
    bool isMonitor; ///< is the data in a monitor group
    std::string monName;
  };

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  // Validate the optional input properties
  bool checkOptionalProperties(bool bseparateMonitors, bool bexcludeMonitor);

  /// Prepare a vector of SpectraBlock structures to simplify loading
  size_t prepareSpectraBlocks(std::map<specnum_t, std::string> &monitors, DataBlockComposite &LoadBlock);
  /// Run LoadInstrument as a ChildAlgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr &);
  /// Load in details about the run
  void loadRunDetails(DataObjects::Workspace2D_sptr &local_workspace, Mantid::NeXus::NXEntry &entry);
  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr &, const Mantid::NeXus::NXEntry &entry);
  /// Load log data from the nexus file
  void loadLogs(DataObjects::Workspace2D_sptr &ws);
  // Load a given period into the workspace
  void loadPeriodData(int64_t period, Mantid::NeXus::NXEntry &entry, DataObjects::Workspace2D_sptr &local_workspace,
                      bool update_spectra2det_mapping = false);
  // Load a data block
  void loadBlock(Mantid::NeXus::NXDataSetTyped<int> &data, int64_t blocksize, int64_t period, int64_t start,
                 int64_t &hist, int64_t &spec_num, DataObjects::Workspace2D_sptr &local_workspace);

  // Create period logs
  void createPeriodLogs(int64_t period, DataObjects::Workspace2D_sptr &local_workspace);
  // Validate multi-period logs
  void validateMultiPeriodLogs(const Mantid::API::MatrixWorkspace_sptr &);

  // build the list of spectra numbers to load and include in the spectra list
  void buildSpectraInd2SpectraNumMap(bool range_supplied, bool hasSpectraList,
                                     const DataBlockComposite &dataBlockComposite);

  /// Check if any of the spectra block ranges overlap
  void checkOverlappingSpectraRange();

  /// The name and path of the input file
  std::string m_filename;
  /// The instrument name from Nexus
  std::string m_instrument_name;
  /// The sample name read from Nexus
  std::string m_samplename;
  // the description of the data block in the file to load.
  // the description of single time-range data block, obtained from detectors
  DataBlockComposite m_detBlockInfo;
  // the description of single time-range data block, obtained from monitors
  DataBlockComposite m_monBlockInfo;
  // description of the block to be loaded may include monitors and detectors
  // with the same time binning if the detectors and monitors are loaded
  // together
  // in single workspace or equal to the detectorBlock if monitors are excluded
  // or monBlockInfo if only monitors are loaded.
  DataBlockComposite m_loadBlockInfo;
  /// Is there a detector block
  bool m_have_detector;
  // Is there a VMS block
  bool m_hasVMSBlock;
  /// if true, a spectra list or range of spectra is supplied
  bool m_load_selected_spectra;
  /// map of workspace Index to spectra Number (spectraID)
  std::map<int64_t, specnum_t> m_wsInd2specNum_map;
  /// spectra Number to detector ID (multi)map
  API::SpectrumDetectorMapping m_spec2det_map;
  /// The number of the input entry
  int64_t m_entrynumber;
  /// List of disjoint data blocks to load
  std::vector<SpectraBlock> m_spectraBlocks;
  /// Time channels
  std::shared_ptr<HistogramData::HistogramX> m_tof_data;
  /// Spectra numbers
  std::vector<specnum_t> m_spec;
  /// Pointer to one-past-the-end of spectrum number array (m_spec)
  const specnum_t *m_spec_end;
  /// Monitors, map spectrum index to monitor group name
  std::map<specnum_t, std::string> m_monitors;
  /// A pointer to the ISISRunLogs creator
  boost::scoped_ptr<API::ISISRunLogs> m_logCreator;
  /// Progress reporting object
  std::shared_ptr<API::Progress> m_progress;
  /// Personal wrapper for sqrt to allow msvs to compile
  static double dblSqrt(double in);
  // Handle to the NeXus file
  boost::scoped_ptr<::NeXus::File> m_nexusFile;

  bool findSpectraDetRangeInFile(const NeXus::NXEntry &entry, std::vector<specnum_t> &spectrum_index, int64_t ndets,
                                 int64_t n_vms_compat_spectra, const std::map<specnum_t, std::string> &monitors,
                                 bool excludeMonitors, bool separateMonitors);

  /// Check if is the file is a multiple time regime file
  bool isMultipleTimeRegimeFile(const NeXus::NXEntry &entry) const;
};

} // namespace DataHandling
} // namespace Mantid
