// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
#include "MantidAPI/ModeratorModel.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace API {

/// Default constructor required by the factory
ModeratorModel::ModeratorModel() : m_tiltAngle(0.0) {}

/**
 * Initialize the object from a string of parameters
 * @param params :: A string containing the parameter names & values
 */
void ModeratorModel::initialize(const std::string &params) {
  if (params.empty())
    throw std::invalid_argument(
        "ModeratorModel::initialize - Empty parameter string.");
  static const char *keyValSep = "=";
  static const char *listSep = ",";

  auto keyValues =
      Kernel::Strings::splitToKeyValues(params, keyValSep, listSep);
  if (keyValues.empty()) {
    throw std::invalid_argument(
        "ModeratorModel::initialize - Parameter string was not empty but no "
        "values"
        "could be parsed. Check it has the key=value format.");
  }

  for (auto &keyValue : keyValues) {
    setParameterValue(keyValue.first, keyValue.second);
  }

  /// Any custom setup
  this->init();
}

/**
 * Sets the tilt angle in radians
 * @param theta :: The value of the tilt angle in radians
 */
void ModeratorModel::setTiltAngleInDegrees(const double theta) {
  static double degToRad(M_PI / 180.0);
  m_tiltAngle = theta * degToRad;
}

/**
 * @return The value of the tilt angle in radians
 */
double ModeratorModel::getTiltAngleInRadians() const { return m_tiltAngle; }
} // namespace API
} // namespace Mantid
