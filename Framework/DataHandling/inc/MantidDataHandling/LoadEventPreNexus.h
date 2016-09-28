#ifndef LOADEVENTPRENEXUS_H_
#define LOADEVENTPRENEXUS_H_

#include <vector>
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"

namespace Mantid {
namespace DataHandling {
/** @class Mantid::DataHandling::LoadEventPreNexus

    A data loading routine for SNS pre-nexus event files

    Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
*/

/// This define is used to quickly turn parallel code on or off.
#undef LOADEVENTPRENEXUS_ALLOW_PARALLEL

/// Make the code clearer by having this an explicit type
typedef int PixelType;

/// Type for the DAS time of flight (data file)
typedef int DasTofType;

/// Structure that matches the form in the binary event list.
#pragma pack(push, 4) // Make sure the structure is 8 bytes.
struct DasEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};
#pragma pack(pop)

/// Structure used as an intermediate for parallel processing of events
#pragma pack(push, 4) // Make sure the structure is 8 bytes.
struct IntermediateEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
  /// Frame index (pulse # of this event)
  size_t frame_index;
  /// Period of the event (not really used at this time)
  uint32_t period;
};
#pragma pack(pop)

/// Structure that matches the form in the new pulseid files.
#pragma pack(push, 4) // Make sure the structure is 16 bytes.
struct Pulse {
  /// The number of nanoseconds since the seconds field. This is not necessarily
  /// less than one second.
  uint32_t nanoseconds;

  /// The number of seconds since January 1, 1990.
  uint32_t seconds;

  /// The index of the first event for this pulse.
  uint64_t event_index;

  /// The proton charge for the pulse.
  double pCurrent;
};
#pragma pack(pop)

class DLLExport LoadEventPreNexus
    : public API::IFileLoader<Kernel::FileDescriptor>,
      public API::DeprecatedAlgorithm {
public:
  /// Constructor
  LoadEventPreNexus();
  /// Virtual destructor
  ~LoadEventPreNexus() override;
  /// Algorithm's name
  const std::string name() const override { return "LoadEventPreNexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads SNS raw neutron event data format and stores it in a "
           "workspace (EventWorkspace class).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\PreNexus";
  }
  /// Algorithm's aliases
  const std::string alias() const override { return "LoadEventPreNeXus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void loadPixelMap(const std::string &filename);

  void openEventFile(const std::string &filename);

  void readPulseidFile(const std::string &filename, const bool throwError);

  void runLoadInstrument(const std::string &eventfilename,
                         API::MatrixWorkspace_sptr localWorkspace);

  inline void fixPixelId(PixelType &pixel, uint32_t &period) const;

  void procEvents(DataObjects::EventWorkspace_sptr &workspace);

  void procEventsLinear(DataObjects::EventWorkspace_sptr &workspace,
                        std::vector<DataObjects::TofEvent> **arrayOfVectors,
                        DasEvent *event_buffer,
                        size_t current_event_buffer_size, size_t fileOffset);

  void setProtonCharge(DataObjects::EventWorkspace_sptr &workspace);

  std::unique_ptr<API::Progress> m_progress;

  /// The list of Spectra
  std::vector<int64_t> m_spectra_list;

  /// The times for each pulse.
  std::vector<Kernel::DateAndTime> m_pulsetimes;

  /// The index of the first event in each pulse.
  std::vector<uint64_t> m_event_indices;

  /// The proton charge on a pulse by pulse basis.
  std::vector<double> m_proton_charge;

  /// The total proton charge for the run.
  double m_proton_charge_tot;

  /// The value of the vector is the workspace index. The index into it is the
  /// pixel ID from DAS
  std::vector<std::size_t> m_pixel_to_wkspindex;

  /// Map between the DAS pixel IDs and our pixel IDs, used while loading.
  std::vector<PixelType> m_pixelmap;

  /// The maximum detector ID possible
  Mantid::detid_t m_detid_max;

  /// Handles loading from the event file
  Mantid::Kernel::BinaryFile<DasEvent> *m_eventfile;

  /// The number of events in the file
  std::size_t m_num_events;

  /// The number of pulses
  std::size_t m_num_pulses; 

  /// The number of pixels
  uint32_t m_numpixel;      

  /// The number of good events loaded
  std::size_t m_num_good_events;  

  /// The number of error events encountered
  std::size_t m_num_error_events; 

  /// The number of events that were ignored (not loaded) because, e.g. of only
  /// loading some spectra.
  std::size_t m_num_ignored_events;

  /// The first event to load (count from zero)
  std::size_t m_first_event; 

  /// Number of events to load
  std::size_t m_max_events;

  /// Set to true if a valid Mapping file was provided.
  bool m_using_mapping_file;

  /// For loading only some spectra
  bool m_loadOnlySomeSpectra;
  
  /// Handle to the loaded spectra map
  std::map<int64_t, bool> m_spectraLoadMap;

  /// Longest TOF limit
  double m_longest_tof;
  /// Shortest TOF limit
  double m_shortest_tof;

  /// Flag to allow for parallel loading
  bool m_parallelProcessing;
};
}
}
#endif /*LOADEVENTPRENEXUS_H_*/
