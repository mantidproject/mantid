#ifndef MANTID_PYTHONAPI_SIMPLEPYTHONAPI
#define MANTID_PYTHONAPI_SIMPLEPYTHONAPI

//------------------------------------------
// Includes
//------------------------------------------
#include <set>
#include "MantidKernel/System.h"
#include "MantidKernel/Property.h"

namespace Mantid
{

namespace PythonAPI
{

/** @class SimplePythonAPI SimplePythonAPI.h PythonAPI/SimplePythonAPI.h

    SimplePythonAPI is a static class designed to write out a Python module containing
    function definitions for all loaded algorithms. A particular function, when called, will
    create an instance of the named Algorithm. 

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/10/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class DLLExport SimplePythonAPI
{
  public:
  
  /// Typedef a vector of strings
  typedef std::vector<std::string> StringVector;
  typedef std::set<std::string> StringSet;
  typedef std::vector<Mantid::Kernel::Property*> PropertyVector;

  ///Public methods
  static void createModule();
  static const std::string & getModuleName(); 
  
  private:
  ///private constructor
  SimplePythonAPI();
  
  ///Private methods
  static StringSet getAlgorithmNames();
  static std::string extractAlgName(const std::string &);
  static void writeFunctionDef(std::ostream &, std::string, const PropertyVector &);
  static void writeGlobalHelp(std::ostream &, const StringSet &);
  
  /// The name of the module file
  static std::string m_strFilename;   
};

}

}

#endif //MANTID_PYTHONAPI_SIMPLEPYTHONAPI
