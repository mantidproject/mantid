// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SmoothData.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Smoothing.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothData)

using namespace Kernel;
using namespace API;
using HistogramData::Histogram;

void SmoothData::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the algorithm");
  std::vector<int> npts0{3};
  auto min = std::make_shared<Kernel::ArrayBoundedValidator<int>>();
  min->setLower(3);
  // The number of points to use in the smoothing.
  declareProperty(std::make_unique<ArrayProperty<int>>("NPoints", std::move(npts0), std::move(min), Direction::Input),
                  "The number of points to average over (minimum 3). If an even number is\n"
                  "given, it will be incremented by 1 to make it odd (default value 3)");
  declareProperty(std::make_unique<WorkspaceProperty<Mantid::DataObjects::GroupingWorkspace>>(
                      "GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "Optional: GroupingWorkspace to use for vector of NPoints.");
}

void SmoothData::exec() {
  // Get the input properties
  inputWorkspace = getProperty("InputWorkspace");

  std::vector<int> nptsGroup = getProperty("NPoints");
  Mantid::DataObjects::GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
  if (groupWS) {
    udet2group.clear();
    int64_t nGroups;
    groupWS->makeDetectorIDToGroupVector(udet2group, nGroups);
  }

  // Check that the number of points in the smoothing isn't larger than the
  // spectrum length
  const auto vecSize = static_cast<int>(inputWorkspace->blocksize());

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

  Progress progress(this, 0.0, 1.0, inputWorkspace->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int i = 0; i < static_cast<int>(inputWorkspace->getNumberHistograms()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    int npts = nptsGroup[0];
    if (groupWS) {
      const int group = validateSpectrumInGroup(static_cast<size_t>(i));
      if (group == -1) {
        npts = 3;
      } else {
        assert(group != 0);
        npts = nptsGroup[group - 1];
      }
    }
    if (npts >= vecSize) {
      g_log.error("The number of averaging points requested is larger than the "
                  "spectrum length");
      throw std::out_of_range("The number of averaging points requested is "
                              "larger than the spectrum length");
    }
    // Number of smoothing points must always be an odd number, so add 1 if it
    // isn't.
    if (!(npts % 2)) {
      g_log.information("Adding 1 to number of smoothing points, since it must "
                        "always be odd");
      ++npts;
    }

    outputWorkspace->setHistogram(i, smooth(inputWorkspace->histogram(i), npts));

    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  } // Loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION

  // Set the output workspace to its property
  setProperty("OutputWorkspace", outputWorkspace);
} // namespace Algorithms
//=============================================================================
/** Verify that all the contributing detectors to a spectrum belongs to the same
 * group
 *  @param wi :: The workspace index in the workspace
 *  @return Group number if successful otherwise return -1
 */
int SmoothData::validateSpectrumInGroup(size_t wi) {
  const auto &dets = inputWorkspace->getSpectrum(wi).getDetectorIDs();
  if (dets.empty()) // Not in group
  {
    g_log.debug() << wi << " <- this workspace index is empty!\n";
    return -1;
  }

  auto it = dets.cbegin();
  if (*it < 0) // bad pixel id
    return -1;

  try { // what if index out of range?
    const int group = udet2group.at(*it);
    if (group <= 0)
      return -1;
    ++it;
    for (; it != dets.end(); ++it) // Loop all other udets
    {
      if (udet2group.at(*it) != group)
        return -1;
    }
    return group;
  } catch (...) {
  }

  return -1;
}

/// Returns a smoothed version of histogram. npts must be odd.
Histogram smooth(const Histogram &histogram, unsigned int const npts) {
  if (npts < 3)
    return histogram;

  Histogram smoothed(histogram);

  auto const &Y = histogram.y().rawData();
  auto const &E = histogram.e().rawData();
  auto &newY = smoothed.mutableY();
  auto &newE = smoothed.mutableE();
  newY = Mantid::Kernel::Smoothing::boxcarSmooth(Y, npts);
  newE = Mantid::Kernel::Smoothing::boxcarErrorSmooth(E, npts);
  return smoothed;
}

} // namespace Mantid::Algorithms
