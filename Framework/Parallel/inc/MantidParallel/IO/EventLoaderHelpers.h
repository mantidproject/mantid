// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"

#include <H5Cpp.h>
#include <vector>

namespace Mantid {
namespace Parallel {
namespace IO {

/** Private parts of EventLoader.

  @author Simon Heybrock
  @date 2017
*/
namespace EventLoader {
/// Read number of events in given banks from file.
std::vector<size_t> readBankSizes(const H5::Group &group, const std::vector<std::string> &bankNames);

H5::DataType readDataType(const H5::Group &group, const std::vector<std::string> &bankNames, const std::string &name);
} // namespace EventLoader
} // namespace IO
} // namespace Parallel
} // namespace Mantid
