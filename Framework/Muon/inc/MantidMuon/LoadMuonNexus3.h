// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/LoadMuonNexus.h"

#include <vector>

namespace Mantid::Algorithms {

using ConfFuncPtr = int (*)(const std::string &, const std::shared_ptr<Mantid::API::Algorithm> &);

struct AlgDetail {
  AlgDetail(const std::string &name, const int version, const ConfFuncPtr &loader,
            const Mantid::API::Algorithm_sptr &alg)
      : m_name(name), m_version(version), m_confFunc(loader), m_alg(alg) {}

  const std::string m_name;
  const int m_version;
  const ConfFuncPtr m_confFunc;
  const Mantid::API::Algorithm_sptr m_alg;
};

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

  // Returns 0, as this wrapper version of the algorithm is never to be selected via load.
  int confidence(Kernel::LegacyNexusDescriptor &) const override { return 0; };
  // Methods to enable testing.
  const std::string &getSelectedAlg() const { return m_loadAlgs[m_selectedIndex].m_name; }
  int getSelectedVersion() const { return m_loadAlgs[m_selectedIndex].m_version; }

private:
  std::vector<AlgDetail> m_loadAlgs;
  size_t m_selectedIndex;

  void exec() override;
  void runSelectedAlg();
  void addAlgToVec(const std::string &name, const int version, const ConfFuncPtr &loader);
};

} // namespace Mantid::Algorithms
