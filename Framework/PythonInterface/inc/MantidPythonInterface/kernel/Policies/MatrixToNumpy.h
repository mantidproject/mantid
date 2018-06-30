#ifndef MANTID_PYTHONINTERFACE_MATRIXTONUMPY_H_
#define MANTID_PYTHONINTERFACE_MATRIXTONUMPY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidPythonInterface/kernel/Converters/CloneToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/PyArrayType.h"

#include <type_traits>

#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Policies {

namespace // anonymous
{
//-----------------------------------------------------------------------
// MPL helper structs
//-----------------------------------------------------------------------
/// MPL struct to figure out if a type is a std::vector
/// The general one inherits from boost::false_type
template <typename T> struct is_matrix : boost::false_type {};

/// Specialization for std::vector types to inherit from
/// boost::true_type
template <typename T> struct is_matrix<Kernel::Matrix<T>> : boost::true_type {};

//-----------------------------------------------------------------------
// MatrixRefToNumpyImpl - Policy for reference returns
//-----------------------------------------------------------------------
/**
 * Helper struct that implements the conversion
 * policy.
 */
template <typename MatrixType, typename ConversionPolicy>
struct MatrixRefToNumpyImpl {
  inline PyObject *operator()(const MatrixType &cmatrix) const {
    return Converters::MatrixToNDArray<typename MatrixType::value_type,
                                       ConversionPolicy>()(cmatrix);
  }

  inline PyTypeObject const *get_pytype() const {
    return Converters::getNDArrayType();
  }
};

template <typename T>
struct MatrixRefToNumpy_Requires_Reference_To_Matrix_Return_Type {};
} // namespace

/**
 * Implements a return value policy that
 * returns a numpy array from a Matrix
 *
 * The type of conversion is specified by a policy:
 * (1) WrapReadOnly - Creates a read-only array around the original data (no
 *copy is performed)
 * (2) WrapReadWrite - Creates a read-write array around the original data (no
 *copy is performed)
 */
template <typename ConversionPolicy> struct MatrixRefToNumpy {
  // The boost::python framework calls return_value_policy::apply<T>::type
  template <class T> struct apply {
    // Typedef that removes and const or reference qualifiers from the return
    // type
    using non_const_type = typename std::remove_const<
        typename std::remove_reference<T>::type>::type;
    // MPL compile-time check that T is a reference to a Kernel::Matrix
    using type = typename boost::mpl::if_c<
        boost::mpl::and_<std::is_reference<T>,
                         is_matrix<non_const_type>>::value,
        MatrixRefToNumpyImpl<non_const_type, ConversionPolicy>,
        MatrixRefToNumpy_Requires_Reference_To_Matrix_Return_Type<T>>::type;
  };
};

//-----------------------------------------------------------------------
// MatrixToNumpy return_value_policy
//-----------------------------------------------------------------------
namespace {
/**
 * Helper struct that implements the conversion policy. This can only clone
 * as wrapping would wrap a temporary
 */
template <typename MatrixType> struct MatrixToNumpyImpl {
  inline PyObject *operator()(const MatrixType &cvector) const {
    return Converters::MatrixToNDArray<typename MatrixType::value_type,
                                       Converters::Clone>()(cvector);
  }

  inline PyTypeObject const *get_pytype() const {
    return Converters::getNDArrayType();
  }
};

template <typename T> struct MatrixToNumpy_Requires_Matrix_Return_By_Value {};
} // namespace

/**
 * Implements a return value policy that
 * returns a numpy array from a function returning a std::vector by value
 *
 * It is only possible to clone these types since a wrapper would wrap temporary
 */
struct MatrixToNumpy {
  // The boost::python framework calls return_value_policy::apply<T>::type
  template <class T> struct apply {
    // Typedef that removes any const from the type
    using non_const_type = typename std::remove_const<T>::type;
    // MPL compile-time check that T is a std::vector
    using type = typename boost::mpl::if_c<
        is_matrix<non_const_type>::value,
        MatrixRefToNumpyImpl<non_const_type, Converters::Clone>,
        MatrixToNumpy_Requires_Matrix_Return_By_Value<T>>::type;
  };
};
} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_MATRIXTONUMPY_H_ */
