// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Chebyshev.h"
#include "MantidAPI/FunctionFactory.h"

#include <algorithm>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Chebyshev)

Chebyshev::Chebyshev() : m_n(0), m_StartX(-1.), m_EndX(1.) {
  declareParameter("A0");
  declareAttribute("n", Attribute(m_n));
  declareAttribute("StartX", Attribute(m_StartX));
  declareAttribute("EndX", Attribute(m_EndX));
}

void Chebyshev::function1D(double *out, const double *xValues,
                           const size_t nData) const {
  if (m_StartX >= m_EndX) {
    throw std::runtime_error("Chebyshev: invalid x-range");
  }
  double b = 2. / (m_EndX - m_StartX);
  double a = 1. - b * m_EndX;
  if (static_cast<int>(m_b.size()) != m_n + 3) {
    m_b.resize(m_n + 3);
  }
  for (size_t i = 0; i < nData; ++i) {
    // scale to interval -1 < x < 1
    double x = a + b * xValues[i];
    m_b[m_n + 2] = 0.;
    m_b[m_n + 1] = 0.;
    for (int j = m_n; j > 0; --j) {
      m_b[j] = m_b[j + 1] * x * 2 - m_b[j + 2] + getParameter(j);
    }
    out[i] = x * m_b[1] - m_b[2] + getParameter(0);
  }
}

void Chebyshev::functionDeriv1D(Jacobian *out, const double *xValues,
                                const size_t nData) {
  if (m_n < 0) {
    return;
  }

  double b = 2. / (m_EndX - m_StartX);
  double a = 1. - b * m_EndX;

  for (size_t i = 0; i < nData; i++) {
    double t0 = 1.;
    double x = a + b * xValues[i];
    double t1 = x;
    out->set(i, 0, 1);
    if (m_n == 0)
      continue;
    out->set(i, 1, x);
    for (int j = 2; j <= m_n; ++j) {
      double t = 2 * x * t1 - t0;
      out->set(i, j, t);
      t0 = t1;
      t1 = t;
    }
  }
}

/**
 * @param attName :: The attribute name. If it is not "n" exception is thrown.
 * @param att :: An int attribute containing the new value. The value cannot be
 * negative.
 */
void Chebyshev::setAttribute(const std::string &attName,
                             const API::IFunction::Attribute &att) {
  storeAttributeValue(attName, att);

  if (attName == "n") {
    // set the polynomial order

    auto newN = att.asInt();
    if (newN < 0) {
      throw std::invalid_argument(
          "Chebyshev: polynomial order cannot be negative.");
    }

    // Save old values
    std::vector<double> oldValues(std::min(m_n, newN) + 1);
    for (size_t i = 0; i < oldValues.size(); ++i) {
      oldValues[i] = getParameter(i);
    }

    if (m_n >= 0) {
      clearAllParameters();
    }
    m_n = att.asInt();
    for (int i = 0; i <= m_n; ++i) {
      std::string parName = "A" + std::to_string(i);
      declareParameter(parName);
    }

    // Reset old values to new parameters
    for (size_t i = 0; i < oldValues.size(); ++i) {
      setParameter(i, oldValues[i]);
    }

  } else if (attName == "StartX") {
    m_StartX = att.asDouble();
  } else if (attName == "EndX") {
    m_EndX = att.asDouble();
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
