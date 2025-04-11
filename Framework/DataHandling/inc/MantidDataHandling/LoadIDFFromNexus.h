// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {
/** @class LoadIDFFromNexus LoadIDFFromNexus.h
DataHandling/LoadIDFFromNexus.h

Load an IDF from a Nexus file, if found there.

LoadIDFFromNexus is intended to be used as a child algorithm of
other Loadxxx algorithms, rather than being used directly.
A such it enables the loadxxx algorithm to take the instrument definition from
an IDF located within
a Nexus file if it is available.
LoadIDFFromNexus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NEXUS file </LI>
<LI> Workspace - The name of the workspace in which to use as a basis for any
data to be added.</LI>
</UL>
*/
class MANTID_DATAHANDLING_DLL LoadIDFFromNexus final : public API::Algorithm {
public:
  /// Default constructor
  LoadIDFFromNexus();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadIDFFromNexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load an IDF from a Nexus file, if found there. You may need to "
           "tell this algorithm where to find the Instrument folder in the "
           "Nexus file";
  }

  /// Get the parameter correction file, if it exists else return ""
  std::string getParameterCorrectionFile(const std::string &instName);

  /// Read parameter correction file, return applicabel parameter file and
  /// whether to append
  void readParameterCorrectionFile(const std::string &correction_file, const std::string &date,
                                   std::string &parameter_file, bool &append);

  /// Load the parameters from Nexus file if possible, else from parameter file,
  /// into workspace
  void LoadParameters(::NeXus::File *nxfile, const API::MatrixWorkspace_sptr &localWorkspace);

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// Load Parameter File specified by full pathname into given workspace,
  /// return success
  bool loadParameterFile(const std::string &fullPathName, const API::MatrixWorkspace_sptr &localWorkspace);
};

} // namespace DataHandling
} // namespace Mantid
