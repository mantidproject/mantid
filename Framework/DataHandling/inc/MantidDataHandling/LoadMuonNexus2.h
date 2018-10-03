// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS2_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {

namespace DataHandling {

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
class DLLExport LoadMuonNexus2 : public LoadMuonNexus {
public:
  /// Default constructor
  LoadMuonNexus2();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMuonNexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 2 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Nexus;Muon\\DataHandling";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Overwrites Algorithm method
  void exec() override;
  /// Execute this version of the algorithm
  void doExec();

  HistogramData::Histogram
  loadData(const Mantid::HistogramData::BinEdges &edges,
           const Mantid::NeXus::NXInt &counts, int period, int spec);
  void loadLogs(API::MatrixWorkspace_sptr ws, Mantid::NeXus::NXEntry &entry,
                int period);
  void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);
  std::map<int, std::set<int>>
  loadDetectorMapping(const Mantid::NeXus::NXInt &spectrumIndex);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS2_H_*/
