#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidAPI/Sample.h"
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

/** Initialize the algorithm's properties.
 */
void FindUBUsingIndexedPeaks::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "Input Peaks Workspace");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Tolerance", 0.1, mustBePositive, "Indexing Tolerance (0.1)");
  declareProperty("ToleranceForSatellite", 0.1, mustBePositive,
                  "Indexing Tolerance for satellite (0.1)");
}

/** Execute the algorithm.
 */
void FindUBUsingIndexedPeaks::exec() {
  PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  std::vector<Peak> peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();

  std::vector<V3D> q_vectors;
  std::vector<V3D> hkl_vectors;
  std::vector<V3D> mnp_vectors;

  double tolerance = getProperty("Tolerance");

  int ModDim = 0;
  int MaxOrder = 0;
  bool CrossTerm = false;
  V3D errorHKL1, errorHKL2, errorHKL3;

  q_vectors.reserve(n_peaks);
  hkl_vectors.reserve(n_peaks);
  mnp_vectors.reserve(n_peaks);

  size_t indexed_count = 0;
  std::unordered_set<int> run_numbers;
  for (Peak peak : peaks) {

    run_numbers.insert(peak.getRunNumber());
    V3D hkl(peak.getIntHKL()); // ##### KEEP
    V3D mnp(peak.getIntMNP());

    if (abs(static_cast<int>(mnp[0])) > MaxOrder)
      MaxOrder = abs(static_cast<int>(mnp[0]));
    if (abs(static_cast<int>(mnp[1])) > MaxOrder)
      MaxOrder = abs(static_cast<int>(mnp[1]));
    if (abs(static_cast<int>(mnp[2])) > MaxOrder)
      MaxOrder = abs(static_cast<int>(mnp[2]));

    if (mnp[0] != 0 && ModDim == 0)
      ModDim = 1;
    if (mnp[1] != 0 && ModDim == 1)
      ModDim = 2;
    if (mnp[2] != 0 && ModDim == 2)
      ModDim = 3;
    if (mnp[0] * mnp[1] != 0 || mnp[1] * mnp[2] != 0 || mnp[2] * mnp[0] != 0)
      CrossTerm = true;

    if (IndexingUtils::ValidIndex(hkl, 1.0) ||
        IndexingUtils::ValidIndex(mnp, 1.0)) // use tolerance == 1 to
                                             // just check for (0,0,0,0,0,0)
    {
      q_vectors.push_back(peak.getQSampleFrame());
      hkl_vectors.emplace_back(hkl[0], hkl[1], hkl[2]);
      mnp_vectors.emplace_back(mnp[0], mnp[1], mnp[2]);
      indexed_count++;
    }
  }

  if (indexed_count < 3) {
    throw std::runtime_error(
        "At least three linearly independent indexed peaks are needed.");
  }

  Matrix<double> UB(3, 3, false);
  Matrix<double> modUB(3, 3, false);
  std::vector<double> sigabc(7);
  std::vector<double> sigq(3);

  IndexingUtils::Optimize_6dUB(UB, modUB, hkl_vectors, mnp_vectors, ModDim,
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
    for (Peak peak : peaks) {
      q_vectors.push_back(peak.getQSampleFrame());
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
        double average_error = 0.;

        for (Peak peak : peaks)
          if (peak.getRunNumber() == run) {
            V3D hkl(peak.getIntHKL()); // ##### KEEP
            V3D mnp(peak.getIntMNP());
            if (IndexingUtils::ValidIndex(hkl, 1.0) ||
                IndexingUtils::ValidIndex(mnp, 1.0))
              run_indexed++;
          }

        g_log.notice() << "Number of Indexed Peaks in Run " << run << " is "
                       << run_indexed << "\n";

        run_q_vectors.reserve(run_indexed);
        run_hkl_vectors.reserve(run_indexed);
        run_mnp_vectors.reserve(run_indexed);
        run_fhkl_vectors.reserve(run_indexed);

        for (Peak peak : peaks) {
          if (peak.getRunNumber() == run) {
            V3D hkl(peak.getIntHKL()); // ##### KEEP
            V3D mnp(peak.getIntMNP());
            if (IndexingUtils::ValidIndex(hkl, 1.0) ||
                IndexingUtils::ValidIndex(mnp, 1.0)) {
              run_q_vectors.push_back(peak.getQSampleFrame());
              run_hkl_vectors.emplace_back(hkl[0], hkl[1], hkl[2]);
              run_mnp_vectors.emplace_back(mnp[0], mnp[1], mnp[2]);
            }
          }
        }

        if (run_indexed < 3)
          continue;

        std::vector<double> rsigabc(7);
        IndexingUtils::Optimize_6dUB(UB, modUB, run_hkl_vectors,
                                     run_mnp_vectors, ModDim, run_q_vectors,
                                     rsigabc, sigq);
        OrientedLattice run_lattice;
        run_lattice.setUB(UB);
        run_lattice.setModUB(modUB);
        run_lattice.setError(rsigabc[0], rsigabc[1], rsigabc[2], rsigabc[3],
                             rsigabc[4], rsigabc[5]);
        g_log.notice() << run_lattice << "\n";

        if (!CrossTerm) {
          IndexingUtils::CalculateMillerIndices(
              UB, run_q_vectors, 1.0, run_fhkl_vectors, average_error);
          for (size_t i = 0; i < run_indexed; i++) {
            if (IndexingUtils::ValidIndex(run_fhkl_vectors[i], tolerance))
              continue;

            V3D fhkl(run_fhkl_vectors[i]);
            if (run_mnp_vectors[i][0] != 0) {
              fhkl -= run_lattice.getModVec(1) * run_mnp_vectors[i][0];
              if (IndexingUtils::ValidIndex(fhkl, satetolerance)) {
                sate_indexed++;
                V3D errhkl = fhkl - run_hkl_vectors[i];
                errhkl = V3D(fabs(errhkl[0]), fabs(errhkl[1]), fabs(errhkl[2]));
                errorHKL1 += errhkl;
              }
            } else if (run_mnp_vectors[i][1] != 0) {
              fhkl -= run_lattice.getModVec(2) * run_mnp_vectors[i][1];
              if (IndexingUtils::ValidIndex(fhkl, satetolerance)) {
                sate_indexed++;
                V3D errhkl = fhkl - run_hkl_vectors[i];
                errhkl = V3D(fabs(errhkl[0]), fabs(errhkl[1]), fabs(errhkl[2]));
                errorHKL2 += errhkl;
              }
            } else if (run_mnp_vectors[i][2] != 0) {
              fhkl -= run_lattice.getModVec(3) * run_mnp_vectors[i][2];
              if (IndexingUtils::ValidIndex(fhkl, satetolerance)) {
                sate_indexed++;
                V3D errhkl = fhkl - run_hkl_vectors[i];
                errhkl = V3D(fabs(errhkl[0]), fabs(errhkl[1]), fabs(errhkl[2]));
                errorHKL3 += errhkl;
              }
            }
          }
        } else {
          V3D err_sigq =
              V3D(sigq[0], sigq[1], sigq[2]) * static_cast<double>(num_indexed);
          errorHKL1 += err_sigq;
          errorHKL2 += err_sigq;
          errorHKL3 += err_sigq;
        }
      }
    }

    std::stringstream stream;
    stream << "New UB will index " << num_indexed
           << " main Peaks with tolerance " << tolerance << " and "
           << sate_indexed << " Satellite Peaks with tolerance "
           << satetolerance << " ,out of " << n_peaks << " Peaks \n";
    g_log.notice(stream.str());

    OrientedLattice o_lattice;
    o_lattice.setUB(UB);
    o_lattice.setModUB(modUB);
    o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                       sigabc[5]);
    double ind_count_inv = 1. / static_cast<double>(indexed_count);
    o_lattice.setErrorModHKL(
        errorHKL1[0] * ind_count_inv, errorHKL1[1] * ind_count_inv,
        errorHKL1[2] * ind_count_inv, errorHKL2[0] * ind_count_inv,
        errorHKL2[1] * ind_count_inv, errorHKL2[2] * ind_count_inv,
        errorHKL3[0] * ind_count_inv, errorHKL3[1] * ind_count_inv,
        errorHKL3[2] * ind_count_inv);

    o_lattice.setMaxOrder(MaxOrder);
    o_lattice.setCrossTerm(CrossTerm);

    // Show the modified lattice parameters
    logLattice(o_lattice, ModDim);

    ws->mutableSample().setOrientedLattice(&o_lattice);
  }
}
void FindUBUsingIndexedPeaks::logLattice(OrientedLattice &o_lattice,
                                         int &ModDim) {
  // Show the modified lattice parameters
  g_log.notice() << o_lattice << "\n";
  g_log.notice() << "Modulation Dimension is: " << ModDim << "\n";
  for (int i = 1; i < ModDim + 1; i++) {
    g_log.notice() << "Modulation Vector " << i << ": "
                   << o_lattice.getModVec(i) << "\n";
    g_log.notice() << "Modulation Vector " << i
                   << " error: " << o_lattice.getVecErr(i) << "\n";
  }
}
} // namespace Crystal
} // namespace Mantid
