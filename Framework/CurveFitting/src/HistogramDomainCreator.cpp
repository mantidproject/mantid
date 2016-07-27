#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/HistogramDomainCreator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace CurveFitting {

using namespace API;

/**
 * Constructor.
 * @param fun :: A function for which a domain is required.
 * @param manager :: Pointer to IPropertyManager (Algorithm) instance.
 * @param workspacePropertyName :: Name of the output property for a created
 * workspace in case a PropertyManager is used.
 */
HistogramDomainCreator::HistogramDomainCreator(Kernel::IPropertyManager &manager,
    const std::string &workspacePropertyName)
    : IMWDomainCreator(&manager, workspacePropertyName) {}

/**
 * Creates a domain corresponding to the assigned MatrixWorkspace
 *
 * @param domain :: Pointer to outgoing FunctionDomain instance.
 * @param values :: Pointer to outgoing FunctionValues object.
 * @param i0 :: Size offset for values object if it already contains data.
 */
void HistogramDomainCreator::createDomain(
    boost::shared_ptr<FunctionDomain> &domain,
    boost::shared_ptr<FunctionValues> &values, size_t i0) {

  setParameters();

  if (!m_matrixWorkspace->isHistogramData()) {
    throw std::runtime_error("Cannot create a histogram domain from point data.");
  }

  if (m_domainType != Simple) {
    throw std::runtime_error("Cannot create non-simple domain for histogram fitting.");
  }

  const Mantid::MantidVec &X = m_matrixWorkspace->readX(m_workspaceIndex);

  // find the fitting interval: from -> to
  size_t endIndex = 0;
  std::tie(m_startIndex, endIndex) = getXInterval();
  auto fromX = X.begin() + m_startIndex;
  auto toX = X.begin() + endIndex + 1;

  domain.reset(new API::FunctionDomain1DHistogram(fromX, toX));
  assert(endIndex - m_startIndex == domain->size());

  if (!values) {
    values.reset(new API::FunctionValues(*domain));
  } else {
    values->expand(i0 + domain->size());
  }

  // set the data to fit to
  const Mantid::MantidVec &Y = m_matrixWorkspace->readY(m_workspaceIndex);
  const Mantid::MantidVec &E = m_matrixWorkspace->readE(m_workspaceIndex);
  if (endIndex > Y.size()) {
    throw std::runtime_error("FitMW: Inconsistent MatrixWorkspace");
  }

  bool isDistribution = m_matrixWorkspace->isDistribution();

  for (size_t i = m_startIndex; i < endIndex; ++i) {
    size_t j = i - m_startIndex + i0;
    double y = Y[i];
    double error = E[i];
    double weight = 0.0;

    if (isDistribution) {
      // If workspace is a distribution, convert data to histogram
      auto dx = X[i + 1] - X[i];
      y *= dx;
      error *= dx;
    }

    if (!boost::math::isfinite(y)) // nan or inf data
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
      y = 0.0; // leaving inf or nan would break the fit
    } else if (!boost::math::isfinite(error)) // nan or inf error
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
    } else if (error <= 0) {
      if (!m_ignoreInvalidData)
        weight = 1.0;
    } else {
      weight = 1.0 / error;
    }

    values->setFitData(j, y);
    values->setFitWeight(j, weight);
  }
  m_domain = boost::dynamic_pointer_cast<API::FunctionDomain1D>(domain);
  m_values = values;

}

} // namespace CurveFitting
} // namespace Mantid
