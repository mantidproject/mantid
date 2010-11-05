#ifndef MANTID_DATAHANDLING_LoadISISNexus22_H_
#define MANTID_DATAHANDLING_LoadISISNexus22_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace Mantid
{
  namespace NeXus
  {

    /** 

    Loads a file in a Nexus format and stores it in a 2D workspace. LoadISISNexus2 is an algorithm and 
    as such inherits  from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input Nexus file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
    (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> SpectrumMin  - The  starting spectrum number</LI>
    <LI> SpectrumMax  - The  final spectrum number (inclusive)</LI>
    <LI> SpectrumList - An ArrayProperty of spectra to load</LI>
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
    class DLLExport LoadISISNexus2 : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadISISNexus2();
      /// Destructor
      virtual ~LoadISISNexus2() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadISISNexus"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 2; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      // Validate the optional input properties
      void checkOptionalProperties();
      /// Run LoadInstrument as a subalgorithm
      void runLoadInstrument(DataObjects::Workspace2D_sptr);
      /// Load in details about the run
      void loadRunDetails(DataObjects::Workspace2D_sptr local_workspace, NXEntry & entry);
      /// Parse an ISO formatted date-time string into separate date and time strings
      void parseISODateTime(const std::string & datetime_iso, std::string & date, std::string & time) const;
      /// Load in details about the sample
      void loadSampleData(DataObjects::Workspace2D_sptr, NXEntry & entry);
      /// Load log data from the nexus file
      void loadLogs(DataObjects::Workspace2D_sptr, NXEntry & entry,int period = 1);
      // Load a given period into the workspace
      void loadPeriodData(int period, NXEntry & entry, DataObjects::Workspace2D_sptr local_workspace);
      // Load a data block
      void loadBlock(NXDataSetTyped<int> & data, int blocksize, int period, int start, 
        int &hist, int& spec_num, DataObjects::Workspace2D_sptr localWorkspace);


      /// The name and path of the input file
      std::string m_filename;
      /// The instrument name from Nexus
      std::string m_instrument_name;
      /// The sample name read from Nexus
      std::string m_samplename;

      /// The number of spectra in the raw file
      int m_numberOfSpectra, m_numberOfSpectraInFile;
      /// The number of periods in the raw file
      int m_numberOfPeriods, m_numberOfPeriodsInFile;
      /// The nuber of time chanels per spectrum
      int m_numberOfChannels, m_numberOfChannelsInFile;
      /// Is there a detector block
      bool m_have_detector;

      /// The value of the SpectrumMin property
      int m_spec_min;
      /// The value of the SpectrumMax property
      int m_spec_max;
      /// The value of the spectrum_list property
      std::vector<int> m_spec_list;
      /// The number of the input entry
      int m_entrynumber;

      /// Have the spectrum_min/max properties been set?
      bool m_range_supplied;
      /// Time channels
      boost::shared_ptr<MantidVec> m_tof_data;
      /// Proton charge
      double m_proton_charge;
      /// Title of workspace is read by loadPeriodData()
      std::string m_wTitle;
      /// Spectra numbers
      boost::shared_array<int> m_spec;
      /// Pointer to one-past-the-end of spectrum number array (m_spec)
      const int * m_spec_end;
      /// Monitors
      std::map<int,std::string> m_monitors;

      ///Progress reporting object
      boost::shared_ptr<API::Progress> m_progress;

      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);
    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadISISNexus2_H_*/
