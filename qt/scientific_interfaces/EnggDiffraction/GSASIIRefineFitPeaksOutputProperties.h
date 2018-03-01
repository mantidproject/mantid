#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSOUTPUTPROPERTIES_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSOUTPUTPROPERTIES_H_

#include "DllConfig.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASIIRefineFitPeaksOutputProperties {
  GSASIIRefineFitPeaksOutputProperties(
      const double _rwp, const double _sigma, const double _gamma,
      const Mantid::API::MatrixWorkspace_sptr _fittedPeaksWS,
      const Mantid::API::ITableWorkspace_sptr _latticeParamsWS);

  const double rwp;
  const double sigma;
  const double gamma;
  const Mantid::API::MatrixWorkspace_sptr fittedPeaksWS;
  const Mantid::API::ITableWorkspace_sptr latticeParamsWS;
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
