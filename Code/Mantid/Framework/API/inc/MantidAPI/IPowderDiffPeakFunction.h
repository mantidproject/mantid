#ifndef MANTID_API_IPOWDERDIFFPEAKFUNCTION_H_
#define MANTID_API_IPOWDERDIFFPEAKFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidGeometry/Crystal/UnitCell.h"

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
class MANTID_API_DLL IPowderDiffPeakFunction
    : public virtual API::ParamFunction,
      public virtual API::IFunction1D {
public:
  /// Constructor and Destructor
  IPowderDiffPeakFunction();

  virtual ~IPowderDiffPeakFunction();

  /// Overwrite IFunction base class methods
  // virtual const std::string name() = 0;
  /// Category of function
  // virtual const std::string category(){ return "General"; }

  /// Get peak's centre
  virtual double centre() const;
  /// Get peak's intensity
  virtual double height() const;
  /// Get peakl's FWHM
  virtual double fwhm() const;
  /// Set peak's height
  virtual void setHeight(const double h);
  /// Set peak's radius
  virtual void setPeakRadius(const int &r);

  //--------------- ThermalNeutron peak function special
  //---------------------------------------
  /// Set Miller Indicies
  virtual void setMillerIndex(int h, int k, int l);

  /// Get Miller Index from this peak
  virtual void getMillerIndex(int &h, int &k, int &l);

  /// Get peak parameters
  virtual double getPeakParameter(std::string) = 0;

  /// Calculate peak parameters (alpha, beta, sigma2..)
  virtual void calculateParameters(bool explicitoutput) const = 0;

  /// Set up the flag to show whether (from client) cell parameter value changed
  virtual void setUnitCellParameterValueChangeFlag(bool changed) {
    m_cellParamValueChanged = changed;
  }

  /// The flag to show whether the parameters set to peak function making an
  /// valid peak
  virtual bool isPhysical() { return m_parameterValid; }

  /// Override setting a new value to the i-th parameter
  virtual void setParameter(size_t i, const double &value,
                            bool explicitlySet = true);

  /// Override setting a new value to a parameter by name
  virtual void setParameter(const std::string &name, const double &value,
                            bool explicitlySe = true);

  /// Check whether a parameter is a profile parameter
  virtual bool hasProfileParameter(std::string paramname);

  // void functionLocal(double* out, const double* xValues, const size_t
  // nData)const;

  /// Calculate function in a range
  using IFunction1D::function;
  virtual void function(std::vector<double> &out,
                        const std::vector<double> &xValues) const = 0;

  /// Get maximum value on a given set of data points
  virtual double getMaximumValue(const std::vector<double> &xValues,
                                 size_t &indexmax) const;

protected:
  /// Local function for GSL minimizer
  // virtual void functionLocal(double*, const double*, int&) const = 0;

  /// Local function for calculation in Mantid
  // virtual void functionLocal(vector<double> &out, const vector<double>
  // &xValues) const = 0;

  /// General implementation of the method for all peaks. Calculates derivatives
  /// only
  /// void functionDeriv1D(Jacobian* out, const double* xValues, const size_t
  /// nData) const;

  /// General implemenation of derivative
  // void functionDerivLocal(Jacobian* out, const double* xValues, const size_t
  // nData) const;

  /// Defines the area around the centre where the peak values are to be
  /// calculated (in FWHM).
  static int s_peakRadius;

  /// Centre of the peak
  mutable double m_centre;
  /// Centre of the peak in d-space
  mutable double m_dcentre;
  /// Peak's FWHM
  mutable double m_fwhm;

  /// Flag if any parameter value changed
  mutable bool m_hasNewParameterValue;
  /// An indicator to re-calculate peak d-space position
  mutable bool m_cellParamValueChanged;

  /// Peak profile parameters names in ascending order
  std::vector<std::string> m_sortedProfileParameterNames;

  /// Unit cell
  mutable Geometry::UnitCell m_unitCell;

  /// Unit cell size
  double m_unitCellSize;

  /// Flag to indicate whether peaks' parameters value can generate a valid peak
  mutable bool m_parameterValid;

  /// Miller Indices
  mutable int mH;
  mutable int mK;
  mutable int mL;
  mutable bool mHKLSet;

  size_t LATTICEINDEX;
  size_t HEIGHTINDEX;
};

typedef boost::shared_ptr<IPowderDiffPeakFunction> IPowderDiffPeakFunction_sptr;

/// Integral for Gamma
std::complex<double> MANTID_API_DLL E1(std::complex<double> z);

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IPOWDERDIFFPEAKFUNCTION_H_*/
