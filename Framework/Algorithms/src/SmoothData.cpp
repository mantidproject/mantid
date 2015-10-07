//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SmoothData.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothData)

using namespace Kernel;
using namespace API;

void SmoothData::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the input workspace");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");
  std::vector<int> npts0;
  npts0.push_back(3);
  auto min = boost::make_shared<Kernel::ArrayBoundedValidator<int>>();
  min->setLower(3);
  // The number of points to use in the smoothing.
  declareProperty(
      new ArrayProperty<int>("NPoints", npts0, min, Direction::Input),
      "The number of points to average over (minimum 3). If an even number is\n"
      "given, it will be incremented by 1 to make it odd (default value 3)");
  declareProperty(
      new WorkspaceProperty<Mantid::DataObjects::GroupingWorkspace>(
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
  PARALLEL_FOR2(inputWorkspace, outputWorkspace)
  // Loop over all the spectra in the workspace
  for (int i = 0; i < static_cast<int>(inputWorkspace->getNumberHistograms());
       ++i) {
    PARALLEL_START_INTERUPT_REGION
    int npts = nptsGroup[0];
    if (groupWS) {
      const int group = validateSpectrumInGroup(static_cast<size_t>(i));
      if (group < 0)
        npts = 3;
      else
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
    int halfWidth = (npts - 1) / 2;

    // Copy the X data over. Preserves data sharing if present in input
    // workspace.
    outputWorkspace->setX(i, inputWorkspace->refX(i));

    // Now get references to the Y & E vectors in the input and output
    // workspaces
    const MantidVec &Y = inputWorkspace->readY(i);
    const MantidVec &E = inputWorkspace->readE(i);
    MantidVec &newY = outputWorkspace->dataY(i);
    MantidVec &newE = outputWorkspace->dataE(i);
    if (npts == 0) {
      newY = Y;
      newE = E;
      continue;
    }
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
  const std::set<detid_t> &dets =
      inputWorkspace->getSpectrum(wi)->getDetectorIDs();
  if (dets.empty()) // Not in group
  {
    g_log.debug() << wi << " <- this workspace index is empty!\n";
    return -1;
  }

  std::set<detid_t>::const_iterator it = dets.begin();
  if (*it < 0) // bad pixel id
    return -1;

  try { // what if index out of range?
    const int group = udet2group.at(*it);
    if (group <= 0)
      return -1;
    ++it;
    for (; it != dets.end(); ++it) // Loop other all other udets
    {
      if (udet2group.at(*it) != group)
        return -1;
    }
    return group;
  } catch (...) {
  }

  return -1;
}
} // namespace Algorithms
} // namespace Mantid
