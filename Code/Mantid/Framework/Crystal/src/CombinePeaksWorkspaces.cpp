#include "MantidCrystal/CombinePeaksWorkspaces.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CombinePeaksWorkspaces)

using namespace Kernel;
using namespace API;
using DataObjects::PeaksWorkspace;
using DataObjects::PeaksWorkspace_const_sptr;
using DataObjects::PeaksWorkspace_sptr;
using DataObjects::Peak;

/** Constructor
 */
CombinePeaksWorkspaces::CombinePeaksWorkspaces() {}

/** Destructor
 */
CombinePeaksWorkspaces::~CombinePeaksWorkspaces() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string CombinePeaksWorkspaces::name() const {
  return "CombinePeaksWorkspaces";
}
/// Algorithm's version for identification. @see Algorithm::version
int CombinePeaksWorkspaces::version() const { return 1; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string CombinePeaksWorkspaces::category() const { return "Crystal"; }

/** Initialises the algorithm's properties.
 */
void CombinePeaksWorkspaces::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("LHSWorkspace", "",
                                                        Direction::Input),
                  "The first set of peaks.");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("RHSWorkspace", "",
                                                        Direction::Input),
                  "The second set of peaks.");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The combined peaks list.");

  declareProperty(
      "CombineMatchingPeaks", false,
      "Whether to combine peaks that are identical across the two workspaces");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  // N.B. Andrei reckons it should be delta_q/q
  declareProperty("Tolerance", EMPTY_DBL(), mustBePositive,
                  "Maximum difference in each component of Q for which peaks "
                  "are considered identical");
  setPropertySettings(
      "Tolerance",
      new EnabledWhenProperty("CombineMatchingPeaks", IS_EQUAL_TO, "1"));
}

/** Executes the algorithm.
 */
void CombinePeaksWorkspaces::exec() {
  PeaksWorkspace_const_sptr LHSWorkspace = getProperty("LHSWorkspace");
  PeaksWorkspace_const_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const bool CombineMatchingPeaks = getProperty("CombineMatchingPeaks");

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

  Progress progress(this, 0, 1, rhsPeaks.size());

  // If not checking for matching peaks, then it's easy...
  if (!CombineMatchingPeaks) {
    // Loop over the peaks in the second workspace, appending each one to the
    // output
    for (size_t i = 0; i < rhsPeaks.size(); ++i) {
      output->addPeak(rhsPeaks[i]);
      progress.report();
    }
  } else // Check for matching peaks
  {
    const double Tolerance = getProperty("Tolerance");

    // Get hold of the peaks in the first workspace as we'll need to examine
    // them
    auto &lhsPeaks = LHSWorkspace->getPeaks();

    // Loop over the peaks in the second workspace, appending ones that don't
    // match any in first workspace
    for (size_t i = 0; i < rhsPeaks.size(); ++i) {
      const Peak &currentPeak = rhsPeaks[i];
      // Now have to go through the first workspace checking for matches
      // Not doing anything clever as peaks workspace are typically not large -
      // just a linear search
      bool match = false;
      for (size_t j = 0; j < lhsPeaks.size(); ++j) {
        const V3D deltaQ =
            currentPeak.getQSampleFrame() - lhsPeaks[j].getQSampleFrame();
        if (deltaQ.nullVector(
                Tolerance)) // Using a V3D method that does the job
        {
          match = true;
          break;
        }
      }
      // Only add the peak if there was no match
      if (!match)
        output->addPeak(currentPeak);
      progress.report();
    }
  }

  setProperty("OutputWorkspace", output);
}

} // namespace Crystal
} // namespace Mantid
