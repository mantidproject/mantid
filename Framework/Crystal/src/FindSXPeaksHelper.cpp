// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindSXPeaksHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/make_unique.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <cmath>

namespace {

const double TWO_PI = 2 * M_PI;

bool isDifferenceLargerThanTolerance(const double angle1, const double angle2,
                                     const double tolerance) {
  auto difference = std::abs(angle1 - angle2);

  // If we have more than 360 degree angle difference then we need to wrap it
  // back to 360
  if (difference > TWO_PI) {
    difference = std::fmod(difference, TWO_PI);
  }

  // If we have more than 180 degrees then we must take the smaller angle
  if (difference > M_PI) {
    difference = TWO_PI - difference;
  }

  return difference > tolerance;
}
} // namespace

using namespace boost;
using Mantid::Kernel::UnitFactory;

namespace Mantid {
namespace Crystal {
namespace FindSXPeaksHelper {

Mantid::Kernel::Logger g_log("FindSXPeaksHelper");

/* ------------------------------------------------------------------------------------------
 * Single Crystal peak representation
 * ------------------------------------------------------------------------------------------
 */

/**
Constructor
@param t : tof
@param phi : psi angle
@param intensity : peak intensity
@param spectral : contributing spectra
@param wsIndex : ws index of the contributing spectrum
@param spectrumInfo: spectrum info of the original ws.
*/
SXPeak::SXPeak(double t, double phi, double intensity,
               const std::vector<int> &spectral, const size_t wsIndex,
               const API::SpectrumInfo &spectrumInfo)
    : m_tof(t), m_phi(phi), m_intensity(intensity), m_spectra(spectral),
      m_wsIndex(wsIndex) {
  // Sanity checks
  if (intensity < 0) {
    throw std::invalid_argument("SXPeak: Cannot have an intensity < 0");
  }
  if (spectral.empty()) {
    throw std::invalid_argument("SXPeak: Cannot have zero sized spectral list");
  }
  if (!spectrumInfo.hasDetectors(m_wsIndex)) {
    throw std::invalid_argument("SXPeak: Spectrum at ws index " +
                                std::to_string(wsIndex) +
                                " doesn't have detectors");
  }

  const auto l1 = spectrumInfo.l1();
  const auto l2 = spectrumInfo.l2(m_wsIndex);

  m_twoTheta = spectrumInfo.twoTheta(m_wsIndex);
  m_LTotal = l1 + l2;
  if (m_LTotal < 0) {
    throw std::invalid_argument("SXPeak: Cannot have detector distance < 0");
  }
  m_detId = spectrumInfo.detector(m_wsIndex).getID();
  m_nPixels = 1;

  const auto unit = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
  unit->initialize(l1, l2, m_twoTheta, 0, 0, 0);
  m_dSpacing = unit->singleFromTOF(m_tof);

  const auto samplePos = spectrumInfo.samplePosition();
  const auto sourcePos = spectrumInfo.sourcePosition();
  const auto detPos = spectrumInfo.position(m_wsIndex);
  // Normalized beam direction
  auto beamDir = samplePos - sourcePos;
  beamDir.normalize();
  // Normalized detector direction
  auto detDir = (detPos - samplePos);
  detDir.normalize();
  m_unitWaveVector = beamDir - detDir;
  m_qConvention = Kernel::ConfigService::Instance().getString("Q.convention");
}

/**
Object comparision
@param rhs : other SXPeak
@param tolerance : tolerance
*/
bool SXPeak::compare(const SXPeak &rhs, double tolerance) const {
  if (std::abs(m_tof / m_nPixels - rhs.m_tof / rhs.m_nPixels) >
      tolerance * m_tof / m_nPixels)
    return false;
  if (std::abs(m_phi / m_nPixels - rhs.m_phi / rhs.m_nPixels) >
      tolerance * m_phi / m_nPixels)
    return false;
  if (std::abs(m_twoTheta / m_nPixels - rhs.m_twoTheta / rhs.m_nPixels) >
      tolerance * m_twoTheta / m_nPixels)
    return false;
  return true;
}

bool SXPeak::compare(const SXPeak &rhs, const double xTolerance,
                     const double phiTolerance, const double thetaTolerance,
                     const XAxisUnit units) const {

  const auto x_1 = (units == XAxisUnit::TOF) ? m_tof : m_dSpacing;
  const auto x_2 = (units == XAxisUnit::TOF) ? rhs.m_tof : rhs.m_dSpacing;

  if (std::abs(x_1 - x_2) > xTolerance) {
    return false;
  }

  if (isDifferenceLargerThanTolerance(m_phi, rhs.m_phi, phiTolerance)) {
    return false;
  }
  if (isDifferenceLargerThanTolerance(m_twoTheta, rhs.m_twoTheta,
                                      thetaTolerance)) {
    return false;
  }
  return true;
}

/**
Getter for LabQ
@return q vector
*/
Mantid::Kernel::V3D SXPeak::getQ() const {
  double qSign = 1.0;
  if (m_qConvention == "Crystallography") {
    qSign = -1.0;
  }
  double vi = m_LTotal / (m_tof * 1e-6);
  // wavenumber = h_bar / mv
  double wi = PhysicalConstants::h_bar / (PhysicalConstants::NeutronMass * vi);
  // in angstroms
  wi *= 1e10;
  // wavevector=1/wavenumber = 2pi/wavelength
  double wvi = 1.0 / wi;
  // Now calculate the wavevector of the scattered neutron
  return m_unitWaveVector * wvi * qSign;
}

/**
Operator addition overload
@param rhs : Right hand slide peak for addition.
*/
SXPeak &SXPeak::operator+=(const SXPeak &rhs) {
  m_tof += rhs.m_tof;
  m_phi += rhs.m_phi;
  m_twoTheta += rhs.m_twoTheta;
  m_intensity += rhs.m_intensity;
  m_LTotal += rhs.m_LTotal;
  m_nPixels += 1;
  m_spectra.insert(m_spectra.end(), rhs.m_spectra.cbegin(),
                   rhs.m_spectra.cend());
  return *this;
}

/// Normalise by number of pixels
void SXPeak::reduce() {
  m_tof /= m_nPixels;
  m_phi /= m_nPixels;
  m_twoTheta /= m_nPixels;
  m_intensity /= m_nPixels;
  m_LTotal /= m_nPixels;
  m_nPixels = 1;
}

/**
Getter for the intensity.
*/
const double &SXPeak::getIntensity() const { return m_intensity; }

/**
Getter for the detector Id.
*/
detid_t SXPeak::getDetectorId() const { return m_detId; }

PeakContainer::PeakContainer(const HistogramData::HistogramY &y)
    : m_y(y), m_startIndex(0), m_stopIndex(m_y.size() - 1), m_maxIndex(0) {}

void PeakContainer::startRecord(yIt item) {
  m_startIndex = std::distance(m_y.begin(), item);
  m_maxIndex = m_startIndex;
  m_maxSignal = *item;
}

void PeakContainer::record(yIt item) {
  if (*item > m_maxSignal) {
    m_maxIndex = std::distance(m_y.begin(), item);
    m_maxSignal = *item;
  }
}

void PeakContainer::stopRecord(yIt item) {
  if (*item > m_maxSignal) {
    m_maxIndex = std::distance(m_y.begin(), item);
    m_maxSignal = *item;
  }
  m_stopIndex = std::distance(m_y.begin(), item);

  // Peak end is one back though
  --m_stopIndex;
}

size_t PeakContainer::getNumberOfPointsInPeak() const {
  // If we didn't record anything then the start iterator is at the end
  if (m_startIndex >= m_y.size()) {
    return 0;
  }

  return m_stopIndex - m_startIndex;
}

yIt PeakContainer::getMaxIterator() const { return m_y.begin() + m_maxIndex; }

/* ------------------------------------------------------------------------------------------
 * Background
 * ------------------------------------------------------------------------------------------
 */
AbsoluteBackgroundStrategy::AbsoluteBackgroundStrategy(const double background)
    : m_background(background) {}

bool AbsoluteBackgroundStrategy::isBelowBackground(
    const double intensity, const HistogramData::HistogramY & /*y*/) const {
  return intensity < m_background;
}

PerSpectrumBackgroundStrategy::PerSpectrumBackgroundStrategy(
    const double backgroundMultiplier)
    : m_backgroundMultiplier(backgroundMultiplier) {}

bool PerSpectrumBackgroundStrategy::isBelowBackground(
    const double intensity, const HistogramData::HistogramY &y) const {
  auto background = 0.5 * (1.0 + y.front() + y.back());
  background *= m_backgroundMultiplier;
  return intensity < background;
}

/* ------------------------------------------------------------------------------------------
 * Peak Finding Strategy
 * ------------------------------------------------------------------------------------------
 */

PeakFindingStrategy::PeakFindingStrategy(
    const BackgroundStrategy *backgroundStrategy,
    const API::SpectrumInfo &spectrumInfo, const double minValue,
    const double maxValue, const XAxisUnit units)
    : m_backgroundStrategy(backgroundStrategy), m_minValue(minValue),
      m_maxValue(maxValue), m_spectrumInfo(spectrumInfo), m_units(units) {}

PeakList PeakFindingStrategy::findSXPeaks(const HistogramData::HistogramX &x,
                                          const HistogramData::HistogramY &y,
                                          const int workspaceIndex) const {
  // ---------------------------------------
  // Get the lower and upper bound iterators
  // ---------------------------------------
  auto boundsIterator = getBounds(x);
  auto lowit = boundsIterator.first;
  auto highit = boundsIterator.second;

  // If range specified doesn't overlap with this spectrum then bail out
  if (lowit == x.end() || highit == x.begin()) {
    return PeakList();
  }

  // Upper limit is the bin before, i.e. the last value smaller than MaxRange
  --highit;

  // ---------------------------------------
  // Perform the search of the peaks
  // ---------------------------------------
  return dofindSXPeaks(x, y, lowit, highit, workspaceIndex);
}

BoundsIterator
PeakFindingStrategy::getBounds(const HistogramData::HistogramX &x) const {
  // Find the range [min,max]
  auto lowit = (m_minValue == EMPTY_DBL())
                   ? x.begin()
                   : std::lower_bound(x.begin(), x.end(), m_minValue);

  auto highit =
      (m_maxValue == EMPTY_DBL())
          ? x.end()
          : std::find_if(lowit, x.end(),
                         std::bind2nd(std::greater<double>(), m_maxValue));

  return std::make_pair(lowit, highit);
}

/**
 * Calculates the average phi value if the workspace contains
 * multiple detectors per spectrum, or returns the value
 * of phi if it is a single detector to spectrum mapping.
 * @param workspaceIndex :: The index to return the phi value of
 * @return :: The averaged or exact value of phi
 */
double PeakFindingStrategy::calculatePhi(size_t workspaceIndex) const {
  double phi;

  // Get the detectors for the workspace index
  const auto &spectrumDefinition =
      m_spectrumInfo.spectrumDefinition(workspaceIndex);
  const auto numberOfDetectors = spectrumDefinition.size();
  const auto &det = m_spectrumInfo.detector(workspaceIndex);
  if (numberOfDetectors == 1) {
    phi = det.getPhi();
  } else {
    // Have to average the value for phi
    auto detectorGroup =
        dynamic_cast<const Mantid::Geometry::DetectorGroup *>(&det);
    if (!detectorGroup) {
      throw std::runtime_error("Could not cast to detector group");
    }
    phi = detectorGroup->getPhi();
  }

  if (phi < 0) {
    phi += 2.0 * M_PI;
  }
  return phi;
}

double PeakFindingStrategy::getXValue(const HistogramData::HistogramX &x,
                                      const size_t peakLocation) const {
  auto leftBinPosition = x.begin() + peakLocation;
  const double leftBinEdge = *leftBinPosition;
  const double rightBinEdge = *std::next(leftBinPosition);
  return 0.5 * (leftBinEdge + rightBinEdge);
}

double PeakFindingStrategy::convertToTOF(const double xValue,
                                         const size_t workspaceIndex) const {
  if (m_units == XAxisUnit::TOF) {
    // we're already using TOF units
    return xValue;
  } else {
    const auto unit = UnitFactory::Instance().create("dSpacing");
    // we're using d-spacing, convert the point to TOF
    unit->initialize(m_spectrumInfo.l1(), m_spectrumInfo.l2(workspaceIndex),
                     m_spectrumInfo.twoTheta(workspaceIndex), 0, 0, 0);
    return unit->singleToTOF(xValue);
  }
}

StrongestPeaksStrategy::StrongestPeaksStrategy(
    const BackgroundStrategy *backgroundStrategy,
    const API::SpectrumInfo &spectrumInfo, const double minValue,
    const double maxValue, const XAxisUnit units)
    : PeakFindingStrategy(backgroundStrategy, spectrumInfo, minValue, maxValue,
                          units) {}

PeakList StrongestPeaksStrategy::dofindSXPeaks(
    const HistogramData::HistogramX &x, const HistogramData::HistogramY &y,
    Bound low, Bound high, const int workspaceIndex) const {
  auto distmin = std::distance(x.begin(), low);
  auto distmax = std::distance(x.begin(), high);

  // Find the max element
  auto maxY = (y.size() > 1)
                  ? std::max_element(y.begin() + distmin, y.begin() + distmax)
                  : y.begin();

  // Perform a check against the background
  double intensity = (*maxY);
  if (m_backgroundStrategy->isBelowBackground(intensity, y)) {
    return PeakList();
  }

  // Create the SXPeak information
  const auto distance = std::distance(y.begin(), maxY);
  const auto xValue = getXValue(x, distance);
  const auto tof = convertToTOF(xValue, workspaceIndex);
  const double phi = calculatePhi(workspaceIndex);

  std::vector<int> specs(1, workspaceIndex);
  std::vector<SXPeak> peaks;
  peaks.emplace_back(tof, phi, *maxY, specs, workspaceIndex, m_spectrumInfo);
  return peaks;
}

AllPeaksStrategy::AllPeaksStrategy(const BackgroundStrategy *backgroundStrategy,
                                   const API::SpectrumInfo &spectrumInfo,
                                   const double minValue, const double maxValue,
                                   const XAxisUnit units)
    : PeakFindingStrategy(backgroundStrategy, spectrumInfo, minValue, maxValue,
                          units) {
  // We only allow the AbsoluteBackgroundStrategy for now
  if (!dynamic_cast<const AbsoluteBackgroundStrategy *>(m_backgroundStrategy)) {
    throw std::invalid_argument("The AllPeaksStrategy has to be initialized "
                                "with the AbsoluteBackgroundStrategy.");
  }
}

PeakList AllPeaksStrategy::dofindSXPeaks(const HistogramData::HistogramX &x,
                                         const HistogramData::HistogramY &y,
                                         Bound low, Bound high,
                                         const int workspaceIndex) const {
  // Get all peaks from the container
  auto foundPeaks = getAllPeaks(x, y, low, high, m_backgroundStrategy);

  // Convert the found peaks to SXPeaks
  auto peaks = convertToSXPeaks(x, y, foundPeaks, workspaceIndex);

  return peaks;
}

std::vector<std::unique_ptr<PeakContainer>> AllPeaksStrategy::getAllPeaks(
    const HistogramData::HistogramX &x, const HistogramData::HistogramY &y,
    Bound low, Bound high,
    const Mantid::Crystal::FindSXPeaksHelper::BackgroundStrategy
        *backgroundStrategy) const {
  // We iterate over the data and only consider data which is above the
  // threshold.
  // Once data starts to be above the threshold we start to record it and add it
  // to a peak. Once it falls below, it concludes recording of that particular
  // peak
  bool isRecording = false;

  std::unique_ptr<PeakContainer> currentPeak = nullptr;
  std::vector<std::unique_ptr<PeakContainer>> peaks;

  // We want to the upper boundary to be inclusive hence we need to increment it
  // by one
  if (high != x.end()) {
    ++high;
  }
  auto distanceMin = std::distance(x.begin(), low);
  auto distanceMax = std::distance(x.begin(), high);

  const auto lowY = y.begin() + distanceMin;
  auto highY = distanceMax < static_cast<int>(y.size())
                   ? y.begin() + distanceMax
                   : y.end();

  for (auto it = lowY; it != highY; ++it) {
    const auto signal = *it;
    const auto isAboveThreshold =
        !backgroundStrategy->isBelowBackground(signal, y);

    // There are four scenarios:
    // 1. Not recording + below threshold => continue
    // 2. Not recording + above treshold => start recording
    // 3. Recording + below threshold => stop recording
    // 4. Recording + above threshold => continue recording
    if (!isRecording && !isAboveThreshold) {
      continue;
    } else if (!isRecording && isAboveThreshold) {
      currentPeak = Mantid::Kernel::make_unique<PeakContainer>(y);
      currentPeak->startRecord(it);
      isRecording = true;
    } else if (isRecording && !isAboveThreshold) {
      currentPeak->stopRecord(it);
      peaks.push_back(std::move(currentPeak));
      currentPeak = nullptr;
      isRecording = false;
    } else {
      currentPeak->record(it);
    }
  }

  // Handle a peak on the edge if it exists
  if (isRecording) {
    if (highY == y.end()) {
      --highY;
    }
    currentPeak->stopRecord(highY);
    peaks.push_back(std::move(currentPeak));
    currentPeak = nullptr;
  }

  return peaks;
}

PeakList AllPeaksStrategy::convertToSXPeaks(
    const HistogramData::HistogramX &x, const HistogramData::HistogramY &y,
    const std::vector<std::unique_ptr<PeakContainer>> &foundPeaks,
    const int workspaceIndex) const {
  PeakList peaks;

  if (foundPeaks.empty()) {
    return peaks;
  }

  // Add a vector to the boost optional
  peaks = std::vector<FindSXPeaksHelper::SXPeak>();

  for (const auto &peak : foundPeaks) {
    // Get the index of the bin
    auto maxY = peak->getMaxIterator();
    const auto distance = std::distance(y.begin(), maxY);
    const auto xValue = getXValue(x, distance);
    const auto tof = convertToTOF(xValue, workspaceIndex);
    const double phi = calculatePhi(workspaceIndex);

    std::vector<int> specs(1, workspaceIndex);
    (*peaks).emplace_back(tof, phi, *maxY, specs, workspaceIndex,
                          m_spectrumInfo);
  }

  return peaks;
}

/* ------------------------------------------------------------------------------------------
 * PeakList Reduction Strategy
 * ------------------------------------------------------------------------------------------
 */

ReducePeakListStrategy::ReducePeakListStrategy(
    const CompareStrategy *compareStrategy)
    : m_compareStrategy(compareStrategy) {}

SimpleReduceStrategy::SimpleReduceStrategy(
    const CompareStrategy *compareStrategy)
    : ReducePeakListStrategy(compareStrategy) {}

std::vector<SXPeak> SimpleReduceStrategy::reduce(
    const std::vector<SXPeak> &peaks,
    Mantid::Kernel::ProgressBase & /*progress*/) const {
  // If the peaks are empty then do nothing
  if (peaks.empty()) {
    return peaks;
  }

  std::vector<SXPeak> finalPeaks;
  for (const auto &currentPeak : peaks) {
    auto pos = std::find_if(finalPeaks.begin(), finalPeaks.end(),
                            [&currentPeak, this](SXPeak &peak) {
                              auto result = this->m_compareStrategy->compare(
                                  currentPeak, peak);
                              // bool result = currentPeak.compare(peak,
                              // resolution);
                              if (result)
                                peak += currentPeak;
                              return result;
                            });
    if (pos == finalPeaks.end()) {
      finalPeaks.push_back(currentPeak);
    }
  }

  return finalPeaks;
}

FindMaxReduceStrategy::FindMaxReduceStrategy(
    const CompareStrategy *compareStrategy)
    : ReducePeakListStrategy(compareStrategy) {}

std::vector<SXPeak>
FindMaxReduceStrategy::reduce(const std::vector<SXPeak> &peaks,
                              Mantid::Kernel::ProgressBase &progress) const {
  // If the peaks are empty then do nothing
  if (peaks.empty()) {
    return peaks;
  }

  // Groups the peaks into elements which are considered alike
  auto peakGroups = getPeakGroups(peaks, progress);

  // Now reduce the peaks groups
  return getFinalPeaks(peakGroups);
}

// Define some graph elements
using PeakGraph = adjacency_list<vecS, vecS, undirectedS, SXPeak *>;
using Vertex = boost::graph_traits<PeakGraph>::vertex_descriptor;
using Edge = boost::graph_traits<PeakGraph>::edge_descriptor;

std::vector<std::vector<SXPeak *>> FindMaxReduceStrategy::getPeakGroups(
    const std::vector<SXPeak> &peakList,
    Mantid::Kernel::ProgressBase &progress) const {

  // Create a vector of addresses. Note that the peaks live on the stack. This
  // here only works, because the peaks are always in a stack frame below.
  std::vector<SXPeak *> peaks;
  peaks.reserve(peakList.size());
  std::transform(peakList.cbegin(), peakList.cend(), std::back_inserter(peaks),
                 [](const auto &peak) { return &const_cast<SXPeak &>(peak); });

  // Add the peaks to a graph
  Edge edge;
  PeakGraph graph;
  PeakGraph::vertex_iterator vertexIt, vertexEnd;

  // Provide a warning if there are more than 500 peaks found.
  const size_t numberOfPeaksFound = peaks.size();
  if (numberOfPeaksFound > 500) {
    std::string warningMessage =
        std::string("There are ") + std::to_string(numberOfPeaksFound) +
        std::string(
            " peaks being processed. This might take a long time. "
            "Please check that the cutoff of the background that "
            "you have selected is high enough, else the algorithm will"
            " mistake background noise for peaks. The instrument view "
            "allows you to easily inspect the typical background level.");
    g_log.warning(warningMessage);
  }

  std::string message = std::string("There are ") +
                        std::to_string(numberOfPeaksFound) +
                        std::string(" peaks. Investigating peak number ");
  int peakCounter = 0;

  for (auto peak : peaks) {
    ++peakCounter;

    // 1. Add the vertex
    auto vertex = add_vertex(peak, graph);

    // 2. Iterate over all elements already in the graph and check if they need
    // to edstablish an edge between them.
    std::tie(vertexIt, vertexEnd) = vertices(graph);

    // Provide a progress report such that users can escape the graph generation
    if (peakCounter > 50) {
      progress.doReport(message + std::to_string(peakCounter));
    }

    for (; vertexIt != vertexEnd; ++vertexIt) {
      // 2.1 Check if we are looking at the new vertex itself. We don't want
      // self-loops
      if (vertex == *vertexIt) {
        continue;
      }

      // 2.2 Check if the edge exists already
      if (boost::edge(vertex, *vertexIt, graph).second) {
        continue;
      }

      // 2.3 Check if the two vertices should have an edge
      const auto toCheck = graph[*vertexIt];
      if (m_compareStrategy->compare(*peak, *toCheck)) {
        // We need to create an edge
        add_edge(vertex, *vertexIt, graph);
      }
    }
  }

  // Create disjoined graphs from graph above
  std::vector<int> components(boost::num_vertices(graph));
  const int numberOfPeaks = connected_components(graph, &components[0]);

  std::vector<std::vector<SXPeak *>> peakGroups(numberOfPeaks);

  for (auto i = 0u; i < components.size(); ++i) {
    auto index = components[i];
    peakGroups[index].push_back(graph[i]);
  }

  return peakGroups;
}

std::vector<SXPeak> FindMaxReduceStrategy::getFinalPeaks(
    const std::vector<std::vector<SXPeak *>> &peakGroups) const {
  std::vector<SXPeak> peaks;
  // For each peak groupf find one peak
  // Currently we select the peak with the largest signal (this strategy could
  // be changed to something like a weighted mean or similar)
  for (auto &group : peakGroups) {
    SXPeak *maxPeak = nullptr;
    double maxIntensity = std::numeric_limits<double>::min();
    for (auto *element : group) {
      if (element->getIntensity() > maxIntensity) {
        maxIntensity = element->getIntensity();
        maxPeak = element;
      }
    }

    // Add the max peak
    peaks.push_back(*maxPeak);
  }
  return peaks;
}

/* ------------------------------------------------------------------------------------------
 * Comparison Strategy
 * ------------------------------------------------------------------------------------------
 */
RelativeCompareStrategy::RelativeCompareStrategy(const double resolution)
    : m_resolution(resolution) {}

bool RelativeCompareStrategy::compare(const SXPeak &lhs,
                                      const SXPeak &rhs) const {
  return lhs.compare(rhs, m_resolution);
}

AbsoluteCompareStrategy::AbsoluteCompareStrategy(
    const double xUnitResolution, const double phiResolution,
    const double twoThetaResolution, const XAxisUnit units)
    : m_xUnitResolution(xUnitResolution), m_phiResolution(phiResolution),
      m_twoThetaResolution(twoThetaResolution), m_units(units) {
  // Convert the input from degree to radians
  constexpr double rad2deg = M_PI / 180.;
  m_phiResolution *= rad2deg;
  m_twoThetaResolution *= rad2deg;
}

bool AbsoluteCompareStrategy::compare(const SXPeak &lhs,
                                      const SXPeak &rhs) const {
  return lhs.compare(rhs, m_xUnitResolution, m_phiResolution,
                     m_twoThetaResolution, m_units);
}

} // namespace FindSXPeaksHelper
} // namespace Crystal
} // namespace Mantid
