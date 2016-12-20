#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"

#include <QMutex>
#include <QObject>
#include <boost/scoped_ptr.hpp>

// Qt classes forward declarations
class QThread;
class QTimer;

namespace MantidQt {
namespace CustomInterfaces {

class TomoPathsConfig;
class TomoToolConfigDialogBase;

/**
Tomography GUI. Presenter for the GUI (as in the MVP
(Model-View-Presenter) pattern). In principle, in a strict MVP setup,
signals from the model should always be handled through this presenter
and never go directly to the view, and viceversa.

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
class MANTIDQT_CUSTOMINTERFACES_DLL TomographyIfacePresenter
    : public QObject,
      public ITomographyIfacePresenter {
  Q_OBJECT

public:
  /// Default constructor - normally used from the concrete view
  TomographyIfacePresenter(ITomographyIfaceView *view);
  ~TomographyIfacePresenter() override;

  void notify(ITomographyIfacePresenter::Notification notif) override;

protected:
  void initialize();

  /// clean shut down of model, view, etc.
  void cleanup();

  // notification methods
  void processSystemSettingsUpdated();
  void processSetupResourcesAndTools();
  void processCompResourceChanged();
  void processToolChanged();
  void processTomoPathsChanged();
  void processTomoPathsEditedByUser();
  void processLogin();
  void processLogout();
  void processSetupReconTool();
  void processRunRecon();
  void processCancelJobs();
  void processVisualizeJobs();
  void processViewImg();
  void processLogMsg();
  void processAggregateEnergyBands();
  void processShutDown();

  void doVisualize(const std::vector<std::string> &ids);

  /// auto-guess additional directories when the user gives the samples path
  void findFlatsDarksFromSampleGivenByUser(TomoPathsConfig &cfg);

  bool
  usableEnergyBandsPaths(const std::map<std::string, std::string> &algParams);

  /// Starts a periodic query just to keep sessions alive when logged in
  void startKeepAliveMechanism(int period);

  /// Stops/kills the periodic query (for example if the user logs out)
  void killKeepAliveMechanism();

  bool isLocalResourceSelected() const;
signals:
  void terminated();

protected slots:
  /// It may be run on user request, or periodically from a timer/thread
  void processRefreshJobs();
  void readWorkerStdOut(const QString &s);
  void readWorkerStdErr(const QString &s);
  void addProcessToJobList();
  void reconProcessFailedToStart();
  void workerFinished(const qint64 pid, const int exitCode);

private:
  /// Asks the user for permission to cancel the running reconstruction
  bool userConfirmationToCancelRecon();
  void setupAndRunLocalReconstruction(const std::string &runnable,
                                      const std::vector<std::string> &args,
                                      const std::string &allOpts);
  /// creates the correct dialog pointer and sets it to the member variable
  void createConfigDialogUsingToolName(const std::string &toolName);

  /// sets up the dialog and uses the settings to update the model
  void
  setupConfigDialogSettingsAndUpdateModel(TomoToolConfigDialogBase *dialog);

  /// configures up the dialog using the view
  void setupConfigDialogSettings(TomoToolConfigDialogBase &dialog);

  /// does the actual path configuration for local resource
  void setupConfigDialogSettingsForLocal(TomoToolConfigDialogBase &dialog);

  /// does the actual path configuration for remote resource
  void setupConfigDialogSettingsForRemote(TomoToolConfigDialogBase &dialog);

  /// update all the model information after the tool's been changed
  void updateModelAfterToolChanged(const TomoToolConfigDialogBase &dialog);

  /// update the model's current tool name using the dialog
  void updateModelCurrentToolName(const TomoToolConfigDialogBase &dialog);

  /// update the model's current tool method using the dialog
  void updateModelCurrentToolMethod(const TomoToolConfigDialogBase &dialog);

  /// update the model's current tool settings using the dialog
  void updateModelCurrentToolSettings(const TomoToolConfigDialogBase &dialog);

  /// Associated view for this presenter (MVP pattern)
  ITomographyIfaceView *const m_view;

  /// Associated model for this presenter (MVP pattern)
  const boost::scoped_ptr<TomographyIfaceModel> m_model;

  // TODO: replace this with an std::mutex. Also below for threads.
  // mutex for the job status info update operations on the view
  QMutex *m_statusMutex;

  // for periodic update of the job status table/tree
  QTimer *m_keepAliveTimer;

  std::unique_ptr<TomographyThread> m_workerThread;

  std::unique_ptr<TomoToolConfigDialogBase> m_configDialog;

  static const std::string g_defOutPathLocal;
  static const std::string g_defOutPathRemote;

  bool m_reconRunning = false;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_
