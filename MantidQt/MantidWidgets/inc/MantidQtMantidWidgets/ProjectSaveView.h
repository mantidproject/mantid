#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H

#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtMantidWidgets/IProjectSaveView.h"
#include "ui_ProjectSave.h"

#include <QWidget>
#include <QMainWindow>
#include <string>
#include <vector>
#include <set>

namespace MantidQt {
namespace MantidWidgets {

class ProjectSavePresenter;

/** @class ProjectSaveView

ProjectSaveView is the interaces for defining the functions that the project
save view needs to implement.

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
class ProjectSaveView : public QDialog, IProjectSaveView {
public:

  ProjectSaveView(const std::vector<MantidQt::API::IProjectSerialisable*> &windows, QWidget *parent = nullptr);

  std::vector<MantidQt::API::IProjectSerialisable*> getWindows() override;
  std::vector<std::string> getCheckedWorkspaceNames() override;
  std::vector<std::string> getUncheckedWorkspaceNames() override;
  void updateWorkspacesList(const std::vector<std::string>& workspaces) override;
  void updateIncludedWindowsList(const std::vector<std::string>& windows) override;
  void updateExcludedWindowsList(const std::vector<std::string>& windows) override;

private:
    std::vector<std::string> getItemsWithCheckState(const Qt::CheckState state) const;
    std::vector<MantidQt::API::IProjectSerialisable*> m_serialisableWindows;
    std::unique_ptr<ProjectSavePresenter> m_presenter;
    Ui::ProjectSave m_ui;
};
}
}
#endif /* MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H */
