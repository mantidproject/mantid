#ifndef MANTID_DATAHANDLING_LoadISISNexus22_H_
#define MANTID_DATAHANDLING_LoadISISNexus22_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include <nexus/NeXusFile.hpp>

#include <boost/scoped_ptr.hpp>

#include <climits>



//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace Mantid
{
  namespace DataHandling
  {

    /** 

    Loads a file in a NeXus format and stores it in a 2D workspace. LoadISISNexus2 is an algorithm and
    as such inherits  from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NeXus file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
    (a multi-period file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multi-period file)
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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadISISNexus2 : public API::IFileLoader<Kernel::NexusDescriptor> 
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
      virtual const std::string category() const { return "DataHandling\\Nexus"; }
      ///Summary of algorithms purpose
      virtual const std::string summary() const {return "Loads a file in ISIS NeXus format.";}

      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::NexusDescriptor & descriptor) const;

      /// Spectra block descriptor
      struct SpectraBlock
      {
        /// Constructor - initialize the block
        SpectraBlock(int64_t f,int64_t l,bool m):first(f),last(l),isMonitor(m){}
        int64_t first; ///< first spectrum number of the block
        int64_t last; ///< last spectrum number of the block
        bool isMonitor; ///< is the data in a monitor group
      };
    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      // Validate the optional input properties
      void checkOptionalProperties();
      /// Prepare a vector of SpectraBlock structures to simplify loading
      size_t prepareSpectraBlocks();
      /// Run LoadInstrument as a ChildAlgorithm
      void runLoadInstrument(DataObjects::Workspace2D_sptr);
      /// Load in details about the run
      void loadRunDetails(DataObjects::Workspace2D_sptr local_workspace, Mantid::NeXus::NXEntry & entry);
      /// Parse an ISO formatted date-time string into separate date and time strings
      void parseISODateTime(const std::string & datetime_iso, std::string & date, std::string & time) const;
      /// Load in details about the sample
      void loadSampleData(DataObjects::Workspace2D_sptr, Mantid::NeXus::NXEntry & entry);
      /// Load log data from the nexus file
      void loadLogs(DataObjects::Workspace2D_sptr ws, Mantid::NeXus::NXEntry & entry);
      // Load a given period into the workspace
      void loadPeriodData(int64_t period, Mantid::NeXus::NXEntry & entry, DataObjects::Workspace2D_sptr local_workspace);
      // Load a data block
      void loadBlock(Mantid::NeXus::NXDataSetTyped<int> & data, int64_t blocksize, int64_t period, int64_t start,
          int64_t &hist, int64_t& spec_num, DataObjects::Workspace2D_sptr localWorkspace);

      // Create period logs
      void createPeriodLogs(int64_t period, DataObjects::Workspace2D_sptr local_workspace);
      // Validate multi-period logs
      void validateMultiPeriodLogs(Mantid::API::MatrixWorkspace_sptr);
      // build the list of spectra numbers to load and include in the spectra list
      void buildSpectraInd2SpectraNumMap(const std::vector<int64_t>  &spec_list);


      /// The name and path of the input file
      std::string m_filename;
      /// The instrument name from Nexus
      std::string m_instrument_name;
      /// The sample name read from Nexus
      std::string m_samplename;

      /// The number of spectra
      std::size_t m_numberOfSpectra;
      /// The number of spectra in the raw file
      std::size_t m_numberOfSpectraInFile;
      /// The number of periods
      int m_numberOfPeriods;
      /// The number of periods in the raw file
      int m_numberOfPeriodsInFile;
      /// The number of time channels per spectrum
      std::size_t m_numberOfChannels;
      /// The number of time channels per spectrum in the raw file
      std::size_t m_numberOfChannelsInFile;
      /// Is there a detector block
      bool m_have_detector;


      /// Have the spectrum_min/max properties been set?
      bool m_range_supplied;
       /// The value of the SpectrumMin property
      int64_t m_spec_min;
      /// The value of the SpectrumMax property
      int64_t m_spec_max;
      /// if true, a spectra list or range of spectra is supplied
      bool m_load_selected_spectra;
      /// map of spectra Index to spectra Number (spectraID)
      std::map<int64_t,specid_t> m_specInd2specNum_map;
      /// spectra Number to detector ID (multi)map
      API::SpectrumDetectorMapping m_spec2det_map; 


      /// The number of the input entry
      int64_t m_entrynumber;
      /// List of disjoint data blocks to load
      std::vector<SpectraBlock> m_spectraBlocks;

      /// Time channels
      boost::shared_ptr<MantidVec> m_tof_data;
      /// Proton charge
      double m_proton_charge;
      /// Spectra numbers
      boost::shared_array<int> m_spec;
      /// Pointer to one-past-the-end of spectrum number array (m_spec)
      const int * m_spec_end;
      /// Monitors, map spectrum index to monitor group name
      std::map<int64_t,std::string> m_monitors;

      /// A pointer to the ISISRunLogs creator
      boost::scoped_ptr<ISISRunLogs> m_logCreator;

      ///Progress reporting object
      boost::shared_ptr<API::Progress> m_progress;

      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);

      // C++ interface to the NXS file
      ::NeXus::File * m_cppFile;

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadISISNexus2_H_*/
