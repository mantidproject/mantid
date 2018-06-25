#ifndef MANTID_KERNEL_CONTAINERDTYPE_H_
#define MANTID_KERNEL_CONTAINERDTYPE_H_

#include <string>
#include <type_traits>
#include <vector>

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/init.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

/**
    ContainerDtype Header File

    A helper free function to allow identification of data type being used by
   providing a numpy friendly string.

    @author Lamar Moore STFC, Bhuvan Bezawada STFC
    @date 21/06/2018

    Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak
    Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace dtypeHelper {

// Free function to determine data type being stored in container
template <template <class> class Container, typename HeldType>
std::string dtype(const Container<HeldType>) {
  if (std::is_same<HeldType, bool>::value) {
    return "b";
  } else if (std::is_integral<HeldType>::value) {
    return "i";
  } else if (std::is_same<HeldType, std::float_t>::value) {
    return "f";
  } else if (std::is_same<HeldType, std::double_t>::value) {
    return "d";
  } else if (std::is_same<HeldType, std::string>::value) {
    return "s";
  } else {
    return "obj";
  }
}

} // namespace dtypeHelper

#endif /*MANTID_KERNEL_CONTAINERDTYPE_H_*/
