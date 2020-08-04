// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/Workspace2D_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

#include <vector>

namespace Mantid {
namespace DataHandling {
namespace MuonNexus {
struct SampleInformation {
  double magneticField;
  double temperature;
};
} // namespace MuonNexus
namespace LoadMuonNexusV2Helper {

// Loads the good frame data from the nexus file
NeXus::NXInt loadGoodFramesDataFromNexus(const NeXus::NXEntry &entry,
                                         bool isFileMultiPeriod);
// Loads the grouping data from the nexus file
std::vector<detid_t>
loadDetectorGroupingFromNexus(const NeXus::NXEntry &entry,
                              const std::vector<detid_t> &loadedDetectors,
                              bool isFileMultiPeriod, const int periodNumber);
// Load the orientation from the nexus entry
std::string loadMainFieldDirectionFromNexus(const NeXus::NXEntry &entry);
// Load deadtime information
std::vector<double>
loadDeadTimesFromNexus(const NeXus::NXEntry &entry,
                       const std::vector<detid_t> &loadedDetectors,
                       const bool isFileMultiPeriod, const int periodNumber);
// Load first good data from the nexus entry
double loadFirstGoodDataFromNexus(const NeXus::NXEntry &entry);
// Load time zero from the nexus entry
double loadTimeZeroFromNexusFile(const NeXus::NXEntry &entry);
// Load time zero from nexus entry into a vector
std::vector<double> loadTimeZeroListFromNexusFile(const NeXus::NXEntry &entry,
                                                  size_t numSpectra);
// Load Muon sample information from the nexus file
MuonNexus::SampleInformation
loadSampleInformationFromNexus(const NeXus::NXEntry &entry);

// Load grouping information from IDF
API::Workspace_sptr loadDefaultDetectorGrouping(
    const DataObjects::Workspace2D_sptr &localWorkspace);

// Load detectors from input workspace
std::vector<detid_t> getLoadedDetectorsFromWorkspace(
    const DataObjects::Workspace2D_sptr &localWorkspace);

} // namespace LoadMuonNexusV2Helper
} // namespace DataHandling
} // namespace Mantid
