// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindUBUsingLatticeParameters.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingLatticeParameters)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void FindUBUsingLatticeParameters::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  auto moreThan2Int = boost::make_shared<BoundedValidator<int>>();
  moreThan2Int->setLower(2);

  auto reasonable_angle = boost::make_shared<BoundedValidator<double>>();
  reasonable_angle->setLower(5.0);
  reasonable_angle->setUpper(175.0);

  // use negative values, force user to input all parameters
  this->declareProperty("a", -1.0, mustBePositive, "Lattice parameter a");
  this->declareProperty("b", -1.0, mustBePositive, "Lattice parameter b");
  this->declareProperty("c", -1.0, mustBePositive, "Lattice parameter c");
  this->declareProperty("alpha", -1.0, reasonable_angle,
                        "Lattice parameter alpha");
  this->declareProperty("beta", -1.0, reasonable_angle,
                        "Lattice parameter beta");
  this->declareProperty("gamma", -1.0, reasonable_angle,
                        "Lattice parameter gamma");
  this->declareProperty("NumInitial", 15, moreThan2Int,
                        "Number of Peaks to Use on First Pass(15)");
  this->declareProperty("Tolerance", 0.15, mustBePositive,
                        "Indexing Tolerance (0.15)");
  this->declareProperty("FixParameters", false,
                        "Do not optimise the UB after finding the orientation");
  this->declareProperty("Iterations", 1, "Iterations to refine UB (1)");
}

/** Execute the algorithm.
 */
void FindUBUsingLatticeParameters::exec() {
  double a = this->getProperty("a");
  double b = this->getProperty("b");
  double c = this->getProperty("c");
  double alpha = this->getProperty("alpha");
  double beta = this->getProperty("beta");
  double gamma = this->getProperty("gamma");
  int num_initial = this->getProperty("NumInitial");
  double tolerance = this->getProperty("Tolerance");
  auto fixAll = this->getProperty("FixParameters");
  auto iterations = this->getProperty("Iterations");

  int base_index = -1; // these "could" be properties if need be
  double degrees_per_step = 1.5;

  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws)
    throw std::runtime_error("Could not read the peaks workspace");

  std::vector<Peak> &peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  q_vectors.reserve(n_peaks);

  for (size_t i = 0; i < n_peaks; i++)
    q_vectors.push_back(peaks[i].getQSampleFrame());

  Matrix<double> UB(3, 3, false);
  OrientedLattice lattice(a, b, c, alpha, beta, gamma);
  double error =
      IndexingUtils::Find_UB(UB, q_vectors, lattice, tolerance, base_index,
                             num_initial, degrees_per_step, fixAll, iterations);

  g_log.notice() << "Error = " << error << '\n';
  g_log.notice() << "UB = " << UB << '\n';

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    g_log.notice(std::string(
        "Found Invalid UB...peaks used might not be linearly independent"));
    g_log.notice(std::string("UB NOT SAVED."));
  } else // tell user how many would be indexed
  {      // and save the UB in the sample
    char logInfo[200];
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
    sprintf(logInfo,
            std::string(
                "New UB will index %1d Peaks out of %1d with tolerance %5.3f")
                .c_str(),
            num_indexed, n_peaks, tolerance);
    g_log.notice(std::string(logInfo));

    double calc_a = lattice.a();
    double calc_b = lattice.b();
    double calc_c = lattice.c();
    double calc_alpha = lattice.alpha();
    double calc_beta = lattice.beta();
    double calc_gamma = lattice.gamma();
    // Show the modified lattice parameters
    g_log.notice() << lattice << "\n";

    sprintf(logInfo,
            std::string("Lattice Parameters (Refined - Input): %11.6f "
                        "%11.6f %11.6f %11.6f %11.6f %11.6f")
                .c_str(),
            calc_a - a, calc_b - b, calc_c - c, calc_alpha - alpha,
            calc_beta - beta, calc_gamma - gamma);
    g_log.notice(std::string(logInfo));
    ws->mutableSample().setOrientedLattice(&lattice);
  }
}

} // namespace Crystal
} // namespace Mantid
