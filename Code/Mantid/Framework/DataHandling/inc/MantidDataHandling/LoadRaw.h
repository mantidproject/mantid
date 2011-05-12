#ifndef MANTID_DATAHANDLING_LOADRAW_H_
#define MANTID_DATAHANDLING_LOADRAW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadRaw LoadRaw.h DataHandling/LoadRaw.h

    Loads an file in ISIS RAW format and stores it in a 2D workspace
    (Workspace2D class). LoadRaw is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec() methods.

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

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadRaw : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadRaw();
      /// Destructor
      ~LoadRaw() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRaw"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      bool isAscii(const std::string & filename) const;
      void checkOptionalProperties();
      void loadData(const MantidVecPtr::ptr_type&,int64_t, int64_t&, ISISRAW& , const int& , int*, DataObjects::Workspace2D_sptr );
      void runLoadInstrument(DataObjects::Workspace2D_sptr);
      void runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr);
      void runLoadMappingTable(DataObjects::Workspace2D_sptr);
      void runLoadLog(DataObjects::Workspace2D_sptr);
	     /// creates time series property showing times when when a particular period was active.
	  Kernel::Property* createPeriodLog(int period)const;

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
      std::vector<int64_t> m_spec_list;
      /// The value of the spectrum_min property
      int64_t m_spec_min;
      /// The value of the spectrum_max property
      int64_t m_spec_max;
      
      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);

	 /// TimeSeriesProperty<int> containing data periods.
	 boost::shared_ptr<Kernel::Property> m_perioids;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADRAW_H_*/
