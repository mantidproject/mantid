// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATRIXPROPERTY_H_
#define MANTID_KERNEL_MATRIXPROPERTY_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Matrix.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace Kernel {

template <class TYPE = double>
class MatrixProperty : public PropertyWithValue<Matrix<TYPE>> {
  /// Typedef the held type
  using HeldType = Kernel::Matrix<TYPE>;

public:
  /// Constructor
  MatrixProperty(const std::string &propName,
                 IValidator_sptr validator = IValidator_sptr(new NullValidator),
                 unsigned int direction = Direction::Input);
  /// Copy constructor
  MatrixProperty(const MatrixProperty &rhs);
  // Unhide base class members (at minimum, avoids Intel compiler warning)
  using PropertyWithValue<HeldType>::operator=;
  /// 'Virtual copy constructor'
  inline MatrixProperty *clone() const override {
    return new MatrixProperty(*this);
  }
  /// Destructor
  ~MatrixProperty() override;

  /// Add the value of another property. Doesn't make sense here.
  MatrixProperty &operator+=(Kernel::Property const *) override {
    throw Exception::NotImplementedError(
        "+= operator is not implemented for MatrixProperty.");
    return *this;
  }

private:
  /// Default constructor
  MatrixProperty();
};
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_MATRIXPROPERTY_H_
