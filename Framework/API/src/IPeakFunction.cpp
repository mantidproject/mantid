// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IFunction1D.tcc"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/PeakFunctionIntegrator.h"
#include "MantidKernel/Exception.h"
#include "boost/make_shared.hpp"
#include <boost/math/distributions/students_t.hpp>

#include <cmath>
#include <limits>
#include <memory>

namespace Mantid {
namespace API {

namespace {

/** A Jacobian for individual functions
 */
class PartialJacobian1 : public Jacobian {
  Jacobian *m_J; ///< pointer to the overall Jacobian
  int m_iY0;     ///< offset in the overall Jacobian for a particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iY0 :: The data offset for a particular function
   */
  PartialJacobian1(Jacobian *J, int iY0) : m_J(J), m_iY0(iY0) {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(size_t iY, size_t iP, double value) override { m_J->set(m_iY0 + iY, iP, value); }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(size_t iY, size_t iP) override { return m_J->get(m_iY0 + iY, iP); }
  /** Zero all matrix elements.
   */
  void zero() override {
    throw Kernel::Exception::NotImplementedError("zero() is not implemented for PartialJacobian1");
  }
};

class TempJacobian : public Jacobian {
public:
  TempJacobian(size_t y, size_t p) : m_y(y), m_p(p), m_J(y * p) {}
  void set(size_t iY, size_t iP, double value) override { m_J[iY * m_p + iP] = value; }
  double get(size_t iY, size_t iP) override { return m_J[iY * m_p + iP]; }
  size_t maxParam(size_t iY) {
    double max = -DBL_MAX;
    size_t maxIndex = 0;
    for (size_t i = 0; i < m_p; ++i) {
      double current = get(iY, i);
      if (current > max) {
        maxIndex = i;
        max = current;
      }
    }

    return maxIndex;
  }
  void zero() override { m_J.assign(m_J.size(), 0.0); }

protected:
  size_t m_y;
  size_t m_p;
  std::vector<double> m_J;
};

/// Tolerance for determining the smallest significant value on the peak
const double PEAK_TOLERANCE = 1e-14;
/// "Infinite" value for the peak radius
const int MAX_PEAK_RADIUS = std::numeric_limits<int>::max();

} // namespace

/**
An implementation of Jacobian using std::vector.

@author Roman Tolchenov
@date 17/02/2012
*/
class CurveFittingJacobian : public API::Jacobian {
  /// Number of data points
  size_t m_ny;
  /// Number of parameters in a function (== IFunction::nParams())
  size_t m_np;
  /// Storage for the derivatives
  std::vector<double> m_data;

public:
  /// Constructor.
  /// @param ny :: Number of data points
  /// @param np :: Number of parameters
  CurveFittingJacobian(size_t ny, size_t np) : m_ny(ny), m_np(np) { m_data.resize(ny * np, 0.0); }
  /// overwrite base method
  /// @param value :: the value
  /// @param iP :: the index of the parameter
  ///  @throw runtime_error Thrown if column of Jacobian to add number to does
  ///  not exist
  void addNumberToColumn(const double &value, const size_t &iP) override {
    if (iP < m_np) {
      // add penalty to first and last point and every 10th point in between
      m_data[iP] += value;
      m_data[(m_ny - 1) * m_np + iP] += value;
      for (size_t iY = 9; iY < m_ny; iY += 10)
        m_data[iY * m_np + iP] += value;
    } else {
      throw std::runtime_error("Try to add number to column of Jacobian matrix "
                               "which does not exist.");
    }
  }
  /// overwrite base method
  void set(size_t iY, size_t iP, double value) override {
    if (iY >= m_ny) {
      throw std::out_of_range("Data index in Jacobian is out of range");
    }
    if (iP >= m_np) {
      throw Kernel::Exception::FitSizeWarning(m_np);
    }
    m_data[iY * m_np + iP] = value;
  }
  /// overwrite base method
  double get(size_t iY, size_t iP) override {
    if (iY >= m_ny) {
      throw std::out_of_range("Data index in Jacobian is out of range");
    }
    if (iP >= m_np) {
      throw Kernel::Exception::FitSizeWarning(m_np);
    }
    return m_data[iY * m_np + iP];
  }
  /// overwrite base method
  void zero() override { m_data.assign(m_data.size(), 0.0); }
};

/**
 * Constructor.
 */
IPeakFunction::IPeakFunction() : m_peakRadius(MAX_PEAK_RADIUS) {}

void IPeakFunction::function(const FunctionDomain &domain, FunctionValues &values) const {
  auto peakRadius = dynamic_cast<const FunctionDomain1D &>(domain).getPeakRadius();
  setPeakRadius(peakRadius);
  IFunction1D::function(domain, values);
}

/**
 * General implementation of the method for all peaks. Limits the peak
 * evaluation to
 * a certain number of FWHMs around the peak centre. The outside points are set
 * to 0.
 * Calls functionLocal() to compute the actual values
 * @param out :: Output function values
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IPeakFunction::function1D(double *out, const double *xValues, const size_t nData) const {
  double c = this->centre();
  double dx = fabs(m_peakRadius * this->fwhm());
  int i0 = -1;
  int n = 0;
  for (size_t i = 0; i < nData; ++i) {
    if (fabs(xValues[i] - c) < dx) {
      if (i0 < 0)
        i0 = static_cast<int>(i);
      ++n;
    } else {
      out[i] = 0.0;
    }
  }
  if (i0 < 0 || n == 0)
    return;
  this->functionLocal(out + i0, xValues + i0, n);
}

/**
 * General implementation of the method for all peaks. Calculates derivatives
 * only
 * for a range of x values limited to a certain number of FWHM around the peak
 * centre.
 * For the points outside the range all derivatives are set to 0.
 * Calls functionDerivLocal() to compute the actual values
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IPeakFunction::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  double c = this->centre();
  double dx = fabs(m_peakRadius * this->fwhm());
  int i0 = -1;
  int n = 0;
  for (size_t i = 0; i < nData; ++i) {
    if (fabs(xValues[i] - c) < dx) {
      if (i0 < 0)
        i0 = static_cast<int>(i);
      ++n;
    } else {
      for (size_t ip = 0; ip < this->nParams(); ++ip) {
        out->set(i, ip, 0.0);
      }
    }
  }
  if (i0 < 0 || n == 0)
    return;
  PartialJacobian1 J(out, i0);
  this->functionDerivLocal(&J, xValues + i0, n);
}

void IPeakFunction::setPeakRadius(int r) const {
  if (r > 0) {
    m_peakRadius = r;
  } else if (r == 0) {
    m_peakRadius = MAX_PEAK_RADIUS;
  }
}

void IPeakFunction::setParameter(size_t i, const double &value, bool explicitlySet) {
  m_parameterContextDirty = true;
  ParamFunction::setParameter(i, value, explicitlySet);
}

void IPeakFunction::setParameter(const std::string &name, const double &value, bool explicitlySet) {
  m_parameterContextDirty = true;
  ParamFunction::setParameter(name, value, explicitlySet);
}

// integrate based on dirty parameters then cache the result
IntegrationResultCache IPeakFunction::integrate() const {
  if (!integrationResult || m_parameterContextDirty) {
    auto const interval = getDomainInterval();

    PeakFunctionIntegrator integrator;

    auto const result = integrator.integrate(*this, interval.first, interval.second);
    if (result.success)
      integrationResult = boost::make_shared<IntegrationResultCache>(result.result, result.error);
    else
      integrationResult = boost::make_shared<IntegrationResultCache>(std::nan(""), std::nan(""));
    m_parameterContextDirty = false;
  }
  return *integrationResult;
}

/// Returns the integral intensity of the peak function, using the peak radius
/// to determine integration borders.
double IPeakFunction::intensity() const { return integrate().first; }

/// Returns the uncertainty associated to the integral intensity of the peak function
double IPeakFunction::intensityError() {

  auto const interval = getDomainInterval();
  const size_t nData = 2;
  double *domainArray = new double[2];
  domainArray[0] = interval.first;
  domainArray[1] = interval.second;

  FunctionDomain1DView domain(domainArray, nData);
  FunctionValues values(domain);
  values->zeroCalucated();

  double sigma = 1;
  double prob = std::erf(sigma / sqrt(2));
  // critical value for t distribution
  double alpha = (1 + prob) / 2;
  double eValue = std::nan("");

  bool hasErrors = false;

  function(domain, values);
  auto covar = getCovarianceMatrix();

  size_t nParams = this->nParams();

  CurveFittingJacobian J(nData, this->nParams());

  if (!covar) {
    for (size_t j = 0; j < nParams; ++j) {
      // throw std::runtime_error(fn->parameterName(j+3));
      if (getError(j) != 0.0) {
        hasErrors = true;
        break;
      }
    }
  }

  if (covar || hasErrors) {

    try {
      functionDeriv(domain, J);
    } catch (...) {
      calNumericalDeriv(domain, J);
    }

    if (covar) {
      double s = 0.0;
      const Kernel::Matrix<double> &C = *covar;
      for (size_t i = 0; i < nParams; ++i) {
        double tmp = J.get(0, i);
        s += C[i][i] * tmp * tmp;
        for (size_t j = i + 1; j < nParams; ++j) {
          s += J.get(0, i) * C[i][j] * J.get(0, j) * 2;
        }
      }

      size_t dof = 1 - nParams;
      double T = 1.0;
      if (dof != 0) {
        boost::math::students_t dist(static_cast<double>(dof));
        T = boost::math::quantile(dist, alpha);
      }

      eValue = T * std::sqrt(s * getReducedChiSquared());
    } else {
      double err = 0.0;
      for (size_t i = 0; i < nData; i++) {
        for (size_t j = 0; j < nParams; ++j) {
          double d = J.get(i, j) * getError(j);
          err += d * d;
        }
      }
      if (err != 0.0)
        eValue = std::sqrt(err);
    }
  }
  return eValue;
}

/// Sets the integral intensity of the peak by adjusting the height.
void IPeakFunction::setIntensity(const double newIntensity) {
  double currentHeight = height();
  double currentIntensity = intensity();

  if (currentIntensity == 0.0) {
    // Try to set a different height first.
    setHeight(2.0);

    currentHeight = height();
    currentIntensity = intensity();

    // If the current intensity is still 0, there's nothing left to do.
    if (currentIntensity == 0.0) {
      throw std::invalid_argument("Cannot set new intensity, not enough information available.");
    }
  }

  setHeight(newIntensity / currentIntensity * currentHeight);
}

std::string IPeakFunction::getCentreParameterName() const {
  FunctionParameterDecorator_sptr fn = std::dynamic_pointer_cast<FunctionParameterDecorator>(
      FunctionFactory::Instance().createFunction("PeakParameterFunction"));

  if (!fn) {
    throw std::runtime_error("PeakParameterFunction could not be created successfully.");
  }

  fn->setDecoratedFunction(this->name());

  FunctionDomain1DVector domain(std::vector<double>(4, 0.0));
  TempJacobian jacobian(4, fn->nParams());

  fn->functionDeriv(domain, jacobian);

  return parameterName(jacobian.maxParam(0));
}

/// Get the interval on which the peak has all its values above a certain
/// level. All values outside the interval are below that level.
/// @param level :: A fraction of the peak height.
/// @return A pair of doubles giving the bounds of the interval.
std::pair<double, double> IPeakFunction::getDomainInterval(double level) const {
  if (level < PEAK_TOLERANCE) {
    level = PEAK_TOLERANCE;
  }
  double left = 0.0;
  double right = 0.0;
  auto h = height();
  auto w = fwhm();
  auto c = centre();
  if (h == 0.0 || w == 0.0 || level >= 1.0) {
    return std::make_pair(c, c);
  }

  auto findBound = [this, c, h, level](double dx) {
    for (size_t i = 0; i < 100; ++i) {
      double x = c + dx;
      double y = 0.0;
      this->functionLocal(&y, &x, 1);
      if (fabs(y / h) < level) {
        return x;
      }
      dx *= 2;
    }
    return c + dx;
  };

  left = findBound(-w);
  right = findBound(w);
  return std::make_pair(left, right);
}

/**
 * Computes the function derivative numerically
 * @param jacobian An output Jacobian to receive the calculated values
 * @param xValues An input array of X data
 * @param nData The number of X values provided
 */
void IPeakFunction::functionDerivLocal(Jacobian *jacobian, const double *xValues, const size_t nData) {
  auto evalMethod = [this](double *out, const double *xValues, const size_t nData) {
    this->functionLocal(out, xValues, nData);
  };
  this->calcNumericalDerivative1D(jacobian, std::move(evalMethod), xValues, nData);
}

} // namespace API
} // namespace Mantid
