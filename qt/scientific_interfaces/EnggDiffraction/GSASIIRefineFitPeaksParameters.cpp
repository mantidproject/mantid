#include "GSASIIRefineFitPeaksParameters.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASIIRefineFitPeaksParameters::GSASIIRefineFitPeaksParameters(
    const Mantid::API::MatrixWorkspace_sptr &_inputWorkspace,
    const RunLabel &_runLabel, const GSASRefinementMethod &_refinementMethod,
    const std::string &_instParamsFile,
    const std::vector<std::string> &_phaseFiles, const std::string &_gsasHome,
    const std::string &_gsasProjectFile, const boost::optional<double> _dMin,
    const boost::optional<double> _negativeWeight,
    const boost::optional<double> _xMin, const boost::optional<double> _xMax,
    const bool _refineSigma, const bool _refineGamma)
    : inputWorkspace(_inputWorkspace), runLabel(_runLabel),
      refinementMethod(_refinementMethod), instParamsFile(_instParamsFile),
      phaseFiles(_phaseFiles), gsasHome(_gsasHome),
      gsasProjectFile(_gsasProjectFile), dMin(_dMin),
      negativeWeight(_negativeWeight), xMin(_xMin), xMax(_xMax),
      refineSigma(_refineSigma), refineGamma(_refineGamma) {}

bool operator==(const GSASIIRefineFitPeaksParameters &lhs,
                const GSASIIRefineFitPeaksParameters &rhs) {
  return lhs.inputWorkspace == rhs.inputWorkspace &&
         lhs.runLabel == rhs.runLabel &&
         lhs.refinementMethod == rhs.refinementMethod &&
         lhs.instParamsFile == rhs.instParamsFile &&
         lhs.phaseFiles == rhs.phaseFiles && lhs.gsasHome == rhs.gsasHome &&
         lhs.gsasProjectFile == rhs.gsasProjectFile && lhs.dMin == rhs.dMin &&
         lhs.negativeWeight == rhs.negativeWeight && lhs.xMin == rhs.xMin &&
         lhs.xMax == rhs.xMax && lhs.refineSigma == rhs.refineSigma &&
         lhs.refineGamma == rhs.refineGamma;
}

bool operator!=(const GSASIIRefineFitPeaksParameters &lhs,
                const GSASIIRefineFitPeaksParameters &rhs) {
  return !(lhs == rhs);
}

} // namespace CustomInterfaces
} // namespace MantidQt
