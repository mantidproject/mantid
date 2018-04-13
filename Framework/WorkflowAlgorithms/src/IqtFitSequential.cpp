#include "MantidWorkflowAlgorithms/IqtFitSequential.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFitSequential");

} // namespace

namespace Mantid {
namespace Algorithms {

using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IqtFitSequential)

/// Algorithms name for identification. @see Algorithm::name
const std::string IqtFitSequential::name() const {
  return "IqtFitSequential";
}

/// Algorithm's version for identification. @see Algorithm::version
int IqtFitSequential::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IqtFitSequential::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string IqtFitSequential::summary() const {
  return "Performs a sequential fit for QENS data";
}
} // namespace Algorithms
} // namespace Mantid
