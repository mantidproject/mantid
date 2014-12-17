#include "MantidMDEvents/CalculateReflectometryQBase.h"
#include <cmath>

namespace Mantid {
namespace MDEvents {

CalculateReflectometryQBase::CalculateReflectometryQBase()
    : to_radians_factor(M_PI / 180) {}

CalculateReflectometryQBase::~CalculateReflectometryQBase() {}
}
}
