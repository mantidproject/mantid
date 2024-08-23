// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindSXPeaks.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <vector>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Crystal::FindSXPeaksHelper;

namespace Mantid::Crystal {

const std::string FindSXPeaks::strongestPeakStrategy = "StrongestPeakOnly";
const std::string FindSXPeaks::allPeaksStrategy = "AllPeaks";
const std::string FindSXPeaks::allPeaksNSigmaStrategy = "AllPeaksNSigma";

const std::string FindSXPeaks::relativeResolutionStrategy = "RelativeResolution";
const std::string FindSXPeaks::absoluteResolutionPeaksStrategy = "AbsoluteResolution";

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXPeaks)

using namespace Kernel;
using namespace API;
using Mantid::Geometry::IPeak_uptr;

FindSXPeaks::FindSXPeaks()
    : API::Algorithm(), m_MinRange(DBL_MAX), m_MaxRange(-DBL_MAX), m_MinWsIndex(0), m_MaxWsIndex(0) {}

/** Initialisation method.
 *
 */
void FindSXPeaks::init() {
  auto wsValidation = std::make_shared<CompositeValidator>();
  wsValidation->add<HistogramValidator>();

  auto unitValidation = std::make_shared<CompositeValidator>(CompositeRelation::OR);
  unitValidation->add<WorkspaceUnitValidator>("TOF");
  unitValidation->add<WorkspaceUnitValidator>("dSpacing");

  wsValidation->add(unitValidation);

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidation),
                  "The name of the Workspace2D to take as input");
  declareProperty("RangeLower", EMPTY_DBL(), "The X value to search from (default 0)");
  declareProperty("RangeUpper", EMPTY_DBL(), "The X value to search to (default total number of bins)");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive, "Start workspace index (default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "End workspace index (default to total number of histograms)");

  // ---------------------------------------------------------------
  // Peak strategies + Threshold
  // ---------------------------------------------------------------
  auto mustBePositiveDouble = std::make_shared<BoundedValidator<double>>();
  mustBePositiveDouble->setLower(0.0);

  std::vector<std::string> peakFindingStrategy = {strongestPeakStrategy, allPeaksStrategy, allPeaksNSigmaStrategy};
  declareProperty("PeakFindingStrategy", strongestPeakStrategy,
                  std::make_shared<StringListValidator>(peakFindingStrategy),
                  "Different options for peak finding."
                  "1. StrongestPeakOnly: Looks only for the strongest peak in each "
                  "spectrum (provided there is "
                  "one). This options is more performant than the AllPeaks option.\n"
                  "2. AllPeaks: This strategy will find all peaks in each "
                  "spectrum. This is slower than StrongestPeakOnly. Note that the "
                  "recommended ResolutionStrategy in this mode is AbsoluteResolution.\n"
                  "3. AllPeaksNSigma: This stratergy will look for peaks by bins that are"
                  " more than nsigma different in intensity. Note that the "
                  "recommended ResolutionStrategy in this mode is AbsoluteResolution.\n");

  // Declare
  declareProperty("SignalBackground", 10.0, mustBePositiveDouble,
                  "Multiplication factor for the signal background. Peaks which are"
                  " below the estimated background are discarded. The background is "
                  "estimated"
                  " to be an average of the first and the last signal and multiplied"
                  " by the SignalBackground property.\n");

  declareProperty("AbsoluteBackground", 30.0, mustBePositiveDouble,
                  "Peaks which are below the specified absolute background are discarded."
                  " The background is gloabally specified for all spectra. Inspect your "
                  "data in the InstrumentView to get a good feeling for the background "
                  "threshold.\n"
                  "Background thresholds which are too low will mistake noise for peaks.");

  declareProperty(
      "NSigma", 5.0, mustBePositiveDouble,
      "Multiplication factor on error used to compare the difference in intensity between consecutive bins.");

  // Enable
  setPropertySettings("SignalBackground", std::make_unique<EnabledWhenProperty>(
                                              "PeakFindingStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                              strongestPeakStrategy));

  setPropertySettings("AbsoluteBackground",
                      std::make_unique<EnabledWhenProperty>(
                          "PeakFindingStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO, allPeaksStrategy));

  setPropertySettings("NSigma", std::make_unique<EnabledWhenProperty>("PeakFindingStrategy",
                                                                      Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                                                      allPeaksNSigmaStrategy));

  // Group
  const std::string peakGroup = "Peak Finding Settings";
  setPropertyGroup("PeakFindingStrategy", peakGroup);
  setPropertyGroup("SignalBackground", peakGroup);
  setPropertyGroup("AbsoluteBackground", peakGroup);
  setPropertyGroup("NSigma", peakGroup);

  // ---------------------------------------------------------------
  // Resolution
  // ---------------------------------------------------------------
  // Declare
  std::vector<std::string> resolutionStrategy = {relativeResolutionStrategy, absoluteResolutionPeaksStrategy};
  declareProperty("ResolutionStrategy", relativeResolutionStrategy,
                  std::make_shared<StringListValidator>(resolutionStrategy),
                  "Different options for the resolution."
                  "1. RelativeResolution: This defines a relative tolerance "
                  "needed to avoid peak duplication in number of pixels. "
                  "This selection will enable the Resolution property and "
                  "disable the XResolution, PhiResolution, ThetaResolution.\n"
                  "1. AbsoluteResolution: This defines an absolute tolerance "
                  "needed to avoid peak duplication in number of pixels. "
                  "This selection will disable the Resolution property and "
                  "enable the XResolution, PhiResolution, "
                  "ThetaResolution.\n");

  declareProperty("Resolution", 0.01, mustBePositiveDouble,
                  "Tolerance needed to avoid peak duplication in number of pixels");

  declareProperty("XResolution", 0., mustBePositiveDouble,
                  "Absolute tolerance in time-of-flight or d-spacing needed to avoid peak "
                  "duplication in number of pixels. The values are specified "
                  "in either microseconds or angstroms.");

  declareProperty("PhiResolution", 1., mustBePositiveDouble,
                  "Absolute tolerance in the phi "
                  "coordinate needed to avoid peak "
                  "duplication in number of pixels. The "
                  "values are specified in degrees.");

  declareProperty("TwoThetaResolution", 1., mustBePositiveDouble,
                  "Absolute tolerance of two theta value needed to avoid peak "
                  "duplication in number of pixels. The values are specified "
                  "in degrees.");

  // Enable
  setPropertySettings("Resolution", std::make_unique<EnabledWhenProperty>(
                                        "ResolutionStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                        relativeResolutionStrategy));

  setPropertySettings("XResolution", std::make_unique<EnabledWhenProperty>(
                                         "ResolutionStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                         absoluteResolutionPeaksStrategy));

  setPropertySettings("PhiResolution", std::make_unique<EnabledWhenProperty>(
                                           "ResolutionStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                           absoluteResolutionPeaksStrategy));

  setPropertySettings("TwoThetaResolution", std::make_unique<EnabledWhenProperty>(
                                                "ResolutionStrategy", Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                                                absoluteResolutionPeaksStrategy));

  // Group
  const std::string resolutionGroup = "Resolution Settings";
  setPropertyGroup("ResolutionStrategy", resolutionGroup);
  setPropertyGroup("Resolution", resolutionGroup);
  setPropertyGroup("XResolution", resolutionGroup);
  setPropertyGroup("PhiResolution", resolutionGroup);
  setPropertyGroup("TwoThetaResolution", resolutionGroup);

  // Declare
  declareProperty("MinNBinsPerPeak", EMPTY_INT(), mustBePositive,
                  "Minimum number of bins contributing to a peak in an individual spectrum");

  declareProperty("MinNSpectraPerPeak", EMPTY_INT(), mustBePositive,
                  "Minimum number of spectra contributing to a peak after they are grouped");

  declareProperty("MaxNSpectraPerPeak", EMPTY_INT(), mustBePositive,
                  "Maximum number of spectra contributing to a peak after they are grouped");

  // Group
  const std::string peakValidationGroup = "Peak Validation Settings";
  setPropertyGroup("MinNBinsPerPeak", peakValidationGroup);
  setPropertyGroup("MinNSpectraPerPeak", peakValidationGroup);
  setPropertyGroup("MaxNSpectraPerPeak", peakValidationGroup);

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the PeaksWorkspace in which to store the list "
                  "of peaks found");

  // Create the output peaks workspace
  m_peaks.reset(new PeaksWorkspace);
}

/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> FindSXPeaks::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;
  const std::string resolutionStrategy = getProperty("ResolutionStrategy");
  const auto xResolutionProperty = getPointerToProperty("XResolution");

  // Check that the user has set a valid value for the x resolution when
  // in absolute resolution mode.
  if (resolutionStrategy == FindSXPeaks::absoluteResolutionPeaksStrategy && xResolutionProperty->isDefault()) {
    validationOutput["XResolution"] = "XResolution must be set to a value greater than 0";
  }

  const int minNSpectraPerPeak = getProperty("MinNSpectraPerPeak");
  const int maxNSpectraPerPeak = getProperty("MaxNSpectraPerPeak");
  if (!isEmpty(minNSpectraPerPeak) && !isEmpty(maxNSpectraPerPeak)) {
    if (maxNSpectraPerPeak < minNSpectraPerPeak) {
      validationOutput["MaxNSpectraPerPeak"] = "MaxNSpectraPerPeak must be greater than MinNSpectraPerPeak";
      validationOutput["MinNSpectraPerPeak"] = "MinNSpectraPerPeak must be lower than MaxNSpectraPerPeak";
    }
  }

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  if (inputWorkspace) {
    const int minWsIndex = getProperty("StartWorkspaceIndex");
    const int maxWsIndex = getProperty("EndWorkspaceIndex");
    size_t numberOfSpectraToConsider =
        !isEmpty(minWsIndex) ? (!isEmpty(maxWsIndex) ? (maxWsIndex - minWsIndex + 1)
                                                     : (inputWorkspace->getNumberHistograms() - minWsIndex))
                             : (!isEmpty(maxWsIndex) ? (maxWsIndex + 1) : (inputWorkspace->getNumberHistograms()));

    if (!isEmpty(minNSpectraPerPeak)) {
      if (static_cast<int>(numberOfSpectraToConsider) < minNSpectraPerPeak) {
        validationOutput["MinNSpectraPerPeak"] =
            "MinNSpectraPerPeak must be less than the number of spectrums considered in InputWorkspace";
      }
    }

    if (!isEmpty(maxNSpectraPerPeak)) {
      if (static_cast<int>(numberOfSpectraToConsider) < maxNSpectraPerPeak) {
        validationOutput["MaxNSpectraPerPeak"] =
            "MaxNSpectraPerPeak must be less than the number of spectrums considered in InputWorkspace";
      }
    }

    const int minNBinsPerPeak = getProperty("MinNBinsPerPeak");
    if (!isEmpty(minNBinsPerPeak)) {
      if (minNBinsPerPeak > static_cast<int>(inputWorkspace->getMaxNumberBins())) {
        validationOutput["MinNBinsPerPeak"] =
            "MinNBinsPerPeak must be less than the number of bins in the InputWorkspace";
      }
    }
  }

  return validationOutput;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void FindSXPeaks::exec() {
  // Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");

  // the assignment below is intended and if removed will break the unit tests
  m_MinWsIndex = static_cast<int>(getProperty("StartWorkspaceIndex"));
  m_MaxWsIndex = static_cast<int>(getProperty("EndWorkspaceIndex"));

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  // copy the instrument across. Cannot generate peaks without doing this
  // first.
  m_peaks->setInstrument(localworkspace->getInstrument());

  size_t numberOfSpectra = localworkspace->getNumberHistograms();

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (m_MinWsIndex > numberOfSpectra) {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    m_MinWsIndex = 0;
  }
  if (m_MinWsIndex > m_MaxWsIndex) {
    throw std::invalid_argument("Cannot have StartWorkspaceIndex > EndWorkspaceIndex");
  }
  if (isEmpty(m_MaxWsIndex))
    m_MaxWsIndex = numberOfSpectra - 1;
  if (m_MaxWsIndex > numberOfSpectra - 1 || m_MaxWsIndex < m_MinWsIndex) {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxWsIndex = numberOfSpectra - 1;
  }
  if (m_MinRange > m_MaxRange) {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to "
                  "frame maximum.");
    m_MaxRange = 0.0;
  }

  Progress progress(this, 0.0, 1.0, m_MaxWsIndex - m_MinWsIndex + 2);

  // Calculate the primary flight path.
  const auto &spectrumInfo = localworkspace->spectrumInfo();

  // Get the background strategy
  auto backgroundStrategy = getBackgroundStrategy();

  // Get the peak finding strategy
  const auto xUnit = getWorkspaceXAxisUnit(localworkspace);
  auto peakFindingStrategy =
      getPeakFindingStrategy(backgroundStrategy.get(), spectrumInfo, m_MinRange, m_MaxRange, xUnit);

  const int minNBinsPerPeak = getProperty("MinNBinsPerPeak");
  if (!isEmpty(minNBinsPerPeak)) {
    peakFindingStrategy->setMinNBinsPerPeak(minNBinsPerPeak);
  }

  peakvector entries;
  entries.reserve(m_MaxWsIndex - m_MinWsIndex);
  // Count the peaks so that we can resize the peak vector at the end.

  PARALLEL_FOR_IF(Kernel::threadSafe(*localworkspace))
  for (auto wsIndex = static_cast<int>(m_MinWsIndex); wsIndex <= static_cast<int>(m_MaxWsIndex); ++wsIndex) {
    PARALLEL_START_INTERRUPT_REGION

    // If no detector found / monitor, skip onto the next spectrum
    const auto wsIndexSize_t = static_cast<size_t>(wsIndex);
    if (!spectrumInfo.hasDetectors(wsIndexSize_t) || spectrumInfo.isMonitor(wsIndexSize_t)) {
      continue;
    }

    // Retrieve the spectrum into a vector
    const auto &x = localworkspace->x(wsIndex);
    const auto &y = localworkspace->y(wsIndex);
    const auto &e = localworkspace->e(wsIndex);

    // Run the peak finding strategy
    auto foundPeaks = peakFindingStrategy->findSXPeaks(x, y, e, wsIndex);
    if (!foundPeaks) {
      continue;
    }

    PARALLEL_CRITICAL(entries) { std::copy(foundPeaks->cbegin(), foundPeaks->cend(), std::back_inserter(entries)); }
    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Now reduce the list with duplicate entries
  reducePeakList(entries, progress);

  setProperty("OutputWorkspace", m_peaks);
  progress.report();
}

/**
Reduce the peak list by removing duplicates
then convert SXPeaks objects to PeakObjects and add them to the output workspace
@param pcv : current peak list containing potential duplicates
@param progress: a progress object
*/
void FindSXPeaks::reducePeakList(const peakvector &pcv, Progress &progress) {
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");
  auto &goniometerMatrix = localworkspace->run().getGoniometer().getR();
  auto compareStrategy = getCompareStrategy();
  auto reductionStrategy = getReducePeakListStrategy(compareStrategy.get());

  const int minNSpectraPerPeak = getProperty("MinNSpectraPerPeak");
  if (!isEmpty(minNSpectraPerPeak)) {
    reductionStrategy->setMinNSpectraPerPeak(minNSpectraPerPeak);
  }

  const int maxNSpectraPerPeak = getProperty("MaxNSpectraPerPeak");
  if (!isEmpty(maxNSpectraPerPeak)) {
    reductionStrategy->setMaxNSpectraPerPeak(maxNSpectraPerPeak);
  }

  auto finalv = reductionStrategy->reduce(pcv, progress);

  for (auto &finalPeak : finalv) {
    finalPeak.reduce();
    try {
      IPeak_uptr ipeak = m_peaks->createPeak(finalPeak.getQ());
      Peak_uptr peak(static_cast<Peak *>(ipeak.release()));
      if (peak) {
        peak->setIntensity(finalPeak.getIntensity());
        peak->setDetectorID(finalPeak.getDetectorId());
        peak->setGoniometerMatrix(goniometerMatrix);
        peak->setRunNumber(localworkspace->getRunNumber());
        m_peaks->addPeak(*peak);
      }
    } catch (std::exception &e) {
      g_log.error() << e.what() << '\n';
    }
  }
}

/** Get the x-axis units of the workspace
 *
 * This will return either TOF or DSPACING depending on unit ID of
 * the workspace.
 *
 * @param workspace :: the workspace to check x-axis units on
 * @return enum of type XAxisUnit with the value of TOF or DSPACING
 */
XAxisUnit FindSXPeaks::getWorkspaceXAxisUnit(const MatrixWorkspace_const_sptr &workspace) const {
  const auto xAxis = workspace->getAxis(0);
  const auto unitID = xAxis->unit()->unitID();

  if (unitID == "TOF") {
    return XAxisUnit::TOF;
  } else {
    return XAxisUnit::DSPACING;
  }
}

std::unique_ptr<BackgroundStrategy> FindSXPeaks::getBackgroundStrategy() const {
  const std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  if (peakFindingStrategy == strongestPeakStrategy) {
    const double signalBackground = getProperty("SignalBackground");
    return std::make_unique<PerSpectrumBackgroundStrategy>(signalBackground);
  } else if (peakFindingStrategy == allPeaksStrategy) {
    const double background = getProperty("AbsoluteBackground");
    return std::make_unique<AbsoluteBackgroundStrategy>(background);
  } else if (peakFindingStrategy == allPeaksNSigmaStrategy) {
    return nullptr; // AllPeaksNSigma stratergy does not require a background stratergy
  } else {
    throw std::invalid_argument("The selected background strategy has not been implemented yet.");
  }
}

std::unique_ptr<FindSXPeaksHelper::PeakFindingStrategy>
FindSXPeaks::getPeakFindingStrategy(const BackgroundStrategy *backgroundStrategy, const API::SpectrumInfo &spectrumInfo,
                                    const double minValue, const double maxValue, const XAxisUnit tofUnits) const {
  // Get the peak finding stratgy
  std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  if (peakFindingStrategy == strongestPeakStrategy) {
    return std::make_unique<StrongestPeaksStrategy>(backgroundStrategy, spectrumInfo, minValue, maxValue, tofUnits);
  } else if (peakFindingStrategy == allPeaksStrategy) {
    return std::make_unique<AllPeaksStrategy>(backgroundStrategy, spectrumInfo, minValue, maxValue, tofUnits);
  } else if (peakFindingStrategy == allPeaksNSigmaStrategy) {
    const double nsigma = getProperty("NSigma");
    return std::make_unique<NSigmaPeaksStrategy>(spectrumInfo, nsigma, minValue, maxValue, tofUnits);
  } else {
    throw std::invalid_argument("The selected peak finding strategy has not been implemented yet.");
  }
}

std::unique_ptr<FindSXPeaksHelper::ReducePeakListStrategy>
FindSXPeaks::getReducePeakListStrategy(const FindSXPeaksHelper::CompareStrategy *compareStrategy) const {
  const std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  auto useSimpleReduceStrategy = peakFindingStrategy == strongestPeakStrategy;
  if (useSimpleReduceStrategy) {
    return std::make_unique<FindSXPeaksHelper::SimpleReduceStrategy>(compareStrategy);
  } else {
    return std::make_unique<FindSXPeaksHelper::FindMaxReduceStrategy>(compareStrategy);
  }
}

std::unique_ptr<FindSXPeaksHelper::CompareStrategy> FindSXPeaks::getCompareStrategy() const {
  const std::string resolutionStrategy = getProperty("ResolutionStrategy");
  auto useRelativeResolutionStrategy = resolutionStrategy == relativeResolutionStrategy;
  if (useRelativeResolutionStrategy) {
    double resolution = getProperty("Resolution");
    return std::make_unique<FindSXPeaksHelper::RelativeCompareStrategy>(resolution);
  } else {
    double xUnitResolution = getProperty("XResolution");
    double phiResolution = getProperty("PhiResolution");
    double twoThetaResolution = getProperty("TwoThetaResolution");
    const auto tofUnits = getWorkspaceXAxisUnit(getProperty("InputWorkspace"));
    return std::make_unique<FindSXPeaksHelper::AbsoluteCompareStrategy>(xUnitResolution, phiResolution,
                                                                        twoThetaResolution, tofUnits);
  }
}

} // namespace Mantid::Crystal
