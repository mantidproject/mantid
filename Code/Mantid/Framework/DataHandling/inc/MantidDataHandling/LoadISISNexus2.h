#ifndef MANTID_DATAHANDLING_LoadISISNexus22_H_
#define MANTID_DATAHANDLING_LoadISISNexus22_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include <nexus/NeXusFile.hpp>

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

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadISISNexus2
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Default constructor
  LoadISISNexus2();
  /// Destructor
  virtual ~LoadISISNexus2() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadISISNexus"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Nexus"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a file in ISIS NeXus format.";
  }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::NexusDescriptor &descriptor) const;

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

  /// The structure describes parameters of a single time-block written in the
  /// nexus file
  struct DataBlock {
    // The number of data periods
    int numberOfPeriods;
    // The number of time channels per spectrum (N histogram bins -1)
    std::size_t numberOfChannels;
    // The number of spectra
    size_t numberOfSpectra;
    // minimal spectra Id (by default 1, undefined -- max_value)
    int64_t spectraID_min;
    // maximal spectra Id (by default 1, undefined  -- 0)
    int64_t spectraID_max;

    DataBlock()
        : numberOfPeriods(0), numberOfChannels(0), numberOfSpectra(0),
          spectraID_min(std::numeric_limits<int64_t>::max()), spectraID_max(0) {
    }

    DataBlock(const NeXus::NXInt &data)
        : numberOfPeriods(data.dim0()), numberOfChannels(data.dim2()),
          numberOfSpectra(data.dim1()),
          spectraID_min(std::numeric_limits<int64_t>::max()),
          spectraID_max(0){};
  };

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
  // Validate the optional input properties
  void checkOptionalProperties(
      const std::map<int64_t, std::string> &ExcludedMonitors);
  /// Prepare a vector of SpectraBlock structures to simplify loading
  size_t
  prepareSpectraBlocks(std::map<int64_t, std::string> &monitors,
                       const std::map<int64_t, specid_t> &specInd2specNum_map,
                       const DataBlock &LoadBlock);
  /// Run LoadInstrument as a ChildAlgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr &);
  /// Load in details about the run
  void loadRunDetails(DataObjects::Workspace2D_sptr &local_workspace,
                      Mantid::NeXus::NXEntry &entry);
  /// Parse an ISO formatted date-time string into separate date and time
  /// strings
  void parseISODateTime(const std::string &datetime_iso, std::string &date,
                        std::string &time) const;
  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr &,
                      Mantid::NeXus::NXEntry &entry);
  /// Load log data from the nexus file
  void loadLogs(DataObjects::Workspace2D_sptr &ws,
                Mantid::NeXus::NXEntry &entry);
  // Load a given period into the workspace
  void loadPeriodData(int64_t period, Mantid::NeXus::NXEntry &entry,
                      DataObjects::Workspace2D_sptr &local_workspace);
  // Load a data block
  void loadBlock(Mantid::NeXus::NXDataSetTyped<int> &data, int64_t blocksize,
                 int64_t period, int64_t start, int64_t &hist,
                 int64_t &spec_num,
                 DataObjects::Workspace2D_sptr &localWorkspace);

  // Create period logs
  void createPeriodLogs(int64_t period,
                        DataObjects::Workspace2D_sptr &local_workspace);
  // Validate multi-period logs
  void validateMultiPeriodLogs(Mantid::API::MatrixWorkspace_sptr);
  // build the list of spectra numbers to load and include in the spectra list
  void buildSpectraInd2SpectraNumMap(
      bool range_supplied, int64_t range_min, int64_t range_max,
      const std::vector<int64_t> &spec_list,
      const std::map<int64_t, std::string> &ExcludedMonitors);

  /// The name and path of the input file
  std::string m_filename;
  /// The instrument name from Nexus
  std::string m_instrument_name;
  /// The sample name read from Nexus
  std::string m_samplename;

  // the description of the data block in the file to load.
  // the description of single time-range data block, obtained from detectors
  DataBlock m_detBlockInfo;
  // the description of single time-range data block, obtained from monitors
  DataBlock m_monBlockInfo;
  // description of the block to be loaded may include monitors and detectors
  // with the same time binning if the detectors and monitors are loaded
  // together
  // in single workspace or equal to the detectorBlock if monitors are excluded
  // or monBlockInfo if only monitors are loaded.
  DataBlock m_loadBlockInfo;

  /// Is there a detector block
  bool m_have_detector;

  /// if true, a spectra list or range of spectra is supplied
  bool m_load_selected_spectra;
  /// map of spectra Index to spectra Number (spectraID)
  std::map<int64_t, specid_t> m_specInd2specNum_map;
  /// spectra Number to detector ID (multi)map
  API::SpectrumDetectorMapping m_spec2det_map;

  /// The number of the input entry
  int64_t m_entrynumber;
  /// List of disjoint data blocks to load
  std::vector<SpectraBlock> m_spectraBlocks;

  /// Time channels
  boost::shared_ptr<MantidVec> m_tof_data;
  /// Proton charge
  double m_proton_charge;
  /// Spectra numbers
  boost::shared_array<int> m_spec;
  /// Pointer to one-past-the-end of spectrum number array (m_spec)
  const int *m_spec_end;
  /// Monitors, map spectrum index to monitor group name
  std::map<int64_t, std::string> m_monitors;

  /// A pointer to the ISISRunLogs creator
  boost::scoped_ptr<ISISRunLogs> m_logCreator;

  /// Progress reporting object
  boost::shared_ptr<API::Progress> m_progress;

  /// Personal wrapper for sqrt to allow msvs to compile
  static double dblSqrt(double in);

  // C++ interface to the NXS file
  boost::scoped_ptr<::NeXus::File> m_cppFile;

  bool findSpectraDetRangeInFile(
      NeXus::NXEntry &entry, boost::shared_array<int> &spectrum_index,
      int64_t ndets, int64_t n_vms_compat_spectra,
      std::map<int64_t, std::string> &monitors, bool excludeMonitors,
      bool separateMonitors, std::map<int64_t, std::string> &ExcludedMonitors);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadISISNexus2_H_*/
