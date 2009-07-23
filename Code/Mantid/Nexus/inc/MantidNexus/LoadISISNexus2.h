#ifndef MANTID_DATAHANDLING_LoadISISNexus22_H_
#define MANTID_DATAHANDLING_LoadISISNexus22_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"
#include <climits>

#include <boost/shared_array.hpp>
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
    namespace NeXus
    {

        /** @class LoadISISNexus2 LoadISISNexus2.h NeXus/LoadISISNexus2.h

        Loads a file in a Nexus format and stores it in a 2D workspace 
        (Workspace2D class). LoadISISNexus2 is an algorithm and as such inherits
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

        Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
        class DLLExport LoadISISNexus2 : public API::Algorithm
        {
        public:
            /// Default constructor
            LoadISISNexus2();
            /// Destructor
            ~LoadISISNexus2() {}
            /// Algorithm's name for identification overriding a virtual method
            virtual const std::string name() const { return "LoadISISNexus"; }
            /// Algorithm's version for identification overriding a virtual method
            virtual const int version() const { return 2; }
            /// Algorithm's category for identification overriding a virtual method
            virtual const std::string category() const { return "DataHandling"; }

        private:
            /// Overwrites Algorithm method.
            void init();
            /// Overwrites Algorithm method
            void exec();

            void runLoadInstrument(DataObjects::Workspace2D_sptr);
            void loadLogs(DataObjects::Workspace2D_sptr, NXEntry entry,int period = 1);

            /// User input spectra selection
            std::vector<int> getSpectraSelection();

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
            /// The value of the spectrum_list property
            std::vector<int> m_spec_list;
            /// The value of the spectrum_min property
            int m_spec_min;
            /// The value of the spectrum_max property
            int m_spec_max;
			  /// The number of the input entry
            int m_entrynumber;
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
            /// Monitors
            std::map<int,std::string> m_monitors;

            /// Personal wrapper for sqrt to allow msvs to compile
            static double dblSqrt(double in);
        };

    } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadISISNexus2_H_*/
