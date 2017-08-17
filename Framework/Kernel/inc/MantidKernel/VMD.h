#ifndef MANTID_KERNEL_VMD_H_
#define MANTID_KERNEL_VMD_H_

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;

/** Simple vector class for multiple dimensions (i.e. > 3).

  @author Janik Zikovsky
  @date 2011-08-30

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename TYPE = double> class DLLExport VMDBase {
public:
  VMDBase();
  VMDBase(size_t nd);
  VMDBase(double val0, double val1);
  VMDBase(double val0, double val1, double val2);
  VMDBase(double val0, double val1, double val2, double val3);
  VMDBase(double val0, double val1, double val2, double val3, double val4);
  VMDBase(double val0, double val1, double val2, double val3, double val4,
          double val5);

  VMDBase(const VMDBase &other);
  VMDBase &operator=(const VMDBase &other);
  VMDBase(VMDBase &&other) noexcept;
  VMDBase &operator=(VMDBase &&other) noexcept;
  VMDBase(size_t nd, const double *bareData);
  VMDBase(size_t nd, const float *bareData);
  VMDBase(const V3D &vector);
  VMDBase(const std::vector<double> &vector);
  VMDBase(const std::vector<float> &vector);
  VMDBase(const std::string &str);
  ~VMDBase();

  size_t getNumDims() const;
  size_t size() const;
  const TYPE &operator[](const size_t index) const;
  TYPE &operator[](const size_t index);
  const TYPE *getBareArray() const;
  std::string toString(const std::string &separator = " ") const;
  template <class T> std::vector<T> toVector() const;
  bool operator==(const VMDBase &v) const;
  bool operator!=(const VMDBase &v) const;
  VMDBase operator+(const VMDBase &v) const;
  VMDBase &operator+=(const VMDBase &v);
  VMDBase operator-(const VMDBase &v) const;
  VMDBase &operator-=(const VMDBase &v);
  VMDBase operator*(const VMDBase &v) const;
  VMDBase &operator*=(const VMDBase &v);
  VMDBase operator/(const VMDBase &v) const;
  VMDBase &operator/=(const VMDBase &v);
  VMDBase operator*(const double scalar) const;
  VMDBase &operator*=(const double scalar);
  VMDBase operator/(const double scalar) const;
  VMDBase &operator/=(const double scalar);
  TYPE scalar_prod(const VMDBase &v) const;
  VMDBase cross_prod(const VMDBase &v) const;
  TYPE length() const;
  TYPE norm() const;
  TYPE norm2() const;
  TYPE normalize();
  TYPE angle(const VMDBase &v) const;

  static std::vector<VMDBase>
  makeVectorsOrthogonal(std::vector<VMDBase> &vectors);
  static VMDBase getNormalVector(const std::vector<VMDBase> &vectors);

protected:
  /// Number of dimensions
  size_t nd;
  /// Data, an array of size nd
  TYPE *data;
};

/// Underlying data type for the VMD type
typedef float VMD_t;

/// Define the VMD as using the double or float data type.
typedef VMDBase<VMD_t> VMD;

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const VMDBase<double> &);
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const VMDBase<float> &);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_VMD_H_ */
