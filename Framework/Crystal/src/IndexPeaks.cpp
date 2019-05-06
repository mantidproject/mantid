// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/IndexPeaks.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IndexPeaks)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void IndexPeaks::init() {
  this->declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  this->declareProperty(
      make_unique<PropertyWithValue<double>>("Tolerance", 0.15, mustBePositive,
                                             Direction::Input),
      "Indexing Tolerance (0.15)");

  this->declareProperty(
      make_unique<PropertyWithValue<double>>("ToleranceForSatellite", 0.15,
                                             mustBePositive, Direction::Input),
      "Satellite Indexing Tolerance (0.15)");

  this->declareProperty("RoundHKLs", true,
                        "Round H, K and L values to integers");

  this->declareProperty("CommonUBForAll", false,
                        "Index all orientations with a common UB");

  this->declareProperty(
      make_unique<PropertyWithValue<int>>("NumIndexed", 0, Direction::Output),
      "Gets set with the number of indexed peaks.");

  this->declareProperty(make_unique<PropertyWithValue<double>>(
                            "AverageError", 0.0, Direction::Output),
                        "Gets set with the average HKL indexing error.");

  this->declareProperty(make_unique<PropertyWithValue<int>>(
                            "TotalNumIndexed", 0, Direction::Output),
                        "Gets set with the number of Total indexed peaks.");

  this->declareProperty(make_unique<PropertyWithValue<int>>("MainNumIndexed", 0,
                                                            Direction::Output),
                        "Gets set with the number of indexed main peaks.");

  this->declareProperty(make_unique<PropertyWithValue<int>>("SateNumIndexed", 0,
                                                            Direction::Output),
                        "Gets set with the number of indexed main peaks.");

  this->declareProperty(
      make_unique<PropertyWithValue<double>>("MainError", 0.0,
                                             Direction::Output),
      "Gets set with the average HKL indexing error of Main Peaks.");

  this->declareProperty(
      make_unique<PropertyWithValue<double>>("SatelliteError", 0.0,
                                             Direction::Output),
      "Gets set with the average HKL indexing error of Satellite Peaks.");
}

/** Execute the algorithm.
 */
void IndexPeaks::exec() {
  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
  const Matrix<double> &UB = o_lattice.getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error(
        "ERROR: The stored UB is not a valid orientation matrix");
  }

  bool round_hkls = this->getProperty("RoundHKLs");
  bool commonUB = this->getProperty("CommonUBForAll");

  std::vector<Peak> &peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();
  int total_indexed = 0;
  int total_main = 0;
  int total_sate = 0;
  double average_error;
  double average_main_error;
  double average_sate_error;
  double tolerance = this->getProperty("Tolerance");

  if (commonUB && o_lattice.getModVec(0) == V3D(0, 0, 0)) {
    std::vector<V3D> miller_indices;
    std::vector<V3D> q_vectors;

    q_vectors.reserve(n_peaks);
    for (const auto &peak : peaks) {
      q_vectors.push_back(peak.getQSampleFrame());
    }

    total_indexed = IndexingUtils::CalculateMillerIndices(
        UB, q_vectors, tolerance, miller_indices, average_error);

    for (size_t i = 0; i < n_peaks; i++) {
      peaks[i].setHKL(miller_indices[i]);
      peaks[i].setIntHKL(miller_indices[i]);
      peaks[i].setIntMNP(V3D(0, 0, 0));
    }
  } else {
    double total_error = 0;
    double total_main_error = 0;
    double total_sate_error = 0;
    double satetolerance = this->getProperty("ToleranceForSatellite");

    // get list of run numbers in this peaks workspace
    std::vector<int> run_numbers;
    for (const auto &peak : peaks) {
      int run = peak.getRunNumber();
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

    for (const int run : run_numbers) {
      std::vector<V3D> miller_indices;
      std::vector<V3D> q_vectors;

      for (const auto &peak : peaks) {
        if (peak.getRunNumber() == run)
          q_vectors.push_back(peak.getQSampleFrame());
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

      g_log.notice() << "Maximum Order: " << o_lattice.getMaxOrder() << '\n';

      if (o_lattice.getMaxOrder() ==
          0) // If data not modulated, recalculate fractional HKL
      {
        if (!round_hkls) // If user wants fractional hkls, recalculate them
        {
          num_indexed = IndexingUtils::CalculateMillerIndices(
              tempUB, q_vectors, tolerance, miller_indices, average_error);
        }
        total_indexed += num_indexed;
        total_error += average_error * num_indexed;

        // tell the user how many were indexed in each run
        if (run_numbers.size() > 1) {
          g_log.notice() << "Run " << run << ": indexed " << num_indexed
                         << " Peaks out of " << q_vectors.size()
                         << " with tolerance of " << tolerance << '\n';
          g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                         << average_error << '\n';
        }

        size_t miller_index_counter = 0;
        for (auto &peak : peaks) {
          if (peak.getRunNumber() == run) {
            peak.setHKL(miller_indices[miller_index_counter]);
            peak.setIntHKL(miller_indices[miller_index_counter]);
            peak.setIntMNP(V3D(0, 0, 0));
            miller_index_counter++;
          }
        }
      } else {
        int ModDim = 0;
        int main_indexed = 0;
        int sate_indexed = 0;
        double main_error = 0;
        double sate_error = 0;
        int maxOrder = o_lattice.getMaxOrder();
        bool crossTerm = o_lattice.getCrossTerm();
        V3D offsets1 = o_lattice.getModVec(0);
        V3D offsets2 = o_lattice.getModVec(1);
        V3D offsets3 = o_lattice.getModVec(2);

        if (offsets1 == V3D(0, 0, 0))
          throw std::runtime_error("Invalid Modulation Vector");
        else if (offsets2 == V3D(0, 0, 0))
          ModDim = 1;
        else if (offsets3 == V3D(0, 0, 0))
          ModDim = 2;
        else
          ModDim = 3;

        IndexingUtils::CalculateMillerIndices(tempUB, q_vectors, 1.0,
                                              miller_indices, average_error);

        // Index satellite peaks
        size_t miller_index_counter = 0;
        for (auto &peak : peaks) {
          if (peak.getRunNumber() == run) {
            peak.setHKL(miller_indices[miller_index_counter]);
            miller_index_counter++;

            auto hkl = peak.getHKL();
            bool suc_indexed = false;

            if (IndexingUtils::ValidIndex(hkl, tolerance)) {
              peak.setIntHKL(hkl);
              peak.setIntMNP(V3D(0, 0, 0));
              suc_indexed = true;
              main_indexed++;
              main_error += hkl.hklError();
            } else if (!crossTerm) {
              if (ModDim > 0) {
                for (int order = -maxOrder; order <= maxOrder; order++) {
                  if (order == 0)
                    continue; // exclude order 0
                  V3D hkl1(hkl);
                  hkl1 -= offsets1 * order;
                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                    peak.setIntHKL(hkl1);
                    peak.setIntMNP(V3D(order, 0, 0));
                    suc_indexed = true;
                    sate_indexed++;
                    sate_error += hkl1.hklError();
                  }
                }
              }
              if (ModDim > 1) {
                for (int order = -maxOrder; order <= maxOrder; order++) {
                  if (order == 0)
                    continue; // exclude order 0
                  V3D hkl1(hkl);
                  hkl1 -= offsets2 * order;
                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                    peak.setIntHKL(hkl1);
                    peak.setIntMNP(V3D(0, order, 0));
                    suc_indexed = true;
                    sate_indexed++;
                    sate_error += hkl1.hklError();
                  }
                }
              }
              if (ModDim > 2) {
                for (int order = -maxOrder; order <= maxOrder; order++) {
                  if (order == 0)
                    continue; // exclude order 0
                  V3D hkl1(hkl);
                  hkl1 -= offsets3 * order;
                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                    peak.setIntHKL(hkl1);
                    peak.setIntMNP(V3D(0, 0, order));
                    suc_indexed = true;
                    sate_indexed++;
                    sate_error += hkl1.hklError();
                  }
                }
              }
            } else {
              if (ModDim == 1) {
                for (int order = -maxOrder; order <= maxOrder; order++) {
                  if (order == 0)
                    continue; // exclude order 0
                  V3D hkl1(hkl);
                  hkl1 -= offsets1 * order;
                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                    peak.setIntHKL(hkl1);
                    peak.setIntMNP(V3D(order, 0, 0));
                    suc_indexed = true;
                    sate_indexed++;
                    sate_error += hkl1.hklError();
                  }
                }
              }
              if (ModDim == 2) {
                for (int m = -maxOrder; m <= maxOrder; m++)
                  for (int n = -maxOrder; n <= maxOrder; n++) {
                    if (m == 0 && n == 0)
                      continue; // exclude 0,0
                    V3D hkl1(hkl);
                    hkl1 -= offsets1 * m + offsets2 * n;
                    if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                      peak.setIntHKL(hkl1);
                      peak.setIntMNP(V3D(m, n, 0));
                      suc_indexed = true;
                      sate_indexed++;
                      sate_error += hkl1.hklError();
                    }
                  }
              }
              if (ModDim == 3) {
                for (int m = -maxOrder; m <= maxOrder; m++)
                  for (int n = -maxOrder; n <= maxOrder; n++)
                    for (int p = -maxOrder; p <= maxOrder; p++) {
                      if (m == 0 && n == 0 && p == 0)
                        continue; // exclude 0,0,0
                      V3D hkl1(hkl);
                      hkl1 -= offsets1 * m + offsets2 * n + offsets3 * p;
                      if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
                        peak.setIntHKL(hkl1);
                        peak.setIntMNP(V3D(m, n, p));
                        suc_indexed = true;
                        sate_indexed++;
                        sate_error += hkl1.hklError();
                      }
                    }
              }
            }
            if (!suc_indexed) {
              peak.setIntHKL(V3D(0, 0, 0));
              peak.setIntMNP(V3D(0, 0, 0));
            }
          }
        }

        num_indexed = main_indexed + sate_indexed;
        total_main += main_indexed;
        total_sate += sate_indexed;
        total_main_error += main_error / 3;
        total_sate_error += sate_error / 3;
        total_indexed += main_indexed + sate_indexed;
        total_error += main_error / 3 + sate_error / 3;

        if (run_numbers.size() > 1) {
          g_log.notice() << "Run " << run << ": indexed " << num_indexed
                         << " Peaks out of " << q_vectors.size() << '\n';
          g_log.notice() << "of which, " << main_indexed
                         << " Main Bragg Peaks are indexed with tolerance of "
                         << tolerance << ", " << sate_indexed
                         << " Satellite Peaks are indexed with tolerance of "
                         << satetolerance << '\n';
        }
      }
    }

    if (total_indexed > 0)
      average_error = total_error / total_indexed;
    else
      average_error = 0;

    if (total_main > 0)
      average_main_error = total_main_error / total_main;
    else
      average_main_error = 0;

    if (total_sate > 0)
      average_sate_error = total_sate_error / total_sate;
    else
      average_sate_error = 0;
  }

  if (o_lattice.getMaxOrder() == 0) {
    // tell the user how many were indexed overall and the overall average error
    g_log.notice() << "ALL Runs: indexed " << total_indexed << " Peaks out of "
                   << n_peaks << " with tolerance of " << tolerance << '\n';
    g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                   << average_error << '\n';

    // Save output properties
    this->setProperty("NumIndexed", total_indexed);
    this->setProperty("AverageError", average_error);
    // Show the lattice parameters
    g_log.notice() << o_lattice << "\n";
  } else {
    g_log.notice() << "ALL Runs: indexed " << total_indexed << " Peaks out of "
                   << n_peaks << " with tolerance of " << tolerance << '\n';
    g_log.notice() << "Out of " << total_indexed << " Indexed Peaks "
                   << total_main << " are Main Bragg Peaks, and " << total_sate
                   << " are satellite peaks " << '\n';

    g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                   << average_error << '\n';
    g_log.notice() << "Average error in h,k,l for indexed main peaks =  "
                   << average_main_error << '\n';
    g_log.notice() << "Average error in h,k,l for indexed satellite peaks =  "
                   << average_sate_error << '\n';

    // Save output properties
    setProperty("TotalNumIndexed", total_indexed);
    setProperty("MainNumIndexed", total_main);
    setProperty("SateNumIndexed", total_sate);
    setProperty("MainError", average_main_error);
    setProperty("SatelliteError", average_sate_error);
    // Show the lattice parameters
    g_log.notice() << o_lattice << "\n";
  }
}

} // namespace Crystal
} // namespace Mantid
