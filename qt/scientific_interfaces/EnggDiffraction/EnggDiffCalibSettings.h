// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings related to calibration in the engineering diffraction custom
interface. This is just a data holder.
*/
struct EnggDiffCalibSettings {
  std::string m_inputDirCalib;
  std::string m_inputDirRaw;
  std::string m_pixelCalibFilename;
  bool m_forceRecalcOverwrite;
  std::string m_templateGSAS_PRM;
  float m_rebinCalibrate;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_
