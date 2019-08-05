// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_STORAGEMODE_H_
#define MANTID_PARALLEL_STORAGEMODE_H_

#include "MantidParallel/DllConfig.h"

#include <map>
#include <stdexcept>
#include <string>

namespace Mantid {
namespace Parallel {

/** Storage mode used for a Workspace in an MPI build.

  Cloned: There is a copy (clone) of the Workspace on each rank.
  Distributed: Each rank holds parts of the Workspace (spectra).
  MasterOnly: The master/root rank has the Workspace.

  @author Simon Heybrock
  @date 2017
*/
enum class StorageMode { Cloned, Distributed, MasterOnly };

MANTID_PARALLEL_DLL std::string toString(StorageMode mode);
MANTID_PARALLEL_DLL std::string
toString(const std::map<std::string, StorageMode> &map);
MANTID_PARALLEL_DLL StorageMode fromString(const std::string &mode);

} // namespace Parallel
} // namespace Mantid

#endif // MANTID_PARALLEL_STORAGEMODE_H_
