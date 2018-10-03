#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSPARAMETERS_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSPARAMETERS_H_

#include "DllConfig.h"
#include "EnggDiffGSASRefinementMethod.h"
#include "RunLabel.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ENGGDIFFRACTION_DLL GSASIIRefineFitPeaksParameters {
  GSASIIRefineFitPeaksParameters(
      const Mantid::API::MatrixWorkspace_sptr &_inputWorkspace,
      const RunLabel &_runLabel, const GSASRefinementMethod &_refinementMethod,
      const std::string &_instParamsFile,
      const std::vector<std::string> &_phaseFiles, const std::string &_gsasHome,
      const std::string &_gsasProjectFile, const boost::optional<double> _dMin,
      const boost::optional<double> _negativeWeight,
      const boost::optional<double> _xMin, const boost::optional<double> _xMax,
      const bool _refineSigma, const bool _refineGamma);

  const Mantid::API::MatrixWorkspace_sptr inputWorkspace;
  const RunLabel runLabel;
  const GSASRefinementMethod refinementMethod;
  const std::string instParamsFile;
  const std::vector<std::string> phaseFiles;
  const std::string gsasHome;
  const std::string gsasProjectFile;

  const boost::optional<double> dMin;
  const boost::optional<double> negativeWeight;
  const boost::optional<double> xMin;
  const boost::optional<double> xMax;

  const bool refineSigma;
  const bool refineGamma;
};

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator==(const GSASIIRefineFitPeaksParameters &lhs,
           const GSASIIRefineFitPeaksParameters &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool
operator!=(const GSASIIRefineFitPeaksParameters &lhs,
           const GSASIIRefineFitPeaksParameters &rhs);

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASIIREFINEFITPEAKSPARAMETERS_H_
