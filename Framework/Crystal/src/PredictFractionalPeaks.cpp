// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <boost/math/special_functions/round.hpp>

using Mantid::API::Algorithm;
using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

namespace {

namespace PropertyNames {
const std::string PEAKS{"Peaks"};
const std::string HOFFSET{"Hoffset"};
const std::string KOFFSET{"Koffset"};
const std::string LOFFSET{"Loffset"};
const std::string INCLUDEPEAKSINRANGE{"IncludeAllPeaksInRange"};
const std::string HMIN{"Hmin"};
const std::string HMAX{"Hmax"};
const std::string KMIN{"Kmin"};
const std::string KMAX{"Kmax"};
const std::string LMIN{"Lmin"};
const std::string LMAX{"Lmax"};
const std::string FRACPEAKS{"FracPeaks"};
} // namespace PropertyNames

/**
 * Defines a type to capture details required
 * to perform a fractional peak search given a HKL range
 */
class PeaksInRangeStrategy {
public:
  PeaksInRangeStrategy(V3D hklMin, V3D hklMax,
                       const PeaksWorkspace *const inputPeaks)
      : m_hklMin{std::move(hklMin)}, m_hklMax{std::move(hklMax)},
        m_inputPeaks{inputPeaks} {}

  Progress createProgressReporter(Algorithm *const alg) const noexcept {
    const int nHKLCombinations = boost::math::iround(
        (m_hklMax[0] - m_hklMin[0] + 1) * (m_hklMax[1] - m_hklMin[1] + 1) *
        (m_hklMax[2] - m_hklMin[2] + 1));
    return Progress(alg, 0.0, 1.0, nHKLCombinations);
  }

  void initialHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) const
      noexcept {
    *hkl = m_hklMin;
    *gonioMatrix = m_inputPeaks->run().getGoniometer().getR();
    *runNumber = m_inputPeaks->getPeak(0).getRunNumber();
  }

  /// Compute the next HKL value in the range
  bool nextHKL(V3D *hkl, DblMatrix * /*gonioMatrix*/, int * /*runNumber*/) const
      noexcept {
    bool canContinue{true};

    auto &currentHKL{*hkl};
    currentHKL[0]++;
    if (currentHKL[0] > m_hklMax[0]) {
      currentHKL[0] = m_hklMin[0];
      currentHKL[1]++;
      if (currentHKL[1] > m_hklMax[1]) {
        currentHKL[1] = m_hklMin[1];
        currentHKL[2]++;
        if (currentHKL[2] > m_hklMax[2])
          canContinue = false;
      }
    }
    return canContinue;
  }

private:
  V3D m_hklMin, m_hklMax;
  const PeaksWorkspace *const m_inputPeaks;
};

/**
 * Defines a type to capture details required
 * to perform a fractional peak search given an input
 * peaks workspace with peaks already indexed.
 */
class PeaksFromIndexedStrategy {
public:
  explicit PeaksFromIndexedStrategy(const PeaksWorkspace *const inputPeaks)
      : m_inputPeaks{inputPeaks}, m_currentPeak{0} {}

  Progress createProgressReporter(Algorithm *const alg) const noexcept {
    return Progress(alg, 0.0, 1.0, m_inputPeaks->getNumberPeaks());
  }

  void initialHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) const
      noexcept {
    const auto &initialPeak = m_inputPeaks->getPeak(m_currentPeak);
    *hkl = initialPeak.getHKL();
    *gonioMatrix = initialPeak.getGoniometerMatrix();
    *runNumber = initialPeak.getRunNumber();
  }

  /// Compute the next HKL value in the range
  bool nextHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) const
      noexcept {
    bool canContinue{true};
    m_currentPeak++;
    if (m_currentPeak < m_inputPeaks->getNumberPeaks()) {
      const auto &initialPeak = m_inputPeaks->getPeak(m_currentPeak);
      *hkl = initialPeak.getHKL();
      *gonioMatrix = initialPeak.getGoniometerMatrix();
      *runNumber = initialPeak.getRunNumber();
    } else {
      canContinue = false;
    }

    return canContinue;
  }

private:
  const PeaksWorkspace *const m_inputPeaks;
  mutable int m_currentPeak;
};

/**
 * Predict fractional peaks in the range specified by [hklMin, hklMax] and
 * add them to a new PeaksWorkspace
 * @param alg The host algorithm pointer
 * @param hOffsets Offsets to apply to HKL in H direction
 * @param kOffsets Offsets to apply to HKL in K direction
 * @param lOffsets Offsets to apply to HKL in L direction
 * @param strategy An object defining wgere to start the search and how to
 * advance to the next HKL
 * @return A new PeaksWorkspace containing the predicted fractional peaks
 */
template <typename SearchStrategy>
IPeaksWorkspace_sptr
predictPeaks(Algorithm *const alg, const std::vector<double> &hOffsets,
             const std::vector<double> &kOffsets,
             const std::vector<double> &lOffsets,
             const PeaksWorkspace &inputPeaks, const SearchStrategy &strategy) {
  using Mantid::API::WorkspaceFactory;
  auto outPeaks = WorkspaceFactory::Instance().createPeaks();
  const auto instrument = inputPeaks.getInstrument();
  outPeaks->setInstrument(instrument);

  using Mantid::Geometry::InstrumentRayTracer;
  const InstrumentRayTracer tracer(instrument);
  const auto &UB = inputPeaks.sample().getOrientedLattice().getUB();
  using PeakHash = std::array<int, 4>;
  std::vector<PeakHash> alreadyDonePeaks;

  V3D currentHKL;
  DblMatrix gonioMatrix;
  int runNumber{0};
  strategy.initialHKL(&currentHKL, &gonioMatrix, &runNumber);
  auto progressReporter = strategy.createProgressReporter(alg);
  while (true) {
    for (double hOffset : hOffsets) {
      for (double kOffset : kOffsets) {
        for (double lOffset : lOffsets) {
          const V3D candidateHKL(currentHKL[0] + hOffset,
                                 currentHKL[1] + kOffset,
                                 currentHKL[2] + lOffset);
          const V3D qSample = (gonioMatrix * UB * candidateHKL) * 2 * M_PI;
          if (qSample[2] <= 0)
            continue;

          using Mantid::Geometry::IPeak;
          std::unique_ptr<IPeak> peak;
          try {
            peak = inputPeaks.createPeak(qSample, 1);
          } catch (std::invalid_argument &) {
            continue;
          }

          peak->setGoniometerMatrix(gonioMatrix);
          if (peak->findDetector(tracer)) {
            PeakHash savedPeak{runNumber,
                               boost::math::iround(1000.0 * candidateHKL[0]),
                               boost::math::iround(1000.0 * candidateHKL[1]),
                               boost::math::iround(1000.0 * candidateHKL[2])};
            auto it = find(alreadyDonePeaks.begin(), alreadyDonePeaks.end(),
                           savedPeak);
            if (it == alreadyDonePeaks.end())
              alreadyDonePeaks.push_back(savedPeak);
            else
              continue;

            peak->setHKL(candidateHKL);
            peak->setRunNumber(runNumber);
            outPeaks->addPeak(*peak);
          }
        }
      }
    }
    progressReporter.report();
    if (!strategy.nextHKL(&currentHKL, &gonioMatrix, &runNumber))
      break;
  }

  return outPeaks;
}

} // namespace

namespace Mantid {
namespace Crystal {

DECLARE_ALGORITHM(PredictFractionalPeaks)

/// Initialise the properties
void PredictFractionalPeaks::init() {
  using API::WorkspaceProperty;
  using Kernel::Direction;
  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace_sptr::element_type>>(
          PropertyNames::PEAKS, "", Direction::Input),
      "Workspace of Peaks with orientation matrix that indexed the peaks and "
      "instrument loaded");

  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(
                      PropertyNames::HOFFSET, "-0.5,0.0,0.5"),
                  "Offset in the h direction");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(
                      PropertyNames::KOFFSET, "0"),
                  "Offset in the k direction");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>(
                      PropertyNames::LOFFSET, "-0.5,0.5"),
                  "Offset in the h direction");
  declareProperty(PropertyNames::INCLUDEPEAKSINRANGE, false,
                  "If false only offsets from peaks from Peaks are used");
  declareProperty(PropertyNames::HMIN, -8.0,
                  "Minimum H value to use during search", Direction::Input);
  declareProperty(PropertyNames::HMAX, 8.0,
                  "Maximum H value to use during search", Direction::Input);
  declareProperty(PropertyNames::KMIN, -8.0,
                  "Minimum K value to use during search", Direction::Input);
  declareProperty(PropertyNames::KMAX, 8.0,
                  "Maximum K value to use during search", Direction::Input);
  declareProperty(PropertyNames::LMIN, -8.0,
                  "Minimum L value to use during search", Direction::Input);
  declareProperty(PropertyNames::LMAX, 8.0,
                  "Maximum L value to use during search", Direction::Input);

  setPropertySettings(
      PropertyNames::HMIN,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      PropertyNames::HMAX,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      PropertyNames::KMIN,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      PropertyNames::KMAX,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      PropertyNames::KMAX,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      PropertyNames::LMAX,
      std::make_unique<Kernel::EnabledWhenProperty>(
          std::string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  // Outputs
  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace_sptr::element_type>>(
          PropertyNames::FRACPEAKS, "", Direction::Output),
      "Workspace of Peaks with peaks with fractional h,k, and/or l values");
}

std::map<std::string, std::string> PredictFractionalPeaks::validateInputs() {
  std::map<std::string, std::string> helpMessages;
  PeaksWorkspace_sptr peaks = getProperty(PropertyNames::PEAKS);
  if (peaks->getNumberPeaks() <= 0) {
    helpMessages[PropertyNames::PEAKS] = "Input workspace has no peaks.";
  }
  return helpMessages;
}

void PredictFractionalPeaks::exec() {
  PeaksWorkspace_sptr inputPeaks = getProperty(PropertyNames::PEAKS);
  std::vector<double> hOffsets = getProperty(PropertyNames::HOFFSET);
  std::vector<double> kOffsets = getProperty(PropertyNames::KOFFSET);
  std::vector<double> lOffsets = getProperty(PropertyNames::LOFFSET);
  if (hOffsets.empty())
    hOffsets.push_back(0.0);
  if (kOffsets.empty())
    kOffsets.push_back(0.0);
  if (lOffsets.empty())
    lOffsets.push_back(0.0);
  const bool includePeaksInRange = getProperty("IncludeAllPeaksInRange");
  const V3D hklMin{getProperty(PropertyNames::HMIN),
                   getProperty(PropertyNames::KMIN),
                   getProperty(PropertyNames::LMIN)};
  const V3D hklMax{getProperty(PropertyNames::HMAX),
                   getProperty(PropertyNames::KMAX),
                   getProperty(PropertyNames::LMAX)};

  IPeaksWorkspace_sptr outPeaks;
  if (includePeaksInRange) {
    outPeaks =
        predictPeaks(this, hOffsets, kOffsets, lOffsets, *inputPeaks,
                     PeaksInRangeStrategy(hklMin, hklMax, inputPeaks.get()));

  } else {
    outPeaks = predictPeaks(this, hOffsets, kOffsets, lOffsets, *inputPeaks,
                            PeaksFromIndexedStrategy(inputPeaks.get()));
  }
  setProperty(PropertyNames::FRACPEAKS, outPeaks);
}

} // namespace Crystal
} // namespace Mantid
