// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/LoadMuonStrategy.h"

namespace Mantid {
namespace DataHandling {
class MultiPeriodLoadMuonStrategy : public LoadMuonStrategy {
public:
  // Constructor
  MultiPeriodLoadMuonStrategy(Kernel::Logger &g_log, const std::string &filename,
                              LoadMuonNexusV2NexusHelper &nexusLoader, API::WorkspaceGroup &workspaceGroup);
  // Loads the muon log data
  void loadMuonLogData() override;
  // Returns the good frames from the nexus entry
  void loadGoodFrames() override;
  // Apply the time zero correction to the workspace time axis
  void applyTimeZeroCorrection() override;
  // Load detector grouping
  API::Workspace_sptr loadDetectorGrouping() const override;
  // Load dead time table
  API::Workspace_sptr loadDeadTimeTable() const override;
  // Get time zero table
  API::Workspace_sptr getTimeZeroTable() override;

private:
  API::WorkspaceGroup &m_workspaceGroup;
  std::vector<detid_t> m_detectors;
  std::vector<detid_t> getLoadedDetectors();
};
} // namespace DataHandling
} // namespace Mantid
