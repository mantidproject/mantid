// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>

namespace Mantid {
namespace DataHandling {
namespace LoadMuonNexus3Helper {

// Loads the good frame data from the nexus file
NeXus::NXInt loadGoodFramesDataFromNexus(const NeXus::NXEntry &entry,
                                         bool isFileMultiPeriod);
// Loads the grouping data from the nexus file
std::tuple<std::vector<detid_t>, std::vector<detid_t>>
loadDetectorGroupingFromNexus(NeXus::NXEntry &entry,
                              DataObjects::Workspace2D_sptr &localWorkspace,
                              bool isFileMultiPeriod);
// Load the orientation from the nexus entry
std::string loadMainFieldDirectionFromNexus(const NeXus::NXEntry &entry);
//Load deadtime i
std::tuple<std::vector<detid_t>, std::vector<double>>
loadDeadTimesFromNexus(const NeXus::NXEntry &entry,
                       const DataObjects::Workspace2D_sptr &localWorkspace,
                       const bool isFileMultiPeriod);
// Load first good data from the nexus entry
double loadFirstGoodDataFromNexus(const NeXus::NXEntry &entry);
} // namespace LoadMuonNexus3Helper
} // namespace DataHandling
} // namespace Mantid
