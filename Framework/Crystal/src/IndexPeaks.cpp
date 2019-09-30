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
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
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
using Kernel::Logger;
using Kernel::V3D;

namespace {
const auto OPTIMIZE_UB_ATTEMPTS{4};

namespace Prop {
const std::string PEAKSWORKSPACE{"PeaksWorkspace"};
const std::string TOLERANCE{"Tolerance"};
const std::string SATE_TOLERANCE{"ToleranceForSatellite"};
const std::string ROUNDHKLS{"RoundHKLs"};
const std::string COMMONUB{"CommonUBForAll"};
const std::string MAXORDER{"MaxOrder"};
const std::string MODVECTOR1{"ModVector1"};
const std::string MODVECTOR2{"ModVector2"};
const std::string MODVECTOR3{"ModVector3"};
const std::string CROSSTERMS{"CrossTerms"};
const std::string SAVEMODINFO{"SaveModulationInfo"};
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
  const bool crossTerms;
};

struct IndexPeaksArgs {
  static IndexPeaksArgs parse(const API::Algorithm &alg) {
    const PeaksWorkspace_sptr peaksWS = alg.getProperty(PEAKSWORKSPACE);
    const int maxOrderFromAlg = alg.getProperty(Prop::MAXORDER);

    auto addIfNonZero = [](const auto &modVec, std::vector<V3D> &modVectors) {
      if (std::fabs(modVec[0]) > 0 || std::fabs(modVec[1]) > 0 ||
          std::fabs(modVec[2]) > 0)
        modVectors.emplace_back(V3D(modVec[0], modVec[1], modVec[2]));
    };

    int maxOrderToUse{0};
    std::vector<V3D> modVectorsToUse;
    modVectorsToUse.reserve(3);
    bool crossTermToUse{false};
    if (maxOrderFromAlg > 0) {
      // Use inputs from algorithm
      maxOrderToUse = maxOrderFromAlg;
      crossTermToUse = alg.getProperty(Prop::CROSSTERMS);
      std::vector<double> modVector = alg.getProperty(Prop::MODVECTOR1);
      addIfNonZero(std::move(modVector), modVectorsToUse);
      modVector = alg.getProperty(Prop::MODVECTOR2);
      addIfNonZero(std::move(modVector), modVectorsToUse);
      modVector = alg.getProperty(Prop::MODVECTOR3);
      addIfNonZero(std::move(modVector), modVectorsToUse);
    } else {
      // Use lattice definitions if they exist
      const auto &lattice = peaksWS->sample().getOrientedLattice();
      maxOrderToUse = lattice.getMaxOrder();
      if (maxOrderToUse > 0) {
        for (auto i = 0; i < 3; ++i) {
          addIfNonZero(lattice.getModVec(i), modVectorsToUse);
        }
      }
      crossTermToUse = lattice.getCrossTerm();
    }

    return {peaksWS,
            alg.getProperty(TOLERANCE),
            alg.getProperty(ROUNDHKLS),
            alg.getProperty(COMMONUB),
            alg.getProperty(SAVEMODINFO),
            SatelliteIndexingArgs{alg.getProperty(SATE_TOLERANCE),
                                  maxOrderToUse, modVectorsToUse,
                                  crossTermToUse}};
  }

  PeaksWorkspace_sptr workspace;
  const double mainTolerance;
  const bool roundHKLs;
  const bool commonUB;
  const bool storeModulationInfo;
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

/// Tie together a modulated peak number with its offset
using MNPOffset = std::tuple<double, double, double, V3D>;

/**
 * Calculate a list of HKL offsets from the given modulation vectors.
 * @param maxOrder Integer specifying the multiples of the modulation vector.
 * @param modVectors A list of modulation vectors form the user
 * @param crossTerms If true then compute products of the modulation vectors
 * @return A list of (m, n, p, V3D) were m,n,p specifies the modulation
 * structure number and V3D specifies the offset to be tested
 */
std::vector<MNPOffset>
calculateOffsetsToTest(const int maxOrder, const std::vector<V3D> &modVectors,
                       const bool crossTerms) {
  assert(modVectors.size() <= 3);

  std::vector<MNPOffset> offsets;
  if (crossTerms && modVectors.size() > 1) {
    const auto &modVector0{modVectors[0]}, modVector1{modVectors[1]};
    if (modVectors.size() == 2) {
      // Calculate m*mod_vec1 + n*mod_vec2 for combinations of m, n in
      // [-maxOrder,maxOrder]
      offsets.reserve(2 * maxOrder);
      for (auto m = -maxOrder; m <= maxOrder; ++m) {
        for (auto n = -maxOrder; n <= maxOrder; ++n) {
          if (m == 0 && n == 0)
            continue;
          offsets.emplace_back(
              std::make_tuple(m, n, 0, modVector0 * m + modVector1 * n));
        }
      }
    } else {
      // Calculate m*mod_vec1 + n*mod_vec2 + p*mod_vec3 for combinations of m,
      // n, p in
      // [-maxOrder,maxOrder]
      const auto &modVector2{modVectors[2]};
      offsets.reserve(3 * maxOrder);
      for (auto m = -maxOrder; m <= maxOrder; ++m) {
        for (auto n = -maxOrder; n <= maxOrder; ++n) {
          for (auto p = -maxOrder; p <= maxOrder; ++p) {
            if (m == 0 && n == 0 && p == 0)
              continue;
            offsets.emplace_back(std::make_tuple(
                m, n, p, modVector0 * m + modVector1 * n + modVector2 * p));
          }
        }
      }
    }
  } else {
    // No cross terms: Compute coeff*mod_vec_i for each modulation vector
    // separately for coeff in [-maxOrder, maxOrder]
    for (auto i = 0u; i < modVectors.size(); ++i) {
      const auto &modVector = modVectors[i];
      for (int order = -maxOrder; order <= maxOrder; ++order) {
        if (order == 0)
          continue;
        V3D offset{modVector * order};
        switch (i) {
        case 0:
          offsets.emplace_back(std::make_tuple(order, 0, 0, std::move(offset)));
          break;
        case 1:
          offsets.emplace_back(std::make_tuple(0, order, 0, std::move(offset)));
          break;
        case 2:
          offsets.emplace_back(std::make_tuple(0, 0, order, std::move(offset)));
          break;
        }
      }
    }
  }

  return offsets;
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
 * @param maxOrder The highest multiple of the offset vector  to search away
 from the peak
 * @param modVectors The list of offset vectors
 * @param tolerance Tolerance used to accept/reject a candidate peak
 * @param crossTerms If true use combinations of offset vectors for search,
 i.e.
 *                   candidateHKL = mainHKL - m*mod_1 - n*mod_2 - p*mod_3
 * @return (IntHKL, IntMNP, error) if a peak was indexed, otherwise none
 */

boost::optional<IndexedSatelliteInfo>
indexSatellite(const V3D &mainHKL, const int maxOrder,
               const std::vector<V3D> &modVectors, const double tolerance,
               const bool crossTerms) {
  const auto offsets = calculateOffsetsToTest(maxOrder, modVectors, crossTerms);
  bool foundSatellite{false};
  V3D indexedIntHKL, indexedMNP;
  for (const auto &mnpOffset : offsets) {
    const auto candidateIntHKL = mainHKL - std::get<3>(mnpOffset);
    const V3D candidateMNP{std::get<0>(mnpOffset), std::get<1>(mnpOffset),
                           std::get<2>(mnpOffset)};
    if (IndexingUtils::ValidIndex(candidateIntHKL, tolerance)) {
      indexedIntHKL = candidateIntHKL;
      indexedMNP = candidateMNP;
      foundSatellite = true;
      // we deliberately don't break and use the last valid
      // reflection we find.
    }
  }
  if (foundSatellite)
    return std::make_tuple(indexedIntHKL, indexedMNP, indexedIntHKL.hklError());
  else
    return boost::none;
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
    V3D nominalHKL = IndexingUtils::CalculateMillerIndices(ub, qSample[i]);
    if (IndexingUtils::ValidIndex(nominalHKL, mainTolerance)) {
      stats.main.numIndexed++;
      stats.main.error += nominalHKL.hklError() / 3.0;
      if (roundHKLs) {
        IndexingUtils::RoundHKL(nominalHKL);
      }
      peak->setHKL(nominalHKL);
      peak->setIntHKL(nominalHKL);
      peak->setIntMNP(V3D(0, 0, 0));
    } else if (satelliteArgs.maxOrder > 0) {
      auto result = indexSatellite(
          nominalHKL, satelliteArgs.maxOrder, satelliteArgs.modVectors,
          satelliteArgs.tolerance, satelliteArgs.crossTerms);
      if (result) {
        const auto &satelliteInfo = result.get();
        if (roundHKLs)
          IndexingUtils::RoundHKL(nominalHKL);
        peak->setHKL(nominalHKL);
        peak->setIntHKL(std::get<0>(satelliteInfo));
        peak->setIntMNP(std::get<1>(satelliteInfo));
        stats.satellites.numIndexed++;
        stats.satellites.error += std::get<2>(satelliteInfo) / 3.;
      }
    }
  }
  return stats;
}

/**
 * Log the results of indexing
 * @param out A stream reference to write to
 * @param indexingInfo Summary of indexing results
 * @param runNo Run number or -1 if all runs indexed together
 * @param nPeaksTotal The number of peaks in total that were attempted for
 * indexing
 * @param args Arguments passed to the algorithm
 */
void logIndexingResults(std::ostream &out,
                        const CombinedIndexingStats &indexingInfo,
                        const int runNo, const size_t nPeaksTotal,
                        const Prop::IndexPeaksArgs &args) {
  if (runNo >= 0)
    out << "Run " << runNo;
  else
    out << "All runs";
  out << " indexed " << indexingInfo.totalNumIndexed() << " peaks out of "
      << nPeaksTotal;
  if (args.satellites.maxOrder > 0) {
    out << " of which, " << indexingInfo.main.numIndexed
        << " main Bragg peaks are indexed with tolerance of "
        << args.mainTolerance << ", " << indexingInfo.satellites.numIndexed
        << " satellite peaks are indexed with tolerance of "
        << args.satellites.tolerance << '\n';
    out << "  Average error in h,k,l for indexed peaks =  "
        << indexingInfo.averageError() << '\n';
    out << "  Average error in h,k,l for indexed main peaks =  "
        << indexingInfo.main.error << '\n';
    out << "  Average error in h,k,l for indexed satellite peaks =  "
        << indexingInfo.satellites.error << '\n';
  } else {
    out << " with tolerance of " << args.mainTolerance << '\n';
    out << "  Average error in h,k,l for indexed peaks =  "
        << indexingInfo.mainError() << '\n';
  }
}

} // namespace

/** Initialize the algorithm's properties.
 */
void IndexPeaks::init() {
  using API::WorkspaceProperty;
  using Kernel::ArrayLengthValidator;
  using Kernel::ArrayProperty;
  using Kernel::BoundedValidator;
  using Kernel::Direction;

  // -- inputs --
  this->declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace_sptr::element_type>>(
          Prop::PEAKSWORKSPACE, "", Direction::InOut),
      "Input Peaks Workspace");

  auto mustBePositiveDbl = boost::make_shared<BoundedValidator<double>>();
  mustBePositiveDbl->setLower(0.0);
  this->declareProperty(Prop::TOLERANCE, 0.15, mustBePositiveDbl,
                        "Main peak indexing tolerance", Direction::Input);
  this->declareProperty(Prop::SATE_TOLERANCE, 0.15, mustBePositiveDbl,
                        "Satellite peak indexing tolerance", Direction::Input);
  this->declareProperty(Prop::ROUNDHKLS, true,
                        "Round H, K and L values to integers");
  this->declareProperty(Prop::COMMONUB, false,
                        "Index all orientations with a common UB");
  auto mustBeLengthThree = boost::make_shared<ArrayLengthValidator<double>>(3);
  this->declareProperty(std::make_unique<ArrayProperty<double>>(
                            Prop::MODVECTOR1, "0.0,0.0,0.0", mustBeLengthThree),
                        "Modulation Vector 1: dh, dk, dl");
  this->declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(
                            Prop::MODVECTOR2, "0.0,0.0,0.0", mustBeLengthThree),
                        "Modulation Vector 2: dh, dk, dl");
  this->declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(
                            Prop::MODVECTOR3, "0.0,0.0,0.0", mustBeLengthThree),
                        "Modulation Vector 3: dh, dk, dl");
  auto mustBePositiveOrZero = boost::make_shared<BoundedValidator<int>>();
  mustBePositiveOrZero->setLower(0);
  this->declareProperty(
      Prop::MAXORDER, 0, mustBePositiveOrZero,
      "Maximum order to apply Modulation Vectors. Default = 0",
      Direction::Input);

  this->declareProperty(
      Prop::CROSSTERMS, false,
      "Include combinations of modulation vectors in satellite search",
      Direction::Input);

  this->declareProperty(
      Prop::SAVEMODINFO, false,
      "If true, update the OrientedLattice with the maxOrder, "
      "modulation vectors & cross terms values input to the algorithm");

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

  // save modulation input if asked
  if (args.storeModulationInfo) {
    auto &lattice = args.workspace->mutableSample().getOrientedLattice();
    lattice.setMaxOrder(args.satellites.maxOrder);
    lattice.setCrossTerm(args.satellites.crossTerms);
    for (auto i = 0u; i < 3; ++i) {
      lattice.setModVec1(args.satellites.modVectors[i]);
    }
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
      const auto &peaks = runPeaks.second;
      const auto indexedInRun =
          indexPeaks(peaks, sampleUB, args.mainTolerance, args.roundHKLs,
                     optimizeUB, args.satellites);
      logIndexingResults(g_log.notice(), indexedInRun, runPeaks.first,
                         peaks.size(), args);
      indexingInfo += indexedInRun;
    }
  }

  setProperty("NumIndexed", indexingInfo.totalNumIndexed());
  setProperty("MainNumIndexed", indexingInfo.main.numIndexed);
  setProperty("SateNumIndexed", indexingInfo.satellites.numIndexed);
  setProperty("AverageError", indexingInfo.averageError());
  setProperty("MainError", indexingInfo.mainError());
  setProperty("SatelliteError", indexingInfo.satelliteError());

  // Final results
  logIndexingResults(g_log.notice(), indexingInfo, -1,
                     args.workspace->getNumberPeaks(), args);
  // Show the lattice parameters
  g_log.notice() << args.workspace->sample().getOrientedLattice() << "\n";
}

} // namespace Crystal
} // namespace Mantid
