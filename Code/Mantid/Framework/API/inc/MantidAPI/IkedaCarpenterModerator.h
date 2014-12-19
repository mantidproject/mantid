#ifndef MANTID_API_IKEDACARPENTERMODERATOR_H_
#define MANTID_API_IKEDACARPENTERMODERATOR_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidAPI/ModeratorModel.h"
#include <vector>

namespace Mantid {
namespace API {
/**
 *
 * Defines an object that models a moderator using an
 * Ikeda-Carpenter function.
 *
 * The model is parameterized by 3 variables:
 *  - \f$\alpha\f$: Coefficient for fast decay;
 *  - \f$\beta\f$: Coefficient for slow decay;
 *  - \f$R\f$: Mixing coefficient
 *
 *  where \f$\tau_mean=3/\alpha + R/\beta\f$
 *  and \f$\tau_sig=\sqrt{\frac{3}{\alpha^2} + R(2-R)*\frac{1}{\beta^2}}\f$
 */
class MANTID_API_DLL IkedaCarpenterModerator : public ModeratorModel {
public:
  /// Default constructor required by the factory
  IkedaCarpenterModerator();
  /// Returns a clone of the current object
  boost::shared_ptr<ModeratorModel> clone() const;

  /// Sets the value of the \f$\alpha\f$ parameter
  void setFastDecayCoefficent(const double value);
  /// Returns the value of the \f$\alpha\f$ parameter
  double getFastDecayCoefficent() const;
  /// Sets the value of the \f$\beta\f$ parameter
  void setSlowDecayCoefficent(const double value);
  /// Returns the value of the \f$\beta\f$ parameter
  double getSlowDecayCoefficent() const;
  /// Sets the value of the \f$R\f$ parameter.
  void setMixingCoefficient(const double value);
  /// Gets the value of the \f$R\f$ parameter.
  double getMixingCoefficient() const;

  /// Returns the mean time for emission in microseconds
  double emissionTimeMean() const;
  /// Returns the variance of emission time in microseconds
  double emissionTimeVariance() const;
  /// Returns a time, in seconds, sampled from the distibution given a flat
  /// random number
  double sampleTimeDistribution(const double flatRandomNo) const;

private:
  /// Custom initialize function, called after parameters have been set
  void init();
  /// Set a parameter value from a string
  void setParameterValue(const std::string &name, const std::string &value);
  /// Initialize the area-to-time lookup table
  void initLookupTable();
  /// For area between [0,1] returns the interpolated value of x
  double interpolateAreaTable(const double area) const;

  /// Returns the value of T such that the integral of a normalised
  /// Ikeda-Carpenter function, M(T), from 0 to T = area
  double areaToTime(const double area) const;
  /// Find the minimum of the areaToTimeFunction between the given interval with
  /// the given tolerance
  double findMinumum(const double rangeMin, const double rangeMax,
                     const double tolerance) const;
  /// Find the minimum of the areaToTimeFunction
  double zeroBrent(const double a, const double b, const double t) const;
  /// Function to pass to root-finder to find the value of the area for the
  /// given fraction of the range
  double areaToTimeFunction(const double fraction) const;
  /// Returns the area of the IKeda-Carpenter function for the given time value
  double area(const double t) const;

  /// The value of the 1/fast decay coefficient
  double m_tau_f;
  /// The value of the 1/slow decay coefficient
  double m_tau_s;
  /// The value of the mixing coefficient
  double m_r;

  /// Size of lookup table
  const unsigned int m_lookupSize;
  /// Lookup table
  std::vector<double> m_areaToTimeLookup;
  /// Value of offset for area calculation
  mutable double m_offset;
};
}
}

#endif /* MANTID_API_IKEDACARPENTERMODERATOR_H_ */
