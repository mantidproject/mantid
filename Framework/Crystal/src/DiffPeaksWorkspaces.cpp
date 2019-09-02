// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/DiffPeaksWorkspaces.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DiffPeaksWorkspaces)

using namespace Kernel;
using namespace API;
using DataObjects::PeaksWorkspace;
using DataObjects::PeaksWorkspace_const_sptr;
using DataObjects::PeaksWorkspace_sptr;

/// Algorithm's name for identification. @see Algorithm::name
const std::string DiffPeaksWorkspaces::name() const {
  return "DiffPeaksWorkspaces";
}
/// Algorithm's version for identification. @see Algorithm::version
int DiffPeaksWorkspaces::version() const { return 1; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string DiffPeaksWorkspaces::category() const {
  return "Crystal\\Peaks";
}

/** Initialises the algorithm's properties.
 */
void DiffPeaksWorkspaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "LHSWorkspace", "", Direction::Input),
                  "The first of peaks.");
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "RHSWorkspace", "", Direction::Input),
                  "The second set of peaks.");
  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                          Direction::Output),
      "The set of peaks that are in the first, but not the second, workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  // N.B. Andrei reckons it should be delta_q/q
  declareProperty("Tolerance", 0.0, mustBePositive,
                  "Maximum difference in each component of Q for which peaks "
                  "are considered identical");
}

/** Executes the algorithm.
 */
void DiffPeaksWorkspaces::exec() {
  PeaksWorkspace_const_sptr LHSWorkspace = getProperty("LHSWorkspace");
  PeaksWorkspace_const_sptr RHSWorkspace = getProperty("RHSWorkspace");
  const double Tolerance = getProperty("Tolerance");

  // Warn if not the same instrument, sample
  if (LHSWorkspace->getInstrument()->getName() !=
      RHSWorkspace->getInstrument()->getName()) {
    g_log.warning("The two input workspaces do not appear to come from data "
                  "take on the same instrument");
  }
  if (LHSWorkspace->sample().getName() != RHSWorkspace->sample().getName()) {
    g_log.warning(
        "The two input workspaces do not appear to relate to the same sample");
  }

  // Copy the first workspace to our output workspace
  PeaksWorkspace_sptr output(LHSWorkspace->clone());
  // Get hold of the peaks in the second workspace
  auto &rhsPeaks = RHSWorkspace->getPeaks();
  // Get hold of the peaks in the first workspace as we'll need to examine them
  auto &lhsPeaks = output->getPeaks();

  Progress progress(this, 0.0, 1.0, rhsPeaks.size());

  std::vector<int> badPeaks;
  // Loop over the peaks in the second workspace, searching for a match in the
  // first
  for (const auto &currentPeak : rhsPeaks) {
    // Now have to go through the first workspace checking for matches
    // Not doing anything clever as peaks workspace are typically not large -
    // just a linear search
    for (int j = 0; j < output->getNumberPeaks(); ++j) {
      const V3D deltaQ =
          currentPeak.getQSampleFrame() - lhsPeaks[j].getQSampleFrame();
      if (deltaQ.nullVector(Tolerance)) // Using a V3D method that does the job
      {
        // As soon as we find a match, remove it from the output and move onto
        // the next rhs peak
        badPeaks.push_back(j);
        break;
      }
    }

    progress.report();
  }
  output->removePeaks(std::move(badPeaks));
  setProperty("OutputWorkspace", output);
}

} // namespace Crystal
} // namespace Mantid
