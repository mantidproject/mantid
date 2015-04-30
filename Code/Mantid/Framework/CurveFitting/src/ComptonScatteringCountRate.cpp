#include "MantidCurveFitting/ComptonScatteringCountRate.h"
#include "MantidCurveFitting/AugmentedLagrangianOptimizer.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Math/Optimization/SLSQPMinimizer.h"

#include <boost/bind.hpp>

#include <sstream>

namespace Mantid {
namespace CurveFitting {
using Kernel::Logger;

namespace {
/// Name of the intensity constraint matrix attribute
const char *CONSTRAINT_MATRIX_NAME = "IntensityConstraints";
/// Attribute to specify the name of the background order. Defaults to "n"
const char *BKGD_ORDER_ATTR_NAME = "BackgroundOrderAttr";
/// static logger
Logger g_log("ComptonScatteringCountRate");
}

DECLARE_FUNCTION(ComptonScatteringCountRate)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ComptonScatteringCountRate::ComptonScatteringCountRate()
    : CompositeFunction(), m_profiles(), m_fixedParamIndices(), m_cmatrix(),
      m_eqMatrix(), m_bkgdOrderAttr("n"), m_bkgdPolyN(0), m_errors(),
      m_dataErrorRatio() {
  // Must be a string to be able to be passed through Fit
  declareAttribute(CONSTRAINT_MATRIX_NAME, IFunction::Attribute("", true));
  declareAttribute(BKGD_ORDER_ATTR_NAME, IFunction::Attribute(m_bkgdOrderAttr));
}

//----------------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------------

/**
 * @param name The name of the attribute that is being set
 * @param value The attribute's value
 */
void ComptonScatteringCountRate::setAttribute(
    const std::string &name, const API::IFunction::Attribute &value) {
  CompositeFunction::setAttribute(name, value);
  if (name == CONSTRAINT_MATRIX_NAME)
    parseIntensityConstraintMatrix(value.asUnquotedString());
  else if (name == BKGD_ORDER_ATTR_NAME)
    m_bkgdOrderAttr = value.asString();
}

/**
 * @param value The string is assumed to be of the form "[0 1 0][1 1 0]..."
 * where
 * each [] pair specifies the row and the number of columns is given by the
 * number of
 * entries within the brackets. The number of columns must ultimately match the
 * total number of intensities within the problem
 */
void ComptonScatteringCountRate::parseIntensityConstraintMatrix(
    const std::string &value) {
  if (value.empty())
    throw std::invalid_argument(
        "ComptonScatteringCountRate - Empty string not allowed.");
  std::istringstream is(value);
  Mantid::Kernel::fillFromStream(is, m_eqMatrix, '|');
}

namespace {
/// Struct to compute norm2,0.5*||Cx-d||^2, when the background is NOT included
/// and the
/// optimizer expects constraints specified as >= 0
struct NoBkgdNorm2 {
  /// Compute the value of the objective function
  NoBkgdNorm2(const Kernel::DblMatrix &cmatrix, const std::vector<double> &data)
      : cm(cmatrix), nrows(cmatrix.numRows()), ncols(cmatrix.numCols()),
        rhs(data) {}

  double eval(const std::vector<double> &xpt) const {
    double norm2(0.0);
    for (size_t i = 0; i < nrows; ++i) {
      const double *cmRow = cm[i];
      double cx(0.0);
      for (size_t j = 0; j < ncols; ++j) {
        cx += cmRow[j] * xpt[j];
      }
      cx -= rhs[i];
      norm2 += cx * cx;
    }
    return 0.5 * norm2;
  }
  const Kernel::DblMatrix &cm;
  size_t nrows;
  size_t ncols;
  const std::vector<double> &rhs;
};
/// Struct to compute norm2 when the background is included and the
/// optimizer expects constraints specified as <= 0.
/// It is assumed the CM matrix has been multiplied by -1
struct BkgdNorm2 {
  /// Compute the value of the objective function
  BkgdNorm2(const Kernel::DblMatrix &cmatrix, const std::vector<double> &data)
      : cm(cmatrix), nrows(cmatrix.numRows()), ncols(cmatrix.numCols()),
        rhs(data) {}

  double eval(const size_t n, const double *xpt) const {
    double norm2(0.0);
    for (size_t i = 0; i < nrows; ++i) {
      const double *cmRow = cm[i];
      double cx(0.0);
      for (size_t j = 0; j < n; ++j) {
        cx += cmRow[j] * xpt[j];
      }
      cx *= -1.0; // our definition of cm has been multiplied by -1
      cx -= rhs[i];
      norm2 += cx * cx;
    }
    return 0.5 * norm2;
  }
  const Kernel::DblMatrix &cm;
  size_t nrows;
  size_t ncols;
  const std::vector<double> &rhs;
};
}

/**
 * Calculates the new values for the intensity coefficents
 */
void ComptonScatteringCountRate::iterationStarting() {
  /**
   * Before calling the functions that make up the composite perform a
   *least-square minimization
   * to compute the values of the intensity coefficients so that they can be
   *taken out of the fit
   * essentially. This also applies the intensity constraints supplied by the
   *user.
   * The required parameters have been fixed by setupForFit
   *
   * It is required that we minimize 0.5*||Cx-d||^2, where C is a matrix of
   *J*(y) values
   * from each intensity coefficient. This minimization is subject to the
   *following constraints:
   *
   *   Cx >= 0, where C is the same matrix of J(y) values above
   *   Ax = 0, where A is the intensity constraints supplied by the user
   */
  const size_t nparams(m_cmatrix.numCols());

  // Compute minimization with of Cx where the amplitudes are set to 1.0
  std::vector<double> x0(nparams, 1);
  setFixedParameterValues(x0);
  // Compute the constraint matrix
  this->updateCMatrixValues();

  if (m_bkgdPolyN > 0) {
    BkgdNorm2 objf(m_cmatrix, m_dataErrorRatio);
    AugmentedLagrangianOptimizer::ObjFunction objfunc =
        boost::bind(&BkgdNorm2::eval, objf, _1, _2);
    AugmentedLagrangianOptimizer lsqmin(nparams, objfunc, m_eqMatrix,
                                        m_cmatrix);
    lsqmin.minimize(x0);
    // Set the parameters for the 'real' function calls
    setFixedParameterValues(x0);
  } else {
    NoBkgdNorm2 objfunc(m_cmatrix, m_dataErrorRatio);
    Kernel::Math::SLSQPMinimizer lsqmin(nparams, objfunc, m_eqMatrix,
                                        m_cmatrix);
    auto res = lsqmin.minimize(x0);
    // Set the parameters for the 'real' function calls
    setFixedParameterValues(res);
  }
}

/**
 * Set the first N fixed parameters to the values given by the N values vector
 * @param values A new set of N values to set
 */
void ComptonScatteringCountRate::setFixedParameterValues(
    const std::vector<double> &values) {
  assert(values.size() == m_fixedParamIndices.size());

  const size_t nparams = values.size();
  for (size_t i = 0; i < nparams; ++i) {
    this->setParameter(m_fixedParamIndices[i], values[i], true);
  }

  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "--- New Intensity Parameters ---\n";
    for (size_t i = 0; i < nparams; ++i) {
      g_log.debug() << "x_" << i << "=" << values[i] << "\n";
    }
  }
}

/**
 * Fills the pre-allocated C matrix with the appropriate values
 */
void ComptonScatteringCountRate::updateCMatrixValues() const {
  // -- Compute constraint matrix from each "member function" --
  const size_t nprofiles = m_profiles.size();
  for (size_t i = 0, start = 0; i < nprofiles; ++i) {
    auto *profile = m_profiles[i];
    const size_t numFilled =
        profile->fillConstraintMatrix(m_cmatrix, start, m_errors);
    start += numFilled;
  }
  // Using different minimizer for background constraints as the original is not
  // stable
  // The background one requires the contraints are specified as <= 0
  if (m_bkgdPolyN > 0) {
    m_cmatrix *= -1.0;
  }

  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "--- CM ---\n";
    for (size_t i = 0; i < m_cmatrix.numRows(); ++i) {
      for (size_t j = 0; j < m_cmatrix.numCols(); ++j) {
        g_log.debug() << m_cmatrix[i][j] << "  ";
      }
      g_log.debug() << "\n";
    }
  }
}

/**
 * Cache workspace reference. Expects a MatrixWorkspace
 * @param matrix A shared_ptr to a Workspace
 * @param wsIndex A workspace index
 * @param startX Starting x-vaue (unused).
 * @param endX Ending x-vaue (unused).
 */
void ComptonScatteringCountRate::setMatrixWorkspace(
    boost::shared_ptr<const API::MatrixWorkspace> matrix, size_t wsIndex,
    double startX, double endX) {
  CompositeFunction::setMatrixWorkspace(matrix, wsIndex, startX, endX);

  const auto &values = matrix->readY(wsIndex);

  // Keep the errors for the constraint calculation
  m_errors = matrix->readE(wsIndex);
  m_dataErrorRatio.resize(m_errors.size());
  std::transform(values.begin(), values.end(), m_errors.begin(),
                 m_dataErrorRatio.begin(), std::divides<double>());

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "-- data/error --\n";
    for (size_t i = 0; i < m_errors.size(); ++i) {
      g_log.debug() << m_dataErrorRatio[i] << "\n";
    }
  }

  cacheFunctions();
  createConstraintMatrices(matrix->readX(wsIndex));
}

/*
 * Casts the pointers to ComptonProfiles to avoid overhead during fit. Also
 * stores the indices, within the composite, of all of the intensity parameters
 * and
 * background parameters
 */
void ComptonScatteringCountRate::cacheFunctions() {
  // Cache ptrs cast to ComptonProfile functions to that we can compute the
  // constraint matrix
  const size_t nfuncs = this->nFunctions();
  m_profiles.reserve(nfuncs);              // won't include background
  m_fixedParamIndices.reserve(nfuncs + 3); // guess at max size
  bool foundBkgd(false);

  for (size_t i = 0; i < nfuncs; ++i) {
    auto func = this->getFunction(i);
    const size_t paramsOffset =
        this->paramOffset(i); // offset for ith function inside composite

    if (auto profile = boost::dynamic_pointer_cast<ComptonProfile>(func)) {
      this->cacheComptonProfile(profile, paramsOffset);
      continue;
    }

    auto function1D = boost::dynamic_pointer_cast<API::IFunction1D>(func);
    if (!foundBkgd) {
      this->cacheBackground(function1D, paramsOffset);
      foundBkgd = true;
    } else {
      std::ostringstream os;
      os << "ComptonScatteringCountRate - Invalid function member at index '"
         << i << "'. "
         << "All members must be of type ComptonProfile and at most a single "
            "1D function";
      throw std::runtime_error(os.str());
    }
  }
}

/**
 * @param profile Function of type ComptonProfile
 * @param paramsOffset The offset of the given function's parameters within
 * composite
 */
void ComptonScatteringCountRate::cacheComptonProfile(
    const boost::shared_ptr<ComptonProfile> &profile,
    const size_t paramsOffset) {
  m_profiles.push_back(profile.get());
  auto fixedParams = profile->intensityParameterIndices();
  for (size_t j = 0; j < fixedParams.size(); ++j) {
    const size_t indexOfFixed = paramsOffset + fixedParams[j];
    this->fix(indexOfFixed);
    m_fixedParamIndices.push_back(indexOfFixed);
  }
}

/**
 * @param function1D Function of type IFunction1D
 * @param paramsOffset The offset of the given function's parameters within
 * composite
 */
void ComptonScatteringCountRate::cacheBackground(
    const API::IFunction1D_sptr &function1D, const size_t paramsOffset) {
  // Check for the order attribute
  if (function1D->hasAttribute(m_bkgdOrderAttr)) {
    m_bkgdPolyN = function1D->getAttribute(m_bkgdOrderAttr).asInt();
    const size_t npars =
        static_cast<size_t>(m_bkgdPolyN + 1); // + constant term
    // we assume the parameters are at index 0->N on the background so we need
    // to
    // reverse them
    for (size_t i = npars; i > 0; --i) // i = from npars->1
    {
      const size_t indexOfFixed = paramsOffset + (i - 1);
      this->fix(indexOfFixed);
      m_fixedParamIndices.push_back(indexOfFixed);
    }
  } else {
    std::ostringstream os;
    os << "ComptonScatteringCountRate - Background function does not have "
          "attribute named '" << m_bkgdOrderAttr
       << "' that specifies its order. Use the '" << BKGD_ORDER_ATTR_NAME
       << "' attribute to specify the name of the order attribute.";
    throw std::runtime_error(os.str());
  }
}

/*
 * The equality constraint matrix is padded out to allow for any
 * additional intensity constraints required each specific Compton profile.
 * Also creates the inequality matrix
 * @param xValues The xdata from the workspace
 */
void
ComptonScatteringCountRate::createConstraintMatrices(const MantidVec &xValues) {
  const size_t nmasses = m_profiles.size();

  // Sanity check that equality constraints matrix has the same number of
  // columns as masses or is zero-sized
  if (m_eqMatrix.numCols() > 0 && m_eqMatrix.numCols() != nmasses) {
    std::ostringstream os;
    os << "ComptonScatteringCountRate - Equality constraint matrix (Aeq) has "
          "incorrect number of columns (" << m_eqMatrix.numCols()
       << "). The number of columns should match the number of masses ("
       << nmasses << ")";
    throw std::invalid_argument(os.str());
  }

  createPositivityCM(xValues);
  createEqualityCM(nmasses);

  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "\n--- aeq ---\n";
    for (size_t i = 0; i < m_eqMatrix.numRows(); ++i) {
      for (size_t j = 0; j < m_eqMatrix.numCols(); ++j) {
        g_log.debug() << m_eqMatrix[i][j] << "  ";
      }
      g_log.debug() << "\n";
    }
  }
}

/**
 * @param xValues The X data for the fitted spectrum
 */
void ComptonScatteringCountRate::createPositivityCM(const MantidVec &xValues) {
  // -- Constraint matrix for J(y) > 0 --
  // The first N columns are filled with J(y) for each mass + N_h for the first
  // mass hermite
  // terms included + (n+1) for each termn the background of order n
  // The background columns are filled with x^j/error where j=(1,n+1)
  const size_t nrows(xValues.size());
  size_t nColsCMatrix(m_fixedParamIndices.size());
  m_cmatrix = Kernel::DblMatrix(nrows, nColsCMatrix);

  // Fill background values as they don't change at all
  if (m_bkgdPolyN > 0) {
    size_t polyStartCol = nColsCMatrix - m_bkgdPolyN - 1;
    for (size_t i = 0; i < nrows; ++i) // rows
    {
      double *row = m_cmatrix[i];
      const double &xi = xValues[i];
      const double &erri = m_errors[i];
      size_t polyN = m_bkgdPolyN;
      for (size_t j = polyStartCol; j < nColsCMatrix; ++j) // cols
      {
        row[j] = std::pow(xi, static_cast<double>(polyN)) / erri;
        --polyN;
      }
    }
  }
}

/**
 * @param nmasses The number of distinct masses being fitted
 */
void ComptonScatteringCountRate::createEqualityCM(const size_t nmasses) {
  // -- Equality constraints --
  // The user-specified equality matrix needs to be padded on the left with the
  // copies of the first column
  // until the number of cols matches the number of fixed parameters to account
  // for the extra coefficients
  // for the hermite term.
  // It then needs to be padded on the right with zeroes to account for the
  // background terms
  auto userMatrix = m_eqMatrix; // copy original
  const size_t nconstr = userMatrix.numRows();

  const size_t nColsCMatrix(m_cmatrix.numCols());
  size_t nFixedProfilePars(m_fixedParamIndices.size());
  // skip background for lhs padding
  if (m_bkgdPolyN > 0) {
    nFixedProfilePars -= (m_bkgdPolyN + 1);
  }
  const size_t nExtraColsLeft = (nFixedProfilePars - nmasses);

  m_eqMatrix =
      Kernel::DblMatrix(nconstr, nColsCMatrix); // all zeroed by default.
  for (size_t i = 0; i < nconstr; ++i) {
    const double *userRow = userMatrix[i];
    double *destRow = m_eqMatrix[i];
    for (size_t j = 0; j < nFixedProfilePars; ++j) {
      destRow[j] =
          (j < nExtraColsLeft) ? userRow[0] : userRow[j - nExtraColsLeft];
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
