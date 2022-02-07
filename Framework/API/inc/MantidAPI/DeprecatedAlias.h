// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include <string>

namespace Mantid {
namespace API {

/** DeprecatedAlias :
 * Class for making algorithm with deprecated names (aliases).
 *
 * This class will ensure that if an algorithm is invoke with a deprecated name,
 * - a warning will be throw to inform the users that
 *   - the algorithm name is deprecated
 *   - the deprecation date
 *   - the new algorithm name the user is recommended to use instead
 *
 * All algorithms with deprecated alias need to inherit from this class.
 *
 * The recommended algorithm naming pattern should be
 * [Technique][Facility/Instrument]ActionTarget
 * For example: the calibration routine of panel detector for single crystal diffraction
 *              beamline can be named as SCDCalibratePanels
 */
class MANTID_API_DLL DeprecatedAlias {
public:
  DeprecatedAlias();
  virtual ~DeprecatedAlias();
  std::string deprecationMessage(const IAlgorithm *);
  void setDeprecationDate(const std::string &date);

private:
  /// Deprecation date
  std::string m_deprecationDate;
};

} // namespace API
} // namespace Mantid
