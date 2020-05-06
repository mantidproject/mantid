// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>
namespace Mantid {
namespace DataHandling {
class SinglePeriodLoadMuonStrategy : public LoadMuonStrategy {
public:
  // Constructor
  SinglePeriodLoadMuonStrategy(Kernel::Logger &g_log,
                               const std::string &filename,
                               NeXus::NXEntry entry,
                               DataObjects::Workspace2D_sptr workspace,
                               int entryNumber, bool isFileMultiPeriod);
  // Loads the muon log data
  void loadMuonLogData() override;
  // Returns the good frames from the nexus entry
  void loadGoodFrames() override;
  // Load detector grouping
  API::Workspace_sptr loadDetectorGrouping() override;
  // Load default grouping from ID
  API::Workspace_sptr loadDefaultDetectorGrouping() const;
  // Load dead time table
  API::Workspace_sptr loadDeadTimeTable() const override;

private:
  NeXus::NXEntry m_entry;
  DataObjects::Workspace2D_sptr m_workspace;
  int m_entryNumber;
  bool m_isFileMultiPeriod;
};
} // namespace DataHandling
} // namespace Mantid