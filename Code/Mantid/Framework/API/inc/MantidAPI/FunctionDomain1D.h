#ifndef MANTID_API_FUNCTIONDOMAIN1D_H_
#define MANTID_API_FUNCTIONDOMAIN1D_H_

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

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL FunctionDomain1D : public FunctionDomain {
public:
  /// Return the number of arguments in the domain
  virtual size_t size() const { return m_n; }
  /// Get an x value.
  /// @param i :: Index
  double operator[](size_t i) const { return m_data[i]; }
  /// Get a pointer to i-th value
  const double *getPointerAt(size_t i) const { return m_data + i; }
  /// Convert to a vector
  std::vector<double> toVector() const;

protected:
  /// Protected constructor, shouldn't be created directly. Use
  /// FunctionDomain1DView instead.
  FunctionDomain1D(const double *x, size_t n) : m_data(x), m_n(n) {}
  /// Reset the pointer and size of the domain
  void resetData(const double *x, size_t n) {
    m_data = x;
    m_n = n;
  }

private:
  /// Private copy constructor - copying is not allowed.
  FunctionDomain1D(const FunctionDomain1D &right);
  /// Private copy assignment operator - copying is not allowed.
  FunctionDomain1D &operator=(const FunctionDomain1D &);
  const double *m_data; ///< pointer to the start of the domain data
  size_t m_n;           ///< size of the data
};

/**
 * Implements FunctionDomain1D with its own storage in form of a std::vector.
 */
class MANTID_API_DLL FunctionDomain1DVector : public FunctionDomain1D {
public:
  /// Constructor.
  FunctionDomain1DVector(const double x);
  /// Constructor.
  FunctionDomain1DVector(const double startX, const double endX,
                         const size_t n);
  /// Constructor.
  FunctionDomain1DVector(const std::vector<double> &xvalues);
  /// Constructor.
  FunctionDomain1DVector(std::vector<double>::const_iterator from,
                         std::vector<double>::const_iterator to);
  /// Copy constructor.
  FunctionDomain1DVector(const FunctionDomain1DVector &);
  /// Copy assignment operator.
  FunctionDomain1DVector &operator=(const FunctionDomain1DVector &);

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
  FunctionDomain1DSpectrum(size_t wi, std::vector<double>::const_iterator from,
                           std::vector<double>::const_iterator to);
  /// Get the workspace index
  size_t getWorkspaceIndex() const { return m_workspaceIndex; }

private:
  /// The workspace index
  size_t m_workspaceIndex;
};

/// typedef for a shared pointer to a FunctionDomain1D
typedef boost::shared_ptr<FunctionDomain1D> FunctionDomain1D_sptr;
/// typedef for a shared pointer to a const FunctionDomain1D
typedef boost::shared_ptr<const FunctionDomain1D> FunctionDomain1D_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAIN1D_H_*/
