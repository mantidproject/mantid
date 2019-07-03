// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Kernel {
/**
 * Constructor taking a reference to a filter. Note that constructing a
 * LogFilter this
 * way only allows filters to be combined. They will not affect a property
 * @param filter :: A reference to a TimeSeriesProperty<bool> that will act as a
 * filter
 */
LogFilter::LogFilter(const TimeSeriesProperty<bool> &filter)
    : m_prop(), m_filter() {
  addFilter(filter);
}

/**
 * Constructor
 * @param prop :: Pointer to property to be filtered. Its actual type must be
 * TimeSeriesProperty<double>
 */
LogFilter::LogFilter(const Property *prop) : m_prop(), m_filter() {
  m_prop.reset(convertToTimeSeriesOfDouble(prop));
}

/**
 * / Constructor from a TimeSeriesProperty<double> object to avoid overhead of
 * casts
 */
LogFilter::LogFilter(const TimeSeriesProperty<double> *timeSeries)
    : m_prop(), m_filter() {
  m_prop.reset(timeSeries->clone());
}

/**
 * Filter using a TimeSeriesProperty<bool>. True values mark the allowed time
 * intervals.
 * The object is cloned
 * @param filter :: Filtering mask
 */
void LogFilter::addFilter(const TimeSeriesProperty<bool> &filter) {
  if (filter.size() == 0)
    return;
  if (!m_filter || m_filter->size() == 0)
    m_filter.reset(filter.clone());
  else {
    auto filterProperty = std::make_unique<TimeSeriesProperty<bool>>("tmp");

    TimeSeriesProperty<bool> *filter1 = m_filter.get();
    auto filter2 = std::unique_ptr<TimeSeriesProperty<bool>>(filter.clone());

    TimeInterval time1 = filter1->nthInterval(filter1->size() - 1);
    TimeInterval time2 = filter2->nthInterval(filter2->size() - 1);

    if (time1.begin() < time2.begin()) {
      filter1->addValue(time2.begin(),
                        true); // should be f1->lastValue, but it doesnt
                               // matter for boolean AND
    } else if (time2.begin() < time1.begin()) {
      filter2->addValue(time1.begin(), true);
    }

    int i = 0;
    int j = 0;

    time1 = filter1->nthInterval(i);
    time2 = filter2->nthInterval(j);

    // Make the two filters start at the same time. An entry is added at the
    // beginning
    // of the filter that starts later to equalise their staring times. The new
    // interval will have
    // value opposite to the one it started with originally.
    if (time1.begin() > time2.begin()) {
      filter1->addValue(time2.begin(), !filter1->nthValue(i));
      time1 = filter1->nthInterval(i);
    } else if (time2.begin() > time1.begin()) {
      filter2->addValue(time1.begin(), !filter2->nthValue(j));
      time2 = filter2->nthInterval(j);
    }

    for (;;) {
      TimeInterval time3;
      time3 = time1.intersection(time2);
      if (time3.isValid()) {
        filterProperty->addValue(
            time3.begin(), (filter1->nthValue(i) && filter2->nthValue(j)));
      }

      if (time1.end() < time2.end()) {
        i++;
      } else if (time2.end() < time1.end()) {
        j++;
      } else {
        i++;
        j++;
      }

      if (i == filter1->size() || j == filter2->size())
        break;
      time1 = filter1->nthInterval(i);
      time2 = filter2->nthInterval(j);
    }
    filterProperty->clearFilter();
    m_filter = std::move(filterProperty);
  }
  if (m_prop) {
    m_prop->clearFilter();
    m_prop->filterWith(m_filter.get());
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
  static TimeSeriesProperty<double> *apply(const Property *prop) {
    auto srcTypeSeries =
        dynamic_cast<const TimeSeriesProperty<SrcType> *>(prop);
    if (!srcTypeSeries)
      return nullptr;
    auto converted = new TimeSeriesProperty<double>(prop->name());
    auto pmap = srcTypeSeries->valueAsMap();
    for (auto it = pmap.begin(); it != pmap.end(); ++it) {
      converted->addValue(it->first, double(it->second));
    }
    return converted;
  }
};

/// Specialization for a double so that it just clones the input
template <> struct ConvertToTimeSeriesDouble<double> {
  static TimeSeriesProperty<double> *apply(const Property *prop) {
    auto doubleSeries = dynamic_cast<const TimeSeriesProperty<double> *>(prop);
    if (!doubleSeries)
      return nullptr;
    return doubleSeries->clone();
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
TimeSeriesProperty<double> *
LogFilter::convertToTimeSeriesOfDouble(const Property *prop) {
  if (auto doubleSeries = ConvertToTimeSeriesDouble<double>::apply(prop)) {
    return doubleSeries;
  } else if (auto doubleSeries = ConvertToTimeSeriesDouble<int>::apply(prop)) {
    return doubleSeries;
  } else if (auto doubleSeries = ConvertToTimeSeriesDouble<bool>::apply(prop)) {
    return doubleSeries;
  } else {
    throw std::invalid_argument(
        "LogFilter::convertToTimeSeriesOfDouble - Cannot convert property, \"" +
        prop->name() + "\", to double series.");
  }
}

} // namespace Kernel
} // namespace Mantid
