// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {

using namespace Types::Core;
namespace Kernel {

/// Default constructor
SplittingInterval::SplittingInterval() : TimeInterval(), m_index(-1) {}

/// Constructor using DateAndTime
SplittingInterval::SplittingInterval(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop,
                                     const int index)
    : TimeInterval(start, stop), m_index(index) {}

/// Return the index (destination of this split time block)
int SplittingInterval::index() const { return m_index; }

/// @cond DOXYGEN_BUG
/// And operator. Return the smallest time interval where both intervals are
/// TRUE.
SplittingInterval SplittingInterval::operator&(const SplittingInterval &b) const {
  const auto &begin = std::max(this->start(), b.start());
  const auto &end = std::min(this->stop(), b.stop());

  return SplittingInterval(begin, end, this->index());
}
/// @endcond DOXYGEN_BUG

/// Or operator. Return the largest time interval.
SplittingInterval SplittingInterval::operator|(const SplittingInterval &b) const {
  if (!this->overlaps(&b))
    throw std::invalid_argument("SplittingInterval: cannot apply the OR (|) "
                                "operator to non-overlapping "
                                "SplittingInterval's.");

  const auto &begin = std::min(this->start(), b.start());
  const auto &end = std::max(this->stop(), b.stop());

  return SplittingInterval(begin, end, this->index());
}

bool SplittingInterval::operator==(const SplittingInterval &ti) const {
  if (TimeInterval::operator==(ti)) {
    return index() == ti.index();
  } else {
    return false;
  }
}

std::string SplittingInterval::debugStrPrint() const {
  std::stringstream ss;
  ss << this->begin_str() << " to " << this->end_str();
  ss << " index: " << index();
  ss << std::endl;
  return ss.str();
}
//------------------------------------------------------------------------------------------------
/** Return true if the SplittingIntervalVec provided is a filter,
 * meaning that it only has an output index of 0.
 */
bool isFilter(const SplittingIntervalVec &a) {
  int max = -1;
  SplittingIntervalVec::const_iterator it;
  for (it = a.begin(); it != a.end(); ++it)
    if (it->index() > max)
      max = it->index();
  return (max <= 0);
}

//------------------------------------------------------------------------------------------------
/** Plus operator for SplittingIntervalVec.
 * Combines a filter and a splitter by removing entries that are filtered out
 *from the splitter.
 * Also, will combine two filters together by "and"ing them
 *
 * @param a :: SplittingIntervalVec splitter OR filter
 * @param b :: SplittingIntervalVec splitter OR filter.
 * @throw std::invalid_argument if two splitters are given.
 */
SplittingIntervalVec operator+(const SplittingIntervalVec &a, const SplittingIntervalVec &b) {
  bool a_filter, b_filter;
  a_filter = isFilter(a);
  b_filter = isFilter(b);

  if (a_filter && b_filter) {
    return a & b;
  } else if (a_filter && !b_filter) {
    return b & a;
  } else if (!a_filter && b_filter) {
    return a & b;
  } else // (!a_filter && !b_filter)
  {
    // Both are splitters.
    throw std::invalid_argument("Cannot combine two splitters together, as the "
                                "output is undefined. Try splitting each "
                                "output workspace by b after the a split has "
                                "been done.");
  }
}

//------------------------------------------------------------------------------------------------
/** AND operator for SplittingIntervalVec
 * Works on Filters - combines them to only keep times where both Filters are
 *TRUE.
 * Works on splitter + filter if (a) is a splitter and b is a filter.
 *  In general, use the + operator since it will resolve the order for you.
 *
 * @param a :: SplittingIntervalVec filter or Splitter.
 * @param b :: SplittingIntervalVec filter.
 * @return the ANDed filter
 */
SplittingIntervalVec operator&(const SplittingIntervalVec &a, const SplittingIntervalVec &b) {
  SplittingIntervalVec out;
  // If either is empty, then no entries in the filter (aka everything is
  // removed)
  if ((a.empty()) || (b.empty()))
    return out;

  SplittingIntervalVec::const_iterator ait;
  SplittingIntervalVec::const_iterator bit;

  // For now, a simple double iteration. Can be made smarter if a and b are
  // sorted.
  for (ait = a.begin(); ait != a.end(); ++ait) {
    for (bit = b.begin(); bit != b.end(); ++bit) {
      if (ait->overlaps(&(*bit))) {
        // The & operator for SplittingInterval keeps the index of the
        // left-hand-side (ait in this case)
        //  meaning that a has to be the splitter because the b index is
        //  ignored.
        out.emplace_back(*ait & *bit);
      }
    }
  }
  return out;
}

//------------------------------------------------------------------------------------------------
/** Remove any overlap in a filter (will not work properly on a splitter)
 *
 * @param a :: SplittingIntervalVec filter.
 */
SplittingIntervalVec removeFilterOverlap(const SplittingIntervalVec &a) {
  SplittingIntervalVec out;
  out.reserve(a.size());

  // Now we have to merge duplicate/overlapping intervals together
  auto it = a.cbegin();
  while (it != a.cend()) {
    // All following intervals will start at or after this one
    const DateAndTime &start = it->start();
    DateAndTime stop = it->stop();

    // Keep looking for the next interval where there is a gap (start > old
    // stop);
    while ((it != a.cend()) && (it->start() <= stop)) {
      // Extend the stop point (the start cannot be extended since the list is
      // sorted)
      if (it->stop() > stop)
        stop = it->stop();
      ++it;
    }
    // We've reached a gap point. Output this merged interval and move on.
    out.emplace_back(start, stop, 0);
  }

  return out;
}

//------------------------------------------------------------------------------------------------
/** OR operator for SplittingIntervalVec
 * Only works on Filters, not splitters. Combines the splitters
 * to only keep times where EITHER Filter is TRUE.
 *
 * @param a :: SplittingIntervalVec filter.
 * @param b :: SplittingIntervalVec filter.
 * @return the ORed filter
 */
SplittingIntervalVec operator|(const SplittingIntervalVec &a, const SplittingIntervalVec &b) {
  // Concatenate the two lists
  SplittingIntervalVec temp;
  auto isValid = [](const SplittingInterval &value) { return value.isValid(); };
  std::copy_if(a.begin(), a.end(), std::back_insert_iterator(temp), isValid);
  std::copy_if(b.begin(), b.end(), std::back_insert_iterator(temp), isValid);

  // Sort by start time rather than the default
  auto compareStart = [](const SplittingInterval &left, const SplittingInterval &right) {
    return left.start() < right.start();
  };
  std::sort(temp.begin(), temp.end(), compareStart);

  SplittingIntervalVec out = removeFilterOverlap(temp);

  return out;
}

//------------------------------------------------------------------------------------------------
/** NOT operator for SplittingIntervalVec
 * Only works on Filters. Returns a filter with the reversed
 * time intervals as the incoming filter.
 *
 * @param a :: SplittingIntervalVec filter.
 */
SplittingIntervalVec operator~(const SplittingIntervalVec &a) {
  SplittingIntervalVec out, temp;
  // First, you must remove any overlapping intervals, otherwise the output is
  // stupid.
  temp = removeFilterOverlap(a);

  // No entries: then make a "filter" that keeps everything
  if ((temp.empty())) {
    out.emplace_back(DateAndTime::minimum(), DateAndTime::maximum(), 0);
    return out;
  }

  SplittingIntervalVec::const_iterator ait;
  ait = temp.begin();
  if (ait != temp.end()) {
    // First entry; start at -infinite time
    out.emplace_back(DateAndTime::minimum(), ait->start(), 0);
    // Now start at the second entry
    while (ait != temp.end()) {
      DateAndTime start, stop;
      start = ait->stop();
      ++ait;
      if (ait == temp.end()) { // Reached the end - go to inf
        stop = DateAndTime::maximum();
      } else { // Stop at the start of the next entry
        stop = ait->start();
      }
      out.emplace_back(start, stop, 0);
    }
  }
  return out;
}

//------------------------------------------------------------------------------------------------
/** For every workspace index, create a TimeROI out of its associated splitting intervals
 * @param splitters :: vector of splitting intervals, each interval has an associated workspace index
 * @return map from workspace index to TimeROI object
 */
std::map<int, Kernel::TimeROI> timeROIsFromSplitters(const SplittingIntervalVec &splitters) {
  std::map<int, Kernel::TimeROI> roisMap;
  for (auto const &splitter : splitters) {
    // some input SplittingInterval can have same `begin` and `end` time, which is disallowed for a TimeROI object
    if (splitter.start() >= splitter.stop())
      continue;
    int destinationIndex = splitter.index();             // if existing, nonsense index -1 will also have its TimeROI
    if (roisMap.find(destinationIndex) == roisMap.end()) // first time we encounter destinationIndex
      roisMap.insert({destinationIndex, TimeROI(splitter.start(), splitter.stop())});
    else
      roisMap[destinationIndex].addROI(splitter.start(), splitter.stop());
  }
  return roisMap;
}

} // namespace Kernel
} // namespace Mantid
