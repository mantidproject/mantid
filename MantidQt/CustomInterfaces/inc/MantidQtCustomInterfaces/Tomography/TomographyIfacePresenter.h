#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"

#include <boost/scoped_ptr.hpp>
#include <QMutex>
#include <QObject>

// Qt classes forward declarations
class QThread;
class QTimer;

namespace MantidQt {
namespace CustomInterfaces {

class TomoPathsConfig;

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
  // Q_OBJECT for the 'keep alive' signals
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

  void subprocessRunReconRemote();
  void subprocessRunReconLocal();

protected slots:
  /// It may be run on user request, or periodically from a timer/thread
  void processRefreshJobs();

protected:
  void processCancelJobs();
  void processVisualizeJobs();
  void processViewImg();
  void processLogMsg();
  void processAggregateEnergyBands();
  void processShutDown();

  void doVisualize(const std::vector<std::string> &ids);

  /// To prepare a local run
  void makeRunnableWithOptionsLocal(const std::string &comp, std::string &run,
                                    std::string &opt);

  /// auto-guess additional directories when the user gives the samples path
  void findFlatsDarksFromSampleGivenByUser(TomoPathsConfig &cfg);

  bool usableEnergyBandsPaths(const std::map<std::string, std::string> &algParams);

  /// Starts a periodic query just to keep sessions alive when logged in
  void startKeepAliveMechanism(int period);
  /// Stops/kills the periodic query (for example if the user logs out)
  void killKeepAliveMechanism();

private:
  /// Associated view for this presenter (MVP pattern)
  ITomographyIfaceView *const m_view;

  /// Associated model for this presenter (MVP pattern)
  const boost::scoped_ptr<TomographyIfaceModel> m_model;

  // TODO: replace this with an std::mutex. Also below for threads.
  // mutex for the job status info update operations on the view
  QMutex *m_statusMutex;

  // for periodic update of the job status table/tree
  QTimer *m_keepAliveTimer;
  QThread *m_keepAliveThread;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEPRESENTER_H_
