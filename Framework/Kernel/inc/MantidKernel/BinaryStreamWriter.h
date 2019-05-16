// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_BINARYSTREAMWRITER_H_
#define MANTID_KERNEL_BINARYSTREAMWRITER_H_
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
 * Assists with writing a binary file by providing standard overloads for the
 * ostream operators (<<) to given types (and vectors of those types). It
 * only allows for writing fixed-width integer types to avoid cross-platform
 * differences on the sizes of various types.
 *
 *
 */
class MANTID_KERNEL_DLL BinaryStreamWriter {
public:
  /// Define the ordering of 2D structures in the file
  enum class MatrixOrdering { RowMajor, ColumnMajor };

  BinaryStreamWriter(std::ostream &ofstrm);

  ///@name Single-value stream operators
  /// @{
  BinaryStreamWriter &operator<<(const int16_t &value);
  BinaryStreamWriter &operator<<(const int32_t &value);
  BinaryStreamWriter &operator<<(const int64_t &value);
  BinaryStreamWriter &operator<<(const float &value);
  BinaryStreamWriter &operator<<(const double &value);
  BinaryStreamWriter &operator<<(const std::string &value);
  BinaryStreamWriter &operator<<(const uint16_t &value);
  BinaryStreamWriter &operator<<(const uint32_t &value);
  /// @}

  ///@name 1D methods
  /// @{
  BinaryStreamWriter &write(const std::vector<int16_t> &value, const size_t nvals);
  BinaryStreamWriter &write(const std::vector<int32_t> &value, const size_t nvals);
  BinaryStreamWriter &write(const std::vector<int64_t> &value, const size_t nvals);
  BinaryStreamWriter &write(const std::vector<float> &value, const size_t nvals);
  BinaryStreamWriter &write(const std::vector<double> &value, const size_t nvals);
  BinaryStreamWriter &write(const std::string &value, const size_t length);
  /// @}

  ///@name 2D methods
  /// @{
  BinaryStreamWriter &write(const std::vector<std::string> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  BinaryStreamWriter &write(const Kernel::Matrix<float> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  BinaryStreamWriter &write(const Kernel::Matrix<double> &value,
                           const std::vector<int32_t> &shape,
                           MatrixOrdering order);
  /// @}



private:
  /// Reference to the stream being written to
  std::ostream &m_ofstrm;
  /// The default size in bytes of the type used to encode the length
  /// of a string in the file. Used by operator<<(std::string&). Use largest
  /// fixed-width unsigned integer as sizeof(size_t) varies
  uint64_t m_strLengthSize;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_BINARYSTREAMWRITER_H_ */
