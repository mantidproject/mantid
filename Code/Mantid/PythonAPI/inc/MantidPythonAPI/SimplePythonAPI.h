#ifndef MANTID_PYTHONAPI_SIMPLEPYTHONAPI
#define MANTID_PYTHONAPI_SIMPLEPYTHONAPI

//------------------------------------------
// Includes
//------------------------------------------
#include <map>
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
  typedef std::vector<Mantid::Kernel::Property*> PropertyVector;
  typedef std::map<std::string, int> VersionMap;
  typedef std::vector<std::pair<std::string, std::string> > IndexVector;

  ///Public methods
  static void createModule();
  static const std::string & getModuleName(); 
  
  private:
  ///private constructor
  SimplePythonAPI();
  
  ///Private methods
  static void createVersionMap(VersionMap & versionMap, const StringVector & algorithmKeys);
  static std::string extractAlgName(const std::string & algKey);
  static std::string extractAlgVersion(const std::string & algKey);
  static void writeFunctionDef(std::ostream & output, const std::string & algName, const PropertyVector & properties);
  static void writeGlobalHelp(std::ostream & output, const VersionMap & versionMap);
  static std::string createHelpString(const std::string & algm, const PropertyVector & properties);
  static void writeFunctionHelp(std::ostream & output, const IndexVector & helpStrings);

  ///Functor for use with std::sort to put the properties that do not
  ///have valid values first
  struct PropertyOrdering
  {
    
    bool operator()(const Mantid::Kernel::Property * p1, 
		    const Mantid::Kernel::Property * p2) const
    {
      return p1->isValid() < p2->isValid();
    }
    
  };
  
  /// The name of the module file
  static std::string g_strFilename;
};

}

}

#endif //MANTID_PYTHONAPI_SIMPLEPYTHONAPI
