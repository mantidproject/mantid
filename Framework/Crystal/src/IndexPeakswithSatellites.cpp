#include "MantidCrystal/IndexPeakswithSatellites.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Quat.h"
#include <Eigen/Geometry>
#include <algorithm>
#include <boost/math/special_functions/round.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <stdexcept>

namespace Mantid {
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IndexPeakswithSatellites)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void IndexPeakswithSatellites::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::InOut),
                  "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "Tolerance", 0.15, mustBePositive, Direction::Input),
                  "Main Indexing Tolerance (0.15)");

  declareProperty(
      make_unique<PropertyWithValue<double>>("ToleranceForSatellite", 0.15,
                                             mustBePositive, Direction::Input),
      "Satellite Indexing Tolerance (0.15)");

  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      std::string("ModVector1"), "0.0,0.0,0.0"),
                  "Modulation Vector 1: dh, dk, dl");

  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      std::string("ModVector2"), "0.0,0.0,0.0"),
                  "Modulation Vector 2: dh, dk, dl");

  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      std::string("ModVector3"), "0.0,0.0,0.0"),
                  "Modulation Vector 3: dh, dk, dl");

  declareProperty(
      make_unique<PropertyWithValue<int>>("MaxOrder", 0, Direction::Input),
      "Maximum order to apply Modulation Vectors. Default = 0");

  declareProperty(make_unique<PropertyWithValue<int>>("TotalNumIndexed", 0,
                                                      Direction::Output),
                  "Gets set with the number of indexed main peaks.");

  declareProperty(make_unique<PropertyWithValue<int>>("MainNumIndexed", 0,
                                                      Direction::Output),
                  "Gets set with the number of indexed main peaks.");

  declareProperty(make_unique<PropertyWithValue<int>>("SateNumIndexed", 0,
                                                      Direction::Output),
                  "Gets set with the number of indexed main peaks.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("MainError", 0.0,
                                             Direction::Output),
      "Gets set with the average HKL indexing error of Main Peaks.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("SatelliteError", 0.0,
                                             Direction::Output),
      "Gets set with the average HKL indexing error of Satellite Peaks.");

  declareProperty(make_unique<PropertyWithValue<bool>>("CrossTerms", false,
                                                       Direction::Input),
                  "Include cross terms (false)");
}

/** Execute the algorithm.
 */
void IndexPeakswithSatellites::exec() {
  bool crossTerms = getProperty("CrossTerms");
  PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
  const Matrix<double> &UB = o_lattice.getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error(
        "ERROR: The stored UB is not a valid orientation matrix");
  }

  std::vector<Peak> &peaks = ws->getPeaks();
  size_t n_peaks = ws->getNumberPeaks();
  int main_indexed = 0;
  int sate_indexed = 0;
  int total_indexed = 0;
  double main_error = 0.;
  double satellite_error = 0.;
  double tolerance = getProperty("Tolerance");
  double satetolerance = getProperty("ToleranceForSatellite");

  V3D offsets1 = getOffsetVector("ModVector1");
  V3D offsets2 = getOffsetVector("ModVector2");
  V3D offsets3 = getOffsetVector("ModVector3");
  int maxOrder = getProperty("MaxOrder");

  auto run = ws->mutableRun();
  run.addProperty<std::vector<double>>("Offset1", offsets1, true);
  run.addProperty<std::vector<double>>("Offset2", offsets2, true);
  run.addProperty<std::vector<double>>("Offset3", offsets3, true);

  double total_error = 0;
  // get list of run numbers in this peaks workspace
  std::unordered_set<int> run_numbers;

  for (Peak peak : peaks) {
    run_numbers.insert(peak.getRunNumber());
  }

  // index the peaks for each run separately, using a UB matrix optimized for
  // that run

  for (int run : run_numbers) {
    std::vector<V3D> miller_indices;
    std::vector<V3D> q_vectors;

    for (Peak peak : peaks) {
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
    double average_error = original_error;

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

    num_indexed = IndexingUtils::CalculateMillerIndices(
        tempUB, q_vectors, 1.0, miller_indices, average_error);

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

    for (Peak peak : peaks) {
      if (peak.getRunNumber() == run) {
        peak.setHKL(miller_indices[miller_index_counter]);
        miller_index_counter++;
      }
    }

    // Index satellite peaks
    for (auto &peak : peaks) {
      if (peak.getRunNumber() == run) {
        V3D hkl;
        hkl[0] = peak.getH();
        hkl[1] = peak.getK();
        hkl[2] = peak.getL();

        if (IndexingUtils::ValidIndex(hkl, tolerance)) {
          peak.setIntHKL(hkl);
          peak.setIntMNP(V3D(0, 0, 0));
          main_indexed++;
          double h_error = fabs(round(hkl[0]) - hkl[0]);
          double k_error = fabs(round(hkl[1]) - hkl[1]);
          double l_error = fabs(round(hkl[2]) - hkl[2]);
          main_error += h_error + k_error + l_error;
        } else if (!crossTerms) {
          predictOffsets(peak, sate_indexed, satetolerance, satellite_error,
                         offsets1, maxOrder, hkl);
          predictOffsets(peak, sate_indexed, satetolerance, satellite_error,
                         offsets2, maxOrder, hkl);
          predictOffsets(peak, sate_indexed, satetolerance, satellite_error,
                         offsets3, maxOrder, hkl);
        } else {
          predictOffsetsWithCrossTerms(peak, sate_indexed, satetolerance,
                                       satellite_error, offsets1, offsets2,
                                       offsets3, maxOrder, hkl);
        }
      }
    }
  }

  total_indexed = main_indexed + sate_indexed;

  if (main_indexed > 0)
    main_error /= (3.0 * main_indexed);
  else
    main_error = 0;

  if (sate_indexed > 0)
    satellite_error /= (3.0 * sate_indexed);
  else
    satellite_error = 0;
  //  }

  // tell the user how many were indexed overall and the overall average error
  g_log.notice() << "ALL Runs: indexed " << total_indexed << " Peaks out of "
                 << n_peaks << " with tolerance of " << tolerance << '\n';
  g_log.notice() << "Out of " << total_indexed << " Indexed Peaks "
                 << main_indexed << " are Main Bragg Peaks, and "
                 << sate_indexed << " are satellite peaks " << '\n';
  g_log.notice() << "Average error in h,k,l for indexed main peaks =  "
                 << main_error << '\n';
  g_log.notice() << "Average error in h,k,l for indexed satellite peaks =  "
                 << satellite_error << '\n';
  // Show the lattice parameters
  g_log.notice() << o_lattice << "\n";

  // Save output properties
  setProperty("TotalNumIndexed", total_indexed);
  setProperty("MainNumIndexed", main_indexed);
  setProperty("SateNumIndexed", sate_indexed);
  setProperty("MainError", main_error);
  setProperty("SatelliteError", satellite_error);
}
void IndexPeakswithSatellites::predictOffsets(Peak &peak, int &sate_indexed,
                                              double &satetolerance,
                                              double &satellite_error,
                                              V3D offsets, int &maxOrder,
                                              V3D &hkl) {
  if (offsets != V3D(0, 0, 0)) {
    for (int order = -maxOrder; order <= maxOrder; order++) {
      if (order == 0)
        continue; // exclude order 0
      V3D hkl1(hkl);
      hkl1[0] -= order * offsets[0];
      hkl1[1] -= order * offsets[1];
      hkl1[2] -= order * offsets[2];
      if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
        peak.setIntHKL(hkl1);
        peak.setIntMNP(V3D(order, 0, 0));
        sate_indexed++;
        double h_error = fabs(round(hkl1[0]) - hkl1[0]);
        double k_error = fabs(round(hkl1[1]) - hkl1[1]);
        double l_error = fabs(round(hkl1[2]) - hkl1[2]);
        satellite_error += h_error + k_error + l_error;
      }
    }
  }
}
void IndexPeakswithSatellites::predictOffsetsWithCrossTerms(
    Peak &peak, int &sate_indexed, double &satetolerance,
    double &satellite_error, V3D offsets1, V3D offsets2, V3D offsets3,
    int &maxOrder, V3D &hkl) {
  DblMatrix offsetsMat(3, 3);
  offsetsMat.setColumn(0, offsets1);
  offsetsMat.setColumn(1, offsets2);
  offsetsMat.setColumn(2, offsets3);
  int maxOrder1 = maxOrder;
  if (offsets1 == V3D(0, 0, 0))
    maxOrder1 = 0;
  int maxOrder2 = maxOrder;
  if (offsets2 == V3D(0, 0, 0))
    maxOrder2 = 0;
  int maxOrder3 = maxOrder;
  if (offsets3 == V3D(0, 0, 0))
    maxOrder3 = 0;
  for (int m = -maxOrder1; m <= maxOrder1; m++)
    for (int n = -maxOrder2; n <= maxOrder2; n++)
      for (int p = -maxOrder3; p <= maxOrder3; p++) {
        if (m == 0 && n == 0 && p == 0)
          continue; // exclude 0,0,0
        V3D hkl1(hkl);
        V3D mnp = V3D(m, n, p);
        hkl1 -= offsetsMat * mnp;
        if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
          peak.setIntHKL(hkl1);
          peak.setIntMNP(V3D(m, n, p));
          sate_indexed++;

          V3D intHkl = hkl1;
          intHkl.round();
          hkl1 = intHkl - hkl1;
          satellite_error += fabs(hkl1[0]) + fabs(hkl1[1]) + fabs(hkl1[2]);
        }
      }
}
V3D IndexPeakswithSatellites::getOffsetVector(const std::string &label) {
  vector<double> offsets = getProperty(label);
  if (offsets.empty()) {
    offsets.push_back(0.0);
    offsets.push_back(0.0);
    offsets.push_back(0.0);
  }
  V3D offsets1 = V3D(offsets[0], offsets[1], offsets[2]);
  return offsets1;
}

} // namespace Crystal
} // namespace Mantid
