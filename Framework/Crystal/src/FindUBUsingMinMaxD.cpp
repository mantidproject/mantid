// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindUBUsingMinMaxD.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingMinMaxD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Constructor
 */
FindUBUsingMinMaxD::FindUBUsingMinMaxD() {
  useAlgorithm("FindUBUsingFFT");
  deprecatedDate("2013-06-03");
}

const std::string FindUBUsingMinMaxD::name() const {
  return "FindUBUsingMinMaxD";
}

int FindUBUsingMinMaxD::version() const { return 1; }

const std::string FindUBUsingMinMaxD::category() const {
  return "Crystal\\UBMatrix";
}

/** Initialize the algorithm's properties.
 */
void FindUBUsingMinMaxD::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  auto atLeast3Int = boost::make_shared<BoundedValidator<int>>();
  atLeast3Int->setLower(3);

  // use negative values, force user to input all parameters
  this->declareProperty(std::make_unique<PropertyWithValue<double>>(
                            "MinD", -1.0, mustBePositive, Direction::Input),
                        "Lower Bound on Lattice Parameters a, b, c");

  this->declareProperty(std::make_unique<PropertyWithValue<double>>(
                            "MaxD", -1.0, mustBePositive, Direction::Input),
                        "Upper Bound on Lattice Parameters a, b, c");

  this->declareProperty(std::make_unique<PropertyWithValue<int>>(
                            "NumInitial", 20, atLeast3Int, Direction::Input),
                        "Number of Peaks to Use on First Pass(20)");

  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "Tolerance", 0.15, mustBePositive, Direction::Input),
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

  std::cout << "Error = " << error << '\n';
  std::cout << "UB = " << UB << '\n';

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

    // Show the modified lattice parameters
    g_log.notice() << o_lattice << "\n";

    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}

} // namespace Crystal
} // namespace Mantid
