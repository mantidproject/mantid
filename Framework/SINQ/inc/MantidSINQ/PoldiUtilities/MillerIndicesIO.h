// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "boost/lexical_cast.hpp"

namespace Mantid {
namespace Poldi {

/** MillerIndicesIO :
 *
  String-I/O for MillerIndices

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2014
*/

class MANTID_SINQ_DLL MillerIndicesIO {
public:
  static std::string toString(const MillerIndices &millerIndices) {
    return (boost::format("%i %i %i") % millerIndices.h() % millerIndices.k() % millerIndices.l()).str();
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
  MillerIndicesIO() = default;
};
} // namespace Poldi
} // namespace Mantid
