#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IDataFileChecker.h"
#include <napi.h>
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MuonNexusReader;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

    Loads an file in NeXus Muon format and stores it in a 2D workspace
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
      int m_entrynumber;
     
      /// The number of spectra in the raw file
      int m_numberOfSpectra;
      /// The number of periods in the raw file
      int m_numberOfPeriods;
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
      /// The group which each detector belongs to in order
      std::vector<int> m_groupings;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      
      void loadData(const MantidVecPtr::ptr_type&,int, int&, MuonNexusReader& , const int, DataObjects::Workspace2D_sptr );
      void runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr);
      void runLoadMappingTable(DataObjects::Workspace2D_sptr);
      void runLoadLog(DataObjects::Workspace2D_sptr);
      void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS_H_*/
