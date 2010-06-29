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

  void loadPixelMap(const std::string &filename);

  void openEventFile(const std::string &filename);

  void readPulseidFile(const std::string &filename);

  void fixPixelId(PixelType &pixel, uint32_t &period) const;

  void procEvents(DataObjects::EventWorkspace_sptr & workspace);

  std::size_t getFrameIndex(const std::size_t event_index,
                              const std::size_t last_frame_index);
};

  }
}
#endif /*LOADEVENTPRENEXUS_H_*/
