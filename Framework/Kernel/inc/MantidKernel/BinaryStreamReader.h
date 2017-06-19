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
  * Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  * National Laboratory & European Spallation Source
  *
  * This file is part of Mantid.
  *
  * Mantid is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 3 of the License, or
  * (at your option) any later version.
  *
  * Mantid is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * File change history is stored at: <https://github.com/mantidproject/mantid>
  * Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class MANTID_KERNEL_DLL BinaryStreamReader {
public:
  /// Define the ordering of 2D structures in the file
  enum class MatrixOrdering { RowMajor, ColumnMajor };

  BinaryStreamReader(std::istream &istrm);

  ///@name Single-value stream operators
  /// @{
  BinaryStreamReader &operator>>(int32_t &value);
  BinaryStreamReader &operator>>(int64_t &value);
  BinaryStreamReader &operator>>(float &value);
  BinaryStreamReader &operator>>(double &value);
  BinaryStreamReader &operator>>(std::string &value);
  /// @}

  ///@name 1D methods
  /// @{
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
