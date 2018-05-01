#include "GSASIIRefineFitPeaksOutputProperties.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASIIRefineFitPeaksOutputProperties::GSASIIRefineFitPeaksOutputProperties(
    const double _rwp, const double _sigma, const double _gamma,
    const Mantid::API::MatrixWorkspace_sptr _fittedPeaksWS,
    const Mantid::API::ITableWorkspace_sptr _latticeParamsWS,
    const RunLabel &_runLabel)
    : rwp(_rwp), sigma(_sigma), gamma(_gamma), fittedPeaksWS(_fittedPeaksWS),
      latticeParamsWS(_latticeParamsWS), runLabel(_runLabel) {}

bool operator==(const GSASIIRefineFitPeaksOutputProperties &lhs,
                const GSASIIRefineFitPeaksOutputProperties &rhs) {
  return lhs.rwp == rhs.rwp && lhs.sigma == rhs.sigma &&
         lhs.gamma == rhs.gamma && lhs.fittedPeaksWS == rhs.fittedPeaksWS &&
         lhs.latticeParamsWS == rhs.latticeParamsWS &&
         lhs.runLabel == rhs.runLabel;
}

bool operator!=(const GSASIIRefineFitPeaksOutputProperties &lhs,
                const GSASIIRefineFitPeaksOutputProperties &rhs) {
  return !(lhs == rhs);
}

} // MantidQt
} // CustomInterfaces
