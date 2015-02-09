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
    : IFunction1D(), IPoldiFunction1D(), m_timeBinCount(0), m_flatBackground() {
}

/// Destructor
PoldiSpectrumConstantBackground::~PoldiSpectrumConstantBackground() {}

void PoldiSpectrumConstantBackground::function1D(double *out,
                                                 const double *xValues,
                                                 const size_t nData) const {

  if (m_flatBackground) {
    m_flatBackground->function1D(out, xValues, nData);
  }
}

void PoldiSpectrumConstantBackground::functionDeriv1D(Jacobian *out,
                                                      const double *xValues,
                                                      const size_t nData) {
  if (m_flatBackground) {
    m_flatBackground->functionDeriv1D(out, xValues, nData);
  }
}

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
  if (m_flatBackground) {
    double backgroundDetector = m_flatBackground->getParameter(0);
    double wireCount = static_cast<double>(indices.size());
    double distributionFactor = wireCount *
                                static_cast<double>(m_timeBinCount) /
                                static_cast<double>(domain.size());
    double backgroundD = backgroundDetector * distributionFactor;

    for (size_t i = 0; i < values.size(); ++i) {
      values.addToCalculated(i, backgroundD);
    }
  }
}

void PoldiSpectrumConstantBackground::setParameter(const std::string &name,
                                                   const double &value,
                                                   bool explicitlySet) {
  ParamFunction::setParameter(name, value, explicitlySet);
}

void PoldiSpectrumConstantBackground::setParameter(size_t i,
                                                   const double &value,
                                                   bool explicitlySet) {
  if (m_flatBackground) {
    m_flatBackground->setParameter(i, value, explicitlySet);
  }
}

double PoldiSpectrumConstantBackground::getParameter(const std::string &name) const
{
    return ParamFunction::getParameter(name);
}

double PoldiSpectrumConstantBackground::getParameter(size_t i) const
{
    if(m_flatBackground) {
        return m_flatBackground->getParameter(i);
    }

    return 0;
}

void PoldiSpectrumConstantBackground::init() {
  m_flatBackground = boost::dynamic_pointer_cast<IFunction1D>(
      FunctionFactory::Instance().createFunction("FlatBackground"));

  declareParameter("A0");
}

} // namespace SINQ
} // namespace Mantid
