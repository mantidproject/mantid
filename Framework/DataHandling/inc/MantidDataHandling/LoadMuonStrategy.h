// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Logger.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>
namespace Mantid {
namespace DataHandling {

class DLLExport LoadMuonStrategy {
public:
  // Constructor
  LoadMuonStrategy(Kernel::Logger &g_log, const std::string &filename);
  // Load muon log data
  virtual void loadMuonLogData() = 0;
  // Returns the good frames from the nexus entry
  virtual void loadGoodFrames() = 0;
  // Load detector grouping
  virtual API::Workspace_sptr loadDetectorGrouping() = 0;
  // Load dead time table
  virtual void loadDeadTimeTable() const = 0;

protected:
  /// Logger
  Kernel::Logger &m_logger;
  // File name, used for running child algorithms
  std::string m_filename;
};
} // namespace DataHandling
} // namespace Mantid