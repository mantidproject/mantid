#ifndef MANTID_SINQ_MILLERINDICESIO_H
#define MANTID_SINQ_MILLERINDICESIO_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

namespace Mantid {
namespace Poldi {

/** MillerIndicesIO :
 *
  String-I/O for MillerIndices

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2014

    Copyright © 2014 PSI-MSS

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

class MANTID_SINQ_DLL MillerIndicesIO {
public:
  static std::string toString(const MillerIndices &millerIndices) {
    return (boost::format("%i %i %i") % millerIndices.h() % millerIndices.k() %
            millerIndices.l()).str();
  }

  static MillerIndices fromString(const std::string &millerIncidesString) {
    std::vector<std::string> substrings;
    boost::split(substrings, millerIncidesString, boost::is_any_of(" "));

    std::vector<int> indices(substrings.size());
    for (size_t i = 0; i < substrings.size(); ++i) {
      indices[i] = boost::lexical_cast<int>(substrings[i]);
    }

    return MillerIndices(indices);
  }

private:
  MillerIndicesIO() {}
};
}
}

#endif // MANTID_SINQ_MILLERINDICESIO_H
