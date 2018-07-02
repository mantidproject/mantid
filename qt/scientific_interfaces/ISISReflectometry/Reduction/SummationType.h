#ifndef MANTID_CUSTOMINTERFACES_SUMMATIONTYPE_H_
#define MANTID_CUSTOMINTERFACES_SUMMATIONTYPE_H_
#include <string>
#include <stdexcept>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

enum class SummationType { SumInLambda, SumInQ };

inline SummationType summationTypeFromString(std::string const& summationType) {
  if (summationType == "SumInLambda")
    return SummationType::SumInLambda;
  else if (summationType == "SumInQ")
    return SummationType::SumInQ;
  else
    throw std::runtime_error("Unexpected summation type.");
}
}
}
#endif // MANTID_CUSTOMINTERFACES_SUMMATIONTYPE_H_
