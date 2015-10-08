#ifndef MANTID_SINQ_UNCERTAINVALUEIO_H
#define MANTID_SINQ_UNCERTAINVALUEIO_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

namespace Mantid {
namespace Poldi {

/** UncertainValueIO :
 *
  String-I/O for UncertainValue

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

class MANTID_SINQ_DLL UncertainValueIO {
public:
  static std::string toString(const UncertainValue &uncertainValue) {
    if (uncertainValue.error() == 0.0) {
      return (boost::format("%f") % uncertainValue.value()).str();
    }

    return (boost::format("%f +/- %f") % uncertainValue.value() %
            uncertainValue.error()).str();
  }

  static UncertainValue fromString(const std::string &uncertainValueString) {
    if (uncertainValueString.size() == 0) {
      return UncertainValue();
    }

    std::vector<std::string> substrings;
    boost::iter_split(substrings, uncertainValueString,
                      boost::first_finder("+/-"));

    if (substrings.size() > 2) {
      throw std::runtime_error(
          "UncertainValue cannot be constructed from more than 2 values.");
    }

    std::vector<double> components(substrings.size());
    for (size_t i = 0; i < substrings.size(); ++i) {
      boost::trim(substrings[i]);
      components[i] = boost::lexical_cast<double>(substrings[i]);
    }

    if (components.size() == 1) {
      return UncertainValue(components[0]);
    }

    return UncertainValue(components[0], components[1]);
  }

private:
  UncertainValueIO() {}
};
}
}

#endif // MANTID_SINQ_UNCERTAINVALUEIO_H
