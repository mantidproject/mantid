#include "MantidCrystal/FindUBUsingMinMaxD.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <cstdio>

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingMinMaxD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//--------------------------------------------------------------------------
/** Constructor
 */
FindUBUsingMinMaxD::FindUBUsingMinMaxD() {
  useAlgorithm("FindUBUsingFFT");
  deprecatedDate("2013-06-03");
}

//--------------------------------------------------------------------------
/** Destructor
 */
FindUBUsingMinMaxD::~FindUBUsingMinMaxD() {}

const std::string FindUBUsingMinMaxD::name() const {
  return "FindUBUsingMinMaxD";
}

int FindUBUsingMinMaxD::version() const { return 1; }

const std::string FindUBUsingMinMaxD::category() const { return "Crystal"; }

//--------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindUBUsingMinMaxD::init() {
  this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  boost::shared_ptr<BoundedValidator<double>> mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

  boost::shared_ptr<BoundedValidator<int>> atLeast3Int(
      new BoundedValidator<int>());
  atLeast3Int->setLower(3);

  // use negative values, force user to input all parameters
  this->declareProperty(new PropertyWithValue<double>(
                            "MinD", -1.0, mustBePositive, Direction::Input),
                        "Lower Bound on Lattice Parameters a, b, c");

  this->declareProperty(new PropertyWithValue<double>(
                            "MaxD", -1.0, mustBePositive, Direction::Input),
                        "Upper Bound on Lattice Parameters a, b, c");

  this->declareProperty(new PropertyWithValue<int>(
                            "NumInitial", 20, atLeast3Int, Direction::Input),
                        "Number of Peaks to Use on First Pass(20)");

  this->declareProperty(new PropertyWithValue<double>("Tolerance", 0.15,
                                                      mustBePositive,
                                                      Direction::Input),
                        "Indexing Tolerance (0.15)");
}

//--------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindUBUsingMinMaxD::exec() {
  double min_d = this->getProperty("MinD");
  double max_d = this->getProperty("MaxD");
  int num_initial = this->getProperty("NumInitial");
  double tolerance = this->getProperty("Tolerance");

  int base_index = -1; // these "could" be properties if need be
  double degrees_per_step = 1;

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
  double error =
      IndexingUtils::Find_UB(UB, q_vectors, min_d, max_d, tolerance, base_index,
                             num_initial, degrees_per_step);

  std::cout << "Error = " << error << std::endl;
  std::cout << "UB = " << UB << std::endl;

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    g_log.notice(std::string(
        "Found Invalid UB...peaks used might not be linearly independent"));
    g_log.notice(std::string("UB NOT SAVED."));
  } else // tell user how many would be indexed
  {      // and save the UB in the sample

    std::vector<double> sigabc(7);
    std::vector<V3D> miller_ind;
    std::vector<V3D> indexed_qs;
    double fit_error;
    miller_ind.reserve(q_vectors.size());
    indexed_qs.reserve(q_vectors.size());
    IndexingUtils::GetIndexedPeaks(UB, q_vectors, tolerance, miller_ind,
                                   indexed_qs, fit_error);

    IndexingUtils::Optimize_UB(UB, miller_ind, indexed_qs, sigabc);
    char logInfo[200];
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
    sprintf(logInfo,
            std::string(
                "New UB will index %1d Peaks out of %1d with tolerance %5.3f")
                .c_str(),
            num_indexed, n_peaks, tolerance);
    g_log.notice(std::string(logInfo));

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);

    double calc_a = o_lattice.a();
    double calc_b = o_lattice.b();
    double calc_c = o_lattice.c();
    double calc_alpha = o_lattice.alpha();
    double calc_beta = o_lattice.beta();
    double calc_gamma = o_lattice.gamma();
    // Show the modified lattice parameters
    sprintf(
        logInfo,
        std::string("Lattice Parameters: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f")
            .c_str(),
        calc_a, calc_b, calc_c, calc_alpha, calc_beta, calc_gamma);
    g_log.notice(std::string(logInfo));

    g_log.notice() << "Parameter Errors  :" << std::fixed
                   << std::setprecision(3) << std::setw(9) << sigabc[0]
                   << std::fixed << std::setprecision(3) << std::setw(9)
                   << sigabc[1] << std::fixed << std::setprecision(3)
                   << std::setw(9) << sigabc[2] << std::fixed
                   << std::setprecision(3) << std::setw(9) << sigabc[3]
                   << std::fixed << std::setprecision(3) << std::setw(9)
                   << sigabc[4] << std::fixed << std::setprecision(3)
                   << std::setw(9) << sigabc[5] << std::endl;
    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}

} // namespace Mantid
} // namespace Crystal
