#include "MantidSINQ/PoldiUtilities/PoldiSpectrumConstantBackground.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Poldi {

using namespace CurveFitting;
using namespace API;

DECLARE_FUNCTION(PoldiSpectrumConstantBackground)

/// Default constructor
PoldiSpectrumConstantBackground::PoldiSpectrumConstantBackground()
    : FlatBackground(), IPoldiFunction1D(), m_timeBinCount(0) {}

/// Destructor
PoldiSpectrumConstantBackground::~PoldiSpectrumConstantBackground() {}

void PoldiSpectrumConstantBackground::setWorkspace(
    boost::shared_ptr<const API::Workspace> ws) {
  MatrixWorkspace_const_sptr matrixWs =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);

  if (matrixWs && matrixWs->getNumberHistograms() > 0) {
    m_timeBinCount = matrixWs->readX(0).size();
  }
}

size_t PoldiSpectrumConstantBackground::getTimeBinCount() const {
  return m_timeBinCount;
}

void PoldiSpectrumConstantBackground::poldiFunction1D(
    const std::vector<int> &indices, const API::FunctionDomain1D &domain,
    API::FunctionValues &values) const {
  double backgroundDetector = getParameter(0);
  double wireCount = static_cast<double>(indices.size());
  double distributionFactor = wireCount * static_cast<double>(m_timeBinCount) /
                              static_cast<double>(domain.size());
  double backgroundD = backgroundDetector * distributionFactor;

  for (size_t i = 0; i < values.size(); ++i) {
    values.addToCalculated(i, backgroundD);
  }
}

} // namespace SINQ
} // namespace Mantid
