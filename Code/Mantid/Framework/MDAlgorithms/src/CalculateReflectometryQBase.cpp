#include "MantidMDAlgorithms/CalculateReflectometryQBase.h"
#include <cmath>

namespace Mantid {
namespace MDAlgorithms {

CalculateReflectometryQBase::CalculateReflectometryQBase()
    : to_radians_factor(M_PI / 180) {}

CalculateReflectometryQBase::~CalculateReflectometryQBase() {}
}
}
