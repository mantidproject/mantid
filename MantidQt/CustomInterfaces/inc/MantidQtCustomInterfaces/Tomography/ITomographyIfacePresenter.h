#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface for what the presenter of the tomography GUI needs to
implement. Here the term presenter is used as in the MVP
(Model-View-Presenter) pattern. The (passive) view will use this.

Copyright &copy; 2014-2016 ISIS Rutherford Appleton Laboratory, NScD
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
class ITomographyIfacePresenter {
public:
  virtual ~ITomographyIfacePresenter() {}

  /// These are user actions, triggered from the (passive) view, that need
  /// handling by the presenter
  enum Notification {
    SystemSettingsUpdated,  ///< Change in the system settings (local or remote)
    SetupResourcesAndTools, ///< Setup what are available, normally at startup
    CompResourceChanged,    ///< 'current' compute resource changed
    ToolChanged,      ///< 'current' tomographic reconstruction tool changed
    TomoPathsChanged, ///< user selects tomography data paths
    TomoPathsEditedByUser, ///< user edits manually paths that may modify other
                           ///paths
    LogInRequested,        ///< user hits 'log in' or similar
    LogOutRequested,       ///< user hits 'log out' or similar
    SetupReconTool,        ///< To start the setup (open dialog, etc.)
    RunReconstruct,        ///< user hits 'reconstruct' or similar
    RefreshJobs,           ///< get updated jobs info in the table/tree
    CancelJobFromTable,    ///< cancel one job from the list
    VisualizeJobFromTable, ///< open visualization window for one job
    ViewImg,               ///< user wants to view an image (2D)
    AggregateEnergyBands,  ///< run aggregate bands (in the background)
    LogMsg,                ///< need to send a message to the Mantid log system
    ShutDown               ///< closing the interface
  };

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(ITomographyIfacePresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEPRESENTER_H_