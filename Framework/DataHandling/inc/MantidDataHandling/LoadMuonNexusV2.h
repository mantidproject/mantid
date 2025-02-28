// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/LoadMuonNexusV2NexusHelper.h"
#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/** @class LoadMuonNexusV2 LoadMuonNexusV2.h DataHandling/LoadMuonNexusV2.h

Loads a file in the Nexus Muon format V2 and stores it in a 2D workspace
(Workspace2D class). LoadMuonNexus is an algorithm that loads
an HDF5 file and as such inherits from API::NexusFileLoader and
the init() & execLoader() methods.
Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data
 (a multiperiod file will store higher periods in workspaces called
OutputWorkspace_PeriodNo)
 [ not yet implemented for Muon Nexus V2 ]</LI>
<LI> spectrum_min  - The spectrum to start loading from</LI>
<LI> spectrum_max  - The spectrum to load to</LI>
<LI> spectrum_list - An ArrayProperty of spectra to load</LI>
</UL>

@author Stephen Smith, ISIS
*/

class MANTID_DATAHANDLING_DLL LoadMuonNexusV2 : public API::NexusFileLoader {
public:
  // Default constructor
  LoadMuonNexusV2();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMuonNexusV2"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 2 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by Load if it is "
           "given a NeXus file of this type.";
  }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  // Version
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void execLoader() override;
  // Determines whether entry contains multi period data
  void isEntryMultiPeriod();
  // Run child algorithm LoadISISNexus2
  API::Workspace_sptr runLoadISISNexus();
  // Load Muon specific properties
  void loadMuonProperties(size_t numSpectra);
  /// The name and path of the input file
  std::string m_filename;
  /// The number of the input entry
  int64_t m_entrynumber;
  // Is file multi period
  bool m_isFileMultiPeriod;
  // Are multi periods loaded
  bool m_multiPeriodsLoaded;
  // Choose loader strategy
  void chooseLoaderStrategy(const API::Workspace_sptr &workspace);
  // The loading strategy used
  std::unique_ptr<LoadMuonStrategy> m_loadMuonStrategy;
  // Nexus loading helper class
  std::unique_ptr<LoadMuonNexusV2NexusHelper> m_nexusLoader;
  // Change the time axis unit as LoadISISNexus has the wrong one
  void applyTimeAxisUnitCorrection(API::Workspace &workspace);
  void loadPeriodInfo(API::Workspace &workspace);
};
} // namespace DataHandling
} // namespace Mantid
