#ifndef MANTID_PYTHONINTERFACE_ASTYPE_H_
#define MANTID_PYTHONINTERFACE_ASTYPE_H_
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
#include <boost/mpl/and.hpp>
#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>
#include <type_traits>

/**
 * Policy that can convert to return type to a super type
 */

namespace Mantid {
namespace PythonInterface {
namespace Policies {
// Utility code to help out
namespace {

//-----------------------------------------------------------------------
// Polciy implementation
//-----------------------------------------------------------------------

// The entry point for the policy is in the struct AsType below. It does
// a check as to whether the return type is valid, if so it forwards the
// call to this struct
template <typename ReturnType, typename InputType> struct AsTypeImpl {

  inline PyObject *operator()(const InputType &p) const {
    using namespace boost::python;
    return to_python_value<ReturnType>()(ReturnType(p));
  }

  inline PyTypeObject const *get_pytype() const {
    using namespace boost::python;
    return converter::registered<ReturnType>::converters
        .to_python_target_type();
  }
};

// Error handler for shared pointer types. If return type is wrong then user
// sees the name of this
// class in the output, which hopefully gives a clue as to what is going on
template <typename T>
struct AsType_Requires_New_Type_Automatically_Convertible_To_Original {};

} // namespace

/**
 * Implements the AsType policy.
 */
template <class ReturnType> struct AsType {
  template <class InputType> struct apply {
    // Deduce if type is correct for policy, needs to be convertible to
    // ReturnType
    using type = typename boost::mpl::if_c<
        std::is_convertible<InputType, ReturnType>::value,
        AsTypeImpl<ReturnType, InputType>,
        AsType_Requires_New_Type_Automatically_Convertible_To_Original<
            InputType>>::type;
  };
};

} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ASTYPE_H */
