// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/BinaryStreamWriter.h"
#include "MantidKernel/Matrix.h"

#include <cassert>
#include <fstream>
#include <stdexcept>

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------
// Anonymous functions
//------------------------------------------------------------------------------
namespace {

/**
 * write a value to the stream based on the template type
 * @param stream The open stream on which to perform the write
 * @param value An object of type T to fill with the value from the file
 */
template <typename T>
inline void writeToStream(std::ostream &stream, const T &value) {
  stream.write(reinterpret_cast<const char *>(&value), sizeof(T));
}

/**
 * Overload to write an array of values to the stream based on the template
 * type for the element of the array.
 * @param stream The open stream on which to perform the write
 * @param value An object of type std::vector<T> to fill with the value from
 * the file
 * @param nvals The number of values to write
 */
template <typename T>
inline void writeToStream(std::ostream &stream, const std::vector<T> &value,
                          size_t nvals) {
  stream.write(reinterpret_cast<const char *>(value.data()), nvals * sizeof(T));
}
} // namespace

//------------------------------------------------------------------------------
// Public members
//------------------------------------------------------------------------------

/**
 * Constructor taking the stream to write.
 * @param ofstrm An open stream from which data will be write. The object does
 * not take ownership of the stream. The caller is responsible for closing
 * it.
 */
BinaryStreamWriter::BinaryStreamWriter(std::ostream &ofstrm)
    : m_ofstrm(ofstrm),
      m_strLengthSize(static_cast<uint64_t>(sizeof(int32_t))) {
  if (!ofstrm) {
    throw std::runtime_error("BinaryStreamWriter: Output stream is in a bad "
                             "state. Cannot continue.");
  }
}

/**
 * write a int16_t to the stream
 * @param value the value is stored in the given stream
 * @return BinaryStreamWriter&
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const int16_t &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a int32_t to the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const int32_t &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a int64_t to the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const int64_t &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a float (4-bytes) to the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const float &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a double (8-bytes) to the stream
 * @param value The value is stored in this object
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const double &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a string of characters from given object. This method assumes that
 * the stream currently points at a type specifying the length followed directly
 * by the string itself.
 * @param value The string value is stored in the given object. It is resized
 * to the appropriate length
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const std::string &value) {
  // First write the size.
  decltype(m_strLengthSize) length(value.length());
  m_ofstrm.write(reinterpret_cast<char *>(&length), m_strLengthSize);
  // Now the value
  m_ofstrm.write(const_cast<char *>(value.data()), static_cast<size_t>(length));
  return *this;
}

/**
 * write a uint16_t to the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const uint16_t &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write a uint32_t to the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::operator<<(const uint32_t &value) {
  writeToStream(m_ofstrm, value);
  return *this;
}

/**
 * write an array of int16_t from the given vector.
 * @param value The array to fille. Its size is increased if necessary
 * @param nvals The number values to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::vector<int16_t> &value,
                                              const size_t nvals) {
  writeToStream(m_ofstrm, value, nvals);
  return *this;
}

/**
 * write an array of int32_t from the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::vector<int32_t> &value,
                                              const size_t nvals) {
  writeToStream(m_ofstrm, value, nvals);
  return *this;
}

/**
 * write an array of int64_t from the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::vector<int64_t> &value,
                                              const size_t nvals) {
  writeToStream(m_ofstrm, value, nvals);
  return *this;
}

/**
 * write an array of float balues from the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::vector<float> &value,
                                              const size_t nvals) {
  writeToStream(m_ofstrm, value, nvals);
  return *this;
}

/**
 * write an array of double values from the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::vector<double> &value,
                                              const size_t nvals) {
  writeToStream(m_ofstrm, value, nvals);
  return *this;
}

/**
 * write a series of characters from a string object.
 * @param value The string to fill. Its size is increased if necessary
 * @param length The number characters to attempt to write to the stream
 * @return A reference to the BinaryStreamWriter object
 */
BinaryStreamWriter &BinaryStreamWriter::write(const std::string &value,
                                              const size_t length) {
  m_ofstrm.write(const_cast<char *>(value.data()), length);
  return *this;
}

} // namespace Kernel
} // namespace Mantid
