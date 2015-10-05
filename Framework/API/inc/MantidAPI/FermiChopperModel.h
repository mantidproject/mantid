#ifndef MANTID_API_FERMICHOPPERMODEL_H_
#define MANTID_API_FERMICHOPPERMODEL_H_
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
#include "MantidAPI/ChopperModel.h"

namespace Mantid {
namespace API {
/**
 * Defines a FermiChopper component modelled by a triangular distibution
 */
class MANTID_API_DLL FermiChopperModel : public ChopperModel {
public:
  /// Default constructor required by the factory
  FermiChopperModel();
  /// Returns a clone of the current object
  virtual boost::shared_ptr<ChopperModel> clone() const;

  /// Set the radius of the chopper in metres
  void setChopperRadius(const double value);
  /// Returns the chopper radius in metres
  inline double getChopperRadius() const { return m_chopperRadius; }
  /// Set the slit thickness in metres
  void setSlitThickness(const double value);
  /// Returns the chopper radius in metres
  inline double getSlitThickness() const { return m_slitThickness; }
  /// Set the radius of curvature of the slit
  void setSlitRadius(const double value);
  /// Returns the chopper radius in metres
  inline double getSlitRadius() const { return m_slitRadius; }
  /// Set the incident energy in meV
  void setIncidentEnergy(const double value);
  /// Set the log used to access the Ei
  void setIncidentEnergyLog(const std::string &logName);
  /// Returns the current incident energy in meV
  double getIncidentEnergy() const;

  /// Returns a time sample from the distribution given a flat random number
  double sampleTimeDistribution(const double randomNo) const;
  /// Returns a time sampled from the jitter distribution
  double sampleJitterDistribution(const double randomNo) const;

private:
  /// Set a parameter value from a string
  void setParameterValue(const std::string &name, const std::string &value);

  /// Returns the variance of a the time pulse through this chopper in seconds^2
  double calculatePulseTimeVariance() const;
  /// Computes the value of the regime-dependent portion of the pulse variance
  double regimeFactor(const double gamma) const;
  /// Map a flat random number to a triangular distibution of unit area
  double sampleFromTriangularDistribution(const double randomNo) const;

  /// The radius of the chopper in metres
  double m_chopperRadius;
  /// The slit thickness
  double m_slitThickness;
  /// Slit's radius of curvature in metres
  double m_slitRadius;
  /// Current incident energy
  double m_incidentEnergy;
  /// Incident energy log
  std::string m_incidentEnergyLog;
};
}
}

#endif /* MANTID_API_FERMICHOPPERMODEL_H_ */
