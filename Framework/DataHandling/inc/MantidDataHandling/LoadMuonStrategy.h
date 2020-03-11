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
#include "MantidNexus/NexusClasses.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>
namespace Mantid {
namespace DataHandling {

class DLLExport LoadMuonStrategy {
public:
  // Constructor
  LoadMuonStrategy(){};
  // Returns the good frames from the nexus entry
  virtual void loadGoodFrames() = 0;
  // Load detector grouping
  virtual API::Workspace_sptr loadDetectorGrouping() = 0;
};
} // namespace DataHandling
} // namespace Mantid