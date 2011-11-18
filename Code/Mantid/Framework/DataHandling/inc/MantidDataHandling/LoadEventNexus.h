#ifndef MANTID_DATAHANDLING_LOADEVENTNEXUS_H_
#define MANTID_DATAHANDLING_LOADEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"
#include "MantidDataObjects/EventWorkspace.h"

//Copy of the NexusCpp API was placed in MantidNexus
#include "nexus/NeXusFile.hpp"
#include "nexus/NeXusException.hpp"
#include "MantidDataObjects/Events.h"


namespace Mantid
{

  namespace DataHandling
  {
    /** @class LoadEventNexus LoadEventNexus.h Nexus/LoadEventNexus.h

    Load Event Nexus files.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NeXus file </LI>
    <LI> Workspace - The name of the workspace to output</LI>
    </UL>

    @author Janik Zikovsky, SNS
    @date Sep 27, 2010

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
    class DLLExport LoadEventNexus : public API::IDataFileChecker
    {
    public:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Default constructor
      LoadEventNexus();

      /// Destructor
      virtual ~LoadEventNexus()
      {
      }

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadEventNexus";};

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling";}

      /// do a quick check that this file can be loaded 
      bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      int fileCheck(const std::string& filePath);

      /** Sets whether the pixel counts will be pre-counted.
       * @param value :: true if you want to precount. */
      void setPrecount(bool value)
      {
        precount = value;
      }

    public:
      void init();
      void exec();

      /// The name and path of the input file
      std::string m_filename;

      /// The workspace being filled out
      DataObjects::EventWorkspace_sptr WS;

      /// Filter by a minimum time-of-flight
      double filter_tof_min;
      /// Filter by a maximum time-of-flight
      double filter_tof_max;

      /// Filter by start time
      Kernel::DateAndTime filter_time_start;
      /// Filter by stop time
      Kernel::DateAndTime filter_time_stop;

      /// Was the instrument loaded?
      bool instrument_loaded_correctly;

      /// Limits found to tof
      double longest_tof;
      /// Limits found to tof
      double shortest_tof;

      /// List of the absolute time of each pulse
      std::vector<Kernel::DateAndTime> pulseTimes;

      /// Do we pre-count the # of events in each pixel ID?
      bool precount;

      /// Tolerance for CompressEvents; use -1 to mean don't compress.
      double compressTolerance;

      /// Do we load the sample logs?
      bool loadlogs;
      
      /// Pointer to the vector of events
      typedef std::vector<Mantid::DataObjects::TofEvent> * EventVector_pt;

      /// Vector where index = event_id; value = ptr to std::vector<TofEvent> in the event list.
      std::vector<EventVector_pt> eventVectors;

      /// Maximum (inclusive) event ID possible for this instrument
      int32_t eventid_max;

      /// Pointer to the map (key = pixel ID, value = workspace index)
      detid2index_map * pixelID_to_wi_map;

      /// True if the event_id is spectrum no not pixel ID
      bool event_id_is_spec;

      DataObjects::EventWorkspace_sptr createEmptyEventWorkspace();
      void makeMapToEventLists();
      void loadEvents(API::Progress * const prog, const bool monitors);
      void createSpectraMapping(const std::string &nxsfile, API::MatrixWorkspace_sptr workspace,
                                const bool monitorsOnly, const std::string & bankName = "");
      bool hasEventMonitors();
      void runLoadMonitors();
      /// Set the filters on TOF.
      void setTimeFilters(const bool monitors);

      static void loadEntryMetadata(const std::string &nexusfilename, Mantid::API::MatrixWorkspace_sptr WS,
          const std::string &entry_name);

      static bool runLoadInstrument(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace,
          const std::string & top_entry_name, Algorithm * alg);

      static bool runLoadNexusLogs(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace,
          std::vector<Kernel::DateAndTime> & pulseTimes, Algorithm * alg);

      /// Load a spectra mapping from the given file
      static Geometry::ISpectraDetectorMap * loadSpectraMapping(const std::string & filename, Geometry::Instrument_const_sptr inst,
          const bool monitorsOnly, const std::string entry_name, Mantid::Kernel::Logger & g_log);

    private:

      // ISIS specific methods for dealing with wide events
      static void loadTimeOfFlight(const std::string &nexusfilename, DataObjects::EventWorkspace_sptr WS,
          const std::string &entry_name, const std::string &classType);

      static void loadTimeOfFlightData(::NeXus::File& file, DataObjects::EventWorkspace_sptr WS, 
        const std::string& binsName,size_t start_wi = 0, size_t end_wi = 0);

    private:
      /// name of top level NXentry to use
      std::string m_top_entry_name;
      /// Set the top entry field name
      void setTopEntryName();

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADEVENTNEXUS_H_*/

