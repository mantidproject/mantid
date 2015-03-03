#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/TomoReconstruction.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/RemoteJobManager.h"

#include <boost/lexical_cast.hpp>
#include <jsoncpp/json/json.h>
#include <QFileDialog>
#include <QMessageBox>

using namespace Mantid::API;

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(TomoReconstruction);
} // namespace CustomInterfaces
} // namespace MantidQt

namespace
{
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

using namespace MantidQt::CustomInterfaces;

size_t TomoReconstruction::m_nameSeqNo = 0;
std::string TomoReconstruction::m_SCARFName = "SCARF@STFC";
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
    m_availPlugins(), m_currPlugins(), m_currentParamPath() {

  m_computeRes.push_back(m_SCARFName);

  m_SCARFtools.push_back("TomoPy");
  m_SCARFtools.push_back("Astra");
  m_SCARFtools.push_back("CCPi CGLS");
  m_SCARFtools.push_back("Savu");
  m_SCARFtools.push_back("Custom command");
}

void TomoReconstruction::doSetupSectionParameters() {
  // TODO: should split the tabs out into their own files

  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(100);
  sizes.push_back(200);
  m_ui.splitterPlugins->setSizes(sizes);

  // Setup Parameter editor tab
  loadAvailablePlugins();
  m_ui.treeCurrentPlugins->setHeaderHidden(true);

  // Connect slots
  // Menu Items
  connect(m_ui.actionOpen, SIGNAL(triggered()), this,
          SLOT(menuOpenClicked()));
  connect(m_ui.actionSave, SIGNAL(triggered()), this,
          SLOT(menuSaveClicked()));
  connect(m_ui.actionSaveAs, SIGNAL(triggered()), this,
          SLOT(menuSaveAsClicked()));

  // Lists/trees
  connect(m_ui.listAvailablePlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(availablePluginSelected()));
  connect(m_ui.treeCurrentPlugins, SIGNAL(itemSelectionChanged()), this,
          SLOT(currentPluginSelected()));
  connect(m_ui.treeCurrentPlugins, SIGNAL(itemExpanded(QTreeWidgetItem *)),
          this, SLOT(expandedItem(QTreeWidgetItem *)));

  // Buttons
  connect(m_ui.btnTransfer, SIGNAL(released()), this,
          SLOT(transferClicked()));
  connect(m_ui.btnMoveUp, SIGNAL(released()), this, SLOT(moveUpClicked()));
  connect(m_ui.btnMoveDown, SIGNAL(released()), this,
          SLOT(moveDownClicked()));
  connect(m_ui.btnRemove, SIGNAL(released()), this, SLOT(removeClicked()));
}

void TomoReconstruction::doSetupSectionSetup() {
  // disable 'local' for now
  m_ui.tabWidget_comp_resource->setTabEnabled(false, 1);

  connect(m_ui.pushButton_SCARF_login, SIGNAL(released()), this,
          SLOT(SCARFLoginClicked()));
  connect(m_ui.pushButton_SCARF_logout, SIGNAL(released()), this,
          SLOT(SCARFLogoutClicked()));

  // 'browse' buttons
  connect(m_ui.pushButton_savu_config_file, SIGNAL(released()), this,
          SLOT(voidBrowseClicked()));
  connect(m_ui.pushButton_fits_dir, SIGNAL(released()), this,
          SLOT(voidBrowseClicked()));
  connect(m_ui.pushButton_flat_dir, SIGNAL(released()), this,
          SLOT(voidBrowseClicked()));
  connect(m_ui.pushButton_dark_dir, SIGNAL(released()), this,
          SLOT(voidBrowseClicked()));
}

void TomoReconstruction::doSetupSectionRun() {
  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(460);
  sizes.push_back(40);
  m_ui.splitter_run_main_vertical->setSizes(sizes);

  sizes[0] = 460;
  sizes[1] = 40;
  m_ui.splitter_image_resource->setSizes(sizes);

  sizes[0] = 420;
  sizes[1] = 80;
  m_ui.splitter_run_jobs->setSizes(sizes);

  setupComputeResource();
  setupRunTool();

  enableLoggedActions(m_loggedIn);

  // Button signals
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

  m_ui.pushButton_reconstruct->setEnabled(false);
  m_ui.pushButton_run_tool_setup->setEnabled(false);
  m_ui.pushButton_run_job_cancel->setEnabled(false);
  m_ui.pushButton_run_job_visualize->setEnabled(false);
}

void TomoReconstruction::initLayout() {
  // TODO: should split the tabs out into their own files
  m_ui.setupUi(this);

  loadSettings();

  doSetupSectionParameters();
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
  std::vector<QPushButton*> buttons;
  buttons.push_back(m_ui.pushButton_run_refresh);
  buttons.push_back(m_ui.pushButton_run_job_cancel);
  buttons.push_back(m_ui.pushButton_run_job_visualize);

  for (size_t i=0; i<buttons.size(); ++i) {
    buttons[i]->setEnabled(enable);
  }
}
void TomoReconstruction::SCARFLoginClicked() {
  try {
    doLogin(getPassword());
  } catch(std::exception &e) {
    throw e;
  }

  enableLoggedActions(true);
  m_loggedIn = true;

  m_ui.pushButton_SCARF_login->setEnabled(false);
  m_ui.pushButton_SCARF_login->setEnabled(true);
}

void TomoReconstruction::SCARFLogoutClicked() {
  try {
    doLogout();
  } catch(std::exception &e) {
    throw e;
  }

  enableLoggedActions(false);
  m_loggedIn = false;

  m_ui.pushButton_SCARF_login->setEnabled(true);
  m_ui.pushButton_SCARF_logout->setEnabled(false);
}

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the
 * default save directory.
 */
void TomoReconstruction::loadSettings() {
  // TODO: define what settings we'll have in the end.
}

/**
 * Load a savu tomo config file into the current plugin list, overwriting it.
 * Uses the algorithm LoadSavuTomoConfig
 */
void TomoReconstruction::loadSavuTomoConfig(
    std::string &filePath,
    std::vector<Mantid::API::ITableWorkspace_sptr> &currentPlugins) {
  // try to load tomo reconstruction parametereization file
  auto alg = Algorithm::fromString("LoadSavuTomoConfig");
  alg->initialize();
  alg->setPropertyValue("Filename", filePath);
  alg->setPropertyValue("OutputWorkspace", createUniqueNameHidden());
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        std::string("Error when trying to load tomography reconstruction "
                    "parameter file: ") +
        e.what());
  }

  // Clear the plugin list and remove any item in the ADS entries
  for (auto it = currentPlugins.begin(); it != currentPlugins.end(); ++it) {
    ITableWorkspace_sptr curr =
        boost::dynamic_pointer_cast<ITableWorkspace>((*it));
    if (AnalysisDataService::Instance().doesExist(curr->getName())) {
      AnalysisDataService::Instance().remove(curr->getName());
    }
  }
  currentPlugins.clear();

  // new processing plugins list
  ITableWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
  currentPlugins.push_back(ws);
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
      userError("Facility not supported", "This interface is designed "
                "to be used at " + m_facility + ". You will probably not be "
                "able to use it in a useful way because your facility "
                "is " + fac.name() + ". If you have set that facility "
                "facility by mistake in your settings, please update it.");
      return;
    }

    if (m_computeRes.size() < 1) {
      userWarning("No remote compute resource set!", "No remote compute "
                  "resource has been set. Please note that without a "
                  "remote compute resource the functionality of this "
                  "interface might be limited.");
    } else {
      // assume the present reality: just SCARF
      const std::string &required = m_computeRes.front();
      std::vector<std::string> res =
          Mantid::Kernel::ConfigService::Instance().getFacility().
          computeResources();
      if ( res.end() ==
           std::find(res.begin(), res.end(), required) ) {
        userError("Compute resource " + required + "not found ",
                  "This interface requires the " + required +
                  " compute resource. Even though your facility is " +
                  fac.name() + ", the compute resource was not found. "
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
    if ("ISIS" == m_facility && "SCARF@STFC" == res) {
      tools = m_SCARFtools;
    }
    // others would/could come here

    rt->clear();
    for (size_t i=0; i<tools.size(); i++) {
      rt->addItem(QString::fromStdString(tools[i].c_str()));

      // put savu but disable, as it's not yet sorted out
      if ("Savu" == tools[i]) {
        QModelIndex idx = rt->model()->index(static_cast<int>(i), 0);
        QVariant disabled(0);
        rt->model()->setData(idx, disabled, Qt::UserRole - 1);
      }
    }
  }
}

/// needs to at least update the 'tool' combo box
void TomoReconstruction::compResourceIndexChanged(int i) {
  UNUSED_ARG(i);
  setupRunTool();
}

/**
 * Log into remote compute resource.
 *
 * @param pw Password/authentication credentials as a string
 */
void TomoReconstruction::doLogin(const std::string &pw) {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use...
  // auto alg = Algorithm::fromString("Authenticate");
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  const std::string user = getUsername();
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
  alg->setPropertyValue("UserName", getUsername());
  alg->setPropertyValue("JobID", "0");
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to cancel a reconstruction job: " +
        std::string(e.what()));
  }
}

void TomoReconstruction::doCancelJob(const std::string &id) {
  // TODO: once the remote algorithms are rearranged into the
  // 'RemoteJobManager' design, this will use:
  // auto alg = Algorithm::fromString("EndRemoteTransaction");
  auto alg = Algorithm::fromString("SCARFTomoReconstruction");
  alg->initialize();
  alg->setPropertyValue("UserName", getUsername());
  alg->setPropertyValue("JobID", id);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to cancel a reconstruction job: " +
        std::string(e.what()));
  }
}

void TomoReconstruction::toolSetupClicked() {
  // big TODO: handle tool specific options / config files
}

void TomoReconstruction::reconstructClicked() {
  const std::string &resource = getComputeResource();

  if (m_localCompName != resource) {
    doSubmitReconstructionJob();
  }
}

void TomoReconstruction::runVisualizeClicked() {
  const std::string &resource = getComputeResource();

  QTableWidget *tbl = m_ui.tableWidget_run_jobs;
  const int idCol = 1;
  QTableWidgetItem *hdr = tbl->horizontalHeaderItem(idCol);
  if ("ID" != hdr->text())
    throw std::runtime_error("Expected to get the Id of jobs from the "
                             "second column of the table of jobs, but I "
                             "found this at that column: " +
                             hdr->text().toStdString());

  QModelIndexList idSel = tbl->selectionModel()->selectedRows();
  if (idSel.count() <= 0)
    return;

  const std::string id =
      tbl->item(idSel[0].row(), idCol)->text().toStdString();
  if (idSel.count() > 1)
    g_log.information() << " Visualizing only the first job: " <<
      id << std::endl;
}

/// processes (cancels) all the jobs selected in the table
void TomoReconstruction::jobCancelClicked()
{
  const std::string &resource = getComputeResource();

  QTableWidget *tbl = m_ui.tableWidget_run_jobs;
  const int idCol = 1;
  QTableWidgetItem *hdr = tbl->horizontalHeaderItem(idCol);
  if ("ID" != hdr->text())
    throw std::runtime_error("Expected to get the Id of jobs from the "
                             "second column of the table of jobs, but I "
                             "found this at that column: " +
                             hdr->text().toStdString());

  QModelIndexList idSel = tbl->selectionModel()->selectedRows();
  for (int i=0; i < idSel.count(); ++i) { 
    std::string id = tbl->item(idSel[i].row(), idCol)->text().toStdString();
    if (m_localCompName != resource) {
      doCancelJob(id);
    }
  }
}

void TomoReconstruction::doQueryJobStatus(std::vector<std::string> ids,
                                          std::vector<std::string> names,
                                          std::vector<std::string> status,
                                          std::vector<std::string> cmds) {
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
  names = alg->getProperty("RemoteJobsID");
  status = alg->getProperty("RemoteJobsStatus");
  cmds = alg->getProperty("RemoteJobsCommands");
}

void  TomoReconstruction::jobTableRefreshClicked() {
  std::vector<std::string> ids, names, status, cmds;
  doQueryJobStatus(ids, names, status, cmds);

  // TODO: sort out table modification process
}

void TomoReconstruction::loadAvailablePlugins() {
  // TODO:: load actual plugins -
  // creating a couple of test choices for now (should fetch from remote api
  // when implemented)
  // - Should also verify the param string is valid json when setting
  // Create plugin tables

  auto plug1 = Mantid::API::WorkspaceFactory::Instance().createTable();
  auto plug2 = Mantid::API::WorkspaceFactory::Instance().createTable();
  plug1->addColumns("str", "name", 4);
  plug2->addColumns("str", "name", 4);
  Mantid::API::TableRow plug1row = plug1->appendRow();
  Mantid::API::TableRow plug2row = plug2->appendRow();
  plug1row << "10001"
           << "{\"key\":\"val\",\"key2\":\"val2\"}"
           << "Plugin #1"
           << "Citation info";
  plug2row << "10002"
           << "{\"key\":\"val\",\"key2\":\"val2\"}"
           << "Plugin #2"
           << "Citation info";

  m_availPlugins.push_back(plug1);
  m_availPlugins.push_back(plug2);

  // Update the UI
  refreshAvailablePluginListUI();
}

// Reloads the GUI list of available plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshAvailablePluginListUI() {
  // Table WS structure, id/params/name/cite
  m_ui.listAvailablePlugins->clear();
  for (auto it = m_availPlugins.begin(); it != m_availPlugins.end(); ++it) {
    QString str = QString::fromStdString((*it)->cell<std::string>(0, 2));
    m_ui.listAvailablePlugins->addItem(str);
  }
}

// Reloads the GUI list of current plugins from the data object ::
// Populating only through this ensures correct indexing.
void TomoReconstruction::refreshCurrentPluginListUI() {
  // Table WS structure, id/params/name/cite
  m_ui.treeCurrentPlugins->clear();
  for (auto it = m_currPlugins.begin(); it != m_currPlugins.end(); ++it) {
    createPluginTreeEntry(*it);
  }
}

// Updates the selected plugin info from Available plugins list.
void TomoReconstruction::availablePluginSelected() {
  if (m_ui.listAvailablePlugins->selectedItems().count() != 0) {
    int currInd = m_ui.listAvailablePlugins->currentIndex().row();
    m_ui.availablePluginDesc->setText(
        tableWSToString(m_availPlugins[currInd]));
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoReconstruction::currentPluginSelected() {
  if (m_ui.treeCurrentPlugins->selectedItems().count() != 0) {
    auto currItem = m_ui.treeCurrentPlugins->selectedItems()[0];

    while (currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex =
        m_ui.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_ui.currentPluginDesc->setText(
        tableWSToString(m_currPlugins[topLevelIndex]));
  }
}

// On user editing a parameter tree item, update the data object to match.
void TomoReconstruction::paramValModified(QTreeWidgetItem *item,
                                          int /*column*/) {
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem *>(item);
  int topLevelIndex = -1;

  if (ownItem->getRootParent() != NULL) {
    topLevelIndex = m_ui.treeCurrentPlugins->indexOfTopLevelItem(
        ownItem->getRootParent());
  }

  if (topLevelIndex != -1) {
    // Recreate the json string from the nodes and write back
    ::Json::Value root;
    std::string json = m_currPlugins[topLevelIndex]->cell<std::string>(0, 1);
    ::Json::Reader r;

    if (r.parse(json, root)) {
      // Look for the key and replace it
      root[ownItem->getKey()] = ownItem->text(0).toStdString();
    }

    m_currPlugins[topLevelIndex]->cell<std::string>(0, 1) =
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

// Clones the selected available plugin object into the current plugin vector
// and refreshes the UI.
void TomoReconstruction::transferClicked() {
  if (m_ui.listAvailablePlugins->selectedItems().count() != 0) {
    int currInd = m_ui.listAvailablePlugins->currentIndex().row();

    ITableWorkspace_sptr newPlugin(m_availPlugins.at(currInd)->clone());

    // Creates a hidden ws entry (with name) in the ADS
    AnalysisDataService::Instance().add(createUniqueNameHidden(), newPlugin);

    m_currPlugins.push_back(newPlugin);

    createPluginTreeEntry(newPlugin);
  }
}

void TomoReconstruction::moveUpClicked() {
  if (m_ui.treeCurrentPlugins->selectedItems().count() != 0) {
    int currInd = m_ui.treeCurrentPlugins->currentIndex().row();
    if (currInd > 0) {
      std::iter_swap(m_currPlugins.begin() + currInd,
                     m_currPlugins.begin() + currInd - 1);
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::moveDownClicked() {
  if (m_ui.treeCurrentPlugins->selectedItems().count() != 0) {
    unsigned int currInd = m_ui.treeCurrentPlugins->currentIndex().row();
    if (currInd < m_currPlugins.size() - 1) {
      std::iter_swap(m_currPlugins.begin() + currInd,
                     m_currPlugins.begin() + currInd + 1);
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::removeClicked() {
  // Also clear ADS entries
  if (m_ui.treeCurrentPlugins->selectedItems().count() != 0) {
    int currInd = m_ui.treeCurrentPlugins->currentIndex().row();
    auto curr = *(m_currPlugins.begin() + currInd);

    if (AnalysisDataService::Instance().doesExist(curr->getName())) {
      AnalysisDataService::Instance().remove(curr->getName());
    }
    m_currPlugins.erase(m_currPlugins.begin() + currInd);

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

    if (m_currPlugins.size() > 0) {
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

void TomoReconstruction::voidBrowseClicked() {
}

void TomoReconstruction::menuSaveClicked() {
  if (m_currentParamPath == "") {
    menuSaveAsClicked();
    return;
  }

  if (m_currPlugins.size() != 0) {
    std::string csvWorkspaceNames = "";
    for (auto it = m_currPlugins.begin(); it != m_currPlugins.end(); ++it) {
      csvWorkspaceNames = csvWorkspaceNames + (*it)->name();
      if (it != m_currPlugins.end() - 1)
        csvWorkspaceNames = csvWorkspaceNames + ",";
    }

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

QString TomoReconstruction::tableWSToString(ITableWorkspace_sptr table) {
  std::stringstream msg;
  TableRow row = table->getFirstRow();
  msg << "ID: " << table->cell<std::string>(0, 0) << std::endl
      << "Params: " << table->cell<std::string>(0, 1) << std::endl
      << "Name: " << table->cell<std::string>(0, 2) << std::endl
      << "Cite: " << table->cell<std::string>(0, 3);
  return QString::fromStdString(msg.str());
}

// Creates a treewidget item for a table workspace
void TomoReconstruction::createPluginTreeEntry(
    Mantid::API::ITableWorkspace_sptr table) {
  QStringList idStr, nameStr, citeStr, paramsStr;
  idStr.push_back(
      QString::fromStdString("ID: " + table->cell<std::string>(0, 0)));
  nameStr.push_back(
      QString::fromStdString("Name: " + table->cell<std::string>(0, 2)));
  citeStr.push_back(
      QString::fromStdString("Cite: " + table->cell<std::string>(0, 3)));
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
  std::string json = table->cell<std::string>(0, 1);
  ::Json::Reader r;
  if (r.parse(json, root)) {
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

      QStringList paramVal(QString::fromStdString(root[*it].asString()));
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
      m_ui.treeCurrentPlugins->setItemWidget(container, 0, w);
    }
  }

  pluginBaseItem->addChildren(items);
  m_ui.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
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
                             "interface: " + supported);
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
 * Show a warning message to the user (pop up)
 *
 * @param err Basic error title
 * @param description More detailed explanation, hints, additional
 * information, etc.
 */
void TomoReconstruction::userWarning(std::string err, std::string description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description),
                       QMessageBox::Ok, QMessageBox::Ok);
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
                        QString::fromStdString(description),
                        QMessageBox::Ok, QMessageBox::Ok);
}
