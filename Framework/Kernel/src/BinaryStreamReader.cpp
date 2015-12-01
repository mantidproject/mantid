//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/Exception.h"

#include <istream>

//------------------------------------------------------------------------------
// Anonymous functions
//------------------------------------------------------------------------------
namespace {

/**
  * Read a value from the stream based on the template type
  * @param stream The open stream on which to perform the read
  * @param value An object of type T to fill with the value from the file
  */
  template<typename T>
  inline void readFromStream(std::istream & stream, T &value) {
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
  }
  
}

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------
// Public members
//------------------------------------------------------------------------------

/**
 * Constructor taking the stream to read.
 * @param istrm An open stream from which data will be read. The object does
 * not take ownership of the stream. The caller is responsible for closing
 * it.
 */
BinaryStreamReader::BinaryStreamReader(std::istream &istrm)
    : m_istrm(istrm), m_strLengthSize(static_cast<uint64_t>(sizeof(int32_t))) {
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

/**
 * Read a int32_t from the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(int32_t &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read a int64_t from the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(int64_t &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read a float (4-bytes) from the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(float &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read a double (8-bytes) from the stream
 * @param value The value is stored in this object
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(double &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read a string of characters into given object. This method assumes that
 * the stream currently points at a type specifying the length followed directly
 * by the string itself.
 * @param value The string value is stored in the given object. It is resized
 * to the appropriate length
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(std::string &value) {
  // First read the size.
  decltype(m_strLengthSize) length(0);
  m_istrm.read(reinterpret_cast<char *>(&length), m_strLengthSize);
  // Now the value
  value.resize(static_cast<std::string::size_type>(length));
  m_istrm.read(const_cast<char *>(value.data()), static_cast<size_t>(length));
  return *this;
}

} // namespace Kernel
} // namespace Mantid
