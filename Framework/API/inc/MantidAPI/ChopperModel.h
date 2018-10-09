// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_CHOPPERMODEL_H_
#define MANTID_API_CHOPPERMODEL_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Run.h"

#include <string>

namespace Mantid {
namespace API {
//--------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------
class Run;

/**
 * Defines an interface to a chopper model which allows
 * calculating the variance in the time of the pulse through
 * the chopper
 */
class MANTID_API_DLL ChopperModel {
public:
  /// Default constructor required by the factory
  ChopperModel();
  /// Allow inheritance
  virtual ~ChopperModel() = default;
  /// Returns a clone of the current object
  virtual boost::shared_ptr<ChopperModel> clone() const = 0;

  /// Set the reference to the run object as we need a default constructor
  /// for the factory
  void setRun(const Run &exptRun);
  /// Initialize with a string of parameters
  void initialize(const std::string &params);

  /// Set the rotation speed in rads/sec
  void setAngularVelocityInHz(const double value);
  /// Set the angular velocity log name
  void setAngularVelocityLog(const std::string &logName);
  /// Returns the current angular velocity in rads/sec
  double getAngularVelocity() const;
  /// Sets the chopper jitter sigma value in microseconds. This is the FWHH
  /// value
  void setJitterFWHH(const double value);
  /// Returns the std deviation of the jitter value in seconds
  inline double getStdDevJitter() const { return m_jitterSigma; }

  /// Returns the variance of a the time pulse through this chopper in seconds^2
  double pulseTimeVariance() const;
  /// Returns a time sampled from the chopper distribution
  virtual double sampleTimeDistribution(const double randomNo) const = 0;
  /// Returns a time sampled from the jitter distribution
  virtual double sampleJitterDistribution(const double randomNo) const = 0;

protected:
  /// Set a named parameter from a string value
  virtual void setParameterValue(const std::string &name,
                                 const std::string &value) = 0;

  /// The variance of a the time pulse through this chopper in seconds^2
  virtual double calculatePulseTimeVariance() const = 0;
  /// Cache a value of the pulse variance
  void cachePulseVariance(const double value);
  /// Return current pulse variance cache
  inline double getCachedPulseVariance() const { return m_pulseVariance; }

  /// Returns a reference to the run object
  inline const Run &exptRun() const { return *m_exptRun; }

private:
  /// Handle any base parameters before passing to derived
  void setBaseParameters(std::map<std::string, std::string> &keyValues);

  /// A pointer to the run object
  const Run *m_exptRun;
  /// Current rotation speed
  double m_angularSpeed;
  /// Rotation speed log name
  std::string m_angularSpeedLog;
  /// Std deviation of chopper jitter in seconds
  double m_jitterSigma;
  /// Current value of the variance of the pulse
  double m_pulseVariance;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CHOPPERMODEL_H_ */
