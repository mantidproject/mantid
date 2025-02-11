// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include <climits>

namespace Mantid {
namespace DataHandling {
/**
Saves a file in NeXus format and from a 2D workspace
(Workspace2D class). SaveNeXus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the output NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace from which to write the
exported data </LI>
</UL>

@author Freddie Akeroyd, STFC ISIS Facility, GB
@date 24/01/2008
*/
class MANTID_DATAHANDLING_DLL SaveNexus : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveNexus"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SaveNexus algorithm will write the given Mantid workspace to a "
           "NeXus file. SaveNexus currently just invokes SaveNexusProcessed.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveNexusPD", "SaveNexusProcessed", "LoadNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of the input file
  std::string m_filename;
  /// The name and path of the input file
  std::string m_entryname;
  /// The prefix to be used to name spectra (_n added for nth spectra)
  std::string m_dataname;
  /// The file type to save, currently only one type possible
  std::string m_filetype;
  /// Pointer to the local workspace
  API::Workspace_sptr m_inputWorkspace;
  /// Method to execute SNP Child Algorithm
  void runSaveNexusProcessed();
  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
                          int perioidNum) override;

protected:
  /// Override process groups
  bool processGroups() override;
};

} // namespace DataHandling
} // namespace Mantid
