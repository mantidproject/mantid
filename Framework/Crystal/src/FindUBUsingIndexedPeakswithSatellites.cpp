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
  for (const auto peak : peaks) {
    V3D hkl(peak.getIntHKL()); // ##### KEEP
    V3D mnp(peak.getIntMNP());
    ModDim = std::max(ModDim, getModulationDimension(mnp));

    if (IndexingUtils::ValidIndex(hkl, 1.0) ||
        IndexingUtils::ValidIndex(mnp, 1.0)) // use tolerance == 1 to
    // just check for (0,0,0,0,0,0)
    {
      q_vectors.push_back(peak.getQSampleFrame());
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
  Matrix<double> modUB(3, 3, false);
  std::vector<double> sigabc(7);
  std::vector<double> sigq(9);

  IndexingUtils::Optimize_6dUB(UB, modUB, hkl_vectors, mnp_vectors, ModDim,
                               q_vectors, sigabc, sigq);

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    throw std::runtime_error(
        "Found Invalid UB...peaks used might not be linearly independent");
  } else               // tell user how many would be indexed
  {                    // from the full list of peaks, and
    q_vectors.clear(); // save the UB in the sample
    q_vectors.reserve(n_peaks);
    for (size_t i = 0; i < n_peaks; i++) {
      q_vectors.push_back(peaks[i].getQSampleFrame());
    }
    double tolerance = getProperty("Tolerance");
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);

    std::stringstream stream;
    stream << "New UB will index " << num_indexed << " main Peaks out of " <<
      n_peaks << " with tolerance " << tolerance << "\n";
    g_log.notice(stream.str());

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
    o_lattice.setModUB(modUB);
    o_lattice.setError(sigabc);
    o_lattice.setErrorModHKL(sigq);

    logLattice(o_lattice, ModDim);

    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}
  void FindUBUsingIndexedPeakswithSatellites::logLattice(OrientedLattice &o_lattice, int &ModDim)
  {
    // Show the modified lattice parameters
    g_log.notice() << o_lattice << "\n";
    g_log.notice() << "Modulation Dimension is: " << ModDim << "\n";
    for (int i = 0; i < ModDim; i++) {
      g_log.notice() << "Modulation Vector 1: " << o_lattice.getModVec(i)
                     << "\n";
      g_log.notice() << "Modulation Vector 1 error: " << o_lattice.getVecErr(i)
                     << "\n";
  }
}
  int FindUBUsingIndexedPeakswithSatellites::getModulationDimension(V3D &mnp)
  {
    int ModDim = 0;
    if (mnp[0] != 0 && ModDim == 0)
      ModDim = 1;
    if (mnp[1] != 0 && ModDim == 1)
      ModDim = 2;
    if (mnp[2] != 0 && ModDim == 2)
      ModDim = 3;
    return ModDim;
  }
} // namespace Mantid
} // namespace Crystal
