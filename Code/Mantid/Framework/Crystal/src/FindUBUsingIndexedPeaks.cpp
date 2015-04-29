#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingIndexedPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

#define round(x) ((x) >= 0 ? (int)((x)+0.5) : (int)((x)-0.5))

//--------------------------------------------------------------------------
/** Constructor
 */
FindUBUsingIndexedPeaks::FindUBUsingIndexedPeaks() {}

//--------------------------------------------------------------------------
/** Destructor
 */
FindUBUsingIndexedPeaks::~FindUBUsingIndexedPeaks() {}

//--------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindUBUsingIndexedPeaks::init() {
  this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");
  boost::shared_ptr<BoundedValidator<double>> mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);
  this->declareProperty("Tolerance", 0.1, mustBePositive,
                        "Indexing Tolerance (0.1)");
}

//--------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindUBUsingIndexedPeaks::exec() {
  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  std::vector<Peak> peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  std::vector<V3D> hkl_vectors;

  q_vectors.reserve(n_peaks);
  hkl_vectors.reserve(n_peaks);

  size_t indexed_count = 0;
  for (size_t i = 0; i < n_peaks; i++) {
    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL()); // ##### KEEP
    if (IndexingUtils::ValidIndex(hkl, 1.0)) // use tolerance == 1 to
                                             // just check for (0,0,0)
    {
      q_vectors.push_back(peaks[i].getQSampleFrame());
      V3D miller_ind(round(hkl[0]), round(hkl[1]), round(hkl[2]));
      hkl_vectors.push_back(V3D(miller_ind));
      indexed_count++;
    }
  }

  if (indexed_count < 3) {
    throw std::runtime_error(
        "At least three linearly independent indexed peaks are needed.");
  }

  Matrix<double> UB(3, 3, false);
  std::vector<double> sigabc(7);
  IndexingUtils::Optimize_UB(UB, hkl_vectors, q_vectors, sigabc);

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    g_log.notice(std::string(
        "Found Invalid UB...peaks used might not be linearly independent"));
    g_log.notice(std::string("UB NOT SAVED."));
  } else               // tell user how many would be indexed
  {                    // from the full list of peaks, and
    q_vectors.clear(); // save the UB in the sample
    q_vectors.reserve(n_peaks);
    for (size_t i = 0; i < n_peaks; i++) {
      q_vectors.push_back(peaks[i].getQSampleFrame());
    }
    double tolerance = this->getProperty("Tolerance");
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);

    char logInfo[200];
    sprintf(logInfo,
            std::string(
                "New UB will index %1d Peaks out of %1d with tolerance %5.3f")
                .c_str(),
            num_indexed, n_peaks, tolerance);
    g_log.notice(std::string(logInfo));

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
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
    g_log.notice() << std::fixed << std::setprecision(3) << std::setw(9);

    g_log.notice() << "Parameter Errors   :" << std::fixed
                   << std::setprecision(3) << std::setw(9) << sigabc[0]
                   << std::fixed << std::setprecision(3) << std::setw(9)
                   << sigabc[1] << std::fixed << std::setprecision(3)
                   << std::setw(9) << sigabc[2] << std::fixed
                   << std::setprecision(3) << std::setw(9) << sigabc[3]
                   << std::fixed << std::setprecision(3) << std::setw(9)
                   << sigabc[4] << std::fixed << std::setprecision(3)
                   << std::setw(9) << sigabc[5] << std::endl;
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);
    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}

} // namespace Mantid
} // namespace Crystal
