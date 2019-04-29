// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

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
namespace API {
class MatrixWorkspace;
}
namespace Geometry {
class CompAssembly;
class Component;
class CSGObject;
class ObjComponent;
class Instrument;
} // namespace Geometry

namespace DataHandling {
/** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

Loads instrument data from an XML or Nexus instrument description file and adds
it to a workspace.

LoadInstrument is an algorithm and as such inherits
from the Algorithm class and overrides the init() & exec()  methods.

Required Properties:
<UL>
<LI> Workspace - The name of the workspace </LI>
<LI> Filename - The name of the instrument file <b>OR</b> InstrumentName - The
    name of the instrument</LI>
</UL>

@author Nick Draper, Tessella Support Services plc
@date 19/11/2007
@author Anders Markvardsen, ISIS, RAL
@date 7/3/2008
*/
class DLLExport LoadInstrument : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadInstrument"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an Instrument Definition File (IDF) into a workspace. After "
           "the IDF has been read this algorithm will attempt to run the Child "
           "Algorithm LoadParameterFile; where if IDF filename is of the form "
           "IDENTIFIER_Definition.xml then the instrument parameters in the "
           "file named IDENTIFIER_Parameters.xml would be loaded.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadInstrumentFromNexus", "LoadInstrumentFromRaw",
            "ExportGeometry", "Load"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

private:
  void init() override;
  void exec() override;

  /// Run the Child Algorithm LoadParameters
  void runLoadParameterFile(const boost::shared_ptr<API::MatrixWorkspace> &ws,
                            std::string filename);

  /// Search directory for Parameter file, return full path name if found, else
  /// "".
  std::string getFullPathParamIDF(std::string directoryName,
                                  std::string filename);

  /// Mutex to avoid simultaneous access
  static std::recursive_mutex m_mutex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/
