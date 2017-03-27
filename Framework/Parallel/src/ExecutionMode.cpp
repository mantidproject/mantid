#include "MantidParallel/ExecutionMode.h"

namespace Mantid {
namespace Parallel {

/// Returns a human-readable string representation of an ExecutionMode.
std::string toString(ExecutionMode mode) {
  switch (mode) {
  case ExecutionMode::Invalid:
    return "Parallel::ExecutionMode::Invalid";
  case ExecutionMode::Serial:
    return "Parallel::ExecutionMode::Serial";
  case ExecutionMode::Identical:
    return "Parallel::ExecutionMode::Identical";
  case ExecutionMode::Distributed:
    return "Parallel::ExecutionMode::Distributed";
  case ExecutionMode::MasterOnly:
    return "Parallel::ExecutionMode::MasterOnly";
  default:
    return "Parallel::ExecutionMode::<undefined>";
  }
}

} // namespace Parallel
} // namespace Mantid
