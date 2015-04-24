//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DateValidator.h"
#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Kernel {

namespace {
//
// Keeps time.h out of the header
//
/** Checks the validity of date string ,expected format is "DD/MM/YYYY"
 *  @param sDate :: expected date format
 *  @param error ::  string which describes the error
 *  @return  tm structure with the date information filled in
 */
struct tm getTimeValue(const std::string &sDate, std::string &error) {
  struct tm timeinfo;
  // Zero it out
  timeinfo.tm_sec = 0;
  timeinfo.tm_min = 0;
  timeinfo.tm_hour = 0;
  timeinfo.tm_mday = 0;
  timeinfo.tm_mon = 0;
  timeinfo.tm_year = 0;
  timeinfo.tm_wday = 0;
  timeinfo.tm_isdst = -1;

  std::basic_string<char>::size_type index, off = 0;
  int day, month, year;

  // look for the first '/' to extract day part from the date string
  index = sDate.find('/', off);
  if (index == std::string::npos) {
    error = "Invalid Date:date format must be DD/MM/YYYY";
    timeinfo.tm_mday = 0;
    return timeinfo;
  }
  // get day part of the date
  try {
    day = boost::lexical_cast<int>(sDate.substr(off, index - off).c_str());
  } catch (boost::bad_lexical_cast &) {
    error = "Invalid Date";
    return timeinfo;
  }
  timeinfo.tm_mday = day;

  // change the offset to the next position after "/"
  off = index + 1;
  // look for 2nd '/' to get month part from  the date string
  index = sDate.find('/', off);
  if (index == std::string::npos) {
    error = "Invalid Date:date format must be DD/MM/YYYY";
    return timeinfo;
  }
  // now get the month part
  try {
    month = boost::lexical_cast<int>(sDate.substr(off, index - off).c_str());

  } catch (boost::bad_lexical_cast &) {
    error = "Invalid Date";
    return timeinfo;
  }
  timeinfo.tm_mon = month - 1;
  // change the offset to the position after "/"
  off = index + 1;
  // now get the year part from the date string
  try {
    year = boost::lexical_cast<int>(sDate.substr(off, 4).c_str());

  } catch (boost::bad_lexical_cast &) {
    error = "Invalid Date";
    return timeinfo;
  }

  timeinfo.tm_year = year - 1900;

  return timeinfo;
}
}

/// constructor
DateValidator::DateValidator() {}

/// destructor
DateValidator::~DateValidator() {}

/// create a copy of the the validator
IValidator_sptr DateValidator::clone() const {
  return boost::make_shared<DateValidator>(*this);
}

/** Checks the given value is a valid date
 *  @param value :: input date property to validate
 *  @return a string which describes the error else ""
 */
std::string DateValidator::checkValidity(const std::string &value) const {
  // empty strings are allowed
  if (value.empty()) {
    return "";
  }
  std::string formaterror;
  struct tm timeinfo = getTimeValue(value, formaterror);
  if (!formaterror.empty()) {
    return formaterror;
  }

  if (timeinfo.tm_mday < 1 || timeinfo.tm_mday > 31) {
    return "Invalid Date:Day part of the Date parameter must be between 1 and "
           "31";
  }
  if (timeinfo.tm_mon < 0 || timeinfo.tm_mon > 11) {
    return "Invalid Date:Month part of the Date parameter must be between 1 "
           "and 12";
  }
  // get current time
  time_t rawtime;
  time(&rawtime);
  struct tm *currenttime = localtime(&rawtime);
  if (timeinfo.tm_year > currenttime->tm_year) {
    return "Invalid Date:Year part of the Date parameter can not be greater "
           "than the current year";
  }
  return "";
}
}
}
