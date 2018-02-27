#include "GSASIIRefineFitPeaksOutputProperties.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASIIRefineFitPeaksOutputProperties::GSASIIRefineFitPeaksOutputProperties(
    const double _rwp, const double _sigma, const double _gamma)
    : rwp(_rwp), sigma(_sigma), gamma(_gamma) {}

bool operator==(const GSASIIRefineFitPeaksOutputProperties &lhs,
                const GSASIIRefineFitPeaksOutputProperties &rhs) {
  return lhs.rwp == rhs.rwp && lhs.sigma == rhs.sigma && lhs.gamma == rhs.gamma;
}

bool operator!=(const GSASIIRefineFitPeaksOutputProperties &lhs,
                const GSASIIRefineFitPeaksOutputProperties &rhs) {
  return !(lhs == rhs);
}

} // MantidQt
} // CustomInterfaces
