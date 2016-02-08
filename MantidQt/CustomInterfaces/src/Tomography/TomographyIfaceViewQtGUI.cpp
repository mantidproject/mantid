#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <boost/lexical_cast.hpp>

#include <Poco/Path.h>

#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomographyIfaceViewQtGUI::g_styleSheetOffline =
    "QPushButton { "
    "margin: 6px;"
    "border-color: #0c457e;"
    "border-style: outset;"
    "border-radius: 3px;"
    "border-width: 0px;"
    "color: black;"
    "background-color: rgb(180, 180, 180); "
    "}"
    "QPushButton:flat { "
    "background-color: rgb(180, 180, 180); "
    "}"
    "QPushButton:pressed { "
    "background-color: rgb(180, 180, 180) "
    "}";

const std::string TomographyIfaceViewQtGUI::g_styleSheetOnline =
    "QPushButton { "
    "margin: 6px;"
    "border-color: #0c457e;"
    "border-style: outset;"
    "border-radius: 3px;"
    "border-width: 0px;"
    "color: black;"
    "background-color: rgb(140, 255, 140); "
    "}"
    "QPushButton:flat { background-color: rgb(120, 255, 120); "
    "}"
    "QPushButton:pressed { background-color: rgb(120, 255, 120); "
    "}";

size_t TomographyIfaceViewQtGUI::g_nameSeqNo = 0;

std::string TomographyIfaceViewQtGUI::g_defLocalExternalPythonPath =
#ifdef _WIN32
    // assume we're using Aanconda Python and it is installed in c:/local
    // could also be c:/local/anaconda/scripts/ipython
    "c:/local/anaconda/python.exe";
#else
    // just use the system Python, assuming is in the system path
    "python";
#endif

// For paths where Python third party tools are installed, if they're
// not included in the system python path. For example you could have
// put the AstraToolbox package in
// C:/local/tomo-tools/astra/astra-1.6-python27-win-x64/astra-1.6
// Leaving this empty implies that the tools must have been installed
// in the system Python path. As an example, in the Anaconda python
// distribution for windows it could be in:
// c:/local/Anaconda/Lib/site-packages/
std::vector<std::string> TomographyIfaceViewQtGUI::g_defAddPathPython;

const std::string TomographyIfaceViewQtGUI::g_defRemotePathScripts =
    "/work/imat/phase_commissioning";

const std::string TomographyIfaceViewQtGUI::g_SCARFName = "SCARF@STFC";
const std::string TomographyIfaceViewQtGUI::g_defOutPathLocal =
#ifdef _WIN32
    "D:/imat/";
#else
    "~/imat/";
#endif

const std::string TomographyIfaceViewQtGUI::g_defOutPathRemote =
#ifdef _WIN32
    "I:/imat-data/";
#else
    "~/imat-data/";
#endif

const std::string TomographyIfaceViewQtGUI::g_defParaviewPath =
#ifdef _WIN32
    "C:\\Program Files\\ParaView\\";
#else
    "/usr/bin/";
#endif

const std::string TomographyIfaceViewQtGUI::g_defParaviewAppendPath =
#ifdef _WIN32
    "bin\\paraview.exe";
#else
    "paraview";
#endif

const std::string notAvailTool = "This tool is not available on this platform";

const std::string TomographyIfaceViewQtGUI::g_defOctopusVisPath =
#ifdef _WIN32
    "C:/Program Files/Octopus Imaging/Octopus Visualisation";
#else
    notAvailTool;
#endif

const std::string TomographyIfaceViewQtGUI::g_defOctopusAppendPath =
#ifdef _WIN32
    "octoviewer3d.exe";
#else
    notAvailTool;
#endif

const std::string TomographyIfaceViewQtGUI::g_defProcessedSubpath = "processed";

// names by which we know image/tomography reconstruction tools (3rd party)
const std::string TomographyIfaceViewQtGUI::g_TomoPyTool = "TomoPy";
const std::string TomographyIfaceViewQtGUI::g_AstraTool = "Astra";
const std::string TomographyIfaceViewQtGUI::g_CCPiTool = "CCPi CGLS";
const std::string TomographyIfaceViewQtGUI::g_SavuTool = "Savu";
const std::string TomographyIfaceViewQtGUI::g_customCmdTool = "Custom command";

// phase or cycle component, like: phase_commissioning, cycle_15_4, cycle_16_1
const std::string TomographyIfaceViewQtGUI::g_defPathComponentPhase =
    "phase_commissioning";

const std::string TomographyIfaceViewQtGUI::g_defRBNumber = "RB000XXX";

const std::string TomographyIfaceViewQtGUI::g_defPathReconScripts =
#ifdef _WIN32
    "C:/MantidInstall/scripts";
#else
    QDir::currentPath().toStdString();
#endif

const std::string TomographyIfaceViewQtGUI::g_defPathReconOut =
#ifdef _WIN32
    "D:/imat";
#else
    "~/imat/";
#endif

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
    : UserSubWindow(parent), ITomographyIfaceView(), m_tabROIW(NULL),
      m_processingJobsIDs(), m_currentComputeRes(""), m_currentReconTool(""),
      m_imgPath(""), m_logMsgs(), m_toolsSettings(), m_settings(),
      m_settingsGroup("CustomInterfaces/Tomography"), m_availPlugins(),
      m_currPlugins(), m_currentParamPath(), m_presenter(NULL) {

  // defaults from the tools
  m_tomopyMethod = ToolConfigTomoPy::methods().front().first;
  m_astraMethod = ToolConfigAstraToolbox::methods().front().first;

  // TODO: find a better place for this Savu stuff when the tool is
  // ready - see other TODOs
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
  QWidget *tabRunW = new QWidget(m_ui.tabMain);
  m_uiTabRun.setupUi(tabRunW);
  m_ui.tabMain->addTab(tabRunW, QString("Run"));
  QWidget *tabSetupW = new QWidget(m_ui.tabMain);
  m_uiTabSetup.setupUi(tabSetupW);
  m_ui.tabMain->addTab(tabSetupW, QString("Setup"));

  m_tabROIW = new ImageROIViewQtWidget(m_ui.tabMain);
  m_ui.tabMain->addTab(m_tabROIW, QString("ROI etc."));

  QWidget *tabFiltersW = new QWidget();
  m_uiTabFilters.setupUi(tabFiltersW);
  m_ui.tabMain->addTab(tabFiltersW, QString("Filters"));

  QWidget *tabVizW = new QWidget();
  m_uiTabVisualize.setupUi(tabVizW);
  m_ui.tabMain->addTab(tabVizW, QString("Visualize"));

  QWidget *tabConvertW = new QWidget();
  m_uiTabConvertFormats.setupUi(tabConvertW);
  m_ui.tabMain->addTab(tabConvertW, QString("Convert"));

  QWidget *tabEBandsW = new QWidget();
  m_uiTabEnergy.setupUi(tabEBandsW);
  m_ui.tabMain->addTab(tabEBandsW, QString("Energy bands"));

  readSettings();

  // basic UI setup
  doSetupGeneralWidgets();
  doSetupSectionSetup();
  doSetupSectionRun();
  doSetupSectionFilters();

  // extra / experimental tabs:
  doSetupSectionVisualize();
  doSetupSectionConvert();
  doSetupSectionEnergy();

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
  // 'local' - not disabled any longer
  // m_uiTabSetup.tabWidget_comp_resource->setTabEnabled(false, 1);
  // m_uiTabSetup.tab_local->setEnabled(false);

  resetRemoteSetup();

  // log in/out
  connect(m_uiTabSetup.pushButton_SCARF_login, SIGNAL(released()), this,
          SLOT(SCARFLoginClicked()));
  connect(m_uiTabSetup.pushButton_SCARF_logout, SIGNAL(released()), this,
          SLOT(SCARFLogoutClicked()));

  // RB number changes
  connect(m_uiTabSetup.lineEdit_cycle, SIGNAL(editingFinished()), this,
          SLOT(updatedCycleName()));

  // 'browse' buttons for image paths
  connect(m_uiTabSetup.pushButton_fits_dir, SIGNAL(released()), this,
          SLOT(fitsPathBrowseClicked()));
  connect(m_uiTabSetup.pushButton_flat_dir, SIGNAL(released()), this,
          SLOT(flatPathBrowseClicked()));
  connect(m_uiTabSetup.pushButton_dark_dir, SIGNAL(released()), this,
          SLOT(darkPathBrowseClicked()));

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

  // reset setup of the remote
  connect(m_uiTabSetup.pushButton_reset_scripts_base_dir, SIGNAL(released()),
          this, SLOT(resetRemoteSetup()));

  // 'browse' buttons for local conf:
  connect(m_uiTabSetup.pushButton_in_out_dir, SIGNAL(released()), this,
          SLOT(browseLocalInOutDirClicked()));
  connect(m_uiTabSetup.pushButton_recon_scripts_dir, SIGNAL(released()), this,
          SLOT(browseLocalReconScriptsDirClicked()));
}

void TomographyIfaceViewQtGUI::resetRemoteSetup() {
  m_uiTabSetup.lineEdit_scripts_base_dir->setText(
      QString::fromStdString(g_defRemotePathScripts));
  m_uiTabSetup.spinBox_SCARFnumNodes->setValue(1);
  m_uiTabSetup.spinBox_SCARFnumCores->setValue(8);
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

  updateCompResourceStatus(false);

  // enable by default, which will use default tools setups
  m_uiTabRun.pushButton_reconstruct->setEnabled(true);
  // setup always possible with local compute resource
  // m_uiTabRun.pushButton_run_tool_setup->setEnabled(false);
  m_uiTabRun.pushButton_run_tool_setup->setEnabled(true);
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

  // RB number changes
  connect(m_uiTabRun.lineEdit_rb_number, SIGNAL(editingFinished()), this,
          SLOT(updatedRBNumber()));

  // update tools for a resource
  connect(m_uiTabRun.comboBox_run_compute_resource,
          SIGNAL(currentIndexChanged(int)), this,
          SLOT(compResourceIndexChanged(int)));

  connect(m_uiTabRun.comboBox_run_tool, SIGNAL(currentIndexChanged(int)), this,
          SLOT(runToolIndexChanged(int)));
}

void TomographyIfaceViewQtGUI::doSetupSectionFilters() {
  connect(m_uiTabFilters.pushButton_reset, SIGNAL(released()), this,
          SLOT(resetPrePostFilters()));
}

void TomographyIfaceViewQtGUI::doSetupSectionVisualize() {
  // TODO: take g_def values first time, when Qsettings are empty, then from
  // QSettings
  m_setupParaviewPath = g_defParaviewPath;
  m_setupProcessedSubpath = g_defProcessedSubpath;
  m_setupOctopusVisPath = g_defOctopusVisPath;

  m_uiTabVisualize.lineEdit_processed_subpath->setText(
      QString::fromStdString(g_defProcessedSubpath));
  m_uiTabVisualize.lineEdit_paraview_location->setText(
      QString::fromStdString(g_defParaviewPath));

  // make a file system model for the visualization browser
  QFileSystemModel *model = new QFileSystemModel;
  model->setRootPath(QDir::currentPath());

  // set the model for the visualization browser
  m_uiTabVisualize.treeView_files->setModel(model);
  m_uiTabVisualize.treeView_files->setSelectionMode(
      QTreeView::ExtendedSelection);
  m_uiTabVisualize.treeView_files->setSelectionBehavior(QTreeView::SelectRows);

  // display: current dir
  const QString startDir =
      QString::fromStdString(Poco::Path::expand(g_defOutPathLocal));

  // start at default local path when possible
  const QString path =
      QString::fromStdString(Poco::Path::expand(g_defOutPathLocal));
  if (!path.isEmpty()) {
    m_uiTabVisualize.treeView_files->setRootIndex(model->index(path));
  } else {
    m_uiTabVisualize.treeView_files->setRootIndex(
        model->index(QDir::currentPath()));
  }

  connect(m_uiTabVisualize.pushButton_paraview, SIGNAL(released()), this,
          SLOT(sendToParaviewClicked()));

  connect(m_uiTabVisualize.pushButton_octopus, SIGNAL(released()), this,
          SLOT(sendToOctopusVisClicked()));

  connect(m_uiTabVisualize.pushButton_browse_files, SIGNAL(released()), this,
          SLOT(browseFilesToVisualizeClicked()));

  connect(m_uiTabVisualize.pushButton_local_default_dir, SIGNAL(released()),
          this, SLOT(defaultDirLocalVisualizeClicked()));
  connect(m_uiTabVisualize.pushButton_remote_default_dir, SIGNAL(released()),
          this, SLOT(defaultDirRemoteVisualizeClicked()));
}

void TomographyIfaceViewQtGUI::doSetupSectionConvert() {
  connect(m_uiTabConvertFormats.pushButton_browse_input, SIGNAL(released()),
          this, SLOT(browseImgInputConvertClicked()));

  connect(m_uiTabConvertFormats.pushButton_browse_output, SIGNAL(released()),
          this, SLOT(browseImgOutputConvertClicked()));
}

void TomographyIfaceViewQtGUI::doSetupSectionEnergy() {
  connect(m_uiTabEnergy.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseEnergyInputClicked()));

  connect(m_uiTabEnergy.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseEnergyOutputClicked()));
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
  if (online) {
    m_uiTabRun.pushButton_remote_status->setText("Online");
    // push buttons won't work with something like:
    // m_uiTabRun.pushButton_remote_status->setBackground(QColor(120, 255,
    // 120));
    m_uiTabRun.pushButton_remote_status->setStyleSheet(
        QString::fromStdString(g_styleSheetOnline));
  } else {
    m_uiTabRun.pushButton_remote_status->setText("Offline");
    m_uiTabRun.pushButton_remote_status->setStyleSheet(
        QString::fromStdString(g_styleSheetOffline));
  }
}

#ifndef _MSC_VER
QDataStream &operator>>(QDataStream &stream, size_t &num) {
  qint64 i;
  stream >> i;
  if (QDataStream::Ok == stream.status()) {
    num = i;
  }
  return stream;
}
#endif

/// deserialize a filters settings, from a QDataStream <= QByteArray
QDataStream &operator>>(QDataStream &stream, TomoReconFiltersSettings &fs) {
  // clang-format off
  stream >> fs.prep.normalizeByProtonCharge
                >> fs.prep.normalizeByFlatDark
                >> fs.prep.medianFilterWidth
                >> fs.prep.rotation
                >> fs.prep.maxAngle
                >> fs.prep.scaleDownFactor
                >> fs.postp.circMaskRadius
                >> fs.postp.cutOffLevel
                >> fs.outputPreprocImages;
  // clang-format on
  fs.prep.rotation *= 90;

  return stream;
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
  // tests and scripted runs may have issues. The CI builds could get stuck
  // when closing this interface.
  m_settings.onCloseAskForConfirmation =
      qs.value("on-close-ask-for-confirmation", false).toBool();

  m_settings.useKeepAlive =
      qs.value("use-keep-alive", m_settings.useKeepAlive).toInt();

  // Get all the pre-/post-processing options from a stream
  QByteArray rawFiltersSettings = qs.value("filters-settings").toByteArray();
  QDataStream stream(rawFiltersSettings);
  TomoReconFiltersSettings filtersSettings;
  stream >> filtersSettings;
  if (QDataStream::Ok == stream.status()) {
    setPrePostProcSettings(filtersSettings);
  } else {
    // something wrong in the settings previously saved => go back to factory
    // defaults
    TomoReconFiltersSettings def;
    setPrePostProcSettings(def);
  }

  // User parameters for reconstructions
  m_setupRBNumber = qs.value("RB-number", QString::fromStdString(g_defRBNumber))
                        .toString()
                        .toStdString();

  m_localExternalPythonPath =
      qs.value("path-local-external-python",
               QString::fromStdString(g_defLocalExternalPythonPath))
          .toString()
          .toStdString();

  int pathSize = qs.beginReadArray("path-default-add-for-python");
  for (int i = 0; i < pathSize; ++i) {
    qs.setArrayIndex(i);
    m_defAddPathPython.push_back(
        qs.value("value", "").toString().toStdString());
  }
  if (0 == m_defAddPathPython.size())
    m_defAddPathPython = g_defAddPathPython;
  qs.endArray();

  m_setupPathComponentPhase =
      qs.value("path-default-phase-component",
               QString::fromStdString(g_defPathComponentPhase))
          .toString()
          .toStdString();
  m_setupPathReconScripts =
      qs.value("path-reconstruction-scripts",
               QString::fromStdString(g_defPathReconScripts))
          .toString()
          .toStdString();
  m_setupPathReconOut = qs.value("path-reconstruction-output",
                                 QString::fromStdString(g_defPathReconOut))
                            .toString()
                            .toStdString();

  // general GUI state
  m_ui.tabMain->setCurrentIndex(qs.value("selected-tab-index").toInt());

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());

  qs.endGroup();

  m_uiTabSetup.lineEdit_SCARF_path->setText(
      QString::fromStdString(m_settings.SCARFBasePath));
}

#ifndef _MSC_VER
QDataStream &operator<<(QDataStream &stream, size_t const &num) {
  return stream << static_cast<qint64>(num);
}
#endif

/// serialize a filters settings, as a QDataStream => QByteArray
QDataStream &operator<<(QDataStream &stream,
                        TomoReconFiltersSettings const &fs) {
  // clang-format off
  stream << fs.prep.normalizeByProtonCharge
                << fs.prep.normalizeByFlatDark
                << fs.prep.medianFilterWidth
                << (fs.prep.rotation/90)
                << fs.prep.maxAngle
                << fs.prep.scaleDownFactor
                << fs.postp.circMaskRadius
                << fs.postp.cutOffLevel
                << fs.outputPreprocImages;
  // clang-format on

  return stream;
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

  // Save all the pre-/post-processing options through a stream
  QByteArray filtersSettings;
  QDataStream stream(&filtersSettings, QIODevice::WriteOnly);
  stream << grabPrePostProcSettings();
  qs.setValue("filters-settings", filtersSettings);

  // User parameters for reconstructions
  qs.setValue("RB-number", QString::fromStdString(m_setupRBNumber));

  qs.setValue("path-local-external-python",
              QString::fromStdString(m_localExternalPythonPath));

  qs.beginWriteArray("path-default-add-for-python");
  for (size_t i = 0; i < m_defAddPathPython.size(); ++i) {
    qs.setArrayIndex(static_cast<int>(i));
    qs.setValue("value", QString::fromStdString(m_defAddPathPython[i]));
  }
  qs.endArray();

  qs.setValue("path-default-phase-component",
              QString::fromStdString(m_setupPathComponentPhase));

  qs.setValue("path-reconstruction-scripts",
              QString::fromStdString(m_setupPathReconScripts));

  qs.setValue("path-reconstruction-output",
              QString::fromStdString(m_setupPathReconOut));

  // general GUI status
  qs.setValue("selected-tab-index", m_ui.tabMain->currentIndex());

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
  auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "LoadSavuTomoConfig");
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
  QString run = "/work/imat/phase_commissioning/scripts/Imaging/IMAT/"
                "tomo_reconstruct.py"; // m_uiAstra.lineEdit_runnable->text();
  static size_t reconIdx = 1;

  const std::string localOutNameAppendix =
      std::string("/processed/") + "reconstruction_" + std::to_string(reconIdx);

  if (g_TomoPyTool == name) {
    TomoToolConfigTomoPy tomopy;
    m_uiTomoPy.setupUi(&tomopy);
    m_uiTomoPy.comboBox_method->clear();
    auto methods = ToolConfigTomoPy::methods();
    for (size_t i = 0; i < methods.size(); i++) {
      m_uiTomoPy.comboBox_method->addItem(
          QString::fromStdString(methods[i].second));
    }
    int res = tomopy.exec();

    if (QDialog::Accepted == res) {
      // TODO: move this
      int mi = m_uiTomoPy.comboBox_method->currentIndex();

      TomoPathsConfig paths = currentPathsConfig();
      // TODO: for the output path, probably better to take the sample path,
      // then up one level
      m_toolsSettings.tomoPy = ToolConfigTomoPy(
          run.toStdString(),
          g_defOutPathLocal + "/" +
              m_uiTabSetup.lineEdit_cycle->text().toStdString() + "/" +
              m_uiTabRun.lineEdit_rb_number->text().toStdString() +
              localOutNameAppendix,
          paths.pathDark(), paths.pathOpenBeam(), paths.pathSamples());
      m_tomopyMethod = methods[mi].first;
    }
  } else if (g_AstraTool == name) {
    TomoToolConfigAstra astra;
    m_uiAstra.setupUi(&astra);
    m_uiAstra.comboBox_method->clear();
    auto methods = ToolConfigAstraToolbox::methods();
    for (size_t i = 0; i < methods.size(); i++) {
      m_uiAstra.comboBox_method->addItem(
          QString::fromStdString(methods[i].second));
    }
    int res = astra.exec();

    if (QDialog::Accepted == res) {
      // TODO: move this
      int mi = m_uiAstra.comboBox_method->currentIndex();

      TomoPathsConfig paths = currentPathsConfig();
      // TODO: for the output path, probably better to take the sample path,
      // then up one level
      m_toolsSettings.astra = ToolConfigAstraToolbox(
          run.toStdString(),
          Poco::Path::expand(
              g_defOutPathLocal + "/" +
              m_uiTabSetup.lineEdit_cycle->text().toStdString() + "/" +
              m_uiTabRun.lineEdit_rb_number->text().toStdString() +
              localOutNameAppendix),
          paths.pathDark(), paths.pathOpenBeam(), paths.pathSamples());
      m_astraMethod = methods[mi].first;
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
    const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &status,
    const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &
        localStatus) {

  QTableWidget *t = m_uiTabRun.tableWidget_run_jobs;
  bool sort = t->isSortingEnabled();
  t->setRowCount(static_cast<int>(status.size() + localStatus.size()));

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

    // beware "Exit" is called "Exited" on the web portal, but the REST
    // responses
    // call it "Exit"
    if (std::string::npos != status[i].status.find("Exit") ||
        std::string::npos != status[i].status.find("Suspend"))
      t->item(ii, 3)->setBackground(QColor(255, 120, 120)); // Qt::red
    else if (std::string::npos != status[i].status.find("Pending"))
      t->item(ii, 3)->setBackground(QColor(150, 150, 150)); // Qt::gray
    else if (std::string::npos != status[i].status.find("Running") ||
             std::string::npos != status[i].status.find("Active"))
      t->item(ii, 3)->setBackground(QColor(120, 120, 255)); // Qt::blue
    else if (std::string::npos != status[i].status.find("Finished") ||
             std::string::npos != status[i].status.find("Done"))
      t->item(ii, 3)->setBackground(QColor(120, 255, 120)); // Qt::green

    t->setItem(ii, 4,
               new QTableWidgetItem(QString::fromStdString(status[i].cmdLine)));
  }

  // Local processes
  for (size_t i = 0; i < localStatus.size(); ++i) {

    // This won't work well, at least on windows.
    // bool runs = Poco::isRunning(
    //    boost::lexical_cast<Poco::Process::PID>(localStatus[i].id));
    // if (!runs)
    //  m_localStatus[i].status = "Done";

    int ii = static_cast<int>(status.size() + i);
    t->setItem(ii, 0, new QTableWidgetItem(QString::fromStdString("local")));
    t->setItem(ii, 1, new QTableWidgetItem(
                          QString::fromStdString(localStatus[i].name)));
    t->setItem(ii, 2,
               new QTableWidgetItem(QString::fromStdString(localStatus[i].id)));

    t->setItem(ii, 3, new QTableWidgetItem(
                          QString::fromStdString(localStatus[i].status)));

    // beware "Exit" is called "Exited" on the web portal, but the
    // REST responses call it "Exit"
    if (std::string::npos != localStatus[i].status.find("Exit") ||
        std::string::npos != localStatus[i].status.find("Suspend"))
      t->item(ii, 3)->setBackground(QColor(255, 120, 120)); // Qt::red
    else if (std::string::npos != localStatus[i].status.find("Pending"))
      t->item(ii, 3)->setBackground(QColor(150, 150, 150)); // Qt::gray
    else if (std::string::npos != localStatus[i].status.find("Running") ||
             std::string::npos != localStatus[i].status.find("Active"))
      t->item(ii, 3)->setBackground(QColor(120, 120, 255)); // Qt::blue
    else if (std::string::npos != localStatus[i].status.find("Finished") ||
             std::string::npos != localStatus[i].status.find("Done"))
      t->item(ii, 3)->setBackground(QColor(120, 255, 120)); // Qt::green

    t->setItem(ii, 4, new QTableWidgetItem(
                          QString::fromStdString(localStatus[i].cmdLine)));
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

void TomographyIfaceViewQtGUI::browseLocalInOutDirClicked() {
  m_setupPathReconScripts =
      checkUserBrowsePath(m_uiTabSetup.lineEdit_local_out_recon_dir);
}

void TomographyIfaceViewQtGUI::browseLocalReconScriptsDirClicked() {
  m_setupPathReconScripts =
      checkUserBrowsePath(m_uiTabSetup.lineEdit_local_recon_scripts);
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
  /*
  // This would remember every widget's old value, and not the last path
  QString prev;
  if (le->text().isEmpty()) {
    prev = algPrev;
  } else {
    prev = le->text();
  }
  */
  QString prev = algPrev;

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

    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
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
  const double scaleFactor = 255.0 / max_min;
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

TomoReconFiltersSettings TomographyIfaceViewQtGUI::prePostProcSettings() const {
  return grabPrePostProcSettings();
}

TomoReconFiltersSettings
TomographyIfaceViewQtGUI::grabPrePostProcSettings() const {
  TomoReconFiltersSettings opts;

  // pre-processing
  opts.prep.normalizeByProtonCharge =
      m_uiTabFilters.checkBox_normalize_proton_charge->isChecked();

  opts.prep.normalizeByFlatDark =
      m_uiTabFilters.checkBox_normalize_flat_dark->isChecked();

  opts.prep.medianFilterWidth = static_cast<size_t>(
      m_uiTabFilters.spinBox_prep_median_filter_width->value());

  opts.prep.rotation =
      90 * m_uiTabFilters.comboBox_prep_rotation->currentIndex();

  opts.prep.maxAngle = m_uiTabFilters.doubleSpinBox_prep_max_angle->value();

  opts.prep.scaleDownFactor =
      static_cast<size_t>(m_uiTabFilters.spinBox_prep_scale_factor->value());

  // post-processing
  opts.postp.circMaskRadius =
      m_uiTabFilters.doubleSpinBox_post_circ_mask->value();

  opts.postp.cutOffLevel = m_uiTabFilters.doubleSpinBox_post_cutoff->value();

  // outputs
  opts.outputPreprocImages =
      m_uiTabFilters.checkBox_out_preproc_images->isChecked();

  return opts;
}

void TomographyIfaceViewQtGUI::setPrePostProcSettings(
    TomoReconFiltersSettings &opts) const {

  // pre-processing
  m_uiTabFilters.checkBox_normalize_proton_charge->setChecked(
      opts.prep.normalizeByProtonCharge);

  m_uiTabFilters.checkBox_normalize_flat_dark->setChecked(
      opts.prep.normalizeByFlatDark);

  m_uiTabFilters.spinBox_prep_median_filter_width->setValue(
      static_cast<int>(opts.prep.medianFilterWidth));

  m_uiTabFilters.comboBox_prep_rotation->setCurrentIndex(
      static_cast<int>(opts.prep.rotation / 90));

  m_uiTabFilters.doubleSpinBox_prep_max_angle->setValue(opts.prep.maxAngle);

  m_uiTabFilters.spinBox_prep_scale_factor->setValue(
      static_cast<int>(opts.prep.scaleDownFactor));

  // post-processing
  m_uiTabFilters.doubleSpinBox_post_circ_mask->setValue(
      opts.postp.circMaskRadius);

  m_uiTabFilters.doubleSpinBox_post_cutoff->setValue(opts.postp.cutOffLevel);

  // outputs
  m_uiTabFilters.checkBox_out_preproc_images->setChecked(
      opts.outputPreprocImages);
}

void TomographyIfaceViewQtGUI::resetPrePostFilters() {
  // default constructors with factory defaults
  TomoReconFiltersSettings def;
  setPrePostProcSettings(def);
}

void TomographyIfaceViewQtGUI::sendToOctopusVisClicked() {
  sendToVisTool("Octopus Visualization 3D", m_setupOctopusVisPath,
                g_defOctopusAppendPath);
}

void TomographyIfaceViewQtGUI::sendToParaviewClicked() {
  sendToVisTool("ParaView", m_setupParaviewPath, g_defParaviewAppendPath);
}

/**
 * Start a third party tool as a process. TODO: This is a very early
 * experimental implementation that should be moved out of this view.
 *
 * @param toolName Human understandable name of the tool/program
 * @param pathString Path where the tool is installed
 * @param appendBin string to append to the path if required, example:
 * bin/tool.exe
 */
void TomographyIfaceViewQtGUI::sendToVisTool(const std::string &toolName,
                                             const std::string &pathString,
                                             const std::string &appendBin) {
  // prepare external tool executable path
  Poco::Path tmpPath(pathString);
  if (!appendBin.empty()) {
    tmpPath.append(appendBin);
  }
  const std::string toolPath = tmpPath.toString();

  // get path to pass as parameter
  const QFileSystemModel *model = dynamic_cast<QFileSystemModel *>(
      m_uiTabVisualize.treeView_files->model());
  if (!model)
    return;

  auto selection =
      m_uiTabVisualize.treeView_files->selectionModel()->selectedIndexes();
  // just take the first selected item/directory
  if (selection.empty())
    return;
  QString selPath = model->filePath(selection.first());

  // Execute
  std::vector<std::string> args;
  args.push_back(selPath.toStdString());

  sendLog("Executing visualization tool: " + toolName + ". Executing: '" +
          toolPath + "', with parameters: '" + args[0] + "'.");
  try {
    Mantid::Kernel::ConfigService::Instance().launchProcess(toolPath, args);
  } catch (std::runtime_error &rexc) {
    sendLog("The execution of " + toolName + "failed. details: " +
            std::string(rexc.what()));
    userWarning("Execution failed ",
                "Coult not execute the tool. Error details: " +
                    std::string(rexc.what()));
  }
}

void TomographyIfaceViewQtGUI::browseFilesToVisualizeClicked() {
  // an alternative would be to start from the current selection, instead of the
  // current root:
  const QFileSystemModel *model = dynamic_cast<QFileSystemModel *>(
      m_uiTabVisualize.treeView_files->model());
  if (!model)
    return;

  const QString currentPath = model->rootPath();
  QString path(QFileDialog::getExistingDirectory(
      this, tr("Select root directory/folder with processed data "
               "(reconstructions) under it"),
      currentPath));

  if (!path.isEmpty()) {
    m_uiTabVisualize.treeView_files->setRootIndex(model->index(path));
  }
}

void TomographyIfaceViewQtGUI::defaultDirLocalVisualizeClicked() {
  const QFileSystemModel *model = dynamic_cast<QFileSystemModel *>(
      m_uiTabVisualize.treeView_files->model());
  if (!model)
    return;

  const QString path =
      QString::fromStdString(Poco::Path::expand(g_defOutPathLocal));
  if (!path.isEmpty()) {
    m_uiTabVisualize.treeView_files->setRootIndex(model->index(path));
  }
}

void TomographyIfaceViewQtGUI::defaultDirRemoteVisualizeClicked() {
  const QFileSystemModel *model = dynamic_cast<QFileSystemModel *>(
      m_uiTabVisualize.treeView_files->model());
  if (!model)
    return;

  const QString path =
      QString::fromStdString(Poco::Path::expand(g_defOutPathRemote));
  if (!path.isEmpty()) {
    m_uiTabVisualize.treeView_files->setRootIndex(model->index(path));
  }
}

void TomographyIfaceViewQtGUI::browseVisToolParaviewClicke() {
  m_setupParaviewPath =
      checkUserBrowsePath(m_uiTabConvertFormats.lineEdit_input);
}

void TomographyIfaceViewQtGUI::browseVisToolOctopusClicked() {
  m_setupOctopusVisPath =
      checkUserBrowsePath(m_uiTabConvertFormats.lineEdit_input);
}

void TomographyIfaceViewQtGUI::browseImgInputConvertClicked() {
  // Not using this path to update the "current" path where to load from, but
  // it could be an option.
  // const std::string path =
  checkUserBrowsePath(m_uiTabConvertFormats.lineEdit_input);
  // m_pathsConfig.updatePathDark(str);
  // m_presenter->notify(ITomographyIfacePresenter::TomoPathsChanged);
}

void TomographyIfaceViewQtGUI::browseImgOutputConvertClicked() {
  // Not using this path to update the "current" path where to load from, but
  // it could be an option.
  // const std::string path =
  checkUserBrowsePath(m_uiTabConvertFormats.lineEdit_output);
  // m_pathsConfig.updatePathDark(str);
  // m_presenter->notify(ITomographyIfacePresenter::TomoPathsChanged);
}

void TomographyIfaceViewQtGUI::browseEnergyInputClicked() {
  checkUserBrowsePath(m_uiTabEnergy.lineEdit_input_path);
}

void TomographyIfaceViewQtGUI::browseEnergyOutputClicked() {
  checkUserBrowsePath(m_uiTabEnergy.lineEdit_output_path);
}

std::string
TomographyIfaceViewQtGUI::checkUserBrowsePath(QLineEdit *le,
                                              const std::string &userMsg) {

  QString prev;
  if (le->text().isEmpty()) {
    prev =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  } else {
    prev = le->text();
  }

  QString path(QFileDialog::getExistingDirectory(
      this, tr(QString::fromStdString(userMsg)), prev));

  if (!path.isEmpty()) {
    le->setText(path);
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  }

  return path.toStdString();
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

void TomographyIfaceViewQtGUI::updatedCycleName() {
  m_setupPathComponentPhase = m_uiTabSetup.lineEdit_cycle->text().toStdString();
}

void TomographyIfaceViewQtGUI::updatedRBNumber() {
  m_setupRBNumber = m_uiTabRun.lineEdit_rb_number->text().toStdString();
  // Might have to change: m_uiTabRun.lineEdit_local_out_recon_dir as well
}

/**
 * To log a message without waiting.
 */
void TomographyIfaceViewQtGUI::sendLog(const std::string &msg) {
  m_logMsgs.push_back(msg);
  m_presenter->notify(ITomographyIfacePresenter::LogMsg);
  m_logMsgs.clear();
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
