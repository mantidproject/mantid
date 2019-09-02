// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ExtractSpectra.h"
#include "MantidAlgorithms/ExtractSpectra2.h"

#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Slice.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

namespace {
/// The percentage 'fuzziness' to use when comparing to bin boundaries
const double xBoundaryTolerance = 1.0e-15;
} // namespace

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using Types::Event::TofEvent;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSpectra)

/// Algorithms name for identification. @see Algorithm::name
const std::string ExtractSpectra::name() const { return "ExtractSpectra"; }

/// Algorithm's version for identification. @see Algorithm::version
int ExtractSpectra::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractSpectra::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractSpectra::summary() const {
  return "Extracts a list of spectra from a workspace and places them in a new "
         "workspace.";
}

/** Initialize the algorithm's properties.
 */
void ExtractSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Name of the output workspace");

  declareProperty("XMin", EMPTY_DBL(),
                  "An X value that is within the first "
                  "(lowest X value) bin that will be "
                  "retained\n"
                  "(default: workspace min)");
  declareProperty("XMax", EMPTY_DBL(),
                  "An X value that is in the highest X "
                  "value bin to be retained (default: max "
                  "X)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The index number of the first entry in the Workspace that "
                  "will be loaded\n"
                  "(default: first entry in the Workspace)");
  // As the property takes ownership of the validator pointer, have to take care
  // to pass in a unique pointer to each property.
  declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
      "The index number of the last entry in the Workspace to be loaded\n"
      "(default: last entry in the Workspace)");
  declareProperty(std::make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "A comma-separated list of individual workspace indices to "
                  "read.  Only used if\n"
                  "explicitly set. The WorkspaceIndexList is only used if the "
                  "DetectorList is empty.");

  declareProperty(std::make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "A comma-separated list of individual detector IDs to read.  "
                  "Only used if\n"
                  "explicitly set. When specifying the WorkspaceIndexList and "
                  "DetectorList property,\n"
                  "the latter is being selected.");
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void ExtractSpectra::exec() {
  m_inputWorkspace = getProperty("InputWorkspace");
  m_histogram = m_inputWorkspace->isHistogramData();
  m_commonBoundaries = m_inputWorkspace->isCommonBins();
  this->checkProperties();

  if (m_workspaceIndexList.empty()) {
    MatrixWorkspace_sptr out = getProperty("OutputWorkspace");
    // No spectra extracted, but not in-place, clone input before cropping.
    if (out != m_inputWorkspace)
      m_inputWorkspace = m_inputWorkspace->clone();
  } else {
    auto extract = boost::make_shared<ExtractSpectra2>();
    setupAsChildAlgorithm(extract);
    extract->setWorkspaceInputProperties(
        "InputWorkspace", m_inputWorkspace, IndexType::WorkspaceIndex,
        std::vector<int64_t>(m_workspaceIndexList.begin(),
                             m_workspaceIndexList.end()));
    extract->execute();
    m_inputWorkspace = extract->getProperty("OutputWorkspace");
  }
  setProperty("OutputWorkspace", m_inputWorkspace);

  if (isDefault("XMin") && isDefault("XMax"))
    return;

  eventW = boost::dynamic_pointer_cast<EventWorkspace>(m_inputWorkspace);
  if (eventW)
    this->execEvent();
  else
    this->execHistogram();
}

/// Execute the algorithm in case of a histogrammed data.
void ExtractSpectra::execHistogram() {
  auto size = static_cast<int>(m_inputWorkspace->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, size);
  for (int i = 0; i < size; ++i) {
    if (m_commonBoundaries) {
      m_inputWorkspace->setHistogram(i, slice(m_inputWorkspace->histogram(i),
                                              m_minX, m_maxX - m_histogram));
    } else {
      this->cropRagged(*m_inputWorkspace, i);
    }
    propagateBinMasking(*m_inputWorkspace, i);
    prog.report();
  }
}

namespace { // anonymous namespace

template <class T> struct eventFilter {
  eventFilter(const double minValue, const double maxValue)
      : minValue(minValue), maxValue(maxValue) {}

  bool operator()(const T &value) {
    const double tof = value.tof();
    return !(tof <= maxValue && tof >= minValue);
  }

  double minValue;
  double maxValue;
};

template <class T>
void filterEventsHelper(std::vector<T> &events, const double xmin,
                        const double xmax) {
  events.erase(
      std::remove_if(events.begin(), events.end(), eventFilter<T>(xmin, xmax)),
      events.end());
}
} // namespace

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void ExtractSpectra::execEvent() {
  double minX_val = getProperty("XMin");
  double maxX_val = getProperty("XMax");
  if (isEmpty(minX_val))
    minX_val = eventW->getTofMin();
  if (isEmpty(maxX_val))
    maxX_val = eventW->getTofMax();

  BinEdges binEdges(2);
  if (m_commonBoundaries) {
    auto &oldX = m_inputWorkspace->x(0);
    binEdges = BinEdges(oldX.begin() + m_minX, oldX.begin() + m_maxX);
  }
  if (m_maxX - m_minX < 2) {
    // create new output X axis
    binEdges = {minX_val, maxX_val};
  }

  eventW->sortAll(TOF_SORT, nullptr);

  Progress prog(this, 0.0, 1.0, eventW->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*eventW))
  for (int i = 0; i < static_cast<int>(eventW->getNumberHistograms()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    EventList &el = eventW->getSpectrum(i);

    switch (el.getEventType()) {
    case TOF: {
      filterEventsHelper(el.getEvents(), minX_val, maxX_val);
      break;
    }
    case WEIGHTED: {
      filterEventsHelper(el.getWeightedEvents(), minX_val, maxX_val);
      break;
    }
    case WEIGHTED_NOTIME: {
      filterEventsHelper(el.getWeightedEventsNoTime(), minX_val, maxX_val);
      break;
    }
    }

    // If the X axis is NOT common, then keep the initial X axis, just clear the
    // events, otherwise:
    if (m_commonBoundaries) {
      const auto oldDx = el.pointStandardDeviations();
      el.setHistogram(binEdges);
      if (oldDx) {
        el.setPointStandardDeviations(oldDx.begin() + m_minX,
                                      oldDx.begin() + (m_maxX - m_histogram));
      }
    }
    propagateBinMasking(*eventW, i);
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/// Propagate bin masking if there is any.
void ExtractSpectra::propagateBinMasking(MatrixWorkspace &workspace,
                                         const int i) const {
  if (workspace.hasMaskedBins(i)) {
    MatrixWorkspace::MaskList filteredMask;
    for (const auto &mask : workspace.maskedBins(i)) {
      const size_t maskIndex = mask.first;
      if (maskIndex >= m_minX && maskIndex < m_maxX - m_histogram)
        filteredMask[maskIndex - m_minX] = mask.second;
    }
    workspace.setMaskedBins(i, filteredMask);
  }
}

/** Retrieves the optional input properties and checks that they have valid
 * values.
 *  Assigns to the defaults if any property has not been set.
 *  @throw std::invalid_argument If the input workspace does not have common
 * binning
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void ExtractSpectra::checkProperties() {
  m_minX = this->getXMin();
  m_maxX = this->getXMax();
  const size_t xSize = m_inputWorkspace->x(0).size();
  if (m_minX > 0 || m_maxX < xSize) {
    if (m_minX > m_maxX) {
      g_log.error("XMin must be less than XMax");
      throw std::out_of_range("XMin must be less than XMax");
    }
    if ((m_minX == m_maxX ||
         (m_inputWorkspace->isHistogramData() && m_maxX == m_minX + 1)) &&
        m_commonBoundaries &&
        !boost::dynamic_pointer_cast<EventWorkspace>(m_inputWorkspace)) {
      g_log.error("The X range given lies entirely within a single bin");
      throw std::out_of_range(
          "The X range given lies entirely within a single bin");
    }
    m_croppingInX = true;
  }
  if (!m_commonBoundaries) {
    m_minX = 0;
    m_maxX = static_cast<int>(m_inputWorkspace->x(0).size());
  }

  // The hierarchy of inputs is (one is being selected):
  // 1. DetectorList
  // 2. WorkspaceIndexList
  // 3. Start and stop index
  std::vector<detid_t> detectorList = getProperty("DetectorList");
  if (!detectorList.empty()) {
    m_workspaceIndexList =
        m_inputWorkspace->getIndicesFromDetectorIDs(detectorList);
  } else {
    m_workspaceIndexList = getProperty("WorkspaceIndexList");

    if (m_workspaceIndexList.empty()) {
      int minSpec_i = getProperty("StartWorkspaceIndex");
      auto minSpec = static_cast<size_t>(minSpec_i);
      const size_t numberOfSpectra = m_inputWorkspace->indexInfo().globalSize();
      int maxSpec_i = getProperty("EndWorkspaceIndex");
      auto maxSpec = static_cast<size_t>(maxSpec_i);
      if (isEmpty(maxSpec_i))
        maxSpec = numberOfSpectra - 1;
      if (maxSpec < minSpec) {
        g_log.error("StartWorkspaceIndex must be less than or equal to "
                    "EndWorkspaceIndex");
        throw std::out_of_range(
            "StartWorkspaceIndex must be less than or equal "
            "to EndWorkspaceIndex");
      }
      if (maxSpec - minSpec + 1 != numberOfSpectra) {
        m_workspaceIndexList.reserve(maxSpec - minSpec + 1);
        for (size_t i = minSpec; i <= maxSpec; ++i)
          m_workspaceIndexList.push_back(i);
      }
    }
  }
}

/** Find the X index corresponding to (or just within) the value given in the
 * XMin property.
 *  Sets the default if the property has not been set.
 *  @param  wsIndex The workspace index to check (default 0).
 *  @return The X index corresponding to the XMin value.
 */
size_t ExtractSpectra::getXMin(const size_t wsIndex) {
  double minX_val = getProperty("XMin");
  size_t xIndex = 0;
  if (!isEmpty(minX_val)) { // A value has been passed to the algorithm, check
                            // it and maybe store it
    auto &X = m_inputWorkspace->x(wsIndex);
    if (m_commonBoundaries && minX_val > X.back()) {
      std::stringstream msg;
      msg << "XMin is greater than the largest X value (" << minX_val << " > "
          << X.back() << ")";
      g_log.error(msg.str());
      throw std::out_of_range(msg.str());
    }
    // Reduce cut-off value slightly to allow for rounding errors
    // when trying to exactly hit a bin boundary.
    minX_val -= std::abs(minX_val * xBoundaryTolerance);
    xIndex = std::lower_bound(X.begin(), X.end(), minX_val) - X.begin();
  }
  return xIndex;
}

/** Find the X index corresponding to (or just within) the value given in the
 * XMax property.
 *  Sets the default if the property has not been set.
 *  @param  wsIndex The workspace index to check (default 0).
 *  @return The X index corresponding to the XMax value.
 */
size_t ExtractSpectra::getXMax(const size_t wsIndex) {
  const auto &X = m_inputWorkspace->x(wsIndex);
  size_t xIndex = X.size();
  // get the value that the user entered if they entered one at all
  double maxX_val = getProperty("XMax");
  if (!isEmpty(maxX_val)) { // we have a user value, check it and maybe store it
    if (m_commonBoundaries && maxX_val < X.front()) {
      std::stringstream msg;
      msg << "XMax is less than the smallest X value (" << maxX_val << " < "
          << X.front() << ")";
      g_log.error(msg.str());
      throw std::out_of_range(msg.str());
    }
    // Increase cut-off value slightly to allow for rounding errors
    // when trying to exactly hit a bin boundary.
    maxX_val += std::abs(maxX_val * xBoundaryTolerance);
    xIndex = std::upper_bound(X.begin(), X.end(), maxX_val) - X.begin();
  }
  return xIndex;
}

/** Zeroes all data points outside the X values given
 *  @param workspace :: The output workspace to crop
 *  @param index ::         The workspace index of the spectrum
 */
void ExtractSpectra::cropRagged(MatrixWorkspace &workspace, int index) {
  auto &Y = workspace.mutableY(index);
  auto &E = workspace.mutableE(index);
  const size_t size = Y.size();
  size_t startX = this->getXMin(index);
  if (startX > size)
    startX = size;
  for (size_t i = 0; i < startX; ++i) {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
  size_t endX = this->getXMax(index);
  if (endX > 0)
    endX -= m_histogram;
  for (size_t i = endX; i < size; ++i) {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
}

} // namespace Algorithms
} // namespace Mantid
