#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
// #include "MantidAPI/WorkspaceGroup_fwd.h"

/* namespace Mantid { */
/* namespace API { */
/* class IRemoteJobManager; */
/* struct IRemoteJobManager::RemoteJobInfo; */
/* } */
/* } */

namespace MantidQt {
namespace CustomInterfaces {

/**
Tomography GUI. Base class / interface for the view of the tomo GUI
(view in the sense of the MVP pattern). This class is Qt-free. Qt
specific functionality/dependencies are added in a class derived from
this.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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
class ITomographyIfaceView {

public:
  ITomographyIfaceView() {}
  virtual ~ITomographyIfaceView() {}

  /**
   * Display a warning to the user (for example as a pop-up window).
   *
   * @param err warning title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userWarning(const std::string &warn,
                           const std::string &description) = 0;

  /**
   * Display an error message (for example as a pop-up window).
   *
   * @param err Error title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userError(const std::string &err,
                         const std::string &description) = 0;

  virtual std::vector<std::string> logMsgs() const = 0;

  /**
   * Set the compute resources available (remote and/or local). Note
   * that the order of the list is/can be important.
   *
   * @param resources identifiers for the resources (their display names
   * normally). Order matters
   *
   * @param enabled tell if the resources are enabled or usable (which
   * should have an effect on how they are displayed and if they can be
   * used (for example, selected) in the interface.
   */
  virtual void setComputeResources(const std::vector<std::string> &resources,
                                   const std::vector<bool> &enabled) = 0;

  /**
   * Set the tools available on a compute resource.
   *
   * @param resources identifiers of the tools that can or could be run.
   * Order matters
   *
   * @param enabled tells if the tools are actually usable (which
   * should have an effect on how they are displayed and if they can
   * be used in the GUI (for example, if they can be selected at all).
   */
  virtual void setReconstructionTools(const std::vector<std::string> &tools,
                                      const std::vector<bool> &enabled) = 0;

  /// Save settings (normally when closing the interface)
  virtual void saveSettings() const = 0;

  virtual std::string getUsername() const = 0;
  virtual std::string getPassword() const = 0;

  virtual std::vector<std::string> processingJobsIDs() const = 0;

  /**
   * Gets the compute resource that is currently selected by the user.
   * This calls a validation method that can throw in case of
   * inconsistencies.
   *
   * @return Name of the compute resource as a string.
   */
  virtual std::string currentComputeResource() const = 0;

  virtual std::string currentReconTool() const = 0;

  /// updates buttons and banners related to the current login status
  virtual void updateLoginControls(bool loggedIn) = 0;

  virtual void enableLoggedActions(bool enable) = 0;

  virtual void enableConfigTool(bool on) = 0;

  virtual void enableRunReconstruct(bool on) = 0;

  virtual std::string showImagePath() = 0;
  /// draw an image on the visualization tab/interface
  virtual void showImage(const Mantid::API::MatrixWorkspace_sptr &wsg) = 0;
  virtual void showImage(const std::string &path) = 0;

  /// Show a tool specific configuration dialog for the user to set it up
  virtual void showToolConfig(const std::string &name) = 0;

  /// Refresh the table, tree etc. that display info on the running/finished
  /// jobs
  virtual void updateJobsInfoDisplay(const std::vector<
      Mantid::API::IRemoteJobManager::RemoteJobInfo> &status) = 0;

  /// Keep alive period for the remote compute resources. 0 == disabled by
  /// default
  virtual int keepAlivePeriod() { return 0; }
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_
