// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;

/** Simple vector class for multiple dimensions (i.e. > 3).

  @author Janik Zikovsky
  @date 2011-08-30
*/
template <typename TYPE = double> class MANTID_KERNEL_DLL VMDBase {
public:
  VMDBase();
  VMDBase(size_t nd);
  VMDBase(double val0, double val1);
  VMDBase(double val0, double val1, double val2);
  VMDBase(double val0, double val1, double val2, double val3);
  VMDBase(double val0, double val1, double val2, double val3, double val4);
  VMDBase(double val0, double val1, double val2, double val3, double val4, double val5);

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

  static std::vector<VMDBase> makeVectorsOrthogonal(std::vector<VMDBase> &vectors);
  static VMDBase getNormalVector(const std::vector<VMDBase> &vectors);

protected:
  /// Number of dimensions
  size_t nd;
  /// Data, an array of size nd
  TYPE *data;
};

/// Underlying data type for the VMD type
using VMD_t = float;

/// Define the VMD as using the double or float data type.
using VMD = VMDBase<VMD_t>;

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const VMDBase<double> &);
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const VMDBase<float> &);

} // namespace Kernel
} // namespace Mantid
