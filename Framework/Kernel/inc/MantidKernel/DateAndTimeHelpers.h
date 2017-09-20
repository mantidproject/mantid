#ifndef MANTID_KERNEL_DATEANDTIMEHELPERS_H_
#define MANTID_KERNEL_DATEANDTIMEHELPERS_H_

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
namespace DateAndTimeHelpers {

MANTID_KERNEL_DLL bool stringIsISO8601(const std::string &date);
MANTID_KERNEL_DLL std::string verifyISO8601(const std::string &date,
                                            bool displayWarnings = true);

} // namespace DateAndTimeHelpers
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DATEANDTIMEHELPERS_H_ */