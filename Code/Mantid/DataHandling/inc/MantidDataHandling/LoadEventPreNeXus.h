#ifndef LOADEVENTPRENEXUS_H_
#define LOADEVENTPRENEXUS_H_

#include <fstream>
#include <string>
#include <vector>
#ifndef _WIN32
#include <stdint.h>
#endif
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"


namespace Mantid
{
  namespace DataHandling
  {

/// Make the code clearer by having this an explicit type
typedef uint32_t PixelType;

/// Structure that matches the form in the binary event list.
struct DasEvent
{
  /// Time of flight.
  uint32_t tof;
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

  std::vector<int> spectra_list;
  std::vector<int> period_list;
  std::vector<boost::posix_time::ptime> pulsetimes;
  std::vector<uint64_t> event_indices;

  std::ifstream * eventfile;
  std::size_t num_events;
  std::size_t num_pulses;
  uint32_t numpixel;

  std::size_t num_good_events;
  std::size_t num_error_events;

  void load_pixel_map(const std::string & );
  void open_event_file(const std::string &);
  void read_pulseid_file(const std::string &);

  /// Turn a pixel id into a "corrected" pixelid and period.
  void fix_pixel_id(PixelType &, uint32_t &) const;

  /// Process the event file properly.
  void proc_events(DataObjects::EventWorkspace_sptr &);

  /**
   * Determine the frame index from the event index.
   * @param event_index The index of the event.
   * @param last_frame_index Last frame found. This parameter reduces the
   * search to be from the current point forward.
   */
  std::size_t get_frame_index(const std::size_t event_index,
                              const std::size_t last_frame_index);
};

  }
}
#endif /*LOADEVENTPRENEXUS_H_*/
