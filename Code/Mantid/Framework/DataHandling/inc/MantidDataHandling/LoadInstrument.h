#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
class Element;
}
}
/// @endcond

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace API {
class MatrixWorkspace;
}
namespace Geometry {
class CompAssembly;
class Component;
class Object;
class ObjComponent;
class Instrument;
}

namespace DataHandling {
/** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

Loads instrument data from a XML instrument description file and adds it
to a workspace.

LoadInstrument is an algorithm and as such inherits
from the Algorithm class and overrides the init() & exec()  methods.

Required Properties:
<UL>
<LI> Workspace - The name of the workspace </LI>
<LI> Filename - The name of the IDF file </LI>
</UL>

@author Nick Draper, Tessella Support Services plc
@date 19/11/2007
@author Anders Markvardsen, ISIS, RAL
@date 7/3/2008

Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class DLLExport LoadInstrument : public API::Algorithm {
public:
  /// Default constructor
  LoadInstrument();

  /// Destructor
  virtual ~LoadInstrument() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadInstrument"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads an Instrument Definition File (IDF) into a workspace. After "
           "the IDF has been read this algorithm will attempt to run the Child "
           "Algorithm LoadParameterFile; where if IDF filename is of the form "
           "IDENTIFIER_Definition.xml then the instrument parameters in the "
           "file named IDENTIFIER_Parameters.xml would be loaded.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Instrument";
  }

private:
  void init();
  void exec();

  /// Run the Child Algorithm LoadParameters
  void runLoadParameterFile();

  /// Search directory for Parameter file, return full path name if found, else
  /// "".
  std::string getFullPathParamIDF(std::string directory);

  /// The name and path of the input file
  std::string m_filename;

  /// Everything can reference the workspace if it needs to
  boost::shared_ptr<API::MatrixWorkspace> m_workspace;

  /// Name of the instrument
  std::string m_instName;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/
