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
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidLegacyNexus/NexusClasses.h"
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/LoadMuonNexus.h"

namespace Mantid {

namespace Algorithms {

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
class MANTID_MUON_DLL LoadMuonNexus2 : public LoadMuonNexus {
public:
  LoadMuonNexus2();

  const std::string summary() const override {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 2 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::LegacyNexusDescriptor &descriptor) const override;

private:
  void exec() override;

  HistogramData::Histogram loadData(const Mantid::HistogramData::BinEdges &edges,
                                    const Mantid::LegacyNexus::NXInt &counts, int period, int spec);
  void loadLogs(const API::MatrixWorkspace_sptr &ws, const Mantid::LegacyNexus::NXEntry &entry, int period);
  void loadRunDetails(const DataObjects::Workspace2D_sptr &localWorkspace);
  std::map<int, std::set<int>> loadDetectorMapping(const Mantid::LegacyNexus::NXInt &spectrumIndex);
};

} // namespace Algorithms
} // namespace Mantid
