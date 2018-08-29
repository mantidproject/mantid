#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEPRESENTER_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEPRESENTER_H

#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/IProjectSaveView.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/ProjectSaveModel.h"

#include "DllOption.h"
#include <vector>

//------------------------------------------------

namespace MantidQt {
namespace MantidWidgets {

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
class EXPORT_OPT_MANTIDQT_COMMON ProjectSavePresenter {
public:
  enum class Notification {
    UncheckWorkspace,
    CheckWorkspace,
    PrepareProjectFolder
  };

  /// Construct a new presenter with a view
  ProjectSavePresenter(IProjectSaveView *view);
  /// Notify the presenter to do something
  void notify(Notification notification);

private:
  /// Update the view to add included windows for a workspace
  void includeWindowsForCheckedWorkspace();
  /// Update the view to add excluded windows for a workspace
  void excludeWindowsForUncheckedWorkspace();
  /// Prepare a project folder given the path
  void prepareProjectFolder();

  // Instance Variables

  /// Handle to the view for this presenter
  IProjectSaveView *m_view;
  /// Hold an instance of the model
  ProjectSaveModel m_model;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // PROJECTSAVEPRESENTER_H
