#ifndef MANTID_API_IPEAKFUNCTION_H_
#define MANTID_API_IPEAKFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionWithLocation.h"

namespace Mantid {
namespace API {
/** An interface to a peak function, which extend the interface of
    IFunctionWithLocation by adding methods to set and get peak width.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class MANTID_API_DLL IPeakFunction : public IFunctionWithLocation {
public:
  /// Constructor
  IPeakFunction();

  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;

  /// Returns the peak FWHM
  virtual double fwhm() const = 0;

  /// Sets the parameters such that FWHM = w
  virtual void setFwhm(const double w) = 0;

  /// Returns the integral intensity of the peak
  virtual double intensity() const;

  /// Sets the integral intensity of the peak
  virtual void setIntensity(const double newIntensity);

  /// General implementation of the method for all peaks.
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// General implementation of the method for all peaks.
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) override;

  /// Get the interval on which the peak has all its values above a certain
  /// level
  virtual std::pair<double, double>
  getDomainInterval(double level = DEFAULT_SEARCH_LEVEL) const;

  /// Function evaluation method to be implemented in the inherited classes
  virtual void functionLocal(double *out, const double *xValues,
                             const size_t nData) const = 0;
  /// Derivative evaluation method to be implemented in the inherited classes
  virtual void functionDerivLocal(Jacobian *out, const double *xValues,
                                  const size_t nData) = 0;

  /// Get name of parameter that is associated to centre.
  std::string getCentreParameterName() const;

  /// Fix a parameter or set up a tie such that value returned
  /// by intensity() is constant during fitting.
  /// @param isDefault :: If true fix intensity by default:
  ///    don't show it in ties
  virtual void fixIntensity(bool isDefault = false) {
    UNUSED_ARG(isDefault);
    throw std::runtime_error(
        "Generic intensity fixing isn't implemented for this function.");
  }

  /// Free the intensity parameter.
  virtual void unfixIntensity() {
    throw std::runtime_error(
        "Generic intensity fixing isn't implemented for this function.");
  }

private:
  /// Set new peak radius
  void setPeakRadius(int r) const;
  /// Defines the area around the centre where the peak values are to be
  /// calculated (in FWHM).
  mutable int m_peakRadius;
  /// The default level for searching a domain interval (getDomainInterval())
  static constexpr double DEFAULT_SEARCH_LEVEL = 1e-5;
};

using IPeakFunction_sptr = boost::shared_ptr<IPeakFunction>;
using IPeakFunction_const_sptr = boost::shared_ptr<const IPeakFunction>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IPEAKFUNCTION_H_*/
