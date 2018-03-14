#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSOUTPUTPROPERTIES_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSOUTPUTPROPERTIES_H_

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASIIRefineFitPeaksOutputProperties {
  GSASIIRefineFitPeaksOutputProperties(const double _rwp, const double _sigma,
                                       const double _gamma);

  const double rwp;
  const double sigma;
  const double gamma;
};

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator==(const GSASIIRefineFitPeaksOutputProperties &lhs,
           const GSASIIRefineFitPeaksOutputProperties &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator!=(const GSASIIRefineFitPeaksOutputProperties &lhs,
           const GSASIIRefineFitPeaksOutputProperties &rhs);

} // MantidQt
} // CustomInterfaces

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSOUTPUTPROPERTIES_H_
