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
  double magneticField = -1;
  double temperature = -1;
};
} // namespace MuonNexus

class LoadMuonNexusV2NexusHelper {
public:
  LoadMuonNexusV2NexusHelper(const NeXus::NXEntry &entry);

  // Loads the good frame data from the nexus file
  NeXus::NXInt loadGoodFramesDataFromNexus(bool isFileMultiPeriod);
  // Loads the grouping data from the nexus file
  std::vector<detid_t> loadDetectorGroupingFromNexus(const std::vector<detid_t> &loadedDetectors,
                                                     bool isFileMultiPeriod, int periodNumber);
  // Load the orientation from the nexus entry
  std::string loadMainFieldDirectionFromNexus();
  // Load deadtime information
  std::vector<double> loadDeadTimesFromNexus(const std::vector<detid_t> &loadedDetectors, bool isFileMultiPeriod,
                                             int periodNumber);
  // Load first good data from the nexus entry
  double loadFirstGoodDataFromNexus();
  // Load time zero from the nexus entry
  double loadTimeZeroFromNexusFile();
  // Load time zero from nexus entry into a vector
  std::vector<double> loadTimeZeroListFromNexusFile(size_t numSpectra);
  // Load Muon sample information from the nexus file
  MuonNexus::SampleInformation loadSampleInformationFromNexus();
  // Number of periods in the nexus file
  int getNumberOfPeriods() const;

private:
  const NeXus::NXEntry &m_entry;
};
} // namespace DataHandling
} // namespace Mantid
