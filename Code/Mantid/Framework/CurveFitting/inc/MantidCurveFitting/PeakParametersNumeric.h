#ifndef MANTID_CURVEFITTING_PEAKPARAMETERSNUMERIC_H_
#define MANTID_CURVEFITTING_PEAKPARAMETERSNUMERIC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {

class ChebfunBase;

/**

Implements IPeakFunction's getters and setters for the peak centre, height and
fwhm. The parameters are calculated numerically.

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
class DLLExport PeakParametersNumeric : public API::IPeakFunction {
public:
  /// Default constructor.
  PeakParametersNumeric()
      : API::IPeakFunction(), m_invalidateCache(true), m_centre(0), m_width(0),
        m_height(0) {}

  /// overwrite IPeakFunction base class methods
  virtual double centre() const;
  virtual void setCentre(const double c);
  virtual double height() const;
  virtual void setHeight(const double h);
  virtual double fwhm() const;
  virtual void setFwhm(const double w);

  /// Set i-th parameter
  virtual void setParameter(size_t, const double &value,
                            bool explicitlySet = true);
  using API::IPeakFunction::setParameter;
  std::string getCentreParameterName() const;

  /// Get boundaries for an interval within which the peak has 
  /// significant values. The interval must contain the maximum
  /// point and points below the half-maximum on both side
  /// from the centre. Ideally the interval should exclude
  /// points with values below 1% of the maximum.
  virtual std::pair<double, double> getExtent() const = 0;

protected:
  double operator()(double) const;
  void updateCache() const;
  boost::shared_ptr<ChebfunBase> makeBase(double start, double end,
                                          std::vector<double> &p,
                                          std::vector<double> &a) const;

  /// Enumerates possible effects of parameters on the width
  enum WidthParamType {Linear, Square, Inverse};

  void defineHeightParameter(const std::string& parName);
  void defineCentreParameter(const std::string& parName);
  void defineWidthParameter(const std::string& parName, WidthParamType wType);

  // setter helpers

  /// Index of parameter proportional to the height
  size_t m_heightIndex;
  /// Index of parameter which shifts the centre
  size_t m_centreIndex;
  /// Indices of parameters which affect the width
  std::vector<size_t> m_widthIndices;
  /// Types of the width parameter effects
  std::vector<WidthParamType> m_widthParTypes;

  // cached values

  mutable bool m_invalidateCache; ///< dirty flag
  mutable double m_centre;        ///< peak centre (the maximum point)
  mutable double m_width;         ///< FWHM
  mutable double m_height;        ///< the maximum value

  mutable double m_start;
  mutable double m_end;
  mutable boost::shared_ptr<ChebfunBase> m_base; ///< function approximator

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PEAKPARAMETERSNUMERIC_H_*/
