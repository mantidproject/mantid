#ifndef MANTID_PYTHONINTERFACE_TOPYLIST_H_
#define MANTID_PYTHONINTERFACE_TOPYLIST_H_
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
#include <boost/python/list.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
//-----------------------------------------------------------------------
// Converter implementation
//-----------------------------------------------------------------------
/**
 * Converter that takes a std::vector and converts it into a python list.
 * It is able to convert anything for which a converter is already registered
 */
template <typename ElementType> struct ToPyList {
  /**
   * Converts a cvector to a numpy array
   * @param cdata :: A const reference to a vector
   * @returns A new python list object
   */
  inline boost::python::list
  operator()(const std::vector<ElementType> &cdata) const {
    boost::python::list result;
    for (const auto &item : cdata) {
      result.append(item);
    }
    return result;
  }
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_TOPYLIST_H_ */
