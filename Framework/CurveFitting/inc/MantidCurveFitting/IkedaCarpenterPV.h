#ifndef MANTID_CURVEFITTING_IKEDACARPENTERPV_H_
#define MANTID_CURVEFITTING_IKEDACARPENTERPV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IFunctionMW.h"

namespace Mantid {
namespace CurveFitting {
/**
Provide Ikeda-Carpenter-pseudo-Voigt peak shape function interface to
IPeakFunction. See wiki
page www.mantidproject.org/IkedaCarpenterPV for documentation for this function.

@author Anders Markvardsen, ISIS, RAL
@date 3/11/2009

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IkedaCarpenterPV : virtual public API::IPeakFunction,
                                   virtual public API::IFunctionMW {
public:
  /// Destructor
  virtual ~IkedaCarpenterPV(){};

  /// contruction used for standard fitting
  IkedaCarpenterPV(){};

  /// overwrite IPeakFunction base class methods
  virtual double centre() const;
  virtual double height() const;
  virtual double fwhm() const;
  virtual void setCentre(const double c);
  virtual void setHeight(const double h);
  virtual void setFwhm(const double w);

  /// overwrite IFunction base class methods
  std::string name() const { return "IkedaCarpenterPV"; }
  virtual const std::string category() const { return "Peak"; }

  // define these instead of functionLocal if you want to custom specify the
  // calculation
  // domain for this function
  // virtual void function(double* out, const double* xValues, const int&
  // nData)const;
  // virtual void functionDeriv(API::Jacobian* out, const double* xValues, const
  // int& nData);

protected:
  virtual void functionLocal(double *out, const double *xValues,
                             const size_t nData) const;
  virtual void functionDerivLocal(API::Jacobian *out, const double *xValues,
                                  const size_t nData);
  virtual void functionDeriv(const API::FunctionDomain &domain,
                             API::Jacobian &jacobian);

  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();

private:
  /// container for storing wavelength values for each data point
  mutable std::vector<double> m_waveLength;

  /// calculate the const function
  void constFunction(double *out, const double *xValues,
                     const int &nData) const;

  /// method for updating m_waveLength
  void calWavelengthAtEachDataPoint(const double *xValues,
                                    const size_t &nData) const;

  /// convert voigt params to pseudo voigt params
  void convertVoigtToPseudo(const double &voigtSigmaSq,
                            const double &voigtGamma, double &H,
                            double &eta) const;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IKEDACARPENTERPV_H_*/
