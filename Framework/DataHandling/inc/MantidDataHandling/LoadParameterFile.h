// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  const std::vector<std::string> seeAlso() const override { return {"SaveParameterFile"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  void init() override;
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
