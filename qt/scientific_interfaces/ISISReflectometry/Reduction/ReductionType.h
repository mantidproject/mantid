// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {
/** The ReductionType determines what type of reduction is to be performed
 * by the reduction algorithm
 */
enum class ReductionType { DivergentBeam, NonFlatSample, Normal };

inline ReductionType reductionTypeFromString(std::string const &reductionType) {
  if (reductionType == "Normal")
    return ReductionType::Normal;
  else if (reductionType == "DivergentBeam")
    return ReductionType::DivergentBeam;
  else if (reductionType == "NonFlatSample")
    return ReductionType::NonFlatSample;
  else
    throw std::invalid_argument("Unexpected reduction type.");
}

inline std::string reductionTypeToString(ReductionType reductionType) {
  switch (reductionType) {
  case ReductionType::DivergentBeam:
    return "DivergentBeam";
  case ReductionType::NonFlatSample:
    return "NonFlatSample";
  case ReductionType::Normal:
    return "Normal";
  }
  throw std::invalid_argument("Unexpected reduction type");
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
