#include "MantidAlgorithms/ExtractSpectra.h"

#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"

#include <algorithm>

namespace {
/// The percentage 'fuzziness' to use when comparing to bin boundaries
const double xBoundaryTolerance = 1.0e-15;
}

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSpectra)

//----------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ExtractSpectra::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name of the output workspace");

  declareProperty("XMin", EMPTY_DBL(), "An X value that is within the first "
                                       "(lowest X value) bin that will be "
                                       "retained\n"
                                       "(default: workspace min)");
  declareProperty("XMax", EMPTY_DBL(), "An X value that is in the highest X "
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
  declareProperty(make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "A comma-separated list of individual workspace indices to "
                  "read.  Only used if\n"
                  "explicitly set. The WorkspaceIndexList is only used if the "
                  "DetectorList is empty.");

  declareProperty(make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "A comma-separated list of individual detector IDs to read.  "
                  "Only used if\n"
                  "explicitly set. When specifying the WorkspaceIndexList and "
                  "DetectorList property,\n"
                  "the latter is being selected.");
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void ExtractSpectra::exec() {
  // Get the input workspace
  m_inputWorkspace = getProperty("InputWorkspace");
  m_histogram = m_inputWorkspace->isHistogramData();
  // Check for common boundaries in input workspace
  m_commonBoundaries = WorkspaceHelpers::commonBoundaries(*m_inputWorkspace);

  eventW = boost::dynamic_pointer_cast<EventWorkspace>(m_inputWorkspace);
  if (eventW != nullptr) {
    // Input workspace is an event workspace. Use the other exec method
    this->execEvent();
  } else {
    // Otherwise it's a Workspace2D
    this->execHistogram();
  }
}

/// Execute the algorithm in case of a histogrammed data.
void ExtractSpectra::execHistogram() {
  // Retrieve and validate the input properties
  this->checkProperties();

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      m_inputWorkspace, m_workspaceIndexList.size(), m_maxX - m_minX,
      m_maxX - m_minX - m_histogram);
  outputWorkspace->setIndexInfo(
      Indexing::extract(m_inputWorkspace->indexInfo(), m_workspaceIndexList));

  // If this is a Workspace2D, get the spectra axes for copying in the spectraNo
  // later
  Axis *inAxis1(nullptr);
  TextAxis *outTxtAxis(nullptr);
  NumericAxis *outNumAxis(nullptr);
  if (m_inputWorkspace->axes() > 1) {
    inAxis1 = m_inputWorkspace->getAxis(1);
    auto outAxis1 = outputWorkspace->getAxis(1);
    outTxtAxis = dynamic_cast<TextAxis *>(outAxis1);
    if (!outTxtAxis)
      outNumAxis = dynamic_cast<NumericAxis *>(outAxis1);
  }

  cow_ptr<HistogramData::HistogramX> newX(nullptr);
  if (m_commonBoundaries) {
    auto &oldX = m_inputWorkspace->x(m_workspaceIndexList.front());
    newX = make_cow<HistogramData::HistogramX>(oldX.begin() + m_minX,
                                               oldX.begin() + m_maxX);
  }

  bool doCrop = ((m_minX != 0) || (m_maxX != m_inputWorkspace->x(0).size()));

  Progress prog(this, 0.0, 1.0, (m_workspaceIndexList.size()));
  // Loop over the required workspace indices, copying in the desired bins
  for (int j = 0; j < static_cast<int>(m_workspaceIndexList.size()); ++j) {
    auto i = m_workspaceIndexList[j];

    bool hasDx = m_inputWorkspace->hasDx(i);

    // Preserve/restore sharing if X vectors are the same
    if (m_commonBoundaries) {
      outputWorkspace->setSharedX(j, newX);
      if (hasDx) {
        auto &oldDx = m_inputWorkspace->dx(i);
        outputWorkspace->setSharedDx(
            j, make_cow<HistogramData::HistogramDx>(
                   oldDx.begin() + m_minX,
                   oldDx.begin() + (m_maxX - m_histogram)));
      }
    } else {
      // Safe to just copy whole vector 'cos can't be cropping in X if not
      // common
      outputWorkspace->setSharedX(j, m_inputWorkspace->sharedX(i));
      outputWorkspace->setSharedDx(j, m_inputWorkspace->sharedDx(i));
    }

    if (doCrop) {
      auto &oldY = m_inputWorkspace->y(i);
      outputWorkspace->mutableY(j)
          .assign(oldY.begin() + m_minX, oldY.begin() + (m_maxX - m_histogram));
      auto &oldE = m_inputWorkspace->e(i);
      outputWorkspace->mutableE(j)
          .assign(oldE.begin() + m_minX, oldE.begin() + (m_maxX - m_histogram));
    } else {
      outputWorkspace->setSharedY(j, m_inputWorkspace->sharedY(i));
      outputWorkspace->setSharedE(j, m_inputWorkspace->sharedE(i));
    }

    // copy over the axis entry for each spectrum, regardless of the type of
    // axes present
    if (inAxis1) {
      if (outTxtAxis) {
        outTxtAxis->setLabel(j, inAxis1->label(i));
      } else if (outNumAxis) {
        outNumAxis->setValue(j, inAxis1->operator()(i));
      }
      // spectra axis is implicit in workspace creation
    }

    if (!m_commonBoundaries)
      this->cropRagged(outputWorkspace, static_cast<int>(i), j);

    // Propagate bin masking if there is any
    if (m_inputWorkspace->hasMaskedBins(i)) {
      const MatrixWorkspace::MaskList &inputMasks =
          m_inputWorkspace->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it) {
        const size_t maskIndex = (*it).first;
        if (maskIndex >= m_minX && maskIndex < m_maxX - m_histogram)
          outputWorkspace->flagMasked(j, maskIndex - m_minX, (*it).second);
      }
    }
    prog.report();
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

namespace { // anonymous namespace

template <class T> struct eventFilter {
  eventFilter(const double minValue, const double maxValue)
      : minValue(minValue), maxValue(maxValue) {}

  bool operator()(const T &value) {
    const double tof = value.tof();
    return (tof <= maxValue && tof >= minValue);
  }

  double minValue;
  double maxValue;
};

template <class T>
void copyEventsHelper(const std::vector<T> &inputEvents,
                      std::vector<T> &outputEvents, const double xmin,
                      const double xmax) {
  copy_if(inputEvents.begin(), inputEvents.end(),
          std::back_inserter(outputEvents), eventFilter<T>(xmin, xmax));
}
}

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

  // Retrieve and validate the input properties
  this->checkProperties();
  HistogramData::BinEdges XValues_new(2);
  if (m_commonBoundaries) {
    auto &oldX = m_inputWorkspace->x(m_workspaceIndexList.front());
    XValues_new =
        HistogramData::BinEdges(oldX.begin() + m_minX, oldX.begin() + m_maxX);
  }

  if (m_maxX - m_minX < 2) {
    // create new output X axis
    std::vector<double> rb_params{minX_val, maxX_val - minX_val, maxX_val};
    static_cast<void>(VectorHelper::createAxisFromRebinParams(
        rb_params, XValues_new.mutableRawData()));
  }

  // run inplace branch if appropriate
  MatrixWorkspace_sptr OutputWorkspace = this->getProperty("OutputWorkspace");
  bool inPlace = (OutputWorkspace == m_inputWorkspace);
  if (inPlace)
    g_log.debug("Cropping EventWorkspace in-place.");

  // Create the output workspace
  eventW->sortAll(TOF_SORT, nullptr);
  auto outputWorkspace = create<EventWorkspace>(
      *m_inputWorkspace,
      Indexing::extract(m_inputWorkspace->indexInfo(), m_workspaceIndexList),
      XValues_new);
  outputWorkspace->sortAll(TOF_SORT, nullptr);

  Progress prog(this, 0.0, 1.0, 2 * m_workspaceIndexList.size());
  eventW->sortAll(Mantid::DataObjects::TOF_SORT, &prog);
  // Loop over the required workspace indices, copying in the desired bins
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWorkspace, *outputWorkspace))
  for (int j = 0; j < static_cast<int>(m_workspaceIndexList.size()); ++j) {
    PARALLEL_START_INTERUPT_REGION
    auto i = m_workspaceIndexList[j];
    const EventList &el = eventW->getSpectrum(i);
    // The output event list
    EventList &outEL = outputWorkspace->getSpectrum(j);

    switch (el.getEventType()) {
    case TOF: {
      std::vector<TofEvent> moreevents;
      moreevents.reserve(el.getNumberEvents()); // assume all will make it
      copyEventsHelper(el.getEvents(), moreevents, minX_val, maxX_val);
      outEL += moreevents;
      break;
    }
    case WEIGHTED: {
      std::vector<WeightedEvent> moreevents;
      moreevents.reserve(el.getNumberEvents()); // assume all will make it
      copyEventsHelper(el.getWeightedEvents(), moreevents, minX_val, maxX_val);
      outEL += moreevents;
      break;
    }
    case WEIGHTED_NOTIME: {
      std::vector<WeightedEventNoTime> moreevents;
      moreevents.reserve(el.getNumberEvents()); // assume all will make it
      copyEventsHelper(el.getWeightedEventsNoTime(), moreevents, minX_val,
                       maxX_val);
      outEL += moreevents;
      break;
    }
    }
    outEL.setSortOrder(el.getSortType());

    bool hasDx = eventW->hasDx(i);

    if (!m_commonBoundaries) {
      // If the X axis is NOT common, then keep the initial X axis, just clear
      // the events
      outEL.setX(el.ptrX());
      outEL.setSharedDx(el.sharedDx());
    } else {
      // X is already set in workspace creation, just set Dx if necessary.
      if (hasDx) {
        auto &oldDx = m_inputWorkspace->dx(i);
        outEL.setPointStandardDeviations(
            oldDx.begin() + m_minX, oldDx.begin() + (m_maxX - m_histogram));
      }
    }

    // Propagate bin masking if there is any
    if (m_inputWorkspace->hasMaskedBins(i)) {
      const MatrixWorkspace::MaskList &inputMasks =
          m_inputWorkspace->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it) {
        const size_t maskIndex = (*it).first;
        if (maskIndex >= m_minX && maskIndex < m_maxX - m_histogram)
          outputWorkspace->flagMasked(j, maskIndex - m_minX, (*it).second);
      }
    }
    // When cropping in place, you can clear out old memory from the input one!
    if (inPlace) {
      eventW->getSpectrum(i).clear();
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", std::move(outputWorkspace));
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
    if (m_minX == m_maxX && m_commonBoundaries && eventW == nullptr) {
      g_log.error("The X range given lies entirely within a single bin");
      throw std::out_of_range(
          "The X range given lies entirely within a single bin");
    }
    m_croppingInX = true;
  }
  if (!m_commonBoundaries)
    m_minX = 0;
  if (!m_commonBoundaries)
    m_maxX = static_cast<int>(m_inputWorkspace->x(0).size());

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
      int minSpec = getProperty("StartWorkspaceIndex");
      const int numberOfSpectra =
          static_cast<int>(m_inputWorkspace->getNumberHistograms());
      int maxSpec = getProperty("EndWorkspaceIndex");
      if (isEmpty(maxSpec))
        maxSpec = numberOfSpectra - 1;

      // Check 'StartSpectrum' is in range 0-numberOfSpectra
      if (minSpec > numberOfSpectra - 1) {
        g_log.error("StartWorkspaceIndex out of range!");
        throw std::out_of_range("StartSpectrum out of range!");
      }
      if (maxSpec > numberOfSpectra - 1) {
        g_log.error("EndWorkspaceIndex out of range!");
        throw std::out_of_range("EndWorkspaceIndex out of range!");
      }
      if (maxSpec < minSpec) {
        g_log.error("StartWorkspaceIndex must be less than or equal to "
                    "EndWorkspaceIndex");
        throw std::out_of_range(
            "StartWorkspaceIndex must be less than or equal "
            "to EndWorkspaceIndex");
      }
      m_workspaceIndexList.reserve(maxSpec - minSpec + 1);
      for (size_t i = static_cast<size_t>(minSpec);
           i <= static_cast<size_t>(maxSpec); ++i) {
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
size_t ExtractSpectra::getXMin(const int wsIndex) {
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
size_t ExtractSpectra::getXMax(const int wsIndex) {
  auto &X = m_inputWorkspace->x(wsIndex);
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
 *  @param outputWorkspace :: The output workspace - data has already been
 * copied
 *  @param inIndex ::         The workspace index of the spectrum in the input
 * workspace
 *  @param outIndex ::        The workspace index of the spectrum in the output
 * workspace
 */
void ExtractSpectra::cropRagged(API::MatrixWorkspace_sptr outputWorkspace,
                                int inIndex, int outIndex) {
  auto &Y = outputWorkspace->mutableY(outIndex);
  auto &E = outputWorkspace->mutableE(outIndex);
  const size_t size = Y.size();
  size_t startX = this->getXMin(inIndex);
  if (startX > size)
    startX = size;
  for (size_t i = 0; i < startX; ++i) {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
  size_t endX = this->getXMax(inIndex);
  if (endX > 0)
    endX -= m_histogram;
  for (size_t i = endX; i < size; ++i) {
    Y[i] = 0.0;
    E[i] = 0.0;
  }
}

} // namespace Algorithms
} // namespace Mantid
