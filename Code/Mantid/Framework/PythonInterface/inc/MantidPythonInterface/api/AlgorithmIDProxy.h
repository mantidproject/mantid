#ifndef MANTID_PYTHONINTERFACE_ALGORITHMIDPROXY_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMIDPROXY_H_
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include "MantidAPI/IAlgorithm.h" // for AlgorithmID typedef

namespace Mantid {
namespace PythonInterface {
/**
 * Provides a concrete type to wrap & return AlgorithmIDs that are actually
 * just typedefs for void*
 */
struct AlgorithmIDProxy {
  /// Construct with existing pointer
  explicit AlgorithmIDProxy(API::AlgorithmID p) : id(p) {}

  bool operator==(const AlgorithmIDProxy &rhs) { return (id == rhs.id); }
  /// held ID value
  API::AlgorithmID id;
};
}
}

#endif /* MANTID_PYTHONINTERFACE_ALGORITHMIDPROXY_H_ */
