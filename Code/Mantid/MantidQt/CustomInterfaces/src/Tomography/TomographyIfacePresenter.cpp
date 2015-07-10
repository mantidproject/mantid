#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <boost/lexical_cast.hpp>

#include <QFileInfo>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QTimer>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

TomographyIfacePresenter::TomographyIfacePresenter(ITomographyIfaceView *view)
    : m_view(view), m_model(new TomographyIfaceModel()),
      m_statusMutex(NULL), m_keepAliveTimer(NULL), m_keepAliveThread(NULL) {
  if (!m_view) {
    throw std::runtime_error("Severe inconsistency found. Presenter created "
                             "with an empty/null view (tomography interface). "
                             "Cannot continue.");
  }
  m_statusMutex = new QMutex();
}

TomographyIfacePresenter::~TomographyIfacePresenter() {
  cleanup();

  if (m_keepAliveThread)
    delete m_keepAliveThread;

  if (m_keepAliveTimer)
    delete m_keepAliveTimer;

  if (m_statusMutex)
    delete m_statusMutex;
}

/**
 * Close open sessions, kill timers/threads etc., save settings, etc. for a
 * graceful window close/destruct
 */
void TomographyIfacePresenter::cleanup() {
  killKeepAliveMechanism();
  m_model->cleanup();
}

void TomographyIfacePresenter::notify(
    ITomographyIfacePresenter::Notification notif) {

  switch (notif) {

  case ITomographyIfacePresenter::SetupResourcesAndTools:
    processSetup();
    break;

  case ITomographyIfacePresenter::CompResourceChanged:
    processCompResourceChange();
    break;

  case ITomographyIfacePresenter::ToolChanged:
    processToolChange();
    break;

  case ITomographyIfacePresenter::TomoPathsChanged:
    processTomoPathsChanged();
    break;

  case ITomographyIfacePresenter::LogInRequested:
    processLogin();
    break;

  case ITomographyIfacePresenter::LogOutRequested:
    processLogout();
    break;

  case ITomographyIfacePresenter::SetupReconTool:
    processSetupReconTool();
    break;

  case ITomographyIfacePresenter::RunReconstruct:
    processRunRecon();
    break;

  case ITomographyIfacePresenter::RefreshJobs:
    processRefreshJobs();
    break;

  case ITomographyIfacePresenter::CancelJobFromTable:
    processCancelJobs();
    break;

  case ITomographyIfacePresenter::VisualizeJobFromTable:
    processVisualizeJobs();
    break;

  case ITomographyIfacePresenter::ViewImg:
    processViewImg();
    break;

  case ITomographyIfacePresenter::LogMsg:
    processLogMsg();
    break;

  case ITomographyIfacePresenter::ShutDown:
    processShutDown();
    break;
  }
}

void TomographyIfacePresenter::processSetup() {
  try {
    m_model->setupComputeResource();
    if (m_model->computeResources().size() < 1) {
      m_view->userWarning("No remote compute resource could be set up!",
                          "No remote compute resource has been set up. Please "
                          "note that without a "
                          "remote compute resource the functionality of this "
                          "interface might be limited.");
    }
    m_model->setupRunTool("");
    processTomoPathsChanged();

    m_view->enableLoggedActions(!m_model->loggedIn().empty());

    m_view->setComputeResources(m_model->computeResources(),
                                m_model->computeResourcesStatus());
    m_view->setReconstructionTools(m_model->reconTools(),
                                   m_model->reconToolsStatus());
  } catch (std::runtime_error &e) {
    const std::string msg =
        "Failed to initialize remote compute resource(s). This "
        "custom interface will not work. Error description: " +
        std::string(e.what());
    m_view->userError("Fatal error", msg);
    m_model->logMsg(msg);
  }
}

void TomographyIfacePresenter::processCompResourceChange() {
  const std::string comp = m_view->currentComputeResource();

  m_model->setupRunTool(comp);
}

void TomographyIfacePresenter::processToolChange() {
  const std::string tool = m_view->currentReconTool();

  // disallow reconstruct on tools that don't run yet: Savu and CCPi
  if (TomographyIfaceModel::g_CCPiTool == tool) {
    m_view->enableRunReconstruct(false);
    m_view->enableConfigTool(false);
  } else if (TomographyIfaceModel::g_SavuTool == tool) {
    // for now, show setup dialog, but cannot run
    m_view->enableRunReconstruct(false);
    m_view->enableConfigTool(true);
  } else {
    m_view->enableRunReconstruct(!(m_model->loggedIn().empty()));
    m_view->enableConfigTool(true);
  }

  m_model->usingTool(tool);
}

void TomographyIfacePresenter::processTomoPathsChanged() {
  m_model->updateTomoPathsConfig(m_view->currentPathsConfig());
}

void TomographyIfacePresenter::processLogin() {
  if (!m_model->loggedIn().empty()) {
    m_view->userError(
        "Better to logout before logging in again",
        "You're currently logged in. Please, log out before logging in "
        "again if that's what you meant.");
  }

  const std::string compRes = m_view->currentComputeResource();
  try {
    const std::string user = m_view->getUsername();
    if (user.empty()) {
      m_view->userError(
          "Cannot log in",
          "To log in you need to specify a username (and a password!).");
      return;
    }

    m_model->doLogin(compRes, m_view->getUsername(), m_view->getPassword());
  } catch (std::exception &e) {
    throw(std::string("Problem when logging in. Error description: ") +
          e.what());
  }

  m_view->updateLoginControls(true);
  m_view->enableLoggedActions(!m_model->loggedIn().empty());

  try {
    m_model->doRefreshJobsInfo(compRes);
  } catch (std::exception &e) {
    throw(std::string("The login operation went apparently fine but an issue "
                      "was found while trying to retrieve the status of the "
                      "jobs currently running on the remote resource. Error "
                      "description: ") +
          e.what());
  }

  startKeepAliveMechanism(m_view->keepAlivePeriod());
  // show table for the first time
  processRefreshJobs();
}

void TomographyIfacePresenter::processLogout() {
  if (m_model->loggedIn().empty()) {
    m_model->logMsg("Cannot log out: not logged into any resource.");
    return;
  }

  try {
    m_model->doLogout(m_view->currentComputeResource(), m_view->getUsername());
  } catch (std::exception &e) {
    throw(std::string("Problem when logging out. Error description: ") +
          e.what());
  }

  m_view->updateLoginControls(false);
}

void TomographyIfacePresenter::processSetupReconTool() {
  if (TomographyIfaceModel::g_CCPiTool != m_view->currentReconTool()) {
    m_view->showToolConfig(m_view->currentReconTool());
    m_model->updateReconToolsSettings(m_view->reconToolsSettings());
  }
}

void TomographyIfacePresenter::processRunRecon() {
  if (m_model->loggedIn().empty())
    return;

  const std::string &resource = m_view->currentComputeResource();

  if (m_model->localComputeResource() != resource) {
    try {
      m_model->doSubmitReconstructionJob(resource);
    } catch (std::exception &e) {
      m_view->userWarning("Issue when trying to start a job", e.what());
    }

    processRefreshJobs();
  }
}

void TomographyIfacePresenter::processRefreshJobs() {
  if (m_model->loggedIn().empty())
    return;

  const std::string &comp = m_view->currentComputeResource();
  m_model->doRefreshJobsInfo(comp);

  {
    // update widgets from that info
    QMutexLocker lockit(m_statusMutex);

    m_view->updateJobsInfoDisplay(m_model->jobsStatus());
  }
}

void TomographyIfacePresenter::processCancelJobs() {
  if (m_model->loggedIn().empty())
    return;

  const std::string &resource = m_view->currentComputeResource();
  if (m_model->localComputeResource() != resource) {
    m_model->doCancelJobs(resource, m_view->processingJobsIDs());
  }
}

void TomographyIfacePresenter::processVisualizeJobs() {
  doVisualize(m_view->processingJobsIDs());
}

void
TomographyIfacePresenter::doVisualize(const std::vector<std::string> &ids) {
  m_model->logMsg(" Visualizing results from job: " + ids.front());
  // TODO: open dialog, send to Paraview, etc.
}

void TomographyIfacePresenter::processLogMsg() {
  std::vector<std::string> msgs = m_view->logMsgs();
  for (size_t i = 0; i < msgs.size(); i++) {
    m_model->logMsg(msgs[i]);
    break;
  }
}

void TomographyIfacePresenter::processShutDown() {
  m_view->saveSettings();
  cleanup();
}

void TomographyIfacePresenter::processViewImg() {
  std::string ip = m_view->showImagePath();
  QString suf = QFileInfo(QString::fromStdString(ip)).suffix();
  // This is not so great, as we check extensions and not really file
  // content/headers, as it should be.
  if ((0 == QString::compare(suf, "fit", Qt::CaseInsensitive)) ||
      (0 == QString::compare(suf, "fits", Qt::CaseInsensitive))) {
    WorkspaceGroup_sptr wsg = m_model->loadFITSImage(ip);
    if (!wsg)
      return;
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));
    if (!ws)
      return;

    m_view->showImage(ws);

    // clean-up container group workspace
    if (wsg)
      AnalysisDataService::Instance().remove(wsg->getName());
  } else if ((0 == QString::compare(suf, "tif", Qt::CaseInsensitive)) ||
             (0 == QString::compare(suf, "tiff", Qt::CaseInsensitive)) ||
             (0 == QString::compare(suf, "png", Qt::CaseInsensitive))) {
    m_view->showImage(ip);
  } else {
    m_view->userWarning(
        "Failed to load image - format issue",
        "Could not load image because the extension of the file " + ip +
            ", suffix: " + suf.toStdString() +
            " does not correspond to FITS or TIFF files.");
  }
}

void TomographyIfacePresenter::startKeepAliveMechanism(int period) {
  if (period <= 0) {
    m_model->logMsg("Tomography GUI: not starting the keep-alive mechanism. "
                    "You might be logged out by the remote compute resource "
                    "after some minutes "
                    "depending on system configuration.");
    return;
  }

  m_model->logMsg(
      "Tomography GUI: starting mechanism to periodically query the "
      "status of jobs. This will update the status of running jobs every " +
      boost::lexical_cast<std::string>(period) +
      " seconds. You can also update it at any moment by clicking "
      "on the refresh button. This periodic update mechanism is "
      "also expected to keep sessions on remote compute resources "
      "alive after logging in.");

  if (m_keepAliveThread)
    delete m_keepAliveThread;
  QThread *m_keepAliveThread = new QThread();

  if (m_keepAliveTimer)
    delete m_keepAliveTimer;
  m_keepAliveTimer = new QTimer(NULL); // no-parent so it can be moveToThread

  m_keepAliveTimer->setInterval(1000 * period); // interval in ms
  m_keepAliveTimer->moveToThread(m_keepAliveThread);
  // direct connection from the thread
  connect(m_keepAliveTimer, SIGNAL(timeout()), SLOT(processRefreshJobs()),
          Qt::DirectConnection);
  QObject::connect(m_keepAliveThread, SIGNAL(started()), m_keepAliveTimer,
                   SLOT(start()));
  m_keepAliveThread->start();
}

void TomographyIfacePresenter::killKeepAliveMechanism() {
  if (m_keepAliveTimer)
    m_keepAliveTimer->stop();
}

} // namespace CustomInterfaces
} // namespace MantidQt
