//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/RegisterFileLoader.h"

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILL)

/// Constructor
LoadILL::LoadILL() : LoadILLTOF() { this->useAlgorithm("LoadILLTOF", 1); }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILL::confidence(Kernel::NexusDescriptor &descriptor) const {
  // Suppress unused warning
  (void)descriptor;
  // Deprecated, always return 0
  return 0;
}

} // namespace DataHandling
} // namespace Mantid
