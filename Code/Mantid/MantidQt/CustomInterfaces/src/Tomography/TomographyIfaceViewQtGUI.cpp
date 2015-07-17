#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <boost/lexical_cast.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

size_t TomographyIfaceViewQtGUI::g_nameSeqNo = 0;

const std::string TomographyIfaceViewQtGUI::g_SCARFName = "SCARF@STFC";
const std::string TomographyIfaceViewQtGUI::g_defOutPath =
    "/work/imat/runs_output";

// names by which we know image/tomography reconstruction tools (3rd party)
const std::string TomographyIfaceViewQtGUI::g_TomoPyTool = "TomoPy";
const std::string TomographyIfaceViewQtGUI::g_AstraTool = "Astra";
const std::string TomographyIfaceViewQtGUI::g_CCPiTool = "CCPi CGLS";
const std::string TomographyIfaceViewQtGUI::g_SavuTool = "Savu";
const std::string TomographyIfaceViewQtGUI::g_customCmdTool = "Custom command";

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(TomographyIfaceViewQtGUI)

/**
 * Default constructor, but note that this interface currently
 * relies on the SCARF cluster (only in ISIS facility) as the only
 * supported remote compute resource.
 *
 * @param parent Parent window (most likely the Mantid main app window).
 */
TomographyIfaceViewQtGUI::TomographyIfaceViewQtGUI(QWidget *parent)
    : UserSubWindow(parent), ITomographyIfaceView(), m_processingJobsIDs(),
      m_currentComputeRes(""), m_currentReconTool(""), m_imgPath(""),
      m_logMsgs(), m_toolsSettings(), m_settings(),
      m_settingsGroup("CustomInterfaces/Tomography"), m_availPlugins(),
      m_currPlugins(), m_currentParamPath(), m_presenter(NULL) {

  // TODO: find a better place for this Savu stuff - see other TODOs
  m_availPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_availPlugins->addColumns("str", "name", 4);
  m_currPlugins = Mantid::API::WorkspaceFactory::Instance().createTable();
  m_currPlugins->addColumns("str", "name", 4);
}

TomographyIfaceViewQtGUI::~TomographyIfaceViewQtGUI() {}

void TomographyIfaceViewQtGUI::initLayout() {
  // setup container ui
  m_ui.setupUi(this);
  // add tab contents and set up their ui's
  QWidget *tab1w = new QWidget(m_ui.tabMain);
  m_uiTabRun.setupUi(tab1w);
  m_ui.tabMain->addTab(tab1w, QString("Run"));
  QWidget *tab2w = new QWidget(m_ui.tabMain);
  m_uiTabSetup.setupUi(tab2w);
  m_ui.tabMain->addTab(tab2w, QString("Setup"));

  readSettings();

  // basic UI setup
  doSetupGeneralWidgets();
  doSetupSectionSetup();
  doSetupSectionRun();

  // presenter that knows how to handle a ITomographyIfaceView should take care
  // of all the logic
  // note the view needs to now the concrete presenter
  m_presenter.reset(new TomographyIfacePresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(ITomographyIfacePresenter::SetupResourcesAndTools);
}

void TomographyIfaceViewQtGUI::doSetupGeneralWidgets() {
  // Menu Items
  connect(m_ui.actionOpen, SIGNAL(triggered()), this, SLOT(menuOpenClicked()));
  connect(m_ui.actionSave, SIGNAL(triggered()), this, SLOT(menuSaveClicked()));
  connect(m_ui.actionSaveAs, SIGNAL(triggered()), this,
          SLOT(menuSaveAsClicked()));

  connect(m_ui.pushButton_help, SIGNAL(released()), this, SLOT(openHelpWin()));
  // note connection to the parent window, otherwise you'd be left
  // with an empty frame window
  connect(m_ui.pushButton_close, SIGNAL(released()), this->parent(),
          SLOT(close()));
}

void TomographyIfaceViewQtGUI::doSetupSectionSetup() {
  // disable 'local' for now
  m_uiTabSetup.tabWidget_comp_resource->setTabEnabled(false, 1);
  m_uiTabSetup.tab_local->setEnabled(false);

  m_uiTabSetup.groupBox_run_config->setEnabled(false);

  connect(m_uiTabSetup.pushButton_SCARF_login, SIGNAL(released()), this,
          SLOT(SCARFLoginClicked()));
  connect(m_uiTabSetup.pushButton_SCARF_logout, SIGNAL(released()), this,
          SLOT(SCARFLogoutClicked()));

  // populate setup values from defaults
  m_uiTabSetup.lineEdit_path_FITS->setText(
      QString::fromStdString(m_pathsConfig.pathSamples()));
  m_uiTabSetup.lineEdit_path_flat->setText(
      QString::fromStdString(m_pathsConfig.pathOpenBeam()));
  m_uiTabSetup.lineEdit_path_dark->setText(
      QString::fromStdString(m_pathsConfig.pathDark()));
  m_uiTabSetup.lineEdit_SCARF_path->setText(
      QString::fromStdString(m_pathsConfig.pathBase()));
  m_uiTabSetup.lineEdit_scripts_base_dir->setText(
      QString::fromStdString(m_pathsConfig.pathScriptsTools()));

  // 'browse' buttons
  connect(m_uiTabSetup.pushButton_fits_dir, SIGNAL(released()), this,
          SLOT(fitsPathBrowseClicked()));
  connect(m_uiTabSetup.pushButton_flat_dir, SIGNAL(released()), this,
          SLOT(flatPathBrowseClicked()));
  connect(m_uiTabSetup.pushButton_dark_dir, SIGNAL(released()), this,
          SLOT(darkPathBrowseClicked()));
}

void TomographyIfaceViewQtGUI::doSetupSectionRun() {
  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(420);
  sizes.push_back(80);
  m_uiTabRun.splitter_run_main_vertical->setSizes(sizes);

  sizes[0] = 470;
  sizes[1] = 30;
  m_uiTabRun.splitter_image_resource->setSizes(sizes);

  sizes[0] = 400;
  sizes[1] = 100;
  m_uiTabRun.splitter_run_jobs->setSizes(sizes);

  m_uiTabRun.label_image_name->setText("none");

  m_uiTabRun.pushButton_reconstruct->setEnabled(false);
  m_uiTabRun.pushButton_run_tool_setup->setEnabled(false);
  m_uiTabRun.pushButton_run_job_cancel->setEnabled(false);
  m_uiTabRun.pushButton_run_job_visualize->setEnabled(false);

  // Button signals
  connect(m_uiTabRun.pushButton_browse_image, SIGNAL(released()), this,
          SLOT(browseImageClicked()));
  connect(m_uiTabRun.pushButton_reconstruct, SIGNAL(released()), this,
          SLOT(reconstructClicked()));
  connect(m_uiTabRun.pushButton_run_tool_setup, SIGNAL(released()), this,
          SLOT(toolSetupClicked()));
  connect(m_uiTabRun.pushButton_run_refresh, SIGNAL(released()), this,
          SLOT(jobTableRefreshClicked()));
  connect(m_uiTabRun.pushButton_run_job_visualize, SIGNAL(released()), this,
          SLOT(runVisualizeClicked()));
  connect(m_uiTabRun.pushButton_run_job_cancel, SIGNAL(released()), this,
          SLOT(jobCancelClicked()));

  // update tools for a resource
  connect(m_uiTabRun.comboBox_run_compute_resource,
          SIGNAL(currentIndexChanged(int)), this,
          SLOT(compResourceIndexChanged(int)));

  connect(m_uiTabRun.comboBox_run_tool, SIGNAL(currentIndexChanged(int)), this,
          SLOT(runToolIndexChanged(int)));
}

void TomographyIfaceViewQtGUI::setComputeResources(
    const std::vector<std::string> &resources,
    const std::vector<bool> &enabled) {
  // set up the compute resource
  QComboBox *cr = m_uiTabRun.comboBox_run_compute_resource;
  if (!cr || resources.size() != enabled.size())
    return;

  cr->clear();

  for (size_t ri = 0; ri < resources.size(); ri++) {
    cr->addItem(QString::fromStdString(resources[ri]));

    if (!enabled[ri]) {
      // trick to display the text in a disabled row
      QModelIndex idx = cr->model()->index(static_cast<int>(ri), 0);
      QVariant disabled(0);
      cr->model()->setData(idx, disabled, Qt::UserRole - 1);
    }
  }
}

// This is here while savu becomes available and we find a better place for savu
// stuff
void TomographyIfaceViewQtGUI::doSetupSavu() {
  // geometry, etc. niceties
  // on the left (just plugin names) 1/2, right: 2/3
  QList<int> sizes;
  sizes.push_back(100);
  sizes.push_back(200);
  m_uiSavu.splitterPlugins->setSizes(sizes);

  // Setup Parameter editor tab
  loadAvailablePlugins();
  m_uiSavu.treeCurrentPlugins->setHeaderHidden(true);

  // Connect slots

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
}

void TomographyIfaceViewQtGUI::setReconstructionTools(
    const std::vector<std::string> &tools, const std::vector<bool> &enabled) {

  // set up the reconstruction tool
  QComboBox *rt = m_uiTabRun.comboBox_run_tool;
  if (!rt || tools.size() != enabled.size())
    return;

  rt->clear();

  for (size_t ti = 0; ti < tools.size(); ti++) {
    rt->addItem(QString::fromStdString(tools[ti]));

    if (!enabled[ti]) {
      // trick to display it in a disabled row
      QModelIndex idx = rt->model()->index(static_cast<int>(ti), 0);
      QVariant disabled(0);
      rt->model()->setData(idx, disabled, Qt::UserRole - 1);
    }
  }
}

/**
 * Enables/disables buttons that require the user to be logged into
 * the (remote) compute resource, for example: reconstruct (submit job),
 * cancel job, etc.
 */
void TomographyIfaceViewQtGUI::enableLoggedActions(bool enable) {
  // TODO: this may not make sense anymore when/if the "Local" compute
  // resource is used in the future (except when none of the tools
  // supported are available/detected on "Local")
  std::vector<QPushButton *> buttons;
  buttons.push_back(m_uiTabRun.pushButton_run_refresh);
  buttons.push_back(m_uiTabRun.pushButton_run_job_cancel);
  // no visualization yet, need vsi etc. support
  // buttons.push_back(m_uiTabSetup.pushButton_run_job_visualize);
  buttons.push_back(m_uiTabRun.pushButton_reconstruct);

  for (size_t i = 0; i < buttons.size(); ++i) {
    buttons[i]->setEnabled(enable);
  }

  if (!enable) {
    m_uiTabRun.pushButton_reconstruct->setToolTip(
        "Start reconstruction job. You need to be logged in to use this");
  } else {
    m_uiTabRun.pushButton_reconstruct->setToolTip("");
  }
}

/**
 * Handle display of the current status of the remote/local compute resource
 * that is selected by the user.
 *
 * @param online whether to show good/working/online status
 */
void TomographyIfaceViewQtGUI::updateCompResourceStatus(bool online) {
  if (online)
    m_uiTabRun.pushButton_remote_status->setText("Online");
  else
    m_uiTabRun.pushButton_remote_status->setText("Offline");
}

/**
 * Load the settings for the tabs and widgets of the interface. This
 * relies on Qt settings functionality (QSettings class).
 *
 * This includes setting the default browsing directory to be the
 * default save directory.
 */
void TomographyIfaceViewQtGUI::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  m_settings.SCARFBasePath =
      qs.value("SCARF-base-path",
               QString::fromStdString(m_settings.SCARFBasePath))
          .toString()
          .toStdString();
  // WARNING: it's critical to keep 'false' as default value, otherwise
  // scripted runs may have issues. The CI builds could get stuck when
  // closing this interface.
  m_settings.onCloseAskForConfirmation =
      qs.value("on-close-ask-for-confirmation", false).toBool();

  m_settings.useKeepAlive =
      qs.value("use-keep-alive", m_settings.useKeepAlive).toInt();
  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();

  m_uiTabSetup.lineEdit_SCARF_path->setText(
      QString::fromStdString(m_settings.SCARFBasePath));
}

/**
 * Save persistent settings. Qt based.
 */
void TomographyIfaceViewQtGUI::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));
  QString s = m_uiTabSetup.lineEdit_SCARF_path->text();
  qs.setValue("SCARF-base-path", s);
  qs.setValue("on-close-ask-for-confirmation",
              m_settings.onCloseAskForConfirmation);
  qs.setValue("use-keep-alive", m_settings.useKeepAlive);
  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

/**
 * Load a savu tomo config file into the current plugin list, overwriting it.
 * Uses the algorithm LoadSavuTomoConfig
 */
void TomographyIfaceViewQtGUI::loadSavuTomoConfig(
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
std::string TomographyIfaceViewQtGUI::createUniqueNameHidden() {
  std::string name;
  do {
    // with __ prefix => hidden
    name = "__TomoConfigTableWS_Seq_" +
           boost::lexical_cast<std::string>(g_nameSeqNo++);
  } while (AnalysisDataService::Instance().doesExist(name));

  return name;
}

/// needs to at least update the 'tool' combo box
void TomographyIfaceViewQtGUI::compResourceIndexChanged(int /* i */) {
  QComboBox *rt = m_uiTabRun.comboBox_run_compute_resource;
  if (!rt)
    return;

  // TODO validateCompResource(rt->currentText().toStdString());
  m_currentComputeRes = rt->currentText().toStdString();
  m_presenter->notify(ITomographyIfacePresenter::CompResourceChanged);
}

void TomographyIfaceViewQtGUI::runToolIndexChanged(int /* i */) {
  QComboBox *rt = m_uiTabRun.comboBox_run_tool;
  if (!rt)
    return;

  m_currentReconTool = rt->currentText().toStdString();
  m_presenter->notify(ITomographyIfacePresenter::ToolChanged);
}

void TomographyIfaceViewQtGUI::enableConfigTool(bool on) {
  m_uiTabRun.pushButton_run_tool_setup->setEnabled(on);
}

void TomographyIfaceViewQtGUI::enableRunReconstruct(bool on) {
  m_uiTabRun.pushButton_reconstruct->setEnabled(on);
}

/**
 * Update or toggle on/off the log in/out control widgets (like
 * enabling/disabling push buttons, updating a display that tells the
 * user if we're logged in, etc.).
 *
 * @param loggedIn Status (true when logged in)
 */
void TomographyIfaceViewQtGUI::updateLoginControls(bool loggedIn) {
  m_uiTabSetup.pushButton_SCARF_login->setEnabled(!loggedIn);
  m_uiTabSetup.pushButton_SCARF_logout->setEnabled(loggedIn);

  enableLoggedActions(loggedIn);
  updateCompResourceStatus(loggedIn);
}

/**
 * Slot for when the 'login' or similar button is clicked (released)
 */
void TomographyIfaceViewQtGUI::SCARFLoginClicked() {
  m_presenter->notify(ITomographyIfacePresenter::LogInRequested);
}

/**
 * Slot for when the 'logout' or similar button is clicked (released)
 */
void TomographyIfaceViewQtGUI::SCARFLogoutClicked() {
  m_presenter->notify(ITomographyIfacePresenter::LogOutRequested);
}

/**
 * Slot for when the user requests to open the tool specific setup dialog.
 */
void TomographyIfaceViewQtGUI::toolSetupClicked() {
  QComboBox *rt = m_uiTabRun.comboBox_run_tool;
  if (!rt)
    return;

  m_presenter->notify(ITomographyIfacePresenter::SetupReconTool);
}

/**
 * Displays and gets the results of a tool specific configuration dialog.
 *
 * @param name Name of the (tomographic reconstruction) tool
 */
void TomographyIfaceViewQtGUI::showToolConfig(const std::string &name) {
  if (g_TomoPyTool == name) {
    TomoToolConfigTomoPy tomopy;
    m_uiTomoPy.setupUi(&tomopy);
    int res = tomopy.exec();
    if (QDialog::Accepted == res) {
      // TODO: move this
      int mi = m_uiTomoPy.comboBox_method->currentIndex();
      QString run = m_uiTomoPy.lineEdit_runnable->text();
      if (1 == mi) {
        // hard-coded for now, this is a different script on SCARF that should
        // be
        // integrated with the FBP script
        run = "/work/imat/runs-scripts/scripts/tomopy/imat_recon_SIRT.py";
      }
      double minAngle = m_uiTomoPy.doubleSpinBox_angle_min->value();
      double maxAngle = m_uiTomoPy.doubleSpinBox_angle_max->value();
      double cor = m_uiTomoPy.doubleSpinBox_center_rot->value();

      TomoPathsConfig paths = currentPathsConfig();
      m_toolsSettings.tomoPy = ToolConfigTomoPy(
          run.toStdString(), g_defOutPath, paths.pathDark(),
          paths.pathOpenBeam(), paths.pathSamples(), cor, minAngle, maxAngle);
    }
  } else if (g_AstraTool == name) {
    TomoToolConfigAstra astra;
    m_uiAstra.setupUi(&astra);
    int res = astra.exec();
    if (QDialog::Accepted == res) {
      // TODO: move this
      int mi = m_uiAstra.comboBox_method->currentIndex();
      QString run = m_uiAstra.lineEdit_runnable->text();
      if (1 == mi) {
        // hard-coded for now, this is a different script on SCARF
        run = "/work/imat/runs-scripts/scripts/astra/astra-3d-SIRT3D.py";
      }
      double cor = m_uiAstra.doubleSpinBox_center_rot->value();
      double minAngle = m_uiAstra.doubleSpinBox_angle_min->value();
      double maxAngle = m_uiAstra.doubleSpinBox_angle_max->value();

      TomoPathsConfig paths = currentPathsConfig();
      m_toolsSettings.astra = ToolConfigAstraToolbox(
          run.toStdString(), cor, minAngle, maxAngle, g_defOutPath,
          paths.pathDark(), paths.pathOpenBeam(), paths.pathSamples());
    }
  } else if (g_SavuTool == name) {
    // TODO: savu not ready. This is a temporary kludge, it just shows
    // the setup dialog so we can chat about it.
    TomoToolConfigSavu savu;
    m_uiSavu.setupUi(&savu);
    doSetupSavu();
    savu.setWindowModality(Qt::ApplicationModal);
    savu.show();
    QEventLoop el;
    connect(this, SIGNAL(destroyed()), &el, SLOT(quit()));
    el.exec();
  } else if (g_customCmdTool == name) {
    TomoToolConfigCustom cmd;
    m_uiCustom.setupUi(&cmd);
    int res = cmd.exec();
    if (QDialog::Accepted == res) {
      // TODO: move this
      QString run = m_uiCustom.lineEdit_runnable->text();
      QString opts = m_uiCustom.textEdit_cl_opts->toPlainText();

      m_toolsSettings.custom =
          ToolConfigCustom(run.toStdString(), opts.toStdString());
    }
  }
  // TODO: 'CCPi CGLS' tool maybe in the future. Tool not ready.
}

/**
 * Slot - when the user clicks the 'reconstruct data' or similar button.
 */
void TomographyIfaceViewQtGUI::reconstructClicked() {
  m_presenter->notify(ITomographyIfacePresenter::RunReconstruct);
}

/**
 * Slot - when the user clicks the 'visualize job results' or similar button.
 */
void TomographyIfaceViewQtGUI::runVisualizeClicked() {
  QTableWidget *tbl = m_uiTabRun.tableWidget_run_jobs;
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
  if (idSel.count() > 1) {
    m_processingJobsIDs.clear();
    m_processingJobsIDs.push_back(id);
    m_presenter->notify(ITomographyIfacePresenter::VisualizeJobFromTable);
  }
}

/**
 * Slot - when the user clicks the 'cancel job' or similar button.
 */
void TomographyIfaceViewQtGUI::jobCancelClicked() {
  m_processingJobsIDs.clear();
  QTableWidget *tbl = m_uiTabRun.tableWidget_run_jobs;
  const int idCol = 2;
  QTableWidgetItem *hdr = tbl->horizontalHeaderItem(idCol);
  if ("ID" != hdr->text())
    throw std::runtime_error("Expected to get the Id of jobs from the "
                             "second column of the table of jobs, but I "
                             "found this at that column: " +
                             hdr->text().toStdString());

  QModelIndexList idSel = tbl->selectionModel()->selectedRows();
  for (int i = 0; i < idSel.count(); ++i) {
    const std::string id =
        tbl->item(idSel[i].row(), idCol)->text().toStdString();
    m_processingJobsIDs.push_back(id);
  }

  m_presenter->notify(ITomographyIfacePresenter::CancelJobFromTable);
}

/**
 * Slot - when the user clicks the 'refresh job list/table' or similar button.
 */
void TomographyIfaceViewQtGUI::jobTableRefreshClicked() {
  m_presenter->notify(ITomographyIfacePresenter::RefreshJobs);
}

/**
 * Slot - user clicks the 'open/browse image' or similar button.
 */
void TomographyIfaceViewQtGUI::browseImageClicked() {
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

  m_imgPath = path.toStdString();
  m_presenter->notify(ITomographyIfacePresenter::ViewImg);
}

/**
 * Update the job status and general info table/tree from the info
 * stored in this class' data members, which ideally should have
 * information from a recent query to the server.
 */
void TomographyIfaceViewQtGUI::updateJobsInfoDisplay(
    const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &status) {

  QTableWidget *t = m_uiTabRun.tableWidget_run_jobs;
  bool sort = t->isSortingEnabled();
  t->setRowCount(static_cast<int>(status.size()));

  for (size_t i = 0; i < status.size(); ++i) {
    int ii = static_cast<int>(i);
    t->setItem(ii, 0,
               new QTableWidgetItem(QString::fromStdString(g_SCARFName)));
    t->setItem(ii, 1,
               new QTableWidgetItem(QString::fromStdString(status[i].name)));
    t->setItem(ii, 2,
               new QTableWidgetItem(QString::fromStdString(status[i].id)));
    t->setItem(ii, 3,
               new QTableWidgetItem(QString::fromStdString(status[i].status)));
    t->setItem(ii, 4,
               new QTableWidgetItem(QString::fromStdString(status[i].cmdLine)));
  }

  t->setSortingEnabled(sort);
}

std::string TomographyIfaceViewQtGUI::getUsername() const {
  if (g_SCARFName ==
      m_uiTabRun.comboBox_run_compute_resource->currentText().toStdString())
    return m_uiTabSetup.lineEdit_SCARF_username->text().toStdString();
  else
    return "invalid";
}

/**
 * Retrieve the username being used for the selected compute resource.
 *
 * @return Username ready to be used in remote queries
 */
std::string TomographyIfaceViewQtGUI::getPassword() const {
  if (g_SCARFName ==
      m_uiTabRun.comboBox_run_compute_resource->currentText().toStdString())
    return m_uiTabSetup.lineEdit_SCARF_password->text().toStdString();
  else
    return "none";
}

void TomographyIfaceViewQtGUI::fitsPathBrowseClicked() {
  std::string str;
  processPathBrowseClick(m_uiTabSetup.lineEdit_path_FITS, str);
  m_pathsConfig.updatePathSamples(str);
  m_presenter->notify(ITomographyIfacePresenter::TomoPathsChanged);
}

void TomographyIfaceViewQtGUI::flatPathBrowseClicked() {
  std::string str;
  processPathBrowseClick(m_uiTabSetup.lineEdit_path_flat, str);
  m_pathsConfig.updatePathOpenBeam(str);
  m_presenter->notify(ITomographyIfacePresenter::TomoPathsChanged);
}

void TomographyIfaceViewQtGUI::darkPathBrowseClicked() {
  std::string str;
  processPathBrowseClick(m_uiTabSetup.lineEdit_path_dark, str);
  m_pathsConfig.updatePathDark(str);
  m_presenter->notify(ITomographyIfacePresenter::TomoPathsChanged);
}

/**
 * Get path from user and update a line edit and a variable.
 *
 * @param le a line edit where the path is shown.
 * @param data variable where the path is stored (in addition to the line
 * edit object).
 */
void TomographyIfaceViewQtGUI::processPathBrowseClick(QLineEdit *le,
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
    std::string pp = path.toStdString();
    // to UNIX, assuming SCARF or similar
    boost::replace_all(pp, "\\", "/");
    if (pp.length() >= 2 && ':' == pp[1]) {
      if (2 == pp.length())
        pp = ""; // don't accept '/'
      else
        pp = pp.substr(2);
    }

    le->setText(QString::fromStdString(pp));
    data = pp;
  }
}

void TomographyIfaceViewQtGUI::showImage(const std::string &path) {
  QString qpath = QString::fromStdString(path);
  QImage rawImg(qpath);
  QPainter painter;
  QPixmap pix(rawImg.width(), rawImg.height());
  painter.begin(&pix);
  painter.drawImage(0, 0, rawImg);
  painter.end();
  m_uiTabRun.label_image->setPixmap(pix);
  m_uiTabRun.label_image->show();

  m_uiTabRun.label_image_name->setText(qpath);
}

void TomographyIfaceViewQtGUI::showImage(const MatrixWorkspace_sptr &ws) {
  // This draw an image on screen using Qt's QPixmap and QImage.
  // From logs we expect a name "run_title", width "Axis1" and height "Axis2"
  const size_t MAXDIM = 2048 * 16;
  size_t width;
  try {
    width = boost::lexical_cast<size_t>(ws->run().getLogData("Axis1")->value());
    // TODO: add a settings option for this (like max mem allocation for
    // images)?
    if (width >= MAXDIM)
      width = MAXDIM;
  } catch (std::exception &e) {
    userError("Cannot load image", "There was a problem while trying to "
                                   "find the width of the image: " +
                                       std::string(e.what()));
    return;
  }

  size_t height;
  try {
    height =
        boost::lexical_cast<size_t>(ws->run().getLogData("Axis2")->value());
    if (height >= MAXDIM)
      height = MAXDIM;
  } catch (std::exception &e) {
    userError("Cannot load image", "There was a problem while trying to "
                                   "find the height of the image: " +
                                       std::string(e.what()));
    return;
  }

  std::string name;
  try {
    name = ws->run().getLogData("run_title")->value();
    m_logMsgs.push_back(" Visualizing image: " + name);
    m_presenter->notify(ITomographyIfacePresenter::LogMsg);
    m_logMsgs.clear();
  } catch (std::exception &e) {
    userWarning("Cannot load image information",
                "There was a problem while "
                " trying to find the name of the image: " +
                    std::string(e.what()));
  }

  // images are loaded as 1 histogram == 1 pixel (1 bin per histogram):
  if (height != ws->getNumberHistograms() || width != ws->blocksize()) {
    userError("Image dimensions do not match in the input image workspace",
              "Could not load the expected "
              "number of rows and columns.");
    return;
  }
  // find min and max to scale pixel values
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      const double &v = ws->readY(i)[j];
      if (v < min)
        min = v;
      if (v > max)
        max = v;
    }
  }
  if (min >= max) {
    userWarning("Empty image!",
                "The image could be loaded but it contains "
                "effectively no information, all pixels have the same value.");
    // black picture
    QPixmap pix(static_cast<int>(width), static_cast<int>(height));
    pix.fill(QColor(0, 0, 0));
    m_uiTabRun.label_image->setPixmap(pix);
    m_uiTabRun.label_image->show();
    return;
  }

  // load / transfer image into a QImage
  QImage rawImg(QSize(static_cast<int>(width), static_cast<int>(height)),
                QImage::Format_RGB32);
  const double max_min = max - min;
  const double scaleFactor = 255.0/max_min;
  for (size_t yi = 0; yi < width; ++yi) {
    for (size_t xi = 0; xi < width; ++xi) {
      const double &v = ws->readY(yi)[xi];
      // color the range min-max in gray scale. To apply different color
      // maps you'd need to use rawImg.setColorTable() or similar.
      const int scaled = static_cast<int>(scaleFactor * (v - min));
      QRgb vRgb = qRgb(scaled, scaled, scaled);
      rawImg.setPixel(static_cast<int>(xi), static_cast<int>(yi), vRgb);
    }
  }

  // paint and show image
  QPainter painter;
  QPixmap pix(static_cast<int>(width), static_cast<int>(height));
  painter.begin(&pix);
  painter.drawImage(0, 0, rawImg);
  painter.end();
  m_uiTabRun.label_image->setPixmap(pix);
  m_uiTabRun.label_image->show();

  m_uiTabRun.label_image_name->setText(QString::fromStdString(name));
}

/**
* Show a warning message to the user (pop up)
*
* @param err Basic error title
* @param description More detailed explanation, hints, additional
* information, etc.
*/
void TomographyIfaceViewQtGUI::userWarning(const std::string &err,
                                           const std::string &description) {
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
void TomographyIfaceViewQtGUI::userError(const std::string &err,
                                         const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void TomographyIfaceViewQtGUI::closeEvent(QCloseEvent *event) {
  int answer = QMessageBox::AcceptRole;

  bool ask = m_settings.onCloseAskForConfirmation;
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

    m_settings.onCloseAskForConfirmation = confirmCheckBox.isChecked();
    answer = msgBox.exec();
  }

  if (answer == QMessageBox::AcceptRole) {
    // TODO? cleanup();
    m_presenter->notify(ITomographyIfacePresenter::ShutDown);
    event->accept();
  } else {
    event->ignore();
  }
}

void TomographyIfaceViewQtGUI::openHelpWin() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Tomographic_Reconstruction"));
}

} // namespace CustomInterfaces
} // namespace MantidQt
