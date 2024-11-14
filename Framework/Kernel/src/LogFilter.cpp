// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid::Kernel {

namespace {
// local name for a time that has been made with the default constructor
const DateAndTime UNSET_TIME(DateAndTime::GPS_EPOCH);
} // namespace

/**
 * Constructor taking a reference to a filter. Note that constructing a
 * LogFilter this
 * way only allows filters to be combined. They will not affect a property
 * @param filter :: A reference to a TimeSeriesProperty<bool> that will act as a
 * filter
 */
LogFilter::LogFilter(const TimeSeriesProperty<bool> &filter)
    : m_prop(), m_filter(std::make_unique<TimeSeriesProperty<bool>>("filter")) {
  addFilter(filter);
}

/**
 * Constructor
 * @param prop :: Pointer to property to be filtered. Its actual type must be
 * TimeSeriesProperty<double>
 */
LogFilter::LogFilter(const Property *prop) : m_filter(std::make_unique<TimeSeriesProperty<bool>>("filter")) {
  m_prop.reset(convertToTimeSeriesOfDouble(prop));
}

/**
 * / Constructor from a TimeSeriesProperty<double> object to avoid overhead of
 * casts
 */
LogFilter::LogFilter(const TimeSeriesProperty<double> *timeSeries)
    : m_filter(std::make_unique<TimeSeriesProperty<bool>>("filter")) {
  m_prop.reset(convertToTimeSeriesOfDouble(timeSeries));
}

/**
 * Filter using a TimeSeriesProperty<bool>. True values mark the allowed time
 * intervals.
 * The object is cloned
 * @param filter :: Filtering mask
 */
void LogFilter::addFilter(const TimeSeriesProperty<bool> &filter) {
  // do nothing with empty filter
  if (filter.size() == 0)
    return;
  // a single value of one at default GPS epoch is also ignorable
  if (filter.size() == 1 && filter.firstTime() == UNSET_TIME)
    return;

  // clear the filter first
  if (m_prop) {
    m_prop->clearFilter();
  }

  // get a local copy of the values for a check further down
  const auto values = filter.valuesAsVector();

  // If the filter ends in USE
  bool filterOpenEnded;

  if (std::find(values.cbegin(), values.cend(), true) == values.cend()) {
    // reset the filter to empty if the new one is all ignore
    // this assumes that the filter is non-empty because
    // that is checked for above
    filterOpenEnded = false;
    this->setFilter(TimeROI(), filterOpenEnded);
  } else if (!m_filter || m_filter->size() == 0) {
    // clean-up and replace current filter
    filterOpenEnded = filter.lastValue();

    // TimeROI constructor does a ton of cleanup
    TimeROI mine(&filter);

    // put the results back into the TimeSeriesProperty
    this->setFilter(mine, filterOpenEnded);
  } else {
    // determine if the current version of things are open-ended
    filterOpenEnded = (m_filter->lastValue() && filter.lastValue());

    // create a local copy of both filters to add extra values to
    auto filter1 = m_filter.get();
    auto filter2 = std::unique_ptr<TimeSeriesProperty<bool>>(filter.clone());

    TimeInterval time1 = filter1->nthInterval(filter1->size() - 1);
    TimeInterval time2 = filter2->nthInterval(filter2->size() - 1);

    if (time1.start() < time2.start()) {
      filter1->addValue(time2.start(),
                        true); // should be f1->lastValue, but it doesnt
                               // matter for boolean AND
    } else if (time2.start() < time1.start()) {
      filter2->addValue(time1.start(), true);
    }

    // temporary objects to handle the intersection calculation
    TimeROI mine(filter1);
    TimeROI theirs(filter2.get());
    mine.update_intersection(theirs);

    // put the results into the TimeSeriesProperty
    this->setFilter(mine, filterOpenEnded);
  }

  // apply the filter to the property
  if (m_prop) {
    // create a TimeROI to apply
    TimeROI timeroi;

    // if the end point is a open, then the last point in the TimeROI needs to be modified to make things work out
    if (filterOpenEnded) {
      if (m_filter->size() == 1 && m_filter->lastValue()) {
        // create a pretend ROI from the start to the year 2100
        timeroi.addROI(m_filter->lastTime(), DateAndTime("2100-Jan-01 00:00:01"));
      } else {
        // default constructor is fine
        timeroi = TimeROI(m_filter.get());
      }

      // do a test filtering so the second to last interval can be inspected
      m_prop->filterWith(timeroi);
      // determine how far out to adjust the last ROI
      const auto mysize = m_prop->size();
      const auto penultimate_interval = m_prop->nthInterval(mysize - 2);
      const auto ultimate_interval = m_prop->nthInterval(mysize - 1);
      const auto oldEnd = ultimate_interval.stop();
      const auto newEnd = ultimate_interval.start() + penultimate_interval.length();
      // extend or shrink the last ROI depending on which value is bigger
      if (oldEnd > newEnd)
        timeroi.addMask(newEnd, ultimate_interval.stop());
      else if (newEnd > oldEnd)
        timeroi.addROI(oldEnd, newEnd);
      // both ends match (lucky guess earlier) so there is nothing to adjust
    } else {
      timeroi = TimeROI(m_filter.get());
    }

    // apply the filter
    m_prop->filterWith(timeroi);
  }
}

//-------------------------------------------------------------------------------------------------
/// Clears filters
void LogFilter::clear() {
  if (m_prop)
    m_prop->clearFilter();
  m_filter.reset();
}

//--------------------------------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------------------------------
namespace {
template <typename SrcType> struct ConvertToTimeSeriesDouble {
  static FilteredTimeSeriesProperty<double> *apply(const Property *prop) {
    auto srcTypeSeries = dynamic_cast<const TimeSeriesProperty<SrcType> *>(prop);
    if (!srcTypeSeries)
      return nullptr;
    auto converted = new FilteredTimeSeriesProperty<double>(prop->name());
    auto pmap = srcTypeSeries->valueAsMap();
    for (auto it = pmap.begin(); it != pmap.end(); ++it) {
      converted->addValue(it->first, double(it->second));
    }
    return converted;
  }
};
} // namespace

/**
 * Converts the given property to a TimeSeriesProperty<double>, throws if
 * invalid.
 * @param prop :: A pointer to a property
 * @return A new TimeSeriesProperty<double> object converted from the input.
 * Throws std::invalid_argument if not possible
 */
FilteredTimeSeriesProperty<double> *LogFilter::convertToTimeSeriesOfDouble(const Property *prop) {
  if (auto doubleSeries = ConvertToTimeSeriesDouble<double>::apply(prop)) {
    return doubleSeries;
  } else if (auto doubleSeries = ConvertToTimeSeriesDouble<int>::apply(prop)) {
    return doubleSeries;
  } else if (auto doubleSeries = ConvertToTimeSeriesDouble<bool>::apply(prop)) {
    return doubleSeries;
  } else {
    throw std::invalid_argument("LogFilter::convertToTimeSeriesOfDouble - Cannot convert property, \"" + prop->name() +
                                "\", to double series.");
  }
}

void LogFilter::setFilter(const TimeROI &roi, const bool filterOpenEnded) {
  // put the results into the TimeSeriesProperty
  std::vector<bool> values;
  std::vector<DateAndTime> times;
  for (const auto &splitter : roi.toTimeIntervals()) {
    values.emplace_back(true);
    values.emplace_back(false);
    times.emplace_back(splitter.start());
    times.emplace_back(splitter.stop());
  }
  // if both are open ended, remove the ending
  if (filterOpenEnded) {
    times.pop_back();
    values.pop_back();
  }

  // set as the filter
  if (!m_filter) {
    m_filter = std::make_unique<TimeSeriesProperty<bool>>("filter");
  }
  m_filter->replaceValues(times, values);
}

} // namespace Mantid::Kernel
