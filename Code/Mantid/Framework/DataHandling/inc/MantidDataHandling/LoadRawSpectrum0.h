#ifndef MANTID_DATAHANDLING_LOADRAWSPECTRUM0_H
#define MANTID_DATAHANDLING_LOADRAWSPECTRUM0_H


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid
{
  namespace DataHandling
  {
    /**

    Loads zeroth spectrum from ISIS RAW format file and stores it in a 2D workspace
    (Workspace2D class). LoadRawSpectrum0 is an algorithm and  inherits
    from the LoadRawHelper class .
   
    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    
    @author Sofia Antony,ISIS,RAL
    @date 12/04/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LoadRawSpectrum0 : public LoadRawHelper
    {
    public:
      /// Default constructor
      LoadRawSpectrum0();
      /// Destructor
      ~LoadRawSpectrum0();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRawSpectrum0"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads spectrum zero  from  ISIS  raw file and stores it in a 2D workspace (Workspace2D class).";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics"; }

    private:
      
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
	  	  
      /// ISISRAW class instance which does raw file reading. Shared pointer to prevent memory leak when an exception is thrown.
      boost::shared_ptr<ISISRAW2> isisRaw;
      /// The name and path of the input file
      std::string m_filename;

      /// The number of spectra in the raw file
      specid_t m_numberOfSpectra;

	   /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;
      /// A map for storing the time regime for each spectrum
      std::map<int64_t,int64_t> m_specTimeRegimes;
      /// The current value of the progress counter
      double m_prog;
      /// Read in the time bin boundaries
      int64_t m_lengthIn;
      /// number of time regime
      int64_t m_noTimeRegimes;
     
      /// TimeSeriesProperty<int> containing data periods.
      boost::shared_ptr<Kernel::Property> m_perioids;
	};
  }
}
#endif
