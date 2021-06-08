// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/ProfileChiSquared1D.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/CalculateChiSquared.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/GSLJacobian.h"

#include <boost/math/distributions/chi_squared.hpp>

namespace {
// The maximum difference of chi squared to search for
// 10.8276 covers  99.9% of the distrubition
constexpr double MAXCHISQUAREDIFFERENCE = 10.8276;

/// Calculate the change in chi2
/// @param domain :: Function's domain.
/// @param nParams :: Number of free fitting parameters.
/// @param values :: Functin's values.
/// @param chi0 :: Chi squared at the minimum.
double getDiff(const Mantid::API::IFunction &fun, size_t nParams, const Mantid::API::FunctionDomain &domain,
               Mantid::API::FunctionValues &values, double chi0) {
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  double dof = 0;
  Mantid::CurveFitting::Algorithms::CalculateChiSquared::calcChiSquared(fun, nParams, domain, values, chiSquared,
                                                                        chiSquaredWeighted, dof);
  return chiSquaredWeighted - chi0;
}

} // namespace

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

DECLARE_ALGORITHM(ProfileChiSquared1D)

using namespace Mantid::API;
/// Helper class to calculate the chi squared along a direction in the parameter
/// space.
class ChiSlice {
public:
  /// Constructor.
  /// @param inputFunction :: The fitting function
  /// @param fixedParameterIndex :: index of the parameter which is fixed
  /// @param inputWS :: The input workspace (used for fit algorithm)
  /// @param workspaceIndex :: Workspace index (used for fit algorithm)
  /// @param domain :: Function's domain.
  /// @param values :: Functin's values.
  /// @param chi0 :: Chi squared at the minimum.
  /// @param freeParameters :: Parameters which are free in the function.
  ChiSlice(IFunction_sptr inputFunction, int fixedParameterIndex, API::MatrixWorkspace_sptr inputWS, int workspaceIndex,
           const API::FunctionDomain &domain, API::FunctionValues &values, double chi0,
           std::vector<int> &freeParameters)
      : m_fixedParameterIndex(fixedParameterIndex), m_domain(domain), m_values(values), m_chi0(chi0),
        m_function(inputFunction), m_ws(inputWS), m_workspaceIndex(workspaceIndex), m_freeParameters(freeParameters) {
    // create a fitting algorithm based on least squares (which is the default)
    m_fitalg = AlgorithmFactory::Instance().create("Fit", -1);
    m_fitalg->setChild(true);
  }
  /// Calculate the value of chi squared along the chosen direction at a
  /// distance from
  /// the minimum point.
  /// @param p :: A distance from the minimum.
  double operator()(double p) {
    m_fitalg->initialize();
    m_fitalg->setProperty("Function", m_function);
    m_fitalg->setProperty("InputWorkspace", m_ws);
    m_fitalg->setProperty("WorkspaceIndex", m_workspaceIndex);
    IFunction_sptr function = m_fitalg->getProperty("Function");
    std::vector<double> originalParamValues(function->nParams());
    for (auto ip = 0u; ip < function->nParams(); ++ip) {
      originalParamValues[ip] = function->getParameter(ip);
    }
    function->setParameter(m_fixedParameterIndex, originalParamValues[m_fixedParameterIndex] + p);
    function->fix(m_fixedParameterIndex);

    // re run the fit to minimze the unfixed parameters
    m_fitalg->execute();
    // find change in chi 2
    // num free parameters is the number of global free parameters - the 1 we've
    // just fixed
    int numFreeParameters = static_cast<int>(m_freeParameters.size() - 1);
    double res = getDiff(*function, numFreeParameters, m_domain, m_values, m_chi0);
    // reset fit to original values
    for (auto ip = 0u; ip < function->nParams(); ++ip) {
      function->setParameter(ip, originalParamValues[ip]);
    }
    function->unfix(m_fixedParameterIndex);
    return res;
  }

  /// Make an approximation for this slice on an interval.
  /// @param lBound :: The left bound of the approximation interval.
  /// @param rBound :: The right bound of the approximation interval.
  /// @param P :: Output vector with approximation parameters.
  /// @param A :: Output vector with approximation parameters.
  Functions::ChebfunBase_sptr makeApprox(double lBound, double rBound, std::vector<double> &P, std::vector<double> &A) {

    auto base = Functions::ChebfunBase::bestFitAnyTolerance(lBound, rBound, *this, P, A, 1.0, 1e-4, 129);
    if (!base) {
      base = std::make_shared<Functions::ChebfunBase>(10, lBound, rBound, 1e-4);
      P = base->fit(*this);
      A = base->calcA(P);
    }
    return base;
  }

  /// Fiind a displacement in the parameter space from the initial point
  /// to a point where the PDF drops significantly.
  /// @param shift :: Initial shift form par0 value.
  double findBound(double shift) {
    double bound0 = 0;
    double diff0 = (*this)(0);
    double bound = shift;
    bool canDecrease = true;
    for (size_t i = 0; i < 100; ++i) {
      double diff = (*this)(bound);

      bool isIncreasing = fabs(bound) > fabs(bound0) && diff > diff0;
      if (canDecrease) {
        if (isIncreasing)
          canDecrease = false;
      } else {
        if (!isIncreasing) {
          bound = bound0;
          break;
        }
      }

      bound0 = bound;
      diff0 = diff;

      if (diff > MAXCHISQUAREDIFFERENCE - 1) {
        if (diff < MAXCHISQUAREDIFFERENCE) {
          break;
        }
        // diff is too large
        bound *= 0.75;
      } else {
        // diff is too small
        bound *= 2;
      }
    }
    return bound;
  }

private:
  // Fixed parameter index
  int m_fixedParameterIndex;
  /// The domain
  const API::FunctionDomain &m_domain;
  /// The values
  API::FunctionValues &m_values;
  /// The chi squared at the minimum
  double m_chi0;
  // fitting algorithm
  IAlgorithm_sptr m_fitalg;
  // Input workspace and function
  IFunction_sptr m_function;
  MatrixWorkspace_sptr m_ws;
  int m_workspaceIndex;
  // Vector of free parameter indices
  std::vector<int> m_freeParameters;
}; // namespace Algorithms

/// Default constructor
ProfileChiSquared1D::ProfileChiSquared1D() : IFittingAlgorithm() {}

const std::string ProfileChiSquared1D::name() const { return "ProfileChiSquared1D"; }

int ProfileChiSquared1D::version() const { return 1; }

const std::string ProfileChiSquared1D::summary() const {
  return "Profiles chi squared about its minimum to obtain parameter errors "
         "for the input function.";
}

void ProfileChiSquared1D::initConcrete() { declareProperty("Output", "", "A base name for output workspaces."); }

void ProfileChiSquared1D::execConcrete() {
  // Number of fiting parameters
  auto nParams = m_function->nParams();
  // Create an output table for displaying slices of the chi squared and
  // the probabilitydensity function
  auto pdfTable = API::WorkspaceFactory::Instance().createTable();

  // Sigma confidence levels, could be an input but for now look for 1 sigma
  // (68%) 2 sigma (95) and 3(99%) error bounds chi2 disturbution has 1 degree
  // of freedom if we are changing 1 parameter at a time
  boost::math::chi_squared chi2Dist(1);
  std::array<double, 3> sigmas = {1, 2, 3};
  std::array<double, 3> qvalues;
  for (size_t i = 0; i < sigmas.size(); i++) {
    double pvalue = std::erf(sigmas[i] / sqrt(2));
    // find chi2 quanitile for given p value
    qvalues[i] = boost::math::quantile(chi2Dist, pvalue);
  }

  // Find number of free parameter, should be >= 2
  std::vector<int> freeParameters;
  for (size_t ip = 0; ip < nParams; ++ip) {
    if (m_function->isActive(ip)) {
      freeParameters.push_back(static_cast<int>(ip));
    }
  }

  if (freeParameters.size() < 2) {
    throw std::invalid_argument("Function must have 2 or more free parameters");
  }

  std::string baseName = getProperty("Output");
  Workspace_sptr ws = getProperty("InputWorkspace");
  int workspaceIndex = getProperty("WorkspaceIndex");
  MatrixWorkspace_sptr inputws = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  if (baseName.empty()) {
    baseName = "ProfileChiSquared1D";
  }
  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("PDFs", "", Kernel::Direction::Output),
                  "The name of the TableWorkspace in which to store the "
                  "pdfs of fit parameters");
  setPropertyValue("PDFs", baseName + "_pdf");
  setProperty("PDFs", pdfTable);

  // Create an output table for displaying the parameter errors.
  auto errorsTable = API::WorkspaceFactory::Instance().createTable();
  auto nameColumn = errorsTable->addColumn("str", "Parameter");
  auto valueColumn = errorsTable->addColumn("double", "Value");
  auto minValueColumn = errorsTable->addColumn("double", "Value at Min");
  auto leftErrColumn = errorsTable->addColumn("double", "Left Error (1-sigma)");
  auto rightErrColumn = errorsTable->addColumn("double", "Right Error (1-sigma)");
  auto leftErrColumn_2 = errorsTable->addColumn("double", "Left Error (2-sigma)");
  auto rightErrColumn_2 = errorsTable->addColumn("double", "Right Error (2-sigma )");
  auto leftErrColumn_3 = errorsTable->addColumn("double", "Left Error (3-sigma)");
  auto rightErrColumn_3 = errorsTable->addColumn("double", "Right Error (3-sigma )");
  auto quadraticErrColumn = errorsTable->addColumn("double", "Quadratic Error (1-sigma)");
  errorsTable->setRowCount(freeParameters.size());
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("Errors", "", Kernel::Direction::Output),
      "The name of the TableWorkspace in which to store the "
      "values and errors of fit parameters");
  setPropertyValue("Errors", baseName + "_errors");
  setProperty("Errors", errorsTable);

  // Calculate initial values
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  double dof = 0;
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  m_domainCreator->createDomain(domain, values);
  CalculateChiSquared::calcChiSquared(*m_function, nParams, *domain, *values, chiSquared, chiSquaredWeighted, dof);
  // Value of chi squared for current parameters in m_function
  double chi0 = chiSquaredWeighted;

  // Parameter bounds that define a volume in the parameter
  // space within which the chi squared is being examined.
  GSLVector lBounds(nParams);
  GSLVector rBounds(nParams);

  // Number of points in lines for plotting
  size_t n = 100;
  pdfTable->setRowCount(n);
  const double fac = 1e-4;

  for (auto p = 0u; p < freeParameters.size(); ++p) {
    int row = p;
    int ip = freeParameters[p];
    // Add columns for the parameter to the pdf table.
    auto parName = m_function->parameterName(ip);
    nameColumn->read(row, parName);
    // Parameter values
    auto col1 = pdfTable->addColumn("double", parName);
    col1->setPlotType(1);
    // Chi squared values
    auto col2 = pdfTable->addColumn("double", parName + "_chi2");
    col2->setPlotType(2);
    // PDF values
    auto col3 = pdfTable->addColumn("double", parName + "_pdf");
    col3->setPlotType(2);

    double par0 = m_function->getParameter(ip);
    double shift = fabs(par0 * fac);
    if (shift == 0.0) {
      shift = fac;
    }

    // Make a slice along this parameter
    ChiSlice slice(m_function, ip, inputws, workspaceIndex, *domain, *values, chi0, freeParameters);

    // Find the bounds withn which the PDF is significantly above zero.
    // The bounds are defined relative to par0:
    //   par0 + lBound is the lowest value of the parameter (lBound <= 0)
    //   par0 + rBound is the highest value of the parameter (rBound >= 0)
    double lBound = slice.findBound(-shift);
    double rBound = slice.findBound(shift);
    lBounds[ip] = lBound;
    rBounds[ip] = rBound;

    // Approximate the slice with a polynomial.
    // P is a vector of values of the polynomial at special points.
    // A is a vector of Chebyshev expansion coefficients.
    // The polynomial is defined on interval [lBound, rBound]
    // The value of the polynomial at 0 == chi squared at par0
    std::vector<double> P, A;
    auto base = slice.makeApprox(lBound, rBound, P, A);

    // Write n slice points into the output table.
    double dp = (rBound - lBound) / static_cast<double>(n);
    for (size_t i = 0; i < n; ++i) {
      double par = lBound + dp * static_cast<double>(i);
      double chi = base->eval(par, P);
      col1->fromDouble(i, par0 + par);
      col2->fromDouble(i, chi);
    }

    // Check if par0 is a minimum point of the chi squared
    std::vector<double> AD;
    // Calculate the derivative polynomial.
    // AD are the Chebyshev expansion of the derivative.
    base->derivative(A, AD);
    // Find the roots of the derivative polynomial
    std::vector<double> minima = base->roots(AD);
    if (minima.empty()) {
      minima.emplace_back(par0);
    }

    // If only 1 extremum is found assume (without checking) that it's a
    // minimum.
    // If there are more than 1, find the one with the smallest chi^2.
    double chiMin = std::numeric_limits<double>::max();
    double parMin = par0;
    for (double minimum : minima) {
      double value = base->eval(minimum, P);
      if (value < chiMin) {
        chiMin = value;
        parMin = minimum;
      }
    }
    // Get intersection of curve and line of constant q value to get confidence
    // interval on parameter ip
    valueColumn->fromDouble(row, par0);
    minValueColumn->fromDouble(row, par0 + parMin);
    for (size_t i = 0; i < qvalues.size(); i++) {
      auto [rootsMin, rootsMax] = getChiSquaredRoots(base, A, qvalues[i], rBound, lBound);
      errorsTable->getColumn(3 + 2 * i)->fromDouble(row, rootsMin - parMin);
      errorsTable->getColumn(4 + 2 * i)->fromDouble(row, rootsMax - parMin);
    }

    // Output the PDF
    for (size_t i = 0; i < n; ++i) {
      double chi = col2->toDouble(i);
      col3->fromDouble(i, exp(-chi + chiMin));
    }
    // reset parameter values back to original value
    m_function->setParameter(ip, par0);
  }

  // Square roots of the diagonals of the covariance matrix give
  // the standard deviations in the quadratic approximation of the chi^2.
  GSLMatrix V = getCovarianceMatrix();
  for (size_t i = 0; i < freeParameters.size(); ++i) {
    int ip = freeParameters[i];
    quadraticErrColumn->fromDouble(i, sqrt(V.get(ip, ip)));
  }
}

GSLMatrix ProfileChiSquared1D::getCovarianceMatrix() {
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  auto nParams = m_function->nParams();
  m_domainCreator->createDomain(domain, values);
  unfixParameters();
  GSLJacobian J(*m_function, values->size());
  m_function->functionDeriv(*domain, J);
  refixParameters();
  // Calculate the hessian at the current point.
  GSLMatrix H;
  H.resize(nParams, nParams);
  for (size_t i = 0; i < nParams; ++i) {
    for (size_t j = i; j < nParams; ++j) {
      double h = 0.0;
      for (size_t k = 0; k < values->size(); ++k) {
        double w = values->getFitWeight(k);
        h += J.get(k, i) * J.get(k, j) * w * w;
      }
      H.set(i, j, h);
      if (i != j) {
        H.set(j, i, h);
      }
    }
  }
  // Covariance matrix is inverse of hessian
  GSLMatrix V(H);
  V.invert();
  return V;
}

/// Temporary unfix any fixed parameters.
void ProfileChiSquared1D::unfixParameters() {
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (!m_function->isActive(i)) {
      m_function->unfix(i);
      m_fixedParameters.emplace_back(i);
    }
  }
}

/// Restore the "fixed" status of previously unfixed paramters.
void ProfileChiSquared1D::refixParameters() {
  for (auto &fixedParameter : m_fixedParameters) {
    m_function->fix(fixedParameter);
  }
  m_fixedParameters.clear();
}

std::tuple<double, double> ProfileChiSquared1D::getChiSquaredRoots(const Functions::ChebfunBase_sptr &approximation,
                                                                   std::vector<double> &coeffs, double qvalue,
                                                                   double rBound, double lBound) const {
  // Points of intersections with line chi^2 = 1  give an estimate of
  // the standard deviation of this parameter if it's uncorrelated with the
  // others.
  // Cache original value of A0
  auto Aold = coeffs[0];
  // Now find roots of curve when quantile is subtracted
  coeffs[0] = Aold - qvalue;
  std::vector<double> roots = approximation->roots(coeffs);
  std::sort(roots.begin(), roots.end());
  if (roots.empty()) {
    // Something went wrong; use the whole interval.
    roots.resize(2);
    roots[0] = lBound;
    roots[1] = rBound;
  } else if (roots.size() == 1) {
    // Only one root found; use a bound for the other root.
    if (roots.front() < 0) {
      roots.emplace_back(rBound);
    } else {
      roots.insert(roots.begin(), lBound);
    }
  } else if (roots.size() > 2) {
    // More than 2 roots; use the smallest and the biggest
    auto smallest = roots.front();
    auto biggest = roots.back();
    roots.resize(2);
    roots[0] = smallest;
    roots[1] = biggest;
  }
  coeffs[0] = Aold;
  return {roots[0], roots[1]};
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid