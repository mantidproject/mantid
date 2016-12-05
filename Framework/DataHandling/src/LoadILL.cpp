//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/RegisterFileLoader.h"

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILL)

/// Constructor
LoadILL::LoadILL() : LoadILLTOF() {}

} // namespace DataHandling
} // namespace Mantid
