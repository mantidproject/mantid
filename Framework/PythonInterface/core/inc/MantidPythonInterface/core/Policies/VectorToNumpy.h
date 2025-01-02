// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/Converters/CloneToNDArray.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/NDArray.h"

#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>
#include <type_traits>
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
/// The general one inherits from std::false_type
template <typename T> struct is_std_vector : std::false_type {};

/// Specialization for std::vector types to inherit from
/// std::true_type
template <typename T> struct is_std_vector<std::vector<T>> : std::true_type {};

//-----------------------------------------------------------------------
// VectorRefToNumpyImpl - Policy for reference returns
//-----------------------------------------------------------------------
/**
 * Helper struct that implements the conversion policy.
 */
template <typename VectorType, typename ConversionPolicy> struct VectorRefToNumpyImpl {
  inline PyObject *operator()(const VectorType &cvector) const {
    return Converters::VectorToNDArray<typename VectorType::value_type, ConversionPolicy>()(cvector);
  }

  inline PyTypeObject const *get_pytype() const { return ndarrayType(); }
};

template <typename T> struct VectorRefToNumpy_Requires_Reference_To_StdVector_Return_Type {};
} // namespace

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
    using non_const_type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    // MPL compile-time check that T is a reference to a std::vector
    using type = typename boost::mpl::if_c<is_std_vector<non_const_type>::value,
                                           VectorRefToNumpyImpl<non_const_type, ConversionPolicy>,
                                           VectorRefToNumpy_Requires_Reference_To_StdVector_Return_Type<T>>::type;
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
    return Converters::VectorToNDArray<typename VectorType::value_type, Converters::Clone>()(cvector);
  }

  inline PyTypeObject const *get_pytype() const { return ndarrayType(); }
};

template <typename T> struct VectorToNumpy_Requires_StdVector_Return {};
} // namespace

/**
 * Implements a return value policy that
 * returns a numpy array from a function returning a std::vector by ref or value
 *
 * It is only possible to clone these types since a wrapper would wrap temporary
 */
struct VectorToNumpy {
  // The boost::python framework calls return_value_policy::apply<T>::type
  template <class T> struct apply {
    // Typedef that removes any const from the type
    using non_const_type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    // MPL compile-time check that T is a std::vector
    using type = typename boost::mpl::if_c<is_std_vector<non_const_type>::value,
                                           VectorRefToNumpyImpl<non_const_type, Converters::Clone>,
                                           VectorToNumpy_Requires_StdVector_Return<T>>::type;
  };
};
} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid
