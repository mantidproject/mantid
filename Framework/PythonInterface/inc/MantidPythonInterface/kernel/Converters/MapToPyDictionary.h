#ifndef MANTID_PYTHONINERFACE_MAPTOPYDICTIONARY_H_
#define MANTID_PYTHONINERFACE_MAPTOPYDICTIONARY_H_

/** Converter to generate a python dictionary from a std::map

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <map>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

template <typename KeyType, typename ValueType>
struct DLLExport MapToPyDictionary {

public:
  using MapType = std::map<KeyType, ValueType>;

  MapToPyDictionary(const MapType &map) : m_map(map) {}
  /// Produces a python dictionary
  boost::python::dict operator()() {
    using namespace boost::python;
    boost::python::dict dictionary;
    for (const auto &pair : this->m_map) {
      dictionary[object(pair.first)] = object(pair.second);
    }
    return dictionary;
  }

private:
  /// Map
  MapType m_map;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_MAPTOPYDICTIONARY_H_ */
