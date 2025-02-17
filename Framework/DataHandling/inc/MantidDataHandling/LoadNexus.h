// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/Property.h"
#include <climits>

namespace Mantid {
namespace DataHandling {
/** @class LoadNexus

Loads a file in NeXus format and stores it in a 2D workspace
(Workspace2D class). LoadNexus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data </LI>
    </UL>
    Optional Properties:
    <UL>
    <LI> spectrum_list - integer list of spectra numbers to load</LI>
    <LI> spectrum_min, spectrum_max - range of spectra to load</LI>
</UL>

@author Ronald Fowler, based on version by Freddie Akeroyd
@date 29/08/2008
*/
class MANTID_DATAHANDLING_DLL LoadNexus final : public API::Algorithm {
public:
  /// Default constructor
  LoadNexus();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexus"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The LoadNexus algorithm will try to identify the type of Nexus "
           "file given to it and invoke the appropriate algorithm to read the "
           "data and populate the named workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMcStasNexus",    "LoadNexusMonitors", "LoadNexusProcessed", "LoadTOFRawNexus",
            "LoadILLDiffraction", "LoadILLTOF",        "LoadILLIndirect",    "LoadILLReflectometry",
            "LoadILLSANS",        "LoadMuonNexus",     "LoadFlexiNexus"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  static const std::string muonTD;
  static const std::string pulsedTD;

  static int getNexusEntryTypes(const std::string &fileName, std::vector<std::string> &entryName,
                                std::vector<std::string> &definition);

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of the input file
  std::string m_filename;

  /// The name of the output workspace
  std::string m_workspace;

  /// run LoadMuonNexus
  void runLoadMuonNexus();

  /// run LoadIsisNexus
  void runLoadIsisNexus();

  /// run LoadNexusProcessed
  void runLoadNexusProcessed();

  /// run LoadTOFRawNexus
  void runLoadTOFRawNexus();

  /// set the output workspaces from the child algorithms
  void setOutputWorkspace(const API::IAlgorithm_sptr &loader);
};
} // namespace DataHandling
} // namespace Mantid
