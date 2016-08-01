#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflSettingsTabView;

/** @class ReflSettingsTabPresenter

ReflSettingsTabPresenter is a presenter class for the tab 'Settings' in the
Reflectometry (Polref) Interface.

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
  ReflSettingsTabPresenter(IReflSettingsTabView *view);
  /// Destructor
  ~ReflSettingsTabPresenter() override;
  /// Accept a main presenter
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;

  /// Returns global options for 'Plus' algorithm
  std::string getPlusOptions() const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;

private:
  void createPlusHints();
  void createTransmissionHints();
  void createReductionHints();
  void createStitchHints();

  /// The view we are managing
  IReflSettingsTabView *m_view;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTER_H */
