// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/HistogramDomainCreator.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"


namespace Mantid {
namespace CurveFitting {

using namespace API;

/**
 * Constructor.
 * @param manager :: Pointer to IPropertyManager (Algorithm) instance.
 * @param workspacePropertyName :: Name of the output property for a created
 * workspace in case a PropertyManager is used.
 */
HistogramDomainCreator::HistogramDomainCreator(
    Kernel::IPropertyManager &manager, const std::string &workspacePropertyName)
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
    throw std::runtime_error(
        "Cannot create a histogram domain from point data.");
  }

  if (m_domainType != Simple) {
    throw std::runtime_error(
        "Cannot create non-simple domain for histogram fitting.");
  }

  const auto &X = m_matrixWorkspace->x(m_workspaceIndex);

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
  const auto &Y = m_matrixWorkspace->counts(m_workspaceIndex);
  const auto &E = m_matrixWorkspace->countStandardDeviations(m_workspaceIndex);
  if (endIndex > Y.size()) {
    throw std::runtime_error("FitMW: Inconsistent MatrixWorkspace");
  }

  for (size_t i = m_startIndex; i < endIndex; ++i) {
    size_t j = i - m_startIndex + i0;
    double y = Y[i];
    double error = E[i];
    double weight = 0.0;

    if (!std::isfinite(y)) // nan or inf data
    {
      if (!m_ignoreInvalidData)
        throw std::runtime_error("Infinte number or NaN found in input data.");
      y = 0.0;                        // leaving inf or nan would break the fit
    } else if (!std::isfinite(error)) // nan or inf error
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

/**
 * Create an output workspace with the calculated values.
 * @param baseName :: Specifies the name of the output workspace
 * @param function :: A Pointer to the fitting function
 * @param domain :: The domain containing x-values for the function
 * @param values :: A API::FunctionValues instance containing the fitting data
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace> HistogramDomainCreator::createOutputWorkspace(
    const std::string &baseName, API::IFunction_sptr function,
    boost::shared_ptr<API::FunctionDomain> domain,
    boost::shared_ptr<API::FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  auto ws = IMWDomainCreator::createOutputWorkspace(
      baseName, function, domain, values, outputWorkspacePropertyName);

  if (m_matrixWorkspace->isDistribution()) {
    // Convert the calculated values to distribution
    auto &mws = dynamic_cast<MatrixWorkspace &>(*ws);
    auto &bins = dynamic_cast<FunctionDomain1DHistogram &>(*domain);
    for (size_t iSpec = 1; iSpec < mws.getNumberHistograms(); ++iSpec) {
      if (iSpec == 2) {
        // skip the diff spectrum
        continue;
      }
      auto &y = mws.mutableY(iSpec);
      auto &e = mws.mutableE(iSpec);
      double left = bins.leftBoundary();
      for (size_t i = 0; i < bins.size(); ++i) {
        double right = bins[i];
        double dx = right - left;
        y[i] /= dx;
        e[i] /= dx;
        left = right;
      }
    }
  }

  return ws;
}

} // namespace CurveFitting
} // namespace Mantid
