#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"

// Qt classes forward declarations
class QMutex;

namespace MantidQt {
namespace CustomInterfaces {

/**
Tomography GUI. Model for the interface (as in the MVP
(Model-View-Presenter) pattern). In principle, in a strict MVP setup,
signals from this model should always be handled through the presenter
and never go directly to the view.

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
class DLLExport TomographyIfaceModel {
  // For now, there is no need to  make this a Q_OBJECT (and derive from
  // QObject, but this class may become a "full" Q_OBJECT as soon as a Qt
  // model (table, tree, etc.), with its signals, is used here
  // Q_OBJECT

public:
  /// Default constructor
  TomographyIfaceModel();
  virtual ~TomographyIfaceModel();

  void setupComputeResource();
  void setupRunTool(const std::string &compRes);

  /// the compure resources supported and their status/availability
  std::vector<std::string> computeResources() { return m_computeRes; }
  std::vector<bool> computeResourcesStatus() { return m_computeResStatus; }

  /// the reconstruction tools supported and their status/availability
  std::vector<std::string> reconTools() { return m_reconTools; }
  std::vector<bool> reconToolsStatus() { return m_reconToolsStatus; }

  // Mantid::Kernel::Logger & logger() const;

  const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
  jobsStatus() const {
    return m_jobsStatus;
  }

  /// Username last logged in, if any.
  std::string loggedIn() const { return m_loggedIn; }
  // TODO: add companion currentComputeResource where LoggedIn() is in
  void usingTool(const std::string &tool) { m_currentTool = tool; }
  std::string usingTool() const { return m_currentTool; }

  bool doPing(const std::string &compRes);
  void doLogin(const std::string &compRes, const std::string &user,
               const std::string &pw);
  void doLogout(const std::string &compRes, const std::string &username);
  void doQueryJobStatus(const std::string &compRes,
                        std::vector<std::string> &ids,
                        std::vector<std::string> &names,
                        std::vector<std::string> &status,
                        std::vector<std::string> &cmds);
  void doSubmitReconstructionJob(const std::string &compRes);
  void doCancelJobs(const std::string &compRes,
                    const std::vector<std::string> &id);
  void doRefreshJobsInfo(const std::string &compRes);

  /// loads an image/picture in FITS format
  Mantid::API::WorkspaceGroup_sptr loadFITSImage(const std::string &path);

  void logMsg(const std::string &msg);

  void cleanup();

  std::string localComputeResource() const { return m_localCompName; }


  // current paths set by the user
  // TODO: make path settings struct available to this and View Interface
  std::string currentPathSCARF() const { return ""; }
  std::string currentPathFITS() const { return ""; }
  std::string currentPathFlat() const { return ""; }
  std::string currentPathDark() const { return ""; }
  std::string currentPathSavuConfig() const { return ""; }


  std::string m_currentTool;

  // Names of image reconstruction tools
  static const std::string g_TomoPyTool;
  static const std::string g_AstraTool;
  static const std::string g_CCPiTool;
  static const std::string g_SavuTool;
  static const std::string g_customCmdTool;

private:
  void getJobStatusInfo(const std::string &compRes);

  std::string validateCompResource(const std::string &res);

  /// makes the command line string to run on the remote/local
  void makeRunnableWithOptions(const std::string &comp, std::string &run,
                               std::string &opt);

  void checkWarningToolNotSetup(const std::string &tool,
                                const std::string &settings);

  void splitCmdLine(const std::string &cmd, std::string &run,
                    std::string &opts);

  void checkDataPathsSet();

  /// username last logged in (empty if not in login status), from local
  /// perspective
  std::string m_loggedIn;

  /// facility for the remote compute resource
  const std::string m_facility;
  /// display name of the "local" compute resource
  const std::string m_localCompName;

  /// compute resources suppoted by this GUI (remote ones, clusters, etc.)
  std::vector<std::string> m_computeRes;
  /// status of the compute resources: availalbe/unavailable or usable/unusable
  std::vector<bool> m_computeResStatus;
  /// identifiers of reconstruction tools supported, given a compute resource
  std::vector<std::string> m_reconTools;
  std::vector<bool> m_reconToolsStatus;

  // status of remote jobs
  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> m_jobsStatus;

  /// reconstruction tools available on SCARF
  std::vector<std::string> m_SCARFtools;

  /// file paths, base dir on scarf
  std::string m_pathSCARFbase;
  /// path to fits file (sample data)
  std::string m_pathFITS;
  /// path to flat/open beam/bright image
  std::string m_pathFlat;
  /// path to dark image
  std::string m_pathDark;

  // Name of the remote compute resource
  static const std::string g_SCARFName;

  /// Settings for the third party (tomographic reconstruction) tools
  struct UserToolsSettings {
    // This is just too basic at the moment. We probably want to store
    // here the real settings objects, and rather than this horror have a
    // dictionary of tools-settings
    std::string tomoPy;
    std::string astra;
    std::string CCPi;
    std::string savu;
    std::string custom;
  };
  UserToolsSettings m_toolsSettings;

  // mutex for the job status info update operations
  QMutex *m_statusMutex;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_
