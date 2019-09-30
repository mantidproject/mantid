// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/Matrix.h"

#include <cassert>
#include <istream>
#include <stdexcept>

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------
// Anonymous functions
//------------------------------------------------------------------------------
namespace {

/**
 * Read a value from the stream based on the template type
 * @param stream The open stream on which to perform the read
 * @param value An object of type T to fill with the value from the file
 */
template <typename T>
inline void readFromStream(std::istream &stream, T &value) {
  stream.read(reinterpret_cast<char *>(&value), sizeof(T));
}

/**
 * Overload to read an array of values from the stream based on the template
 * type for the element of the array.
 * @param stream The open stream on which to perform the read
 * @param value An object of type std::vector<T> to fill with the value from
 * the file
 * @param nvals The number of values to read
 */
template <typename T>
inline void readFromStream(std::istream &stream, std::vector<T> &value,
                           size_t nvals) {
  if (value.size() < nvals)
    value.resize(nvals);
  stream.read(reinterpret_cast<char *>(value.data()), nvals * sizeof(T));
}

/**
 * Read a stream of numbers and interpret them as a 2D matrix of specified
 * shape & order
 * @param stream The open stream on which to perform the read
 * @param value The Matrix to fill. Its size is increased if necessary
 * @param shape 2D-vector defined as (nrows, ncols)
 * @param order Defines whether the stream of bytes is interpreted as moving
 * across the rows first (RowMajor) or down columns first (ColumnMajor)
 */
template <typename T>
inline void readFromStream(std::istream &stream, Matrix<T> &value,
                           const std::vector<int32_t> &shape,
                           BinaryStreamReader::MatrixOrdering order) {
  assert(2 <= shape.size());
  const size_t s0(shape[0]), s1(shape[1]), totalLength(s0 * s1);
  std::vector<T> buffer;
  readFromStream(stream, buffer, totalLength);
  value = Matrix<T>(s0, s1);
  if (order == BinaryStreamReader::MatrixOrdering::RowMajor) {
    for (size_t i = 0; i < s0; ++i) {
      auto row = value[i];
      const size_t offset = i * s1;
      for (size_t j = 0; j < s1; ++j) {
        row[j] = buffer[offset + j];
      }
    }
  } else {
    for (size_t i = 0; i < s0; ++i) {
      auto row = value[i];
      for (size_t j = 0; j < s1; ++j) {
        row[j] = buffer[j * s0 + i];
      }
    }
  }
}
} // namespace

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
 * Read a int16_t from the stream
 * @param value the value is stored in the given stream
 * @return BinaryStreamReader&
 */
BinaryStreamReader &BinaryStreamReader::operator>>(int16_t &value) {
  readFromStream(m_istrm, value);
  return *this;
}

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

/**
 * Read a uint16_t from the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(uint16_t &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read a uint32_t from the stream
 * @param value The value is stored in the given stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::operator>>(uint32_t &value) {
  readFromStream(m_istrm, value);
  return *this;
}

/**
 * Read an array of int16_t into the given vector.
 * @param value The array to fille. Its size is increased if necessary
 * @param nvals The number values to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::vector<int16_t> &value,
                                             const size_t nvals) {
  readFromStream(m_istrm, value, nvals);
  return *this;
}

/**
 * Read an array of int32_t into the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::vector<int32_t> &value,
                                             const size_t nvals) {
  readFromStream(m_istrm, value, nvals);
  return *this;
}

/**
 * Read an array of int64_t into the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::vector<int64_t> &value,
                                             const size_t nvals) {
  readFromStream(m_istrm, value, nvals);
  return *this;
}

/**
 * Read an array of float balues into the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::vector<float> &value,
                                             const size_t nvals) {
  readFromStream(m_istrm, value, nvals);
  return *this;
}

/**
 * Read an array of double values into the given vector.
 * @param value The array to fill. Its size is increased if necessary
 * @param nvals The number values to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::vector<double> &value,
                                             const size_t nvals) {
  readFromStream(m_istrm, value, nvals);
  return *this;
}

/**
 * Read a series of characters into a string object.
 * @param value The string to fill. Its size is increased if necessary
 * @param length The number characters to attempt to read from the stream
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &BinaryStreamReader::read(std::string &value,
                                             const size_t length) {
  value.resize(length);
  m_istrm.read(const_cast<char *>(value.data()), length);
  return *this;
}

/**
 * Read a stream of characters and interpret them as a 2D matrix of specified
 * shape & order
 * @param value The array to fill. Its size is increased if necessary
 * @param shape 2D-vector defined as (nstrs, strLength)
 * @param order Defines whether the stream of bytes is interpreted as moving
 * across the rows first (RowMajor) or down columns first (ColumnMajor)
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &
BinaryStreamReader::read(std::vector<std::string> &value,
                         const std::vector<int32_t> &shape,
                         BinaryStreamReader::MatrixOrdering order) {
  assert(2 <= shape.size());

  const size_t s0(shape[0]), s1(shape[1]), totalLength(s0 * s1);
  std::string buffer;
  this->read(buffer, totalLength);
  value.resize(s0);
  if (order == MatrixOrdering::RowMajor) {
    size_t pos(0);
    for (auto &str : value) {
      str = buffer.substr(pos, s1);
      pos += s1;
    }
  } else {
    for (size_t i = 0; i < s0; ++i) {
      auto &str = value[i];
      str.resize(s1);
      for (size_t j = 0; j < s1; ++j) {
        str[j] = buffer[j * s0 + i];
      }
    }
  }
  return *this;
}

/**
 * Read a stream of floats and interpret them as a 2D matrix of specified
 * shape & order
 * @param value The Matrix to fill. Its size is increased if necessary
 * @param shape 2D-vector defined as (nrows, ncols)
 * @param order Defines whether the stream of bytes is interpreted as moving
 * across the rows first (RowMajor) or down columns first (ColumnMajor)
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &
BinaryStreamReader::read(Kernel::Matrix<float> &value,
                         const std::vector<int32_t> &shape,
                         BinaryStreamReader::MatrixOrdering order) {
  readFromStream(m_istrm, value, shape, order);
  return *this;
}

/**
 * Read a stream of doubles and interpret them as a 2D matrix of specified
 * shape & order
 * @param value The Matrix to fill. Its size is increased if necessary
 * @param shape 2D-vector defined as (nrows, ncols)
 * @param order Defines whether the stream of bytes is interpreted as moving
 * across the rows first (RowMajor) or down columns first (ColumnMajor)
 * @return A reference to the BinaryStreamReader object
 */
BinaryStreamReader &
BinaryStreamReader::read(Kernel::Matrix<double> &value,
                         const std::vector<int32_t> &shape,
                         BinaryStreamReader::MatrixOrdering order) {
  readFromStream(m_istrm, value, shape, order);
  return *this;
}

/**
 * Will move the stream to the given position
 * @param nbytes The number of bytes from position 0 to move
 */
void BinaryStreamReader::moveStreamToPosition(size_t nbytes) {
  m_istrm.seekg(nbytes, std::ios_base::beg);
}

} // namespace Kernel
} // namespace Mantid
