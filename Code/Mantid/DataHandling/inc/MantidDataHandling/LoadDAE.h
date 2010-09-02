#ifndef MANTID_DATAHANDLING_LOADDAE_H_
#define MANTID_DATAHANDLING_LOADDAE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>
#include "MantidAPI/WorkspaceGroup.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

struct idc_info;
typedef struct idc_info* idc_handle_t;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadDAE LoadDAE.h DataHandling/LoadDAE.h

    Loads data from the ISIS DATA acquisition system and stores it in a 2D workspace 
    (Workspace2D class).

    @todo Doesn't currently support multiple time regimes

    Required Properties:
    <UL>
    <LI> DAEname - The name of and path to the input DAE host </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    </UL>

    @author Freddie Akeroyd, STFC ISIS Facility
    @date 30/07/08

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
    class DLLExport LoadDAE : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadDAE();
      /// Destructor
      ~LoadDAE() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadDAE"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }
      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method. 
      void exec();
      /// Does actual loading
      void loadDAE();

      /// Validates the optional 'spectra to read' properties, if they have been set
      void checkOptionalProperties();
      ///Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw).
      void runLoadInstrument(DataObjects::Workspace2D_sptr, const char* iName);

      /// Populate spectra-detector map
      void loadSpectraMap(idc_handle_t dae_handle, DataObjects::Workspace2D_sptr ws);

      /// load data from the DAE
      void loadData(const MantidVecPtr::ptr_type& tcbs,int hist, int& ispec, idc_handle_t dae_handle, const int& lengthIn,
    		int* spectrum, DataObjects::Workspace2D_sptr localWorkspace, int* allData = 0);

      /// The host name of the DAE
      std::string m_daename;
     
      /// The number of spectra in the DAE
      int m_numberOfSpectra;
      /// The number of periods in the DAE
      int m_numberOfPeriods;
      /// The number of chanels per spectrum
      int m_channelsPerSpectrum;
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
      /// Good proton charge
      float m_proton_charge;
      /// has true during the first run of loadDAE and false all subsequent runs
      bool m_firstRun;
      
      ///a flag int value to indicate that the value wasn't set by users
      static const int unSetInt = INT_MAX-15;
      ///static reference to the logger class
      static Kernel::Logger& g_StaticLog;
      
      /// reporter function called when the IDC reading routines raise an error
      static void IDCReporter(int status, int code, const char* message);

    };

  } // namespace DataHandling
} // namespace Mantid

#endif
