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
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/LoadMuonNexus.h"

// Forward declaration (here, because is not in Mantid namespace)
class MuonNexusReader;

namespace Mantid {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace NeXus {
class NXRoot;
}

namespace Algorithms {
/** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

Loads an file in Nexus Muon format version 1 and stores it in a 2D workspace
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
     [ not yet implemented for Nexus ]</LI>
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
class MANTID_MUON_DLL LoadMuonNexus1 : public LoadMuonNexus {
public:
  /// Default constructor
  LoadMuonNexus1();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMuonNexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 1 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus;Muon\\DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

protected:
  /// Overwrites Algorithm method
  void exec() override;

private:
  void loadData(size_t hist, specnum_t &i, specnum_t specNo, MuonNexusReader &nxload, const int64_t lengthIn,
                const DataObjects::Workspace2D_sptr &localWorkspace);
  void runLoadMappingTable(DataObjects::Workspace2D_sptr);
  void runLoadLog(const DataObjects::Workspace2D_sptr &);
  void loadRunDetails(const DataObjects::Workspace2D_sptr &localWorkspace);
  void addPeriodLog(const DataObjects::Workspace2D_sptr &localWorkspace, int64_t period);
  void addGoodFrames(const DataObjects::Workspace2D_sptr &localWorkspace, int64_t period, int nperiods);

  /// Loads dead time table for the detector
  void loadDeadTimes(Mantid::NeXus::NXRoot &root);

  /// Creates Dead Time Table using all the data between begin and end
  DataObjects::TableWorkspace_sptr createDeadTimeTable(std::vector<int> specToLoad, std::vector<double> deadTimes);

  /// Loads detector grouping information
  API::Workspace_sptr loadDetectorGrouping(Mantid::NeXus::NXRoot &root,
                                           const Mantid::Geometry::Instrument_const_sptr &inst);

  /// Creates Detector Grouping Table using all the data from the range
  DataObjects::TableWorkspace_sptr createDetectorGroupingTable(std::vector<int> specToLoad, std::vector<int> grouping);
};

} // namespace Algorithms
} // namespace Mantid
