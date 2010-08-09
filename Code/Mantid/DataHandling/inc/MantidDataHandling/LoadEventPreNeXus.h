#ifndef LOADEVENTPRENEXUS_H_
#define LOADEVENTPRENEXUS_H_

#include <fstream>
#include <string>
#include <vector>
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"


namespace Mantid
{
  namespace DataHandling
  {
/** @class Mantid::DataHandling::LoadEventPreNeXus

    A data loading routine for SNS pre-nexus event files
    
    @author Janik, SNS ORNL
    @date 4/02/2010
    
    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

/// Make the code clearer by having this an explicit type
typedef uint32_t PixelType;

/// Type for the DAS time of flight (data file)
typedef uint32_t DasTofType;

/// Structure that matches the form in the binary event list.
struct DasEvent
{
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};

/// Structure that matches the form in the new pulseid files.
struct Pulse
{
  /// The number of nanoseconds since the seconds field. This is not
  /// necessarily less than one second.
  uint32_t nanoseconds;

  /// The number of seconds since January 1, 1990.
  uint32_t seconds;

  /// The index of the first event for this pulse.
  uint64_t event_index;

  /// The proton charge for the pulse.
  double pCurrent;
};


class DLLExport LoadEventPreNeXus : public Mantid::API::Algorithm
{
public:
  /// Constructor
  LoadEventPreNeXus();
  /// Virtual destructor
  virtual ~LoadEventPreNeXus();
  /// Algorithm's name
  virtual const std::string name() const { return "LoadEventPreNeXus"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

  /// Map between the DAS pixel IDs and our pixel IDs, used while loading.
  std::vector<PixelType> pixelmap;

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  std::vector<int> spectra_list; ///<the list of Spectra
  std::vector<int> period_list; ///< the list of periods

  /// The times for each pulse.
  std::vector<boost::posix_time::ptime> pulsetimes;
  /// The index of the first event in each pulse.
  std::vector<uint64_t> event_indices;
  /// The proton charge on a pulse by pulse basis.
  std::vector<double> proton_charge;
  /// The total proton charge for the run.
  double proton_charge_tot;

  std::ifstream * eventfile; ///<File stream to the event file
  std::size_t num_events; ///<the number of events
  std::size_t num_pulses; ///<the number of pulses
  uint32_t numpixel; ///<the number of pixels

  std::size_t num_good_events; ///<the number of good events
  std::size_t num_error_events; ///<the number of error events

  ///Set to true if a valid Mapping file was provided.
  bool using_mapping_file;

  bool instrument_loaded_correctly; /// Set to true when instrument geometry was loaded.

  void loadPixelMap(const std::string &filename);

  void openEventFile(const std::string &filename);

  void readPulseidFile(const std::string &filename);

  void runLoadInstrument(const std::string &filename, API::MatrixWorkspace_sptr localWorkspace);

  void fixPixelId(PixelType &pixel, uint32_t &period) const;

  void procEvents(DataObjects::EventWorkspace_sptr & workspace);

  void procEventsParallel(DataObjects::EventWorkspace_sptr & workspace);

  void setProtonCharge(DataObjects::EventWorkspace_sptr & workspace);

  std::size_t getFrameIndex(const std::size_t event_index,
                              const std::size_t last_frame_index);
};

  }
}
#endif /*LOADEVENTPRENEXUS_H_*/
