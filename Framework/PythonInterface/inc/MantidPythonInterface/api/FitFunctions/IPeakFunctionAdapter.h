#ifndef MANTID_PYTHONINTERFACE_IPEAKFUNCTIONADAPTER_H_
#define MANTID_PYTHONINTERFACE_IPEAKFUNCTIONADAPTER_H_
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
#include "MantidAPI/IPeakFunction.h"
#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"

#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {

/**
 * Provides a layer class for boost::python to allow C++ virtual functions
 * to be overridden in a Python object that is derived from IPeakFunction.
 *
 * This is essentially a transparent layer that handles the function calls up
 *into Python.
 */
class IPeakFunctionAdapter : public API::IPeakFunction,
                             public IFunctionAdapter {
public:
  // Convenience typedef
  using Base = API::IPeakFunction;

  /// A constructor that looks like a Python __init__ method
  IPeakFunctionAdapter(PyObject *self);

  /// Disable copy operator - The PyObject must be supplied to construct the
  /// object
  IPeakFunctionAdapter(const IPeakFunctionAdapter &) = delete;

  /// Disable assignment operator
  IPeakFunctionAdapter &operator=(const IPeakFunctionAdapter &) = delete;

  /// Calls 'centre' method in Python
  double centre() const override;
  /// Calls 'height' method in Python
  double height() const override;
  /// Calls 'setCentre' method in Python
  void setCentre(const double c) override;
  /// Calls 'setHeight' method in Python
  void setHeight(const double h) override;

  /// Calls Python fwhm method
  double fwhm() const override;
  /// Called by framework when the width is changed
  void setFwhm(const double w) override;

  /// Required to solve compiler ambiguity between IPeakFunction &
  /// IFunction1DAdapter
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    IPeakFunction::function1D(out, xValues, nData);
  }
  /// Required to solve compiler ambiguity between IPeakFunction &
  /// IFunction1DAdapter
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override {
    IPeakFunction::functionDeriv1D(out, xValues, nData);
  }

  /// Implemented Base-class method
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override;
  /// Python-type signature for above method
  boost::python::object functionLocal(const boost::python::object &xvals) const;
  /// Implemented base-class method
  void functionDerivLocal(API::Jacobian *jacobian, const double *xValues,
                          const size_t nData) override;
  /// Python signature
  void functionDerivLocal(const boost::python::object &xvals,
                          boost::python::object &jacobian);

private:
};
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_IPEAKFUNCTIONADAPTER_H_ */
