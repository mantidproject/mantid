#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/RemoteJobManager.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/TomoReconstruction/TomoReconstruction.h"

#include <boost/lexical_cast.hpp>

#include <QElapsedTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QThread>
#include <QTimer>

using namespace Mantid::API;

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(TomoReconstruction)

namespace {
Mantid::Kernel::Logger g_log("TomoReconstruction");
}

class OwnTreeWidgetItem : public QTreeWidgetItem {
public:
  OwnTreeWidgetItem(QTreeWidgetItem *parent,
                    QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(parent), m_rootParent(logicalParent), m_key(key) {}

  OwnTreeWidgetItem(QStringList list, QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(list), m_rootParent(logicalParent), m_key(key) {}

  OwnTreeWidgetItem(QTreeWidgetItem *parent, QStringList list,
                    QTreeWidgetItem *logicalParent = NULL,
                    const std::string &key = "")
      : QTreeWidgetItem(parent, list), m_rootParent(logicalParent), m_key(key) {
  }

  QTreeWidgetItem *getRootParent() { return m_rootParent; }

  std::string getKey() { return m_key; }

private:
  QTreeWidgetItem *m_rootParent;
  std::string m_key;
};

TomoToolConfigTomoPy::TomoToolConfigTomoPy(QWidget *parent) : QDialog(parent) {}

TomoToolConfigSavu::TomoToolConfigSavu(QWidget *parent) : QMainWindow(parent) {}

TomoToolConfigAstra::TomoToolConfigAstra(QWidget *parent) : QDialog(parent) {}

TomoToolConfigCustom::TomoToolConfigCustom(QWidget *parent) : QDialog(parent) {}

using namespace MantidQt::CustomInterfaces;

size_t TomoReconstruction::m_nameSeqNo = 0;

// names by which we know compute resourcess
const std::string TomoReconstruction::m_SCARFName = "SCARF@STFC";

// names by which we knoe image/tomography reconstruction tools (3rd party)
const std::string TomoReconstruction::m_TomoPyTool = "TomoPy";
const std::string TomoReconstruction::m_AstraTool = "Astra";
const std::string TomoReconstruction::m_CCPiTool = "CCPi CGLS";
const std::string TomoReconstruction::m_SavuTool = "Savu";
const std::string TomoReconstruction::m_CustomCmdTool = "Custom command";

/**
 * Almost default constructor, but note that this interface currently
 * relies on the SCARF cluster (only in ISIS facility) as the only
 * supported remote compute resource.
 *
 * @param parent Parent window (most likely the Mantid main app window).
 */
TomoReconstruction::TomoReconstruction(QWidget *parent)
    : UserSubWindow(parent), m_loggedIn(false), m_facility("ISIS"),
      m_computeRes(), m_localCompName("Local"), m_SCARFtools(),
      m_pathSCARFbase("/work/imat/recon/"),
      m_pathFITS(m_pathSCARFbase + "data/fits"),
      m_pathFlat(m_pathSCARFbase + "data/flat"),
      m_pathDark(m_pathSCARFbase + "data/dark"), m_currentParamPath(),
      m_settingsGroup("CustomInterfaces/TomoReconstruction"),
      m_keepAliveTimer(NULL), m_keepAliveThread(NULL) {

  m_computeRes.push_back(m_SCARFName);

  m_SCARFtools.push_back(m_TomoPyTool);
  m_SCARFtools.push_back(m_AstraTool);
  m_SCARFtools.push_back(m_CCPiTool);
  m_SCARFtools.push_back(m_SavuTool);
  m_SCARFtools.push_back(m_CustomCmdTool);

  m_availPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_availPlugins->addColumns("str", "name", 4);
  m_currPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_currPlugins->addColumns("str", "name", 4);
}

TomoReconstruction::~TomoReconstruction() {
  cleanup();
  delete m_keepAliveTimer;
  delete m_keepAliveThread;
}

/**
 * Close open sessions, kill timers/threads etc., save settings, etc. for a
 * graceful window close/destruct
 */
void TomoReconstruction::cleanup() {
  killKeepAliveMechanism();

  saveSettings();

  // be tidy and always log out if we're in.
  if (m_loggedIn)
    SCARFLogoutClicked();
}

void TomoReconstruction::doSetupSectionParameters() {
  // TODO: should split the tabs out into their own files?

  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(100);
  sizes.push_back(200);
  m_uiSavu.splitterPlugins->setSizes(sizes);

  // Setup Parameter editor tab
  loadAvailablePlugins();
  m_uiSavu.treeCurrentPlugins->setHeaderHidden(true);

  // Lists/trees
  connect(m_uiSavu.listAvailablePlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(availablePluginSelected()));
  connect(m_uiSavu.treeCurrentPlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(currentPluginSelected()));
  connect(m_uiSavu.treeCurrentPlugins, SIGNAL(itemExpanded(QTreeWidgetItem *)),
          this, SLOT(expandedItem(QTreeWidgetItem *)));

  // Buttons
  connect(m_uiSavu.btnTransfer, SIGNAL(released()), this,
          SLOT(transferClicked()));
  connect(m_uiSavu.btnMoveUp, SIGNAL(released()), this, SLOT(moveUpClicked()));
  connect(m_uiSavu.btnMoveDown, SIGNAL(released()), this,
          SLOT(moveDownClicked()));
  connect(m_uiSavu.btnRemove, SIGNAL(released()), this, SLOT(removeClicked()));

  // Connect slots
  // Menu Items
  connect(m_ui.actionOpen, SIGNAL(triggered()), this, SLOT(menuOpenClicked()));
  connect(m_ui.actionSave, SIGNAL(triggered()), this, SLOT(menuSaveClicked()));
  connect(m_ui.actionSaveAs, SIGNAL(triggered()), this,
          SLOT(menuSaveAsClicked()));
}

void TomoReconstruction::doSetupSectionSetup() {
  // disable 'local' for now
  m_ui.tabWidget_comp_resource->setTabEnabled(false, 1);
  m_ui.tab_local->setEnabled(false);

  m_ui.groupBox_run_config->setEnabled(false);

  connect(m_ui.pushButton_SCARF_login, SIGNAL(released()), this,
          SLOT(SCARFLoginClicked()));
  connect(m_ui.pushButton_SCARF_logout, SIGNAL(released()), this,
          SLOT(SCARFLogoutClicked()));

  // 'browse' buttons
  connect(m_ui.pushButton_fits_dir, SIGNAL(released()), this,
          SLOT(fitsPathBrowseClicked()));
  connect(m_ui.pushButton_flat_dir, SIGNAL(released()), this,
          SLOT(flatPathBrowseClicked()));
  connect(m_ui.pushButton_dark_dir, SIGNAL(released()), this,
          SLOT(darkPathBrowseClicked()));
}

void TomoReconstruction::doSetupSectionRun() {
  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(420);
  sizes.push_back(80);
  m_ui.splitter_run_main_vertical->setSizes(sizes);

  sizes[0] = 470;
  sizes[1] = 30;
  m_ui.splitter_image_resource->setSizes(sizes);

  sizes[0] = 400;
  sizes[1] = 100;
  m_ui.splitter_run_jobs->setSizes(sizes);

  setupComputeResource();
  setupRunTool();

  m_ui.label_image_name->setText("none");

  enableLoggedActions(m_loggedIn);

  // Button signals
  connect(m_ui.pushButton_browse_image, SIGNAL(released()), this,
          SLOT(browseImageClicked()));
  connect(m_ui.pushButton_reconstruct, SIGNAL(released()), this,
          SLOT(reconstructClicked()));
  connect(m_ui.pushButton_run_tool_setup, SIGNAL(released()), this,
          SLOT(toolSetupClicked()));
  connect(m_ui.pushButton_run_refresh, SIGNAL(released()), this,
          SLOT(jobTableRefreshClicked()));
  connect(m_ui.pushButton_run_job_visualize, SIGNAL(released()), this,
          SLOT(runVisualizeClicked()));
  connect(m_ui.pushButton_run_job_cancel, SIGNAL(released()), this,
          SLOT(jobCancelClicked()));

  // update tools for a resource
  connect(m_ui.comboBox_run_compute_resource, SIGNAL(currentIndexChanged(int)),
          this, SLOT(compResourceIndexChanged(int)));

  connect(m_ui.comboBox_run_tool, SIGNAL(currentIndexChanged(int)), this,
          SLOT(runToolIndexChanged(int)));

  m_ui.pushButton_reconstruct->setEnabled(false);
  m_ui.pushButton_run_tool_setup->setEnabled(false);
  m_ui.pushButton_run_job_cancel->setEnabled(false);
  m_ui.pushButton_run_job_visualize->setEnabled(false);
}

void TomoReconstruction::doSetupGeneralWidgets() {
  connect(m_ui.pushButton_help, SIGNAL(released()), this, SLOT(openHelpWin()));
  // note connection to the parent window, otherwise you'll be left
  // with an empty frame window
  connect(m_ui.pushButton_close, SIGNAL(released()), this->parent(),
          SLOT(close()));
}

void TomoReconstruction::initLayout() {
  m_ui.setupUi(this);

  readSettings();

  doSetupGeneralWidgets();
  doSetupSectionSetup();
  doSetupSectionRun();
}

/**
 * Enables/disables buttons that require the user to be logged into
 * the (remote) compute resource, for example: reconstruct (submit job),
 * cancel job, etc.
 */
void TomoReconstruction::enableLoggedActions(bool enable) {
  // TODO: this may not make sense anymore when/if the "Local" compute
  // resource is used in the future (except when none of the tools
  // supported are available/detected on "Local")
  std::vector<QPushButton *> buttons;
  buttons.push_back(m_ui.pushButton_run_refresh);
  buttons.push_back(m_ui.pushButton_run_job_cancel);
  // no visualization yet, need vsi etc. support
  // buttons.push_back(m_ui.pushButton_run_job_visualize);
  buttons.push_back(m_ui.pushButton_reconstruct);

  for (size_t i = 0; i < buttons.size(); ++i) {
    buttons[i]->setEnabled(enable);
  }

  if (!enable) {
    m_ui.pushButton_reconstruct->setToolTip(
        "Start reconstruction job. You need to be logged in to use this");
  } else {
    m_ui.pushButton_reconstruct->setToolTip("");
  }
}

/**
 * Handle display of the current status of the remote/local compute resource
 * that is selected by the user.
 *
 * @param online whether to show good/working/online status
 */
void TomoReconstruction::updateCompResourceStatus(bool online) {
  const std::string res = getComputeResource();
  if (res == m_SCARFName) {
    if (online)
      m_ui.pushButton_remote_status->setText("Online");
    else
      m_ui.pushButton_remote_status->setText("Offline");
  } else if (res == m_localCompName) {
    if (online)
      m_ui.pushButton_remote_status->setText("Tools available");
    else
      m_ui.pushButton_remote_status->setText("No tools available!");
  }
}

void TomoReconstruction::SCARFLoginClicked() {
  try {
    doLogin(getPassword());
    m_loggedIn = true;
  } catch (std::exception &e) {
    throw(std::string("Problem when logging in. Error description: ") +
          e.what());
  }

  try {
    jobTableRefreshClicked();
  } catch (std::exception &e) {
    throw(std::string("The login operation went apparently fine but an issue "
                      "was found while trying to retrieve the status of the "
                      "jobs currently running on the remote resource. Error "
                      "description: ") +
          e.what());
  }

  enableLoggedActions(true);
  updateCompResourceStatus(true);

  m_ui.pushButton_SCARF_login->setEnabled(false);
  m_ui.pushButton_SCARF_logout->setEnabled(true);

  int kat = settings.useKeepAlive;
  if (kat > 0) {
    g_log.notice()
        << "Reconstruction GUI: starting mechanism to periodically query the "
           "status of jobs. This will update the status of running jobs every "
        << kat << " seconds. You can also update it at any moment by clicking "
                  "on the refresh button. This periodic update mechanism is "
                  "also expected to keep sessions on remote compute resources "
                  "alive after logging in." << std::endl;
    startKeepAliveMechanism(kat);
  }
}

void TomoReconstruction::SCARFLogoutClicked() {
  try {
    doLogout();
  } catch (std::exception &e) {
    throw(std::string("Problem when logging out. Error description: ") +
          e.what());
  }

  enableLoggedActions(false);
  m_loggedIn = false;

  m_ui.pushButton_SCARF_login->setEnabled(true);
  m_ui.pushButton_SCARF_logout->setEnabled(false);
}

/**
 * Load the settings for the tabs and widgets of the interface. This
 * relies on Qt settings functionality (QSettings class).
 *
 * This includes setting the default browsing directory to be the
 * default save directory.
 */
void TomoReconstruction::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  settings.SCARFBasePath =
      qs.value("SCARF-base-path",
               QString::fromStdString(settings.SCARFBasePath))
          .toString()
          .toStdString();
  // WARNING: it's critical to keep 'false' as default value, otherwise
  // scripted runs may have issues. The CI builds could get stuck when
  // closing this interface.
  settings.onCloseAskForConfirmation =
      qs.value("on-close-ask-for-confirmation", false).toBool();

  settings.useKeepAlive =
      qs.value("use-keep-alive", settings.useKeepAlive).toInt();
  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();

  m_ui.lineEdit_SCARF_path->setText(
      QString::fromStdString(settings.SCARFBasePath));
}

/**
 * Save persistent settings. Qt based.
 */
void TomoReconstruction::saveSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));
  QString s = m_ui.lineEdit_SCARF_path->text();
  qs.setValue("SCARF-base-path", s);
  qs.setValue("on-close-ask-for-confirmation",
              settings.onCloseAskForConfirmation);
  qs.setValue("use-keep-alive", settings.useKeepAlive);
  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

/**
 * Load a savu tomo config file into the current plugin list, overwriting it.
 * Uses the algorithm LoadSavuTomoConfig
 */
void TomoReconstruction::loadSavuTomoConfig(
    std::string &filePath, Mantid::API::ITableWorkspace_sptr &currentPlugins) {
  // try to load tomo reconstruction parametereization file
  auto alg = Algorithm::fromString("LoadSavuTomoConfig");
  alg->initialize();
  alg->setPropertyValue("Filename", filePath);
  alg->setPropertyValue("OutputWorkspace", createUniqueNameHidden());
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        std::string("Error when trying to load tomographic reconstruction "
                    "parameter file: ") +
        e.what());
  }

  // new processing plugins list
  try {
    currentPlugins = alg->getProperty("OutputWorkspace");
  } catch (std::exception &e) {
    userError("Could not load config file", "Failed to load the file "
                                            "with the following error: " +
                                                std::string(e.what()));
  }
}

// Build a unique (and hidden) name for the table ws
std::string TomoReconstruction::createUniqueNameHidden() {
  std::string name;
  do {
    // with __ prefix => hidden
    name = "__TomoConfigTableWS_Seq_" +
           boost::lexical_cast<std::string>(m_nameSeqNo++);
  } while (AnalysisDataService::Instance().doesExist(name));

  return name;
}

/**
 * Sets the compute resource that will be used to run reconstruction
 * jobs. It checks that the facility and compute resource are fine
 * (the one expected). Otherwise, shows an error and not much can be
 * done.
 */
void TomoReconstruction::setupComputeResource() {
  // set up the compute resource
  QComboBox *cr = m_ui.comboBox_run_compute_resource;
  if (cr) {
    cr->clear();

    const Mantid::Kernel::FacilityInfo &fac =
        Mantid::Kernel::ConfigService::Instance().getFacility();
    if (fac.name() != m_facility) {
      userError("Facility not supported",
                "This interface is designed "
                "to be used at " +
                    m_facility +
                    ". You will probably not be "
                    "able to use it in a useful way because your facility "
                    "is " +
                    fac.name() +
                    ". If you have set that facility "
                    "facility by mistake in your settings, please update it.");
      return;
    }

    if (m_computeRes.size() < 1) {
      userWarning("No remote compute resource set!",
                  "No remote compute "
                  "resource has been set. Please note that without a "
                  "remote compute resource the functionality of this "
                  "interface might be limited.");
    } else {
      // assume the present reality: just SCARF
      const std::string &required = m_computeRes.front();
      std::vector<std::string> res = Mantid::Kernel::ConfigService::Instance()
                                         .getFacility()
                                         .computeResources();
      if (res.end() == std::find(res.begin(), res.end(), required)) {
        userError("Compute resource " + required + "not found ",
                  "This interface requires the " + required +
                      " compute resource. Even though your facility is " +
                      fac.name() +
                      ", the compute resource was not found. "
                      "In principle the compute resource should have been "
                      "defined in the facilities file for you facility. "
                      "Please check your settings.");
      }
      cr->addItem(QString::fromStdString(required));
    }

    // put local but disable, as it's not yet sorted out how it will work
    cr->addItem(QString::fromStdString(m_localCompName));
    QModelIndex idx = cr->model()->index(1, 0);
    QVariant disabled(0);
    cr->model()->setData(idx, disabled, Qt::UserRole - 1);
  }
}

void TomoReconstruction::setupRunTool() {
  // set up the reconstruction tool
  QComboBox *rt = m_ui.comboBox_run_tool;
  if (rt) {
    std::vector<std::string> tools;
    // catch all the useable/relevant tools for the compute
    // resources. For the time being this is rather simple (just
    // SCARF) and will probably stay like this for a while.
    const std::string res = getComputeResource();
    if ("ISIS" == m_facility && m_SCARFName == res) {
      tools = m_SCARFtools;
    }
    // others would/could come here

    rt->clear();
    for (size_t i = 0; i < tools.size(); i++) {
      rt->addItem(QString::fromStdString(tools[i].c_str()));

      // put CCPi but disable it, as it's not yet sorted out how it is
      // configured / run
      if (m_CCPiTool == tools[i]) {
        QModelIndex idx = rt->model()->index(static_cast<int>(i), 0);
        QVariant disabled(0);
        rt->model()->setData(idx, disabled, Qt::UserRole - 1);
      }

      // We cannot run Savu at present
      if (m_SavuTool == tools[i] || m_CCPiTool == tools[i]) {
        m_ui.pushButton_reconstruct->setEnabled(false);
      }
    }
  }
}

/// needs to at least update the 'tool' combo box
void TomoReconstruction::compResourceIndexChanged(int i) {
  UNUSED_ARG(i);
  setupRunTool();
}

void TomoReconstruction::runToolIndexChanged(int i) {
  UNUSED_ARG(i);
  QComboBox *rt = m_ui.comboBox_run_tool;

  if (!rt)
    return;

  std::string tool = rt->currentText().toStdString();
  // disallow reconstruct on tools that don't run yet: Savu and CCPi
  if (m_CCPiTool == tool || m_SavuTool == tool) {
    m_ui.pushButton_reconstruct->setEnabled(false);
  } else {
    m_ui.pushButton_reconstruct->setEnabled(true);
  }
}

/**
 * Log into remote compute resource.
 *
 * @param pw Password/authentication credentials as a string
 */
void TomoReconstruction::doLogin(const std::string &pw) {
  if (m_loggedIn) {
    userError("Better to logout before logging in again",
              "You're currently logged in. Please, log out before logging in "
              "again if that's what you meant.");
  }

  const std::string user = getUsername();
  if (user.empty()) {
    userError("Cannot log in",
              "To log in you need to specify a username (and a password!).");
    return;
  }

  // TODO (trac #11538): once the remote algorithms are rearranged
  // into the 'RemoteJobManager' design, this will use...
  // auto alg = Algorithm::fromString("Authenticate");
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("UserName", user);
  alg->setPropertyValue("Action", "LogIn");
  alg->setPropertyValue("Password", pw);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to log into the remote compute resource " +
        getComputeResource() + " with username " + user + ": " + e.what());
  }
}

void TomoReconstruction::doLogout() {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use...
  // auto alg = Algorithm::fromString("???"); - need an alg for this
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  const std::string user = getUsername();
  alg->setPropertyValue("UserName", user);
  alg->setPropertyValue("Action", "LogOut");
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to log out from the remote compute resource " +
        getComputeResource() + " with username " + user + ": " + e.what());
  }
}

/**
 * Ping the compute resource / server to check if it's alive and
 * responding.
 *
 * @return True if ping succeeded
 */
bool TomoReconstruction::doPing() {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use...
  // auto alg = Algorithm::fromString("???");
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("UserName", getUsername());
  alg->setPropertyValue("Action", "Ping");
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to ping the remote compute resource " +
        getComputeResource() + ": " + e.what());
  }
  return true;
}

/**
 * Handle the job submission request relies on a submit algorithm.
 */
void TomoReconstruction::doSubmitReconstructionJob() {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use:
  // auto transAlg = Algorithm::fromString("StartRemoteTransaction");
  // auto submitAlg = Algorithm::fromString("SubmitRemoteJob");
  // submitAlg->setPropertyValue("ComputeResource", res);
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("Action", "SubmitJob");
  alg->setPropertyValue("UserName", getUsername());

  std::string run, opt;
  makeRunnableWithOptions(run, opt);
  alg->setProperty("RunnablePath", run);
  alg->setProperty("JobOptions", opt);

  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to cancel a reconstruction job: " +
        std::string(e.what()));
  }
}

/**
 * Build the components of the command line to run on the remote
 * compute resource.Produces a (normally full) path to a runnable, and
 * the options (quite like $0 and $* in scripts).
 *
 * @param run Path to a runnable application (script, python module, etc.)
 * @param opt Command line parameters to the application
 */
void TomoReconstruction::makeRunnableWithOptions(std::string &run,
                                                 std::string &opt) {
  // For now we only know how to 'aproximately' run commands on SCARF
  if (m_SCARFName ==
      m_ui.comboBox_run_compute_resource->currentText().toStdString()) {
    const std::string tool =
        m_ui.comboBox_run_tool->currentText().toStdString();
    std::string base = currentPathSCARF() + "/";
    if (tool == m_TomoPyTool) {
      run = "/work/imat/z-tests-fedemp/scripts/tomopy/imat_recon_FBP.py";
      opt = "--input_dir " + base + currentPathFITS() + " " + "--dark " + base +
            currentPathDark() + " " + "--white " + base + currentPathFlat();
    } else if (tool == m_AstraTool) {
      run = "/work/imat/scripts/astra/astra-3d-SIRT3D.py";
      opt = base + currentPathFITS();
    } else {
      userWarning("Unable to use this tool",
                  "I do not know how to submit jobs to use this tool: " + tool +
                      ". It seems that this interface is "
                      "misconfigured or there has been an unexpected "
                      "failure.");
    }
  } else {
    run = "error_dont_know_what_to_do";
    opt = "no_options_known";
  }
}

void TomoReconstruction::doCancelJob(const std::string &id) {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use:
  // auto alg = Algorithm::fromString("EndRemoteTransaction");
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("UserName", getUsername());
  alg->setPropertyValue("Action", "CancelJob");
  alg->setPropertyValue("JobID", id);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to cancel a reconstruction job: " +
        std::string(e.what()));
  }
}

TomoToolSetupDialog::TomoToolSetupDialog(QWidget *parent) : QDialog(parent) {
  labelRun = new QLabel("Runnable script");
  editRun = new QLineEdit("/work/imat/");
  hRun = new QHBoxLayout();
  hRun->addWidget(labelRun);
  hRun->addWidget(editRun);

  labelOpt = new QLabel("Command line options");
  editOpt = new QLineEdit("/work/imat");
  hOpt = new QHBoxLayout();
  hOpt->addWidget(labelOpt);
  hOpt->addWidget(editOpt);

  layout = new QGridLayout();
  layout->addLayout(hRun, 0, 0);
  layout->addLayout(hOpt, 1, 0);

  // connect(lineEdit, SIGNAL(textChanged(const QString &)),
  //     this, SLOT(enableFindButton(const QString &)));
  connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void TomoToolSetupDialog::okClicked() {}

void TomoToolSetupDialog::cancelClicked() {}

void TomoReconstruction::toolSetupClicked() {
  // big TODO: handle tool specific options / config files
  QComboBox *rt = m_ui.comboBox_run_tool;
  if (!rt)
    return;

  const std::string tool = rt->currentText().toStdString();
  if (m_CCPiTool != tool) {
    showToolConfig(tool);
  }
}

void TomoReconstruction::showToolConfig(const std::string &name) {
  if (m_TomoPyTool == name) {
    TomoToolConfigTomoPy tomopy;
    m_uiTomoPy.setupUi(&tomopy);
    tomopy.show();
  } else if (m_AstraTool == name) {
    TomoToolConfigAstra astra;
    m_uiAstra.setupUi(&astra);
    astra.show();
  } else if (m_SavuTool == name) {
    TomoToolConfigSavu savu;
    m_uiSavu.setupUi(&savu);
    doSetupSectionParameters();
    savu.show();
  } else if (m_CustomCmdTool == name) {
    TomoToolConfigCustom cmd;
    m_uiCustom.setupUi(&cmd);
    cmd.show();
  }
}

void TomoReconstruction::reconstructClicked() {
  const std::string &resource = getComputeResource();

  if (m_localCompName != resource) {
    doSubmitReconstructionJob();

    jobTableRefreshClicked();
  }
}

void TomoReconstruction::runVisualizeClicked() {
  QTableWidget *tbl = m_ui.tableWidget_run_jobs;
  const int idCol = 2;
  QTableWidgetItem *hdr = tbl->horizontalHeaderItem(idCol);
  if ("ID" != hdr->text())
    throw std::runtime_error("Expected to get the Id of jobs from the "
                             "second column of the table of jobs, but I "
                             "found this at that column: " +
                             hdr->text().toStdString());

  QModelIndexList idSel = tbl->selectionModel()->selectedRows();
  if (idSel.count() <= 0)
    return;

  const std::string id = tbl->item(idSel[0].row(), idCol)->text().toStdString();
  if (idSel.count() > 1)
    g_log.information() << " Visualizing only the first job: " << id
                        << std::endl;
}

/// processes (cancels) all the jobs selected in the table
void TomoReconstruction::jobCancelClicked() {
  const std::string &resource = getComputeResource();

  QTableWidget *tbl = m_ui.tableWidget_run_jobs;
  const int idCol = 2;
  QTableWidgetItem *hdr = tbl->horizontalHeaderItem(idCol);
  if ("ID" != hdr->text())
    throw std::runtime_error("Expected to get the Id of jobs from the "
                             "second column of the table of jobs, but I "
                             "found this at that column: " +
                             hdr->text().toStdString());

  QModelIndexList idSel = tbl->selectionModel()->selectedRows();
  for (int i = 0; i < idSel.count(); ++i) {
    std::string id = tbl->item(idSel[i].row(), idCol)->text().toStdString();
    if (m_localCompName != resource) {
      doCancelJob(id);
    }
  }
}

void TomoReconstruction::jobTableRefreshClicked() {
  // get the info from the server into data members. This operation is subject
  // to delays in the connection, etc.
  try {
    getJobStatusInfo();
  } catch (std::runtime_error &e) {
    g_log.warning() << "There was an issue while trying to retrieve job status "
                       "information from the remote compute resource ("
                    << getComputeResource()
                    << "). Stopping periodic (automatic) status update to "
                       "prevent more failures. You can start the automatic "
                       "update mechanism again by logging in, as apparently "
                       "there is some problem with the last session. "
                    << std::endl;
  }

  // update widgets from that info
  updateJobsTable();
}

void TomoReconstruction::getJobStatusInfo() {
  if (!m_loggedIn)
    return;

  std::vector<std::string> ids, names, status, cmds;
  doQueryJobStatus(ids, names, status, cmds);

  size_t jobMax = ids.size();
  if (ids.size() != names.size() || ids.size() != status.size() ||
      ids.size() != cmds.size()) {
    // this should not really happen
    jobMax = std::min(ids.size(), names.size());
    jobMax = std::min(jobMax, status.size());
    jobMax = std::min(jobMax, cmds.size());
    userWarning("Problem retrieving job status information",
                "The response from the compute resource did not seem "
                "correct. The table of jobs may not be fully up to date.");
  }

  {
    QMutexLocker lockit(&m_statusMutex);
    m_jobsStatus.clear();
    m_jobsStatusCmds.clear();
    // TODO: udate when we update to remote algorithms v2
    // As SCARF doesn't provide all the info at the moment, and as we're
    // using the SCARFTomoReconstruction algorithm, the
    // IRemoteJobManager::RemoteJobInfo struct is for now used only partially
    // (cmds out). So this loop feels both incomplete and an unecessary second
    // step that could be avoided.
    for (size_t i = 0; i < ids.size(); ++i) {
      IRemoteJobManager::RemoteJobInfo ji;
      ji.id = ids[i];
      ji.name = names[i];
      ji.status = status[i];
      m_jobsStatus.push_back(ji);
      m_jobsStatusCmds.push_back(cmds[i]);
    }
  }
}

void TomoReconstruction::doQueryJobStatus(std::vector<std::string> &ids,
                                          std::vector<std::string> &names,
                                          std::vector<std::string> &status,
                                          std::vector<std::string> &cmds) {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use...
  // auto alg = Algorithm::fromString("QueryAllRemoteJobs");
  // and
  // auto alg = Algorithm::fromString("QueryRemoteJob");

  // output properties to get: RemoteJobsID, RemoteJobsNames,
  //    RemoteJobsStatus, RemoteJobsCommands
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("UserName", getUsername());
  alg->setPropertyValue("Action", "JobStatus");
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to query the status of jobs in " +
        getComputeResource() + ": " + e.what());
  }
  ids = alg->getProperty("RemoteJobsID");
  names = alg->getProperty("RemoteJobsNames");
  status = alg->getProperty("RemoteJobsStatus");
  cmds = alg->getProperty("RemoteJobsCommands");
}

/**
 * Update the job status and general info table/tree from the info
 * stored in this class' data members, which ideally should have
 * information from a recent query to the server.
 */
void TomoReconstruction::updateJobsTable() {

  QTableWidget *t = m_ui.tableWidget_run_jobs;
  bool sort = t->isSortingEnabled();
  t->setRowCount(static_cast<int>(m_jobsStatus.size()));

  {
    QMutexLocker lockit(&m_statusMutex);
    for (size_t i = 0; i < m_jobsStatus.size(); ++i) {
      int ii = static_cast<int>(i);
      t->setItem(ii, 0,
                 new QTableWidgetItem(QString::fromStdString(m_SCARFName)));
      t->setItem(ii, 1, new QTableWidgetItem(
                            QString::fromStdString(m_jobsStatus[i].name)));
      t->setItem(ii, 2, new QTableWidgetItem(
                            QString::fromStdString(m_jobsStatus[i].id)));
      t->setItem(ii, 3, new QTableWidgetItem(
                            QString::fromStdString(m_jobsStatus[i].status)));
      t->setItem(ii, 4, new QTableWidgetItem(
                            QString::fromStdString(m_jobsStatusCmds[i])));
    }
  }

  t->setSortingEnabled(sort);
}

void TomoReconstruction::browseImageClicked() {
  // get path
  QString fitsStr = QString("Supported formats: FITS, TIFF and PNG "
                            "(*.fits *.fit *.tiff *.tif *.png);;"
                            "FITS, Flexible Image Transport System images "
                            "(*.fits *.fit);;"
                            "TIFF, Tagged Image File Format "
                            "(*.tif *.tiff);;"
                            "PNG, Portable Network Graphics "
                            "(*.png);;"
                            "Other extensions/all files (*.*)");
  // Note that this could be done using UserSubWindow::openFileDialog(),
  // but that method doesn't give much control over the text used for the
  // allowed extensions.
  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString path(QFileDialog::getOpenFileName(this, tr("Open image file"),
                                            prevPath, fitsStr));
  if (!path.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        QFileInfo(path).absoluteDir().path());
  } else {
    return;
  }

  QString suf = QFileInfo(path).suffix();
  bool loaded = false;
  // This is not so great, as we check extensions and not really file
  // content/headers, as it should be.
  if ((0 == QString::compare(suf, "fit", Qt::CaseInsensitive)) ||
      (0 == QString::compare(suf, "fits", Qt::CaseInsensitive))) {
    WorkspaceGroup_sptr wsg = loadFITSImage(path.toStdString());
    if (!wsg)
      return;
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));
    if (!ws)
      return;
    drawImage(ws);
    loaded = true;
    // clean-up container group workspace
    if (wsg)
      AnalysisDataService::Instance().remove(wsg->getName());
  } else if ((0 == QString::compare(suf, "tif", Qt::CaseInsensitive)) ||
             (0 == QString::compare(suf, "tiff", Qt::CaseInsensitive)) ||
             (0 == QString::compare(suf, "png", Qt::CaseInsensitive))) {
    QImage rawImg(path);
    QPainter painter;
    QPixmap pix(rawImg.width(), rawImg.height());
    painter.begin(&pix);
    painter.drawImage(0, 0, rawImg);
    painter.end();
    m_ui.label_image->setPixmap(pix);
    m_ui.label_image->show();
    loaded = true;
  } else {
    userWarning("Failed to load image - format issue",
                "Could not load image because the extension of the file " +
                    path.toStdString() + ", suffix: " + suf.toStdString() +
                    " does not correspond to FITS or TIFF files.");
  }

  if (loaded)
    m_ui.label_image_name->setText(path);
}

/**
 * Helper to get a FITS image into a workspace. Uses the LoadFITS
 * algorithm. If the algorithm throws, this method shows user (pop-up)
 * warning/error messages but does not throw.
 *
 * This method returns a workspace group which most probably you want
 * to delete after using the image to draw it.
 *
 * @param path Path to a FITS image
 *
 * @return Group Workspace containing a Matrix workspace with a FITS
 * image, one pixel per histogram, as loaded by LoadFITS (can be empty
 * if the load goes wrong and the workspace is not available from the
 * ADS).
 */
WorkspaceGroup_sptr TomoReconstruction::loadFITSImage(const std::string &path) {
  // get fits file into workspace and retrieve it from the ADS
  auto alg = Algorithm::fromString("LoadFITS");
  alg->initialize();
  alg->setPropertyValue("Filename", path);
  std::string wsName = "__fits_ws_imat_tomography_gui";
  alg->setProperty("OutputWorkspace", wsName);
  try {
    alg->execute();
  } catch (std::exception &e) {
    userWarning("Failed to load image", "Could not load this file as a "
                                        "FITS image: " +
                                            std::string(e.what()));
    return WorkspaceGroup_sptr();
  }
  if (!alg->isExecuted()) {
    userWarning("Failed to load image correctly",
                "Note that even though "
                "the image file has been loaded it seems to contain "
                "errors.");
  }
  WorkspaceGroup_sptr wsg;
  MatrixWorkspace_sptr ws;
  try {
    wsg = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        wsg->getNames()[0]);
  } catch (std::exception &e) {
    userWarning("Could not load image contents",
                "An unrecoverable error "
                "happened when trying to load the image contents. Cannot "
                "display it. Error details: " +
                    std::string(e.what()));
    return WorkspaceGroup_sptr();
  }

  // draw image from workspace
  if (wsg && ws &&
      Mantid::API::AnalysisDataService::Instance().doesExist(ws->name())) {
    return wsg;
  } else {
    return WorkspaceGroup_sptr();
  }
}

void TomoReconstruction::loadAvailablePlugins() {
  // TODO:: load actual plugins when we know them
  // creating a few relatively realistic choices for now (should crossh check
  //  with the savu api when finalized).
  // - Should also verify the param string is valid json when setting

  // Create plugin table
  Mantid::API::TableRow row = m_availPlugins->appendRow();
  row << "savu.plugins.timeseries_field_corrections"
      << "{}"
      << "Time Series Field Corrections"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.median_filter"
      << "{\"kernel_size\":[1, 3, 3]}"
      << "Median Filter"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.vo_centering"
      << "{}"
      << "Vo Centering"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.simple_recon"
      << "{\"center_of_rotation\":86}"
      << "Simple Reconstruction"
      << "Citation info";

  row = m_availPlugins->appendRow();
  row << "savu.plugins.astra_recon"
      << "{\"center_of_rotation\":\"86\", "
         "\"reconsturction_type\":\"SIRT\", \"number_of_iterations\":5}"
      << "Simple Reconstruction"
      << "Citation info";

  // Update the UI
  refreshAvailablePluginListUI();
}

// Reloads the GUI list of available plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshAvailablePluginListUI() {
  // Table WS structure, id/params/name/cite
  m_uiSavu.listAvailablePlugins->clear();
  for (size_t i = 0; i < m_availPlugins->rowCount(); ++i) {
    QString str =
        QString::fromStdString(m_availPlugins->cell<std::string>(i, 2));
    m_uiSavu.listAvailablePlugins->addItem(str);
  }
}

// Reloads the GUI list of current plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshCurrentPluginListUI() {
  // Table WS structure, id/params/name/cite
  m_uiSavu.treeCurrentPlugins->clear();
  createPluginTreeEntries(m_currPlugins);
}

// Updates the selected plugin info from Available plugins list.
void TomoReconstruction::availablePluginSelected() {
  if (m_uiSavu.listAvailablePlugins->selectedItems().count() != 0) {
    size_t idx = static_cast<size_t>(
        m_uiSavu.listAvailablePlugins->currentIndex().row());
    if (idx < m_availPlugins->rowCount()) {
      m_uiSavu.availablePluginDesc->setText(
          tableWSRowToString(m_availPlugins, idx));
    }
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoReconstruction::currentPluginSelected() {
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    auto currItem = m_uiSavu.treeCurrentPlugins->selectedItems()[0];

    while (currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex =
        m_uiSavu.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_uiSavu.currentPluginDesc->setText(
        tableWSRowToString(m_currPlugins, topLevelIndex));
  }
}

// On user editing a parameter tree item, update the data object to match.
void TomoReconstruction::paramValModified(QTreeWidgetItem *item,
                                          int /*column*/) {
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem *>(item);
  int topLevelIndex = -1;

  if (ownItem->getRootParent() != NULL) {
    topLevelIndex = m_uiSavu.treeCurrentPlugins->indexOfTopLevelItem(
        ownItem->getRootParent());
  }

  if (topLevelIndex != -1) {
    // Recreate the json string from the nodes and write back
    std::string json = m_currPlugins->cell<std::string>(topLevelIndex, 1);
    // potential new line out, and trim spaces
    json.erase(std::remove(json.begin(), json.end(), '\n'), json.end());
    json.erase(std::remove(json.begin(), json.end(), '\r'), json.end());
    json = Poco::trimInPlace(json);

    ::Json::Reader r;
    ::Json::Value root;
    if (r.parse(json, root)) {
      // Look for the key and replace it
      root[ownItem->getKey()] = ownItem->text(0).toStdString();
    }

    m_currPlugins->cell<std::string>(topLevelIndex, 1) =
        ::Json::FastWriter().write(root);
    currentPluginSelected();
  }
}

// When a top level item is expanded, also expand its child items - if tree
// items
void TomoReconstruction::expandedItem(QTreeWidgetItem *item) {
  if (item->parent() == NULL) {
    for (int i = 0; i < item->childCount(); ++i) {
      item->child(i)->setExpanded(true);
    }
  }
}

// Adds one plugin from the available plugins list into the list of
// current plugins
void TomoReconstruction::transferClicked() {
  if (m_uiSavu.listAvailablePlugins->selectedItems().count() != 0) {
    int idx = m_uiSavu.listAvailablePlugins->currentIndex().row();
    Mantid::API::TableRow row = m_currPlugins->appendRow();
    for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
      row << m_availPlugins->cell<std::string>(idx, j);
    }
    createPluginTreeEntry(row);
  }
}

void TomoReconstruction::moveUpClicked() {
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    size_t idx =
        static_cast<size_t>(m_uiSavu.treeCurrentPlugins->currentIndex().row());
    if (idx > 0 && idx < m_currPlugins->rowCount()) {
      // swap row, all columns
      for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
        std::string swap = m_currPlugins->cell<std::string>(idx, j);
        m_currPlugins->cell<std::string>(idx, j) =
            m_currPlugins->cell<std::string>(idx - 1, j);
        m_currPlugins->cell<std::string>(idx - 1, j) = swap;
      }
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::moveDownClicked() {
  // TODO: this can be done with the same function as above...
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    size_t idx =
        static_cast<size_t>(m_uiSavu.treeCurrentPlugins->currentIndex().row());
    if (idx < m_currPlugins->rowCount() - 1) {
      // swap all columns
      for (size_t j = 0; j < m_currPlugins->columnCount(); ++j) {
        std::string swap = m_currPlugins->cell<std::string>(idx, j);
        m_currPlugins->cell<std::string>(idx, j) =
            m_currPlugins->cell<std::string>(idx + 1, j);
        m_currPlugins->cell<std::string>(idx + 1, j) = swap;
      }
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::removeClicked() {
  // Also clear ADS entries
  if (m_uiSavu.treeCurrentPlugins->selectedItems().count() != 0) {
    int idx = m_uiSavu.treeCurrentPlugins->currentIndex().row();
    m_currPlugins->removeRow(idx);

    refreshCurrentPluginListUI();
  }
}

void TomoReconstruction::menuOpenClicked() {
  QString s =
      QFileDialog::getOpenFileName(0, "Open file", QDir::currentPath(),
                                   "NeXus files (*.nxs);;All files (*.*)",
                                   new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if (returned != "") {
    bool opening = true;

    if (m_currPlugins->rowCount() > 0) {
      QMessageBox::StandardButton reply = QMessageBox::question(
          this, "Open file confirmation",
          "Opening the configuration file will clear the current list."
          "\nWould you like to continue?",
          QMessageBox::Yes | QMessageBox::No);
      if (reply == QMessageBox::No) {
        opening = false;
      }
    }

    if (opening) {
      loadSavuTomoConfig(returned, m_currPlugins);

      m_currentParamPath = returned;
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::menuSaveClicked() {
  if (m_currentParamPath == "") {
    menuSaveAsClicked();
    return;
  }

  if (m_currPlugins->rowCount() != 0) {
    AnalysisDataService::Instance().add(createUniqueNameHidden(),
                                        m_currPlugins);
    std::string csvWorkspaceNames = m_currPlugins->name();

    auto alg = Algorithm::fromString("SaveTomoConfig");
    alg->initialize();
    alg->setPropertyValue("Filename", m_currentParamPath);
    alg->setPropertyValue("InputWorkspaces", csvWorkspaceNames);
    alg->execute();

    if (!alg->isExecuted()) {
      throw std::runtime_error("Error when trying to save config file");
    }
  } else {
    // Alert that the plugin list is empty
    QMessageBox::information(this, tr("Unable to save file"),
                             "The current plugin list is empty, please add one "
                             "or more to the list.");
  }
}

void TomoReconstruction::menuSaveAsClicked() {
  QString s =
      QFileDialog::getSaveFileName(0, "Save file", QDir::currentPath(),
                                   "NeXus files (*.nxs);;All files (*.*)",
                                   new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if (returned != "") {
    m_currentParamPath = returned;
    menuSaveClicked();
  }
}

QString TomoReconstruction::tableWSRowToString(ITableWorkspace_sptr table,
                                               size_t i) {
  std::stringstream msg;
  msg << "ID: " << table->cell<std::string>(i, 0) << std::endl
      << "Params: " << table->cell<std::string>(i, 1) << std::endl
      << "Name: " << table->cell<std::string>(i, 2) << std::endl
      << "Cite: " << table->cell<std::string>(i, 3);
  return QString::fromStdString(msg.str());
}

/**
 * Creates a treewidget item for a row of a table workspace.
 *
 * @param row Row from a table workspace with each row specfying a savu plugin
 */
void TomoReconstruction::createPluginTreeEntry(TableRow &row) {
  QStringList idStr, nameStr, citeStr, paramsStr;
  idStr.push_back(QString::fromStdString("ID: " + row.cell<std::string>(0)));
  nameStr.push_back(
      QString::fromStdString("Name: " + row.cell<std::string>(2)));
  citeStr.push_back(
      QString::fromStdString("Cite: " + row.cell<std::string>(3)));
  paramsStr.push_back(QString::fromStdString("Params:"));

  // Setup editable tree items
  QList<QTreeWidgetItem *> items;
  OwnTreeWidgetItem *pluginBaseItem = new OwnTreeWidgetItem(nameStr);
  OwnTreeWidgetItem *pluginParamsItem =
      new OwnTreeWidgetItem(pluginBaseItem, paramsStr, pluginBaseItem);

  // Add to the tree list. Adding now to build hierarchy for later setItemWidget
  // call
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, idStr, pluginBaseItem));
  items.push_back(
      new OwnTreeWidgetItem(pluginBaseItem, nameStr, pluginBaseItem));
  items.push_back(
      new OwnTreeWidgetItem(pluginBaseItem, citeStr, pluginBaseItem));
  items.push_back(pluginParamsItem);

  // Params will be a json string which needs splitting into child tree items
  // [key/value]
  ::Json::Value root;
  std::string paramString = row.cell<std::string>(1);
  ::Json::Reader r;
  if (r.parse(paramString, root)) {
    auto members = root.getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it) {
      OwnTreeWidgetItem *container =
          new OwnTreeWidgetItem(pluginParamsItem, pluginBaseItem);

      QWidget *w = new QWidget();
      w->setAutoFillBackground(true);

      QHBoxLayout *layout = new QHBoxLayout(w);
      layout->setMargin(1);
      QLabel *label1 = new QLabel(QString::fromStdString((*it) + ": "));

      QTreeWidget *paramContainerTree = new QTreeWidget(w);
      connect(paramContainerTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
              this, SLOT(paramValModified(QTreeWidgetItem *, int)));
      paramContainerTree->setHeaderHidden(true);
      paramContainerTree->setIndentation(0);

      auto jsonVal = root.get(*it, "");
      std::string valStr = pluginParamValString(jsonVal, *it);

      QStringList paramVal(QString::fromStdString(valStr));
      OwnTreeWidgetItem *paramValueItem =
          new OwnTreeWidgetItem(paramVal, pluginBaseItem, *it);
      paramValueItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);

      paramContainerTree->addTopLevelItem(paramValueItem);
      QRect rect = paramContainerTree->visualItemRect(paramValueItem);
      paramContainerTree->setMaximumHeight(rect.height());
      paramContainerTree->setFrameShape(QFrame::NoFrame);

      layout->addWidget(label1);
      layout->addWidget(paramContainerTree);

      pluginParamsItem->addChild(container);
      m_uiSavu.treeCurrentPlugins->setItemWidget(container, 0, w);
    }
  }

  pluginBaseItem->addChildren(items);
  m_uiSavu.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
}

/**
 * This is a kind of .asString() method for arrays. It iterates
 * through the array elements and builds the string enclosed by [].
 *
 * @param jsonVal Value of a parameter that seems to be an array
 *(isArray()==true)
 * @param name Name of the parameter (to give informative messages)
 *
 * @return String with a parameter value(s), enclosed by [] and
 * separated by commas
 */
std::string
TomoReconstruction::paramValStringFromArray(const Json::Value &jsonVal,
                                            const std::string &name) {
  std::string s;
  s = "[";
  for (Json::ArrayIndex i = 0; i < jsonVal.size(); ++i) {
    if (jsonVal[i].isArray()) {
      userWarning(
          "Could not recognize parameter value in list/array",
          "The value of parameter '" + name +
              "' could not be interpreted "
              "as a string. It does not seem to be well formed or supported. "
              "For example, parameter values given as lists of lists are not "
              "supported.");
    } else {
      try {
        s += jsonVal[i].asString() + " ,";
      } catch (std::exception &e) {
        userWarning(
            "Could not recognize value in list/array of values",
            "The " + boost::lexical_cast<std::string>(i) +
                "-th value of the list/array could not be interpreted "
                "as a text string. It will be empty in the list of current "
                "plugins. You can still edit it. Error details: " +
                std::string(e.what()));
      }
    }
  }
  // this could be s.back() with C++11
  s[s.length() - 1] = ']'; // and last comma becomes closing ]
  return s;
}

/**
 * Build a string with the value of a parameter in a json
 * string. Works for scalar and list/array values.
 *
 * @param jsonVal Value of a parameter that seems to be an array
 * @param name Name of the parameter (to give informative messages)
 *
 * @return String with a parameter value
 */
std::string TomoReconstruction::pluginParamValString(const Json::Value &jsonVal,
                                                     const std::string &name) {
  std::string s;
  // string and numeric values can (normally) be converted to string but arrays
  // cannot
  if (!jsonVal.isArray()) {
    try {
      s = jsonVal.asString();
    } catch (std::exception &e) {
      userWarning(
          "Could not recognize parameter value",
          "The value of parameter '" + name +
              "' could not be interpreted "
              "as a string. It will be empty in the list of current plugins. "
              "You can still edit it. Error details: " +
              std::string(e.what()));
    }
  } else {
    s = paramValStringFromArray(jsonVal, name);
  }
  return s;
}

void TomoReconstruction::createPluginTreeEntries(ITableWorkspace_sptr table) {
  for (size_t i = 0; i < table->rowCount(); ++i) {
    TableRow r = table->getRow(i);
    createPluginTreeEntry(r);
  }
}

/**
 * Get path from user and update a line edit and a variable.
 *
 * @param le a line edit where the path is shown.
 * @param data variable where the path is stored (in addition to the line
 * edit object).
 */
void TomoReconstruction::processPathBrowseClick(QLineEdit *le,
                                                std::string &data) {
  QString algPrev =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString prev;
  if (le->text().isEmpty()) {
    prev = algPrev;
  } else {
    prev = le->text();
  }

  QString path(QFileDialog::getExistingDirectory(
      this, tr("Open directory/folder"), prev));

  if (!path.isEmpty()) {
    le->setText(path);
    data = path.toStdString();
  }
}

void TomoReconstruction::fitsPathBrowseClicked() {
  processPathBrowseClick(m_ui.lineEdit_path_FITS, m_pathFITS);
}

void TomoReconstruction::flatPathBrowseClicked() {
  processPathBrowseClick(m_ui.lineEdit_path_flat, m_pathFlat);
}

void TomoReconstruction::darkPathBrowseClicked() {
  processPathBrowseClick(m_ui.lineEdit_path_dark, m_pathDark);
}

/**
 * Check that the selected compute resource is listed as supported and
 * usable for the remote manager (if it is not local). Local jobs are
 * not supported for the time being, so this currently raises an
 * exception if the local resource has been selected.
 *
 * This should never throw an exception if the
 * construction/initialization and setup steps went fine and the rest
 * of the code is kept consistent with those steps.
 *
 * @param res Name of the compute resource selected in the interface
 *
 * @return Name of a compute resource (which can be the 'Local' one)
 *
 * @throws std::runtime_error on inconsistent selection of compute
 * resource
 */
std::string TomoReconstruction::validateCompResource(const std::string &res) {
  if (res == m_localCompName) {
    // Nothing yet
    throw std::runtime_error("There is no support for the local compute "
                             "resource. You should not have got here.");
  }

  if (m_computeRes.size() <= 0) {
    throw std::runtime_error("No compute resource registered in the list "
                             "of supported resources. This graphical interface "
                             "is in an inconsistent status.");
  }

  const std::string supported = m_computeRes.front();
  if (supported.empty()) {
    throw std::runtime_error("The first compute resource registered in this "
                             "interface has an empty name.");
  }

  if (res != supported) {
    throw std::runtime_error("The compute resource selected (" + res +
                             ") is not the one in principle supported by this "
                             "interface: " +
                             supported);
  }

  return supported;
}

/**
 * Gets the compute resource that is currently selected by the user.
 * This calls a validation method that can throw in case of
 * inconsistencies.
 *
 * @return Name of the compute resource as a string.
 */
std::string TomoReconstruction::getComputeResource() {
  QComboBox *cb = m_ui.comboBox_run_compute_resource;
  QString rs = cb->currentText();
  return validateCompResource(rs.toStdString());
}

/**
 * Retrieve the username being used for the selected compute resource.
 *
 * @return Username ready to be used in remote queries
 */
std::string TomoReconstruction::getUsername() {
  if (m_SCARFName ==
      m_ui.comboBox_run_compute_resource->currentText().toStdString())
    return m_ui.lineEdit_SCARF_username->text().toStdString();
  else
    return "invalid";
}

std::string TomoReconstruction::currentPathSCARF() {
  return m_ui.lineEdit_SCARF_path->text().toStdString();
}

std::string TomoReconstruction::currentPathFITS() {
  return m_ui.lineEdit_path_FITS->text().toStdString();
}

std::string TomoReconstruction::currentPathFlat() {
  return m_ui.lineEdit_path_flat->text().toStdString();
}

std::string TomoReconstruction::currentPathDark() {
  return m_ui.lineEdit_path_dark->text().toStdString();
}

/**
 * Retrieve the username being used for the selected compute resource.
 *
 * @return Username ready to be used in remote queries
 */
std::string TomoReconstruction::getPassword() {
  if (m_SCARFName ==
      m_ui.comboBox_run_compute_resource->currentText().toStdString())
    return m_ui.lineEdit_SCARF_password->text().toStdString();
  else
    return "none";
}

/**
 * draw an image on screen using Qt's QPixmap and QImage. It assumes
 * that the workspace contains an image in the form in which LoadFITS
 * loads FITS images. Checks dimensions and workspace structure and
 * shows user warning/error messages appropriately. But in principle
 * it should not raise any exceptions under reasonable circumstances.
 *
 * @param ws Workspace where a FITS image has been loaded with LoadFITS
 */
void TomoReconstruction::drawImage(const MatrixWorkspace_sptr &ws) {
  // From logs we expect a name "run_title", width "Axis1" and height "Axis2"
  size_t width, height;
  try {
    width = boost::lexical_cast<size_t>(ws->run().getLogData("Axis1")->value());
  } catch (std::exception &e) {
    userError("Cannot load image", "There was a problem while trying to "
                                   "find the width of the image: " +
                                       std::string(e.what()));
    return;
  }
  try {
    height =
        boost::lexical_cast<size_t>(ws->run().getLogData("Axis2")->value());
  } catch (std::exception &e) {
    userError("Cannot load image", "There was a problem while trying to "
                                   "find the height of the image: " +
                                       std::string(e.what()));
    return;
  }
  try {
    std::string name = ws->run().getLogData("run_title")->value();
    g_log.information() << " Visualizing image: " << name << std::endl;
  } catch (std::exception &e) {
    userWarning("Cannot load image information",
                "There was a problem while "
                " trying to find the name of the image: " +
                    std::string(e.what()));
  }

  // images are loaded as 1 histogram == 1 pixel (1 bin per histogram):
  if ((width * height) != ws->getNumberHistograms()) {
    userError("Image dimensions do not match", "Could not load the expected "
                                               "number of pixels.");
    return;
  }
  // find min and max to scale pixel values
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    const double &v = ws->readY(i)[0];
    if (v < min)
      min = v;
    if (v > max)
      max = v;
  }
  if (min >= max) {
    userWarning("Empty image!",
                "The image could be loaded but it contains "
                "effectively no information, all pixels have the same value.");
    // black picture
    QPixmap pix(static_cast<int>(width), static_cast<int>(height));
    pix.fill(QColor(0, 0, 0));
    m_ui.label_image->setPixmap(pix);
    m_ui.label_image->show();
    return;
  }

  // load / transfer image into a QImage
  QImage rawImg(QSize(static_cast<int>(width), static_cast<int>(height)),
                QImage::Format_RGB32);
  size_t i = 0;
  double max_min = max - min;
  for (size_t yi = 0; yi < width; ++yi) {
    for (size_t xi = 0; xi < width; ++xi) {
      const double &v = ws->readY(i)[0];
      // color the range min-max in gray scale. To apply different color
      // maps you'd need to use rawImg.setColorTable() or similar.
      int scaled = static_cast<int>(255.0 * (v - min) / max_min);
      QRgb vRgb = qRgb(scaled, scaled, scaled);
      rawImg.setPixel(static_cast<int>(xi), static_cast<int>(yi), vRgb);
      ++i;
    }
  }

  // paint and show image
  QPainter painter;
  QPixmap pix(static_cast<int>(width), static_cast<int>(height));
  painter.begin(&pix);
  painter.drawImage(0, 0, rawImg);
  painter.end();
  m_ui.label_image->setPixmap(pix);
  m_ui.label_image->show();
}

/**
 * Show a warning message to the user (pop up)
 *
 * @param err Basic error title
 * @param description More detailed explanation, hints, additional
 * information, etc.
 */
void TomoReconstruction::userWarning(std::string err, std::string description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

/**
 * Show an error (serious) message to the user (pop up)
 *
 * @param err Basic error title
 * @param description More detailed explanation, hints, additional
 * information, etc.
 */
void TomoReconstruction::userError(std::string err, std::string description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void TomoReconstruction::openHelpWin() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Tomographic_Reconstruction"));
}

void TomoReconstruction::periodicStatusUpdateRequested() {
  // does just the widgets update
  updateJobsTable();
}

void TomoReconstruction::startKeepAliveMechanism(int period) {
  if (m_keepAliveThread)
    delete m_keepAliveThread;
  QThread *m_keepAliveThread = new QThread();

  if (m_keepAliveTimer)
    delete m_keepAliveTimer;
  m_keepAliveTimer = new QTimer(NULL); // no-parent so it can be moveToThread

  m_keepAliveTimer->setInterval(1000 * period);
  m_keepAliveTimer->moveToThread(m_keepAliveThread);
  // direct connection from the thread
  connect(m_keepAliveTimer, SIGNAL(timeout()), SLOT(jobTableRefreshClicked()),
          Qt::DirectConnection);
  QObject::connect(m_keepAliveThread, SIGNAL(started()), m_keepAliveTimer,
                   SLOT(start()));
  m_keepAliveThread->start();
}

void TomoReconstruction::killKeepAliveMechanism() {
  if (m_keepAliveTimer)
    m_keepAliveTimer->stop();
}

void TomoReconstruction::closeEvent(QCloseEvent *event) {
  int answer = QMessageBox::AcceptRole;

  bool ask = settings.onCloseAskForConfirmation;
  if (ask) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Close the tomographic reconstruction interface");
    // with something like this, we'd have layout issues:
    // msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    // msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setIconPixmap(QPixmap(":/win/unknown.png"));
    QCheckBox confirmCheckBox("Always ask for confirmation", &msgBox);
    confirmCheckBox.setCheckState(Qt::Checked);
    msgBox.layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    msgBox.layout()->addWidget(&confirmCheckBox);
    QPushButton *bYes = msgBox.addButton("Yes", QMessageBox::YesRole);
    bYes->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    QPushButton *bNo = msgBox.addButton("No", QMessageBox::NoRole);
    bNo->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    msgBox.setDefaultButton(bNo);
    msgBox.setText("You are about to close this interface");
    msgBox.setInformativeText(
        "If you close this interface you will need to log in again "
        "and you might loose some of the current state. Jobs running on remote "
        "compute resources will remain unaffected though. Are you sure?");

    settings.onCloseAskForConfirmation = confirmCheckBox.isChecked();
    answer = msgBox.exec();
  }

  if (answer == QMessageBox::AcceptRole) {
    cleanup();
    event->accept();
  } else {
    event->ignore();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
