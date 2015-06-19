#include "MantidDataObjects/CalculateReflectometryQBase.h"
#include <cmath>

namespace Mantid {
namespace DataObjects {

CalculateReflectometryQBase::CalculateReflectometryQBase()
    : to_radians_factor(M_PI / 180) {}

CalculateReflectometryQBase::~CalculateReflectometryQBase() {}
}
}
