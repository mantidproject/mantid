#ifndef MANTID_DATAHANDLING_LOADISISNEXUS_H_
#define MANTID_DATAHANDLING_LOADISISNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include <napi.h>
#include <climits>

#include <boost/shared_array.hpp>
#include "MantidAPI/WorkspaceGroup.h"
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
    namespace NeXus
    {

        /** Structure for keeping information about a Nexus data set,
         *  such as the dimensions and type
         */
        struct NexusInfo
        {
            int rank;    ///< number of dimensions of the data
            int dims[4]; ///< sizes along each dimension
            int type;    ///< type of the data, e.g. NX_CHAR, NX_FLOAT32, see napi.h
        };

        /** @class LoadISISNexus LoadISISNexus.h NeXus/LoadISISNexus.h

        Loads a file in a Nexus format and stores it in a 2D workspace 
        (Workspace2D class). LoadISISNexus is an algorithm and as such inherits
        from the Algorithm class, via DataHandlingCommand, and overrides
        the init() & exec() methods.

        Required Properties:
        <UL>
        <LI> Filename - The name of and path to the input Nexus file </LI>
        <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
        (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)
        [ not yet implemented for Nexus ]</LI>
        </UL>

        Optional Properties: (note that these options are not available if reading a multiperiod file)
        <UL>
        <LI> spectrum_min  - The spectrum to start loading from</LI>
        <LI> spectrum_max  - The spectrum to load to</LI>
        <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
        </UL>

        @author Roman Tolchenov, Tessella plc

        Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */
        class DLLExport LoadISISNexus : public API::Algorithm
        {
        public:
            /// Default constructor
            LoadISISNexus();
            /// Destructor
            virtual ~LoadISISNexus() {}
            /// Algorithm's name for identification overriding a virtual method
            virtual const std::string name() const { return "LoadISISNexus"; }
            /// Algorithm's version for identification overriding a virtual method
            virtual int version() const { return 1; }
            /// Algorithm's category for identification overriding a virtual method
            virtual const std::string category() const { return "DataHandling"; }

        private:
            /// Overwrites Algorithm method.
            void init();
            /// Overwrites Algorithm method
            void exec();

            void checkOptionalProperties();
            void loadData(int, int, int&, DataObjects::Workspace2D_sptr );
            void runLoadInstrument(DataObjects::Workspace2D_sptr);
            void loadMappingTable(DataObjects::Workspace2D_sptr);
            void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);
            void parseISODateTime(const std::string & datetime_iso, std::string & date, std::string & time) const;
            template<class TYPE>
            TYPE getEntryValue(const std::string & name);
            template<class TYPE>
            TYPE getNXData(const std::string & name);

            void loadLogs(DataObjects::Workspace2D_sptr,int period = 1);

            /// The name and path of the input file
            std::string m_filename;
            /// The instrument name from Nexus
            std::string m_instrument_name;
            /// The sample name read from Nexus
            std::string m_samplename;

            /// The number of spectra in the raw file
            int m_numberOfSpectra;
            /// The number of periods in the raw file
            int m_numberOfPeriods;
            /// The nuber of time chanels per spectrum
            int m_numberOfChannels;
            /// Has the spectrum_list property been set?
            bool m_list;
            /// Have the spectrum_min/max properties been set?
            bool m_interval;
			  /// The number of the input entry
            int m_entrynumber;
            /// The value of the spectrum_list property
            std::vector<int> m_spec_list;
            /// The value of the spectrum_min property
            int m_spec_min;
            /// The value of the spectrum_max property
            int m_spec_max;
            /// The group which each detector belongs to in order
            std::vector<int> m_groupings;
            /// Time channels
            boost::shared_ptr<MantidVec> m_timeChannelsVec;
            /// Counts buffer
            boost::shared_array<int> m_data;
            /// Proton charge
            double m_proton_charge;
            /// Spectra numbers
            boost::shared_array<int> m_spec;

            /// Nexus file id
            NXhandle m_fileID;
            /// Reads in a string value from the nexus file
            std::string getNexusString(const std::string& name)const;
            /// Opens a Nexus group.
            void openNexusGroup(const std::string& name, const std::string& nx_class)const;
            /// Closes Nexus group
            void closeNexusGroup()const;
            /// Reads in the dimensions: number of periods, spectra and time bins.
            void readDataDimensions();
            /// Get time channels
            void getTimeChannels();
            /// Opens a Nexus data set
            void openNexusData(const std::string& name);
            /// Close a Nexus data set
            void closeNexusData();
            /// Get the data from Nexus
            void getNexusData(void* p);
            /// Get info for the open data set
            NexusInfo getNexusInfo();
            
            /// Personal wrapper for sqrt to allow msvs to compile
            static double dblSqrt(double in);
        };
      
      /**
       * Get a value from the nexus file. The name should be relative and the parent group already opened.
       * @param name The name of the NX entry
       * @returns The value of entry
       */
      template<class TYPE>
      TYPE LoadISISNexus::getEntryValue(const std::string & name)
      {
	openNexusData(name);
	TYPE value;
	getNexusData(&value);
	closeNexusData();
	return value;
      }

      /**
       * Get the first entry from an NX data group
       * @param name The group name
       * @returns The data value
       */
      template<class TYPE>
      TYPE LoadISISNexus::getNXData(const std::string & name)
      {
	openNexusData(name);
	TYPE value[1];
	getNexusData(value);
	closeNexusData();
	return value[0];
      }




    } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADISISNEXUS_H_*/
