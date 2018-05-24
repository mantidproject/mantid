#ifndef MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTER_H_
#define MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTER_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"

#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {

/**
 * Provides a layer class for boost::python to allow C++ virtual functions
 * to be overridden in a Python object that is derived from IFunction1D.
 *
 * This is essentially a transparent layer that handles the function calls up
 *into Python.
 */
#if defined(_MSC_VER) && _MSC_VER >= 1900
// All Python tests segfault on MSVC 2015 if the virtual specifiers are included
// The segault happens on initializing this constructor. Conversely on all other
// compilers it won't even compile without the virtual specifier.
class IFunction1DAdapter : public API::ParamFunction,
                           public API::IFunction1D,
                           public IFunctionAdapter {
#else
class IFunction1DAdapter : public virtual API::ParamFunction,
                           public virtual API::IFunction1D,
                           public IFunctionAdapter {
#endif
public:
  // Convenience type def
  using Base = API::IFunction1D;

  /// A constructor that looks like a Python __init__ method
  IFunction1DAdapter(PyObject *self);

  /// Disable copy operator - The PyObject must be supplied to construct the
  /// object
  IFunction1DAdapter(const IFunction1DAdapter &) = delete;

  /// Disable assignment operator
  IFunction1DAdapter &operator=(const IFunction1DAdapter &) = delete;

  /** @name Virtual methods */
  ///@{
  /// Base-class method
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// Python-type signature
  boost::python::object function1D(const boost::python::object &xvals) const;

  /// Derivatives of function with respect to active parameters (C++ override)
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
  ///@}
};
}
}

#endif /* MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTER_H_ */
