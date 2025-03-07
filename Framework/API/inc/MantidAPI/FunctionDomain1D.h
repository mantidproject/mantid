// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain.h"

#include <vector>

namespace Mantid {
namespace API {
/**
    Represent a domain for functions of one real argument. This class does not
   contain
    any data, only a pointer to it. It is not to be instantiated but serves as a
   base class
    for FunctionDomain1DVector and FunctionDomain1DView classes. The data access
   methods
    are not virtual for efficiency.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_API_DLL FunctionDomain1D : public FunctionDomain {
public:
  /// copying is not allowed.
  FunctionDomain1D(const FunctionDomain1D &right) = delete;
  /// copying is not allowed.
  FunctionDomain1D &operator=(const FunctionDomain1D &) = delete;
  /// Return the number of arguments in the domain
  size_t size() const override { return m_n; }
  /// Get an x value.
  /// @param i :: Index
  double operator[](size_t i) const { return m_data[i]; }
  /// Get a pointer to i-th value
  const double *getPointerAt(size_t i) const { return m_data + i; }
  /// Convert to a vector
  std::vector<double> toVector() const;
  /// Set a peak radius to pass to peak functions.
  void setPeakRadius(int radius);
  /// Get the peak radius.
  int getPeakRadius() const;

protected:
  /// Protected constructor, shouldn't be created directly. Use
  /// FunctionDomain1DView instead.
  FunctionDomain1D(const double *x, size_t n);
  /// Reset the pointer and size of the domain
  void resetData(const double *x, size_t n) {
    m_data = x;
    m_n = n;
  }

private:
  /// pointer to the start of the domain data
  const double *m_data;
  /// size of the data
  size_t m_n;
  /// A peak radius that IPeakFunctions should use
  int m_peakRadius;
};

/**
 * Implements FunctionDomain1D with its own storage in form of a std::vector.
 */
class MANTID_API_DLL FunctionDomain1DVector : public FunctionDomain1D {
public:
  /// Constructor.
  FunctionDomain1DVector(const double x);
  /// Constructor.
  FunctionDomain1DVector(const double startX, const double endX, const size_t n);
  /// Constructor.
  FunctionDomain1DVector(const std::vector<double> &xvalues);
  /// No-copy constructor.
  FunctionDomain1DVector(std::vector<double> &&xvalues);
  /// Constructor.
  FunctionDomain1DVector(std::vector<double>::const_iterator from, std::vector<double>::const_iterator to);
  /// Copy constructor.
  FunctionDomain1DVector(const FunctionDomain1DVector &);
  /// Copy assignment operator.
  FunctionDomain1DVector &operator=(const FunctionDomain1DVector &);
  /// Get the underlying vector
  const std::vector<double> &getVector() { return m_X; }

protected:
  std::vector<double> m_X; ///< vector of function arguments
};

/**
 * 1D domain - a wrapper around an array of doubles.
 */
class MANTID_API_DLL FunctionDomain1DView : public FunctionDomain1D {
public:
  /**
   * Creates a FunctionDomain1DView pointing to an array of doubles of size n.
   * @param x :: The start of the array.
   * @param n :: The size of the array.
   */
  FunctionDomain1DView(const double *x, size_t n) : FunctionDomain1D(x, n) {}

private:
  /// Private copy constructor - copying is not allowed.
  FunctionDomain1DView(const FunctionDomain1DView &);
  /// Private copy assignment operator - copying is not allowed.
  FunctionDomain1DView &operator=(const FunctionDomain1DView &);
};

/**
 * Specialization of FunctionDomain1DVector for spectra of MatrixWorkspaces.
 * The domain holds the workspace index allowing functions to use
 * spectra-specific
 * information.
 */
class MANTID_API_DLL FunctionDomain1DSpectrum : public FunctionDomain1DVector {
public:
  /// Constructor.
  FunctionDomain1DSpectrum(size_t wi, const std::vector<double> &xvalues);
  /// Constructor.
  FunctionDomain1DSpectrum(size_t wi, std::vector<double>::const_iterator from, std::vector<double>::const_iterator to);
  /// Get the workspace index
  size_t getWorkspaceIndex() const { return m_workspaceIndex; }

private:
  /// The workspace index
  size_t m_workspaceIndex;
};

/// Implements FunctionDomain1D as a set of bins for a histogram.
/// operator[i] returns the right boundary of i-th bin.
/// The left boundary of the first bin (#0) is returned by leftBoundary()
/// method.
class MANTID_API_DLL FunctionDomain1DHistogram : public FunctionDomain1D {
public:
  /// Constructor.
  FunctionDomain1DHistogram(const std::vector<double> &bins);
  /// Constructor.
  FunctionDomain1DHistogram(std::vector<double>::const_iterator from, std::vector<double>::const_iterator to);

  /// Disable copy operator
  FunctionDomain1DHistogram(const FunctionDomain1DHistogram &) = delete;

  /// Disable assignment operator
  FunctionDomain1DHistogram &operator=(const FunctionDomain1DHistogram &) = delete;

  /// Get the leftmost boundary
  double leftBoundary() const;

protected:
  std::vector<double> m_bins; ///< vector of bin boundaries
};

/// typedef for a shared pointer to a FunctionDomain1D
using FunctionDomain1D_sptr = std::shared_ptr<FunctionDomain1D>;
/// typedef for a shared pointer to a const FunctionDomain1D
using FunctionDomain1D_const_sptr = std::shared_ptr<const FunctionDomain1D>;

} // namespace API
} // namespace Mantid
