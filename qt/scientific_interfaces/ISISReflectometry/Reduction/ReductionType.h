#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {
enum class ReductionType { DivergentBeam, NonFlatSample, Normal };

inline ReductionType reductionTypeFromString(std::string const &reductionType) {
  if (reductionType == "Normal")
    return ReductionType::Normal;
  else if (reductionType == "DivergentBeam")
    return ReductionType::DivergentBeam;
  else if (reductionType == "NonFlatSample")
    return ReductionType::NonFlatSample;
  else
    throw std::runtime_error("Unexpected reduction type.");
}
}
}
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONTYPE_H_
