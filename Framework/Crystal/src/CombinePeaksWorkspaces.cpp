// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/CombinePeaksWorkspaces.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/EnabledWhenProperty.h"

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CombinePeaksWorkspaces)

using namespace Kernel;
using namespace API;

/// Algorithm's name for identification. @see Algorithm::name
const std::string CombinePeaksWorkspaces::name() const { return "CombinePeaksWorkspaces"; }
/// Algorithm's version for identification. @see Algorithm::version
int CombinePeaksWorkspaces::version() const { return 1; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string CombinePeaksWorkspaces::category() const { return "Crystal\\Peaks"; }

/** Initialises the algorithm's properties.
 */
void CombinePeaksWorkspaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("LHSWorkspace", "", Direction::Input),
                  "The first set of peaks.");
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("RHSWorkspace", "", Direction::Input),
                  "The second set of peaks.");
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The combined peaks list.");

  declareProperty("CombineMatchingPeaks", false,
                  "Whether to combine peaks that are identical across the two workspaces");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  // N.B. Andrei reckons it should be delta_q/q
  declareProperty("Tolerance", EMPTY_DBL(), mustBePositive,
                  "Maximum difference in each component of Q for which peaks "
                  "are considered identical");
  setPropertySettings("Tolerance", std::make_unique<EnabledWhenProperty>("CombineMatchingPeaks", IS_EQUAL_TO, "1"));
}

/** Executes the algorithm.
 */
void CombinePeaksWorkspaces::exec() {
  IPeaksWorkspace_const_sptr LHSWorkspace = getProperty("LHSWorkspace");
  IPeaksWorkspace_const_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const bool CombineMatchingPeaks = getProperty("CombineMatchingPeaks");

  // Warn if not the same instrument, sample
  if (LHSWorkspace->getInstrument()->getName() != RHSWorkspace->getInstrument()->getName()) {
    g_log.warning("The two input workspaces do not appear to come from data "
                  "take on the same instrument");
  }
  if (LHSWorkspace->sample().getName() != RHSWorkspace->sample().getName()) {
    g_log.warning("The two input workspaces do not appear to relate to the same sample");
  }

  // Copy the first workspace to our output workspace
  IPeaksWorkspace_sptr output(LHSWorkspace->clone());
  // Get hold of the peaks in the second workspace
  const int rhs_n_peaks = RHSWorkspace->getNumberPeaks();

  Progress progress(this, 0.0, 1.0, rhs_n_peaks);

  // Combine modulation vectors
  // Currently, the lattice can only support up to 3 modulation vectors. If any
  // more than that are combined, a warning is shown and the LHS values are
  // used.
  try {
    std::vector<Kernel::V3D> rhsModVectors;
    std::vector<Kernel::V3D> lhsModVectors;

    // Collect modulation vectors for both workspaces.
    for (int i = 0; i < 3; ++i) { // Currently contains up to 3 vectors.
      const auto modVecR = RHSWorkspace->sample().getOrientedLattice().getModVec(i);
      // Each vector contains 3 values, check that at least one is not 0.
      if (!(modVecR[0] == 0 && modVecR[1] == 0 && modVecR[2] == 0)) {
        rhsModVectors.push_back(modVecR);
      }
      const auto modVecL = LHSWorkspace->sample().getOrientedLattice().getModVec(i);
      if (!(modVecL[0] == 0 && modVecL[1] == 0 && modVecL[2] == 0)) {
        lhsModVectors.push_back(modVecL);
      }
    }

    // Add only unique mod vectors from the rhs list to the lhs list.
    std::remove_copy_if(rhsModVectors.begin(), rhsModVectors.end(), back_inserter(lhsModVectors),
                        [lhsModVectors](Kernel::V3D modVec) {
                          return lhsModVectors.end() != std::find(lhsModVectors.begin(), lhsModVectors.end(), modVec);
                        });

    // Hard limit of 3 mod vectors until PeaksWorkspace is refactored.
    if (lhsModVectors.size() > 3) {
      g_log.warning("There are too many modulation vectors. Using vectors from "
                    "LHSWorkspace");
    } else {
      // This is horrible, but setting mod vectors has to be done by 3 separate
      // methods.
      for (size_t i = 0; i < lhsModVectors.size(); ++i) {
        if (i == 0) {
          output->mutableSample().getOrientedLattice().setModVec1(lhsModVectors[i]);
        } else if (i == 1) {
          output->mutableSample().getOrientedLattice().setModVec2(lhsModVectors[i]);
        } else if (i == 2) {
          output->mutableSample().getOrientedLattice().setModVec3(lhsModVectors[i]);
        }
      }
    }
  } catch (std::runtime_error &e) {
    g_log.error() << "Failed to combine modulation vectors with the following "
                     "error: "
                  << e.what();
  }

  // If not checking for matching peaks, then it's easy...
  if (!CombineMatchingPeaks) {
    // Loop over the peaks in the second workspace, appending each one to the
    // output
    for (int i = 0; i < rhs_n_peaks; i++) {
      output->addPeak(RHSWorkspace->getPeak(i));
      progress.report();
    }
  } else // Check for matching peaks
  {
    const double Tolerance = getProperty("Tolerance");

    // Get hold of the peaks in the first workspace as we'll need to examine
    // them
    const int lhs_n_peaks = LHSWorkspace->getNumberPeaks();
    std::vector<V3D> q_vectors;
    for (int i = 0; i < lhs_n_peaks; i++)
      q_vectors.emplace_back(LHSWorkspace->getPeak(i).getQSampleFrame());

    // Loop over the peaks in the second workspace, appending ones that don't
    // match any in first workspace
    for (int j = 0; j < rhs_n_peaks; j++) {
      const Geometry::IPeak &currentPeak = RHSWorkspace->getPeak(j);
      // Now have to go through the first workspace checking for matches
      // Not doing anything clever as peaks workspace are typically not large -
      // just a linear search
      bool match = false;
      for (int i = 0; i < lhs_n_peaks; i++) {
        const V3D deltaQ = currentPeak.getQSampleFrame() - q_vectors[i];
        if (deltaQ.nullVector(Tolerance)) // Using a V3D method that does the job
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

} // namespace Mantid::Crystal
