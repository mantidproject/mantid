// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidDataObjects/Workspace2D_fwd.h"

namespace Mantid {
namespace DataHandling {
class SinglePeriodLoadMuonStrategy : public LoadMuonStrategy {
public:
  // Constructor
  SinglePeriodLoadMuonStrategy(Kernel::Logger &g_log, const std::string &filename,
                               LoadMuonNexusV2NexusHelper &nexusLoader, DataObjects::Workspace2D &workspace,
                               int entryNumber, bool isFileMultiPeriod);
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
  DataObjects::Workspace2D &m_workspace;
  int m_entryNumber;
  bool m_isFileMultiPeriod;
  std::vector<detid_t> m_detectors;
  std::vector<detid_t> getLoadedDetectors();
};
} // namespace DataHandling
} // namespace Mantid
