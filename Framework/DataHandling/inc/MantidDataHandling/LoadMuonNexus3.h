// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/NexusDescriptor.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <nexus/NeXusFile.hpp>

#include <climits>

namespace Mantid {
namespace DataHandling {
/** @class LoadMuonNexus3 LoadMuonNexus3.h DataHandling/LoadMuonNexus3.h

Loads a file in the Nexus Muon format version 2 and stores it in a 2D workspace
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

@author Stephen Smith, ISIS
*/
class DLLExport LoadMuonNexus3
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  // Default constructor
  LoadMuonNexus3();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMuonNexus3"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a Muon Nexus V2 data file and stores it in a 2D "
           "workspace (Workspace2D class).";
  }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  // Version
  int version() const override { return 3; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  // Determines whether entry contains multi period data
  void isEntryMultiPeriod(const NeXus::NXEntry &entry);
  // Run child algorithm LoadISISNexus3
  void runLoadISISNexus();
  // Load Muon specific properties
  void loadMuonProperties(const NeXus::NXEntry &entry);
  /// The name and path of the input file
  std::string m_filename;
  /// The sample name read from Nexus
  std::string m_sampleName;
  /// The number of the input entry
  int64_t m_entrynumber;
  /// The number of periods in the raw file
  int64_t m_numberOfPeriods;
  /// Has the spectrum_list property been set?
  bool m_list;
  /// Have the spectrum_min/max properties been set?
  bool m_interval;
  // Is file multi period
  bool m_isFileMultiPeriod;
  // Are multi periods loaded
  bool m_multiPeriodsLoaded;
  /// The value of the spectrum_list property
  std::vector<specnum_t> m_specList;
  /// The value of the spectrum_min property
  int64_t m_specMin;
  /// The value of the spectrum_max property
  int64_t m_specMax;
  /// The group which each detector belongs to in order
  std::vector<specnum_t> m_groupings;
  // The loading stratergy used
  std::unique_ptr<LoadMuonStrategy> m_loadMuonStrategy;
};

} // namespace DataHandling
} // namespace Mantid
