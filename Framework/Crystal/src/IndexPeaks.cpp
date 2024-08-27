// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/IndexPeaks.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/PeakAlgorithmHelpers.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <optional>

#include <algorithm>

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IndexPeaks)

using API::IPeaksWorkspace;
using API::IPeaksWorkspace_sptr;
using Geometry::IndexingUtils;
using Geometry::IPeak;
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
  /**
   * @brief parse input arguments about mod vector and max order
   * @param alg: reference to algorithm instance
   * @return: SatelliteIndexingArgs
   */
  static IndexPeaksArgs parse(const API::Algorithm &alg) {
    const IPeaksWorkspace_sptr peaksWS = alg.getProperty(PEAKSWORKSPACE);
    const int maxOrderFromAlg = alg.getProperty(ModulationProperties::MaxOrder);

    // Init variables
    int maxOrderToUse{0};
    std::vector<V3D> modVectorsToUse;
    modVectorsToUse.reserve(3);
    bool crossTermToUse{false};

    // Parse mod vectors
    modVectorsToUse = addModulationVectors(alg.getProperty(ModulationProperties::ModVector1),
                                           alg.getProperty(ModulationProperties::ModVector2),
                                           alg.getProperty(ModulationProperties::ModVector3));
    // check the 3 mod vectors added from properties
    modVectorsToUse = validModulationVectors(modVectorsToUse[0], modVectorsToUse[1], modVectorsToUse[2]);

    // deal with case: max order > 0 and no mod vector is specified
    if (maxOrderFromAlg > 0 && modVectorsToUse.size() == 0) {
      // Max Order is larger than zero but no modulated vector specified
      // Assume that the caller method will handle this
      maxOrderToUse = maxOrderFromAlg;
    } else if (maxOrderFromAlg == 0 && modVectorsToUse.size() == 0) {
      // Use lattice definitions if they exist
      const auto &lattice = peaksWS->sample().getOrientedLattice();
      crossTermToUse = lattice.getCrossTerm();
      maxOrderToUse = lattice.getMaxOrder(); // the lattice can return a 0 here

      // if lattice has maxOrder, we will use the modVec from it, otherwise
      // stick to the input got from previous assignment
      if (maxOrderToUse > 0) {
        modVectorsToUse = validModulationVectors(lattice.getModVec(0), lattice.getModVec(1), lattice.getModVec(2));
      }
    } else {
      // Use user specified
      // default behavior: map everything automatically
      maxOrderToUse = maxOrderFromAlg;
      crossTermToUse = alg.getProperty(ModulationProperties::CrossTerms);
    }

    return {peaksWS,
            alg.getProperty(TOLERANCE),
            alg.getProperty(ROUNDHKLS),
            alg.getProperty(COMMONUB),
            alg.getProperty(SAVEMODINFO),
            SatelliteIndexingArgs{alg.getProperty(SATE_TOLERANCE), maxOrderToUse, modVectorsToUse, crossTermToUse}};
  }

  IPeaksWorkspace_sptr workspace;
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
  inline int totalNumIndexed() const { return main.numIndexed + satellites.numIndexed; }
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
  inline double averageError() const { return (main.error + satellites.error) / totalNumIndexed(); }

  PeakIndexingStats main;
  PeakIndexingStats satellites;
};

/**
 * Attempt to optimize the UB for the given set of peaks
 * @param ubOrig Original sample UB matrix
 * @param qSample The Q_sample of each peak to optimize
 * @return A new optimized UB
 */
DblMatrix optimizeUBMatrix(const DblMatrix &ubOrig, const std::vector<V3D> &qSample, const double tolerance) {
  DblMatrix optimizedUB(ubOrig);

  double errorAtStart{0.0};
  std::vector<V3D> millerIndices;
  millerIndices.reserve(qSample.size());
  const int numIndexedAtStart =
      IndexingUtils::CalculateMillerIndices(optimizedUB, qSample, tolerance, millerIndices, errorAtStart);

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
    const int numIndexedInLoop =
        IndexingUtils::CalculateMillerIndices(optimizedUB, qSample, tolerance, millerIndices, errorInLoop);
    if (numIndexedInLoop < numIndexedAtStart) // use the original UB
      break;
  }
  return optimizedUB;
}

/// <HKL, IntHKL, IntMNP, error>
using IndexedSatelliteInfo = std::tuple<V3D, V3D, V3D, double>;

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

std::optional<IndexedSatelliteInfo> indexSatellite(const V3D &mainHKL, const int maxOrder,
                                                   const std::vector<V3D> &modVectors, const double tolerance,
                                                   const bool crossTerms) {
  const auto offsets = generateOffsetVectors(modVectors, maxOrder, crossTerms);
  bool foundSatellite{false};
  V3D indexedIntHKL, indexedMNP, fractionalOffset;
  for (const auto &mnpOffset : offsets) {
    const auto candidateIntHKL = mainHKL - std::get<3>(mnpOffset);
    const V3D candidateMNP{std::get<0>(mnpOffset), std::get<1>(mnpOffset), std::get<2>(mnpOffset)};
    if (IndexingUtils::ValidIndex(candidateIntHKL, tolerance)) {
      indexedIntHKL = candidateIntHKL;
      indexedMNP = candidateMNP;
      fractionalOffset = std::get<3>(mnpOffset);
      foundSatellite = true;
      // we deliberately don't break and use the last valid
      // reflection we find.
    }
  }
  if (foundSatellite)
    return std::make_tuple(fractionalOffset, indexedIntHKL, indexedMNP, indexedIntHKL.hklError());
  else
    return std::nullopt;
}

/**
 * Index the main reflections on the workspace using the given UB matrix
 * @param peaks Vector of pointer to peaks
 * @param ub A UB matrix to define the transform from Q_sample to hkl
 * @param tolerance If an index is within this tolerance of an integer then
 * accept it
 * @param optimizeUB If true optimize the UB for these peaks
 * @param satelliteArgs If set, attempt to index peaks as satellies if main
 * indexing fails
 * @return A CombinedIndexingStats detailing the output found
 */
CombinedIndexingStats indexPeaks(const std::vector<IPeak *> &peaks, DblMatrix ub, const double mainTolerance,
                                 const bool roundHKLs, const bool optimizeUB,
                                 const Prop::SatelliteIndexingArgs &satelliteArgs) {
  const auto nPeaks = peaks.size();
  std::vector<V3D> qSample(nPeaks);
  std::generate(std::begin(qSample), std::end(qSample),
                [&peaks, i = 0u]() mutable { return peaks[i++]->getQSampleFrame(); });
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
      auto result = indexSatellite(nominalHKL, satelliteArgs.maxOrder, satelliteArgs.modVectors,
                                   satelliteArgs.tolerance, satelliteArgs.crossTerms);
      if (result) {
        const auto &satelliteInfo = result.value();
        V3D hkl;
        ;
        if (roundHKLs) {
          hkl = std::get<1>(satelliteInfo);
          IndexingUtils::RoundHKL(hkl);
          hkl += std::get<0>(satelliteInfo);
        } else {
          hkl = nominalHKL;
        }
        peak->setHKL(hkl);
        peak->setIntHKL(std::get<1>(satelliteInfo));
        peak->setIntMNP(std::get<2>(satelliteInfo));
        stats.satellites.numIndexed++;
        stats.satellites.error += std::get<3>(satelliteInfo) / 3.;
      } else {
        // clear these to make sure leftover values from previous index peaks
        // run are not used
        peak->setHKL(V3D(0, 0, 0));
        peak->setIntHKL(V3D(0, 0, 0));
        peak->setIntMNP(V3D(0, 0, 0));
      }
    } else {
      peak->setHKL(V3D(0, 0, 0));
      peak->setIntHKL(V3D(0, 0, 0));
      peak->setIntMNP(V3D(0, 0, 0));
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
void logIndexingResults(std::ostream &out, const CombinedIndexingStats &indexingInfo, const int runNo,
                        const size_t nPeaksTotal, const Prop::IndexPeaksArgs &args) {
  if (runNo >= 0)
    out << "Run " << runNo;
  else
    out << "All runs";
  out << " indexed " << indexingInfo.totalNumIndexed() << " peaks out of " << nPeaksTotal;
  if (args.satellites.maxOrder > 0) {
    out << " of which, " << indexingInfo.main.numIndexed << " main Bragg peaks are indexed with tolerance of "
        << args.mainTolerance << ", " << indexingInfo.satellites.numIndexed
        << " satellite peaks are indexed with tolerance of " << args.satellites.tolerance << '\n';
    out << "  Average error in h,k,l for indexed peaks =  " << indexingInfo.averageError() << '\n';
    out << "  Average error in h,k,l for indexed main peaks =  " << indexingInfo.main.error << '\n';
    out << "  Average error in h,k,l for indexed satellite peaks =  " << indexingInfo.satellites.error << '\n';
  } else {
    out << " with tolerance of " << args.mainTolerance << '\n';
    out << "  Average error in h,k,l for indexed peaks =  " << indexingInfo.mainError() << '\n';
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
  this->declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace_sptr::element_type>>(Prop::PEAKSWORKSPACE,
                                                                                                "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositiveDbl = std::make_shared<BoundedValidator<double>>();
  mustBePositiveDbl->setLower(0.0);
  this->declareProperty(Prop::TOLERANCE, 0.15, mustBePositiveDbl, "Main peak indexing tolerance", Direction::Input);
  this->declareProperty(Prop::SATE_TOLERANCE, 0.15, mustBePositiveDbl, "Satellite peak indexing tolerance",
                        Direction::Input);
  this->declareProperty(Prop::ROUNDHKLS, true, "Round H, K and L values to integers");
  this->declareProperty(Prop::COMMONUB, false, "Index all orientations with a common UB");
  ModulationProperties::appendTo(this);
  this->declareProperty(Prop::SAVEMODINFO, false,
                        "If true, update the OrientedLattice with the maxOrder, "
                        "modulation vectors & cross terms values input to the algorithm");

  // -- outputs --
  this->declareProperty(Prop::NUM_INDEXED, 0, "Gets set with the number of indexed peaks.", Direction::Output);
  this->declareProperty(Prop::AVERAGE_ERR, 0.0, "Gets set with the average HKL indexing error.", Direction::Output);
  this->declareProperty(Prop::MAIN_NUM_INDEXED, 0, "Gets set with the number of indexed main peaks.",
                        Direction::Output);
  this->declareProperty(Prop::SATE_NUM_INDEXED, 0, "Gets set with the number of indexed main peaks.",
                        Direction::Output);
  this->declareProperty(Prop::MAIN_ERR, 0.0, "Gets set with the average HKL indexing error of Main Peaks.",
                        Direction::Output);
  this->declareProperty(Prop::SATE_ERR, 0.0, "Gets set with the average HKL indexing error of Satellite Peaks.",
                        Direction::Output);
}

/**
 * Validate all inputs once set
 * @return A map of property name to help message
 */
std::map<std::string, std::string> IndexPeaks::validateInputs() {
  std::map<std::string, std::string> helpMsgs;

  IPeaksWorkspace_sptr ws = this->getProperty(Prop::PEAKSWORKSPACE);
  try {
    ws->sample().getOrientedLattice();
  } catch (std::runtime_error &) {
    helpMsgs[Prop::PEAKSWORKSPACE] = "No UB Matrix defined in the lattice.";
    return helpMsgs;
  }

  const auto args = Prop::IndexPeaksArgs::parse(*this);
  const bool isSave = this->getProperty(Prop::SAVEMODINFO);
  const bool isMOZero = (args.satellites.maxOrder == 0);
  bool isAllVecZero = true;
  // parse() validates all the mod vectors. There should not be any modulated
  // vector in modVectors is equal to (0, 0, 0)
  for (size_t vecNo = 0; vecNo < args.satellites.modVectors.size(); vecNo++) {
    if (args.satellites.modVectors[vecNo] != V3D(0.0, 0.0, 0.0)) {
      isAllVecZero = false;
    } else {
      g_log.warning() << "Mod vector " << vecNo << " is invalid (0, 0, 0)"
                      << "\n";
    }
  }
  if (isMOZero && !isAllVecZero) {
    helpMsgs["MaxOrder"] = "Max Order cannot be zero if a Modulation Vector has been supplied.";
  }
  if (!isMOZero && isAllVecZero) {
    helpMsgs["ModVector1"] = "At least one Modulation Vector must be supplied if Max Order set.";
  }
  if (isSave && isAllVecZero) {
    helpMsgs[Prop::SAVEMODINFO] = "Modulation info cannot be saved with no "
                                  "valid Modulation Vectors supplied.";
  }
  if (isSave && isMOZero) {
    helpMsgs["MaxOrder"] = "Modulation info cannot be saved with Max Order = 0.";
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

    if (args.satellites.modVectors.size() >= 1) {
      lattice.setModVec1(args.satellites.modVectors[0]);
    } else {
      g_log.warning("empty modVector 1, skipping saving");
      lattice.setModVec1(V3D(0.0, 0.0, 0.0));
    }

    if (args.satellites.modVectors.size() >= 2) {
      lattice.setModVec2(args.satellites.modVectors[1]);
    } else {
      g_log.warning("empty modVector 2, skipping saving");
      lattice.setModVec2(V3D(0.0, 0.0, 0.0));
    }

    if (args.satellites.modVectors.size() >= 3) {
      lattice.setModVec3(args.satellites.modVectors[2]);
    } else {
      g_log.warning("empty modVector 3, skipping saving");
      lattice.setModVec3(V3D(0.0, 0.0, 0.0));
    }
    // set modUB now mod vectors populated
    lattice.setModUB(lattice.getUB() * lattice.getModHKL());
  }

  CombinedIndexingStats indexingInfo;
  const auto &lattice = args.workspace->sample().getOrientedLattice();
  const auto &sampleUB = lattice.getUB();
  if (args.commonUB) {
    // Use sample UB an all peaks regardless of run
    std::vector<IPeak *> allPeaksRef;
    allPeaksRef.reserve(args.workspace->getNumberPeaks());
    for (int i = 0; i < args.workspace->getNumberPeaks(); i++) {
      allPeaksRef.emplace_back(args.workspace->getPeakPtr(i));
    }
    const bool optimizeUB{false};
    indexingInfo = indexPeaks(allPeaksRef, sampleUB, args.mainTolerance, args.roundHKLs, optimizeUB, args.satellites);
  } else {
    // Use a UB optimized for each run
    std::unordered_map<int, std::vector<IPeak *>> peaksPerRun;
    for (int i = 0; i < args.workspace->getNumberPeaks(); i++)
      peaksPerRun[args.workspace->getPeak(i).getRunNumber()].emplace_back(args.workspace->getPeakPtr(i));
    if (peaksPerRun.size() < 2) {
      g_log.warning("Peaks from only one run exist but CommonUBForAll=True so peaks will be indexed with an optimised "
                    "UB which will not be saved in the workspace.");
    }
    const bool optimizeUB{true};
    for (const auto &runPeaks : peaksPerRun) {
      const auto &peaks = runPeaks.second;
      const auto indexedInRun =
          indexPeaks(peaks, sampleUB, args.mainTolerance, args.roundHKLs, optimizeUB, args.satellites);
      logIndexingResults(g_log.notice(), indexedInRun, runPeaks.first, peaks.size(), args);
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
  logIndexingResults(g_log.notice(), indexingInfo, -1, args.workspace->getNumberPeaks(), args);
  // Show the lattice parameters
  g_log.notice() << args.workspace->sample().getOrientedLattice() << "\n";
}

} // namespace Mantid::Crystal
