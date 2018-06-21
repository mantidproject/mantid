#ifndef MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H

#include "DllConfig.h"
#include "IReflMainWindowPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowView;
class IReflRunsTabPresenter;
class IReflEventTabPresenter;
class IReflSettingsTabPresenter;
class IReflSaveTabPresenter;

/** @class ReflMainWindowPresenter

ReflMainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IReflMainWindowPresenter.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflMainWindowPresenter
    : public IReflMainWindowPresenter {
public:
  /// Constructor
  ReflMainWindowPresenter(IReflMainWindowView *view)
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;
private:
  IReflMainWindowView* m_view;
  void notify(IReflMainWIndowPresenter::Flag flag) override;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H */
