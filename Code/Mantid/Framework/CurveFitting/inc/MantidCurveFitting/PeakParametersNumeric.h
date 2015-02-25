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
fwhm.

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

  virtual std::pair<double, double> getExtent() const = 0;

protected:
  double operator()(double) const;
  void updateCache() const;
  boost::shared_ptr<ChebfunBase> makeBase(double start, double end,
                                          std::vector<double> &p,
                                          std::vector<double> &a) const;

  enum WidthParamType {Linear, Inverse};

  void defineHeightParameter(const std::string& parName);
  void defineCentreParameter(const std::string& parName);
  void defineWidthParameter(const std::string& parName, WidthParamType wType);

  mutable bool m_invalidateCache;
  mutable double m_centre;
  mutable double m_width;
  mutable double m_height;

  mutable double m_start;
  mutable double m_end;
  mutable boost::shared_ptr<ChebfunBase> m_base;

  size_t m_heightIndex;
  size_t m_centreIndex;
  std::vector<size_t> m_widthIndices;
  std::vector<WidthParamType> m_widthParTypes;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PEAKPARAMETERSNUMERIC_H_*/
