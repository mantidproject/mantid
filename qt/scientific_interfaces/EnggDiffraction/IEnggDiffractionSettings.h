// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_

#include <string>

#include "EnggDiffCalibSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface to settings of the Engineering Diffraction (EnggDiffraction)
GUI. This can be used in different tabs/widgets as well as in the
main/central view. Normally the individual / area specific
tabs/widgets will forward to the main view.
*/
class IEnggDiffractionSettings {
public:
  virtual ~IEnggDiffractionSettings() = default;

  /**
   * Calibration settings as defined by the user.
   *
   * @return calibration settings object with current user settings
   */
  virtual EnggDiffCalibSettings currentCalibSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_
