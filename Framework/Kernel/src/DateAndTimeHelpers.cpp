// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/Logger.h"
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <numeric>
#include <stdexcept>

namespace {
std::tuple<bool, size_t, std::string> isARGUSDateTime(const std::string &date) {
  // Some ARGUS files have an invalid date with a space instead of zero.
  // To enable such files to be loaded we correct the date and issue a warning
  // Normal ISO8601 Date "2009-07-08T10:23:50"
  // Possible Argus Format "2009-07- 8T10:23:50"

  // (ticket #4017).
  // just take the date not the time or any date-time separator
  std::string strippedDate = date.substr(0, 10);
  const size_t nSpace = strippedDate.find(' ');
  return std::make_tuple(nSpace != std::string::npos, nSpace, strippedDate);
}
} // namespace

namespace Mantid {
namespace Kernel {
namespace DateAndTimeHelpers {
// Initialize the logger
Logger g_log("DateAndTime");

/** Creates a DateAndTime object from a date string even if the string does not
 *exactly conform to ISO8601 (ARGUS File)
 *@param date Date used to create DateAndTime object. May be sanitized first.
 *
 */
Types::Core::DateAndTime createFromSanitizedISO8601(const std::string &date) {
  return Types::Core::DateAndTime(verifyAndSanitizeISO8601(date));
}

/** Verifies whether or not a string conforms to ISO8601. Corrects the string
 *if it does not and is of the ARGUS file date/time format.
 *e.g 2009-07- 8T10:23:50 becomes 2009-07-08T10:23:50.
 *
 *@param date Date to be checked/corrected
 *@param displayWarnings display warning messages in the log if the date is non
 *conforming.
 */
std::string verifyAndSanitizeISO8601(const std::string &date,
                                     bool displayWarnings) {
  auto res = isARGUSDateTime(date);

  if (std::get<0>(res)) {
    auto time = date;
    if (displayWarnings) {
      g_log.warning() << "Invalid ISO8601 date " << date;
    }

    auto nSpace = std::get<1>(res);

    time[nSpace] = '0'; // replace space with 0

    // Do again in case of second space
    auto strippedDate = std::get<2>(res);
    strippedDate[nSpace] = '0';

    const size_t nSecondSpace = strippedDate.find(' ');
    if (nSecondSpace != std::string::npos)
      time[nSecondSpace] = '0';
    if (displayWarnings) {
      g_log.warning() << " corrected to " << time << '\n';
    }

    return time;
  }

  return date;
}

/**
 * @brief averageSorted Assuming that the vector is sorted, find the average
 * time
 */
Types::Core::DateAndTime
averageSorted(const std::vector<Types::Core::DateAndTime> &times) {
  if (times.empty())
    throw std::invalid_argument("Cannot find average of empty vector");

  // to avoid overflow subtract the first time off from everything
  // and find the average in between
  const int64_t first = times.begin()->totalNanoseconds();
  int64_t total =
      std::accumulate(times.begin(), times.end(), int64_t{0},
                      [first](int64_t a, const Types::Core::DateAndTime time) {
                        return std::move(a) + (time.totalNanoseconds() - first);
                      });

  double avg = static_cast<double>(total) / static_cast<double>(times.size());

  return times.front() + static_cast<int64_t>(avg);
}

Types::Core::DateAndTime
averageSorted(const std::vector<Types::Core::DateAndTime> &times,
              const std::vector<double> &weights) {
  if (times.empty())
    throw std::invalid_argument("Cannot find average of empty vector");
  if (times.size() != weights.size())
    throw std::invalid_argument(
        "time and weight vectors must be the same length");
  if (times.size() == 1)
    return times.front();

  double totalWeight = std::accumulate(weights.begin(), weights.end(), 0.);

  // to avoid overflow subtract the first time off from everything
  // and find the average in between
  const int64_t first = times.begin()->totalNanoseconds();

  // second operation is done to each parallel vector element
  // first operation accumulates the result
  double totalValue = std::inner_product(
      times.begin(), times.end(), weights.begin(), double{0.}, std::plus<>(),
      [first](const Types::Core::DateAndTime time, const double weight) {
        return static_cast<double>(time.totalNanoseconds() - first) * weight;
      });

  // add the result to the original first value
  return times.front() + static_cast<int64_t>(totalValue / totalWeight);
}

} // namespace DateAndTimeHelpers
} // namespace Kernel
} // namespace Mantid
