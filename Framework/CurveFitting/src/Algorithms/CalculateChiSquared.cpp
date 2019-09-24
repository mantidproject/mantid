// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/CalculateChiSquared.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Functions/ChebfunBase.h"
#include "MantidCurveFitting/GSLJacobian.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Functions;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateChiSquared)

//----------------------------------------------------------------------------------------------
namespace {

/// Caclculate the chi squared, weighted chi squared and the number of degrees
/// of freedom.
/// @param domain :: Function's domain.
/// @param nParams :: Number of free fitting parameters.
/// @param values :: Functin's values.
/// @param chi0 :: Chi squared at the minimum.
/// @param sigma2 :: Estimated variance of the fitted data.
void calcChiSquared(const API::IFunction &fun, size_t nParams,
                    const API::FunctionDomain &domain,
                    API::FunctionValues &values, double &chiSquared,
                    double &chiSquaredWeighted, double &dof) {

  // Calculate function values.
  fun.function(domain, values);

  // Calculate the chi squared.
  chiSquared = 0.0;
  chiSquaredWeighted = 0.0;
  dof = -static_cast<double>(nParams);
  for (size_t i = 0; i < values.size(); ++i) {
    auto weight = values.getFitWeight(i);
    if (weight > 0.0) {
      double tmp = values.getFitData(i) - values.getCalculated(i);
      chiSquared += tmp * tmp;
      tmp *= weight;
      chiSquaredWeighted += tmp * tmp;
      dof += 1.0;
    }
  }
  if (dof <= 0.0) {
    dof = 1.0;
  }
}
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateChiSquared::name() const {
  return "CalculateChiSquared";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculateChiSquared::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateChiSquared::summary() const {
  return "Calculate chi squared for a function and a data set in a workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void CalculateChiSquared::initConcrete() {
  declareProperty("ChiSquared", 0.0, "Output value of chi squared.",
                  Direction::Output);
  declareProperty("ChiSquaredDividedByDOF", 0.0,
                  "Output value of chi squared divided by the "
                  "number of degrees of freedom (NofData "
                  "- nOfParams).",
                  Direction::Output);
  declareProperty("ChiSquaredDividedByNData", 0.0,
                  "Output value of chi squared divided by the "
                  "number of data points).",
                  Direction::Output);
  declareProperty("ChiSquaredWeighted", 0.0,
                  "Output value of weighted chi squared.", Direction::Output);
  declareProperty("ChiSquaredWeightedDividedByDOF", 0.0,
                  "Output value of weighted chi squared divided by the "
                  "number of degrees of freedom (NofData "
                  "- nOfParams).",
                  Direction::Output);
  declareProperty("ChiSquaredWeightedDividedByNData", 0.0,
                  "Output value of weighted chi squared divided by the "
                  "number of  data points).",
                  Direction::Output);
  declareProperty("Output", "", "A base name for output workspaces.");
  declareProperty("Weighted", false,
                  "Option to use the weighted chi squared "
                  "in error estimation. Default is false.");
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void CalculateChiSquared::execConcrete() {

  // Function may need some preparation.
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  m_domainCreator->createDomain(domain, values);

  // Do something with the function which may depend on workspace.
  m_domainCreator->initFunction(m_function);

  // Get the number of free fitting parameters
  size_t nParams = 0;
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (m_function->isActive(i))
      nParams += 1;
  }

  // Calculate function values.
  m_function->function(*domain, *values);

  // Calculate the chi squared.
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  double dof = 0.0;
  calcChiSquared(*m_function, nParams, *domain, *values, chiSquared,
                 chiSquaredWeighted, dof);
  g_log.notice() << "Chi squared " << chiSquared << "\n"
                 << "Chi squared weighted " << chiSquaredWeighted << "\n";

  // Store the result.
  setProperty("ChiSquared", chiSquared);
  setProperty("chiSquaredWeighted", chiSquaredWeighted);

  // Divided by NParams
  double nData = dof + static_cast<double>(nParams);
  const double chiSquaredNData = chiSquared / nData;
  const double chiSquaredWeightedNData = chiSquaredWeighted / nData;
  g_log.notice() << "Chi squared / NData " << chiSquaredNData << "\n"
                 << "Chi squared weighed / NData " << chiSquaredWeightedNData
                 << "\n"
                 << "NParams " << nParams << "\n";

  // Store the result.
  setProperty("ChiSquaredDividedByNData", chiSquaredNData);
  setProperty("ChiSquaredWeightedDividedByNData", chiSquaredWeightedNData);

  // Divided by the DOF
  if (dof <= 0.0) {
    dof = 1.0;
    g_log.warning("DOF has a non-positive value, changing to 1.0");
  }
  const double chiSquaredDOF = chiSquared / dof;
  const double chiSquaredWeightedDOF = chiSquaredWeighted / dof;
  g_log.notice() << "Chi squared / DOF " << chiSquaredDOF << "\n"
                 << "Chi squared weighed / DOF " << chiSquaredWeightedDOF
                 << "\n"
                 << "DOF " << dof << "\n";

  // Store the result.
  setProperty("ChiSquaredDividedByDOF", chiSquaredDOF);
  setProperty("ChiSquaredWeightedDividedByDOF", chiSquaredWeightedDOF);

  std::string baseName = getProperty("Output");
  if (!baseName.empty()) {
    estimateErrors();
  }
}

//----------------------------------------------------------------------------------------------
namespace {

/// Calculate the negative logarithm of the probability density function (PDF):
/// if a = getDiff(...) then exp(-a) is a value of the PDF.
/// @param domain :: Function's domain.
/// @param nParams :: Number of free fitting parameters.
/// @param values :: Functin's values.
/// @param chi0 :: Chi squared at the minimum.
/// @param sigma2 :: Estimated variance of the fitted data.
double getDiff(const API::IFunction &fun, size_t nParams,
               const API::FunctionDomain &domain, API::FunctionValues &values,
               double chi0, double sigma2) {
  double chiSquared = 0.0;
  double chiSquaredWeighted = 0.0;
  double dof = 0;
  calcChiSquared(fun, nParams, domain, values, chiSquared, chiSquaredWeighted,
                 dof);
  double res = 0.0;
  if (sigma2 > 0) {
    res = (chiSquared - chi0) / 2 / sigma2;
  } else {
    res = (chiSquaredWeighted - chi0) / 2;
  }
  return res;
}

/// Helper class to calculate the chi squared along a direction in the parameter
/// space.
class ChiSlice {
public:
  /// Constructor.
  /// @param f :: The fitting function
  /// @param dir :: A normalised direction vector in the parameter space.
  /// @param domain :: Function's domain.
  /// @param values :: Functin's values.
  /// @param chi0 :: Chi squared at the minimum.
  /// @param sigma2 :: Estimated variance of the fitted data.
  ChiSlice(IFunction &f, const GSLVector &dir,
           const API::FunctionDomain &domain, API::FunctionValues &values,
           double chi0, double sigma2)
      : m_function(f), m_direction(dir), m_domain(domain), m_values(values),
        m_chi0(chi0), m_sigma2(sigma2) {}
  /// Calculate the value of chi squared along the chosen direction at a
  /// distance from
  /// the minimum point.
  /// @param p :: A distance from the minimum.
  double operator()(double p) {
    std::vector<double> par0(m_function.nParams());
    for (size_t ip = 0; ip < m_function.nParams(); ++ip) {
      par0[ip] = m_function.getParameter(ip);
      m_function.setParameter(ip, par0[ip] + p * m_direction[ip]);
    }
    double res = getDiff(m_function, m_function.nParams(), m_domain, m_values,
                         m_chi0, m_sigma2);
    for (size_t ip = 0; ip < m_function.nParams(); ++ip) {
      m_function.setParameter(ip, par0[ip]);
    }
    return res;
  }

  /// Make an approximation for this slice on an interval.
  /// @param lBound :: The left bound of the approximation interval.
  /// @param rBound :: The right bound of the approximation interval.
  /// @param P :: Output vector with approximation parameters.
  /// @param A :: Output vector with approximation parameters.
  ChebfunBase_sptr makeApprox(double lBound, double rBound,
                              std::vector<double> &P, std::vector<double> &A,
                              bool &ok) {

    auto base = ChebfunBase::bestFitAnyTolerance(lBound, rBound, *this, P, A,
                                                 1.0, 1e-4, 129);
    ok = bool(base);
    if (!base) {
      base = boost::make_shared<ChebfunBase>(10, lBound, rBound, 1e-4);
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

      if (diff > 3.0) {
        if (diff < 4.0) {
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
  /// The fitting function
  IFunction &m_function;
  /// The direction in the parameter space
  GSLVector m_direction;
  /// The domain
  const API::FunctionDomain &m_domain;
  /// The values
  API::FunctionValues &m_values;
  /// The chi squared at the minimum
  double m_chi0;
  /// The data variance.
  double m_sigma2;
};
} // namespace

//----------------------------------------------------------------------------------------------
/// Examine the chi squared as a function of fitting parameters and estimate
/// errors for each parameter.
void CalculateChiSquared::estimateErrors() {
  // Number of fiting parameters
  auto nParams = m_function->nParams();
  // Create an output table for displaying slices of the chi squared and
  // the probabilitydensity function
  auto pdfTable = API::WorkspaceFactory::Instance().createTable();

  std::string baseName = getProperty("Output");
  if (baseName.empty()) {
    baseName = "CalculateChiSquared";
  }
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "PDFs", "", Kernel::Direction::Output),
      "The name of the TableWorkspace in which to store the "
      "pdfs of fit parameters");
  setPropertyValue("PDFs", baseName + "_pdf");
  setProperty("PDFs", pdfTable);

  // Create an output table for displaying the parameter errors.
  auto errorsTable = API::WorkspaceFactory::Instance().createTable();
  auto nameColumn = errorsTable->addColumn("str", "Parameter");
  auto valueColumn = errorsTable->addColumn("double", "Value");
  auto minValueColumn = errorsTable->addColumn("double", "Value at Min");
  auto leftErrColumn = errorsTable->addColumn("double", "Left Error");
  auto rightErrColumn = errorsTable->addColumn("double", "Right Error");
  auto quadraticErrColumn = errorsTable->addColumn("double", "Quadratic Error");
  auto chiMinColumn = errorsTable->addColumn("double", "Chi2 Min");
  errorsTable->setRowCount(nParams);
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "Errors", "", Kernel::Direction::Output),
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
  calcChiSquared(*m_function, nParams, *domain, *values, chiSquared,
                 chiSquaredWeighted, dof);
  // Value of chi squared for current parameters in m_function
  double chi0 = chiSquared;
  // Fit data variance
  double sigma2 = chiSquared / dof;
  bool useWeighted = getProperty("Weighted");

  if (useWeighted) {
    chi0 = chiSquaredWeighted;
    sigma2 = 0.0;
  }

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "chi0=" << chi0 << '\n';
    g_log.debug() << "sigma2=" << sigma2 << '\n';
    g_log.debug() << "dof=" << dof << '\n';
  }

  // Parameter bounds that define a volume in the parameter
  // space within which the chi squared is being examined.
  GSLVector lBounds(nParams);
  GSLVector rBounds(nParams);

  // Number of points in lines for plotting
  size_t n = 100;
  pdfTable->setRowCount(n);
  const double fac = 1e-4;

  // Loop over each parameter
  for (size_t ip = 0; ip < nParams; ++ip) {

    // Add columns for the parameter to the pdf table.
    auto parName = m_function->parameterName(ip);
    nameColumn->read(ip, parName);
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
    GSLVector dir(nParams);
    dir.zero();
    dir[ip] = 1.0;
    ChiSlice slice(*m_function, dir, *domain, *values, chi0, sigma2);

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
    bool ok = true;
    auto base = slice.makeApprox(lBound, rBound, P, A, ok);
    if (!ok) {
      g_log.warning() << "Approximation failed for parameter " << ip << '\n';
    }
    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Parameter " << ip << '\n';
      g_log.debug() << "Slice approximated by polynomial of order "
                    << base->size() - 1;
      g_log.debug() << " between " << lBound << " and " << rBound << '\n';
    }

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
      minima.push_back(par0);
    }

    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Minima: ";
    }

    // If only 1 extremum is found assume (without checking) that it's a
    // minimum.
    // If there are more than 1, find the one with the smallest chi^2.
    double chiMin = std::numeric_limits<double>::max();
    double parMin = par0;
    for (double minimum : minima) {
      double value = base->eval(minimum, P);
      if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
        g_log.debug() << minimum << " (" << value << ") ";
      }
      if (value < chiMin) {
        chiMin = value;
        parMin = minimum;
      }
    }
    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << '\n';
      g_log.debug() << "Smallest minimum at " << parMin << " is " << chiMin
                    << '\n';
    }

    // Points of intersections with line chi^2 = 1/2 give an estimate of
    // the standard deviation of this parameter if it's uncorrelated with the
    // others.
    A[0] -= 0.5; // Now A are the coefficients of the original polynomial
                 // shifted down by 1/2.
    std::vector<double> roots = base->roots(A);
    std::sort(roots.begin(), roots.end());

    if (roots.empty()) {
      // Something went wrong; use the whole interval.
      roots.resize(2);
      roots[0] = lBound;
      roots[1] = rBound;
    } else if (roots.size() == 1) {
      // Only one root found; use a bound for the other root.
      if (roots.front() < 0) {
        roots.push_back(rBound);
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

    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Roots: ";
      for (double root : roots) {
        g_log.debug() << root << ' ';
      }
      g_log.debug() << '\n';
    }

    // Output parameter info to the table.
    valueColumn->fromDouble(ip, par0);
    minValueColumn->fromDouble(ip, par0 + parMin);
    leftErrColumn->fromDouble(ip, roots[0] - parMin);
    rightErrColumn->fromDouble(ip, roots[1] - parMin);
    chiMinColumn->fromDouble(ip, chiMin);

    // Output the PDF
    for (size_t i = 0; i < n; ++i) {
      double chi = col2->toDouble(i);
      col3->fromDouble(i, exp(-chi + chiMin));
    }

    // make sure function parameters don't change.
    m_function->setParameter(ip, par0);
  }

  // Improve estimates for standard deviations.
  // If parameters are correlated the found deviations
  // most likely underestimate the true values.
  unfixParameters();
  GSLJacobian J(*m_function, values->size());
  m_function->functionDeriv(*domain, J);
  refixParameters();
  // Calculate the hessian at the current point.
  GSLMatrix H;
  if (useWeighted) {
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
  } else {
    H = J.matrix().tr() * J.matrix();
  }
  // Square roots of the diagonals of the covariance matrix give
  // the standard deviations in the quadratic approximation of the chi^2.
  GSLMatrix V(H);
  if (!useWeighted) {
    V *= 1. / sigma2;
  }
  V.invert();
  // In a non-quadratic asymmetric case the following procedure can give a
  // better result:
  // Find the direction in which the chi^2 changes slowest and the positive and
  // negative deviations in that direction. The change in a parameter at those
  // points can be a better estimate for the standard deviation.
  GSLVector v(nParams);
  GSLMatrix Q(nParams, nParams);
  // One of the eigenvectors of the hessian is the direction of the slowest
  // change.
  H.eigenSystem(v, Q);

  // Loop over the eigenvectors
  for (size_t i = 0; i < nParams; ++i) {
    auto dir = Q.copyColumn(i);
    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Direction " << i << '\n';
      g_log.debug() << dir << '\n';
    }
    // Make a slice in that direction
    ChiSlice slice(*m_function, dir, *domain, *values, chi0, sigma2);
    double rBound0 = dir.dot(rBounds);
    double lBound0 = dir.dot(lBounds);
    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "lBound " << lBound0 << '\n';
      g_log.debug() << "rBound " << rBound0 << '\n';
    }
    double lBound = slice.findBound(lBound0);
    double rBound = slice.findBound(rBound0);
    std::vector<double> P, A;
    // Use a polynomial approximation
    bool ok = true;
    auto base = slice.makeApprox(lBound, rBound, P, A, ok);
    if (!ok) {
      g_log.warning() << "Approximation failed in direction " << i << '\n';
    }
    // Find the deviation points where the chi^2 = 1/2
    A[0] -= 0.5;
    std::vector<double> roots = base->roots(A);
    std::sort(roots.begin(), roots.end());
    // Sort out the roots
    auto nRoots = roots.size();
    if (nRoots == 0) {
      roots.resize(2, 0.0);
    } else if (nRoots == 1) {
      if (roots.front() > 0.0) {
        roots.insert(roots.begin(), 0.0);
      } else {
        roots.push_back(0.0);
      }
    } else if (nRoots > 2) {
      roots[1] = roots.back();
      roots.resize(2);
    }
    if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Roots " << roots[0] << " (" << slice(roots[0]) << ") "
                    << roots[1] << " (" << slice(roots[1]) << ") \n";
    }
    // Loop over the parameters and see if there deviations along
    // this direction is greater than any previous value.
    for (size_t ip = 0; ip < nParams; ++ip) {
      auto lError = roots.front() * dir[ip];
      auto rError = roots.back() * dir[ip];
      if (lError > rError) {
        std::swap(lError, rError);
      }
      if (lError < leftErrColumn->toDouble(ip)) {
        if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
          g_log.debug() << "  left for  " << ip << ' ' << lError << ' '
                        << leftErrColumn->toDouble(ip) << '\n';
        }
        leftErrColumn->fromDouble(ip, lError);
      }
      if (rError > rightErrColumn->toDouble(ip)) {
        if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
          g_log.debug() << "  right for " << ip << ' ' << rError << ' '
                        << rightErrColumn->toDouble(ip) << '\n';
        }
        rightErrColumn->fromDouble(ip, rError);
      }
    }
    // Output the quadratic estimate for comparrison.
    quadraticErrColumn->fromDouble(i, sqrt(V.get(i, i)));
  }
}

/// Temporary unfix any fixed parameters.
void CalculateChiSquared::unfixParameters() {
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (!m_function->isActive(i)) {
      m_function->unfix(i);
      m_fixedParameters.push_back(i);
    }
  }
}

/// Restore the "fixed" status of previously unfixed paramters.
void CalculateChiSquared::refixParameters() {
  for (auto &fixedParameter : m_fixedParameters) {
    m_function->fix(fixedParameter);
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
