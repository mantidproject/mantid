#ifndef MANTID_MPI_TYPES_H_
#define MANTID_MPI_TYPES_H_

#include <string>
#include <map>

namespace Mantid {
namespace MPI {

/** Storage mode used for a Workspace in an MPI build.
 *
 * Cloned: There is a copy (clone) of the Workspace on each rank.
 * Distributed: Each rank holds parts of the Workspace (spectra).
 * MasterOnly: The master/root rank has the Workspace */
enum class StorageMode { Cloned, Distributed, MasterOnly };

/** Execution mode used for an Algorithm in an MPI build.
 *
 * Invalid: Indicates a state where execution is not possible.
 * Serial: Serial execution (non-MPI build MPI build with single rank).
 * Identical: Independent execution in the same way on each rank.
 * Distributed: Distributed execution, may involve communication.
 * MasterOnly: Execution only on the master rank. */
enum class ExecutionMode {
  Invalid,
  Serial,
  Identical,
  Distributed,
  MasterOnly
};

/// Returns a human-readable string representation of a StorageMode.
std::string toString(StorageMode mode);
/// Returns a human-readable string representation of an ExecutionMode.
std::string toString(ExecutionMode mode);
/// Returns a human-readable string representation of a StorageMode map.
std::string toString(const std::map<std::string, StorageMode> &map);

} // namespace MPI
} // namespace Mantid

#endif /* MANTID_MPI_TYPES_H_*/
