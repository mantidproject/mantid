// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <cmath>
#include <memory>
#include <vector>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Performes convolution of two functions.


@author Roman Tolchenov, Tessella plc
@date 28/01/2010
*/
class MANTID_CURVEFITTING_DLL Convolution : public API::CompositeFunction {
public:
  /**
   * Class for helping to read the transformed data. It represent an output of
   * the
   * GSL real fast fourier transform routine. The routine transforms an array of
   * n
   * real numbers into an array of about n/2 complex numbers which are the
   * amplitudes of
   * the positive frequencies of the full complex fourier transform.
   */
  class HalfComplex {
    size_t m_size;  ///< size of the transformed data
    double *m_data; ///< pointer to the transformed data
    bool m_even;    ///< true if the size of the original data is even
  public:
    /**
     * Constructor.
     * @param data :: A pointer to the transformed complex data
     * @param n :: The size of untransformed real data
     */
    HalfComplex(double *data, const size_t &n) : m_size(n / 2 + 1), m_data(data), m_even(n / 2 * 2 == n) {}
    /// Returns the size of the transform
    size_t size() const { return m_size; }
    /**
     * The real part of i-th transform coefficient
     * @param i :: The index of the complex transform coefficient
     * @return The real part
     */
    double real(size_t i) const {
      if (i >= m_size)
        return 0.;
      if (i == 0)
        return m_data[0];
      return m_data[2 * i - 1];
    }
    /**
     * The imaginary part of i-th transform coefficient
     * @param i :: The index of the complex transform coefficient
     * @return The imaginary part
     */
    double imag(size_t i) const {
      if (i >= m_size)
        return 0.;
      if (i == 0)
        return 0;
      if (m_even && i == m_size - 1)
        return 0;
      return m_data[2 * i];
    }
    /**
     * Set a new value for i-th complex coefficient
     * @param i :: The index of the coefficient
     * @param re :: The real part of the new value
     * @param im :: The imaginary part of the new value
     */
    void set(size_t i, const double &re, const double &im) {
      if (i >= m_size)
        return;
      if (i == 0) // this is purely real
      {
        m_data[0] = re;
      } else if (m_even && i == m_size - 1) // this is also purely real
      {
        m_data[2 * i - 1] = re;
      } else {
        m_data[2 * i - 1] = re;
        m_data[2 * i] = im;
      }
    }
  };

  /// Constructor
  Convolution();

  /// overwrite IFunction base class methods
  std::string name() const override { return "Convolution"; }
  const std::string category() const { return "General"; }
  /// Function you want to fit to.
  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;
  void functionFFTMode(const API::FunctionDomain &domain, API::FunctionValues &values) const;
  void functionDirectMode(const API::FunctionDomain &domain, API::FunctionValues &values) const;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

  /// Add a function.
  size_t addFunction(API::IFunction_sptr f) override;
  /// Set up the function for a fit.
  void setUpForFit() override;

  /// Deletes and zeroes pointer m_resolution forsing function(...) to
  /// recalculate the resolution function
  void refreshResolution() const;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Keep the Fourier transform of the resolution function (divided by the
  /// step in xValues) when in FFT mode, and the inverted resolution if in
  /// Direct mode
  mutable std::vector<double> m_resolution;
  void innerFunctionsAre1D() const;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
