#include "GSASIIRefineFitPeaksParameters.h"

namespace MantidQt {
namespace CustomInterfaces {

GSASIIRefineFitPeaksParameters::GSASIIRefineFitPeaksParameters(
    const Mantid::API::MatrixWorkspace_sptr &_inputWorkspace,
    const RunLabel &_runLabel, const GSASRefinementMethod _refinementMethod,
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

} // MantidQt
} // CustomInterfaces
