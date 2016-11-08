#ifndef MANTIDQTCUSTOMINTERFACES_PROJECTSAVEPRESENTER_H
#define MANTIDQTCUSTOMINTERFACES_PROJECTSAVEPRESENTER_H

#include "MantidAPI/Workspace.h"
#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtCustomInterfaces/ProjectSaveModel.h"
#include "MantidQtCustomInterfaces/IProjectSaveView.h"

#include <vector>


//------------------------------------------------

namespace MantidQt {
namespace CustomInterfaces {

/**
Implements a presenter for the project saving dialog.

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class ProjectSavePresenter
{
public:
  enum class Notification {
    UncheckWorkspace,
    CheckWorkspace
  };

  ProjectSavePresenter(IProjectSaveView* view);
  void notify(Notification notification);

private:
  void includeWindowsForCheckedWorkspace();
  void excludeWindowsForUncheckedWorkspace();

  /// Handle to the view for this presenter
  IProjectSaveView *m_view;
  ProjectSaveModel m_model;
};

} //CustomInterfaces
} //MantidQt

#endif // PROJECTSAVEPRESENTER_H
