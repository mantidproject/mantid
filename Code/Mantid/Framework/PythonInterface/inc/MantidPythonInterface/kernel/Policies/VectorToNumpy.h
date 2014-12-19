#ifndef MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
#define MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
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
#include "MantidKernel/System.h"
#include "MantidPythonInterface/kernel/Converters/CloneToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/PyArrayType.h"

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>
#include <vector>

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
template <typename T> struct is_std_vector : boost::false_type {};

/// Specialization for std::vector types to inherit from
/// boost::true_type
template <typename T>
struct is_std_vector<std::vector<T>> : boost::true_type {};

//-----------------------------------------------------------------------
// VectorRefToNumpyImpl - Policy for reference returns
//-----------------------------------------------------------------------
/**
 * Helper struct that implements the conversion policy.
 */
template <typename VectorType, typename ConversionPolicy>
struct VectorRefToNumpyImpl {
  inline PyObject *operator()(const VectorType &cvector) const {
    return Converters::VectorToNDArray<typename VectorType::value_type,
                                       ConversionPolicy>()(cvector);
  }

  inline PyTypeObject const *get_pytype() const {
    return Converters::getNDArrayType();
  }
};

template <typename T>
struct VectorRefToNumpy_Requires_Reference_To_StdVector_Return_Type {};
}

/**
 * Implements a return value policy that
 * returns a numpy array from a rerence to a std::vector
 *
 * The type of conversion is specified by a policy:
 * (1) WrapReadOnly - Creates a read-only array around the original data (no
 *copy is performed)
 * (2) WrapReadWrite - Creates a read-write array around the original data (no
 *copy is performed)
 */
template <typename ConversionPolicy> struct VectorRefToNumpy {
  // The boost::python framework calls return_value_policy::apply<T>::type
  template <class T> struct apply {
    // Typedef that removes and const or reference qualifiers from the type
    typedef typename boost::remove_const<
        typename boost::remove_reference<T>::type>::type non_const_type;
    // MPL compile-time check that T is a reference to a std::vector
    typedef typename boost::mpl::if_c<
        boost::mpl::and_<boost::is_reference<T>,
                         is_std_vector<non_const_type>>::value,
        VectorRefToNumpyImpl<non_const_type, ConversionPolicy>,
        VectorRefToNumpy_Requires_Reference_To_StdVector_Return_Type<T>>::type
        type;
  };
};

//-----------------------------------------------------------------------
// VectorToNumpy return_value_policy
//-----------------------------------------------------------------------
namespace {
/**
 * Helper struct that implements the conversion policy. This can only clone
 * as wrapping would wrap a temporary
 */
template <typename VectorType> struct VectorToNumpyImpl {
  inline PyObject *operator()(const VectorType &cvector) const {
    return Converters::VectorToNDArray<typename VectorType::value_type,
                                       Converters::Clone>()(cvector);
  }

  inline PyTypeObject const *get_pytype() const {
    return Converters::getNDArrayType();
  }
};

template <typename T>
struct VectorToNumpy_Requires_StdVector_Return_By_Value {};
}

/**
 * Implements a return value policy that
 * returns a numpy array from a function returning a std::vector by value
 *
 * It is only possible to clone these types since a wrapper would wrap temporary
 */
struct VectorToNumpy {
  // The boost::python framework calls return_value_policy::apply<T>::type
  template <class T> struct apply {
    // Typedef that removes any const from the type
    typedef typename boost::remove_const<T>::type non_const_type;
    // MPL compile-time check that T is a std::vector
    typedef typename boost::mpl::if_c<
        is_std_vector<non_const_type>::value,
        VectorRefToNumpyImpl<non_const_type, Converters::Clone>,
        VectorToNumpy_Requires_StdVector_Return_By_Value<T>>::type type;
  };
};
}
}
}

#endif // MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
