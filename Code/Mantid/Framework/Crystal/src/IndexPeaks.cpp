#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include <cstdio>

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IndexPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//--------------------------------------------------------------------------
/** Constructor
 */
IndexPeaks::IndexPeaks() {}

//--------------------------------------------------------------------------
/** Destructor
 */
IndexPeaks::~IndexPeaks() {}

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IndexPeaks::init() {
  this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  boost::shared_ptr<BoundedValidator<double>> mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

  this->declareProperty(new PropertyWithValue<double>("Tolerance", 0.15,
                                                      mustBePositive,
                                                      Direction::Input),
                        "Indexing Tolerance (0.15)");

  this->declareProperty(
      new PropertyWithValue<int>("NumIndexed", 0, Direction::Output),
      "Gets set with the number of indexed peaks.");

  this->declareProperty(
      new PropertyWithValue<double>("AverageError", 0.0, Direction::Output),
      "Gets set with the average HKL indexing error.");

  this->declareProperty("RoundHKLs", true,
                        "Round H, K and L values to integers");
}

//--------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IndexPeaks::exec() {
  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
  Matrix<double> UB = o_lattice.getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error(
        "ERROR: The stored UB is not a valid orientation matrix");
  }

  bool round_hkls = this->getProperty("RoundHKLs");

  std::vector<Peak> &peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  // get list of run numbers in this peaks workspace
  std::vector<int> run_numbers;
  for (size_t i = 0; i < n_peaks; i++) {
    int run = peaks[i].getRunNumber();
    bool found = false;
    size_t k = 0;
    while (k < run_numbers.size() && !found) {
      if (run == run_numbers[k])
        found = true;
      else
        k++;
    }
    if (!found)
      run_numbers.push_back(run);
  }

  // index the peaks for each run separately, using a UB matrix optimized for
  // that run
  int total_indexed = 0;
  double average_error;
  double total_error = 0;
  double tolerance = this->getProperty("Tolerance");

  for (size_t run_index = 0; run_index < run_numbers.size(); run_index++) {
    std::vector<V3D> miller_indices;
    std::vector<V3D> q_vectors;

    int run = run_numbers[run_index];
    for (size_t i = 0; i < n_peaks; i++) {
      if (peaks[i].getRunNumber() == run)
        q_vectors.push_back(peaks[i].getQSampleFrame());
    }

    Matrix<double> tempUB(UB);

    int num_indexed = 0;
    int original_indexed = 0;
    double original_error = 0;
    original_indexed = IndexingUtils::CalculateMillerIndices(
        tempUB, q_vectors, tolerance, miller_indices, original_error);

    IndexingUtils::RoundHKLs(miller_indices); // HKLs must be rounded for
                                              // Optimize_UB to work
    num_indexed = original_indexed;
    average_error = original_error;

    bool done = false;
    if (num_indexed < 3) // can't optimize without at least 3
    {                    // peaks
      done = true;
    }

    int iteration = 0;
    while (iteration < 4 && !done) // try repeatedly optimizing 4 times
    {                              // which is usually sufficient
      try {
        IndexingUtils::Optimize_UB(tempUB, miller_indices, q_vectors);
      } catch (...) // If there is any problem, such as too few
      {             // independent peaks, just use the original UB
        tempUB = UB;
        done = true;
      }

      num_indexed = IndexingUtils::CalculateMillerIndices(
          tempUB, q_vectors, tolerance, miller_indices, average_error);

      IndexingUtils::RoundHKLs(miller_indices); // HKLs must be rounded for
                                                // Optimize_UB to work

      if (num_indexed < original_indexed) // just use the original UB
      {
        num_indexed = original_indexed;
        average_error = original_error;
        done = true;
      }

      iteration++;
    }

    if (!round_hkls) // If user wants fractional hkls, recalculate them
    {
      num_indexed = IndexingUtils::CalculateMillerIndices(
          tempUB, q_vectors, tolerance, miller_indices, average_error);
    }

    total_indexed += num_indexed;
    total_error = average_error * num_indexed;

    // tell the user how many were indexed in each run
    if (run_numbers.size() > 1) {
      g_log.notice() << "Run " << run << ": indexed " << num_indexed
                     << " Peaks out of " << q_vectors.size()
                     << " with tolerance of " << tolerance << std::endl;
      g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                     << average_error << std::endl;
    }

    size_t miller_index_counter = 0;
    for (size_t i = 0; i < n_peaks; i++) {
      if (peaks[i].getRunNumber() == run) {
        peaks[i].setHKL(miller_indices[miller_index_counter]);
        miller_index_counter++;
      }
    }
  }

  if (total_indexed > 0)
    average_error = total_error / total_indexed;
  else
    average_error = 0;

  // tell the user how many were indexed overall and the overall average error
  g_log.notice() << "ALL Runs: indexed " << total_indexed << " Peaks out of "
                 << n_peaks << " with tolerance of " << tolerance << std::endl;
  g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                 << average_error << std::endl;

  // Save output properties
  this->setProperty("NumIndexed", total_indexed);
  this->setProperty("AverageError", average_error);
  // Show the lattice parameters
  g_log.notice() << o_lattice << "\n";
}

} // namespace Mantid
} // namespace Crystal
