#ifndef MANTID_DATAHANDLING_LOADPARAMETERFILE_H_
#define MANTID_DATAHANDLING_LOADPARAMETERFILE_H_

#include "MantidAPI/DistributedAlgorithm.h"

/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco
/// @endcond

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class CompAssembly;
class Component;
class CSGObject;
class ObjComponent;
class Instrument;
} // namespace Geometry

namespace DataHandling {
/** @class LoadParameterFile LoadParameterFile.h
DataHandling/LoadParameterFile.h

Loads instrument parameter data from a XML instrument parameter file and adds it
to a workspace.

LoadParameterFile is an algorithm and as such inherits
from the Algorithm class and overrides the init() & exec()  methods.

Required Properties:
<UL>
<LI> Workspace - The name of the workspace </LI>
<LI> Filename - The name of the parameter file </LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 19/4/2010

Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadParameterFile : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadParameterFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads instrument parameters into a workspace. where these "
           "parameters are associated component names as defined in Instrument "
           "Definition File (IDF) or a string consisting of the contents of "
           "such.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveParameterFile"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

private:
  void init() override;
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPARAMETERFILE_H_*/
