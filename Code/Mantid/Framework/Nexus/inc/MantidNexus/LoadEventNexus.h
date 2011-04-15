#ifndef MANTID_NEXUS_LOADEVENTNEXUS_H_
#define MANTID_NEXUS_LOADEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"

//Copy of the NexusCpp API was placed in MantidNexus
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"


namespace Mantid
{

  namespace NeXus
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
      virtual const std::string category() const { return "Nexus";}

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

//      void loadBankEventData_OBSOLETE(const std::string entry_name, API::IndexToIndexMap * pixelID_to_wi_map, Mantid::API::Progress * prog);
      void loadEntryMetadata(const std::string &entry_name);
      void runLoadInstrument(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace);
      void runLoadMonitors();

    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADEVENTNEXUS_H_*/

