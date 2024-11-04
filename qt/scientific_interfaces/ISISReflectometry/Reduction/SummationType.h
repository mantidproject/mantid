// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/optional.hpp>
#include <stdexcept>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** SummationType holds information about what type of summation should be
 * done in the reduction
 */
enum class SummationType { SumInLambda, SumInQ };

inline SummationType summationTypeFromString(std::string const &summationType) {
  if (summationType == "SumInLambda")
    return SummationType::SumInLambda;
  else if (summationType == "SumInQ")
    return SummationType::SumInQ;
  else
    throw std::invalid_argument("Unexpected summation type.");
}

inline std::string summationTypeToString(SummationType summationType) {
  switch (summationType) {
  case SummationType::SumInLambda:
    return "SumInLambda";
  case SummationType::SumInQ:
    return "SumInQ";
  }
  throw std::invalid_argument("Unexpected summation type");
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
