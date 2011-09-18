#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS_H_
/*WIKI* 


The algorithm LoadMuonNexus will read a Muon Nexus data file (original format) and place the data
into the named workspace.
The file name can be an absolute or relative path and should have the extension
.nxs or .NXS.
If the file contains data for more than one period, a separate workspace will be generated for each.
After the first period the workspace names will have "_2", "_3", and so on, appended to the given workspace name.
For single period data, the optional parameters can be used to control which spectra are loaded into the workspace.
If spectrum_min and spectrum_max are given, then only that range to data will be loaded.
If a spectrum_list is given than those values will be loaded.
* TODO get XML descriptions of Muon instruments. This data is not in existing Muon Nexus files.
* TODO load the spectra detector mapping. This may be very simple for Muon instruments.

===Time series data===
The log data in the Nexus file (NX_LOG sections) will be loaded as TimeSeriesProperty data within the workspace.
Time is stored as seconds from the Unix epoch.

===Errors===

The error for each histogram count is set as the square root of the number of counts.

===Time bin data===

The ''corrected_times'' field of the Nexus file is used to provide time bin data and the bin edge values are calculated from these
bin centre times.

===Multiperiod data===

To determine if a file contains data from more than one period the field ''switching_states'' is read from the Nexus file.
If this value is greater than one it is taken to be the number of periods, <math>N_p</math> of the data.
In this case the <math>N_s</math> spectra in the ''histogram_data'' field are split with <math>N_s/N_p</math> assigned to each period.

===Subalgorithms used===

The subalgorithms used by LoadMuonNexus are:
* LoadMuonLog - this reads log information from the Nexus file and uses it to create TimeSeriesProperty entries in the workspace.
* LoadInstrument - this algorithm looks for an XML description of the instrument and if found reads it.
* LoadIntstrumentFromNexus - this is called if the normal LoadInstrument fails. As the Nexus file has limited instrument data, this only populates a few fields.

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IDataFileChecker.h"
#include "MantidKernel/System.h"
#include <napi.h>
#include "MantidAPI/SpectraDetectorMap.h"
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MuonNexusReader;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

    Loads an file in Nexus Muon format and stores it in a 2D workspace 
    (Workspace2D class). LoadMuonNexus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NeXus file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)
         [ not yet implemented for Nexus ]</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    <LI> auto_group - Determines whether the spectra are automatically grouped together based on the groupings in the NeXus file. </LI>
    </UL>
    
    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadMuonNexus : public API::IDataFileChecker
    {
    public:
      /// Default constructor
      LoadMuonNexus();
      /// Destructor
      virtual ~LoadMuonNexus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMuonNexus"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }
      
    /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);
    protected:
      /// Overwrites Algorithm method
      void exec();
      
      void checkOptionalProperties();
      void runLoadInstrument(DataObjects::Workspace2D_sptr);

      /// The name and path of the input file
      std::string m_filename;
      /// The instrument name from Nexus
      std::string m_instrument_name;
      /// The sample name read from Nexus
      std::string m_samplename;

      /// The number of the input entry
      int64_t m_entrynumber;
     
      /// The number of spectra in the raw file
      int64_t m_numberOfSpectra;
      /// The number of periods in the raw file
      int64_t m_numberOfPeriods;
      /// Has the spectrum_list property been set?
      bool m_list;
      /// Have the spectrum_min/max properties been set?
      bool m_interval;
      /// The value of the spectrum_list property
      std::vector<specid_t> m_spec_list;
      /// The value of the spectrum_min property
      int64_t m_spec_min;
      /// The value of the spectrum_max property
      int64_t m_spec_max;
      /// The group which each detector belongs to in order
      std::vector<specid_t> m_groupings;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      
      void loadData(const MantidVecPtr::ptr_type& tcbs,size_t hist, specid_t& i,
          MuonNexusReader& nxload, const int64_t lengthIn, DataObjects::Workspace2D_sptr localWorkspace);
      void runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr);
      void runLoadMappingTable(DataObjects::Workspace2D_sptr);
      void runLoadLog(DataObjects::Workspace2D_sptr);
      void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS_H_*/
