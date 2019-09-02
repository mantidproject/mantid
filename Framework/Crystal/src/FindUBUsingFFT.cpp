// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindUBUsingFFT.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingFFT)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

const std::string FindUBUsingFFT::name() const { return "FindUBUsingFFT"; }

int FindUBUsingFFT::version() const { return 1; }

const std::string FindUBUsingFFT::category() const {
  return "Crystal\\UBMatrix";
}

/** Initialize the algorithm's properties.
 */
void FindUBUsingFFT::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  // use negative values, force user to input all parameters
  this->declareProperty("MinD", -1.0, mustBePositive,
                        "Lower Bound on Lattice Parameters a, b, c");
  this->declareProperty("MaxD", -1.0, mustBePositive,
                        "Upper Bound on Lattice Parameters a, b, c");
  this->declareProperty("Tolerance", 0.15, mustBePositive,
                        "Indexing Tolerance (0.15)");
  this->declareProperty("Iterations", 4, "Iterations to refine UB (4)");
  this->declareProperty("DegreesPerStep", 1.5,
                        "The resolution of the search through possible "
                        "orientations is specified by this parameter.  One to "
                        "two degrees per step is usually adequate.");
}

/** Execute the algorithm.
 */
void FindUBUsingFFT::exec() {
  double min_d = this->getProperty("MinD");
  double max_d = this->getProperty("MaxD");
  double tolerance = this->getProperty("Tolerance");
  int iterations = this->getProperty("Iterations");

  double degrees_per_step = this->getProperty("DegreesPerStep");

  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");

  const std::vector<Peak> &peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  for (size_t i = 0; i < n_peaks; i++) {
    q_vectors.push_back(peaks[i].getQSampleFrame());
  }

  Matrix<double> UB(3, 3, false);
  double error = IndexingUtils::Find_UB(UB, q_vectors, min_d, max_d, tolerance,
                                        degrees_per_step, iterations);

  g_log.notice() << "Error = " << error << '\n';
  g_log.notice() << "UB = " << UB << '\n';

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    g_log.notice(std::string(
        "Found Invalid UB...peaks used might not be linearly independent"));
    g_log.notice(std::string("UB NOT SAVED."));
  } else // tell user how many would be indexed
  {
    std::vector<double> sigabc(7);
    std::vector<V3D> miller_ind;
    std::vector<V3D> indexed_qs;
    double fit_error;
    miller_ind.reserve(q_vectors.size());
    indexed_qs.reserve(q_vectors.size());
    IndexingUtils::GetIndexedPeaks(UB, q_vectors, tolerance, miller_ind,
                                   indexed_qs, fit_error);
    IndexingUtils::Optimize_UB(UB, miller_ind, indexed_qs, sigabc);

    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
    g_log.notice() << "New UB will index " << num_indexed << " Peaks out of "
                   << n_peaks << " with tolerance of " << std::setprecision(3)
                   << std::setw(5) << tolerance << "\n";

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);

    // Show the modified lattice parameters
    g_log.notice() << o_lattice << "\n";

    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}

} // namespace Crystal
} // namespace Mantid
