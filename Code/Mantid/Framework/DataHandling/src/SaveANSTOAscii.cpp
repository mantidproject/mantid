//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveANSTOAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveANSTOAscii)
using namespace Kernel;
using namespace API;

/** virtual method to add information to the file before the data
 *  however this class doesn't have any but must implement it.
 *  @param file :: pointer to output file stream
 */
void SaveANSTOAscii::extraHeaders(std::ofstream &file) { UNUSED_ARG(file); }
} // namespace DataHandling
} // namespace Mantid
