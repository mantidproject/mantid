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

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IEnggDiffractionParam {
public:
  virtual ~IEnggDiffractionParam() = default;

  virtual Poco::Path outFilesUserDir(const std::string &addToDir) const = 0;

  /// Get the name of a HDF file for a given run number to save to
  virtual std::string userHDFRunFilename(const int runNumber) const = 0;

  /// Get the name of a HDF file for a range of runs
  virtual std::string
  userHDFMultiRunFilename(const std::vector<RunLabel> &runLabels) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIOPYTHONRUNNER_H_
