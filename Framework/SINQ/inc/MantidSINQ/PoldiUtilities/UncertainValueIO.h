// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "boost/lexical_cast.hpp"

namespace Mantid {
namespace Poldi {

/** UncertainValueIO :
 *
  String-I/O for UncertainValue

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2014
*/

class MANTID_SINQ_DLL UncertainValueIO {
public:
  static std::string toString(const UncertainValue &uncertainValue) {
    if (uncertainValue.error() == 0.0) {
      return (boost::format("%f") % uncertainValue.value()).str();
    }

    return (boost::format("%f +/- %f") % uncertainValue.value() % uncertainValue.error()).str();
  }

  static UncertainValue fromString(const std::string &uncertainValueString) {
    if (uncertainValueString.empty()) {
      return UncertainValue();
    }

    std::vector<std::string> substrings;
    boost::iter_split(substrings, uncertainValueString, boost::first_finder("+/-"));

    if (substrings.size() > 2) {
      throw std::runtime_error("UncertainValue cannot be constructed from more than 2 values.");
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
  UncertainValueIO() = default;
};
} // namespace Poldi
} // namespace Mantid
