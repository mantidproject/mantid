// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/CalculatePeaksHKL.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePeaksHKL)

/// Algorithm's name for identification. @see Algorithm::name
const std::string CalculatePeaksHKL::name() const {
  return "CalculatePeaksHKL";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculatePeaksHKL::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculatePeaksHKL::category() const {
  return "Crystal\\Peaks";
}

/** Initialize the algorithm's properties.
 */
void CalculatePeaksHKL::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  this->declareProperty(
      "OverWrite", false,
      "Overwrite existing miller indices as well as empty ones.");

  this->declareProperty(
      std::make_unique<PropertyWithValue<int>>("NumIndexed", 0, Direction::Output),
      "Gets set with the number of indexed peaks.");
}

/** Execute the algorithm.
 */
void CalculatePeaksHKL::exec() {
  Mantid::DataObjects::PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  const bool overwrite = getProperty("OverWrite");
  const int n_peaks = ws->getNumberPeaks();

  OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
  const Matrix<double> &UB = o_lattice.getUB();

  DblMatrix UB_inverse(UB);

  if (IndexingUtils::CheckUB(UB)) {
    UB_inverse.Invert();
  } else {
    throw std::runtime_error("The UB is not valid");
  }

  int peaksIndexed = 0;
  for (int i = 0; i < n_peaks; i++) {
    Peak &peak = ws->getPeak(i);
    if (overwrite || (peak.getHKL().nullVector())) {
      V3D qlab = peak.getQSampleFrame();
      V3D hkl = UB_inverse * qlab / (2.0 * M_PI);
      peak.setHKL(hkl);
      ++peaksIndexed;
    }
  }
  setProperty("NumIndexed", peaksIndexed);
}

} // namespace Crystal
} // namespace Mantid
