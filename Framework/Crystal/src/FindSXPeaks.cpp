//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/FindSXPeaks.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <unordered_map>
#include <vector>

using namespace Mantid::DataObjects;
using namespace Mantid::Crystal::FindSXPeaksHelper;


namespace Mantid {
namespace Crystal {

const std::string FindSXPeaks::perSpectrumBackground = "PerSpectrumBackground";
const std::string FindSXPeaks::absoluteBackground = "AbsoluteBackground";


const std::string FindSXPeaks::strongestPeakStrategy = "StrongestPeakOnly";
const std::string FindSXPeaks::allPeaksStrategy = "AllPeaks";

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXPeaks)

using namespace Kernel;
using namespace API;


FindSXPeaks::FindSXPeaks()
    : API::Algorithm(), m_MinRange(DBL_MAX), m_MaxRange(-DBL_MAX),
      m_MinWsIndex(0), m_MaxWsIndex(0) {}

/** Initialisation method.
 *
 */
void FindSXPeaks::init() {
  auto wsValidation = boost::make_shared<CompositeValidator>();
  wsValidation->add<HistogramValidator>();
  wsValidation->add<WorkspaceUnitValidator>("TOF");

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidation),
                  "The name of the Workspace2D to take as input");
  declareProperty("RangeLower", EMPTY_DBL(),
                  "The X value to search from (default 0)");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The X value to search to (default FindSXPeaks)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "Start spectrum number (default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "End spectrum number  (default FindSXPeaks)");


  std::vector<std::string> peakFindingStrategy = {strongestPeakStrategy,
                                                  allPeaksStrategy};
    declareProperty(
        "PeakFindingStrategy", strongestPeakStrategy,
        boost::make_shared<StringListValidator>(peakFindingStrategy),
        "Different options for peak finding."
        "1. StrongestPeakOnly: This is a much more performant way of finding the strongest peak per spectrum (provided there is one).\n"
        "2. AllPeaks: This strategy will find all peaks in each spectrum. This is slower than StrongestPeakOnly.\n");


  declareProperty("SignalBackground", 10.0,
                  "Multiplication factor for the signal background. Peaks which are"
                  " below the estimated background are discarded. The background is estimated"
                  " to be an average of the first and the last signal and multiplied"
                  " by the SignalBackground property.\n");

  setPropertySettings("SignalBackground",
                        make_unique<EnabledWhenProperty>(
                            "PeakFindingStrategy",
                            Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                            strongestPeakStrategy));

  declareProperty(absoluteBackground, 10.0,
                  "Peaks which are below the specified absolute background are discarded."
                  " The background is specified for all spectra.");

  setPropertySettings(absoluteBackground,
                        make_unique<EnabledWhenProperty>(
                            "PeakFindingStrategy",
                            Mantid::Kernel::ePropertyCriterion::IS_EQUAL_TO,
                            allPeaksStrategy));

  declareProperty(
      "Resolution", 0.01,
      "Tolerance needed to avoid peak duplication in number of pixels");
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the PeaksWorkspace in which to store the list "
                  "of peaks found");

  // Create the output peaks workspace
  m_peaks.reset(new PeaksWorkspace);
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
    throw std::invalid_argument(
        "Cannot have StartWorkspaceIndex > EndWorkspaceIndex");
  }
  if (isEmpty(m_MaxWsIndex))
    m_MaxWsIndex = numberOfSpectra - 1;
  if (m_MaxWsIndex > numberOfSpectra - 1 || m_MaxWsIndex < m_MinWsIndex) {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxWsIndex = numberOfSpectra;
  }
  if (m_MinRange > m_MaxRange) {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to "
                  "frame maximum.");
    m_MaxRange = 0.0;
  }

  Progress progress(this, 0.0, 1.0, m_MaxWsIndex - m_MinWsIndex + 1);

  // Calculate the primary flight path.
  const auto &spectrumInfo = localworkspace->spectrumInfo();

  // Get the background strategy
  auto backgroundStrategy = getBackgroundStrategy();

  // Get the peak finding strategy
  auto peakFindingStrategy = getPeakFindingStrategy(backgroundStrategy.get(), spectrumInfo, m_MinRange, m_MaxRange);

  peakvector entries;
  entries.reserve(m_MaxWsIndex - m_MinWsIndex);
  // Count the peaks so that we can resize the peak vector at the end.
  PARALLEL_FOR_IF(Kernel::threadSafe(*localworkspace))
  for (int wsIndex = static_cast<int>(m_MinWsIndex);
       wsIndex <= static_cast<int>(m_MaxWsIndex); ++wsIndex) {
    PARALLEL_START_INTERUPT_REGION

    // If no detector found / monitor, skip onto the next spectrum
    const size_t wsIndexSize_t = static_cast<size_t>(wsIndex);
    if (!spectrumInfo.hasDetectors(wsIndexSize_t) ||
        spectrumInfo.isMonitor(wsIndexSize_t)) {
      continue;
    }

    // Retrieve the spectrum into a vector
    const auto &x = localworkspace->x(wsIndex);
    const auto &y = localworkspace->y(wsIndex);

    // Run the peak finding strategy
    auto foundPeaks = peakFindingStrategy->findSXPeaks(x, y, wsIndex);
    if (!foundPeaks) {
      continue;
    }

    PARALLEL_CRITICAL(entries) {
      for (const auto& peak : *foundPeaks) {
        entries.push_back(peak);
      }
    }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Now reduce the list with duplicate entries
  reducePeakList(entries);

  setProperty("OutputWorkspace", m_peaks);
  progress.report();
}


/**
Reduce the peak list by removing duplicates
then convert SXPeaks objects to PeakObjects and add them to the output workspace
@param pcv : current peak list containing potential duplicates
*/
void FindSXPeaks::reducePeakList(const peakvector &pcv) {
  double resolution = getProperty("Resolution");
  auto reductionStrategy = getReducePeakListStrategy();
  auto finalv = reductionStrategy->reduce(pcv, resolution);

  for (auto &finalPeak : finalv) {
    finalPeak.reduce();
    try {
      Geometry::IPeak *peak = m_peaks->createPeak(finalPeak.getQ());
      if (peak) {
        peak->setIntensity(finalPeak.getIntensity());
        peak->setDetectorID(finalPeak.getDetectorId());
        m_peaks->addPeak(*peak);
        delete peak;
      }
    } catch (std::exception &e) {
      g_log.error() << e.what() << '\n';
    }
  }
}

std::unique_ptr<BackgroundStrategy> FindSXPeaks::getBackgroundStrategy() const {
  const std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  if (peakFindingStrategy == strongestPeakStrategy) {
      const double signalBackground = getProperty("SignalBackground");
      return Mantid::Kernel::make_unique<PerSpectrumBackgroundStrategy>(signalBackground);
  } else if (peakFindingStrategy == allPeaksStrategy) {
      const double background = getProperty(absoluteBackground);
      return Mantid::Kernel::make_unique<AbsoluteBackgroundStrategy>(background);
  } else {
    throw std::invalid_argument("The selected background strategy has not been implemented yet.");
  }
}


std::unique_ptr<FindSXPeaksHelper::PeakFindingStrategy> FindSXPeaks::getPeakFindingStrategy(const BackgroundStrategy* backgroundStrategy,
                                                                                            const API::SpectrumInfo &spectrumInfo,
                                                                                            const double minValue, const double maxValue) const {
  // Get the peak finding stratgy
  std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  if (peakFindingStrategy == strongestPeakStrategy) {
    return Mantid::Kernel::make_unique<StrongestPeaksStrategy>(backgroundStrategy, spectrumInfo, minValue, maxValue);
  } else if (peakFindingStrategy == allPeaksStrategy) {
    return Mantid::Kernel::make_unique<AllPeaksStrategy>(backgroundStrategy, spectrumInfo, minValue, maxValue);
  } else {
    throw std::invalid_argument("The selected peak finding strategy has not been implemented yet.");
  }
}


std::unique_ptr<FindSXPeaksHelper::ReducePeakListStrategy> FindSXPeaks::getReducePeakListStrategy() const {
  const std::string peakFindingStrategy = getProperty("PeakFindingStrategy");
  auto useSimpleReduceStrategy = peakFindingStrategy == strongestPeakStrategy;
  if (useSimpleReduceStrategy) {
    return Mantid::Kernel::make_unique<FindSXPeaksHelper::SimpleReduceStrategy>();
  } else {
    return Mantid::Kernel::make_unique<FindSXPeaksHelper::FindMaxReduceStrategy>();
  }
}



} // namespace Algorithms
} // namespace Mantid
