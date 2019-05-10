// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAM_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAM_H_

#include "RunLabel.h"

#include <Poco/Path.h>

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface to the current parameters functionality of the Engineering
Diffraction (EnggDiffraction) GUI. This can be used in different
tabs/widgets as well as in the main/central view. Normally this
interface will be provided by the presenters of the widgets (assuming
an MVP design). The individual / area specific tabs/widgets (their
presenters) will forward to the widget responsible for the
calibration.
*/
class IEnggDiffractionParam {
public:
  virtual ~IEnggDiffractionParam() = default;

  virtual Poco::Path outFilesUserDir(const std::string &addToDir) const = 0;

  /// Get the name of a HDF file for a given run number to save to
  virtual std::string userHDFRunFilename(const std::string runNumber) const = 0;

  /// Get the name of a HDF file for a range of runs
  virtual std::string
  userHDFMultiRunFilename(const std::vector<RunLabel> &runLabels) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIOPYTHONRUNNER_H_
