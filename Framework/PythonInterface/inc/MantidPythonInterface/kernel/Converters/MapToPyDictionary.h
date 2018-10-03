// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINERFACE_MAPTOPYDICTIONARY_H_
#define MANTID_PYTHONINERFACE_MAPTOPYDICTIONARY_H_

/** Converter to generate a python dictionary from a std::map
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
