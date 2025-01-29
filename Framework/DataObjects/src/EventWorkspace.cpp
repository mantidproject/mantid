// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "tbb/parallel_for.h"
#include <algorithm>
#include <limits>
#include <numeric>

using namespace boost::posix_time;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::DataObjects {
namespace {
// static logger
Kernel::Logger g_log("EventWorkspace");
} // namespace

DECLARE_WORKSPACE(EventWorkspace)

using Kernel::Exception::NotImplementedError;
using std::size_t;
using namespace Mantid::Kernel;

EventWorkspace::EventWorkspace() : IEventWorkspace(), mru(std::make_unique<EventWorkspaceMRU>()) {}

EventWorkspace::EventWorkspace(const EventWorkspace &other)
    : IEventWorkspace(other), mru(std::make_unique<EventWorkspaceMRU>()) {
  for (const auto &el : other.data) {
    // Create a new event list, copying over the events
    auto newel = std::make_unique<EventList>(*el);
    // Make sure to update the MRU to point to THIS event workspace.
    newel->setMRU(this->mru.get());
    this->data.emplace_back(std::move(newel));
  }
}

EventWorkspace::~EventWorkspace() { data.clear(); }

/** Returns true if the EventWorkspace is safe for multithreaded operations.
 * WARNING: This is only true for OpenMP threading. EventWorkspace is NOT thread
 * safe with Poco threads or other threading mechanisms.
 */
bool EventWorkspace::threadSafe() const {
  // Since there is a mutex lock around sorting, EventWorkspaces are always
  // safe.
  return true;
}

/** Initialize the pixels
 *  @param NVectors :: The number of vectors/histograms/detectors in the
 * workspace. Does not need
 *         to be set, but needs to be > 0
 *  @param XLength :: The number of X data points/bin boundaries in each vector
 * (ignored)
 *  @param YLength :: The number of data/error points in each vector (ignored)
 */
void EventWorkspace::init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength) {
  static_cast<void>(XLength);
  static_cast<void>(YLength);

  // Check validity of arguments
  if (NVectors == 0) {
    throw std::out_of_range("Zero pixels specified to EventWorkspace::init");
  }

  // Set each X vector to have one bin of 0 & extremely close to zero
  // Move the rhs very,very slightly just incase something doesn't like them
  // being the same
  HistogramData::BinEdges edges{0.0, std::numeric_limits<double>::min()};

  // Initialize the data
  data.resize(NVectors);
  // Make sure SOMETHING exists for all initialized spots.
  EventList el;
  el.setHistogram(edges);
  for (size_t i = 0; i < NVectors; i++) {
    data[i] = std::make_unique<EventList>(el);
    data[i]->setMRU(mru.get());
    data[i]->setSpectrumNo(specnum_t(i));
  }

  // Create axes.
  m_axes.resize(2);
  m_axes[0] = std::make_unique<API::RefAxis>(this);
  m_axes[1] = std::make_unique<API::SpectraAxis>(this);
}

void EventWorkspace::init(const HistogramData::Histogram &histogram) {
  if (histogram.xMode() != HistogramData::Histogram::XMode::BinEdges)
    throw std::runtime_error("EventWorkspace can only be initialized with XMode::BinEdges");

  if (histogram.sharedY() || histogram.sharedE())
    throw std::runtime_error("EventWorkspace cannot be initialized non-NULL Y or E data");

  data.resize(numberOfDetectorGroups());
  EventList el;
  el.setHistogram(histogram);
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = std::make_unique<EventList>(el);
    data[i]->setMRU(mru.get());
    data[i]->setSpectrumNo(specnum_t(i));
  }

  m_axes.resize(2);
  m_axes[0] = std::make_unique<API::RefAxis>(this);
  m_axes[1] = std::make_unique<API::SpectraAxis>(this);
}

///  Returns true if the workspace is ragged (has differently sized spectra).
/// @returns true if the workspace is ragged.
bool EventWorkspace::isRaggedWorkspace() const {
  if (data.empty()) {
    throw std::runtime_error("There are no pixels in the event workspace, "
                             "therefore cannot determine if it is ragged.");
  } else {
    const auto numberOfBins = data[0]->histogram_size();
    return std::any_of(data.cbegin(), data.cend(),
                       [&numberOfBins](const auto &eventList) { return numberOfBins != eventList->histogram_size(); });
  }
}

/// The total size of the workspace
/// @returns the number of single indexable items in the workspace
size_t EventWorkspace::size() const {
  return std::accumulate(
      data.begin(), data.end(), static_cast<size_t>(0),
      [](size_t value, const std::unique_ptr<EventList> &histo) { return value + histo->histogram_size(); });
}

/// Get the blocksize, aka the number of bins in the histogram
/// @returns the number of bins in the Y data
size_t EventWorkspace::blocksize() const {
  if (data.empty()) {
    throw std::range_error("EventWorkspace::blocksize, no pixels in workspace, "
                           "therefore cannot determine blocksize (# of bins).");
  } else {
    size_t numBins = data[0]->histogram_size();
    const auto iterPos = std::find_if_not(data.cbegin(), data.cend(),
                                          [numBins](const auto &iter) { return numBins == iter->histogram_size(); });
    if (iterPos != data.cend())
      throw std::length_error("blocksize undefined because size of histograms is not equal");
    return numBins;
  }
}

/** Returns the number of bins for a given histogram index.
 * @param index :: The histogram index to check for the number of bins.
 * @return the number of bins for a given histogram index.
 */
std::size_t EventWorkspace::getNumberBins(const std::size_t &index) const {
  if (index < data.size())
    return data[index]->histogram_size();

  throw std::invalid_argument("Could not find number of bins in a histogram at index " + std::to_string(index) +
                              ": index is too large.");
}

/** Returns the maximum number of bins in a workspace (works on ragged data).
 * @return the maximum number of bins in a workspace.
 */
std::size_t EventWorkspace::getMaxNumberBins() const {
  if (data.empty()) {
    return 0;
  } else {
    auto maxNumberOfBins = data[0]->histogram_size();
    for (const auto &iter : data) {
      const auto numberOfBins = iter->histogram_size();
      if (numberOfBins > maxNumberOfBins)
        maxNumberOfBins = numberOfBins;
    }
    return maxNumberOfBins;
  }
}

/** Get the number of histograms, usually the same as the number of pixels or
 detectors.
 @returns the number of histograms / event lists
 */
size_t EventWorkspace::getNumberHistograms() const { return this->data.size(); }

/// Return const reference to EventList at the given workspace index.
EventList &EventWorkspace::getSpectrumWithoutInvalidation(const size_t index) {
  auto &spec = const_cast<EventList &>(static_cast<const EventWorkspace &>(*this).getSpectrum(index));
  spec.setMatrixWorkspace(this, index);
  return spec;
}

/// Return const reference to EventList at the given workspace index.
const EventList &EventWorkspace::getSpectrum(const size_t index) const {
  if (index >= data.size())
    throw std::range_error("EventWorkspace::getSpectrum, workspace index out of range");
  return *data[index];
}

/**
 * Returns a pointer to the EventList for a given spectrum in a timely manner.
 *
 * Very minimal checking and preprocessing is performed by this function and it
 * should only be used in tight loops where getSpectrum is too costly.
 *
 * See the implementation of the non-const getSpectrum to see what is missing.
 *
 * @param index Workspace index
 * @return Pointer to EventList
 */
EventList *EventWorkspace::getSpectrumUnsafe(const size_t index) { return data[index].get(); }

double EventWorkspace::getTofMin() const { return this->getEventXMin(); }

double EventWorkspace::getTofMax() const { return this->getEventXMax(); }

/**
 Get the minimum pulse time for events accross the entire workspace.
 @return minimum pulse time as a DateAndTime.
 */
DateAndTime EventWorkspace::getPulseTimeMin() const {
  // set to crazy values to start
  Mantid::Types::Core::DateAndTime tMin = DateAndTime::maximum();
  size_t numWorkspace = this->data.size();
  DateAndTime temp;
  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const EventList &evList = this->getSpectrum(workspaceIndex);
    temp = evList.getPulseTimeMin();
    if (temp < tMin)
      tMin = temp;
  }
  return tMin;
}

/**
 Get the maximum pulse time for events accross the entire workspace.
 @return maximum pulse time as a DateAndTime.
 */
DateAndTime EventWorkspace::getPulseTimeMax() const {
  // set to crazy values to start
  Mantid::Types::Core::DateAndTime tMax = DateAndTime::minimum();
  size_t numWorkspace = this->data.size();
  DateAndTime temp;
  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const EventList &evList = this->getSpectrum(workspaceIndex);
    temp = evList.getPulseTimeMax();
    if (temp > tMax)
      tMax = temp;
  }
  return tMax;
}
/**
Get the maximum and mimumum pulse time for events accross the entire workspace.
@param Tmin minimal pulse time as a DateAndTime.
@param Tmax maximal pulse time as a DateAndTime.
*/
void EventWorkspace::getPulseTimeMinMax(Mantid::Types::Core::DateAndTime &Tmin,
                                        Mantid::Types::Core::DateAndTime &Tmax) const {

  Tmax = DateAndTime::minimum();
  Tmin = DateAndTime::maximum();

  auto numWorkspace = static_cast<int64_t>(this->data.size());
#pragma omp parallel
  {
    DateAndTime tTmax = DateAndTime::minimum();
    DateAndTime tTmin = DateAndTime::maximum();
#pragma omp for nowait
    for (int64_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
      const EventList &evList = this->getSpectrum(workspaceIndex);
      DateAndTime tempMin, tempMax;
      evList.getPulseTimeMinMax(tempMin, tempMax);
      tTmin = std::min(tTmin, tempMin);
      tTmax = std::max(tTmax, tempMax);
    }
#pragma omp critical
    {
      Tmin = std::min(Tmin, tTmin);
      Tmax = std::max(Tmax, tTmax);
    }
  }
}

/**
 Get the minimum time at sample for events across the entire workspace.
 @param tofOffset :: Time of flight offset. defaults to zero.
 @return minimum time at sample as a DateAndTime.
 */
DateAndTime EventWorkspace::getTimeAtSampleMin(double tofOffset) const {
  const auto &specInfo = spectrumInfo();
  const auto L1 = specInfo.l1();

  // set to crazy values to start
  Mantid::Types::Core::DateAndTime tMin = DateAndTime::maximum();
  size_t numWorkspace = this->data.size();
  DateAndTime temp;

  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const auto L2 = specInfo.l2(workspaceIndex);
    const double tofFactor = L1 / (L1 + L2);

    const EventList &evList = this->getSpectrum(workspaceIndex);
    temp = evList.getTimeAtSampleMin(tofFactor, tofOffset);
    if (temp < tMin)
      tMin = temp;
  }
  return tMin;
}

/**
 Get the maximum time at sample for events across the entire workspace.
 @param tofOffset :: Time of flight offset. defaults to zero.
 @return maximum time at sample as a DateAndTime.
 */
DateAndTime EventWorkspace::getTimeAtSampleMax(double tofOffset) const {
  const auto &specInfo = spectrumInfo();
  const auto L1 = specInfo.l1();

  // set to crazy values to start
  Mantid::Types::Core::DateAndTime tMax = DateAndTime::minimum();
  size_t numWorkspace = this->data.size();
  DateAndTime temp;
  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const auto L2 = specInfo.l2(workspaceIndex);
    const double tofFactor = L1 / (L1 + L2);

    const EventList &evList = this->getSpectrum(workspaceIndex);
    temp = evList.getTimeAtSampleMax(tofFactor, tofOffset);
    if (temp > tMax)
      tMax = temp;
  }
  return tMax;
}

/**
 * Get them minimum x-value for the events themselves, ignoring the histogram
 * representation.
 *
 * @return The minimum x-value for the all events.
 *
 * This does copy some of the code from getEventXMinXMax, but that is because
 * getting both min and max then throwing away the max is significantly slower
 * on an unsorted event list.
 */
double EventWorkspace::getEventXMin() const {
  // set to crazy values to start
  double xmin = std::numeric_limits<double>::max();
  if (this->getNumberEvents() == 0)
    return xmin;
  size_t numWorkspace = this->data.size();
  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const EventList &evList = this->getSpectrum(workspaceIndex);
    const double temp = evList.getTofMin();
    if (temp < xmin)
      xmin = temp;
  }
  return xmin;
}

/**
 * Get them maximum x-value for the events themselves, ignoring the histogram
 * representation.
 *
 * @return The maximum x-value for the all events.
 *
 * This does copy some of the code from getEventXMinXMax, but that is because
 * getting both min and max then throwing away the min is significantly slower
 * on an unsorted event list.
 */
double EventWorkspace::getEventXMax() const {
  // set to crazy values to start
  double xmax = std::numeric_limits<double>::lowest();
  if (this->getNumberEvents() == 0)
    return xmax;
  size_t numWorkspace = this->data.size();
  for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
    const EventList &evList = this->getSpectrum(workspaceIndex);
    const double temp = evList.getTofMax();
    if (temp > xmax)
      xmax = temp;
  }
  return xmax;
}

/**
 * Get them minimum and maximum x-values for the events themselves, ignoring the
 * histogram representation. Since this does not modify the sort order, the
 * method
 * will run significantly faster on a TOF_SORT event list.
 */
void EventWorkspace::getEventXMinMax(double &xmin, double &xmax) const {
  // set to crazy values to start
  xmin = std::numeric_limits<double>::max();
  xmax = -1.0 * xmin;
  if (this->getNumberEvents() == 0)
    return;

  auto numWorkspace = static_cast<int64_t>(this->data.size());
#pragma omp parallel
  {
    double tXmin = xmin;
    double tXmax = xmax;
#pragma omp for nowait
    for (int64_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++) {
      const EventList &evList = this->getSpectrum(workspaceIndex);
      tXmin = std::min(evList.getTofMin(), tXmin);
      tXmax = std::max(evList.getTofMax(), tXmax);
    }
#pragma omp critical
    {
      xmin = std::min(xmin, tXmin);
      xmax = std::max(xmax, tXmax);
    }
  }
}

/// The total number of events across all of the spectra.
/// @returns The total number of events
size_t EventWorkspace::getNumberEvents() const {
  return std::accumulate(data.cbegin(), data.cend(), size_t{0},
                         [](const auto total, const auto &list) { return total + list->getNumberEvents(); });
}

/** Get the EventType of the most-specialized EventList in the workspace
 *
 * @return the EventType of the most-specialized EventList in the workspace
 */
Mantid::API::EventType EventWorkspace::getEventType() const {
  Mantid::API::EventType out = Mantid::API::TOF;
  for (const auto &list : this->data) {
    Mantid::API::EventType thisType = list->getEventType();
    if (static_cast<int>(out) < static_cast<int>(thisType)) {
      out = thisType;
      // This is the most-specialized it can get.
      if (out == Mantid::API::WEIGHTED_NOTIME)
        return out;
    }
  }
  return out;
}

/** Switch all event lists to the given event type
 *
 * @param type :: EventType to switch to
 */
void EventWorkspace::switchEventType(const Mantid::API::EventType type) {
  for (auto &eventList : this->data)
    eventList->switchTo(type);
}

/// Returns true always - an EventWorkspace always represents histogramm-able
/// data
/// @returns If the data is a histogram - always true for an eventWorkspace
bool EventWorkspace::isHistogramData() const { return true; }

/** Return how many entries in the Y MRU list are used.
 * Only used in tests. It only returns the 0-th MRU list size.
 * @return :: number of entries in the MRU list.
 */
size_t EventWorkspace::MRUSize() const { return mru->MRUSize(); }

/** Clears the MRU lists */
void EventWorkspace::clearMRU() const { mru->clear(); }

/// Returns the amount of memory used in bytes
size_t EventWorkspace::getMemorySize() const {
  // TODO: Add the MRU buffer

  // Add the memory from all the event lists
  size_t total = std::accumulate(data.begin(), data.end(), size_t{0},
                                 [](size_t total, auto &list) { return total + list->getMemorySize(); });

  total += run().getMemorySize();

  total += this->getMemorySizeForXAxes();

  // Return in bytes
  return total;
}

/// Deprecated, use mutableX() instead. Return the data X vector at a given
/// workspace index
/// @param index :: the workspace index to return
/// @returns A reference to the vector of binned X values
MantidVec &EventWorkspace::dataX(const std::size_t index) { return getSpectrum(index).dataX(); }

/// Deprecated, use mutableDx() instead. Return the data X error vector at a
/// given workspace index
/// @param index :: the workspace index to return
/// @returns A reference to the vector of binned error values
MantidVec &EventWorkspace::dataDx(const std::size_t index) { return getSpectrum(index).dataDx(); }

/// Deprecated, use mutableY() instead. Return the data Y vector at a given
/// workspace index
/// Note: these non-const access methods will throw NotImplementedError
MantidVec &EventWorkspace::dataY(const std::size_t /*index*/) {
  throw NotImplementedError("EventWorkspace::dataY cannot return a non-const "
                            "array: you can't modify the histogrammed data in "
                            "an EventWorkspace!");
}

/// Deprecated, use mutableE() instead. Return the data E vector at a given
/// workspace index
/// Note: these non-const access methods will throw NotImplementedError
MantidVec &EventWorkspace::dataE(const std::size_t /*index*/) {
  throw NotImplementedError("EventWorkspace::dataE cannot return a non-const "
                            "array: you can't modify the histogrammed data in "
                            "an EventWorkspace!");
}

/** Deprecated, use x() instead.
 * @return the const data X vector at a given workspace index
 * @param index :: workspace index   */
const MantidVec &EventWorkspace::dataX(const std::size_t index) const { return getSpectrum(index).readX(); }

/** Deprecated, use dx() instead.
 * @return the const data X error vector at a given workspace index
 * @param index :: workspace index   */
const MantidVec &EventWorkspace::dataDx(const std::size_t index) const { return getSpectrum(index).readDx(); }

/** Deprecated, use y() instead.
 * @return the const data Y vector at a given workspace index
 * @param index :: workspace index   */
const MantidVec &EventWorkspace::dataY(const std::size_t index) const { return getSpectrum(index).readY(); }

/** Deprecated, use e() instead.
 * @return the const data E (error) vector at a given workspace index
 * @param index :: workspace index   */
const MantidVec &EventWorkspace::dataE(const std::size_t index) const { return getSpectrum(index).readE(); }

/** Deprecated, use sharedX() instead.
 * @return a pointer to the X data vector at a given workspace index
 * @param index :: workspace index   */
Kernel::cow_ptr<HistogramData::HistogramX> EventWorkspace::refX(const std::size_t index) const {
  return getSpectrum(index).ptrX();
}

/** Using the event data in the event list, generate a histogram of it w.r.t
 *TOF.
 *
 * @param index :: workspace index to generate
 * @param X :: input X vector of the bin boundaries.
 * @param Y :: output vector to be filled with the Y data.
 * @param E :: output vector to be filled with the Error data (optionally)
 * @param skipError :: if true, the error vector is NOT calculated.
 *        This may save some processing time.
 */
void EventWorkspace::generateHistogram(const std::size_t index, const MantidVec &X, MantidVec &Y, MantidVec &E,
                                       bool skipError) const {
  if (index >= data.size())
    throw std::range_error("EventWorkspace::generateHistogram, histogram number out of range");
  this->data[index]->generateHistogram(X, Y, E, skipError);
}

/** Using the event data in the event list, generate a histogram of it w.r.t
 *PULSE TIME.
 *
 * @param index :: workspace index to generate
 * @param X :: input X vector of the bin boundaries.
 * @param Y :: output vector to be filled with the Y data.
 * @param E :: output vector to be filled with the Error data (optionally)
 * @param skipError :: if true, the error vector is NOT calculated.
 *        This may save some processing time.
 */
void EventWorkspace::generateHistogramPulseTime(const std::size_t index, const MantidVec &X, MantidVec &Y, MantidVec &E,
                                                bool skipError) const {
  if (index >= data.size())
    throw std::range_error("EventWorkspace::generateHistogramPulseTime, "
                           "histogram number out of range");
  this->data[index]->generateHistogramPulseTime(X, Y, E, skipError);
}

/** Set all histogram X vectors.
 * @param x :: The X vector of histogram bins to use.
 */
void EventWorkspace::setAllX(const HistogramData::BinEdges &x) {
  // This is an EventWorkspace, so changing X size is ok as long as we clear
  // the MRU below, i.e., we avoid the size check of Histogram::setBinEdges and
  // just reset the whole Histogram.
  invalidateCommonBinsFlag();
  for (auto &eventList : this->data)
    eventList->setHistogram(x);

  // Clear MRU lists now, free up memory
  this->clearMRU();
}

/**
 * Set all Histogram X vectors to a single bin with boundaries that fit the
 * TOF of all events across all spectra.
 */
void EventWorkspace::resetAllXToSingleBin() {
  double tofmin, tofmax;
  getEventXMinMax(tofmin, tofmax);

  // Sanitize TOF min/max to ensure it always passes HistogramX validation.
  // They would be invalid when number of events are 0 or 1, for example.
  if (tofmin > tofmax) {
    tofmin = 0;
    tofmax = std::numeric_limits<double>::min();
  } else if (tofmin == tofmax) {
    tofmax += std::numeric_limits<double>::min();
  }
  setAllX({tofmin, tofmax});
}

/** Task for sorting an event list */
class EventSortingTask {
public:
  /// ctor
  EventSortingTask(const EventWorkspace *WS, EventSortType sortType, Mantid::API::Progress *prog)
      : m_sortType(sortType), m_WS(WS), prog(prog) {}

  // Execute the sort as specified.
  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t wi = range.begin(); wi < range.end(); ++wi) {
      // because EventList::sort calls tbb::parallel_sort checking that the EventList is non-empty reduces the number of
      // threads created that return immediately
      const auto &spectrum = m_WS->getSpectrum(wi); // follow the method signature
      if (spectrum.empty())
        spectrum.setSortOrder(m_sortType); // empty lists are easy to sort
      else
        spectrum.sort(m_sortType);
    }
    // Report progress
    if (prog)
      prog->report("Sorting");
  }

private:
  /// How to sort
  EventSortType m_sortType;
  /// EventWorkspace on which to sort
  const EventWorkspace *m_WS;
  /// Optional Progress dialog.
  Mantid::API::Progress *prog;
};

/*
 * Review each event list to get the sort type
 * If any 2 have different order type, then be unsorted
 */
EventSortType EventWorkspace::getSortType() const {
  size_t dataSize = this->data.size();
  EventSortType order = data[0]->getSortType();
  for (size_t i = 1; i < dataSize; i++) {
    if (order != data[i]->getSortType())
      return UNSORTED;
  }
  return order;
}

/*** Sort all event lists. Uses a parallelized algorithm
 * @param sortType :: How to sort the event lists.
 * @param prog :: a progress report object. If the pointer is not NULL, each
 * event list will call prog.report() once.
 */
void EventWorkspace::sortAll(EventSortType sortType, Mantid::API::Progress *prog) const {
  if (this->getSortType() == sortType) {
    if (prog != nullptr) {
      prog->reportIncrement(this->data.size());
    }
    return;
  }

  // Create the thread pool using tbb
  EventSortingTask task(this, sortType, prog);
  constexpr size_t GRAINSIZE_DEFAULT{100}; // somewhat arbitrary
  const size_t grainsize = std::min<size_t>(GRAINSIZE_DEFAULT, (this->getNumberHistograms() / GRAINSIZE_DEFAULT) + 1);
  tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size(), grainsize), task);
}

/** Integrate all the spectra in the matrix workspace within the range given.
 * Default implementation, can be overridden by base classes if they know
 *something smarter!
 *
 * @param out :: returns the vector where there is one entry per spectrum in the
 *workspace. Same
 *            order as the workspace indices.
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 */
void EventWorkspace::getIntegratedSpectra(std::vector<double> &out, const double minX, const double maxX,
                                          const bool entireRange) const {
  // Start with empty vector
  out.resize(this->getNumberHistograms(), 0.0);

  // We can run in parallel since there is no cross-reading of event lists
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int wksp_index = 0; wksp_index < int(this->getNumberHistograms()); wksp_index++) {
    // Get Handle to data
    EventList *el = this->data[wksp_index].get();

    // Let the eventList do the integration
    out[wksp_index] = el->integrate(minX, maxX, entireRange);
  }
}

} // namespace Mantid::DataObjects

namespace Mantid::Kernel {
template <>
DLLExport Mantid::DataObjects::EventWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<EventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::EventWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_const_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<EventWorkspace>.";
    throw std::runtime_error(message);
  }
}
} // namespace Mantid::Kernel
