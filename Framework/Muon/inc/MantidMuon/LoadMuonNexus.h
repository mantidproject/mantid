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
#include "MantidAPI/IFileLoader.h"
#include "MantidMuon/DllConfig.h"
#include "MantidDataObjects/Workspace2D_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidMuon/DllConfig.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MuonNexusReader;

namespace Mantid {
namespace Algorithms {
/** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

It is a base class for loaders for versions 1 and 2 of the muon nexus file
format.
It implements property initialization and some common for both versions methods.

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
class MANTID_MUON_DLL LoadMuonNexus : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Default constructor
  LoadMuonNexus();
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
  virtual void runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr) {}
  void checkOptionalProperties();
  void runLoadInstrument(const DataObjects::Workspace2D_sptr &);
  Mantid::API::Algorithm_sptr createSampleLogAlgorithm(DataObjects::Workspace2D_sptr &ws);
  void addToSampleLog(const std::string &logName, const int logNumber, DataObjects::Workspace2D_sptr &ws);
  void addToSampleLog(const std::string &logName, const std::string &log, DataObjects::Workspace2D_sptr &ws);

  /// The name and path of the input file
  std::string m_filename;
  /// The first top-level entry name in the file
  std::string m_entry_name;
  /// The instrument name from Nexus
  std::string m_instrument_name;
  /// The sample name read from Nexus
  std::string m_samplename;
  /// The number of the input entry
  int64_t m_entrynumber;
  /// The number of spectra in the raw file
  specnum_t m_numberOfSpectra;
  /// The number of periods in the raw file
  int64_t m_numberOfPeriods;
  /// Has the spectrum_list property been set?
  bool m_list;
  /// Have the spectrum_min/max properties been set?
  bool m_interval;
  /// The value of the spectrum_list property
  std::vector<specnum_t> m_spec_list;
  /// The value of the spectrum_min property
  specnum_t m_spec_min;
  /// The value of the spectrum_max property
  specnum_t m_spec_max;
  /// The group which each detector belongs to in order
  std::vector<detid_t> m_groupings;

private:
  /// Overwrites Algorithm method.
  void init() override;
};

} // namespace Algorithms
} // namespace Mantid
