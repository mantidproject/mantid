#ifndef MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_
#define MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_

#include <string>
#include <type_traits>

/**
    ContainerDtype Header File

    A helper free function to allow identification of data type being used by
    providing a numpy friendly string (using the numpy array interface).

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
    return "b";
  } else if (std::is_integral<HeldType>::value) {
    return "i";
  } else if (std::is_floating_point<HeldType>::value) {
    return "f";
  } else {
    return "O";
  }
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /*MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_*/
