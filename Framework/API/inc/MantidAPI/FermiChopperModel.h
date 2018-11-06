// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FERMICHOPPERMODEL_H_
#define MANTID_API_FERMICHOPPERMODEL_H_

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
  boost::shared_ptr<ChopperModel> clone() const override;

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
  double sampleTimeDistribution(const double randomNo) const override;
  /// Returns a time sampled from the jitter distribution
  double sampleJitterDistribution(const double randomNo) const override;

private:
  /// Set a parameter value from a string
  void setParameterValue(const std::string &name,
                         const std::string &value) override;

  /// Returns the variance of a the time pulse through this chopper in seconds^2
  double calculatePulseTimeVariance() const override;
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
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FERMICHOPPERMODEL_H_ */
