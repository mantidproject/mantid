// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/TableWorkspace_fwd.h"
#include "MantidDataObjects/Workspace2D_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Logger.h"

#include <optional>

namespace Mantid {
namespace DataHandling {
class LoadMuonNexusV2NexusHelper;

// Create time zero table
MANTID_DATAHANDLING_DLL DataObjects::TableWorkspace_sptr createTimeZeroTable(const size_t numSpec,
                                                                             const std::vector<double> &timeZeros);

class MANTID_DATAHANDLING_DLL LoadMuonStrategy {
public:
  // Constructor
  LoadMuonStrategy(Kernel::Logger &g_log, std::string filename, LoadMuonNexusV2NexusHelper &nexusLoader);
  // Virtual destructor
  virtual ~LoadMuonStrategy() = default;
  // Load muon log data
  virtual void loadMuonLogData() = 0;
  // Returns the good frames from the nexus entry
  virtual void loadGoodFrames() = 0;
  // Apply time zero correction
  virtual void applyTimeZeroCorrection() = 0;
  // Load detector grouping
  virtual API::Workspace_sptr loadDetectorGrouping() const = 0;
  // Load dead time table
  virtual API::Workspace_sptr loadDeadTimeTable() const = 0;
  // Get time zero table
  virtual API::Workspace_sptr getTimeZeroTable() = 0;

protected:
  // Create grouping table
  std::optional<DataObjects::TableWorkspace_sptr>
  createDetectorGroupingTable(const std::vector<detid_t> &specToLoad,
                              const std::optional<std::vector<detid_t>> &grouping) const;
  // Create deadtimes table
  DataObjects::TableWorkspace_sptr createDeadTimeTable(const std::vector<detid_t> &detectorsLoaded,
                                                       const std::vector<double> &deadTimes) const;

  API::Workspace_sptr loadDefaultDetectorGrouping(const DataObjects::Workspace2D &localWorkspace) const;

  std::vector<detid_t> getLoadedDetectorsFromWorkspace(const DataObjects::Workspace2D &localWorkspace) const;
  // Logger
  Kernel::Logger &m_logger;
  // Filename, used for running child algorithms
  const std::string m_filename;
  // Nexus file loader, used for manipulating the nexus entry
  LoadMuonNexusV2NexusHelper &m_nexusLoader;
};
} // namespace DataHandling
} // namespace Mantid
