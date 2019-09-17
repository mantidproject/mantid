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

#include <boost/optional/optional.hpp>

#include <algorithm>

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IndexPeaks)

using DataObjects::Peak;
using DataObjects::PeaksWorkspace;
using DataObjects::PeaksWorkspace_sptr;
using Geometry::IndexingUtils;
using Geometry::OrientedLattice;
using Kernel::DblMatrix;
using Kernel::V3D;

namespace {
const auto OPTIMIZE_UB_ATTEMPTS{4};

namespace Prop {
const std::string PEAKSWORKSPACE{"PeaksWorkspace"};
const std::string TOLERANCE{"Tolerance"};
const std::string SATE_TOLERANCE{"ToleranceForSatellite"};
const std::string ROUNDHKLS{"RoundHKLs"};
const std::string COMMONUB{"CommonUBForAll"};
const std::string AVERAGE_ERR{"AverageError"};
const std::string NUM_INDEXED{"NumIndexed"};
const std::string MAIN_NUM_INDEXED{"MainNumIndexed"};
const std::string SATE_NUM_INDEXED{"SateNumIndexed"};
const std::string MAIN_ERR{"MainError"};
const std::string SATE_ERR{"SatelliteError"};

struct SatelliteIndexingArgs {
  const double tolerance;
  const int maxOrder;
  const std::vector<V3D> modVectors;
};

struct IndexPeaksArgs {
  static IndexPeaksArgs parse(const API::Algorithm &alg) {
    PeaksWorkspace_sptr peaksWS = alg.getProperty(PEAKSWORKSPACE);
    const auto &lattice = peaksWS->sample().getOrientedLattice();

    auto addIfNonZero = [](const V3D &modVec, std::vector<V3D> &modVectors) {
      if (std::fabs(modVec[0]) > 0 && std::fabs(modVec[1]) > 0 &&
          std::fabs(modVec[2]) > 0)
        modVectors.emplace_back(modVec);
    };
    const int maxOrder = lattice.getMaxOrder();
    std::vector<V3D> modVectorsToUse;
    if (maxOrder > 0) {
      modVectorsToUse.reserve(3);
      for (auto i = 0; i < 3; ++i) {
        addIfNonZero(lattice.getModVec(i), modVectorsToUse);
      }
    }

    return {peaksWS, alg.getProperty(TOLERANCE), alg.getProperty(ROUNDHKLS),
            alg.getProperty(COMMONUB),
            SatelliteIndexingArgs{alg.getProperty(SATE_TOLERANCE), maxOrder,
                                  modVectorsToUse}};
  }

  PeaksWorkspace_sptr workspace;
  const double mainTolerance;
  const bool roundHKLs;
  const bool commonUB;
  const SatelliteIndexingArgs satellites;
};
} // namespace Prop

/**
 * Track details about the peaks successfully indexed
 */
struct PeakIndexingStats {
  PeakIndexingStats &operator+=(const PeakIndexingStats &rhs) {
    numIndexed += rhs.numIndexed;
    error += rhs.error;
    return *this;
  }
  int numIndexed{0};
  double error{0.0};
};

/**
 * Track details of main and satellite reflections that have
 * been indexed
 */
struct CombinedIndexingStats {
  CombinedIndexingStats &operator+=(const CombinedIndexingStats &rhs) {
    main += rhs.main;
    satellites += rhs.satellites;
    return *this;
  }

  /// Return the total number of peaks indexed
  inline int totalNumIndexed() const {
    return main.numIndexed + satellites.numIndexed;
  }
  /// Return the number of fundamental peaks indexed
  inline double mainError() const {
    if (main.numIndexed == 0)
      return 0.0;
    return main.error / main.numIndexed;
  }
  /// Return the number of satellite peaks indexed
  inline double satelliteError() const {
    if (satellites.numIndexed == 0)
      return 0.0;
    return satellites.error / satellites.numIndexed;
  }
  /// Return the average error for both main/satellites indexed
  inline double averageError() const {
    return (main.error + satellites.error) / totalNumIndexed();
  }

  PeakIndexingStats main;
  PeakIndexingStats satellites;
};

/**
 * Attempt to optimize the UB for the given set of peaks
 * @param ubOrig Original sample UB matrix
 * @param qSample The Q_sample of each peak to optimize
 * @return A new optimized UB
 */
DblMatrix optimizeUBMatrix(const DblMatrix &ubOrig,
                           const std::vector<V3D> &qSample,
                           const double tolerance) {
  DblMatrix optimizedUB(ubOrig);

  double errorAtStart{0.0};
  std::vector<V3D> millerIndices;
  millerIndices.reserve(qSample.size());
  const int numIndexedAtStart = IndexingUtils::CalculateMillerIndices(
      optimizedUB, qSample, tolerance, millerIndices, errorAtStart);

  if (numIndexedAtStart < 3) {
    // can't optimize without at least 3 indexed peaks
    return optimizedUB;
  }

  for (auto i = 0; i < OPTIMIZE_UB_ATTEMPTS; ++i) {
    try {
      // optimization requires rounded indices
      IndexingUtils::RoundHKLs(millerIndices);
      IndexingUtils::Optimize_UB(optimizedUB, millerIndices, qSample);
    } catch (...) {
      // If there is any problem, such as too few
      // independent peaks, just use the original UB
      optimizedUB = ubOrig;
      break;
    }
    double errorInLoop{0.0};
    const int numIndexedInLoop = IndexingUtils::CalculateMillerIndices(
        optimizedUB, qSample, tolerance, millerIndices, errorInLoop);
    if (numIndexedInLoop < numIndexedAtStart) // use the original UB
      break;
  }
  return optimizedUB;
}

/// <IntHKL, IntMNP, error>
using IndexedSatelliteInfo = std::tuple<V3D, V3D, double>;

/**
 * @brief Attempt to index a satellite reflection given a HKL from a failed
 * indexing of a main reflection.
 *   - loop over [-maxOrder, maxOrder] (skipping 0) and for each order:
 *     - compute "hkl_main - order x mod_vector" and see if
 *       this is a valid set of indices
 *     - if it is accept it as a satellite
 *     - if not do nothing
 * this continues for each modulation provided and the satellite will be the
 * last found with a valid index.

 * @param mainHKL The HKL of a failed attempt to index a fundamental
 reflection
 * @param maxOrder The highest order of to search away from the peak
 * @param modVectors
 * @param tolerance
 * @return
 */
boost::optional<IndexedSatelliteInfo>
indexSatellite(const V3D &mainHKL, int maxOrder,
               const std::vector<V3D> &modVectors, double tolerance) {
  assert(modVectors.size() <= 3);

  bool foundSatellite{false};
  V3D candidateIntHKL, candidateMNP;
  for (auto i = 0u; i < modVectors.size(); ++i) {
    const auto &modVector = modVectors[i];
    for (int order = -maxOrder; order <= maxOrder; ++order) {
      if (order == 0)
        continue;
      candidateIntHKL = mainHKL - modVector * order;
      if (IndexingUtils::ValidIndex(candidateIntHKL, tolerance)) {
        candidateMNP[i] = order;
        foundSatellite = true;
        // we deliberately don't break and use the last valid
        // reflection we find.
      }
    }
  }
  if (foundSatellite)
    return std::make_tuple(candidateIntHKL, candidateMNP,
                           candidateIntHKL.hklError());
  else
    return boost::none;
}

/**
 * Attempt to index any unindexed peaks as if they were satellites and set
 * the HKLs of any indexed peaks.
 *
 * @param peaks A list of peaks, with some possibly indexed as main
 * reflections
 * @param millerIndices A list of miller indices calculated from attempting
 * to index the peaks in the peaks list as fundamental peaks. The size is
 * assumed to match the size of peaks.
 * @param roundHKLs If true round the output HKL indices
 * @param args Additional arguments relating to the satellites peaks
 * @returns A PeakIndexingStats object describing the output
 */
PeakIndexingStats indexSatellites(const std::vector<Peak *> &peaks,
                                  std::vector<V3D> &millerIndices,
                                  const bool roundHKLs,
                                  const Prop::SatelliteIndexingArgs &args) {
  PeakIndexingStats stats;

  for (auto i = 0u; i < peaks.size(); ++i) {
    auto peak = peaks[i];
    auto &mainHKL{millerIndices[i]};

    // TODO: THIS TEST IS WRONG AS WE HAVE NOT SET THE HKL YET!!!!
    if (peak->isIndexed()) {
      if (roundHKLs)
        IndexingUtils::RoundHKL(mainHKL);
      peak->setHKL(mainHKL);
      peak->setIntHKL(mainHKL);
      peak->setIntMNP(V3D(0, 0, 0));
    } else {
      auto result = indexSatellite(mainHKL, args.maxOrder, args.modVectors,
                                   args.tolerance);
      if (result) {
        const auto &satelliteInfo = result.get();
        if (roundHKLs)
          IndexingUtils::RoundHKL(mainHKL);
        peak->setHKL(mainHKL);
        peak->setIntHKL(std::get<0>(satelliteInfo));
        peak->setIntMNP(std::get<1>(satelliteInfo));
        stats.numIndexed++;
        stats.error += std::get<2>(satelliteInfo) / 3.;
      }
    }
  }
  return stats;
}

/**
 * Index the main reflections on the workspace using the given UB matrix
 * @param peaksWS Workspace containing peaks
 * @param ub A UB matrix to define the the transform from Q_sample to hkl
 * @param tolerance If an index is within this tolerance of an integer then
 * accept it
 * @param optimizeUB If true optimize the UB for these peaks
 * @param satelliteArgs If set, attempt to index peaks as satellies if main
 * indexing fails
 * @return A CombinedIndexingStats detailing the output found
 */
CombinedIndexingStats
indexPeaks(const std::vector<Peak *> &peaks, DblMatrix ub,
           const double mainTolerance, const bool roundHKLs,
           const bool optimizeUB,
           const Prop::SatelliteIndexingArgs &satelliteArgs) {
  const auto nPeaks = peaks.size();
  std::vector<V3D> qSample(nPeaks);
  std::generate(
      std::begin(qSample), std::end(qSample),
      [&peaks, i = 0u ]() mutable { return peaks[i++]->getQSampleFrame(); });

  if (optimizeUB) {
    ub = optimizeUBMatrix(ub, qSample, mainTolerance);
  }

  CombinedIndexingStats stats;
  ub.Invert();
  for (auto i = 0u; i < peaks.size(); ++i) {
    const auto peak = peaks[i];
    V3D millerIndices;
    if (IndexingUtils::CalculateMillerIndices(ub, qSample[i], mainTolerance,
                                              millerIndices)) {
      stats.main.numIndexed++;
      stats.main.error += millerIndices.hklError() / 3.0;
      if (roundHKLs) {
        IndexingUtils::RoundHKL(millerIndices);
      }
      peak->setHKL(millerIndices);
      peak->setIntHKL(millerIndices);
      peak->setIntMNP(V3D(0, 0, 0));
    }

    //  std::vector<V3D> millerIndices;
    //  millerIndices.reserve(qSample.size());
    //  CombinedIndexingStats stats;
    //  double averageError{0.0};
    //  stats.main.numIndexed = IndexingUtils::CalculateMillerIndices(
    //      ub, qSample, mainTolerance, millerIndices, averageError);
    //  stats.main.error = averageError * stats.main.numIndexed;

    //  if (static_cast<size_t>(stats.main.numIndexed) != nPeaks &&
    //      satelliteArgs.maxOrder > 0) {
    //    stats.satellites =
    //        indexSatellites(peaks, millerIndices, roundHKLs, satelliteArgs);
    //  } else {
    //    if (roundHKLs)
    //      IndexingUtils::RoundHKLs(millerIndices);

    //    std::for_each(std::begin(peaks), std::end(peaks),
    //                  [&millerIndices, i = 0u ](Peak * peak) mutable {
    //                    peak->setHKL(millerIndices[i]);
    //                    peak->setIntHKL(millerIndices[i]);
    //                    peak->setIntMNP(V3D(0, 0, 0));
    //                    ++i;
    //                  });
    //  }
  }
  return stats;
}

} // namespace

/** Initialize the algorithm's properties.
 */
void IndexPeaks::init() {
  using Mantid::API::WorkspaceProperty;
  using Mantid::Kernel::BoundedValidator;
  using Mantid::Kernel::Direction;

  // -- inputs --
  this->declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace_sptr::element_type>>(
          Prop::PEAKSWORKSPACE, "", Direction::InOut),
      "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  this->declareProperty(Prop::TOLERANCE, 0.15, mustBePositive,
                        "Main peak indexing tolerance", Direction::Input);
  this->declareProperty(Prop::SATE_TOLERANCE, 0.15, mustBePositive,
                        "Satellite peak indexing tolerance", Direction::Input);
  this->declareProperty(Prop::ROUNDHKLS, true,
                        "Round H, K and L values to integers");
  this->declareProperty(Prop::COMMONUB, false,
                        "Index all orientations with a common UB");
  // -- outputs --
  this->declareProperty(Prop::NUM_INDEXED, 0,
                        "Gets set with the number of indexed peaks.",
                        Direction::Output);
  this->declareProperty(Prop::AVERAGE_ERR, 0.0,
                        "Gets set with the average HKL indexing error.",
                        Direction::Output);
  this->declareProperty(Prop::MAIN_NUM_INDEXED, 0,
                        "Gets set with the number of indexed main peaks.",
                        Direction::Output);
  this->declareProperty(Prop::SATE_NUM_INDEXED, 0,
                        "Gets set with the number of indexed main peaks.",
                        Direction::Output);
  this->declareProperty(
      Prop::MAIN_ERR, 0.0,
      "Gets set with the average HKL indexing error of Main Peaks.",
      Direction::Output);
  this->declareProperty(
      Prop::SATE_ERR, 0.0,
      "Gets set with the average HKL indexing error of Satellite Peaks.",
      Direction::Output);
}

/**
 * Validate all inputs once set
 * @return A map of property name to help message
 */
std::map<std::string, std::string> IndexPeaks::validateInputs() {
  std::map<std::string, std::string> helpMsgs;

  PeaksWorkspace_sptr ws = this->getProperty(Prop::PEAKSWORKSPACE);
  try {
    ws->sample().getOrientedLattice();
  } catch (std::runtime_error &exc) {
    helpMsgs[Prop::PEAKSWORKSPACE] = exc.what();
  }

  return helpMsgs;
}

/**
 * Execute the algorithm.
 */
void IndexPeaks::exec() {
  const auto args = Prop::IndexPeaksArgs::parse(*this);

  // quick exit
  if (args.workspace->getNumberPeaks() == 0) {
    g_log.warning("Empty peaks workspace. Nothing to index");
    return;
  }

  CombinedIndexingStats indexingInfo;
  const auto &lattice = args.workspace->sample().getOrientedLattice();
  const auto &sampleUB = lattice.getUB();
  if (args.commonUB) {
    // Use sample UB an all peaks regardless of run
    std::vector<Peak *> allPeaksRef(args.workspace->getNumberPeaks());
    std::transform(std::begin(args.workspace->getPeaks()),
                   std::end(args.workspace->getPeaks()),
                   std::begin(allPeaksRef), [](Peak &peak) { return &peak; });
    const bool optimizeUB{false};
    indexingInfo = indexPeaks(allPeaksRef, sampleUB, args.mainTolerance,
                              args.roundHKLs, optimizeUB, args.satellites);
  } else {
    // Use a UB optimized for each run
    auto &allPeaks = args.workspace->getPeaks();
    std::unordered_map<int, std::vector<Peak *>> peaksPerRun;
    std::for_each(std::begin(allPeaks), std::end(allPeaks),
                  [&peaksPerRun](Peak &peak) {
                    peaksPerRun[peak.getRunNumber()].emplace_back(&peak);
                  });
    const bool optimizeUB{true};
    for (const auto &runPeaks : peaksPerRun) {
      indexingInfo += indexPeaks(runPeaks.second, sampleUB, args.mainTolerance,
                                 args.roundHKLs, optimizeUB, args.satellites);
    }
  }

  // else {
  //    assert(false);
  //    auto ws = args.workspace;
  //    const auto &o_lattice = ws->sample().getOrientedLattice();
  //    const auto &UB = o_lattice.getUB();
  //    const bool round_hkls = this->getProperty(Prop::ROUNDHKLS);
  //    std::vector<Peak> &peaks = ws->getPeaks();
  //    const size_t n_peaks = static_cast<size_t>(ws->getNumberPeaks());
  //    int total_indexed{0}, total_main{0}, total_sate{0};
  //    double average_error{0.}, average_main_error{0.},
  //    average_sate_error{0.}; const double tolerance =
  //    this->getProperty("Tolerance");

  //    double total_error{0.}, total_main_error{0.}, total_sate_error{0.};
  //    double satetolerance = this->getProperty("ToleranceForSatellite");

  //    // get list of run numbers in this peaks workspace
  //    std::vector<int> run_numbers;
  //    for (const auto &peak : peaks) {
  //      int run = peak.getRunNumber();
  //      bool found = false;
  //      size_t k = 0;
  //      while (k < run_numbers.size() && !found) {
  //        if (run == run_numbers[k])
  //          found = true;
  //        else
  //          k++;
  //      }
  //      if (!found)
  //        run_numbers.push_back(run);
  //    }

  //    // index the peaks for each run separately, using a UB matrix
  //    optimized
  //    // for that run

  //    for (const int run : run_numbers) {
  //      std::vector<V3D> miller_indices;
  //      std::vector<V3D> q_vectors;

  //      for (const auto &peak : peaks) {
  //        if (peak.getRunNumber() == run)
  //          q_vectors.push_back(peak.getQSampleFrame());
  //      }

  //      DblMatrix tempUB(UB);

  //      int num_indexed = 0;
  //      double original_error = 0;
  //      int original_indexed = IndexingUtils::CalculateMillerIndices(
  //          tempUB, q_vectors, tolerance, miller_indices, original_error);

  //      IndexingUtils::RoundHKLs(miller_indices); // HKLs must be rounded
  //      for
  //      // Optimize_UB to work
  //      num_indexed = original_indexed;
  //      average_error = original_error;

  //      bool done = false;
  //      if (num_indexed < 3) // can't optimize without at least 3
  //      {                    // peaks
  //        done = true;
  //      }

  //      int iteration = 0;
  //      while (iteration < 4 && !done) // try repeatedly optimizing 4 times
  //      {                              // which is usually sufficient
  //        try {
  //          IndexingUtils::Optimize_UB(tempUB, miller_indices, q_vectors);
  //        } catch (...) // If there is any problem, such as too few
  //        {             // independent peaks, just use the original UB
  //          tempUB = UB;
  //          done = true;
  //        }

  //        num_indexed = IndexingUtils::CalculateMillerIndices(
  //            tempUB, q_vectors, tolerance, miller_indices, average_error);

  //        IndexingUtils::RoundHKLs(miller_indices); // HKLs must be rounded
  //        for
  //        // Optimize_UB to work

  //        if (num_indexed < original_indexed) // just use the original UB
  //        {
  //          num_indexed = original_indexed;
  //          average_error = original_error;
  //          done = true;
  //        }

  //        iteration++;
  //      }

  //      // If data not modulated, recalculate fractional HKL
  //      if (o_lattice.getMaxOrder() == 0) {
  //        // If user wants fractional hkls, recalculate them
  //        if (!round_hkls) {
  //          num_indexed = IndexingUtils::CalculateMillerIndices(
  //              tempUB, q_vectors, tolerance, miller_indices,
  //              average_error);
  //        }
  //        total_indexed += num_indexed;
  //        total_main += num_indexed;
  //        total_error += average_error * num_indexed;
  //        total_main_error += average_error * num_indexed;

  //        // tell the user how many were indexed in each run
  //        if (run_numbers.size() > 1) {
  //          g_log.notice() << "Run " << run << ": indexed " << num_indexed
  //                         << " Peaks out of " << q_vectors.size()
  //                         << " with tolerance of " << tolerance << '\n';
  //          g_log.notice() << "Average error in h,k,l for indexed peaks =  "
  //                         << average_error << '\n';
  //        }

  //        size_t miller_index_counter = 0;
  //        for (auto &peak : peaks) {
  //          if (peak.getRunNumber() == run) {
  //            peak.setHKL(miller_indices[miller_index_counter]);
  //            peak.setIntHKL(miller_indices[miller_index_counter]);
  //            peak.setIntMNP(V3D(0, 0, 0));
  //            miller_index_counter++;
  //          }
  //        }

  //      } else {
  //        g_log.notice() << "Maximum Order: " << o_lattice.getMaxOrder() <<
  //        '\n'; int ModDim = 0; int main_indexed = 0; int sate_indexed = 0;
  //        double main_error = 0;
  //        double sate_error = 0;
  //        const int maxOrder = o_lattice.getMaxOrder();
  //        const bool crossTerm = o_lattice.getCrossTerm();
  //        const V3D offsets1 = o_lattice.getModVec(0);
  //        const V3D offsets2 = o_lattice.getModVec(1);
  //        const V3D offsets3 = o_lattice.getModVec(2);

  //        if (offsets1 == V3D(0, 0,
  //        0))test_zero_satellite_tol_only_indexes_main_refl_with_modvectors_from_ub
  //          throw std::runtime_error("Invalid Modulation Vector");
  //        else if (offsets2 == V3D(0, 0, 0))
  //          ModDim = 1;
  //        else if (offsets3 == V3D(0, 0, 0))
  //          ModDim = 2;
  //        else
  //          ModDim = 3;

  //        IndexingUtils::CalculateMillerIndices(tempUB, q_vectors, 1.0,
  //                                              miller_indices,
  //                                              average_error);

  //        // Index satellite peaks
  //        size_t miller_index_counter = 0;
  //        for (auto &peak : peaks) {
  //          if (peak.getRunNumber() == run) {
  //            peak.setHKL(miller_indices[miller_index_counter]);
  //            miller_index_counter++;
  //            auto hkl = peak.getHKL();
  //            bool suc_indexed = false;

  //            if (IndexingUtils::ValidIndex(hkl, tolerance)) {
  //              if (round_hkls) {
  //                IndexingUtils::RoundHKL(hkl);
  //                peak.setHKL(hkl);
  //              }
  //              peak.setIntHKL(hkl);
  //              peak.setIntMNP(V3D(0, 0, 0));
  //              suc_indexed = true;
  //              main_indexed++;
  //              main_error += hkl.hklError();
  //            } else if (!crossTerm) {
  //              if (ModDim > 0) {
  //                for (int order = -maxOrder; order <= maxOrder; order++) {
  //                  if (order == 0)
  //                    continue; // exclude order 0
  //                  V3D hkl1(hkl);
  //                  hkl1 -= offsets1 * order;
  //                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
  //                    peak.setIntHKL(hkl1);
  //                    peak.setIntMNP(V3D(order, 0, 0));
  //                    suc_indexed = true;
  //                    sate_indexed++;
  //                    sate_error += hkl1.hklError();
  //                  }
  //                }
  //              }
  //              if (ModDim > 1) {
  //                for (int order = -maxOrder; order <= maxOrder; order++) {
  //                  if (order == 0)
  //                    continue; // exclude order 0
  //                  V3D hkl1(hkl);
  //                  hkl1 -= offsets2 * order;
  //                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
  //                    peak.setIntHKL(hkl1);
  //                    peak.setIntMNP(V3D(0, order, 0));
  //                    suc_indexed = true;
  //                    sate_indexed++;
  //                    sate_error += hkl1.hklError();
  //                  }
  //                }
  //              }
  //              if (ModDim > 2) {
  //                for (int order = -maxOrder; order <= maxOrder; order++) {
  //                  if (order == 0)
  //                    continue; // exclude order 0
  //                  V3D hkl1(hkl);
  //                  hkl1 -= offsets3 * order;
  //                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
  //                    peak.setIntHKL(hkl1);
  //                    peak.setIntMNP(V3D(0, 0, order));
  //                    suc_indexed = true;
  //                    sate_indexed++;
  //                    sate_error += hkl1.hklError();
  //                  }
  //                }
  //              }
  //            } else {
  //              if (ModDim == 1) {
  //                for (int order = -maxOrder; order <= maxOrder; order++) {
  //                  if (order == 0)
  //                    continue; // exclude order 0
  //                  V3D hkl1(hkl);
  //                  hkl1 -= offsets1 * order;
  //                  if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
  //                    peak.setIntHKL(hkl1);
  //                    peak.setIntMNP(V3D(order, 0, 0));
  //                    suc_indexed = true;
  //                    sate_indexed++;
  //                    sate_error += hkl1.hklError();
  //                  }
  //                }
  //              }
  //              if (ModDim == 2) {
  //                for (int m = -maxOrder; m <= maxOrder; m++)
  //                  for (int n = -maxOrder; n <= maxOrder; n++) {
  //                    if (m == 0 && n == 0)
  //                      continue; // exclude 0,0
  //                    V3D hkl1(hkl);
  //                    hkl1 -= offsets1 * m + offsets2 * n;
  //                    if (IndexingUtils::ValidIndex(hkl1, satetolerance)) {
  //                      peak.setIntHKL(hkl1);
  //                      peak.setIntMNP(V3D(m, n, 0));
  //                      suc_indexed = true;
  //                      sate_indexed++;
  //                      sate_error += hkl1.hklError();
  //                    }
  //                  }
  //              }
  //              if (ModDim == 3) {
  //                for (int m = -maxOrder; m <= maxOrder; m++)
  //                  for (int n = -maxOrder; n <= maxOrder; n++)
  //                    for (int p = -maxOrder; p <= maxOrder; p++) {
  //                      if (m == 0 && n == 0 && p == 0)
  //                        continue; // exclude 0,0,0
  //                      V3D hkl1(hkl);
  //                      hkl1 -= offsets1 * m + offsets2 * n + offsets3 * p;
  //                      if (IndexingUtils::ValidIndex(hkl1, satetolerance))
  //                      {
  //                        peak.setIntHKL(hkl1);
  //                        peak.setIntMNP(V3D(m, n, p));
  //                        suc_indexed = true;
  //                        sate_indexed++;
  //                        sate_error += hkl1.hklError();
  //                      }
  //                    }
  //              }
  //            }
  //            if (!suc_indexed) {
  //              peak.setHKL(V3D(0, 0, 0));
  //              peak.setIntHKL(V3D(0, 0, 0));
  //              peak.setIntMNP(V3D(0, 0, 0));
  //            }
  //          }
  //        }

  //        num_indexed = main_indexed + sate_indexed;
  //        total_main += main_indexed;
  //        total_sate += sate_indexed;
  //        total_main_error += main_error / 3;
  //        total_sate_error += sate_error / 3;
  //        total_indexed += main_indexed + sate_indexed;
  //        total_error += main_error / 3 + sate_error / 3;

  //        if (run_numbers.size() > 1) {
  //          g_log.notice() << "Run " << run << ": indexed " << num_indexed
  //                         << " Peaks out of " << q_vectors.size() << '\n';
  //          g_log.notice() << "of which, " << main_indexed
  //                         << " Main Bragg Peaks are indexed with tolerance
  //                         of "
  //                         << tolerance << ", " << sate_indexed
  //                         << " Satellite Peaks are indexed with tolerance
  //                         of "
  //                         << satetolerance << '\n';
  //        }
  //      }
  //    }

  //    if (total_indexed > 0)
  //      average_error = total_error / total_indexed;
  //    else
  //      average_error = 0;

  //    if (total_main > 0)
  //      average_main_error = total_main_error / total_main;
  //    else
  //      average_main_error = 0;

  //    if (total_sate > 0)
  //      average_sate_error = total_sate_error / total_sate;
  //    else
  //      average_sate_error = 0;

  //    indexingInfo.numMainIndexed = total_main;
  //    indexingInfo.numSatIndexed = total_sate;
  //    indexingInfo.averageError = average_error;
  //    indexingInfo.mainError = average_main_error;
  //    indexingInfo.satelliteError = average_sate_error;
  //}

  setProperty("NumIndexed", indexingInfo.totalNumIndexed());
  setProperty("MainNumIndexed", indexingInfo.main.numIndexed);
  setProperty("SateNumIndexed", indexingInfo.satellites.numIndexed);
  setProperty("AverageError", indexingInfo.averageError());
  setProperty("MainError", indexingInfo.mainError());
  setProperty("SatelliteError", indexingInfo.satelliteError());

  if (args.satellites.maxOrder > 0) {
    g_log.notice() << "ALL Runs: indexed " << indexingInfo.totalNumIndexed()
                   << " Peaks out of " << args.workspace->getNumberPeaks()
                   << " with tolerance of " << args.mainTolerance << '\n';
    g_log.notice() << "Out of " << indexingInfo.totalNumIndexed()
                   << " Indexed Peaks " << indexingInfo.main.numIndexed
                   << " are Main Bragg Peaks, and "
                   << indexingInfo.satellites.numIndexed
                   << " are satellite peaks " << '\n';

    g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                   << indexingInfo.averageError() << '\n';
    g_log.notice() << "Average error in h,k,l for indexed main peaks =  "
                   << indexingInfo.main.error << '\n';
    g_log.notice() << "Average error in h,k,l for indexed satellite peaks =  "
                   << indexingInfo.satellites.error << '\n';

    // Show the lattice parameters
    g_log.notice() << args.workspace->sample().getOrientedLattice() << "\n";
  } else {
    // tell the user how many were indexed overall and the overall average
    // error
    g_log.notice() << "ALL Runs: indexed " << indexingInfo.totalNumIndexed()
                   << " Peaks out of " << args.workspace->getNumberPeaks()
                   << " with tolerance of " << args.mainTolerance << '\n';
    g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                   << indexingInfo.main.error << '\n';

    // Show the lattice parameters
    g_log.notice() << args.workspace->sample().getOrientedLattice() << "\n";
  }
}

} // namespace Crystal
} // namespace Mantid
