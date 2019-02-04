// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_MODERATORMODEL_H_
#define MANTID_API_MODERATORMODEL_H_

#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include <string>

namespace Mantid {
namespace API {
/**
 *
 * Defines a base class for a moderator model
 */
class MANTID_API_DLL ModeratorModel {
public:
  /// Default constructor required by the factory
  ModeratorModel();
  /// Allow inheritance
  virtual ~ModeratorModel() = default;
  /// Returns a clone of the current object
  virtual boost::shared_ptr<ModeratorModel> clone() const = 0;

  /// Initialize the object from a string of parameters
  void initialize(const std::string &params);
  /// Custom init function called after parameters have been processed. Default
  /// action is to do nothing
  virtual void init() {}

  /// Sets the tilt angle in degrees (converted to radians internally)
  void setTiltAngleInDegrees(const double theta);
  /// Returns the value of the tilt angle in radians
  double getTiltAngleInRadians() const;

  /// Returns the mean time for emission in microseconds
  virtual double emissionTimeMean() const = 0;
  /// Returns the variance of emission time in microseconds
  virtual double emissionTimeVariance() const = 0;
  /// Returns a time, in seconds, sampled from the distibution given a flat
  /// random number
  virtual double sampleTimeDistribution(const double flatRandomNo) const = 0;

protected:
  /// Set a named parameter from a string value
  virtual void setParameterValue(const std::string &name,
                                 const std::string &value) = 0;

private:
  /// Moderator tilt angle in radians
  double m_tiltAngle;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MODERATORMODEL_H_ */
