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
  /// Typedef a vector of Property pointers
  typedef std::vector<Mantid::Kernel::Property*> PropertyVector;
  /// Typedef a map of a string to a integer for versioning
  typedef std::map<std::string, int> VersionMap;
  /// Typedef a vector of a pair of strings for help commands
  typedef std::vector<std::pair<std::string, std::string> > IndexVector;

  ///Public methods
  static void createModule(bool gui);
  static std::string getModuleName(); 
  
  private:
  ///private constructor
  SimplePythonAPI();
  
  // Private methods
  /// Creates a map between algorithms and versions 
  static void createVersionMap(VersionMap & versionMap, const StringVector & algorithmKeys);
  /// Gets the algorithm name from a fully qualified name
  static std::string extractAlgName(const std::string & algKey);
  /// Writes the Python function definition for the given algorithm
  static void writeFunctionDef(std::ostream & output, const std::string & algName, const PropertyVector & properties, bool gui);
  /// Writes the global help command
  static void writeGlobalHelp(std::ostream & output, const VersionMap & versionMap);
  /// Creates a help string for the given algorithm
  static std::string createHelpString(const std::string & algm, const PropertyVector & properties, bool dialog);
  /// Writes the given help strings to the Python module
  static void writeFunctionHelp(std::ostream & output, const IndexVector & helpStrings);
  /// Convert EOL characters to their string representation
  static std::string convertEOLToString(const std::string & value);
  /// Removes all non-alphanumeric characters (those not [0-9, a-z, A-Z])
  static std::string removeCharacters(const std::string & value, const std::string & cs = "", bool eol_to_space = false);
  /// Converts windows style paths to forward-slash paths
  static std::string convertPathToUnix(const std::string & path);

  ///Functor for use with std::sort to put the properties that do not
  ///have valid values first
  struct PropertyOrdering
  {
    ///Comparator operator for sort algorithm, places optional properties lower in the list
    bool operator()(const Mantid::Kernel::Property * p1, 
		    const Mantid::Kernel::Property * p2) const
    {
  		//this is false, unless p1 is not valid and p2 is valid
	  	return ( p1->isValid() != "" ) && ( p2->isValid() == "" );
	  }
  };
  
  /// The name of the module file
  static std::string g_strFilename;
};

}

}

#endif //MANTID_PYTHONAPI_SIMPLEPYTHONAPI
