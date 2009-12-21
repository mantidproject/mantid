#ifndef MANTID_DATAHANDLING_LoadRaw3_H_
#define MANTID_DATAHANDLING_LoadRaw3_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
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
    (Workspace2D class). LoadRaw is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec() methods.
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
    class DLLExport LoadRaw3 : public API::Algorithm//, public ISISRAW2
    {
    public:
      /// Default constructor
      LoadRaw3();
      /// Destructor
      ~LoadRaw3();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRaw"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 3; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      /// Check if this actually is a binary file
	bool isAscii(const std::string& filename) const;
      void checkOptionalProperties();
      int calculateWorkspaceSize();
      void goManagedRaw(bool bincludeMonitors,bool bexcludeMonitors,bool bseparateMonitors );
      std::vector<boost::shared_ptr<MantidVec> > getTimeChannels(const int& regimes, 
                                                                 const int& lengthIn);
      void runLoadInstrument(DataObjects::Workspace2D_sptr);
      void runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr);
      void runLoadMappingTable(DataObjects::Workspace2D_sptr);
      void runLoadLog(DataObjects::Workspace2D_sptr,int period=1);
      void populateInstrumentParameters(DataObjects::Workspace2D_sptr);
      
	  ///getting the monitor spectrum indexes 
	  void getmonitorSpectrumList(DataObjects::Workspace2D_sptr localWorkspace,std::vector<int>& monitorSpecList);
	  
	  /// /// returns true if the given spectrum is a monitor
	  bool isMonitor(const std::vector<int>& monitorIndexes,int spectrumNum);
	  /// returns true if the Exclude Monitor option(property) selected
	  bool isExcludeMonitors();
	  ///  returns true if the Separate Monitor Option  selected
	  bool isSeparateMonitors();
	   ///  returns true if the Include Monitor Option  selected
	  bool isIncludeMonitors();

	  /// creates  shared pointer to group workspace 
	  API::WorkspaceGroup_sptr createGroupWorkspace();
	  /// craetes hared pointer to workspace
	  DataObjects::Workspace2D_sptr createWorkspace(int nVectors,int lengthIn);

	 /// sets the workspace property 
	 void setWorkspaceProperty(const std::string & propertyName,const std::string& title,
		 API::WorkspaceGroup_sptr grpWS,DataObjects::Workspace2D_sptr workspace,bool bMonitor);

	  /// sets the workspace property 
	  void setWorkspaceProperty(DataObjects::Workspace2D_sptr wsPtr,API::WorkspaceGroup_sptr wsGrpSptr,const int period,bool bmonitors);

	  /// calculates the workspace size for normal output workspace and monitor workspace
	  void calculateWorkspacesizes(const std::vector<int>& monitorSpecList,const int total_specs,int& normalwsSpecs,int & monitorwsSpecs);

	  /// This method sets the raw file data to workspace vectors
	 void setWorkspaceData(DataObjects::Workspace2D_sptr newWorkspace,
								const std::vector<boost::shared_ptr<MantidVec> >& timeChannelsVec,int wsIndex,int nspecNum,int noTimeRegimes);

	 /// This method is useful for separating  or excluding   monitors from the output workspace
	 void  separateOrexcludeMonitors(DataObjects::Workspace2D_sptr localWorkspace,bool bincludeMonitors,bool bexcludeMonitors,bool bseparateMonitors);

	 /// creates time series property showing times when when a particular period was active.
	 Kernel::Property* createPeriodLog(int period)const;

      /// ISISRAW class instance which does raw file reading. Shared pointer to prevent memory leak when an exception is thrown.
      boost::shared_ptr<ISISRAW2> isisRaw;
      /// The name and path of the input file
      std::string m_filename;

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
      /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;
      /// A map for storing the time regime for each spectrum
      std::map<int,int> m_specTimeRegimes;
      /// The current value of the progress counter
      double m_prog;

	    /// A flag int value to indicate that the value wasn't set by users
      static const int unSetInt = INT_MAX-15;
	  /// a vector holding the indexes of monitors
	  std::vector<int> m_monitordetectorList;
	  ///a vector holding allowed values for Monitor selection property
	  std::vector<std::string> m_monitorOptions;
	 // Read in the time bin boundaries
	 int m_lengthIn;
	 /// boolean for list spectra options
	 bool m_bmspeclist;
	 /// TimeSeriesProperty<int> containing data periods.
	 boost::shared_ptr<Kernel::Property> m_perioids;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadRaw3_H_*/
