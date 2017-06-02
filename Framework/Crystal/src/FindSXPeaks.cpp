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

#include <unordered_map>
#include <vector>

using namespace Mantid::DataObjects;

namespace {
// Anonymous namespace
using namespace Mantid;
using WsIndexToDetIds = std::unordered_map<size_t, std::vector<detid_t>>;

WsIndexToDetIds mapDetectorsToWsIndexes(const API::DetectorInfo &detectorInfo,
                                        const detid2index_map &mapping) {
  const auto &detectorIds = detectorInfo.detectorIDs();
  WsIndexToDetIds indexToDetMapping;

  indexToDetMapping.reserve(detectorIds.size());
  for (const auto detectorID : detectorIds) {
    auto detMapEntry = mapping.find(detectorID);
    if (detMapEntry == mapping.end()) {
      throw std::runtime_error(
          "Detector ID " + std::to_string(detectorID) +
          " was not found in the workspace index mapping.");
    }

    const size_t wsIndex = detMapEntry->second;
    auto indexMapEntry = indexToDetMapping.find(wsIndex);
    if (indexMapEntry == indexToDetMapping.end()) {
      // Create a new vector if one does not exist
      indexToDetMapping[wsIndex] = std::vector<detid_t>{detectorID};
    } else {
      // Otherwise add the detector ID to the current list
      indexToDetMapping[wsIndex].push_back(detectorID);
    }
  }
  return indexToDetMapping;
}
}

namespace Mantid {
namespace Crystal {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXPeaks)

using namespace Kernel;
using namespace API;

// Type def the index to detector mapping
using WsIndexToDetIds = std::unordered_map<size_t, std::vector<detid_t>>;

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
  declareProperty("SignalBackground", 10.0,
                  "Multiplication factor for the signal background");
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
  double SB = getProperty("SignalBackground");

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
  const auto &detectorInfo = localworkspace->detectorInfo();

  const WsIndexToDetIds wsIndexToDetIdMap = mapDetectorsToWsIndexes(
      detectorInfo, localworkspace->getDetectorIDToWorkspaceIndexMap());

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
    const auto &X = localworkspace->x(wsIndex);
    const auto &Y = localworkspace->y(wsIndex);

    // Find the range [min,max]
    auto lowit = (m_MinRange == EMPTY_DBL())
                     ? X.begin()
                     : std::lower_bound(X.begin(), X.end(), m_MinRange);

    auto highit =
        (m_MaxRange == EMPTY_DBL())
            ? X.end()
            : std::find_if(lowit, X.end(),
                           std::bind2nd(std::greater<double>(), m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if (lowit == X.end() || highit == X.begin())
      continue;

    --highit; // Upper limit is the bin before, i.e. the last value smaller than
              // MaxRange

    auto distmin = std::distance(X.begin(), lowit);
    auto distmax = std::distance(X.begin(), highit);

    // Find the max element
    auto maxY = (Y.size() > 1)
                    ? std::max_element(Y.begin() + distmin, Y.begin() + distmax)
                    : Y.begin();

    double intensity = (*maxY);
    double background = 0.5 * (1.0 + Y.front() + Y.back());
    if (intensity < SB * background) // This is not a peak.
      continue;

    // t.o.f. of the peak
    auto d = std::distance(Y.begin(), maxY);
    auto leftBinPosition = X.begin() + d;
    double leftBinEdge = *leftBinPosition;
    double rightBinEdge = *std::next(leftBinPosition);
    double tof = 0.5 * (leftBinEdge + rightBinEdge);

    const double phi =
        calculatePhi(wsIndexToDetIdMap, spectrumInfo, wsIndexSize_t);

    std::vector<int> specs(1, wsIndex);
    SXPeak peak(tof, phi, *maxY, specs, wsIndex, spectrumInfo);
    PARALLEL_CRITICAL(entries) { entries.push_back(peak); }
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
  * Calculates the average phi value if the workspace contains
  * multiple detectors per spectrum, or returns the value
  * of phi if it is a single detector to spectrum mapping.
  * @param detectorMapping :: The mapping of workspace index to detector id(s)
  * @param spectrumInfo :: The spectrum info of this workspace
  * @param wsIndex :: The index to return the phi value of
  * @return :: The averaged or exact value of phi
  */
double FindSXPeaks::calculatePhi(const WsIndexToDetIds &detectorMapping,
                                 const SpectrumInfo &spectrumInfo,
                                 size_t wsIndex) {
  double phi = std::numeric_limits<double>::infinity();
  const size_t numDetectors = detectorMapping.at(wsIndex).size();
  const auto &det = spectrumInfo.detector(wsIndex);
  if (numDetectors == 1) {
    phi = det.getPhi();
  } else {
    // Have to average the value for phi
    auto detectorGroup = dynamic_cast<const Geometry::DetectorGroup *>(&det);
    if (!detectorGroup) {
      throw std::runtime_error("Could not cast to detector group");
    }
    detectorGroup->getPhi();
  }

  if (phi < 0) {
    phi += 2.0 * M_PI;
  }
  return phi;
}

/**
Reduce the peak list by removing duplicates
then convert SXPeaks objects to PeakObjects and add them to the output workspace
@param pcv : current peak list containing potential duplicates
*/
void FindSXPeaks::reducePeakList(const peakvector &pcv) {
  double resol = getProperty("Resolution");
  peakvector finalv;

  for (const auto &currentPeak : pcv) {
    auto pos = std::find_if(finalv.begin(), finalv.end(),
                            [&currentPeak, resol](SXPeak &peak) {
                              bool result = currentPeak.compare(peak, resol);
                              if (result)
                                peak += currentPeak;
                              return result;
                            });
    if (pos == finalv.end())
      finalv.push_back(currentPeak);
  }

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
} // namespace Algorithms
} // namespace Mantid
