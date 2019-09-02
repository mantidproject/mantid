// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_BINARYSTREAMREADER_H_
#define MANTID_KERNEL_BINARYSTREAMREADER_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

// Forward declarations
template <typename T> class Matrix;

/**
 * Assists with reading a binary file by providing standard overloads for the
 * istream operators (>>) to given types (and vectors of those types). It
 * only allows for reading fixed-width integer types to avoid cross-platform
 * differences on the sizes of various types.
 *
 *
 */
class MANTID_KERNEL_DLL BinaryStreamReader {
public:
  /// Define the ordering of 2D structures in the file
  enum class MatrixOrdering { RowMajor, ColumnMajor };

  BinaryStreamReader(std::istream &istrm);

  ///@name Single-value stream operators
  /// @{
  BinaryStreamReader &operator>>(int16_t &value);
  BinaryStreamReader &operator>>(int32_t &value);
  BinaryStreamReader &operator>>(int64_t &value);
  BinaryStreamReader &operator>>(float &value);
  BinaryStreamReader &operator>>(double &value);
  BinaryStreamReader &operator>>(std::string &value);
  BinaryStreamReader &operator>>(uint16_t &value);
  BinaryStreamReader &operator>>(uint32_t &value);
  /// @}

  ///@name 1D methods
  /// @{
  BinaryStreamReader &read(std::vector<int16_t> &value, const size_t nvals);
  BinaryStreamReader &read(std::vector<int32_t> &value, const size_t nvals);
  BinaryStreamReader &read(std::vector<int64_t> &value, const size_t nvals);
  BinaryStreamReader &read(std::vector<float> &value, const size_t nvals);
  BinaryStreamReader &read(std::vector<double> &value, const size_t nvals);
  BinaryStreamReader &read(std::string &value, const size_t length);
  /// @}

  ///@name 2D methods
  /// @{
  BinaryStreamReader &read(std::vector<std::string> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  BinaryStreamReader &read(Kernel::Matrix<float> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  BinaryStreamReader &read(Kernel::Matrix<double> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  /// @}

  /// Move the stream to nbytes past the beginning of the file
  void moveStreamToPosition(size_t nbytes);

private:
  /// Reference to the stream being read
  std::istream &m_istrm;
  /// The default size in bytes of the type used to encode the length
  /// of a string in the file. Used by operator>>(std::string&). Use largest
  /// fixed-width unsigned integer as sizeof(size_t) varies
  uint64_t m_strLengthSize;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_BINARYSTREAMREADER_H_ */
