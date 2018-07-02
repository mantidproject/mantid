#ifndef MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_
#define MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_

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

namespace Mantid {
namespace PythonInterface {
namespace Converters {

// Free function to determine data type being stored in container
template <template <class> class Container, typename HeldType>
std::string dtype(const Container<HeldType> &) {
  if (std::is_same<HeldType, bool>::value) {
    return "bool_";
  } else if (std::is_same<HeldType, short>::value) {
    return "int16";
  } else if (std::is_same<HeldType, std::int8_t>::value) {
    return "int8";
  } else if (std::is_same<HeldType, std::int16_t>::value) {
    return "int16";
  } else if (std::is_same<HeldType, std::int32_t>::value) {
    return "int32";
  } else if (std::is_same<HeldType, std::int64_t>::value) {
    return "int64";
  } else if (std::is_same<HeldType, long>::value) {
    return "int_";
  } else if (std::is_same<HeldType, long long>::value) {
    return "int64";
  } else if (std::is_same<HeldType, std::float_t>::value) {
    return "float32";
  } else if (std::is_same<HeldType, std::double_t>::value) {
    return "float64";
  } else if (std::is_same<HeldType, std::string>::value) {
    return "string_";
  } else {
    return "object_";
  }
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /*MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_*/
