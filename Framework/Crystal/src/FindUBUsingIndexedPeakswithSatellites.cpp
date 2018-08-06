//
//
//  FindUBUsingIndexedPeakswithSatellites.cpp
//
//
//  Created by Shiyun Jin on 7/16/18.
//
//

#include "MantidCrystal/FindUBUsingIndexedPeakswithSatellites.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/Sample.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingIndexedPeakswithSatellites)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void FindUBUsingIndexedPeakswithSatellites::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "Input Peaks Workspace");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Tolerance", 0.1, mustBePositive, "Indexing Tolerance (0.1)");
}

/** Execute the algorithm.
 */
void FindUBUsingIndexedPeakswithSatellites::exec() {
  PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  std::vector<Peak> peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  std::vector<V3D> hkl_vectors;
  std::vector<V3D> mnp_vectors;

  int ModDim = 0;

  q_vectors.reserve(n_peaks);
  hkl_vectors.reserve(n_peaks);
  mnp_vectors.reserve(n_peaks);

  size_t indexed_count = 0;
  for (size_t i = 0; i < n_peaks; i++) {
    V3D hkl(peaks[i].getIntHKL()); // ##### KEEP
    V3D mnp(peaks[i].getIntMNP());
    if (mnp[0] != 0 && ModDim == 0)
      ModDim = 1;
    if (mnp[1] != 0 && ModDim == 1)
      ModDim = 2;
    if (mnp[2] != 0 && ModDim == 2)
      ModDim = 3;

    if (IndexingUtils::ValidIndex(hkl, 1.0) ||
        IndexingUtils::ValidIndex(mnp, 1.0)) // use tolerance == 1 to
    // just check for (0,0,0,0,0,0)
    {
      q_vectors.push_back(peaks[i].getQSampleFrame());
      hkl_vectors.emplace_back(hkl);
      mnp_vectors.emplace_back(mnp);
      indexed_count++;
    }
  }

  if (ModDim == 0) {
    throw std::runtime_error("No satellite Peaks indexed, run "
                             "IndexPeakswithSatellites or use "
                             "FindUBUsingIndexedPeaks");
  }

  if (indexed_count < 4) {
    throw std::runtime_error(
        "At least four linearly independent indexed peaks are needed.");
  }

  Matrix<double> UB(3, 3, false);
  Matrix<double> ModUB(3, 3, false);
  std::vector<double> sigabc(7);
  std::vector<double> sigq(9);

  IndexingUtils::Optimize_6dUB(UB, ModUB, hkl_vectors, mnp_vectors, ModDim,
                               q_vectors, sigabc, sigq);

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
    double tolerance = getProperty("Tolerance");
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);

    char logInfo[200];
    sprintf(
        logInfo,
        std::string(
            "New UB will index %1d main Peaks out of %1d with tolerance %5.3f")
            .c_str(),
        num_indexed, n_peaks, tolerance);
    g_log.notice(std::string(logInfo));

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
    o_lattice.setModUB(ModUB);
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);
    o_lattice.setErrorModHKL(sigq[0], sigq[1], sigq[2], sigq[3], sigq[4],
                             sigq[5], sigq[6], sigq[7], sigq[8]);

    // Show the modified lattice parameters
    g_log.notice() << o_lattice << "\n";
    g_log.notice() << "Modulation Dimension is: " << ModDim << "\n";

    if (ModDim == 1) {
      g_log.notice() << "Modulation Vector 1: " << o_lattice.getModVec(1)
                     << "\n";
      g_log.notice() << "Modulation Vector 1 error: " << o_lattice.getVecErr(1)
                     << "\n";
    }
    if (ModDim == 2) {
      g_log.notice() << "Modulation Vector 1: " << o_lattice.getModVec(1)
                     << "\n";
      g_log.notice() << "Modulation Vector 1 error: " << o_lattice.getVecErr(1)
                     << "\n";
      g_log.notice() << "Modulation Vector 2: " << o_lattice.getModVec(2)
                     << "\n";
      g_log.notice() << "Modulation Vector 2 error: " << o_lattice.getVecErr(2)
                     << "\n";
    }
    if (ModDim == 3) {
      g_log.notice() << "Modulation Vector 1: " << o_lattice.getModVec(1)
                     << "\n";
      g_log.notice() << "Modulation Vector 1 error: " << o_lattice.getVecErr(1)
                     << "\n";
      g_log.notice() << "Modulation Vector 2: " << o_lattice.getModVec(2)
                     << "\n";
      g_log.notice() << "Modulation Vector 2 error: " << o_lattice.getVecErr(2)
                     << "\n";
      g_log.notice() << "Modulation Vector 3: " << o_lattice.getModVec(3)
                     << "\n";
      g_log.notice() << "Modulation Vector 3 error: " << o_lattice.getVecErr(3)
                     << "\n";
    }

    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}
} // namespace Mantid
} // namespace Crystal
