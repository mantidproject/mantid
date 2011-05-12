#ifndef MANTID_DATAHANDLING_LoadRaw3_H_
#define MANTID_DATAHANDLING_LoadRaw3_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidAPI/IDataFileChecker.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadRaw3 LoadRaw3.h DataHandling/LoadRaw3.h

    Loads an file in ISIS RAW format and stores it in a 2D workspace
    (Workspace2D class). LoadRaw is an algorithm and LoadRawHelper class and
	overrides the init() & exec() methods.
    LoadRaw3 uses less memory by only loading up the datablocks as required.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007

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
    class DLLExport LoadRaw3 : public LoadRawHelper
    {
	
    public:
      /// Default constructor
      LoadRaw3();
      /// Destructor
      ~LoadRaw3();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRaw"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 3; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }
      
     /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);
      

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
	        
      /// returns true if the given spectrum is a monitor
      bool isMonitor(const std::vector<int64_t>& monitorIndexes,int64_t spectrumNum);
      /// returns true if the Exclude Monitor option(property) selected
      bool isExcludeMonitors();
      ///  returns true if the Separate Monitor Option  selected
      bool isSeparateMonitors();
      ///  returns true if the Include Monitor Option  selected
      bool isIncludeMonitors();

      /// validate workspace sizes
      void validateWorkspaceSizes( bool bexcludeMonitors ,bool bseparateMonitors,
                                   const int64_t normalwsSpecs,const int64_t  monitorwsSpecs);

      /// this method will be executed if not enough memory.
      void goManagedRaw(bool bincludeMonitors,bool bexcludeMonitors,
                        bool bseparateMonitors,const std::string& fileName);
      

      /// This method is useful for separating  or excluding   monitors from the output workspace
      void  separateOrexcludeMonitors(DataObjects::Workspace2D_sptr localWorkspace,
                                      bool binclude,bool bexclude,bool bseparate,
                                      int64_t numberOfSpectra,const std::string &fileName);

      /// creates output workspace, monitors excluded from this workspace
      void excludeMonitors(FILE* file,const int64_t& period,const std::vector<int64_t>& monitorList,
                           DataObjects::Workspace2D_sptr ws_sptr);

      /// creates output workspace whcih includes monitors
      void includeMonitors(FILE* file,const int64_t& period,DataObjects::Workspace2D_sptr ws_sptr);

      /// creates two output workspaces none normal workspace and separate one for monitors
      void separateMonitors(FILE* file,const int64_t& period,const std::vector<int64_t>& monitorList,
                            DataObjects::Workspace2D_sptr ws_sptr,DataObjects::Workspace2D_sptr mws_sptr);

      ///sets optional properties
      void setOptionalProperties();
      
      /// The name and path of the input file
      std::string m_filename;

      /// The number of spectra in the raw file
      int64_t m_numberOfSpectra;
      /// The number of periods in the raw file
      int64_t m_numberOfPeriods;
      /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;
      /// A map for storing the time regime for each spectrum
      std::map<int64_t,int64_t> m_specTimeRegimes;
      /// number of time regime
      int64_t m_noTimeRegimes;
      /// The current value of the progress counter
      double m_prog;

      /// a vector holding the indexes of monitors
      std::vector<int64_t> m_monitordetectorList;
      /// Read in the time bin boundaries
      int64_t m_lengthIn;
      /// boolean for list spectra options
      bool m_bmspeclist;
      /// time channels vector
      std::vector<boost::shared_ptr<MantidVec> > m_timeChannelsVec;
      /// total number of specs
      int64_t m_total_specs;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadRaw3_H_*/
