// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindUBUsingIndexedPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

const int MIN_INDEXED_PEAKS = 3;
/** Initialize the algorithm's properties.
 */
void FindUBUsingIndexedPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                  "Input Peaks Workspace");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Tolerance", 0.1, mustBePositive, "Indexing Tolerance (0.1)");
  declareProperty("ToleranceForSatellite", 0.1, mustBePositive, "Indexing Tolerance for satellite (0.1)");
  declareProperty("CommonUBForAll", false, "Used when evaluating the uncertainty of modHKL");
}

/** Execute the algorithm.
 */
void FindUBUsingIndexedPeaks::exec() {
  IPeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  const int n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  std::vector<V3D> hkl_vectors;
  std::vector<V3D> mnp_vectors;

  double tolerance = getProperty("Tolerance");
  bool commonUB = getProperty("CommonUBForAll");

  int ModDim = 0;
  int MaxOrder = 0;
  bool CrossTerm = false;
  DblMatrix errorHKL(3, 3);

  q_vectors.reserve(n_peaks);
  hkl_vectors.reserve(n_peaks);
  mnp_vectors.reserve(n_peaks);

  size_t indexed_count = 0;
  std::unordered_set<int> run_numbers;
  for (int i = 0; i < n_peaks; i++) {
    const IPeak &peak = ws->getPeak(i);
    run_numbers.insert(peak.getRunNumber());
    V3D hkl(peak.getIntHKL());
    V3D mnp(peak.getIntMNP());
    int maxOrder = mnp.maxCoeff();
    if (maxOrder > MaxOrder)
      MaxOrder = maxOrder;

    if (mnp[0] != 0 && ModDim == 0)
      ModDim = 1;
    if (mnp[1] != 0 && ModDim == 1)
      ModDim = 2;
    if (mnp[2] != 0 && ModDim == 2)
      ModDim = 3;
    if (mnp[0] * mnp[1] != 0 || mnp[1] * mnp[2] != 0 || mnp[2] * mnp[0] != 0)
      CrossTerm = true;

    if (isPeakIndexed(peak)) {
      q_vectors.emplace_back(peak.getQSampleFrame());
      hkl_vectors.emplace_back(hkl);
      mnp_vectors.emplace_back(mnp);
      indexed_count++;
    }
  }

  // too few indexed peaks to work with
  if (indexed_count < MIN_INDEXED_PEAKS) {
    throw std::runtime_error("At least three linearly independent indexed peaks are needed.");
  }

  Matrix<double> UB(3, 3, false);
  Matrix<double> modUB(3, 3, false);
  std::vector<double> sigabc(7);
  std::vector<double> sigq(3);

  IndexingUtils::Optimize_6dUB(UB, modUB, hkl_vectors, mnp_vectors, ModDim, q_vectors, sigabc, sigq);

  if (!IndexingUtils::CheckUB(UB)) // UB not found correctly
  {
    g_log.notice(std::string("Found Invalid UB...peaks used might not be linearly independent"));
    g_log.notice(std::string("UB NOT SAVED."));
  } else               // tell user how many would be indexed
  {                    // from the full list of peaks, and
    q_vectors.clear(); // save the UB in the sample
    q_vectors.reserve(n_peaks);

    for (int i = 0; i < n_peaks; i++) {
      q_vectors.emplace_back(ws->getPeak(i).getQSampleFrame());
    }

    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
    int sate_indexed = 0;
    double satetolerance = getProperty("ToleranceForSatellite");

    if (ModDim > 0) {
      for (int run : run_numbers) {
        std::vector<V3D> run_q_vectors;
        std::vector<V3D> run_fhkl_vectors;
        std::vector<V3D> run_hkl_vectors;
        std::vector<V3D> run_mnp_vectors;
        size_t run_indexed = 0;

        for (int i = 0; i < n_peaks; i++) {
          const IPeak &peak = ws->getPeak(i);
          if (peak.getRunNumber() == run) {
            if (isPeakIndexed(peak))
              run_indexed++;
          }
        }

        g_log.notice() << "Number of Indexed Peaks in Run " << run << " is " << run_indexed << "\n";

        run_q_vectors.reserve(run_indexed);
        run_hkl_vectors.reserve(run_indexed);
        run_mnp_vectors.reserve(run_indexed);
        run_fhkl_vectors.reserve(run_indexed);

        for (int i = 0; i < n_peaks; i++) {
          const IPeak &peak = ws->getPeak(i);
          if (peak.getRunNumber() == run) {
            V3D hkl(peak.getIntHKL());
            V3D mnp(peak.getIntMNP());
            if (isPeakIndexed(peak)) {
              run_q_vectors.emplace_back(peak.getQSampleFrame());
              run_hkl_vectors.emplace_back(hkl);
              run_mnp_vectors.emplace_back(mnp);
            }
          }
        }

        if (run_indexed < 3)
          continue;

        OrientedLattice run_lattice;

        if (commonUB) {
          run_lattice.setUB(UB);
          run_lattice.setModUB(modUB);
          run_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);
        } else {
          Matrix<double> run_UB(3, 3, false);
          Matrix<double> run_modUB(3, 3, false);
          std::vector<double> run_sigabc(7);
          std::vector<double> run_sigq(3);
          IndexingUtils::Optimize_6dUB(run_UB, run_modUB, run_hkl_vectors, run_mnp_vectors, ModDim, run_q_vectors,
                                       run_sigabc, run_sigq);
          run_lattice.setUB(run_UB);
          run_lattice.setModUB(run_modUB);
          run_lattice.setError(run_sigabc[0], run_sigabc[1], run_sigabc[2], run_sigabc[3], run_sigabc[4],
                               run_sigabc[5]);
        }
        g_log.notice() << run_lattice << "\n";

        double average_error = 0.;
        IndexingUtils::CalculateMillerIndices(run_lattice.getUB(), run_q_vectors, 1.0, run_fhkl_vectors, average_error);
        for (size_t i = 0; i < run_indexed; i++) {
          if (IndexingUtils::ValidIndex(run_fhkl_vectors[i], tolerance))
            continue;

          V3D fhkl(run_fhkl_vectors[i]);
          for (int j = 0; j < 3; j++) {
            if (run_mnp_vectors[i][j] != 0) {
              fhkl -= run_lattice.getModVec(j) * run_mnp_vectors[i][j];
              if (IndexingUtils::ValidIndex(fhkl, satetolerance)) {
                sate_indexed++;
                V3D errhkl = fhkl - run_hkl_vectors[i];
                errhkl = errhkl.absoluteValue();
                for (int k = 0; k < 3; k++)
                  errorHKL[k][j] += errhkl[k];
              }
            }
          }
        }
      }
    }

    std::stringstream stream;
    stream << "New UB will index " << num_indexed << " main Peaks with tolerance " << tolerance << " and "
           << sate_indexed << " Satellite Peaks with tolerance " << satetolerance << " ,out of " << n_peaks
           << " Peaks \n";
    g_log.notice(stream.str());

    auto o_lattice = std::make_unique<OrientedLattice>();
    o_lattice->setUB(UB);
    o_lattice->setModUB(modUB);
    o_lattice->setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);
    double ind_count_inv = 1. / static_cast<double>(indexed_count);
    errorHKL *= ind_count_inv;
    o_lattice->setErrorModHKL(errorHKL);

    o_lattice->setMaxOrder(MaxOrder);
    o_lattice->setCrossTerm(CrossTerm);

    // Show the modified lattice parameters
    logLattice(*o_lattice, ModDim);

    ws->mutableSample().setOrientedLattice(std::move(o_lattice));
  }
}
void FindUBUsingIndexedPeaks::logLattice(const OrientedLattice &o_lattice, const int &ModDim) {
  // Show the modified lattice parameters
  g_log.notice() << o_lattice << "\n";
  g_log.notice() << "Modulation Dimension is: " << ModDim << "\n";
  for (int i = 0; i < ModDim; i++) {
    g_log.notice() << "Modulation Vector " << i + 1 << ": " << o_lattice.getModVec(i) << "\n";
    g_log.notice() << "Modulation Vector " << i + 1 << " error: " << o_lattice.getVecErr(i) << "\n";
  }
}
bool FindUBUsingIndexedPeaks::isPeakIndexed(const IPeak &peak) {
  const V3D hkl(peak.getIntHKL());
  const V3D mnp(peak.getIntMNP());
  return (IndexingUtils::ValidIndex(hkl, 1.0) || IndexingUtils::ValidIndex(mnp, 1.0));
}
} // namespace Mantid::Crystal
