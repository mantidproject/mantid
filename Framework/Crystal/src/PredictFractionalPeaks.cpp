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
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/math/special_functions/round.hpp>

using Mantid::API::Algorithm;
using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::HKLFilter;
using Mantid::Geometry::HKLFilter_uptr;
using Mantid::Geometry::HKLGenerator;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Geometry::ReflectionCondition_sptr;
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
const std::string REFLECTION_COND{"ReflectionCondition"};
const std::string ON_DETECTOR{"RequirePeaksOnDetector"};
const std::string FRACPEAKS{"FracPeaks"};
} // namespace PropertyNames

/**
 * Defines a type to capture details required
 * to perform a fractional peak search given a HKL range
 */
class PeaksInRangeStrategy {
public:
  /**
   * @param hklMin Minimum for HKL range
   * @param hklMax Maximum for HKL range
   * @param filter An optional filter to throw away some HKL values
   * @param inputPeaks A peaks workspace used to pull the goniometer and run
   * number.
   */
  PeaksInRangeStrategy(V3D hklMin, V3D hklMax, HKLFilter *filter,
                       const PeaksWorkspace *const inputPeaks)
      : m_hklGenerator(std::move(hklMin), std::move(hklMax)),
        m_hklIterator(m_hklGenerator.begin()), m_hklFilter(filter),
        m_inputPeaks(inputPeaks) {
    assert(filter);
  }

  Progress createProgressReporter(Algorithm *const alg) const noexcept {
    return Progress(alg, 0.0, 1.0, m_hklGenerator.size());
  }

  void initialHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) const
      noexcept {
    *hkl = *(m_hklGenerator.begin());
    *gonioMatrix = m_inputPeaks->run().getGoniometer().getR();
    *runNumber = m_inputPeaks->getPeak(0).getRunNumber();
  }

  /// Compute the next HKL value in the range
  bool nextHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) noexcept {
    ++m_hklIterator;
    if (m_hklIterator != m_hklGenerator.end()) {
      if (!m_hklFilter->isAllowed(*m_hklIterator)) {
        return nextHKL(hkl, gonioMatrix, runNumber);
      } else {
        *hkl = *m_hklIterator;
        return true;
      }
    } else
      return false;
  }

private:
  HKLGenerator m_hklGenerator;
  HKLGenerator::const_iterator m_hklIterator;
  HKLFilter *m_hklFilter;
  const PeaksWorkspace *const m_inputPeaks;
}; // namespace

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
  bool nextHKL(V3D *hkl, DblMatrix *gonioMatrix, int *runNumber) noexcept {
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
  int m_currentPeak;
};

/**
 * Predict fractional peaks in the range specified by [hklMin, hklMax] and
 * add them to a new PeaksWorkspace
 * @param alg The host algorithm pointer
 * @param hOffsets Offsets to apply to HKL in H direction
 * @param kOffsets Offsets to apply to HKL in K direction
 * @param lOffsets Offsets to apply to HKL in L direction
 * @param requirePeaksOnDetector If true the peaks is required to hit a detector
 * @param inputPeaks A peaks workspace used to created new peaks. Defines the
 * instrument and metadata for the search
 * @param strategy An object defining were to start the search and how to
 * advance to the next HKL
 * @return A new PeaksWorkspace containing the predicted fractional peaks
 */
template <typename SearchStrategy>
IPeaksWorkspace_sptr
predictPeaks(Algorithm *const alg, const std::vector<double> &hOffsets,
             const std::vector<double> &kOffsets,
             const std::vector<double> &lOffsets,
             const bool requirePeaksOnDetector,
             const PeaksWorkspace &inputPeaks, SearchStrategy strategy) {
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
          const V3D qLab = (gonioMatrix * UB * candidateHKL) * 2 * M_PI;
          if (qLab[2] <= 0)
            continue;

          using Mantid::Geometry::IPeak;
          std::unique_ptr<IPeak> peak;
          try {
            peak = inputPeaks.createPeak(qLab);
          } catch (...) {
            // If we can't create a valid peak we have no choice but to skip it
            continue;
          }

          peak->setGoniometerMatrix(gonioMatrix);
          if (requirePeaksOnDetector && peak->getDetectorID() < 0)
            continue;
          GNU_DIAG_OFF("missing-braces")
          PeakHash savedPeak{runNumber,
                             boost::math::iround(1000.0 * candidateHKL[0]),
                             boost::math::iround(1000.0 * candidateHKL[1]),
                             boost::math::iround(1000.0 * candidateHKL[2])};
          GNU_DIAG_ON("missing-braces")
          auto it =
              find(alreadyDonePeaks.begin(), alreadyDonePeaks.end(), savedPeak);
          if (it == alreadyDonePeaks.end())
            alreadyDonePeaks.emplace_back(std::move(savedPeak));
          else
            continue;

          peak->setHKL(candidateHKL);
          peak->setRunNumber(runNumber);
          outPeaks->addPeak(*peak);
        }
      }
    }
    progressReporter.report();
    if (!strategy.nextHKL(&currentHKL, &gonioMatrix, &runNumber))
      break;
  }

  return outPeaks;
} // namespace

} // namespace

namespace Mantid {
namespace Crystal {

DECLARE_ALGORITHM(PredictFractionalPeaks)

/// Initialise the properties
void PredictFractionalPeaks::init() {
  using API::WorkspaceProperty;
  using Geometry::getAllReflectionConditions;
  using Kernel::Direction;
  using Kernel::StringListValidator;

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

  const auto &reflectionConditions = getAllReflectionConditions();
  std::vector<std::string> propOptions;
  propOptions.reserve(reflectionConditions.size() + 1);
  propOptions.emplace_back("");
  std::transform(reflectionConditions.cbegin(), reflectionConditions.cend(),
                 std::back_inserter(propOptions),
                 [](const auto &condition) { return condition->getName(); });
  declareProperty(PropertyNames::REFLECTION_COND, "",
                  boost::make_shared<StringListValidator>(propOptions),
                  "If provided, generate a list of possible peaks from this "
                  "reflection condition and use them to predict the fractional "
                  "peaks. This option requires a range of HKL values and "
                  "implies IncludeAllPeaksInRange=true");

  declareProperty(PropertyNames::ON_DETECTOR, true,
                  "If true then the predicted peaks are required to hit a "
                  "detector pixel. Default=true",
                  Direction::Input);

  // enable range properties if required
  using Kernel::EnabledWhenProperty;
  for (const auto &name :
       {PropertyNames::HMIN, PropertyNames::HMAX, PropertyNames::KMIN,
        PropertyNames::KMAX, PropertyNames::LMIN, PropertyNames::LMAX}) {
    EnabledWhenProperty includeInRangeEqOne{PropertyNames::INCLUDEPEAKSINRANGE,
                                            Kernel::IS_EQUAL_TO, "1"};
    EnabledWhenProperty reflConditionNotEmpty{PropertyNames::REFLECTION_COND,
                                              Kernel::IS_NOT_EQUAL_TO, ""};
    setPropertySettings(name,
                        std::make_unique<Kernel::EnabledWhenProperty>(
                            std::move(includeInRangeEqOne),
                            std::move(reflConditionNotEmpty), Kernel::OR));
  }

  // Outputs
  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace_sptr::element_type>>(
          PropertyNames::FRACPEAKS, "", Direction::Output),
      "Workspace of Peaks with peaks with fractional h,k, and/or l values");
}

std::map<std::string, std::string> PredictFractionalPeaks::validateInputs() {
  std::map<std::string, std::string> helpMessages;
  const PeaksWorkspace_sptr peaks = getProperty(PropertyNames::PEAKS);
  if (peaks && peaks->getNumberPeaks() <= 0) {
    helpMessages[PropertyNames::PEAKS] = "Input workspace has no peaks.";
  }

  auto validateRange = [&helpMessages, this](const std::string &minName,
                                             const std::string &maxName) {
    const double min{getProperty(minName)}, max{getProperty(maxName)};
    if (max < min) {
      const std::string helpMsg = "Inconsistent " + minName + "/" + maxName +
                                  ": " + maxName + " < " + minName;
      helpMessages[minName] = helpMsg;
      helpMessages[maxName] = helpMsg;
    }
  };
  validateRange(PropertyNames::HMIN, PropertyNames::HMAX);
  validateRange(PropertyNames::KMIN, PropertyNames::KMAX);
  validateRange(PropertyNames::LMIN, PropertyNames::LMAX);

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
  const std::string reflectionConditionName =
      getProperty(PropertyNames::REFLECTION_COND);
  const bool requirePeakOnDetector = getProperty(PropertyNames::ON_DETECTOR);

  IPeaksWorkspace_sptr outPeaks;
  if (includePeaksInRange || !reflectionConditionName.empty()) {
    using Mantid::Geometry::getAllReflectionConditions;
    const auto &allConditions = getAllReflectionConditions();
    const auto found =
        std::find_if(std::cbegin(allConditions), std::cend(allConditions),
                     [&reflectionConditionName](const auto &condition) {
                       return condition->getName() == reflectionConditionName;
                     });
    using Geometry::HKLFilterCentering;
    HKLFilter_uptr filter;
    if (found != std::cend(allConditions)) {
      filter = std::make_unique<HKLFilterCentering>(*found);
    } else {
      using Mantid::Geometry::HKLFilterNone;
      filter = std::make_unique<HKLFilterNone>();
    }
    outPeaks = predictPeaks(
        this, hOffsets, kOffsets, lOffsets, requirePeakOnDetector, *inputPeaks,
        PeaksInRangeStrategy(hklMin, hklMax, filter.get(), inputPeaks.get()));

  } else {
    outPeaks =
        predictPeaks(this, hOffsets, kOffsets, lOffsets, requirePeakOnDetector,
                     *inputPeaks, PeaksFromIndexedStrategy(inputPeaks.get()));
  }
  setProperty(PropertyNames::FRACPEAKS, outPeaks);
}

} // namespace Crystal
} // namespace Mantid
