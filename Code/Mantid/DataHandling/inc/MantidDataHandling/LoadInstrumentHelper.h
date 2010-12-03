#ifndef MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
  class Element;
}}
/// @endcond

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadInstrumentHelper LoadInstrumentHelper.h DataHandling/LoadInstrumentHelper.h

    Contains method for assisting the loading of an instrument definition file (IDF)

    @author Anders Markvardsen, ISIS, RAL
    @date 29/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    */
    class DLLExport LoadInstrumentHelper 
    {
    public:
      /// Default constructor
      LoadInstrumentHelper() {};

      /// Destructor
      virtual ~LoadInstrumentHelper() {}

      /// Given an instrument name and a date return filename of appropriate IDF
      std::string getIDF_Filename(std::string& IDFname, std::string date) {return std::string();}
      

    private:

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_*/

