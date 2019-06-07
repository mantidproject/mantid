// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SmoothData.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothData)

using namespace Kernel;
using namespace API;
using HistogramData::Histogram;

void SmoothData::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Name of the input workspace");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");
  std::vector<int> npts0{3};
  auto min = boost::make_shared<Kernel::ArrayBoundedValidator<int>>();
  min->setLower(3);
  // The number of points to use in the smoothing.
  declareProperty(
      std::make_unique<ArrayProperty<int>>("NPoints", std::move(npts0),
                                           std::move(min), Direction::Input),
      "The number of points to average over (minimum 3). If an even number is\n"
      "given, it will be incremented by 1 to make it odd (default value 3)");
  declareProperty(
      std::make_unique<
          WorkspaceProperty<Mantid::DataObjects::GroupingWorkspace>>(
          "GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: GroupingWorkspace to use for vector of NPoints.");
}

void SmoothData::exec() {
  // Get the input properties
  inputWorkspace = getProperty("InputWorkspace");

  std::vector<int> nptsGroup = getProperty("NPoints");
  Mantid::DataObjects::GroupingWorkspace_sptr groupWS =
      getProperty("GroupingWorkspace");
  if (groupWS) {
    udet2group.clear();
    int64_t nGroups;
    groupWS->makeDetectorIDToGroupVector(udet2group, nGroups);
  }

  // Check that the number of points in the smoothing isn't larger than the
  // spectrum length
  const int vecSize = static_cast<int>(inputWorkspace->blocksize());

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(inputWorkspace);

  Progress progress(this, 0.0, 1.0, inputWorkspace->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int i = 0; i < static_cast<int>(inputWorkspace->getNumberHistograms());
       ++i) {
    PARALLEL_START_INTERUPT_REGION
    int npts = nptsGroup[0];
    if (groupWS) {
      const int group = validateSpectrumInGroup(static_cast<size_t>(i));
      if (group < 0)
        npts = 3;
      else
        // group is never 0. We can safely subtract.
        npts = nptsGroup[group - 1];
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

    outputWorkspace->setHistogram(i,
                                  smooth(inputWorkspace->histogram(i), npts));

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  } // Loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  // Set the output workspace to its property
  setProperty("OutputWorkspace", outputWorkspace);
}
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
Histogram smooth(const Histogram &histogram, int npts) {
  if (npts == 0)
    return histogram;
  int halfWidth = (npts - 1) / 2;

  Histogram smoothed(histogram);

  const auto &Y = histogram.y();
  const auto &E = histogram.e();
  auto &newY = smoothed.mutableY();
  auto &newE = smoothed.mutableE();
  // Use total to help hold our moving average
  double total = 0.0, totalE = 0.0;
  // First push the values ahead of the current point onto total
  for (int k = 0; k < halfWidth; ++k) {
    if (Y[k] == Y[k])
      total += Y[k]; // Exclude if NaN
    totalE += E[k] * E[k];
  }
  // Now calculate the smoothed values for the 'end' points, where the number
  // contributing
  // to the smoothing will be less than NPoints
  for (int j = 0; j <= halfWidth; ++j) {
    const int index = j + halfWidth;
    if (Y[index] == Y[index])
      total += Y[index]; // Exclude if NaN
    newY[j] = total / (index + 1);
    totalE += E[index] * E[index];
    newE[j] = sqrt(totalE) / (index + 1);
  }
  // This is the main part, where each data point is the average of NPoints
  // points centred on the
  // current point. Note that the statistical error will be reduced by
  // sqrt(npts) because more
  // data is now contributing to each point.
  const int vecSize = static_cast<int>(histogram.size());
  for (int k = halfWidth + 1; k < vecSize - halfWidth; ++k) {
    const int kp = k + halfWidth;
    const int km = k - halfWidth - 1;
    total += (Y[kp] != Y[kp] ? 0.0 : Y[kp]) -
             (Y[km] != Y[km] ? 0.0 : Y[km]); // Exclude if NaN
    newY[k] = total / npts;
    totalE += E[kp] * E[kp] - E[km] * E[km];
    // Use of a moving average can lead to rounding error where what should be
    // zero actually comes out as a tiny negative number - bad news for sqrt
    // so protect
    newE[k] = std::sqrt(std::abs(totalE)) / npts;
  }
  // This deals with the 'end' at the tail of each spectrum
  for (int l = vecSize - halfWidth; l < vecSize; ++l) {
    const int index = l - halfWidth;
    total -=
        (Y[index - 1] != Y[index - 1] ? 0.0 : Y[index - 1]); // Exclude if NaN
    newY[l] = total / (vecSize - index);
    totalE -= E[index - 1] * E[index - 1];
    newE[l] = std::sqrt(std::abs(totalE)) / (vecSize - index);
  }
  return smoothed;
}

} // namespace Algorithms
} // namespace Mantid
