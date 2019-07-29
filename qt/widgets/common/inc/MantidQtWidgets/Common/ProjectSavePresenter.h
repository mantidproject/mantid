// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  /// Find out if a project needs a save warning.
  bool needsSizeWarning(std::vector<std::string> &wsNames);

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
