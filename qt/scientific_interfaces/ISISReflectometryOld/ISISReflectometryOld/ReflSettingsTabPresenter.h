#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflSettingsPresenter;

/** @class ReflSettingsTabPresenter

ReflSettingsTabPresenter is a presenter class for the tab 'Settings' in the
ISIS Reflectometry Interface.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ReflSettingsTabPresenter
    : public IReflSettingsTabPresenter {
public:
  /// Constructor
  ReflSettingsTabPresenter(std::vector<IReflSettingsPresenter *> presenters);
  /// Destructor
  ~ReflSettingsTabPresenter() override;
  /// Set the instrument name
  void setInstrumentName(const std::string &instName) override;

  /// Returns values passed for 'Transmission run(s)'
  std::string getTransmissionRuns(int group, bool loadRuns) const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions(int group) const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions(int group) const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions(int group) const override;

private:
  /// The presenters for each group as a vector
  std::vector<IReflSettingsPresenter *> m_settingsPresenters;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H */
