#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONFILTERSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONFILTERSETTINGS_H_

#include "MantidQtCustomInterfaces/Tomography/TomoReconPreprocSettings.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconPostprocSettings.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings for pre-/post-/other-processing filters to run a
reconstruction jobs. Note this has been defined as general processing
steps, not tied to any particular tool.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
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
struct MANTIDQT_CUSTOMINTERFACES_DLL TomoReconFiltersSettings {
  MantidQt::CustomInterfaces::TomoReconPreprocSettings prep;
  MantidQt::CustomInterfaces::TomoReconPostprocSettings postp;

  /// whether to write pre-processed images in addition to the output
  /// reconstructed volume (slices/images)
  bool outputPreprocImages;

  TomoReconFiltersSettings() : prep(), postp(), outputPreprocImages(true) {}
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONFILTERSETTINGS_H_
