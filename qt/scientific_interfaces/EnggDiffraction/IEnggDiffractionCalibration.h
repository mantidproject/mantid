// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONCALIBRATION_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONCALIBRATION_H_

#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Parameters from a GSAS calibration. They define a conversion of
 * units time-of-flight<->d-spacing that can be calculated with the
 * algorithm AlignDetectors for example.
 */
struct GSASCalibrationParms {
  GSASCalibrationParms(size_t bid, double dc, double da, double tz)
      : bankid(bid), difc(dc), difa(da), tzero(tz) {}

  size_t bankid{0};
  double difc{0};
  double difa{0};
  double tzero{0};
};

/**
Interface to the current calibration functionality of the Engineering
Diffraction (EnggDiffraction) GUI. This can be used in different
tabs/widgets as well as in the main/central view. Normally this
interface will be provided by the presenters of the widgets (assuming
an MVP design). The individual / area specific tabs/widgets (their
presenters) will forward to the widget responsible for the
calibration.
*/
class IEnggDiffractionCalibration {
public:
  virtual ~IEnggDiffractionCalibration() = default;

  virtual std::vector<GSASCalibrationParms> currentCalibration() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIOPYTHONRUNNER_H_
