#ifndef MANTID_NEXUS_LOADSNSEVENTNEXUS_H_
#define MANTID_NEXUS_LOADSNSEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
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
    /** @class LoadSNSEventNexus LoadSNSEventNexus.h Nexus/LoadSNSEventNexus.h

    Load SNS EventNexus files.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NEXUS file </LI>
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
    class DLLExport LoadSNSEventNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadSNSEventNexus();

      /// Destructor
      virtual ~LoadSNSEventNexus() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadSNSEventNexus";};

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus";}

    private:

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

      void loadBankEventData(const std::string entry_name, API::IndexToIndexMap * pixelID_to_wi_map);
      void runLoadInstrument(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace);
      void runLoadMonitors();

    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADSNSEVENTNEXUS_H_*/

