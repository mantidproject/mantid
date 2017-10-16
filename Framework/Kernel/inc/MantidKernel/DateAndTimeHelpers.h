#ifndef MANTID_KERNEL_DATEANDTIMEHELPERS_H_
#define MANTID_KERNEL_DATEANDTIMEHELPERS_H_

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
namespace DateAndTimeHelpers {

MANTID_KERNEL_DLL DateAndTime
createFromSanitizedISO8601(const std::string &date);

MANTID_KERNEL_DLL bool stringIsISO8601(const std::string &date);

MANTID_KERNEL_DLL std::string
verifyAndSanitizeISO8601(const std::string &date, bool displayWarnings = true);

} // namespace DateAndTimeHelpers
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DATEANDTIMEHELPERS_H_ */