//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/Exception.h"

#include <istream>

namespace Mantid {
namespace Kernel {

/**
 * Constructor taking the stream to read.
 * @param istrm An open stream from which data will be read. The object does
 * not take ownership of the stream. The caller is responsible for closing
 * it.
 */
BinaryStreamReader::BinaryStreamReader(std::istream &istrm) : m_istrm(istrm) {
  if (!istrm) {
    throw std::runtime_error(
        "BinaryStreamReader: Input stream is in a bad state. Cannot continue.");
  }
}

/**
 * Destructor
 * The stream state is left as it was in the last call to a read operation.
 * It is up to the caller to close it.
 */
BinaryStreamReader::~BinaryStreamReader() {}

} // namespace Kernel
} // namespace Mantid
