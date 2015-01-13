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
  /// Virtual destructor
  /// (avoids warnings about non-trivial move assignment in virtually inheriting
  /// classes)
  virtual ~IPeakFunction() {}
  /// Returns the peak FWHM
  virtual double fwhm() const = 0;

  /// Sets the parameters such that FWHM = w
  virtual void setFwhm(const double w) = 0;

  /// General implementation of the method for all peaks.
  void function1D(double *out, const double *xValues, const size_t nData) const;
  /// General implementation of the method for all peaks.
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData);
  /// Set new peak radius
  static void setPeakRadius(const int &r = 5);

  /// Function evaluation method to be implemented in the inherited classes
  virtual void functionLocal(double *out, const double *xValues,
                             const size_t nData) const = 0;
  /// Derivative evaluation method to be implemented in the inherited classes
  virtual void functionDerivLocal(Jacobian *out, const double *xValues,
                                  const size_t nData) = 0;

protected:
  /// Defines the area around the centre where the peak values are to be
  /// calculated (in FWHM).
  static int s_peakRadius;
};

typedef boost::shared_ptr<IPeakFunction> IPeakFunction_sptr;
typedef boost::shared_ptr<const IPeakFunction> IPeakFunction_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IPEAKFUNCTION_H_*/
