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
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/LoadMuonNexus.h"

#include <map>

namespace Mantid {

namespace Algorithms {

using ConfFuncPtr = int (*)(const std::string &, const std::shared_ptr<API::Algorithm> &);

/**
Loads an file in NeXus Muon format version 1 and 2 and stores it in a 2D
workspace
(Workspace2D class). LoadMuonNexus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data
(a multiperiod file will store higher periods in workspaces called
OutputWorkspace_PeriodNo)
[ not yet implemented for NeXus ]</LI>
</UL>

Optional Properties: (note that these options are not available if reading a
multiperiod file)
<UL>
<LI> spectrum_min  - The spectrum to start loading from</LI>
<LI> spectrum_max  - The spectrum to load to</LI>
<LI> spectrum_list - An ArrayProperty of spectra to load</LI>
<LI> auto_group - Determines whether the spectra are automatically grouped
together based on the groupings in the NeXus file. </LI>
</UL>
*/
class MANTID_MUON_DLL LoadMuonNexus3 : public LoadMuonNexus {
public:
  LoadMuonNexus3();

  const std::string summary() const override {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 1 or 2 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  int version() const override { return 3; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus", "LoadMuonNexusV2"}; }

  /// Returns 0, as this version of the algorithm is never to be selected via load.
  int confidence(Kernel::NexusDescriptor &descriptor) const override { return 0; };

private:
  std::map<std::shared_ptr<API::Algorithm>, ConfFuncPtr> m_loadAlgs;

  void exec() override;
  void runSelectedAlg(const std::string &algName, const int version);
};

} // namespace Algorithms
} // namespace Mantid
