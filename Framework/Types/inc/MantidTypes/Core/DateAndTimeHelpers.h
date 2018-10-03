// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TYPES_CORE_DATEANDTIMEHELPERS_H_
#define MANTID_TYPES_CORE_DATEANDTIMEHELPERS_H_

#include "MantidTypes/DllConfig.h"

#include <string>

namespace Mantid {
namespace Types {
namespace Core {
namespace DateAndTimeHelpers {
MANTID_TYPES_DLL bool stringIsISO8601(const std::string &date);
MANTID_TYPES_DLL bool stringIsPosix(const std::string &date);
} // namespace DateAndTimeHelpers
} // namespace Core
} // namespace Types
} // namespace Mantid

#endif /* MANTID_TYPES_CORE_DATEANDTIMEHELPERS_H_ */
