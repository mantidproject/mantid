// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SANSRunWindow.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/V3D.h"

#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "SANSAddFiles.h"
#include "SANSBackgroundCorrectionSettings.h"
#include "SANSEventSlicing.h"

#include <QClipboard>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>

#include <Poco/Message.h>
#include <Poco/StringTokenizer.h>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include <cmath>

using Mantid::detid_t;

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(SANSRunWindow)

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::API;
using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid;
using Mantid::Geometry::Instrument_const_sptr;

namespace {
/// static logger for main window
Logger g_log("SANSRunWindow");
/// static logger for centre finding
Logger g_centreFinderLog("CentreFinder");

using ReductionSettings_sptr = boost::shared_ptr<Kernel::PropertyManager>;

/**
 * Returns the PropertyManager object that is used to store the settings
 * used by the reduction.
 *
 * There is a corresponding function in scripts/SANS/isis_reducer.py with
 * more information.
 *
 * @returns the reduction settings.
 */
ReductionSettings_sptr getReductionSettings() {
  // Must match name of the PropertyManager used in the reduction.
  static const std::string SETTINGS_PROP_MAN_NAME = "ISISSANSReductionSettings";

  if (!PropertyManagerDataService::Instance().doesExist(
          SETTINGS_PROP_MAN_NAME)) {
    g_log.debug()
        << "Creating reduction settings PropertyManager object, with name "
        << SETTINGS_PROP_MAN_NAME << ".";

    const auto propertyManager = boost::make_shared<Kernel::PropertyManager>();
    PropertyManagerDataService::Instance().add(SETTINGS_PROP_MAN_NAME,
                                               propertyManager);

    return propertyManager;
  }

  return PropertyManagerDataService::Instance().retrieve(
      SETTINGS_PROP_MAN_NAME);
}

/**
 * Returns the value of the setting with given name, unless the setting does not
 * exist in which case the given defaultValue is returned.
 *
 * @param settingName :: the name of the setting who's value to return
 * @param defaultValue :: the value to return if the setting does not exist
 *
 * @returns the setting value else defaultValue if the setting does not exist
 */
QString getSettingWithDefault(const QString &settingName,
                              const QString &defaultValue) {
  const auto settings = getReductionSettings();

  if (settings->existsProperty(settingName.toStdString()))
    return QString::fromStdString(
        settings->getPropertyValue(settingName.toStdString()));
  else
    return defaultValue;
}

/**
 * Convenience method to set the setting with given name to the given value.
 * If a property with the given name does not exist, then one is created.
 *
 * We could have a templated method at some later date, but at the moment this
 * only works for string properties.
 *
 * @param settingName :: the name of the setting to set
 * @param settingValue :: the value to set this setting with
 */
void setStringSetting(const QString &settingName, const QString &settingValue) {
  const auto settings = getReductionSettings();
  const auto name = settingName.toStdString();
  const auto value = settingValue.toStdString();

  if (!settings->existsProperty(name))
    settings->declareProperty(
        Kernel::make_unique<Kernel::PropertyWithValue<std::string>>(name, ""),
        value);
  else
    settings->setProperty(name, value);
}

/**
 * Converts a c++ bool into a Python string representation.
 * @param input: a c++ bool
 * @returns a string which is either True or False
 */
QString convertBoolToPythonBoolString(bool input) {
  return input
             ? MantidQt::CustomInterfaces::SANSConstants::getPythonTrueKeyword()
             : MantidQt::CustomInterfaces::SANSConstants::
                   getPythonFalseKeyword();
}

/**
 * Converts string representation of a Python bool to a C++ bool
 * @param input: the python string representation
 * @returns a true or false
 */
bool convertPythonBoolStringToBool(QString input) {
  bool value = false;
  if (input ==
      MantidQt::CustomInterfaces::SANSConstants::getPythonTrueKeyword()) {
    value = true;
  } else if (input == MantidQt::CustomInterfaces::SANSConstants::
                          getPythonFalseKeyword()) {
    value = false;
  }

  return value;
}

void setTransmissionOnSaveCommand(
    QString &saveCommand, Mantid::API::MatrixWorkspace_sptr matrix_workspace,
    const QString &detectorSelection) {
  if (matrix_workspace->getInstrument()->getName() == "SANS2D")
    saveCommand += "'front-detector, rear-detector'";
  if (matrix_workspace->getInstrument()->getName() == "LOQ")
    saveCommand += "'HAB, main-detector-bank'";
  if (matrix_workspace->getInstrument()->getName() == "LARMOR")
    saveCommand += "'" + detectorSelection + "'";

  /* From v2, SaveCanSAS1D is able to save the Transmission workspaces
  related to the
  reduced data. The name of workspaces of the Transmission are
  available at the
  sample logs. This part add the parameters Transmission=trans_ws_name
  and
  TransmissionCan=trans_ws_name_can if they are available at the
  Workspace Sample log
  and still available inside MantidPlot. */
  const Mantid::API::Run &run = matrix_workspace->run();
  QStringList list;
  list << "Transmission"
       << "TransmissionCan";
  foreach (QString property, list) {
    if (run.hasProperty(property.toStdString())) {
      std::string trans_ws_name =
          run.getLogData(property.toStdString())->value();
      if (AnalysisDataService::Instance().isValid(trans_ws_name).empty()) {
        saveCommand += ", " + property + "=\"" +
                       QString::fromStdString(trans_ws_name) + "\"";
      }
    }
  }
}

bool checkSaveOptions(QString &message, bool is1D, bool isCanSAS) {
  // Check we are dealing with 1D or 2D data
  bool isValid = true;

  if (!is1D && isCanSAS) {
    isValid = false;
    message += "Save option issue: Cannot save in CanSAS format for 2D data.\n";
  }
  return isValid;
}
} // namespace

//----------------------------------------------
// Public member functions
//----------------------------------------------
/// Constructor
SANSRunWindow::SANSRunWindow(QWidget *parent)
    : UserSubWindow(parent), m_addFilesTab(nullptr), m_displayTab(nullptr),
      m_diagnosticsTab(nullptr), m_saveWorkspaces(nullptr), m_ins_defdir(""),
      m_last_dir(""), m_cfg_loaded(true), m_userFname(false), m_sample_file(),
      m_reducemapper(nullptr), m_warnings_issued(false), m_force_reload(false),
      m_newInDir(*this, &SANSRunWindow::handleInputDirChange),
      m_delete_observer(*this, &SANSRunWindow::handleMantidDeleteWorkspace),
      m_s2d_detlabels(), m_loq_detlabels(), m_allowed_batchtags(),
      m_have_reducemodule(false), m_dirty_batch_grid(false),
      m_tmp_batchfile(""), m_batch_paste(nullptr), m_batch_clear(nullptr),
      m_mustBeDouble(nullptr), m_doubleValidatorZeroToMax(nullptr),
      m_intValidatorZeroToMax(nullptr), slicingWindow(nullptr) {
  ConfigService::Instance().addObserver(m_newInDir);
}

/// Destructor
SANSRunWindow::~SANSRunWindow() {
  try {
    ConfigService::Instance().removeObserver(m_newInDir);
    if (isInitialized()) {
      // Seems to crash on destruction of if I don't do this
      AnalysisDataService::Instance().notificationCenter.removeObserver(
          m_delete_observer);
      saveSettings();
      delete m_addFilesTab;
    }
    delete m_displayTab;
    delete m_diagnosticsTab;
  } catch (...) {
    // we've cleaned up the best we can, move on
  }
}

//--------------------------------------------
// Private member functions
//--------------------------------------------
/**
 * Set up the dialog layout
 */
void SANSRunWindow::initLayout() {
  g_log.debug("Initializing interface layout");
  m_uiForm.setupUi(this);
  m_uiForm.inst_opt->addItem("LARMOR");
  m_uiForm.inst_opt->addItem("LOQ");
  m_uiForm.inst_opt->addItem("SANS2D");
  m_uiForm.inst_opt->addItem("SANS2DTUBES");

  m_reducemapper = new QSignalMapper(this);

  // Set column stretch on the mask table
  m_uiForm.mask_table->horizontalHeader()->setStretchLastSection(true);

  setupSaveBox();

  connectButtonSignals();

  m_uiForm.tabWidget->setCurrentWidget(m_uiForm.runNumbers);
  // Disable most things so that load is the only thing that can be done
  m_uiForm.oneDBtn->setEnabled(false);
  m_uiForm.twoDBtn->setEnabled(false);
  m_uiForm.saveDefault_btn->setEnabled(false);
  for (int i = 1; i < 4; ++i) {
    m_uiForm.tabWidget->setTabEnabled(i, false);
  }

  // Mode switches
  connect(m_uiForm.single_mode_btn, SIGNAL(clicked()), this,
          SLOT(switchMode()));
  connect(m_uiForm.batch_mode_btn, SIGNAL(clicked()), this, SLOT(switchMode()));

  // Set a custom context menu for the batch table
  m_uiForm.batch_table->setContextMenuPolicy(Qt::ActionsContextMenu);
  m_batch_paste = new QAction(tr("&Paste"), m_uiForm.batch_table);
  m_batch_paste->setShortcut(tr("Ctrl+P"));
  connect(m_batch_paste, SIGNAL(triggered()), this, SLOT(pasteToBatchTable()));
  m_uiForm.batch_table->addAction(m_batch_paste);

  m_batch_clear = new QAction(tr("&Clear"), m_uiForm.batch_table);
  m_uiForm.batch_table->addAction(m_batch_clear);
  connect(m_batch_clear, SIGNAL(triggered()), this, SLOT(clearBatchTable()));

  // Main Logging
  m_uiForm.logging_field->attachLoggingChannel();
  connect(m_uiForm.logging_field, SIGNAL(warningReceived(const QString &)),
          this, SLOT(setLoggerTabTitleToWarn()));
  connect(m_uiForm.logger_clear, SIGNAL(clicked()), this, SLOT(clearLogger()));

  // Centre finder logger
  m_uiForm.centre_logging->attachLoggingChannel();
  connect(m_uiForm.clear_centre_log, SIGNAL(clicked()), m_uiForm.centre_logging,
          SLOT(clear()));
  connect(m_uiForm.up_down_checkbox, SIGNAL(stateChanged(int)), this,
          SLOT(onUpDownCheckboxChanged()));
  connect(m_uiForm.left_right_checkbox, SIGNAL(stateChanged(int)), this,
          SLOT(onLeftRightCheckboxChanged()));

  // Create the widget hash maps
  initWidgetMaps();

  m_runFiles.reserve(6);
  // Text edit map
  m_runFiles.push_back(m_uiForm.scatterSample);
  m_runFiles.push_back(m_uiForm.scatCan);

  m_runFiles.push_back(m_uiForm.transmis);
  m_runFiles.push_back(m_uiForm.transCan);

  m_runFiles.push_back(m_uiForm.direct);
  m_runFiles.push_back(m_uiForm.dirCan);
  std::vector<MWRunFiles *>::const_iterator it = m_runFiles.begin();
  for (; it != m_runFiles.end(); ++it) {
    (*it)->doButtonOpt(MWRunFiles::Icon);
  }

  connectFirstPageSignals();

  initAnalysDetTab();

  if (!m_addFilesTab) { // sets up the AddFiles tab which must be deleted in the
                        // destructor
    m_addFilesTab = new SANSAddFiles(this, &m_uiForm);
  }

  // diagnostics tab
  if (!m_diagnosticsTab) {
    m_diagnosticsTab = new SANSDiagnostics(this, &m_uiForm);
  }
  connect(this, SIGNAL(userfileLoaded()), m_diagnosticsTab,
          SLOT(enableMaskFileControls()));
  // Listen for Workspace delete signals
  AnalysisDataService::Instance().notificationCenter.addObserver(
      m_delete_observer);

  // Create the "Display" tab
  if (!m_displayTab) {
    m_displayTab = new SANSPlotSpecial(this);
    m_uiForm.displayLayout->addWidget(m_displayTab);
  }

  const QString ISIS_SANS_WIKI = "http://www.mantidproject.org/ISIS_SANS:";
  m_helpPageUrls[Tab::RUN_NUMBERS] = ISIS_SANS_WIKI + "_Run_Numbers";
  m_helpPageUrls[Tab::REDUCTION_SETTINGS] =
      ISIS_SANS_WIKI + "_Reduction_Settings";
  m_helpPageUrls[Tab::GEOMETRY] = ISIS_SANS_WIKI + "_Geometry";
  m_helpPageUrls[Tab::MASKING] = ISIS_SANS_WIKI + "_Masking";
  m_helpPageUrls[Tab::LOGGING] = ISIS_SANS_WIKI + "_Logging";
  m_helpPageUrls[Tab::ADD_RUNS] = ISIS_SANS_WIKI + "_Add_Runs";
  m_helpPageUrls[Tab::DIAGNOSTICS] = ISIS_SANS_WIKI + "_Diagnostics";
  m_helpPageUrls[Tab::ONE_D_ANALYSIS] = ISIS_SANS_WIKI + "_1D_Analysis";

  // connect up phi masking on analysis tab to be in sync with info on masking
  // tab
  connect(m_uiForm.mirror_phi, SIGNAL(clicked()), this,
          SLOT(phiMaskingChanged()));
  connect(m_uiForm.detbank_sel, SIGNAL(currentIndexChanged(int)), this,
          SLOT(phiMaskingChanged(int)));
  connect(m_uiForm.phi_min, SIGNAL(editingFinished()), this,
          SLOT(phiMaskingChanged()));
  connect(m_uiForm.phi_max, SIGNAL(editingFinished()), this,
          SLOT(phiMaskingChanged()));
  connect(m_uiForm.slicePb, SIGNAL(clicked()), this,
          SLOT(handleSlicePushButton()));
  connect(m_uiForm.pushButton_Help, SIGNAL(clicked()), this,
          SLOT(openHelpPage()));

  // Setup the Transmission Settings
  initTransmissionSettings();

  // Setup the QResolution Settings
  initQResolutionSettings();

  // Set the validators
  setValidators();

  readSettings();
}
/** Ssetup the controls for the Analysis Tab on this form
 */
void SANSRunWindow::initAnalysDetTab() {
  // Add shortened forms of step types to step boxes
  m_uiForm.q_dq_opt->setItemData(0, "LIN");
  m_uiForm.q_dq_opt->setItemData(1, "LOG");
  m_uiForm.qy_dqy_opt->setItemData(0, "LIN");
  // remove the following two lines once the beamfinder is in the new framework
  m_uiForm.wav_dw_opt->setItemData(0, "LIN");
  m_uiForm.wav_dw_opt->setItemData(1, "LOG");

  // the file widget always has a *.* filter, passing an empty list means we get
  // only that
  m_uiForm.floodRearFile->setAlgorithmProperty("CorrectToFile|Filename");
  m_uiForm.floodRearFile->isOptional(true);
  m_uiForm.floodFrontFile->setAlgorithmProperty("CorrectToFile|Filename");
  m_uiForm.floodFrontFile->isOptional(true);

  // the unicode code for the angstrom symbol is 197, doing the below keeps this
  // file ASCII compatible
  static const QChar ANGSROM_SYM(197);
  m_uiForm.wavlength_lb->setText(QString("Wavelength (%1)").arg(ANGSROM_SYM));
  m_uiForm.qx_lb->setText(QString("Qx (%1^-1)").arg(ANGSROM_SYM));
  m_uiForm.qxy_lb->setText(QString("Qxy (%1^-1)").arg(ANGSROM_SYM));
  m_uiForm.transFitOnOff->setText(QString("Trans Fit (%1)").arg(ANGSROM_SYM));
  m_uiForm.transFitOnOff_can->setText(
      QString("Trans Fit (%1)").arg(ANGSROM_SYM));
  m_uiForm.q_rebin->setToolTip(
      "Any string allowed by the Rebin algorithm may be used");

  makeValidator(m_uiForm.wavRanVal_lb, m_uiForm.wavRanges, m_uiForm.tab_2,
                "A comma separated list of numbers is required here");

  connectAnalysDetSignals();
}

/** Formats a Qlabel to be a validator and adds it to the list
 *  @param newValid :: a QLabel to use as a validator
 *  @param control :: the control whose entry the validator is validates
 *  @param tab :: the tab that contains this widgets
 *  @param errorMsg :: the tooltip message that the validator should have
 */
void SANSRunWindow::makeValidator(QLabel *const newValid, QWidget *control,
                                  QWidget *tab, const QString &errorMsg) {
  QPalette pal = newValid->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  newValid->setPalette(pal);
  newValid->setToolTip(errorMsg);

  // register the validator       and say      where it's control is
  m_validators[newValid] = std::pair<QWidget *, QWidget *>(control, tab);
}

/**
 * Run local Python initialization code
 */
void SANSRunWindow::initLocalPython() {
  // Import the SANS module and set the correct instrument
  QString result = runPythonCode(
      "try:\n\timport isis_reducer\nexcept (ImportError,SyntaxError) as "
      "details:\tprint('Error importing isis_reducer: ' + str(details))\n");
  if (result.trimmed().isEmpty()) {
    m_have_reducemodule = true;
  } else {
    showInformationBox(result);
    m_have_reducemodule = false;
    setProcessingState(NoSample);
  }
  runPythonCode("import ISISCommandInterface as i\nimport copy");
  runPythonCode("import isis_instrument\nimport isis_reduction_steps");

  // Make sure that user file is valid
  if (!isValidUserFile()) {
    m_cfg_loaded = false;
  } else {
    loadUserFile();
    handleInstrumentChange();
    m_cfg_loaded = true;
  }
}

/** Initialise some of the data and signal connections in the save box
 */
void SANSRunWindow::setupSaveBox() {
  connect(m_uiForm.saveDefault_btn, SIGNAL(clicked()), this,
          SLOT(handleDefSaveClick()));
  connect(m_uiForm.saveSel_btn, SIGNAL(clicked()), this,
          SLOT(saveWorkspacesDialog()));
  connect(m_uiForm.saveFilename_btn, SIGNAL(clicked()), this,
          SLOT(saveFileBrowse()));
  connect(m_uiForm.outfile_edit, SIGNAL(textEdited(const QString &)), this,
          SLOT(setUserFname()));

  // link the save option tick boxes to their save algorithm
  m_savFormats.insert(m_uiForm.saveCan_check, "SaveCanSAS1D");
  m_savFormats.insert(m_uiForm.saveRKH_check, "SaveRKH");
  m_savFormats.insert(m_uiForm.saveNXcanSAS_check, "SaveNXcanSAS");

  for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end();
       ++i) {
    connect(i.key(), SIGNAL(stateChanged(int)), this,
            SLOT(enableOrDisableDefaultSave()));
  }
}
/** Raises a saveWorkspaces dialog which allows people to save any workspace
 *  workspaces the user chooses
 */
void SANSRunWindow::saveWorkspacesDialog() {
  // Qt::WA_DeleteOnClose must be set for the dialog to aviod a memory leak
  m_saveWorkspaces =
      new SaveWorkspaces(this, m_uiForm.outfile_edit->text(), m_savFormats,
                         m_uiForm.zeroErrorCheckBox->isChecked());
  // this dialog sometimes needs to run Python, pass this to Mantidplot via our
  // runAsPythonScript() signal
  connect(m_saveWorkspaces, SIGNAL(runAsPythonScript(const QString &, bool)),
          this, SIGNAL(runAsPythonScript(const QString &, bool)));
  // we need know if we have a pointer to a valid window or not
  connect(m_saveWorkspaces, SIGNAL(closing()), this,
          SLOT(saveWorkspacesClosed()));
  // Connect the request for a zero-error-free workspace
  // cpp-check does not understand that the input are two references
  connect(m_saveWorkspaces,
          SIGNAL(createZeroErrorFreeWorkspace(QString &, QString &)), this,
          SLOT(createZeroErrorFreeClone(QString &, QString &)));
  // Connect the request for deleting a zero-error-free workspace
  connect(m_saveWorkspaces, SIGNAL(deleteZeroErrorFreeWorkspace(QString &)),
          this, SLOT(deleteZeroErrorFreeClone(QString &)));
  // Connect to change in the zero-error removal checkbox
  connect(m_uiForm.zeroErrorCheckBox, SIGNAL(stateChanged(int)),
          m_saveWorkspaces, SLOT(onSaveAsZeroErrorFreeChanged(int)));
  // Connect the transfer of geometry inforamtion
  connect(m_saveWorkspaces, SIGNAL(updateGeometryInformation()), this,
          SLOT(onUpdateGeometryRequest()));
  connect(this,
          SIGNAL(sendGeometryInformation(QString &, QString &, QString &,
                                         QString &)),
          m_saveWorkspaces,
          SLOT(onUpdateGeomtryInformation(QString &, QString &, QString &,
                                          QString &)));

  m_uiForm.saveSel_btn->setEnabled(false);
  m_saveWorkspaces->show();
}
/**When the save workspaces dialog box is closes its pointer, m_saveWorkspaces,
 * is set to NULL and the raise dialog button is re-enabled
 */
void SANSRunWindow::saveWorkspacesClosed() {
  m_uiForm.saveSel_btn->setEnabled(true);
  m_saveWorkspaces = nullptr;
}
/** Connection the buttons to their signals
 */
void SANSRunWindow::connectButtonSignals() {
  connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
  connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this,
          SLOT(selectUserFile()));
  connect(m_uiForm.csv_browse_btn, SIGNAL(clicked()), this,
          SLOT(selectCSVFile()));

  connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this,
          SLOT(handleLoadButtonClick()));
  connect(m_uiForm.runcentreBtn, SIGNAL(clicked()), this,
          SLOT(handleRunFindCentre()));

  // Reduction buttons
  connect(m_uiForm.oneDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
  m_reducemapper->setMapping(m_uiForm.oneDBtn, "1D");
  connect(m_uiForm.twoDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
  m_reducemapper->setMapping(m_uiForm.twoDBtn, "2D");
  connect(m_reducemapper, SIGNAL(mapped(const QString &)), this,
          SLOT(handleReduceButtonClick(const QString &)));

  connect(m_uiForm.showMaskBtn, SIGNAL(clicked()), this,
          SLOT(handleShowMaskButtonClick()));
}
/**  Calls connect to fix up all the slots for the run tab to their events
 */
void SANSRunWindow::connectFirstPageSignals() {
  // controls on the first tab page

  connect(m_uiForm.outfile_edit, SIGNAL(textEdited(const QString &)), this,
          SLOT(enableOrDisableDefaultSave()));

  connect(m_uiForm.allowPeriods_ck, SIGNAL(stateChanged(int)), this,
          SLOT(disOrEnablePeriods(const int)));
}
/** Calls connect to fix up all the slots for the analysis details tab to their
 * events
 */
void SANSRunWindow::connectAnalysDetSignals() {
  // controls on the second page
  connect(m_uiForm.wav_dw_opt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleWavComboChange(int)));
  connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleStepComboChange(int)));
  connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleStepComboChange(int)));

  connect(m_uiForm.inst_opt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleInstrumentChange()));

  connect(m_uiForm.transFit_ck, SIGNAL(stateChanged(int)), this,
          SLOT(updateTransInfo(int)));
  connect(m_uiForm.transFit_ck_can, SIGNAL(stateChanged(int)), this,
          SLOT(updateTransInfo(int)));
  updateTransInfo(m_uiForm.transFit_ck->checkState());
  m_uiForm.transFit_ck_can->toggle();

  connect(m_uiForm.frontDetQrangeOnOff, SIGNAL(stateChanged(int)), this,
          SLOT(updateFrontDetQrange(int)));
  updateFrontDetQrange(m_uiForm.frontDetQrangeOnOff->checkState());

  connect(m_uiForm.mergeQRangeOnOff, SIGNAL(stateChanged(int)), this,
          SLOT(updateMergeQRange(int)));
  updateMergeQRange(m_uiForm.mergeQRangeOnOff->checkState());

  connect(m_uiForm.enableRearFlood_ck, SIGNAL(stateChanged(int)), this,
          SLOT(prepareFlood(int)));
  connect(m_uiForm.enableFrontFlood_ck, SIGNAL(stateChanged(int)), this,
          SLOT(prepareFlood(int)));

  connect(m_uiForm.trans_selector_opt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(transSelectorChanged(int)));
  transSelectorChanged(0);

  connect(m_uiForm.wavRanges, SIGNAL(editingFinished()), this,
          SLOT(checkList()));
}
/**
 * Initialize the widget maps
 */
void SANSRunWindow::initWidgetMaps() {
  //       batch mode settings
  m_allowed_batchtags.insert("sample_sans", 0);
  m_allowed_batchtags.insert("sample_trans", 1);
  m_allowed_batchtags.insert("sample_direct_beam", 2);
  m_allowed_batchtags.insert("can_sans", 3);
  m_allowed_batchtags.insert("can_trans", 4);
  m_allowed_batchtags.insert("can_direct_beam", 5);
  m_allowed_batchtags.insert("background_sans", -1);
  m_allowed_batchtags.insert("background_trans", -1);
  m_allowed_batchtags.insert("background_direct_beam", -1);
  m_allowed_batchtags.insert("output_as", 6);
  m_allowed_batchtags.insert("user_file", 7);
  //            detector info
  // SANS2D det names/label map
  QHash<QString, QLabel *> labelsmap;
  labelsmap.insert("Front_Det_Z", m_uiForm.dist_smp_frontZ);
  labelsmap.insert("Front_Det_X", m_uiForm.dist_smp_frontX);
  labelsmap.insert("Front_Det_Rot", m_uiForm.smp_rot);
  labelsmap.insert("Rear_Det_X", m_uiForm.dist_smp_rearX);
  labelsmap.insert("Rear_Det_Z", m_uiForm.dist_smp_rearZ);
  m_s2d_detlabels.append(labelsmap);

  labelsmap.clear();
  labelsmap.insert("Front_Det_Z", m_uiForm.dist_can_frontZ);
  labelsmap.insert("Front_Det_X", m_uiForm.dist_can_frontX);
  labelsmap.insert("Front_Det_Rot", m_uiForm.can_rot);
  labelsmap.insert("Rear_Det_X", m_uiForm.dist_can_rearX);
  labelsmap.insert("Rear_Det_Z", m_uiForm.dist_can_rearZ);
  m_s2d_detlabels.append(labelsmap);

  labelsmap.clear();
  labelsmap.insert("Front_Det_Z", m_uiForm.dist_bkgd_frontZ);
  labelsmap.insert("Front_Det_X", m_uiForm.dist_bkgd_frontX);
  labelsmap.insert("Front_Det_Rot", m_uiForm.bkgd_rot);
  labelsmap.insert("Rear_Det_X", m_uiForm.dist_bkgd_rearX);
  labelsmap.insert("Rear_Det_Z", m_uiForm.dist_bkgd_rearZ);
  m_s2d_detlabels.append(labelsmap);

  // LOQ labels
  labelsmap.clear();
  labelsmap.insert("moderator-sample", m_uiForm.dist_sample_ms);
  labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_smp_mdb);
  labelsmap.insert("sample-HAB", m_uiForm.dist_smp_hab);
  m_loq_detlabels.append(labelsmap);

  labelsmap.clear();
  labelsmap.insert("moderator-sample", m_uiForm.dist_can_ms);
  labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_can_mdb);
  labelsmap.insert("sample-HAB", m_uiForm.dist_can_hab);
  m_loq_detlabels.append(labelsmap);

  labelsmap.clear();
  labelsmap.insert("moderator-sample", m_uiForm.dist_bkgd_ms);
  labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_bkgd_mdb);
  labelsmap.insert("sample-HAB", m_uiForm.dist_bkgd_hab);
  m_loq_detlabels.append(labelsmap);

  // Full workspace names as they appear in the service
  m_workspaceNames.clear();
}

/**
 * Restore previous input
 */
void SANSRunWindow::readSettings() {
  g_log.debug("Reading settings.");
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");

  m_uiForm.userfile_edit->setText(value_store.value("user_file").toString());

  m_last_dir = value_store.value("last_dir", "").toString();

  int index = m_uiForm.inst_opt->findText(
      value_store.value("instrum", "LOQ").toString());
  // if the saved instrument no longer exists set index to zero
  index = index < 0 ? 0 : index;
  m_uiForm.inst_opt->setCurrentIndex(index);

  int mode_flag = value_store.value("runmode", 0).toInt();
  if (mode_flag == SANSRunWindow::SingleMode) {
    m_uiForm.single_mode_btn->click();
  } else {
    m_uiForm.batch_mode_btn->click();
  }

  // The instrument definition directory
  m_ins_defdir = QString::fromStdString(
      ConfigService::Instance().getString("instrumentDefinition.directory"));
  upDateDataDir();

  // Set allowed extensions
  m_uiForm.file_opt->clear();
  m_uiForm.file_opt->addItem("nexus", QVariant(".nxs"));
  m_uiForm.file_opt->addItem("raw", QVariant(".raw"));
  // Set old file extension
  m_uiForm.file_opt->setCurrentIndex(
      value_store.value("fileextension", 0).toInt());

  m_uiForm.allowPeriods_ck->setChecked(
      value_store.value("allow_periods", false).toBool());

  int i = m_uiForm.wav_dw_opt->findText(
      value_store.value("wave_binning", "Linear").toString());
  i = i > -1 ? i : 0;
  m_uiForm.wav_dw_opt->setCurrentIndex(i);
  // ensure this is called once even if the index hadn't changed
  handleWavComboChange(i);

  value_store.endGroup();
  readSaveSettings(value_store);

  g_log.debug() << "Found previous data directory "
                << "\nFound previous user mask file "
                << m_uiForm.userfile_edit->text().toStdString()
                << "\nFound instrument definition directory "
                << m_ins_defdir.toStdString() << '\n';
}
/** Sets the states of the checkboxes in the save box using those
 * in the passed QSettings object
 *  @param valueStore :: where the settings will be stored
 */
void SANSRunWindow::readSaveSettings(QSettings &valueStore) {
  valueStore.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  m_uiForm.saveCan_check->setChecked(
      valueStore.value("canSAS", false).toBool());
  m_uiForm.saveRKH_check->setChecked(valueStore.value("RKH", false).toBool());
  m_uiForm.saveNXcanSAS_check->setChecked(
      valueStore.value("NXcanSAS", false).toBool());
}

/**
 * Save input through QSettings (-> .mantidplot or -> windows registerary) for
 * future use
 */
void SANSRunWindow::saveSettings() {
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  if (!m_uiForm.userfile_edit->text().isEmpty()) {
    value_store.setValue("user_file", m_uiForm.userfile_edit->text());
  }

  value_store.setValue("last_dir", m_last_dir);

  value_store.setValue("instrum", m_uiForm.inst_opt->currentText());
  value_store.setValue("fileextension", m_uiForm.file_opt->currentIndex());
  value_store.setValue("allow_periods", m_uiForm.allowPeriods_ck->isChecked());

  value_store.setValue("wave_binning", m_uiForm.wav_dw_opt->currentText());

  unsigned int mode_id(0);
  if (m_uiForm.single_mode_btn->isChecked()) {
    mode_id = SANSRunWindow::SingleMode;
  } else {
    mode_id = SANSRunWindow::BatchMode;
  }
  value_store.setValue("runmode", mode_id);
  value_store.endGroup();
  saveSaveSettings(value_store);
}
/** Stores the state of the checkboxes in the save box with the
 * passed QSettings object
 *  @param valueStore :: where the settings will be stored
 */
void SANSRunWindow::saveSaveSettings(QSettings &valueStore) {
  valueStore.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  valueStore.setValue("canSAS", m_uiForm.saveCan_check->isChecked());
  valueStore.setValue("RKH", m_uiForm.saveRKH_check->isChecked());
  valueStore.setValue("NXcanSAS", m_uiForm.saveNXcanSAS_check->isChecked());
}
/**
 * Run a function from the SANS reduction script, ensuring that the first call
 * imports the module
 * @param pycode :: The code to execute
 * @returns A trimmed string containing the output of the code execution
 */
QString SANSRunWindow::runReduceScriptFunction(const QString &pycode) {
  if (!m_have_reducemodule) {
    return QString();
  }
  g_log.debug() << "Executing Python: " << pycode.toStdString() << '\n';

  const static QString PYTHON_SEP("C++runReduceScriptFunctionC++");
  QString code_torun = pycode + ";print('" + PYTHON_SEP + "')";
  QString pythonOut = runPythonCode(code_torun).trimmed();

  QStringList allOutput = pythonOut.split(PYTHON_SEP);

  if (allOutput.count() < 2) {
    QMessageBox::critical(this, "Fatal error found during reduction",
                          "Error reported by Python script, more information "
                          "maybe found in the scripting console and results "
                          "log");
    return "Error";
  }

  return allOutput[0].trimmed();
}

/**
 * Trim off Python markers surrounding things like strings or lists that have
 * been
 * printed by Python by removing the first and last character
 */
void SANSRunWindow::trimPyMarkers(QString &txt) {
  txt.remove(0, 1);
  txt.chop(1);
}
/** Issues a Python command to load the user file and returns any output if
 *  there are warnings or errors
 *  @return the output printed by the Python commands
 */
bool SANSRunWindow::loadUserFile() {
  // Check the user file
  if (!isValidUserFile()) {
    return false;
  }

  QString filetext = m_uiForm.userfile_edit->text().trimmed();
  // Clear the def masking info table.
  int mask_table_count = m_uiForm.mask_table->rowCount();
  for (int i = mask_table_count - 1; i >= 0; --i) {
    m_uiForm.mask_table->removeRow(i);
  }

  QString pyCode = "i.Clean()";
  pyCode += "\ni." + getInstrumentClass();
  pyCode += "\ni.ReductionSingleton().user_settings =";
  // Use python function to read the settings file and then extract the fields
  pyCode += "isis_reduction_steps.UserFile(r'" + filetext + "')";

  runReduceScriptFunction(pyCode);

  QString errors =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().user_settings.execute(i."
                              "ReductionSingleton()))")
          .trimmed();
  // create a string list with a string for each line
  const QStringList allOutput = errors.split("\n");
  errors.clear();
  bool canContinue = false;
  for (int i = 0; i < allOutput.count(); ++i) {
    if (i < allOutput.count() - 1) {
      errors += allOutput[i] + "\n";
    } else {
      canContinue = allOutput[i].trimmed() == "True";
    }
  }

  if (!canContinue) {
    m_cfg_loaded = false;
    return false;
  }

  const auto settings = getReductionSettings();

  const double unit_conv(1000.);
  // Radius
  double dbl_param =
      runReduceScriptFunction("print(i.ReductionSingleton().mask.min_radius)")
          .toDouble();
  m_uiForm.rad_min->setText(QString::number(dbl_param * unit_conv));
  dbl_param =
      runReduceScriptFunction("print(i.ReductionSingleton().mask.max_radius)")
          .toDouble();
  m_uiForm.rad_max->setText(QString::number(dbl_param * unit_conv));
  // EventsTime
  m_uiForm.l_events_binning->setText(
      getSettingWithDefault("events.binning", "").trimmed());
  // Wavelength
  m_uiForm.wav_min->setText(runReduceScriptFunction(
      "print(i.ReductionSingleton().to_wavelen.wav_low)"));
  m_uiForm.wav_max->setText(
      runReduceScriptFunction(
          "print(i.ReductionSingleton().to_wavelen.wav_high)")
          .trimmed());
  const QString wav_step =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().to_wavelen.wav_step)")
          .trimmed();
  setLimitStepParameter("wavelength", wav_step, m_uiForm.wav_dw,
                        m_uiForm.wav_dw_opt);
  // RCut WCut
  dbl_param =
      runReduceScriptFunction("print(i.ReductionSingleton().to_Q.r_cut)")
          .toDouble();
  m_uiForm.r_cut_line_edit->setText(QString::number(dbl_param * unit_conv));

  dbl_param =
      runReduceScriptFunction("print(i.ReductionSingleton().to_Q.w_cut)")
          .toDouble();
  m_uiForm.w_cut_line_edit->setText(QString::number(dbl_param));

  // Q
  QString text =
      runReduceScriptFunction("print(i.ReductionSingleton().to_Q.binning)");
  QStringList values = text.split(",");
  if (values.count() == 3) {
    m_uiForm.q_min->setText(values[0].trimmed());
    m_uiForm.q_max->setText(values[2].trimmed());
    setLimitStepParameter("Q", values[1].trimmed(), m_uiForm.q_dq,
                          m_uiForm.q_dq_opt);
  } else {
    m_uiForm.q_rebin->setText(text.trimmed());
    m_uiForm.q_dq_opt->setCurrentIndex(2);
  }

  // Qxy
  m_uiForm.qy_max->setText(
      runReduceScriptFunction("print(i.ReductionSingleton().QXY2)"));
  setLimitStepParameter(
      "Qxy", runReduceScriptFunction("print(i.ReductionSingleton().DQXY)"),
      m_uiForm.qy_dqy, m_uiForm.qy_dqy_opt);

  // The tramission line of the Limits section (read settings for sample and
  // can)
  loadTransmissionSettings();

  // The front rescale/shift section
  m_uiForm.frontDetRescale->setText(
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').rescaleAndShift.scale)")
          .trimmed());
  m_uiForm.frontDetShift->setText(
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').rescaleAndShift.shift)")
          .trimmed());

  QString fitScale =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').rescaleAndShift.fitScale)")
          .trimmed();
  QString fitShift =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').rescaleAndShift.fitShift)")
          .trimmed();

  if (fitScale == "True")
    m_uiForm.frontDetRescaleCB->setChecked(true);
  else
    m_uiForm.frontDetRescaleCB->setChecked(false);

  if (fitShift == "True")
    m_uiForm.frontDetShiftCB->setChecked(true);
  else
    m_uiForm.frontDetShiftCB->setChecked(false);

  QString qRangeUserSelected =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').rescaleAndShift.qRangeUserSelected)")
          .trimmed();
  if (qRangeUserSelected == "True") {
    m_uiForm.frontDetQrangeOnOff->setChecked(true);
    m_uiForm.frontDetQmin->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').rescaleAndShift.qMin)")
            .trimmed());
    m_uiForm.frontDetQmax->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').rescaleAndShift.qMax)")
            .trimmed());
  } else
    m_uiForm.frontDetQrangeOnOff->setChecked(false);

  QString qMergeRangeUserSelected =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().instrument.getDetector('"
                              "FRONT').mergeRange.q_merge_range)")
          .trimmed();
  if (qMergeRangeUserSelected == "True") {
    m_uiForm.mergeQRangeOnOff->setChecked(true);
    m_uiForm.mergeQMin->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').mergeRange.q_min)")
            .trimmed());
    m_uiForm.mergeQMax->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').mergeRange.q_max)")
            .trimmed());
  } else
    m_uiForm.mergeQRangeOnOff->setChecked(false);

  // Monitor spectra
  m_uiForm.monitor_spec->setText(
      runReduceScriptFunction(
          "print(i.ReductionSingleton().instrument.get_incident_mon())")
          .trimmed());
  m_uiForm.trans_monitor->setText(
      runReduceScriptFunction(
          "print(i.ReductionSingleton().instrument.incid_mon_4_trans_calc)")
          .trimmed());
  m_uiForm.monitor_interp->setChecked(
      runReduceScriptFunction(
          "print(i.ReductionSingleton().instrument.is_interpolating_norm())")
          .trimmed() == "True");
  m_uiForm.trans_interp->setChecked(
      runReduceScriptFunction(
          "print(i.ReductionSingleton().transmission_calculator.interpolate)")
          .trimmed() == "True");

  // Transmission settings
  setTransmissionSettingsFromUserFile();

  // Direct efficiency correction
  m_uiForm.direct_file->setText(runReduceScriptFunction(
      "print(i.ReductionSingleton().instrument.detector_file('rear'))"));
  m_uiForm.front_direct_file->setText(runReduceScriptFunction(
      "print(i.ReductionSingleton().instrument.detector_file('front'))"));

  QString file = runReduceScriptFunction(
      "print(i.ReductionSingleton().prep_normalize.getPixelCorrFile('REAR'))");
  file = file.trimmed();
  // Check if the file name is set to Python's None object and then adjust the
  // controls if there is an empty entry

  m_uiForm.floodRearFile->setFileTextWithSearch(file == "None" ? "" : file);
  m_uiForm.enableRearFlood_ck->setChecked(!m_uiForm.floodRearFile->isEmpty());
  m_uiForm.floodRearFile->setEnabled(
      m_uiForm.enableRearFlood_ck->checkState() == Qt::Checked);
  file = runReduceScriptFunction(
      "print(i.ReductionSingleton().prep_normalize.getPixelCorrFile('FRONT'))");
  file = file.trimmed();
  m_uiForm.floodFrontFile->setFileTextWithSearch(file == "None" ? "" : file);
  m_uiForm.enableFrontFlood_ck->setChecked(!m_uiForm.floodFrontFile->isEmpty());
  m_uiForm.floodFrontFile->setEnabled(
      m_uiForm.enableFrontFlood_ck->checkState() == Qt::Checked);

  // Scale factor
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton()._corr_and_scale.rescale)")
                  .toDouble();
  m_uiForm.scale_factor->setText(QString::number(dbl_param / 100.));

  // Sample offset if one has been specified
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().instrument.SAMPLE_Z_CORR)")
                  .toDouble();
  m_uiForm.smpl_offset->setText(QString::number(dbl_param * unit_conv));

  // Centre coordinates
  // Update the beam centre coordinates
  updateBeamCenterCoordinates();
  // Set the beam finder specific settings
  setBeamFinderDetails();
  // get the scale factor1 for the beam centre to scale it correctly
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('rear')[0])")
                  .toDouble();
  double dbl_paramsf =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().get_beam_center_scale_factor1())")
          .toDouble();
  m_uiForm.rear_beam_x->setText(QString::number(dbl_param * dbl_paramsf));
  // get scale factor2 for the beam centre to scale it correctly
  dbl_paramsf =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().get_beam_center_scale_factor2())")
          .toDouble();
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('rear')[1])")
                  .toDouble();
  m_uiForm.rear_beam_y->setText(QString::number(dbl_param * dbl_paramsf));
  // front
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('front')[0])")
                  .toDouble();
  m_uiForm.front_beam_x->setText(QString::number(dbl_param * 1000.0));
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('front')[1])")
                  .toDouble();
  m_uiForm.front_beam_y->setText(QString::number(dbl_param * 1000.0));
  // Gravity switch
  QString param = runReduceScriptFunction(
                      "print(i.ReductionSingleton().to_Q.get_gravity())")
                      .trimmed();
  if (param == "True") {
    m_uiForm.gravity_check->setChecked(true);
  } else {
    m_uiForm.gravity_check->setChecked(false);
  }

  // Read the extra length for the gravity correction
  const double extraLengthParam =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().to_Q.get_extra_length())")
          .toDouble();
  m_uiForm.gravity_extra_length_line_edit->setText(
      QString::number(extraLengthParam));

  ////Detector bank: support REAR, FRONT, HAB, BOTH, MERGED, MERGE options
  QString detName =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().instrument.det_selection)")
          .trimmed();

  if (detName == "REAR" || detName == "MAIN") {
    m_uiForm.detbank_sel->setCurrentIndex(0);
  } else if (detName == "FRONT" || detName == "HAB") {
    m_uiForm.detbank_sel->setCurrentIndex(1);
  } else if (detName == "BOTH") {
    m_uiForm.detbank_sel->setCurrentIndex(2);
  } else if (detName == "MERGED" || detName == "MERGE") {
    m_uiForm.detbank_sel->setCurrentIndex(3);
  }

  // Phi values
  m_uiForm.phi_min->setText(
      runReduceScriptFunction("print(i.ReductionSingleton().mask.phi_min)"));
  m_uiForm.phi_max->setText(
      runReduceScriptFunction("print(i.ReductionSingleton().mask.phi_max)"));

  // Masking table
  updateMaskTable();

  // Setup the QResolution
  retrieveQResolutionSettings();

  // Setup the BackgroundCorrection
  initializeBackgroundCorrection();
  retrieveBackgroundCorrection();

  if (runReduceScriptFunction("print(i.ReductionSingleton().mask.phi_mirror)")
          .trimmed() == "True") {
    m_uiForm.mirror_phi->setChecked(true);
  } else {
    m_uiForm.mirror_phi->setChecked(false);
  }

  if (!errors.isEmpty()) {
    showInformationBox("User file opened with some warnings:\n" + errors);
  }

  m_cfg_loaded = true;
  m_uiForm.userfileBtn->setText("Reload");
  m_uiForm.tabWidget->setTabEnabled(m_uiForm.tabWidget->count() - 1, true);

  m_cfg_loaded = true;
  emit userfileLoaded();
  m_uiForm.tabWidget->setTabEnabled(1, true);
  m_uiForm.tabWidget->setTabEnabled(2, true);
  m_uiForm.tabWidget->setTabEnabled(3, true);

  // Display which IDf is currently being used by the reducer
  updateIDFFilePath();

  return true;
}

/**
 * Load a CSV file specifying information run numbers and populate the batch
 * mode grid
 */
bool SANSRunWindow::loadCSVFile() {
  QString filename = m_uiForm.csv_filename->text();
  QFile csv_file(filename);
  if (!csv_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    showInformationBox("Error: Cannot open CSV file \"" + filename + "\"");
    return false;
  }

  // Clear the current table
  clearBatchTable();
  QTextStream file_in(&csv_file);
  int errors(0);
  while (!file_in.atEnd()) {
    QString line = file_in.readLine().simplified();
    if (!line.isEmpty()) {
      // if first line of batch contain string MANTID_BATCH_FILE this is a
      // 'metadata' line
      if (!line.toUpper().contains("MANTID_BATCH_FILE"))
        errors += addBatchLine(line, ",");
    }
  }
  if (errors > 0) {
    showInformationBox("Warning: " + QString::number(errors) +
                       " malformed lines detected in \"" + filename +
                       "\". Lines skipped.");
  }

  // In order to allow the user to populate the single mode Widgets from a csv
  // file,
  // this code takes the first line of a valid csv batch file and insert inside
  // the
  // single mode widgets. It is usefull for testing.
  QTableWidgetItem *batch_items[] = {
      m_uiForm.batch_table->item(0, 0), m_uiForm.batch_table->item(0, 1),
      m_uiForm.batch_table->item(0, 2), m_uiForm.batch_table->item(0, 3),
      m_uiForm.batch_table->item(0, 4), m_uiForm.batch_table->item(0, 5)};
  MWRunFiles *run_files[] = {m_uiForm.scatterSample, m_uiForm.transmis,
                             m_uiForm.direct,        m_uiForm.scatCan,
                             m_uiForm.transCan,      m_uiForm.dirCan};
  // if the cell is not empty, set the text to the single mode file
  for (unsigned short i = 0; i < 6; i++) {
    if (batch_items[i])
      run_files[i]->setUserInput(batch_items[i]->text());
    else
      run_files[i]->setUserInput("");
  }

  return true;
}

/**
 * Set a pair of an QLineEdit field and type QComboBox using the parameter given
 * @param pname :: The name of the parameter
 * @param param :: A string representing a value that maybe prefixed with a
 * minus to indicate a different step type
 * @param step_value :: The field to store the actual value
 * @param step_type :: The combo box with the type options
 */
void SANSRunWindow::setLimitStepParameter(const QString &pname, QString param,
                                          QLineEdit *step_value,
                                          QComboBox *step_type) {
  if (param.startsWith("-")) {
    int index = step_type->findText("Logarithmic");
    if (index < 0) {
      raiseOneTimeMessage(
          "Warning: Unable to find logarithmic scale option for " + pname +
              ", setting as linear.",
          1);
      index = step_type->findText("Linear");
    }
    step_type->setCurrentIndex(index);
    step_value->setText(param.remove(0, 1));
  } else {
    step_type->setCurrentIndex(step_type->findText("Linear"));
    step_value->setText(param);
  }
}

/**
 * Construct the mask table on the Mask tab
 */
void SANSRunWindow::updateMaskTable() {
  // Clear the current contents
  for (int i = m_uiForm.mask_table->rowCount() - 1; i >= 0; --i) {
    m_uiForm.mask_table->removeRow(i);
  }

  QString reardet_name("rear-detector"), frontdet_name("front-detector");
  if (m_uiForm.inst_opt->currentText() == "LOQ") {
    reardet_name = "main-detector-bank";
    frontdet_name = "HAB";
  }

  // First create 2 default mask cylinders at min and max radius for the beam
  // stop and
  // corners
  m_uiForm.mask_table->insertRow(0);
  m_uiForm.mask_table->setItem(0, 0, new QTableWidgetItem("beam stop"));
  m_uiForm.mask_table->setItem(0, 1, new QTableWidgetItem(reardet_name));
  m_uiForm.mask_table->setItem(
      0, 2, new QTableWidgetItem("infinite-cylinder, r = rmin"));
  if (m_uiForm.rad_max->text() != "-1") {
    m_uiForm.mask_table->insertRow(1);
    m_uiForm.mask_table->setItem(1, 0, new QTableWidgetItem("corners"));
    m_uiForm.mask_table->setItem(1, 1, new QTableWidgetItem(reardet_name));
    m_uiForm.mask_table->setItem(
        1, 2, new QTableWidgetItem("infinite-cylinder, r = rmax"));
  }

  // Now add information from the mask file
  // Spectrum mask, "Rear" det
  QString mask_string = runReduceScriptFunction(
      "print (i.ReductionSingleton().mask.spec_mask_r)");
  addSpectrumMasksToTable(mask_string, reardet_name);
  //"Front" det
  mask_string = runReduceScriptFunction(
      "print (i.ReductionSingleton().mask.spec_mask_f)");
  addSpectrumMasksToTable(mask_string, frontdet_name);

  // Time masks
  mask_string =
      runReduceScriptFunction("print (i.ReductionSingleton().mask.time_mask)");
  addTimeMasksToTable(mask_string, "-");
  // Rear detector
  mask_string =
      runReduceScriptFunction("print(i.ReductionSingleton().mask.time_mask_r)");
  addTimeMasksToTable(mask_string, reardet_name);
  // Front detectors
  mask_string =
      runReduceScriptFunction("print(i.ReductionSingleton().mask.time_mask_f)");
  addTimeMasksToTable(mask_string, frontdet_name);
  // Rear detectors for SANS2D if monitor 4 in place (arm shadow detector)
  mask_string =
      runReduceScriptFunction("print(i.ReductionSingleton().mask.time_mask_f)");
  addTimeMasksToTable(mask_string, frontdet_name);

  if (getInstrumentClass() == "SANS2D()") {
    QString arm_width =
        runReduceScriptFunction("print(i.ReductionSingleton().mask.arm_width)");
    QString arm_angle =
        runReduceScriptFunction("print(i.ReductionSingleton().mask.arm_angle)");
    QString arm_x =
        runReduceScriptFunction("print(i.ReductionSingleton().mask.arm_x)");
    QString arm_y =
        runReduceScriptFunction("print(i.ReductionSingleton().mask.arm_y)");
    if (arm_width != "None" && arm_angle != "None") {
      int row = m_uiForm.mask_table->rowCount();
      m_uiForm.mask_table->insertRow(row);
      m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem("Arm"));
      m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(reardet_name));
      if (arm_x != "None" && arm_y != "None")
        m_uiForm.mask_table->setItem(
            row, 2,
            new QTableWidgetItem("LINE " + arm_width + " " + arm_angle + " " +
                                 arm_x + " " + arm_y));
      else
        m_uiForm.mask_table->setItem(
            row, 2,
            new QTableWidgetItem("LINE " + arm_width + " " + arm_angle));
    }
  }

  auto settings = getReductionSettings();

  if (settings->existsProperty("MaskFiles")) {
    const auto maskFiles =
        QString::fromStdString(settings->getProperty("MaskFiles")).split(",");

    foreach (const auto &maskFile, maskFiles)
      appendRowToMaskTable("Mask File", "-", maskFile);
  }

  // add phi masking to table
  QString phiMin = m_uiForm.phi_min->text();
  QString phiMax = m_uiForm.phi_max->text();
  int row = m_uiForm.mask_table->rowCount();
  m_uiForm.mask_table->insertRow(row);
  m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem("Phi"));
  m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem("-"));
  if (m_uiForm.mirror_phi->isChecked()) {
    m_uiForm.mask_table->setItem(
        row, 2, new QTableWidgetItem("L/PHI " + phiMin + " " + phiMax));
  } else {
    m_uiForm.mask_table->setItem(
        row, 2,
        new QTableWidgetItem("L/PHI/NOMIRROR " + phiMin + " " + phiMax));
  }
}

/**
 * Add a spectrum mask string to the mask table
 * @param mask_string :: The string of mask information
 * @param det_name :: The detector it relates to
 */
void SANSRunWindow::addSpectrumMasksToTable(const QString &mask_string,
                                            const QString &det_name) {
  QStringList elements = mask_string.split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while (sitr.hasNext()) {
    QString item = sitr.next().trimmed();
    QString col1_txt;
    if (item.startsWith('s', Qt::CaseInsensitive)) {
      col1_txt = "Spectrum";
    } else if (item.startsWith('h', Qt::CaseInsensitive) ||
               item.startsWith('v', Qt::CaseInsensitive)) {
      if (item.contains('+')) {
        col1_txt = "Box";
      } else {
        col1_txt = "Strip";
      }
    } else
      continue;

    int row = m_uiForm.mask_table->rowCount();
    // Insert line after last row
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem(col1_txt));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(item));
  }
}

/**
 * Add a time mask string to the mask table
 * @param mask_string :: The string of mask information
 * @param det_name :: The detector it relates to
 */
void SANSRunWindow::addTimeMasksToTable(const QString &mask_string,
                                        const QString &det_name) {
  QStringList elements = mask_string.split(";", QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while (sitr.hasNext()) {
    int row = m_uiForm.mask_table->rowCount();
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem("time"));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    const QString shape(sitr.next().trimmed());
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(shape));
  }
}

/**
 * Append the given information as a new row to the masking table.
 *
 * @param type     :: the type of masking information
 * @param detector :: the detector bank this information applies to
 * @param details  :: the details of the mask
 */
void SANSRunWindow::appendRowToMaskTable(const QString &type,
                                         const QString &detector,
                                         const QString &details) {
  const int row = m_uiForm.mask_table->rowCount();

  m_uiForm.mask_table->insertRow(row);
  m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem(type));
  m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(detector));
  m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(details));
}

/**
 * Retrieve and set the component distances
 * @param workspace :: The workspace pointer
 * @param lms :: The result of the moderator-sample distance
 * @param lsda :: The result of the sample-detector bank 1 distance
 * @param lsdb :: The result of the sample-detector bank 2 distance
 */
void SANSRunWindow::componentLOQDistances(
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
    double &lms, double &lsda, double &lsdb) {
  Instrument_const_sptr instr = workspace->getInstrument();
  if (!instr)
    return;

  Mantid::Geometry::IComponent_const_sptr source = instr->getSource();
  if (source == boost::shared_ptr<Mantid::Geometry::IObjComponent>())
    return;
  Mantid::Geometry::IComponent_const_sptr sample = instr->getSample();
  if (sample == boost::shared_ptr<Mantid::Geometry::IObjComponent>())
    return;

  lms = source->getPos().distance(sample->getPos()) * 1000.;

  // Find the main detector bank
  Mantid::Geometry::IComponent_const_sptr comp =
      instr->getComponentByName("main-detector-bank");
  if (comp != boost::shared_ptr<Mantid::Geometry::IComponent>()) {
    lsda = sample->getPos().distance(comp->getPos()) * 1000.;
  }

  comp = instr->getComponentByName("HAB");
  if (comp != boost::shared_ptr<Mantid::Geometry::IComponent>()) {
    lsdb = sample->getPos().distance(comp->getPos()) * 1000.;
  }
}

/**
 * Set the state of processing.
 * @param action :: can be loading, 1D or 2D reduction
 */
void SANSRunWindow::setProcessingState(const States action) {
  const bool running(action == Loading || action == OneD || action == TwoD);

  // we only need a load button for single run mode and even then only when the
  // form isn't busy
  if (m_uiForm.single_mode_btn->isChecked()) {
    m_uiForm.load_dataBtn->setEnabled(!running);
  } else {
    m_uiForm.load_dataBtn->setEnabled(false);
  }

  // buttons that are available as long as Python is available
  m_uiForm.oneDBtn->setEnabled(!running);
  m_uiForm.twoDBtn->setEnabled(!running);
  m_uiForm.saveSel_btn->setEnabled(!running);
  m_uiForm.runcentreBtn->setEnabled(!running);
  m_uiForm.userfileBtn->setEnabled(!running);
  m_uiForm.data_dirBtn->setEnabled(!running);

  m_uiForm.oneDBtn->setText(action == OneD ? "Running ..." : "1D Reduce");
  m_uiForm.twoDBtn->setText(action == TwoD ? "Running ..." : "2D Reduce");

  if (running) {
    m_uiForm.saveDefault_btn->setEnabled(false);
  } else {
    enableOrDisableDefaultSave();
  }

  for (int i = 0; i < 4; ++i) {
    if (i == m_uiForm.tabWidget->currentIndex())
      continue;
    m_uiForm.tabWidget->setTabEnabled(i, !running);
  }

  QCoreApplication::processEvents();
}

/**
 * Does the workspace exist in the AnalysisDataService
 * @param ws_name :: The name of the workspace
 * @returns A boolean indicatingif the given workspace exists in the
 * AnalysisDataService
 */
bool SANSRunWindow::workspaceExists(const QString &ws_name) const {
  return AnalysisDataService::Instance().doesExist(ws_name.toStdString());
}

/**
 * @returns A list of the currently available workspaces
 */
QStringList SANSRunWindow::currentWorkspaceList() const {
  auto ws_list = AnalysisDataService::Instance().getObjectNames();
  auto iend = ws_list.end();
  QStringList current_list;
  for (auto itr = ws_list.begin(); itr != iend; ++itr) {
    current_list.append(QString::fromStdString(*itr));
  }
  return current_list;
}

/**
 * Is the user file loaded
 * @returns A boolean indicating whether the user file has been parsed in to the
 * details tab
 */
bool SANSRunWindow::isUserFileLoaded() const { return m_cfg_loaded; }

/**
 * Create the mask strings for spectra and times
 * @param exec_script Create userfile type execution script
 * @param importCommand This may e.g. be mask.parse_instruction
 * @param mType This parameter appears to take values PixelMask or TimeMask
 */
void SANSRunWindow::addUserMaskStrings(QString &exec_script,
                                       const QString &importCommand,
                                       enum MaskType mType) {
  // Clear current

  QString temp = importCommand + "('MASK/CLEAR')\n";
  exec_script += temp;
  temp = importCommand + "('MASK/CLEAR/TIME')\n";
  exec_script += temp;

  // Pull in the table details first, skipping the first two rows
  int nrows = m_uiForm.mask_table->rowCount();
  for (int row = 0; row < nrows; ++row) {
    if (m_uiForm.mask_table->item(row, 2)->text().startsWith("inf")) {
      continue;
    }
    if (m_uiForm.mask_table->item(row, 0)->text() == "Mask File") {
      continue;
    }
    if (mType == PixelMask) {
      if (m_uiForm.mask_table->item(row, 0)->text() == "time") {
        continue;
      }
    } else if (mType == TimeMask) {
      if (m_uiForm.mask_table->item(row, 0)->text() != "time") {
        continue;
      }
    }

    // 'special' case for phi masking since it uses the L command instead of the
    // MASK command
    if (m_uiForm.mask_table->item(row, 0)->text() == "Phi") {

      exec_script += importCommand + "('" +
                     m_uiForm.mask_table->item(row, 2)->text() + "')\n";
      continue;
    }

    temp = importCommand + "('MASK";
    exec_script += temp;
    QString type = m_uiForm.mask_table->item(row, 0)->text();
    if (type == "time") {
      exec_script += "/TIME";
    }
    QString details = m_uiForm.mask_table->item(row, 2)->text();
    QString detname = m_uiForm.mask_table->item(row, 1)->text().trimmed();
    if (detname == "-") {
      exec_script += " " + details;
    } else if (detname == "rear-detector" || detname == "main-detector-bank") {
      if (type != "Arm") {
        // whether it is front or rear bank is inferred from the spectrum number
        if (type == "Spectrum")
          exec_script += " " + details;
        else
          exec_script += "/REAR " + details;
      }
    } else {
      // whether it is front or rear bank is inferred from the spectrum number
      if (type == "Spectrum")
        exec_script += " " + details;
      else
        exec_script += "/FRONT " + details;
    }
    exec_script += "')\n";
  }

  // Spectra mask first
  QStringList mask_params =
      m_uiForm.user_spec_mask->text().split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(mask_params);
  QString bad_masks;
  while (sitr.hasNext()) {
    QString item = sitr.next().trimmed();
    if (item.startsWith("REAR", Qt::CaseInsensitive) ||
        item.startsWith("FRONT", Qt::CaseInsensitive)) {
      temp = importCommand + "('MASK/" + item + "')\n";
      exec_script += temp;
    } else if (item.startsWith('S', Qt::CaseInsensitive) ||
               item.startsWith('H', Qt::CaseInsensitive) ||
               item.startsWith('V', Qt::CaseInsensitive)) {
      temp = importCommand + " ('MASK " + item + "')\n";
      exec_script += temp;
    } else {
      bad_masks += item + ",";
    }
  }
  if (!bad_masks.isEmpty()) {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(
        QString("Warning: Could not parse the following spectrum masks: ") +
        bad_masks + ". Values skipped.");
  }

  // Time masks
  mask_params =
      m_uiForm.user_time_mask->text().split(",", QString::SkipEmptyParts);
  sitr = QStringListIterator(mask_params);
  bad_masks = "";
  while (sitr.hasNext()) {
    QString item = sitr.next().trimmed();
    if (item.startsWith("REAR", Qt::CaseInsensitive) ||
        item.startsWith("FRONT", Qt::CaseInsensitive)) {
      int ndetails = item.split(" ").count();
      if (ndetails == 3 || ndetails == 2) {
        temp = importCommand + "('/TIME" + item + "')\n";
        exec_script += temp;

      } else {
        bad_masks += item + ",";
      }
    }
  }

  if (!bad_masks.isEmpty()) {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(
        QString("Warning: Could not parse the following time masks: ") +
        bad_masks + ". Values skipped.");
  }
}
/** This method applys mask to a given workspace
 * @param wsName name of the workspace
 * @param time_pixel  true if time mask needs to be applied
 */
void SANSRunWindow::applyMask(const QString &wsName, bool time_pixel) {
  QString script = "mask= isis_reduction_steps.Mask_ISIS()\n";
  QString str;
  if (time_pixel) {
    addUserMaskStrings(str, "mask.parse_instruction", TimeMask);
  } else {
    addUserMaskStrings(str, "mask.parse_instruction", PixelMask);
  }

  script += str;
  script += "mask.execute(i.ReductionSingleton(),\"";
  script += wsName;
  script += "\"";
  script += ",xcentre=0,ycentre=0)";
  runPythonCode(script.trimmed());
}
/**
 * Set the information about component distances on the geometry tab
 */
void SANSRunWindow::setGeometryDetails() {
  resetGeometryDetailsBox();

  const std::string wsName = m_experWksp.toStdString();
  if (wsName.empty())
    return;

  const auto &ADS = AnalysisDataService::Instance();

  assert(ADS.doesExist(wsName));
  auto ws = ADS.retrieveWS<const Workspace>(wsName);

  if (boost::dynamic_pointer_cast<const WorkspaceGroup>(ws))
    // Assume all geometry information is in the first member of the group and
    // it is
    // constant for all group members.
    ws = getGroupMember(ws, 1);

  MatrixWorkspace_const_sptr monitorWs;

  if (boost::dynamic_pointer_cast<const IEventWorkspace>(ws)) {
    // EventWorkspaces have their monitors loaded into a separate workspace.
    const std::string monitorWsName = ws->getName() + "_monitors";

    if (!ADS.doesExist(monitorWsName)) {
      g_log.error() << "Expected a sister monitor workspace called \""
                    << monitorWsName << "\" "
                    << "for the EventWorkspace \"" << ws->getName()
                    << "\", but could not find one "
                    << "so unable to set geometry details.\n";
      return;
    }

    monitorWs = ADS.retrieveWS<const MatrixWorkspace>(monitorWsName);
  } else {
    // MatrixWorkspaces have their monitors loaded in the same workspace.
    monitorWs = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
    assert(monitorWs);
  }

  const auto sampleWs = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);

  // Moderator-monitor distance is common to LOQ and SANS2D.
  size_t monitorWsIndex = 0;
  const specnum_t monitorSpectrum = m_uiForm.monitor_spec->text().toInt();
  try {
    monitorWsIndex = monitorWs->getIndexFromSpectrumNumber(monitorSpectrum);
  } catch (std::runtime_error &) {
    g_log.error() << "The reported incident monitor spectrum number \""
                  << monitorSpectrum
                  << "\" does not have a corresponding workspace index in \""
                  << monitorWs->getName()
                  << "\", so unable to set geometry details.\n";
    return;
  }

  const auto &monitorDetectorIDs =
      monitorWs->getSpectrum(monitorWsIndex).getDetectorIDs();
  if (monitorDetectorIDs.empty())
    return;

  double dist_mm(0.0);
  QString colour("black");

  const auto &detectorInfo = sampleWs->detectorInfo();

  try {
    const auto &detector = detectorInfo.detector(
        detectorInfo.indexOf(*monitorDetectorIDs.begin()));
    const double unit_conv(1000.);
    const auto &source = sampleWs->getInstrument()->getSource();
    dist_mm = detector.getDistance(*source) * unit_conv;
  } catch (std::runtime_error &) {
    colour = "red";
  }

  if (m_uiForm.inst_opt->currentText() == "LOQ") {
    if (colour == "red") {
      m_uiForm.dist_mod_mon->setText("<font color='red'>error<font>");
    } else {
      m_uiForm.dist_mod_mon->setText(formatDouble(dist_mm, colour));
    }
    setLOQGeometry(sampleWs, 0);
    QString can = m_experCan;
    if (!can.isEmpty()) {
      Workspace_sptr workspace_ptr =
          Mantid::API::AnalysisDataService::Instance().retrieve(
              can.toStdString());
      MatrixWorkspace_sptr can_workspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);

      if (!can_workspace) { // assume all geometry information is in the first
                            // member of the group and it is constant for all
                            // group members
        // function throws if a fisrt member can't be retrieved
        can_workspace = getGroupMember(workspace_ptr, 1);
      }
      setLOQGeometry(can_workspace, 1);
    }
  } else if (m_uiForm.inst_opt->currentText() == "SANS2D" ||
             m_uiForm.inst_opt->currentText() == "SANS2DTUBES") {
    if (colour == "red") {
      m_uiForm.dist_mon_s2d->setText("<font color='red'>error<font>");
    } else {
      m_uiForm.dist_mon_s2d->setText(formatDouble(dist_mm, colour));
    }

    // SANS2D - Sample
    setSANS2DGeometry(sampleWs, 0);
    // Get the can workspace if there is one
    QString can = m_experCan;
    if (can.isEmpty()) {
      return;
    }
    Workspace_sptr workspace_ptr;
    try {
      workspace_ptr =
          AnalysisDataService::Instance().retrieve(can.toStdString());
    } catch (std::runtime_error &) {
      return;
    }

    Mantid::API::MatrixWorkspace_sptr can_workspace =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            workspace_ptr);
    if (!can_workspace) { // assume all geometry information is in the first
                          // member of the group and it is constant for all
                          // group members
      // function throws if a fisrt member can't be retrieved
      can_workspace = getGroupMember(workspace_ptr, 1);
    }

    setSANS2DGeometry(can_workspace, 1);

    // Check for discrepancies
    bool warn_user(false);
    double lms_sample(m_uiForm.dist_sample_ms_s2d->text().toDouble()),
        lms_can(m_uiForm.dist_can_ms_s2d->text().toDouble());
    if (std::fabs(lms_sample - lms_can) > 5e-03) {
      warn_user = true;
      markError(m_uiForm.dist_sample_ms_s2d);
      markError(m_uiForm.dist_can_ms_s2d);
    }

    QString marked_dets =
        runReduceScriptFunction("print(i.GetMismatchedDetList()),").trimmed();
    trimPyMarkers(marked_dets);
    if (!marked_dets.isEmpty()) {
      QStringList detnames = marked_dets.split(",");
      QStringListIterator itr(detnames);
      while (itr.hasNext()) {
        QString name = itr.next().trimmed();
        trimPyMarkers(name);
        for (int i = 0; i < 2; ++i) {
          markError(m_s2d_detlabels[i].value(name));
          warn_user = true;
        }
      }
    }
    if (warn_user) {
      raiseOneTimeMessage("Warning: Some detector distances do not match for "
                          "the assigned Sample/Can runs, see Geometry tab for "
                          "details.");
    }
  }
}

/**
 * Set SANS2D geometry info
 * @param workspace :: The workspace
 * @param wscode :: 0 for sample, 1 for can, others not defined
 */
void SANSRunWindow::setSANS2DGeometry(
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
    int wscode) {
  const double unitconv = 1000.;
  const double distance = workspace->spectrumInfo().l1() * unitconv;

  // Moderator-sample
  QLabel *dist_label(nullptr);
  if (wscode == 0) {
    dist_label = m_uiForm.dist_sample_ms_s2d;
  } else if (wscode == 1) {
    dist_label = m_uiForm.dist_can_ms_s2d;
  } else {
    dist_label = m_uiForm.dist_bkgd_ms_s2d;
  }
  dist_label->setText(formatDouble(distance, "black", 'f', 1));

  // get the tuple of log values and convert to a list of
  QString code_to_run =
      QString("print(','.join([str(a) for a in "
              "i.ReductionSingleton().instrument.getDetValues('%1')]))")
          .arg(QString::fromStdString(workspace->getName()));

  QStringList logvalues = runReduceScriptFunction(code_to_run).split(",");

  QStringList dets_names;
  dets_names << "Front_Det_Z"
             << "Front_Det_X"
             << "Front_Det_Rot"
             << "Rear_Det_Z"
             << "Rear_Det_X";
  int index = 0;
  foreach (QString detname, dets_names) {
    QString distance = logvalues[index];
    try {
      double d = distance.toDouble();
      distance = QString::number(d, 'f', 1);
    } catch (...) {
      // if distance is not a double, for now just proceed
    }
    QLabel *lbl = m_s2d_detlabels[wscode].value(detname);
    if (lbl)
      lbl->setText(distance);
    index += 1;
  }
}

/**
 * Set LOQ geometry information
 * @param workspace :: The workspace to operate on
 * @param wscode :: ?????
 */
void SANSRunWindow::setLOQGeometry(
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
    int wscode) {
  double dist_ms(0.0), dist_mdb(0.0), dist_hab(0.0);
  // Sample
  componentLOQDistances(workspace, dist_ms, dist_mdb, dist_hab);

  QHash<QString, QLabel *> &labels = m_loq_detlabels[wscode];
  QLabel *detlabel = labels.value("moderator-sample");
  if (detlabel) {
    detlabel->setText(QString::number(dist_ms));
  }

  detlabel = labels.value("sample-main-detector-bank");
  if (detlabel) {
    detlabel->setText(QString::number(dist_mdb));
  }

  detlabel = labels.value("sample-HAB");
  if (detlabel) {
    detlabel->setText(QString::number(dist_hab));
  }
}

/**
 * Mark an error on a label
 * @param label :: A pointer to a QLabel instance
 */
void SANSRunWindow::markError(QLabel *label) {
  if (label) {
    label->setText("<font color=\"red\">" + label->text() + "</font>");
  }
}

//-------------------------------------
// Private SLOTS
//------------------------------------
/**
 * Select the base directory for the data
 */
void SANSRunWindow::selectDataDir() {
  MantidQt::API::ManageUserDirectories::openUserDirsDialog(this);
}

/**
 * Select and load the user file
 */
void SANSRunWindow::selectUserFile() {
  if (!browseForFile("Select a user file", m_uiForm.userfile_edit,
                     "Text files (*.txt)")) {
    return;
  }
  // possibly redudent code now
  runReduceScriptFunction("i.ReductionSingleton().user_file_path='" +
                          QFileInfo(m_uiForm.userfile_edit->text()).path() +
                          "'");

  if (!loadUserFile()) { // the load was successful
    return;
  }

  // path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.userfile_edit->text()).path();
}

/**
 * Select and load a CSV file
 */
void SANSRunWindow::selectCSVFile() {
  if (!m_cfg_loaded) {
    showInformationBox("Please load the relevant user file.");
    return;
  }

  if (!browseForFile("Select CSV file", m_uiForm.csv_filename,
                     "CSV files (*.csv)")) {
    return;
  }

  if (!loadCSVFile()) {
    return;
  }
  // path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.csv_filename->text()).path();
  if (m_cfg_loaded)
    setProcessingState(Ready);
}
/** Raises a browse dialog and inserts the selected file into the
 *  save text edit box, outfile_edit
 */
void SANSRunWindow::saveFileBrowse() {
  QString title = "Save output workspace as";

  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  // use their previous directory first and go to their default if that fails
  QString prevPath =
      prevValues
          .value("dir",
                 QString::fromStdString(ConfigService::Instance().getString(
                     "defaultsave.directory")))
          .toString();

  const QString filter = ";;AllFiles (*)";

  QString oFile = QFileDialog::getSaveFileName(
      this, title, prevPath + "/" + m_uiForm.outfile_edit->text());

  if (!oFile.isEmpty()) {
    m_uiForm.outfile_edit->setText(oFile);

    QString directory = QFileInfo(oFile).path();
    prevValues.setValue("dir", directory);
  }
}
/**
 * Flip the flag to confirm whether data is reloaded
 * @param force :: If true, the data is reloaded when reduce is clicked
 */
void SANSRunWindow::forceDataReload(bool force) { m_force_reload = force; }

/**
 * Browse for a file and set the text of the given edit box
 * @param box_title :: The title field for the display box
 * @param file_field :: QLineEdit box to use for the file path
 * @param file_filter :: An optional file filter
 */
bool SANSRunWindow::browseForFile(const QString &box_title,
                                  QLineEdit *file_field, QString file_filter) {
  QString box_text = file_field->text();
  QString start_path = box_text;
  if (box_text.isEmpty()) {
    start_path = m_last_dir;
  }
  file_filter += ";;AllFiles (*)";
  QString file_path =
      QFileDialog::getOpenFileName(this, box_title, start_path, file_filter);
  if (file_path.isEmpty() || QFileInfo(file_path).isDir())
    return false;
  file_field->setText(file_path);
  return true;
}
/**
 * Receive a load button click signal
 */
bool SANSRunWindow::handleLoadButtonClick() {
  // this function looks for and reports any errors to the user
  if (!entriesAreValid(LOAD)) {
    return false;
  }

  // Check if we have loaded the data_file
  if (!isUserFileLoaded()) {
    showInformationBox("Please load the relevant user file.");
    return false;
  }

  setProcessingState(Loading);
  m_uiForm.load_dataBtn->setText("Loading ...");

  if (m_force_reload)
    cleanup();

  bool is_loaded(true);
  if ((!m_uiForm.transmis->isEmpty()) && m_uiForm.direct->isEmpty()) {
    showInformationBox(
        "Error: Can run supplied without direct run, cannot continue.");
    setProcessingState(NoSample);
    m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }

  QString error;
  // set the detector just before loading so to correctly move the instrument
  runReduceScriptFunction("\ni.ReductionSingleton().instrument.setDetector('" +
                          m_uiForm.detbank_sel->currentText() + "')");
  QString sample = m_uiForm.scatterSample->getFirstFilename();
  try { // preliminarly error checking is over try to load that data
    is_loaded &= assignDetBankRun(*(m_uiForm.scatterSample), "AssignSample");
    readNumberOfEntries("get_sample().loader", m_uiForm.scatterSample);
    if (m_uiForm.scatCan->isEmpty()) {
      m_experCan = "";
    } else {
      is_loaded &= assignDetBankRun(*(m_uiForm.scatCan), "AssignCan");
      readNumberOfEntries("get_can().loader", m_uiForm.scatCan);
    }
    if ((!m_uiForm.transmis->isEmpty()) && (!m_uiForm.direct->isEmpty())) {
      is_loaded &= assignMonitorRun(*(m_uiForm.transmis), *(m_uiForm.direct),
                                    "TransmissionSample");
      readNumberOfEntries("samp_trans_load.trans", m_uiForm.transmis);
      readNumberOfEntries("samp_trans_load.direct", m_uiForm.direct);
    }

    // Quick check that there is a can direct run if a trans can is defined. If
    // not use the sample one
    if ((!m_uiForm.transCan->isEmpty()) && m_uiForm.dirCan->isEmpty()) {
      m_uiForm.dirCan->setFileTextWithSearch(m_uiForm.direct->getText());
      m_uiForm.dirCan->setEntryNum(m_uiForm.direct->getEntryNum());
    }
    if ((!m_uiForm.transCan->isEmpty()) && (!m_uiForm.dirCan->isEmpty())) {
      is_loaded &= assignMonitorRun(*(m_uiForm.transCan), *(m_uiForm.dirCan),
                                    "TransmissionCan");
      readNumberOfEntries("can_trans_load.trans", m_uiForm.transCan);
      readNumberOfEntries("can_trans_load.direct", m_uiForm.dirCan);
    }
  } catch (std::runtime_error &) { // the user should already have seen an error
                                   // message box pop up
    g_log.error() << "Problem loading file\n";
    is_loaded = false;
  }
  if (!is_loaded) {
    setProcessingState(NoSample);
    m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }

  // Sort out the log information
  setGeometryDetails();

  Mantid::API::Workspace_sptr baseWS =
      Mantid::API::AnalysisDataService::Instance().retrieve(
          m_experWksp.toStdString());
  // Enter information from sample workspace on to analysis and geometry tab
  Mantid::API::MatrixWorkspace_sptr sample_workspace =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(baseWS);

  if (sample_workspace && (!sample_workspace->x(0).empty())) {
    m_uiForm.tof_min->setText(QString::number(sample_workspace->x(0).front()));
    m_uiForm.tof_max->setText(QString::number(sample_workspace->x(0).back()));
  }

  // Set the geometry if the sample has been changed
  if (m_sample_file != sample) {
    const auto sample = sample_workspace->sample();
    const int geomId = sample.getGeometryFlag();

    if (geomId > 0 && geomId < 4) {
      m_uiForm.sample_geomid->setCurrentIndex(geomId - 1);

      using namespace boost;
      using GeomSampleInfo =
          tuple<QLineEdit *, function<double(const Sample *)>, std::string>;

      std::vector<GeomSampleInfo> sampleInfoList;
      sampleInfoList.push_back(make_tuple(m_uiForm.sample_thick,
                                          &Sample::getThickness, "thickness"));
      sampleInfoList.push_back(
          make_tuple(m_uiForm.sample_width, &Sample::getWidth, "width"));
      sampleInfoList.push_back(
          make_tuple(m_uiForm.sample_height, &Sample::getHeight, "height"));

      // Populate the sample geometry fields, but replace any zero values with
      // 1.0, and
      // warn the user where this has occured.
      for (auto info : sampleInfoList) {
        const auto value = info.get<1>()(&sample);
        if (value == 0.0)
          g_log.warning("The sample geometry " + info.get<2>() +
                        " was found to be zero, so using a default value of "
                        "1.0 instead.");

        info.get<0>()->setText(QString::number(value == 0.0 ? 1.0 : value));
      }
    } else {
      m_uiForm.sample_geomid->setCurrentIndex(2);
      m_uiForm.sample_thick->setText("1");
      m_uiForm.sample_width->setText("8");
      m_uiForm.sample_height->setText("8");
      // Warn user
      showInformationBox("Warning: Incorrect geometry flag encountered: " +
                         QString::number(geomId) + ". Using default values.");
    }
  }

  forceDataReload(false);

  for (int index = 1; index < m_uiForm.tabWidget->count(); ++index) {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }

  m_sample_file = sample;
  setProcessingState(Ready);
  m_uiForm.load_dataBtn->setText("Load Data");

  // Update the beam center position
  updateBeamCenterCoordinates();
  // Set the beam finder specific settings
  setBeamFinderDetails();

  // Display which IDF is currently being used by the reducer
  updateIDFFilePath();

  return true;
}

/** Queries the number of periods from the Python object whose name was passed
 *  @param RunStep name of the RunStep Python object
 *  @param output where the number will be displayed
 */
void SANSRunWindow::readNumberOfEntries(const QString &RunStep,
                                        API::MWRunFiles *const output) {
  QString periods = runReduceScriptFunction("print(i.ReductionSingleton()." +
                                            RunStep + ".periods_in_file)");
  output->setNumberOfEntries(periods.toInt());
}
/** Construct the python code to perform the analysis using the
 * current settings
 * @param type :: The reduction type: 1D or 2D
 */
QString SANSRunWindow::readUserFileGUIChanges(const States type) {
  const bool invalidRearFlood = m_uiForm.enableRearFlood_ck->isChecked() &&
                                !m_uiForm.floodRearFile->isValid();
  const bool invalidFrontFlood = m_uiForm.enableFrontFlood_ck->isChecked() &&
                                 !m_uiForm.floodFrontFile->isValid();

  if (invalidRearFlood || invalidFrontFlood)
    throw std::runtime_error("Invalid flood file(s). Check the path shown in "
                             "the \"Reduction Settings\" tab.");

  // Construct a run script based upon the current values within the various
  // widgets
  QString exec_reduce;
  if (m_uiForm.detbank_sel->currentIndex() < 2)
    exec_reduce = "i.ReductionSingleton().instrument.setDetector('" +
                  m_uiForm.detbank_sel->currentText() + "')\n";
  else
    // currently, if currentIndex has MAIN,HAB,BOTH,MERGED options. If the user
    // selects BOTH or MERGED the reduction will start by the DefaultDetector
    // that is the low-angle detector(MAIN). This is important, because, when
    // loading
    // the data, the reducer needs to know what is the bank detector selected in
    // order
    // to correctly answer the question: get_beam_center. Added for #5942
    exec_reduce = "i.ReductionSingleton().instrument.setDefaultDetector()\n";

  const QString outType(type == OneD ? "1D" : "2D");
  exec_reduce += "i.ReductionSingleton().to_Q.output_type='" + outType + "'\n";
  // Analysis details
  exec_reduce +=
      "i.ReductionSingleton().user_settings.readLimitValues('L/R '+'" +
      // get rid of the 1 in the line below, a character is need at the moment
      // to give the correct number of characters
      m_uiForm.rad_min->text() + " '+'" + m_uiForm.rad_max->text() +
      " '+'1', i.ReductionSingleton())\n";

  exec_reduce +=
      "i.ReductionSingleton().user_settings.readLimitValues('L/Q/RCut '+'" +
      m_uiForm.r_cut_line_edit->text() + "', i.ReductionSingleton())\n";

  exec_reduce +=
      "i.ReductionSingleton().user_settings.readLimitValues('L/Q/WCut '+'" +
      m_uiForm.w_cut_line_edit->text() + "', i.ReductionSingleton())\n";

  setStringSetting("events.binning", m_uiForm.l_events_binning->text());

  QString logLin = m_uiForm.wav_dw_opt->currentText().toUpper();
  if (logLin.contains("LOG")) {
    logLin = "LOG";
  }
  if (logLin.contains("LIN")) {
    logLin = "LIN";
  }
  exec_reduce += "i.LimitsWav(" + m_uiForm.wav_min->text().trimmed() + "," +
                 m_uiForm.wav_max->text() + "," + m_uiForm.wav_dw->text() +
                 ",'" + logLin + "')\n";

  if (m_uiForm.q_dq_opt->currentIndex() == 2) {
    exec_reduce +=
        "i.ReductionSingleton().user_settings.readLimitValues('L/Q " +
        m_uiForm.q_rebin->text() + "', i.ReductionSingleton())\n";
  } else {
    exec_reduce +=
        "i.ReductionSingleton().user_settings.readLimitValues('L/Q " +
        m_uiForm.q_min->text() + " " + m_uiForm.q_max->text() + " " +
        m_uiForm.q_dq->text() + "/" +
        m_uiForm.q_dq_opt->itemData(m_uiForm.q_dq_opt->currentIndex())
            .toString() +
        "', i.ReductionSingleton())\n";
  }
  exec_reduce +=
      "i.LimitsQXY(0.0," + m_uiForm.qy_max->text().trimmed() + "," +
      m_uiForm.qy_dqy->text().trimmed() + ",'" +
      m_uiForm.qy_dqy_opt->itemData(m_uiForm.qy_dqy_opt->currentIndex())
          .toString() +
      "')\n";
  exec_reduce += "i.SetPhiLimit(" + m_uiForm.phi_min->text().trimmed() + "," +
                 m_uiForm.phi_max->text().trimmed();
  if (m_uiForm.mirror_phi->isChecked()) {
    exec_reduce += ", True";
  } else {
    exec_reduce += ", False";
  }
  exec_reduce += ")\n";

  QString floodRearFile =
      m_uiForm.enableRearFlood_ck->isChecked()
          ? m_uiForm.floodRearFile->getFirstFilename().trimmed()
          : "";
  QString floodFrontFile =
      m_uiForm.enableFrontFlood_ck->isChecked()
          ? m_uiForm.floodFrontFile->getFirstFilename().trimmed()
          : "";
  exec_reduce += "i.SetDetectorFloodFile('" + floodRearFile + "','REAR')\n";
  exec_reduce += "i.SetDetectorFloodFile('" + floodFrontFile + "','FRONT')\n";

  // Set the wavelength ranges, equal to those for the sample unless this box is
  // checked
  // Also check if the Trans Fit on/off tick is on or off. If Off then set the
  // trans_opt to off
  {
    QCheckBox *fit_ck;
    QCheckBox *use_ck;
    QComboBox *method_opt;
    QLineEdit *_min;
    QLineEdit *_max;
    QString selector = "BOTH";
    // if trans_selector_opt == BOTH (index 0) it executes only once.
    // if trans_selector_opt == SAMPLE (index 1) it executes twice.
    for (int i = 0; i < m_uiForm.trans_selector_opt->currentIndex() + 1; i++) {
      if (i == 0) {
        fit_ck = m_uiForm.transFitOnOff;
        use_ck = m_uiForm.transFit_ck;
        method_opt = m_uiForm.trans_opt;
        _min = m_uiForm.trans_min;
        _max = m_uiForm.trans_max;
        if (m_uiForm.trans_selector_opt->currentIndex() == 1)
          selector = "SAMPLE";
      } else {
        fit_ck = m_uiForm.transFitOnOff_can;
        use_ck = m_uiForm.transFit_ck_can;
        method_opt = m_uiForm.trans_opt_can;
        _min = m_uiForm.trans_min_can;
        _max = m_uiForm.trans_max_can;
        selector = "CAN";
      }

      QString lambda_min_option = "lambdamin=None";
      QString lambda_max_option = "lambdamax=None";
      QString mode_option; // = "mode='OFF'";
      QString selector_option = "selector='" + selector + "'";

      if (!fit_ck->isChecked())
        mode_option = "mode='Off'";
      else {
        mode_option = "mode='" + method_opt->currentText() + "'";
        if (use_ck->isChecked()) {
          lambda_min_option = "lambdamin='" + _min->text().trimmed() + "'";
          lambda_max_option = "lambdamax='" + _max->text().trimmed() + "'";
        }
      }
      exec_reduce += "i.TransFit(" + mode_option + ", " + lambda_min_option +
                     ", " + lambda_max_option + ", " + selector_option + ")\n";
    }
  }
  // Set the Front detector Rescale and Shift
  QString fdArguments = "scale=" + m_uiForm.frontDetRescale->text().trimmed() +
                        "," +
                        "shift=" + m_uiForm.frontDetShift->text().trimmed();
  if (m_uiForm.frontDetRescaleCB->isChecked())
    fdArguments += ", fitScale=True";
  if (m_uiForm.frontDetShiftCB->isChecked())
    fdArguments += ", fitShift=True";
  if (m_uiForm.frontDetQrangeOnOff->isChecked() &&
      !m_uiForm.frontDetQmin->text().isEmpty() &&
      !m_uiForm.frontDetQmax->text().isEmpty()) {
    fdArguments += ", qMin=" + m_uiForm.frontDetQmin->text().trimmed();
    fdArguments += ", qMax=" + m_uiForm.frontDetQmax->text().trimmed();
  }

  exec_reduce += "i.SetFrontDetRescaleShift(" + fdArguments + ")\n";

  // Set the merge q range
  QString mergeArguments = "";
  if (m_uiForm.mergeQRangeOnOff->isChecked() &&
      !m_uiForm.mergeQMin->text().isEmpty() &&
      !m_uiForm.mergeQMax->text().isEmpty()) {
    mergeArguments += "q_min=" + m_uiForm.mergeQMin->text().trimmed();
    mergeArguments += ", q_max=" + m_uiForm.mergeQMax->text().trimmed();
  }

  exec_reduce += "i.SetMergeQRange(" + mergeArguments + ")\n";

  // Gravity correction
  exec_reduce += "i.Gravity(";
  if (m_uiForm.gravity_check->isChecked()) {
    exec_reduce += "True";
  } else {
    exec_reduce += "False";
  }
  // Take into acount of the additional length
  exec_reduce += ", extra_length=" +
                 m_uiForm.gravity_extra_length_line_edit->text().trimmed() +
                 ")\n";

  // Sample offset
  exec_reduce += "i.SetSampleOffset('" + m_uiForm.smpl_offset->text() + "')\n";

  // Monitor spectrum
  exec_reduce +=
      "i.SetMonitorSpectrum('" + m_uiForm.monitor_spec->text().trimmed() + "',";
  exec_reduce += m_uiForm.monitor_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";
  // the monitor to normalise the tranmission spectrum against
  exec_reduce +=
      "i.SetTransSpectrum('" + m_uiForm.trans_monitor->text().trimmed() + "',";
  exec_reduce += m_uiForm.trans_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";

  // Set the Transmision settings
  writeTransmissionSettingsToPythonScript(exec_reduce);

  // Set the QResolution settings
  writeQResolutionSettingsToPythonScript(exec_reduce);

  // Set the BackgroundCorrection settings
  writeBackgroundCorrectionToPythonScript(exec_reduce);

  // set the user defined center (Geometry Tab)
  // this information is used just after loading the data in order to move to
  // the center
  // Introduced for #5942
  QString set_centre =
      QString(
          "i.SetCentre('%1','%2','rear') \ni.SetCentre('%3','%4','front')\n")
          .arg(m_uiForm.rear_beam_x->text())
          .arg(m_uiForm.rear_beam_y->text())
          .arg(m_uiForm.front_beam_x->text())
          .arg(m_uiForm.front_beam_y->text());
  exec_reduce += set_centre;

  // mask strings that the user has entered manually on to the GUI
  addUserMaskStrings(exec_reduce, "i.Mask", DefaultMask);

  // add slicing definition
  if (!m_uiForm.sliceEvent->isHidden())
    exec_reduce +=
        "i.SetEventSlices('" + m_uiForm.sliceEvent->text().trimmed() + "')\n";

  return exec_reduce;
}
/// Reads the sample geometry, these settings will override what is stored in
/// the run file
QString SANSRunWindow::readSampleObjectGUIChanges() {
  QString exec_reduce(
      "\ni.ReductionSingleton().get_sample().geometry.shape = ");
  exec_reduce += m_uiForm.sample_geomid->currentText().at(0);

  exec_reduce += "\ni.ReductionSingleton().get_sample().geometry.height = ";
  exec_reduce += m_uiForm.sample_height->text();

  exec_reduce += "\ni.ReductionSingleton().get_sample().geometry.width = ";
  exec_reduce += m_uiForm.sample_width->text();

  exec_reduce += "\ni.ReductionSingleton().get_sample().geometry.thickness = ";
  exec_reduce += m_uiForm.sample_thick->text();

  exec_reduce += "\n";

  return exec_reduce;
}
/**
 * Run the analysis script
 * @param typeStr :: The data reduction type, 1D or 2D
 */
void SANSRunWindow::handleReduceButtonClick(const QString &typeStr) {
  const States type = typeStr == "1D" ? OneD : TwoD;
  // Make sure that all settings are valid
  if (!areSettingsValid(type)) {
    return;
  }

  // new reduction is going to take place, remove the results from the last
  // reduction
  resetDefaultOutput();

  // The possiblities are batch mode or single run mode
  const RunMode runMode =
      m_uiForm.single_mode_btn->isChecked() ? SingleMode : BatchMode;
  if (runMode == SingleMode) {
    // Currently the components are moved with each reduce click. Check if a
    // load is necessary
    // This must be done before the script is written as we need to get correct
    // values from the
    // loaded raw data
    if (!handleLoadButtonClick()) {
      return;
    }
  }

  if (!entriesAreValid(RUN)) {
    return;
  }

  QString py_code;

  try {
    py_code = readUserFileGUIChanges(type);
  } catch (const std::runtime_error &e) {
    showInformationBox(e.what());
    return;
  }
  if (py_code.isEmpty()) {
    showInformationBox("Error: An error occurred while constructing the "
                       "reduction code, please check installation.");
    return;
  }

  const static QString PYTHON_SEP("C++handleReduceButtonClickC++");

  // copy the user setting to use as a base for future reductions after the one
  // that is about to start
  py_code += "\n_user_settings_copy = "
             "copy.deepcopy(i.ReductionSingleton().user_settings)";
  py_code += "\ni.SetVerboseMode(False)";
  // Need to check which mode we're in
  if (runMode == SingleMode) {
    py_code += readSampleObjectGUIChanges();

    // Provide a final check here to ensure that the settings are consistent. If
    // they are not consistent, the function
    // throws and the user has to fix these inconsistencies
    py_code += "\ni.are_settings_consistent()";

    py_code += reduceSingleRun();
    // output the name of the output workspace, this is returned up by the
    // runPythonCode() call below
    py_code += "\nprint('" + PYTHON_SEP + "'+reduced+'" + PYTHON_SEP + "')";
  } else {
    // Have we got anything to reduce?
    if (m_uiForm.batch_table->rowCount() == 0) {
      showInformationBox("Error: No run information specified.");
      return;
    }

    // Update the IDF file path for batch reductions
    updateIDFFilePathForBatch();

    // check for the detectors combination option
    // transform the SANS Diagnostic gui option in: 'rear', 'front' , 'both',
    // 'merged', None WavRangeReduction option
    QString combineDetOption, combineDetGuiOption;
    combineDetGuiOption = m_uiForm.detbank_sel->currentText();
    if (combineDetGuiOption == "main-detector-bank" ||
        combineDetGuiOption == "rear-detector")
      combineDetOption = "'rear'";
    else if (combineDetGuiOption == "HAB" ||
             combineDetGuiOption == "front-detector")
      combineDetOption = "'front'";
    else if (combineDetGuiOption == "both")
      combineDetOption = "'both'";
    else if (combineDetGuiOption == "merged")
      combineDetOption = "'merged'";
    else
      combineDetOption = "None";

    QString csv_file(m_uiForm.csv_filename->text());
    if (m_dirty_batch_grid) {
      QString selected_file =
          QFileDialog::getSaveFileName(this, "Save as CSV", m_last_dir);
      csv_file = saveBatchGrid(selected_file);
    }
    py_code.prepend("import SANSBatchMode as batch\n");
    const int fileFormat = m_uiForm.file_opt->currentIndex();
    // create a instance of fit_settings, so it will not complain if the
    // reduction fails
    // when restoring the scale and fit.
    QString fit = QString("\nfit_settings={'scale':%1,'shift':%2}")
                      .arg(m_uiForm.frontDetRescale->text())
                      .arg(m_uiForm.frontDetShift->text());
    py_code += fit;
    py_code += "\nfit_settings = batch.BatchReduce('" + csv_file + "','" +
               m_uiForm.file_opt->itemData(fileFormat).toString() + "'";
    if (m_uiForm.plot_check->isChecked()) {
      py_code += ", plotresults=True";
    }

    py_code += ", saveAlgs={";
    QStringList algs(getSaveAlgs());
    for (QStringList::const_iterator it = algs.begin(); it != algs.end();
         ++it) { // write a Python dict object in the form { algorithm_name :
                 // file extension , ... ,}
      py_code += "'" + *it + "':'" + SaveWorkspaces::getSaveAlgExt(*it) + "',";
    }
    py_code += "}";

    py_code += ", reducer=i.ReductionSingleton().reference(),";

    py_code += "combineDet=";
    py_code += combineDetOption;
    py_code += ",";
    py_code += " save_as_zero_error_free=";
    py_code += m_uiForm.zeroErrorCheckBox->isChecked() ? "True" : "False";
    py_code += ")";
  }

  // Disable buttons so that interaction is limited while processing data
  setProcessingState(type);

  // std::cout << "\n\n" << py_code.toStdString() << "\n\n";
  QString pythonStdOut = runReduceScriptFunction(py_code);

  // update fields in GUI as a consequence of results obtained during reduction
  double scale, shift;
  if (runMode == SingleMode) {
    // update front rescale and fit values
    scale =
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').rescaleAndShift.scale)")
            .trimmed()
            .toDouble();

    shift =
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().instrument.getDetector("
                                "'FRONT').rescaleAndShift.shift)")
            .trimmed()
            .toDouble();

  } else {
    scale = runReduceScriptFunction("print(fit_settings['scale'])")
                .trimmed()
                .toDouble();
    shift = runReduceScriptFunction("print(fit_settings['shift'])")
                .trimmed()
                .toDouble();
  }
  // update gui
  m_uiForm.frontDetRescale->setText(QString::number(scale, 'f', 8));
  m_uiForm.frontDetShift->setText(QString::number(shift, 'f', 8));
  // first process pythonStdOut
  QStringList pythonDiag = pythonStdOut.split(PYTHON_SEP);
  if (pythonDiag.count() > 1) {
    QString reducedWS = pythonDiag[1];
    reducedWS = reducedWS.split("\n")[0];
    resetDefaultOutput(reducedWS);
  }

  // Reset the objects by initialising a new reducer object
  if (runMode == SingleMode) // TODO: test if it is really necessary to reload
                             // the file settings.
  {
    py_code = "\ni.ReductionSingleton.clean(isis_reducer.ISISReducer)";
    py_code += "\ni." + getInstrumentClass();
    // restore the settings from the user file
    py_code += "\ni.ReductionSingleton().user_file_path='" +
               QFileInfo(m_uiForm.userfile_edit->text()).path() + "'";
    py_code += "\ni.ReductionSingleton().user_settings = _user_settings_copy";
    py_code += "\ni.ReductionSingleton().user_settings.execute(i."
               "ReductionSingleton())";

    std::cout << "\n\n" << py_code.toStdString() << "\n\n";

    runReduceScriptFunction(py_code);
  }
  // Mark that a reload is necessary to rerun the same reduction
  forceDataReload();
  // Reenable stuff
  setProcessingState(Ready);

  // If we used a temporary file in batch mode, remove it
  if (m_uiForm.batch_mode_btn->isChecked() && !m_tmp_batchfile.isEmpty()) {
    QFile tmp_file(m_tmp_batchfile);
    tmp_file.remove();
  }
}
/** Iterates through the validators and stops if it finds one that is shown and
 * enabled
 *  @param check the validator set to check
 *  @return true if there are no validator problems if false if it finds one
 */
bool SANSRunWindow::entriesAreValid(const ValCheck check) {
  if (check == LOAD || check == ALL) {
    return entriesAreValid(m_loadValids) && runFilesAreValid();
  }
  if (check == RUN || check == ALL) {
    return entriesAreValid(m_validators);
  }
  return false;
}
bool SANSRunWindow::entriesAreValid(ValMap &vals) {
  for (ValMap::const_iterator it = vals.begin(); it != vals.end(); ++it) {
    // is the validator active denoting a problem? don't do anything if it's
    // been disabled
    if ((!it->first->isHidden()) &&
        (it->first->isEnabled())) { // the first in the pair is the widget whose
                                    // value we're having a problem with
      it->second.first->setFocus();
      // the second part of the pair is the tab it's in
      m_uiForm.tabWidget->setCurrentWidget(it->second.second);
      QMessageBox::warning(this, "Validation Error",
                           "There is a problem with one or more entries on the "
                           "form. These are marked\nwith an *");
      return false;
    }
  }
  // no problems have been found
  return true;
}
/** Loop through all the m_runFiles file widgets and check they are all in the
 *  no error state
 *  @return true if there are no red stars on any run widgets, false otherwise
 */
bool SANSRunWindow::runFilesAreValid() {
  std::vector<MWRunFiles *>::const_iterator it = m_runFiles.begin();
  for (; it != m_runFiles.end(); ++it) {
    if (!(*it)->isValid()) {
      m_uiForm.runNumbers->setFocus();
      m_uiForm.tabWidget->setCurrentWidget(*it);
      QMessageBox::warning(this, "Validation Error",
                           "There is a problem with one or more entries on the "
                           "form. These are marked\nwith an *");
      return false;
    }
  }
  // there are no problems
  return true;
}
/** Generates the code that can run a reduction chain (and then reset it)
 *  @return Python code that can be passed to a Python interpreter
 */
QString SANSRunWindow::reduceSingleRun() const {
  QString reducer_code;
  if (m_uiForm.wav_dw_opt->currentText().toUpper().startsWith("RANGE")) {
    reducer_code += "\nreduced = i.CompWavRanges( ";
    reducer_code += "(" + m_uiForm.wavRanges->text() + ") ";
    reducer_code += ", plot=";
    reducer_code += m_uiForm.plot_check->isChecked() ? "True" : "False";
    if (m_uiForm.detbank_sel->currentIndex() >= 2) {
      reducer_code +=
          ", combineDet='" + m_uiForm.detbank_sel->currentText() + "'";
    }
    reducer_code += ", resetSetup=False)";
  } else {
    if (m_uiForm.detbank_sel->currentIndex() < 2) {
      reducer_code += "\nreduced = i.WavRangeReduction(full_trans_wav=False";
      reducer_code += ", resetSetup=False)";
    } else {
      reducer_code += "\nreduced = i.WavRangeReduction(full_trans_wav=False";
      reducer_code +=
          ", combineDet='" + m_uiForm.detbank_sel->currentText() + "'";
      reducer_code += ", resetSetup=False)";
    }

    if (m_uiForm.plot_check->isChecked()) {
      reducer_code += "\ni.PlotResult(reduced)";
    }
  }
  return reducer_code;
}

/** Returns the Python instrument class name to create for the current
  instrument
  @returns the Python class name corrosponding to the user selected instrument
*/
QString SANSRunWindow::getInstrumentClass() const {
  QString instrum = m_uiForm.inst_opt->currentText();
  instrum = instrum.isEmpty() ? "LOQ" : instrum;
  return instrum + "()";
}
void SANSRunWindow::handleRunFindCentre() {
  // Make sure that user file is valid
  if (!hasUserFileValidFileExtension()) {
    return;
  }

  // Set the log level of to at least notice:
  const auto initialLogLevel = g_centreFinderLog.getLevel();
  auto noticeLevelAsInt = static_cast<int>(Poco::Message::PRIO_NOTICE);
  auto hasToBeSwapped = initialLogLevel < noticeLevelAsInt ? true : false;
  if (hasToBeSwapped) {
    // Set to a notice setting
    g_centreFinderLog.setLevel(noticeLevelAsInt);
  }

  QLineEdit *beam_x;
  QLineEdit *beam_y;

  // this function looks for and reports any errors to the user
  if (!entriesAreValid()) {
    return;
  }

  if (m_uiForm.beamstart_box->currentIndex() == 1) {
    // Index == Start looking the position from the current one
    // check if the user provided the current position:
    // see wich radio is selected (REAR or FRONT) and confirm
    // that the position x and y are given.
    if ((m_uiForm.rear_radio->isChecked() &&
         (m_uiForm.rear_beam_x->text().isEmpty() ||
          m_uiForm.rear_beam_y->text().isEmpty())) ||
        (m_uiForm.front_radio->isChecked() &&
         (m_uiForm.front_beam_x->text().isEmpty() ||
          m_uiForm.front_beam_y->text().isEmpty()))) {
      showInformationBox(
          "Current centre postion is invalid, please check input.");
      return;
    }
  }

  /*
    A hidden feature. The handleLoadButtonClick method, set the detector
    based on the m_uiForm.detbank_sel, wich will influentiate the loading
    algorithm and the movement of the detector bank. So, we have to
    set the detector bank according to the selected Center.
   */
  QString coordinates_python_code;
  if (m_uiForm.rear_radio
          ->isChecked()) { // REAR selected -> detbank_sel <- REAR
    m_uiForm.detbank_sel->setCurrentIndex(0);
    beam_x = m_uiForm.rear_beam_x;
    beam_y = m_uiForm.rear_beam_y;
    coordinates_python_code =
        "print(i.ReductionSingleton().get_beam_center('rear')[0]);print("
        "i.ReductionSingleton().get_beam_center('rear')[1])";
  } else {
    coordinates_python_code =
        "print(i.ReductionSingleton().get_beam_center('front')[0]);print("
        "i.ReductionSingleton().get_beam_center('front')[1])";
    m_uiForm.detbank_sel->setCurrentIndex(
        1); // FRONT selected -> detbank_sel <- FRONT
    beam_x = m_uiForm.front_beam_x;
    beam_y = m_uiForm.front_beam_y;
  }

  // Start iteration
  g_centreFinderLog.notice("Loading data\n");
  handleLoadButtonClick();

  // Disable interaction
  setProcessingState(OneD);

  // This checks whether we have a sample run and that it has been loaded
  QString py_code(readUserFileGUIChanges(OneD));
  py_code += readSampleObjectGUIChanges();

  if (py_code.isEmpty()) {
    setProcessingState(Ready);
    return;
  }

  if (m_uiForm.beam_rmin->text().isEmpty()) {
    m_uiForm.beam_rmin->setText("60");
  }

  if (m_uiForm.beam_rmax->text().isEmpty()) {
    if (m_uiForm.inst_opt->currentText() == "LOQ") {
      m_uiForm.beam_rmax->setText("200");
    } else if (m_uiForm.inst_opt->currentText() == "SANS2D" ||
               m_uiForm.inst_opt->currentText() == "SANS2DTUBES") {
      m_uiForm.beam_rmax->setText("280");
    }
  }
  if (m_uiForm.beam_iter->text().isEmpty()) {
    m_uiForm.beam_iter->setText("15");
  }

  // FIXME: disable the flood file for the front detector. #6061
  if (m_uiForm.front_radio->isChecked())
    py_code += "i.SetDetectorFloodFile('')\n";

  // We need to load the FinDirectionEnum class
  py_code +=
      "from centre_finder import FindDirectionEnum as FindDirectionEnum \n";
  // Find centre function
  py_code += "i.FindBeamCentre(rlow=" + m_uiForm.beam_rmin->text() +
             ",rupp=" + m_uiForm.beam_rmax->text() +
             ",MaxIter=" + m_uiForm.beam_iter->text() + ",";

  if (m_uiForm.beamstart_box->currentIndex() == 0) {
    py_code += "xstart = None, ystart = None";
  } else {
    py_code += "xstart=float(" + beam_x->text() + ")/1000.,ystart=float(" +
               beam_y->text() + ")/1000.";
  }

  // define the number of interactions and close the FindBeamCentre method call.
  bool ok;
  QString tolerance_str(m_uiForm.toleranceLineEdit->text());
  double tolerance = tolerance_str.toDouble(&ok);
  if (ok)
    tolerance *= 1e-4; // transform in um
  if ((!ok || tolerance < 0) && !tolerance_str.isEmpty()) {
    QString info("You have chosen an invalid value for tolerance. Correct it "
                 "or leave it blank to use the default value.");
    QMessageBox::warning(this, "Wrong Input", info);
    m_uiForm.toleranceLineEdit->setFocus(Qt::OtherFocusReason);
    setProcessingState(Ready);
    return;
  }
  py_code += ", tolerance=" + QString::number(tolerance);

  // Set which part of the beam centre finder should be used
  auto updownIsRequired = m_uiForm.up_down_checkbox->isChecked();
  auto leftRightIsRequired = m_uiForm.left_right_checkbox->isChecked();
  if (updownIsRequired && leftRightIsRequired) {
    py_code += ", find_direction=FindDirectionEnum.ALL";
  } else if (updownIsRequired) {
    py_code += ", find_direction=FindDirectionEnum.UP_DOWN";
  } else if (leftRightIsRequired) {
    py_code += ", find_direction=FindDirectionEnum.LEFT_RIGHT";
  }
  py_code += ")";

  g_centreFinderLog.notice("Beam Centre Finder Start\n");
  m_uiForm.beamstart_box->setFocus();

  // Execute the code
  runReduceScriptFunction(py_code);

  QString coordstr = runReduceScriptFunction(coordinates_python_code);

  QString result("");
  if (coordstr.isEmpty()) {
    result = "No coordinates returned!";
  } else {
    // Remove all internal whitespace characters and replace with single space
    coordstr = coordstr.simplified();
    QStringList xycoords = coordstr.split(" ");
    if (xycoords.count() == 2) {
      double coord = xycoords[0].toDouble();
      beam_x->setText(QString::number(coord * 1000.));
      coord = xycoords[1].toDouble();
      beam_y->setText(QString::number(coord * 1000.));
      result = "Coordinates updated";
    } else {
      result = "Incorrect number of parameters returned from function, check "
               "script.";
    }
  }
  QString pyCode = "i.ReductionSingleton.clean(isis_reducer.ISISReducer)";
  pyCode += "\ni." + getInstrumentClass();
  pyCode += "\ni.ReductionSingleton().user_settings =";
  // Use python function to read the settings file and then extract the fields
  pyCode += "isis_reduction_steps.UserFile(r'" +
            m_uiForm.userfile_edit->text().trimmed() + "')";

  runReduceScriptFunction(pyCode);

  QString errors =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().user_settings.execute(i."
                              "ReductionSingleton()))")
          .trimmed();

  g_centreFinderLog.notice() << result.toStdString() << "\n";

  // Set the centre logger back to the initial log level
  if (hasToBeSwapped) {
    g_centreFinderLog.setLevel(initialLogLevel);
  }

  // Reenable stuff
  setProcessingState(Ready);
}
/** Save the output workspace from a single run reduction (i.e. the
 *  workspace m_outputWS) in all the user selected formats
 */
void SANSRunWindow::handleDefSaveClick() {
  const QString fileBase = m_uiForm.outfile_edit->text();
  if (fileBase.isEmpty()) {
    QMessageBox::warning(
        this, "Filename required",
        "A filename must be entered into the text box above to save this file");
  }

  if (!areSaveSettingsValid(m_outputWS)) {
    return;
  }

  // If we save with a zero-error-free correction we need to swap the
  QString workspaceNameBuffer = m_outputWS;
  QString clonedWorkspaceName = m_outputWS + "_cloned_temp";
  if (m_uiForm.zeroErrorCheckBox->isChecked()) {
    createZeroErrorFreeClone(m_outputWS, clonedWorkspaceName);
    if (AnalysisDataService::Instance().doesExist(
            clonedWorkspaceName.toStdString())) {
      m_outputWS = clonedWorkspaceName;
    }
  }

  const QStringList algs(getSaveAlgs());
  QString saveCommand;
  for (QStringList::const_iterator alg = algs.begin(); alg != algs.end();
       ++alg) {
    QString ext = SaveWorkspaces::getSaveAlgExt(*alg);
    QString fname = fileBase.endsWith(ext) ? fileBase : fileBase + ext;
    if ((*alg) == "SaveRKH")
      saveCommand +=
          (*alg) + "('" + m_outputWS + "','" + fname + "', Append=False)\n";
    else if ((*alg) == "SaveCanSAS1D") {
      saveCommand +=
          (*alg) + "('" + m_outputWS + "','" + fname + "', DetectorNames=";
      Workspace_sptr workspace_ptr =
          AnalysisDataService::Instance().retrieve(m_outputWS.toStdString());
      MatrixWorkspace_sptr matrix_workspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
      if (matrix_workspace) {
        auto detectorSelection = m_uiForm.detbank_sel->currentText();
        setTransmissionOnSaveCommand(saveCommand, matrix_workspace,
                                     detectorSelection);
      }

      // Add the sample information to the output
      auto sampleWidth = m_uiForm.sample_width->text();
      auto sampleHeight = m_uiForm.sample_height->text();
      auto sampleThickness = m_uiForm.sample_thick->text();
      auto geometryID = m_uiForm.sample_geomid->currentText();
      // Remove the first three characters, since they are unwanted
      auto geometryName = geometryID.mid(3);
      saveCommand += ", Geometry='" + geometryName +
                     "', SampleHeight=" + sampleHeight +
                     ", SampleWidth=" + sampleWidth +
                     ", SampleThickness=" + sampleThickness;
      saveCommand += ")\n";
    } else if ((*alg) == "SaveNXcanSAS") {
      saveCommand +=
          (*alg) + "('" + m_outputWS + "','" + fname + "', DetectorNames=";
      Workspace_sptr workspace_ptr =
          AnalysisDataService::Instance().retrieve(m_outputWS.toStdString());
      MatrixWorkspace_sptr matrix_workspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);

      if (matrix_workspace) {
        auto detectorSelection = m_uiForm.detbank_sel->currentText();
        setTransmissionOnSaveCommand(saveCommand, matrix_workspace,
                                     detectorSelection);
      }
      saveCommand += ")\n";
    } else
      saveCommand += (*alg) + "('" + m_outputWS + "','" + fname + "')\n";
  }

  saveCommand += "print('success')\n";
  QString result = runPythonCode(saveCommand).trimmed();

  // Revert changes and delete the zero-free workspace
  if (this->m_uiForm.zeroErrorCheckBox->isChecked()) {
    if (AnalysisDataService::Instance().doesExist(
            clonedWorkspaceName.toStdString())) {
      deleteZeroErrorFreeClone(clonedWorkspaceName);
    }
  }
  m_outputWS = workspaceNameBuffer;

  if (result != "success") {
    QMessageBox::critical(this, "Error saving workspace",
                          "Problem encountered saving workspace, does it still "
                          "exist. There may be more information in the results "
                          "console?");
  }
}

/**
 * Checks if the save options are valid
 */
bool SANSRunWindow::areSaveSettingsValid(const QString &workspaceName) {
  Mantid::API::MatrixWorkspace_sptr ws =
      AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(
          workspaceName.toStdString());
  auto is1D = ws->getNumberHistograms() == 1;
  auto isCanSAS = m_uiForm.saveCan_check->isChecked();

  QString message;

  auto isValid = checkSaveOptions(message, is1D, isCanSAS);

  // Print the error message if there are any
  if (!message.isEmpty()) {
    QString warning = "Please correct these settings before proceeding:\n";
    warning += message;
    QMessageBox::warning(this, "Inconsistent input", warning);
  }
  return isValid;
}

/**
 * Set up controls based on the users selection in the combination box
 * @param new_index :: The new index that has been set
 */
void SANSRunWindow::handleWavComboChange(int new_index) {
  QString userSel = m_uiForm.wav_dw_opt->itemText(new_index);

  if (userSel.toUpper().contains("LOG")) {
    m_uiForm.wav_step_lbl->setText("dW / W");
  } else {
    m_uiForm.wav_step_lbl->setText("step");
  }

  if (userSel.toUpper().startsWith("RANGE")) {
    m_uiForm.wav_stack->setCurrentIndex(1);
    m_uiForm.wavRanVal_lb->setEnabled(true);
  } else {
    m_uiForm.wav_stack->setCurrentIndex(0);
    m_uiForm.wavRanVal_lb->setEnabled(false);
  }
}
/**
 * A ComboBox option change
 * @param new_index :: The new index that has been set
 */
void SANSRunWindow::handleStepComboChange(int new_index) {
  if (!sender())
    return;

  QString origin = sender()->objectName();
  if (origin.startsWith("q_dq")) {
    if (new_index == 0) {
      m_uiForm.q_stack->setCurrentIndex(0);
      m_uiForm.q_step_lbl->setText("step");
    } else if (new_index == 1) {
      m_uiForm.q_stack->setCurrentIndex(0);
      m_uiForm.q_step_lbl->setText("dQ / Q");
    } else {
      m_uiForm.q_stack->setCurrentIndex(1);
    }
  } else {
    if (new_index == 0)
      m_uiForm.qy_step_lbl->setText("XY step");
    else
      m_uiForm.qy_step_lbl->setText("dQ / Q");
  }
}

/**
 * Called when the show mask button has been clicked
 */
void SANSRunWindow::handleShowMaskButtonClick() {
  QString analysis_script;
  addUserMaskStrings(analysis_script, "i.Mask", DefaultMask);
  analysis_script += "\ni.DisplayMask()";

  m_uiForm.showMaskBtn->setEnabled(false);
  m_uiForm.showMaskBtn->setText("Working...");

  runReduceScriptFunction(analysis_script);

  m_uiForm.showMaskBtn->setEnabled(true);
  m_uiForm.showMaskBtn->setText("Display mask");
}

/** Update the GUI and the Python objects with the instrument selection
 * @throw runtime_error if the instrument doesn't have exactly two detectors
 */
void SANSRunWindow::handleInstrumentChange() {
  const std::string facility = ConfigService::Instance().getFacility().name();
  if (facility != "ISIS") {
    QMessageBox::critical(
        this, "Unsupported facility",
        QString("Only the ISIS facility is supported by this interface.\n") +
            "Select ISIS as your default facility in "
            "View->Preferences...->Mantid to continue.");
    return;
  }

  // need this if facility changed to force update of technique at this point
  // m_uiForm.inst_opt->setTechniques(m_uiForm.inst_opt->getTechniques());

  if (m_uiForm.inst_opt->currentText() == "SANS2DTUBES")
    ConfigService::Instance().setString("default.instrument", "SANS2D");
  else
    ConfigService::Instance().setString(
        "default.instrument", m_uiForm.inst_opt->currentText().toStdString());

  // Hide the "SANS2D_EVENT" instrument, if present.
  const int sans2dEventIndex = m_uiForm.inst_opt->findText("SANS2D_EVENT");
  if (sans2dEventIndex != -1)
    m_uiForm.inst_opt->removeItem(sans2dEventIndex);

  // set up the required Python objects and delete what's out of date (perhaps
  // everything is cleaned here)
  const QString instClass(getInstrumentClass());

  // Only set the instrument if it isn't alread set to what has been selected.
  // This is useful on interface start up, where we have already loaded the user
  // file
  // and don't want to set the instrument twice.
  const QString currentInstName =
      runPythonCode(
          "print(i.ReductionSingleton().get_instrument().versioned_name())")
          .trimmed();
  if (currentInstName != m_uiForm.inst_opt->currentText()) {
    QString pyCode("i.ReductionSingleton.clean(isis_reducer.ISISReducer)");
    pyCode += "\ni." + instClass;
    runReduceScriptFunction(pyCode);
  }

  // now update the GUI
  fillDetectNames(m_uiForm.detbank_sel);
  QString detect = runReduceScriptFunction(
      "print(i.ReductionSingleton().instrument.cur_detector().name())");
  QString detectorSelection =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().instrument.det_selection)")
          .trimmed();
  int ind = m_uiForm.detbank_sel->findText(detect);
  // We set the detector selection only if nothing is set yet.
  // Previously, we didn't handle merged and both at this point
  if (detectorSelection == m_constants.getPythonEmptyKeyword() ||
      detectorSelection.isEmpty()) {
    if (ind != -1) {
      m_uiForm.detbank_sel->setCurrentIndex(ind);
    }
  }

  m_uiForm.beam_rmin->setText("60");
  if (instClass == "LOQ()") {
    m_uiForm.beam_rmax->setText("200");

    m_uiForm.geom_stack->setCurrentIndex(0);

  } else if (instClass == "SANS2D()" || instClass == "SANS2DTUBES()") {
    m_uiForm.beam_rmax->setText("280");

    m_uiForm.geom_stack->setCurrentIndex(1);
  }
  // flag that the user settings file needs to be loaded for this instrument
  m_cfg_loaded = false;

  // disable the Geometry -> Set Centre widgets that can not be edited
  // for SANS2D experiments.
  QWidget *front_center_widgets[] = {
      m_uiForm.front_beam_x, m_uiForm.front_beam_y, m_uiForm.front_radio};
  bool loq_selected = (instClass == "LOQ()");
  for (int i = 0; i < 3; i++)
    front_center_widgets[i]->setEnabled(true);
  // Set the label of the radio buttons according to the
  // beamline usage:
  // REAR/FRONT -> SANS2D
  // MAIN/HAB -> LOQ
  if (loq_selected) {
    m_uiForm.front_radio->setText("&HAB");
    m_uiForm.rear_radio->setText("&Main");
  } else {
    m_uiForm.front_radio->setText("&Front");
    m_uiForm.rear_radio->setText("&Rear");
  }

  // LOQ does not have event mode collection
  // hence, hide the widgets related to slice event mode data.
  bool hide_events_gui = loq_selected;
  m_uiForm.slicePb->setHidden(hide_events_gui);
  m_uiForm.sliceEvent->setHidden(hide_events_gui);
  m_uiForm.l_events_label->setHidden(hide_events_gui);
  m_uiForm.l_events_binning->setHidden(hide_events_gui);
}

/** Record if the user has changed the default filename, because then we don't
 *  change it
 */
void SANSRunWindow::setUserFname() { m_userFname = true; }

/** Enables or disables the floodFile run widget
 *  @param state :: Qt::CheckState enum value, Checked means enable otherwise
 * disabled
 */
void SANSRunWindow::prepareFlood(int state) {
  if (sender() == m_uiForm.enableRearFlood_ck)
    m_uiForm.floodRearFile->setEnabled(state == Qt::Checked);
  if (sender() == m_uiForm.enableFrontFlood_ck)
    m_uiForm.floodFrontFile->setEnabled(state == Qt::Checked);
}
/**Enables  the default save button, saveDefault_Btn, if there is an output
 * workspace
 * stored in m_outputWS and text in outfile_edit
 */
void SANSRunWindow::enableOrDisableDefaultSave() {
  if (m_outputWS.isEmpty()) { // setEnabled(false) gets run below
  } else if (m_uiForm.outfile_edit->text()
                 .isEmpty()) { // setEnabled(false) gets run below
  } else {                     // ensure that one format box is checked
    for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end();
         ++i) {
      if (i.key()->isChecked()) {
        m_uiForm.saveDefault_btn->setEnabled(true);
        return;
      }
    }
  }
  m_uiForm.saveDefault_btn->setEnabled(false);
}
/** connected to the Multi-period check box it shows or hides the multi-period
 * boxes
 *  on the file widgets
 *  @param tickState an enum (Qt::CheckState) that indicates if check box was
 * ticked or not
 */
void SANSRunWindow::disOrEnablePeriods(const int tickState) {
  const bool enable = tickState == Qt::Checked;
  std::vector<MWRunFiles *>::const_iterator it = m_runFiles.begin();
  for (; it != m_runFiles.end(); ++it) {
    (*it)->doMultiEntry(enable);
  }
}

/**
 * Enable or disable the controls that corrospond to batch or single run mode
 */
void SANSRunWindow::switchMode() {
  const RunMode modeId =
      m_uiForm.single_mode_btn->isChecked() ? SingleMode : BatchMode;

  if (modeId == SingleMode) {
    m_uiForm.mode_stack->setCurrentIndex(0);
    m_uiForm.load_dataBtn->setEnabled(true);
    m_uiForm.sampDetails_gb->setEnabled(true);
    m_uiForm.sampDetails_gb->setToolTip("The dimensions of the sample");
  } else if (modeId == BatchMode) {
    m_uiForm.mode_stack->setCurrentIndex(1);
    m_uiForm.load_dataBtn->setEnabled(false);
    m_uiForm.sampDetails_gb->setEnabled(false);
    m_uiForm.sampDetails_gb->setToolTip("Batch mode has been selected the "
                                        "sample geometry will be read from the "
                                        "sample workspace");
  }
}

/**
 * Paste to the batch table
 */
void SANSRunWindow::pasteToBatchTable() {
  if (!m_cfg_loaded) {
    showInformationBox("Please load the relevant user file before continuing.");
    return;
  }

  QClipboard *clipboard = QApplication::clipboard();
  QString copied_text = clipboard->text();
  if (copied_text.isEmpty())
    return;

  QStringList runlines = copied_text.split("\n");
  QStringListIterator sitr(runlines);
  int errors(0);
  while (sitr.hasNext()) {
    QString line = sitr.next().simplified();
    if (!line.isEmpty()) {
      errors += addBatchLine(line);
    }
  }
  if (errors > 0) {
    showInformationBox(
        "Warning: " + QString::number(errors) +
        " malformed lines detected in pasted text. Lines skipped.");
  }
  if (m_uiForm.batch_table->rowCount() > 0) {
    m_dirty_batch_grid = true;
    setProcessingState(Ready);
  }
}

/**
 * Clear the batch table
 */
void SANSRunWindow::clearBatchTable() {
  int row_count = m_uiForm.batch_table->rowCount();
  for (int i = row_count - 1; i >= 0; --i) {
    m_uiForm.batch_table->removeRow(i);
  }
  m_dirty_batch_grid = false;
  m_tmp_batchfile = "";
}

/**
 * Clear the logger field
 */
void SANSRunWindow::clearLogger() {
  m_uiForm.logging_field->clear();
  m_uiForm.tabWidget->setTabText(4, "Logging");
}

/**Respond to the Front detector Q range check box.
 * @param state :: equal to Qt::Checked or not
 */
void SANSRunWindow::updateFrontDetQrange(int state) {
  if (state == Qt::Checked) {
    m_uiForm.frontDetQmin->setEnabled(true);
    m_uiForm.frontDetQmax->setEnabled(true);
    runReduceScriptFunction("i.ReductionSingleton().instrument.getDetector('"
                            "FRONT').rescaleAndShift.qRangeUserSelected=True");
  } else {
    m_uiForm.frontDetQmin->setEnabled(false);
    m_uiForm.frontDetQmax->setEnabled(false);
    runReduceScriptFunction("i.ReductionSingleton().instrument.getDetector('"
                            "FRONT').rescaleAndShift.qRangeUserSelected=False");
  }
}

/**Respond to the Merge Q range check box.
 * @param state :: equal to Qt::Checked or not
 */
void SANSRunWindow::updateMergeQRange(int state) {
  if (state == Qt::Checked) {
    m_uiForm.mergeQMax->setEnabled(true);
    m_uiForm.mergeQMin->setEnabled(true);
    runReduceScriptFunction("i.ReductionSingleton().instrument.getDetector('"
                            "FRONT').mergeRange.merge_range=True");
  } else {
    m_uiForm.mergeQMax->setEnabled(false);
    m_uiForm.mergeQMin->setEnabled(false);
    runReduceScriptFunction("i.ReductionSingleton().instrument.getDetector('"
                            "FRONT').mergeRange.merge_range=False");
  }
}

/**Respond to the "Use default transmission" check box being clicked. If
 * the box is checked the transmission fit wavelength maximum and minimum
 * boxs with be set to the defaults for the instrument and disabled.
 * Otherwise they are enabled
 * @param state :: equal to Qt::Checked or not
 */
void SANSRunWindow::updateTransInfo(int state) {
  QLineEdit *_min = m_uiForm.trans_min, *_max = m_uiForm.trans_max;

  if (sender() == m_uiForm.transFit_ck_can) {
    _min = m_uiForm.trans_min_can;
    _max = m_uiForm.trans_max_can;
  }

  if (state == Qt::Checked) {
    _min->setEnabled(true);
    _min->setText(runReduceScriptFunction(
                      "print(i.ReductionSingleton().instrument.WAV_RANGE_MIN)")
                      .trimmed());

    _max->setEnabled(true);
    _max->setText(runReduceScriptFunction(
                      "print(i.ReductionSingleton().instrument.WAV_RANGE_MAX)")
                      .trimmed());

  } else {
    _min->setEnabled(false);
    _min->setText("");

    _max->setEnabled(false);
    _max->setText("");
  }
}

/** A slot to validate entries for Python lists and tupples
 */
void SANSRunWindow::checkList() {
  // may be a need to generalise this
  QLineEdit *toValdate = m_uiForm.wavRanges;
  QLabel *validator = m_uiForm.wavRanVal_lb;
  const std::string input(toValdate->text().trimmed().toStdString());

  bool valid(false);
  // split up the comma separated list ignoring spaces
  Poco::StringTokenizer in(input, ",", Poco::StringTokenizer::TOK_TRIM);
  try {
    for (Poco::StringTokenizer::Iterator i = in.begin(), end = in.end();
         i != end; ++i) { // try a lexical cast, we don't need its result only
                          // if there was an error
      boost::lexical_cast<double>(*i);
    }
    // there were no errors
    if (!input.empty()) {
      valid = true;
    }
  } catch (boost::bad_lexical_cast &) { // there is a problem with the input
                                        // somewhere
    valid = false;
  }

  if (valid) {
    validator->hide();
  } else {
    validator->show();
  }
}

void SANSRunWindow::setLoggerTabTitleToWarn() {
  m_uiForm.tabWidget->setTabText(4, "Logging - WARNINGS");
}

/** Record the output workspace name, if there is no output
 *  workspace pass an empty string or an empty argument list
 *  @param wsName :: the name of the output workspace or empty for no output
 */
void SANSRunWindow::resetDefaultOutput(const QString &wsName) {
  m_outputWS = wsName;
  enableOrDisableDefaultSave();

  if (!m_userFname) {
    if (m_uiForm.detbank_sel->currentIndex() == 2) // both selected
      m_uiForm.outfile_edit->setText("");
    else
      m_uiForm.outfile_edit->setText(wsName);
  }
}
/** Passes information about the selected transmission runs to the Python
 * objects
 *  @param trans run widget box with the selected transmission (run with a
 * sample present) file
 *  @param direct run widget box with the selected direct (run with no sample
 * present) file
 *  @param assignFn this is different for can or sample
 */
bool SANSRunWindow::assignMonitorRun(API::MWRunFiles &trans,
                                     API::MWRunFiles &direct,
                                     const QString &assignFn) {
  // need something to place between names printed by Python that won't be
  // intepreted as the names or removed as white space
  const static QString PYTHON_SEP("C++assignMonitorRunC++");

  QString assignCom("i." + assignFn + "(r'" + trans.getFirstFilename() + "'");
  assignCom.append(", r'" + direct.getFirstFilename() + "'");

  int period = trans.getEntryNum();
  if (period != MWRunFiles::ALL_ENTRIES) {
    assignCom.append(", period_t=" + QString::number(period));
  }

  period = direct.getEntryNum();
  // we can only do single period reductions now
  if (period != MWRunFiles::ALL_ENTRIES) {
    assignCom.append(", period_d=" + QString::number(period));
  }
  assignCom.append(")");
  // assign the workspace name to a Python variable and read back some details
  QString pythonC = "t1, t2 = " + assignCom + ";print('" + PYTHON_SEP +
                    "' + ' ' +  t1 + ' ' + '" + PYTHON_SEP + "' + ' ' + t2)";
  QString ws_names = runReduceScriptFunction(pythonC);
  if (ws_names.startsWith("error", Qt::CaseInsensitive)) {
    throw std::runtime_error("Couldn't load a transmission file");
  }

  // read the informtion returned from Python
  QString trans_ws = ws_names.section(PYTHON_SEP, 1, 1).trimmed();
  QString direct_ws = ws_names.section(PYTHON_SEP, 2).trimmed();

  bool status = (!trans_ws.isEmpty()) && (!direct_ws.isEmpty());

  // if the workspaces have loaded
  if (status) { // save the workspace names
    m_workspaceNames.insert(trans_ws);
    m_workspaceNames.insert(direct_ws);
  }
  return status;
}
/**
 * Load a scatter sample file or can run via Python objects using the passed
 * Python command
 * @param[in] runFile name of file to load
 * @param[in] assignFn the Python command to run
 * @return true if there were no Python errors, false otherwise
 */
bool SANSRunWindow::assignDetBankRun(API::MWRunFiles &runFile,
                                     const QString &assignFn) {
  // need something to place between names printed by Python that won't be
  // intepreted as the names or removed as white space
  const static QString PYTHON_SEP("C++assignDetBankRunC++");

  QString assignCom("i." + assignFn + "(r'" + runFile.getFirstFilename() + "'");
  assignCom.append(", reload = True");
  int period = runFile.getEntryNum();

  if (period != MWRunFiles::ALL_ENTRIES) {
    assignCom.append(", period = " + QString::number(period));
  }

  assignCom.append(")");

  // assign the workspace name to a Python variable and read back some details

  QString run_info;
  run_info =
      QString(
          "i.SetCentre('%1','%2','rear') \ni.SetCentre('%3','%4','front')\n")
          .arg(m_uiForm.rear_beam_x->text())
          .arg(m_uiForm.rear_beam_y->text())
          .arg(m_uiForm.front_beam_x->text())
          .arg(m_uiForm.front_beam_y->text());
  run_info += "SCATTER_SAMPLE = " + assignCom;
  run_info += ";ws_name = SCATTER_SAMPLE if not isinstance(SCATTER_SAMPLE, "
              "tuple) else SCATTER_SAMPLE[0]";
  run_info += ";print('" + PYTHON_SEP + "' + ' ' + ws_name)";
  run_info = runReduceScriptFunction(run_info);
  if (run_info.startsWith("error", Qt::CaseInsensitive)) {
    throw std::runtime_error("Couldn't load sample or can");
  }
  // read the informtion returned from Python
  QString base_workspace = run_info.section(PYTHON_SEP, 1, 1).trimmed();

  if (assignFn.contains("can", Qt::CaseInsensitive)) {
    m_experCan = base_workspace;
  } else {
    m_experWksp = base_workspace;
  }

  m_workspaceNames.insert(base_workspace);

  return !base_workspace.isEmpty();
}
/** Gets the detectors that the instrument has and fills the
 *  combination box with these, there must exactly two detectors
 *  @param output [out] this combination box will be cleared and filled with the
 * new names
 *  @throw runtime_error if there aren't exactly two detectors
 */
void SANSRunWindow::fillDetectNames(QComboBox *output) {
  QString detsTuple = runReduceScriptFunction(
      "print(i.ReductionSingleton().instrument.listDetectors())");

  if (detsTuple.isEmpty()) { // this happens if the run Python signal hasn't yet
                             // been connected
    return;
  }

  QStringList dets = detsTuple.split("'", QString::SkipEmptyParts);
  // the tuple will be of the form ('det1', 'det2'), hence the split should
  // return 5 parts
  if (dets.count() != 5) {
    QMessageBox::critical(this, "Can't Load Instrument",
                          "The instrument must have only 2 detectors. Can't "
                          "proceed with this instrument");
    throw std::runtime_error("Invalid instrument setting, you should be able "
                             "to continue by selecting a valid instrument");
  }

  // The setting of the detector here has been the cause of problems for
  // (apparently years).
  // The code assumes for the indices
  // |     | LOQ                | SANS2D         | LARMOR                  |
  // |-----|--------------------|----------------|-------------------------|
  // |  0  | main-detector-bank | rear-detector  | DetectorBench           |
  // |  1  | HAB                | front-detector | front-detector (unused) |
  // |  2  | both               | both           | both                    |
  // |  3  | merged             | merged         | merged                  |
  // But the Python method above listDetectors will return the selected detector
  // first,
  // ie if HAB was selected on LOQ, then it would return
  // ["HAB","main-detector-bank"]
  // if main-detector-bank was selected on LOQ, then it would return
  // ["main-detector-bank", "HAB"]
  // which means we need to assign the names to the right slots.
  QStringList detectorNames = {dets[1], dets[3]};
  for (auto &name : detectorNames) {
    if (name == "main-detector-bank" || name == "rear-detector" ||
        name == "DetectorBench") {
      output->setItemText(0, name);
    }

    if (name == "HAB" || name == "front-detector") {
      output->setItemText(1, name);
    }
  }
}
/** Checks if the workspace is a group and returns the first member of group,
 * throws
 *  if nothing can be retrived
 *  @param in [in] the group to examine
 *  @param member [in] entry or period number of the requested workspace, these
 * start at 1
 *  @return the first member of the passed group
 *  @throw NotFoundError if a workspace can't be returned
 */
Mantid::API::MatrixWorkspace_sptr
SANSRunWindow::getGroupMember(Mantid::API::Workspace_const_sptr in,
                              const int member) const {
  Mantid::API::WorkspaceGroup_const_sptr group =
      boost::dynamic_pointer_cast<const Mantid::API::WorkspaceGroup>(in);
  if (!group) {
    throw Mantid::Kernel::Exception::NotFoundError(
        "Problem retrieving workspace ", in->getName());
  }

  const std::vector<std::string> gNames = group->getNames();
  // currently the names array starts with the name of the group
  if (static_cast<int>(gNames.size()) < member + 1) {
    throw Mantid::Kernel::Exception::NotFoundError(
        "Workspace group" + in->getName() + " doesn't have " +
            boost::lexical_cast<std::string>(member) + " entries",
        member);
  }
  // throws NotFoundError if the workspace couldn't be found
  Mantid::API::Workspace_sptr base =
      Mantid::API::AnalysisDataService::Instance().retrieve(gNames[member]);
  Mantid::API::MatrixWorkspace_sptr memberWS =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(base);
  if (!memberWS) {
    throw Mantid::Kernel::Exception::NotFoundError(
        "Problem getting period number " +
            boost::lexical_cast<std::string>(member) +
            " from group workspace " + base->getName(),
        member);
  }

  return memberWS;
}
/** Find which save formats have been selected by the user
 *  @return save algorithm names
 */
QStringList SANSRunWindow::getSaveAlgs() {
  QStringList checked;
  for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end();
       ++i) {                   // the key is the check box
    if (i.key()->isChecked()) { // and value() is the name of the algorithm
                                // associated with that checkbox
      checked.append(i.value());
    }
  }
  return checked;
}
/**
 * Handle a delete notification from Mantid
 * @param p_dnf :: A Mantid delete notification
 */
void SANSRunWindow::handleMantidDeleteWorkspace(
    Mantid::API::WorkspacePostDeleteNotification_ptr p_dnf) {
  QString wkspName = QString::fromStdString(p_dnf->objectName());
  if (m_workspaceNames.find(wkspName) != m_workspaceNames.end()) {
    forceDataReload();
  }
}
/**
 * Format a double as a string
 * @param value :: The double to convert to a string
 * @param colour :: The colour
 * @param format :: The format char
 * @param precision :: The precision
 */
QString SANSRunWindow::formatDouble(double value, const QString &colour,
                                    char format, int precision) {
  return QString("<font color='") + colour + QString("'>") +
         QString::number(value, format, precision) + QString("</font>");
}

/**
 * Raise a message if current status allows
 * @param msg :: The message to include in the box
 * @param index :: The tab index to set as current
 */
void SANSRunWindow::raiseOneTimeMessage(const QString &msg, int index) {
  if (m_warnings_issued)
    return;
  if (index >= 0) {
    m_uiForm.tabWidget->setCurrentIndex(index);
  }
  showInformationBox(msg);
  m_warnings_issued = true;
}

/**
 * Rest the geometry details box
 */
void SANSRunWindow::resetGeometryDetailsBox() {
  QString blank("-");
  // LOQ
  m_uiForm.dist_mod_mon->setText(blank);

  // SANS2D
  m_uiForm.dist_mon_s2d->setText(blank);
  m_uiForm.dist_sample_ms_s2d->setText(blank);
  m_uiForm.dist_can_ms_s2d->setText(blank);
  m_uiForm.dist_bkgd_ms_s2d->setText(blank);

  for (int i = 0; i < 3; ++i) {
    // LOQ
    QMutableHashIterator<QString, QLabel *> litr(m_loq_detlabels[i]);
    while (litr.hasNext()) {
      litr.next();
      litr.value()->setText(blank);
    }
    // SANS2D
    QMutableHashIterator<QString, QLabel *> sitr(m_s2d_detlabels[i]);
    while (sitr.hasNext()) {
      sitr.next();
      sitr.value()->setText(blank);
    }
  }
}

void SANSRunWindow::cleanup() {
  Mantid::API::AnalysisDataServiceImpl &ads =
      Mantid::API::AnalysisDataService::Instance();
  auto workspaces = ads.getObjectNames();
  auto iend = workspaces.end();
  for (auto itr = workspaces.begin(); itr != iend; ++itr) {
    QString name = QString::fromStdString(*itr);
    if (name.endsWith("_raw") || name.endsWith("_nxs")) {
      ads.remove(*itr);
    }
  }
}

/**
 * Add a csv line to the batch grid
 * @param csv_line :: Add a line of csv text to the grid
 * @param separator :: An optional separator, default = ","
 */
int SANSRunWindow::addBatchLine(QString csv_line, QString separator) {
  // Try to detect separator if one is not specified
  if (separator.isEmpty()) {
    if (csv_line.contains(",")) {
      separator = ",";
    } else {
      separator = " ";
    }
  }
  QStringList elements = csv_line.split(separator);
  // Insert new row
  int row = m_uiForm.batch_table->rowCount();
  m_uiForm.batch_table->insertRow(row);

  int nelements = elements.count() - 1;
  bool error(false);
  for (int i = 0; i < nelements;) {
    QString cola = elements.value(i);
    QString colb = elements.value(i + 1);
    if (m_allowed_batchtags.contains(cola)) {
      if (!m_allowed_batchtags.contains(colb)) {
        if (!colb.isEmpty() && !cola.contains("background")) {
          m_uiForm.batch_table->setItem(row, m_allowed_batchtags.value(cola),
                                        new QTableWidgetItem(colb));
        }
        i += 2;
      } else {
        ++i;
      }
    } else {
      error = true;
      break;
    }
  }
  if (error) {
    m_uiForm.batch_table->removeRow(row);
    return 1;
  }
  return 0;
}

/**
 * Save the batch file to a CSV file.
 * @param filename :: An optional filename. If none is given then a temporary
 * file is used and its name returned
 */
QString SANSRunWindow::saveBatchGrid(const QString &filename) {
  QString csv_filename = filename;
  if (csv_filename.isEmpty()) {
    // Generate a temporary filename
    QTemporaryFile tmp;
    tmp.open();
    csv_filename = tmp.fileName();
    tmp.close();
    m_tmp_batchfile = csv_filename;
  }

  QFile csv_file(csv_filename);
  if (!csv_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    showInformationBox("Error: Cannot write to CSV file \"" + csv_filename +
                       "\".");
    return "";
  }

  QTextStream out_strm(&csv_file);
  int nrows = m_uiForm.batch_table->rowCount();
  const QString separator(",");
  for (int r = 0; r < nrows; ++r) {
    for (int c = 0; c < 7; ++c) {
      out_strm << m_allowed_batchtags.key(c) << separator;
      if (QTableWidgetItem *item = m_uiForm.batch_table->item(r, c)) {
        out_strm << item->text();
      }
      if (c < 6)
        out_strm << separator;
    }
    out_strm << "\n";
  }
  csv_file.close();
  if (!filename.isEmpty()) {
    m_tmp_batchfile = "";
    m_dirty_batch_grid = false;
    m_uiForm.csv_filename->setText(csv_filename);
  } else {
    m_uiForm.csv_filename->clear();
  }
  return csv_filename;
}

/** Display the first data search and the number of data directorys to users and
 *  update our input directory
 */
void SANSRunWindow::upDateDataDir() {
  const std::vector<std::string> &dirs =
      ConfigService::Instance().getDataSearchDirs();
  if (!dirs.empty()) { // use the first directory in the list
    QString dataDir = QString::fromStdString(dirs.front());
    // check for windows and its annoying path separator thing, windows' paths
    // can't contain /
    if (dataDir.contains('\\') && !dataDir.contains('/')) {
      dataDir.replace('\\', '/');
    }
    m_uiForm.loadDir_lb->setText(dataDir);

    m_uiForm.plusDirs_lb->setText(
        QString("+ ") + QString::number(dirs.size() - 1) + QString(" others"));
  } else {
    m_uiForm.loadDir_lb->setText("No input search directories defined");
    m_uiForm.plusDirs_lb->setText("");
  }
}
/** Update the input directory labels if the Mantid system input
 *  directories have changed
 *  @param pDirInfo :: a pointer to an object with the output directory name in
 * it
 */
void SANSRunWindow::handleInputDirChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo) {
  if (pDirInfo->key() == "datasearch.directories") {
    upDateDataDir();
  }
}

/** Slot when phi masking changed in GUI
 */
void SANSRunWindow::phiMaskingChanged() { updateMaskTable(); }

/** Slot when phi masking changed in GUI
    @param i unused argument required for combobox signal/slot
*/
void SANSRunWindow::phiMaskingChanged(int i) {
  Q_UNUSED(i);
  updateMaskTable();
}

void SANSRunWindow::transSelectorChanged(int currindex) {
  bool visible = false;
  if (currindex != 0)
    visible = true;

  QWidget *wid[] = {m_uiForm.trans_can_label, m_uiForm.transFitOnOff_can,
                    m_uiForm.transFit_ck_can, m_uiForm.trans_min_can,
                    m_uiForm.trans_max_can,   m_uiForm.trans_opt_can};
  for (size_t i = 0; i < 6; i++)
    wid[i]->setVisible(visible);
}

void SANSRunWindow::loadTransmissionSettings() {

  QString transMin =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().transmission_calculator."
                              "lambdaMin('SAMPLE'))")
          .trimmed();
  if (transMin == "None") {
    m_uiForm.transFit_ck->setChecked(false);
  } else {
    m_uiForm.transFit_ck->setChecked(true);
    m_uiForm.trans_min->setText(transMin);
    m_uiForm.trans_max->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().transmission_"
                                "calculator.lambdaMax('SAMPLE'))")
            .trimmed());
  }

  QString text =
      runReduceScriptFunction("print("
                              "i.ReductionSingleton().transmission_calculator."
                              "fitMethod('SAMPLE'))")
          .trimmed();
  int index = m_uiForm.trans_opt->findText(text, Qt::MatchFixedString);
  if (index >= 0) {
    m_uiForm.trans_opt->setCurrentIndex(index);
  }
  if (text == "OFF" || text == "None")
    m_uiForm.transFitOnOff->setChecked(false);
  else
    m_uiForm.transFitOnOff->setChecked(true);

  transMin = runReduceScriptFunction("print("
                                     "i.ReductionSingleton().transmission_"
                                     "calculator.lambdaMin('CAN'))")
                 .trimmed();
  if (transMin == "None") {
    m_uiForm.transFit_ck_can->setChecked(false);
  } else {
    m_uiForm.transFit_ck_can->setChecked(true);
    m_uiForm.trans_min_can->setText(transMin);
    m_uiForm.trans_max_can->setText(
        runReduceScriptFunction("print("
                                "i.ReductionSingleton().transmission_"
                                "calculator.lambdaMax('CAN'))")
            .trimmed());
  }
  text = runReduceScriptFunction("print("
                                 "i.ReductionSingleton().transmission_"
                                 "calculator.fitMethod('CAN'))")
             .trimmed();
  index = m_uiForm.trans_opt_can->findText(text, Qt::MatchFixedString);
  if (index >= 0) {
    m_uiForm.trans_opt_can->setCurrentIndex(index);
  }
  if (text == "OFF" || text == "None")
    m_uiForm.transFitOnOff_can->setChecked(false);
  else
    m_uiForm.transFitOnOff_can->setChecked(true);

  bool separated =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().transmission_calculator.isSeparate())")
          .trimmed() == "True";

  m_uiForm.trans_selector_opt->setCurrentIndex(separated ? 1 : 0);
}

void SANSRunWindow::handleSlicePushButton() {
  if (!slicingWindow) {
    slicingWindow = new SANSEventSlicing(this);
    connect(slicingWindow, SIGNAL(runAsPythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    //    slicingWindow->setParent(this);
    slicingWindow->initializeLayout();
    slicingWindow->initializeLocalPython();
  }

  slicingWindow->show();
  slicingWindow->raise();
}

/**
 * Slot to open the help page of whichever tab the user is currently viewing.
 */
void SANSRunWindow::openHelpPage() {
  const auto helpPageUrl =
      m_helpPageUrls[static_cast<Tab>(m_uiForm.tabWidget->currentIndex())];
  MantidDesktopServices::openUrl(QUrl(helpPageUrl));
}

// Set the validators for inputs
void SANSRunWindow::setValidators() {
  // Validator policies
  if (!m_mustBeDouble) {
    m_mustBeDouble = new QDoubleValidator(this);
  }

  if (!m_doubleValidatorZeroToMax) {
    m_doubleValidatorZeroToMax = new QDoubleValidator(
        0.0, m_constants.getMaxDoubleValue(), m_constants.getDecimals(), this);
  }

  // Range is [0, max]
  if (!m_intValidatorZeroToMax) {
    m_intValidatorZeroToMax =
        new QIntValidator(0, m_constants.getMaxIntValue(), this);
  }

  // Run Numbers tab

  // ----------- Run Settings Tab---------------------------------
  m_uiForm.gravity_extra_length_line_edit->setValidator(m_mustBeDouble);
  m_uiForm.rad_min->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.rad_max->setValidator(m_mustBeDouble);

  m_uiForm.wav_min->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.wav_max->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.wav_dw->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.r_cut_line_edit->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.w_cut_line_edit->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.q_min->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.q_max->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.q_dq->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.qy_max->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.qy_dqy->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.trans_min->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.trans_max->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.trans_min_can->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.trans_max_can->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.monitor_spec->setValidator(m_intValidatorZeroToMax);
  m_uiForm.trans_monitor->setValidator(m_intValidatorZeroToMax);

  m_uiForm.trans_M3M4_line_edit->setValidator(m_mustBeDouble);
  m_uiForm.trans_radius_line_edit->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.phi_min->setValidator(m_mustBeDouble);
  m_uiForm.phi_max->setValidator(m_mustBeDouble);

  m_uiForm.frontDetRescale->setValidator(m_mustBeDouble);
  m_uiForm.frontDetShift->setValidator(m_mustBeDouble);
  m_uiForm.frontDetQmin->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.frontDetQmax->setValidator(m_doubleValidatorZeroToMax);

  m_uiForm.tof_min->setValidator(m_mustBeDouble);
  m_uiForm.tof_max->setValidator(m_mustBeDouble);
  m_uiForm.scale_factor->setValidator(m_mustBeDouble);

  // ----------- Geometry Tab-----------------------------------
  m_uiForm.rear_beam_x->setValidator(m_mustBeDouble);
  m_uiForm.rear_beam_y->setValidator(m_mustBeDouble);
  m_uiForm.front_beam_x->setValidator(m_mustBeDouble);
  m_uiForm.front_beam_y->setValidator(m_mustBeDouble);

  // Geometry
  m_uiForm.sample_thick->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.sample_height->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.sample_width->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.smpl_offset->setValidator(m_mustBeDouble);

  // Beam Centre Finder
  m_uiForm.beam_rmin->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.beam_rmax->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.toleranceLineEdit->setValidator(m_doubleValidatorZeroToMax);
  m_uiForm.beam_iter->setValidator(m_intValidatorZeroToMax);
}

/**
 * Create a zero-error free workspace clone of a reduced workspace, ie one which
 * has been through either
 * Q1D or Qxy
 * @param originalWorkspaceName :: The name of the original workspace which
 * might contain errors with 0 value.
 * @param clonedWorkspaceName :: The name of cloned workspace which should have
 * its zero erros removed.
 * @returns The name of the cloned workspace
 */
void SANSRunWindow::createZeroErrorFreeClone(QString &originalWorkspaceName,
                                             QString &clonedWorkspaceName) {
  if (workspaceExists(originalWorkspaceName) &&
      isValidWsForRemovingZeroErrors(originalWorkspaceName)) {
    // Run the python script which creates the cloned workspace
    QString pythonCode(
        "print(i.CreateZeroErrorFreeClonedWorkspace(input_workspace_name='");
    pythonCode += originalWorkspaceName + "',";
    pythonCode += " output_workspace_name='" + clonedWorkspaceName + "'))\n";
    pythonCode += "print('" + m_constants.getPythonSuccessKeyword() + "')\n";
    QString result(runPythonCode(pythonCode, false));
    result = result.simplified();
    if (result != m_constants.getPythonSuccessKeyword()) {
      result.replace(m_constants.getPythonSuccessKeyword(), "");
      g_log.warning("Error creating a zerror error free cloned workspace. Will "
                    "save original workspace. More info: " +
                    result.toStdString());
    }
  }
}

/**
 * Destroy a zero-error free workspace clone.
 * @param clonedWorkspaceName :: The name of cloned workspace which should have
 * its zero erros removed.
 */
void SANSRunWindow::deleteZeroErrorFreeClone(QString &clonedWorkspaceName) {
  if (workspaceExists(clonedWorkspaceName)) {
    // Run the python script which destroys the cloned workspace
    QString pythonCode(
        "print(i.DeleteZeroErrorFreeClonedWorkspace(input_workspace_name='");
    pythonCode += clonedWorkspaceName + "'))\n";
    pythonCode += "print('" + m_constants.getPythonSuccessKeyword() + "')\n";
    QString result(runPythonCode(pythonCode, false));
    result = result.simplified();
    if (result != m_constants.getPythonSuccessKeyword()) {
      result.replace(m_constants.getPythonSuccessKeyword(), "");
      g_log.warning(
          "Error deleting a zerror error free cloned workspace. More info: " +
          result.toStdString());
    }
  }
}

/**
 * Check if the workspace can have a zero error correction performed on it
 * @param wsName :: The name of the workspace.
 */
bool SANSRunWindow::isValidWsForRemovingZeroErrors(QString &wsName) {
  QString pythonCode(
      "\nprint(i.IsValidWsForRemovingZeroErrors(input_workspace_name='");
  pythonCode += wsName + "'))";
  pythonCode += "\nprint('" + m_constants.getPythonSuccessKeyword() + "')";
  QString result(runPythonCode(pythonCode, false));
  result = result.simplified();
  bool isValid = true;
  if (result != m_constants.getPythonSuccessKeyword()) {
    result.replace(m_constants.getPythonSuccessKeyword(), "");
    g_log.notice("Not a valid workspace for zero error replacement. Will save "
                 "original workspace. More info: " +
                 result.toStdString());
    isValid = false;
  }
  return isValid;
}

/**
 * Set the M3M4 check box and line edit field logic
 * @param setting :: the checked item
 * @param isNowChecked :: What is the current check-state of the setting?
 */
void SANSRunWindow::setM3M4Logic(TransSettings setting, bool isNowChecked) {
  switch (setting) {
  case TransSettings::M3:
    this->m_uiForm.trans_M4_check_box->setChecked(false);
    // Enable the M3M4 line edit field
    this->m_uiForm.trans_M3M4_line_edit->setEnabled(false);
    break;
  case TransSettings::M4:
    this->m_uiForm.trans_M3_check_box->setChecked(false);
    // Enable the M3M4 line edit field
    this->m_uiForm.trans_M3M4_line_edit->setEnabled(isNowChecked);
    break;
  default:
    return;
  }

  // Disable all ROI, Radius and Mask related options
  setRadiusAndMaskLogic(false);
  setROIAndMaskLogic(false);

  // Uncheck the both Radius and ROI
  this->m_uiForm.trans_radius_check_box->setChecked(false);
  this->m_uiForm.trans_roi_files_checkbox->setChecked(false);
}

/**
 * React to changes of the Up/Down checkbox
 */
void SANSRunWindow::onUpDownCheckboxChanged() {
  auto checked = m_uiForm.up_down_checkbox->isChecked();
  if (m_uiForm.rear_radio->isChecked()) {
    m_uiForm.rear_beam_y->setEnabled(checked);
  } else {
    m_uiForm.front_beam_y->setEnabled(checked);
  }
}

/**
 * React to changes of the Left/Right checkbox
 */
void SANSRunWindow::onLeftRightCheckboxChanged() {
  auto checked = m_uiForm.left_right_checkbox->isChecked();
  if (m_uiForm.rear_radio->isChecked()) {
    m_uiForm.rear_beam_x->setEnabled(checked);
  } else {
    m_uiForm.front_beam_x->setEnabled(checked);
  }
}
/**
 * Set beam stop logic for Radius, ROI and Mask
 * @param setting :: the checked item
 * @param isNowChecked :: What is the current check-state of the setting?
 */
void SANSRunWindow::setBeamStopLogic(TransSettings setting, bool isNowChecked) {
  if (setting == TransSettings::RADIUS) {
    setRadiusAndMaskLogic(isNowChecked);
    // If we are turning off the radius checkbox and have then ROI checkbox
    // enabled, then we don' want to turn off the mask
    if (this->m_uiForm.trans_roi_files_checkbox->isChecked() && !isNowChecked) {
      this->m_uiForm.trans_masking_line_edit->setEnabled(true);
    }
  } else if (setting == TransSettings::ROI) {
    setROIAndMaskLogic(isNowChecked);
    // If we are turning off the radius checkbox and have then ROI checkbox
    // enabled, then we don' want to turn off the mask
    if (this->m_uiForm.trans_radius_check_box->isChecked() && !isNowChecked) {
      this->m_uiForm.trans_masking_line_edit->setEnabled(true);
    }
  } else {
    return;
  }

  // Disable the M3M4 line edit field and uncheck the M3 and M4 box
  if (isNowChecked) {
    this->m_uiForm.trans_M3M4_line_edit->setEnabled(false);
    this->m_uiForm.trans_M3_check_box->setChecked(false);
    this->m_uiForm.trans_M4_check_box->setChecked(false);
  }
}

/**
 * Reads the transmission settings from the user file and sets it in the GUI
 */
void SANSRunWindow::setTransmissionSettingsFromUserFile() {
  // Reset all trans-related fields
  resetAllTransFields();

  // Read the Radius settings
  QString transmissionRadiusRequest("\nprint(i.GetTransmissionRadiusInMM())");
  QString resultTransmissionRadius(
      runPythonCode(transmissionRadiusRequest, false));
  resultTransmissionRadius = resultTransmissionRadius.simplified();
  if (resultTransmissionRadius != m_constants.getPythonEmptyKeyword()) {
    this->m_uiForm.trans_radius_line_edit->setText(resultTransmissionRadius);
    this->m_uiForm.trans_radius_check_box->setChecked(true);
    setBeamStopLogic(TransSettings::RADIUS, true);
  }

  // Read the ROI settings
  QString transmissionROIRequest("\nprint(i.GetTransmissionROI())");
  QString resultTransmissionROI(runPythonCode(transmissionROIRequest, false));
  resultTransmissionROI = resultTransmissionROI.simplified();
  if (resultTransmissionROI != m_constants.getPythonEmptyKeyword()) {
    resultTransmissionROI =
        runPythonCode("\nprint(i.ConvertFromPythonStringList(to_convert=" +
                          resultTransmissionROI + "))",
                      false);
    this->m_uiForm.trans_roi_files_line_edit->setText(resultTransmissionROI);
    this->m_uiForm.trans_roi_files_checkbox->setChecked(true);
    setBeamStopLogic(TransSettings::ROI, true);
  }

  // Read the MASK settings
  QString transmissionMaskRequest("\nprint(i.GetTransmissionMask())");
  QString resultTransmissionMask(runPythonCode(transmissionMaskRequest, false));
  resultTransmissionMask = resultTransmissionMask.simplified();
  if (resultTransmissionMask != m_constants.getPythonEmptyKeyword()) {
    resultTransmissionMask =
        runPythonCode("\nprint(i.ConvertFromPythonStringList(to_convert=" +
                          resultTransmissionMask + "))",
                      false);
    this->m_uiForm.trans_masking_line_edit->setText(resultTransmissionMask);
  }

  // Read the Transmission Monitor Spectrum Shift
  QString transmissionMonitorSpectrumShiftRequest(
      "\nprint(i.GetTransmissionMonitorSpectrumShift())");
  QString resultTransmissionMonitorSpectrumShift(
      runPythonCode(transmissionMonitorSpectrumShiftRequest, false));
  resultTransmissionMonitorSpectrumShift =
      resultTransmissionMonitorSpectrumShift.simplified();
  if (resultTransmissionMonitorSpectrumShift !=
      m_constants.getPythonEmptyKeyword()) {
    this->m_uiForm.trans_M3M4_line_edit->setText(
        resultTransmissionMonitorSpectrumShift);
  }

  // Read Transmission Monitor Spectrum, we expect either 3 or 4. If this is
  // selected, then this takes precedence over
  // the radius, roi and mask settings
  QString transmissionMonitorSpectrumRequest(
      "\nprint(i.GetTransmissionMonitorSpectrum())");
  QString resultTransmissionMonitorSpectrum(
      runPythonCode(transmissionMonitorSpectrumRequest, false));
  resultTransmissionMonitorSpectrum =
      resultTransmissionMonitorSpectrum.simplified();
  if (resultTransmissionMonitorSpectrum !=
      m_constants.getPythonEmptyKeyword()) {
    if (resultTransmissionMonitorSpectrum == "3") {
      this->m_uiForm.trans_M3_check_box->setChecked(true);
      setM3M4Logic(TransSettings::M3, true);
    } else if (resultTransmissionMonitorSpectrum == "4") {
      this->m_uiForm.trans_M4_check_box->setChecked(true);
      setM3M4Logic(TransSettings::M4, true);
    } else {
      this->m_uiForm.trans_M3_check_box->setChecked(false);
      this->m_uiForm.trans_M4_check_box->setChecked(false);
      setM3M4Logic(TransSettings::M3, false);
      setM3M4Logic(TransSettings::M4, false);
      g_log.notice("No transmission monitor, transmission radius nor "
                   "trasmission ROI was set. The reducer will use the default "
                   "value.");
    }
  }

  // In case we don't have anything, have M3 checked.
  // This has appeared in LOQ.
  resetToM3IfNecessary();
}

/**
 * Initialize the transmission settings. We are setting up checkboxes
 * and want to make use of the clicked signal in order to distinguish
 * between user-induced and programmatic changes to the checkbox.
 */
void SANSRunWindow::initTransmissionSettings() {
  QObject::connect(m_uiForm.trans_M3_check_box, SIGNAL(clicked()), this,
                   SLOT(onTransmissionM3CheckboxChanged()));
  QObject::connect(m_uiForm.trans_M4_check_box, SIGNAL(clicked()), this,
                   SLOT(onTransmissionM4CheckboxChanged()));
  QObject::connect(m_uiForm.trans_radius_check_box, SIGNAL(clicked()), this,
                   SLOT(onTransmissionRadiusCheckboxChanged()));
  QObject::connect(m_uiForm.trans_roi_files_checkbox, SIGNAL(clicked()), this,
                   SLOT(onTransmissionROIFilesCheckboxChanged()));

  // Set the Tooltips
  const QString m3CB = "Selects the monitor spectrum 3\n"
                       "for the transmission calculation.";
  const QString m4CB = "Selects the monitor spectrum 4\n"
                       "for the transmission calculation.";
  const QString shift = "Sets the shift of the selected monitor in mm. This "
                        "shift is only applicable to M4";
  const QString radiusCB = "Selects a radius when using the beam stop\n"
                           "for the transmission calculation.";
  const QString radius =
      "Sets a radius in mm when using the beam stop out method\n"
      "for the transmission calculation.";
  const QString roiCB = "Selects a comma-separated list of ROI files\n"
                        "when using the beam stop out method for the\n"
                        "transmission calculation.";
  const QString roi = "Sets a comma-separated list of ROI files\n"
                      "when using the beam stop out method for the\n"
                      "transmission calculation.";
  const QString mask = "Sets a comma-separated list of Mask files\n"
                       "when using the beam stop out method for the\n"
                       "transmission calculation.";

  m_uiForm.trans_M3_check_box->setToolTip(m3CB);
  m_uiForm.trans_M4_check_box->setToolTip(m4CB);
  m_uiForm.trans_M3M4_line_edit->setToolTip(shift);
  m_uiForm.trans_radius_check_box->setToolTip(radiusCB);
  m_uiForm.trans_radius_line_edit->setToolTip(radius);
  m_uiForm.trans_roi_files_checkbox->setToolTip(roiCB);
  m_uiForm.trans_roi_files_line_edit->setToolTip(roi);
  m_uiForm.trans_masking_line_edit->setToolTip(mask);
}

/**
 * React to a change of the M3 transmission monitor spectrum checkbox
 */
void SANSRunWindow::onTransmissionM3CheckboxChanged() {
  setM3M4Logic(TransSettings::M3,
               this->m_uiForm.trans_M3_check_box->isChecked());
}

/**
 * React to a change of the M3 transmission monitor spectrum checkbox
 */
void SANSRunWindow::onTransmissionM4CheckboxChanged() {
  setM3M4Logic(TransSettings::M4,
               this->m_uiForm.trans_M4_check_box->isChecked());
}

/**
 * React to the change of the Radius checkbox
 */
void SANSRunWindow::onTransmissionRadiusCheckboxChanged() {
  setBeamStopLogic(TransSettings::RADIUS,
                   this->m_uiForm.trans_radius_check_box->isChecked());
}

/**
 * React to the change of the ROI file checkbox
 */
void SANSRunWindow::onTransmissionROIFilesCheckboxChanged() {
  setBeamStopLogic(TransSettings::ROI,
                   this->m_uiForm.trans_roi_files_checkbox->isChecked());
}

/**
 * Set the radius and the mask logic
 * @param isNowChecked :: The check state
 */
void SANSRunWindow::setRadiusAndMaskLogic(bool isNowChecked) {
  this->m_uiForm.trans_masking_line_edit->setEnabled(isNowChecked);
  this->m_uiForm.trans_radius_line_edit->setEnabled(isNowChecked);

  resetToM3IfNecessary();
}

/**
 * Set the ROI and the mask logic
 * @param isNowChecked :: The check state
 */
void SANSRunWindow::setROIAndMaskLogic(bool isNowChecked) {
  this->m_uiForm.trans_masking_line_edit->setEnabled(isNowChecked);
  this->m_uiForm.trans_roi_files_line_edit->setEnabled(isNowChecked);

  resetToM3IfNecessary();
}

/**
 * Write the transmission settings to a python code string. If there
 * is a transmission monitor set use it, otherwise check if there is
 * a radius or a ROI being set.
 * @param pythonCode :: The python code string
 */
void SANSRunWindow::writeTransmissionSettingsToPythonScript(
    QString &pythonCode) {
  auto m3 = m_uiForm.trans_M3_check_box->isChecked();
  auto m4 = m_uiForm.trans_M4_check_box->isChecked();

  if (m3 || m4) {
    // Handle M3/M4 settings and the TRANSPEC
    auto spectrum = m3 ? 3 : 4;
    pythonCode += "i.SetTransmissionMonitorSpectrum(trans_mon=" +
                  QString::number(spectrum) + ")\n";

    auto transSpec = m_uiForm.trans_M3M4_line_edit->text();
    if (!transSpec.isEmpty()) {
      pythonCode += "i.SetTransmissionMonitorSpectrumShift(trans_mon_shift=" +
                    transSpec + ")\n";
    }
  } else {
    // Handle Radius
    auto radius = m_uiForm.trans_radius_line_edit->text();
    if (m_uiForm.trans_radius_check_box->isChecked() && !radius.isEmpty()) {
      pythonCode +=
          "i.SetTransmissionRadiusInMM(trans_radius=" + radius + ")\n";
    }
    // Handle ROI
    auto roi = m_uiForm.trans_roi_files_line_edit->text();
    if (m_uiForm.trans_roi_files_checkbox->isChecked() && !roi.isEmpty()) {
      roi = "'" + roi.simplified() + "'";
      roi = runPythonCode(
          "\nprint(i.ConvertToPythonStringList(to_convert=" + roi + "))",
          false);
      pythonCode += "i.SetTransmissionROI(trans_roi_files=" + roi + ")\n";
    }
    // Handle Mask
    auto mask = m_uiForm.trans_masking_line_edit->text();
    if (!mask.isEmpty()) {
      mask = "'" + mask.simplified() + "'";
      mask = runPythonCode(
          "\nprint(i.ConvertToPythonStringList(to_convert=" + mask + "))",
          false);
      pythonCode += "i.SetTransmissionMask(trans_mask_files=" + mask + ")\n";
    }

    // Unset a potential monitor setting which had been set by the user file.
    pythonCode += "i.UnsetTransmissionMonitorSpectrum()\n";
  }
}

/**
 * Set the enabled state for all trans-related fields
 */
void SANSRunWindow::resetAllTransFields() {
  bool state = false;
  m_uiForm.trans_radius_line_edit->setEnabled(state);
  m_uiForm.trans_radius_line_edit->clear();

  m_uiForm.trans_roi_files_line_edit->setEnabled(state);
  m_uiForm.trans_roi_files_line_edit->clear();

  m_uiForm.trans_masking_line_edit->setEnabled(state);
  m_uiForm.trans_masking_line_edit->clear();

  m_uiForm.trans_M3M4_line_edit->setEnabled(state);
  m_uiForm.trans_M3M4_line_edit->clear();

  m_uiForm.trans_M3_check_box->setChecked(state);
  m_uiForm.trans_M4_check_box->setChecked(state);
  m_uiForm.trans_roi_files_checkbox->setChecked(state);
  m_uiForm.trans_radius_check_box->setChecked(state);
}

/**
 * Enable the M3 checkbox if M3, M4, Radius and ROI are disabled.
 * We need to select one.
 */
void SANSRunWindow::resetToM3IfNecessary() {
  const auto isM3Disabled = !m_uiForm.trans_M3_check_box->isChecked();
  const auto isM4Disabled = !m_uiForm.trans_M4_check_box->isChecked();
  const auto isROIDisabled = !m_uiForm.trans_roi_files_checkbox->isChecked();
  const auto isRadiusDisabled = !m_uiForm.trans_radius_check_box->isChecked();

  if (isM3Disabled && isM4Disabled && isROIDisabled && isRadiusDisabled) {
    m_uiForm.trans_M3_check_box->setChecked(true);
  }
}

/**
 * Check tha the Settings are valid. We need to do this for inputs which cannot
 * be checked with simple validators
 */
bool SANSRunWindow::areSettingsValid(States type) {
  bool isValid = true;
  QString message;
  // ------------ GUI INPUT CHECKS ------------

  // We currently do not allow a 2D reduction with a merged flag and fitting
  // because we can only fit 1D functions
  auto isMergedReduction = m_uiForm.detbank_sel->currentIndex() == 3;
  auto hasFitEnabled = m_uiForm.frontDetShiftCB->isChecked() ||
                       m_uiForm.frontDetRescaleCB->isChecked();
  if (type == States::TwoD && isMergedReduction && hasFitEnabled) {
    isValid = false;
    message +=
        "A merged reduction with fitting is currently not supported for 2D "
        "reductions. You can run a merged reduction wihthout fitting enabled"
        " for 2D reductions.\n";
  }

  // R_MAX -- can be only >0 or -1
  auto r_max = m_uiForm.rad_max->text().simplified().toDouble();
  if ((r_max < 0.0) && (r_max != -1)) {
    isValid = false;
    message += "R_max issue: Only values >= 0 and -1 are allowed.\n";
  }

  // WAVELENGTH
  checkWaveLengthAndQValues(isValid, message, m_uiForm.wav_min,
                            m_uiForm.wav_max, m_uiForm.wav_dw_opt,
                            "Wavelength");

  // QX
  checkWaveLengthAndQValues(isValid, message, m_uiForm.q_min, m_uiForm.q_max,
                            m_uiForm.q_dq_opt, "Qx");

  // TRANS SAMPLE
  if (m_uiForm.transFit_ck->isChecked())
    checkWaveLengthAndQValues(isValid, message, m_uiForm.trans_min,
                              m_uiForm.trans_max, m_uiForm.trans_opt, "Trans");

  // TRANS CAN
  if (m_uiForm.trans_selector_opt->currentText().toUpper().contains(
          "SEPARATE")) {
    checkWaveLengthAndQValues(isValid, message, m_uiForm.trans_min_can,
                              m_uiForm.trans_max_can, m_uiForm.trans_opt_can,
                              "Trans Can");
  }

  // Geometry
  if (m_uiForm.sample_thick->text().simplified().toDouble() == 0.0) {
    isValid = false;
    message += "Sample height issue: Only values > 0 are allowed.\n";
  }

  if (m_uiForm.sample_height->text().simplified().toDouble() == 0.0) {
    isValid = false;
    message += "Sample height issue: Only values > 0 are allowed.\n";
  }

  if (m_uiForm.sample_width->text().simplified().toDouble() == 0.0) {
    isValid = false;
    message += "Sample width issue: Only values > 0 are allowed.\n";
  }

  // Check save format consistency for batch mode reduction
  // 2D --> cannot be CanSAS
  auto isBatchMode = !m_uiForm.single_mode_btn->isChecked();
  if (isBatchMode) {
    auto is1D = type == OneD;
    auto isCanSAS = m_uiForm.saveCan_check->isChecked();
    QString saveMessage;
    auto isValidSaveOption = checkSaveOptions(saveMessage, is1D, isCanSAS);
    if (!isValidSaveOption) {
      isValid = false;
      message += saveMessage;
    }
  }

  // Print the error message if there are any
  if (!message.isEmpty()) {
    QString warning = "Please correct these settings before proceeding:\n";
    warning += message;
    QMessageBox::warning(this, "Inconsistent input", warning);
  }

  return isValid;
}

/**
 * Check the wavelength and Q values
 * @param isValid: flag by reference to set the check if invalid
 * @param message: the message to display
 * @param min: the min line edit field
 * @param max: the max line edit field
 * @param selection: the combo box which is being querried
 * @param type: message type
 */
void SANSRunWindow::checkWaveLengthAndQValues(bool &isValid, QString &message,
                                              QLineEdit *min, QLineEdit *max,
                                              QComboBox *selection,
                                              QString type) {
  auto min_value = min->text().simplified().toDouble();
  auto max_value = max->text().simplified().toDouble();

  // Make sure that min<=max
  if (min_value > max_value) {
    isValid = false;
    message += type;
    message += " issue: The min value is larger than the max value. \n";
  }

  // Make sure that when selecting log, then we don't have 0 values
  if (selection->currentText().toUpper().contains("LOG") &&
      (min_value == 0.0 || max_value == 0.0)) {
    isValid = false;
    message += type;
    message += " issue: Trying to use Logarithmic steps and values which are "
               "<= 0.0. \n";
  }
}

/**
 *  Update the beam centre coordinates
 */
void SANSRunWindow::updateBeamCenterCoordinates() {
  // Centre coordinates
  // from the ticket #5942 both detectors have center coordinates
  double dbl_param =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().get_beam_center('rear')[0])")
          .toDouble();
  // get the scale factor1 for the beam centre to scale it correctly
  double dbl_paramsf =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().get_beam_center_scale_factor1())")
          .toDouble();
  m_uiForm.rear_beam_x->setText(QString::number(dbl_param * dbl_paramsf));
  // get scale factor2 for the beam centre to scale it correctly
  dbl_paramsf =
      runReduceScriptFunction(
          "print(i.ReductionSingleton().get_beam_center_scale_factor2())")
          .toDouble();
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('rear')[1])")
                  .toDouble();
  m_uiForm.rear_beam_y->setText(QString::number(dbl_param * dbl_paramsf));
  // front
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('front')[0])")
                  .toDouble();
  m_uiForm.front_beam_x->setText(QString::number(dbl_param * 1000.0));
  dbl_param = runReduceScriptFunction(
                  "print(i.ReductionSingleton().get_beam_center('front')[1])")
                  .toDouble();
  m_uiForm.front_beam_y->setText(QString::number(dbl_param * 1000.0));
}

/**
 * Set the beam finder details
 */
void SANSRunWindow::setBeamFinderDetails() {
  // The instrument name
  auto instrumentName = m_uiForm.inst_opt->currentText();

  // Set the labels according to the instrument
  auto requiresAngle = runReduceScriptFunction(
                           "print(i.is_current_workspace_an_angle_workspace())")
                           .simplified();
  QString labelPosition;
  if (requiresAngle == m_constants.getPythonTrueKeyword()) {
    labelPosition = "Current ( " + QString(QChar(0x03B2)) + " , y ) [";
    labelPosition.append(QChar(0xb0));
    labelPosition += ",mm]";
  } else {
    labelPosition = "Current ( x , y ) [mm,mm]";
  }
  m_uiForm.beam_centre_finder_groupbox->setTitle(labelPosition);
}

/**
 * Retrieves the Q resolution settings and apply them to the GUI
 */
void SANSRunWindow::retrieveQResolutionSettings() {
  // Set if the QResolution should be used at all
  QString getUseage = "i.get_q_resultution_use()\n";
  QString resultUsage(runPythonCode(getUseage, false));
  resultUsage = resultUsage.simplified();
  if (resultUsage == m_constants.getPythonTrueKeyword()) {
    m_uiForm.q_resolution_group_box->setChecked(true);
  } else if (resultUsage == m_constants.getPythonFalseKeyword()) {
    m_uiForm.q_resolution_group_box->setChecked(false);
  } else {
    g_log.warning(resultUsage.toStdString());
    g_log.warning("Not a valid setting for the useage of QResolution");
    m_uiForm.q_resolution_group_box->setChecked(false);
  }

  // Set the Collimation length
  auto resultCollimationLength =
      retrieveQResolutionGeometry("i.get_q_resolution_collimation_length()\n");
  m_uiForm.q_resolution_collimation_length_input->setText(
      resultCollimationLength);

  // Set the Delta R value
  auto resultDeltaR =
      retrieveQResolutionGeometry("i.get_q_resolution_delta_r()\n");
  m_uiForm.q_resolution_delta_r_input->setText(resultDeltaR);

  // Set the moderator file
  QString getModeratorFile = "i.get_q_resolution_moderator()\n";
  QString resultModeratorFile = runPythonCode(getModeratorFile, false);
  if (resultModeratorFile == m_constants.getPythonEmptyKeyword()) {
    resultModeratorFile = "";
  }
  m_uiForm.q_resolution_moderator_input->setText(resultModeratorFile);

  // Set the geometry, ie if rectangular or circular aperture
  retrieveQResolutionAperture();
}

/**
 * Retrieve the QResolution setting for the aperture. Select the aperture
 * type depending on the available values, ie if there are H1, W1, H2, W2
 * specified,
 * then we are dealing with are rectangular aperture, else with a circular
 */
void SANSRunWindow::retrieveQResolutionAperture() {
  // Get the H1, W1, H2, W2
  auto h1 = retrieveQResolutionGeometry("i.get_q_resolution_h1()\n");
  auto w1 = retrieveQResolutionGeometry("i.get_q_resolution_w1()\n");
  auto h2 = retrieveQResolutionGeometry("i.get_q_resolution_h2()\n");
  auto w2 = retrieveQResolutionGeometry("i.get_q_resolution_w2()\n");

  // If at least one of them is empty, then use circular, otherwise use
  // rectangular
  auto useCircular =
      h1.isEmpty() || w1.isEmpty() || h2.isEmpty() || w2.isEmpty();
  if (useCircular) {
    setupQResolutionCircularAperture();
  } else {
    setupQResolutionRectangularAperture(h1, w1, h2, w2);
  }
}

/**
 * Gets the geometry settings and checks if they are empty or not
 * @param command: the python command to execute
 * @returns either a length (string) in mm or an empty string
 */
QString SANSRunWindow::retrieveQResolutionGeometry(QString command) {
  QString result(runPythonCode(command, false));
  result = result.simplified();
  if (result == m_constants.getPythonEmptyKeyword()) {
    result = "";
  }
  return result;
}

/**
 * Setup the GUI for use with a circular aperture
 */
void SANSRunWindow::setupQResolutionCircularAperture() {
  // Get the apertures of the diameter
  auto a1 = retrieveQResolutionGeometry("i.get_q_resolution_a1()\n");
  auto a2 = retrieveQResolutionGeometry("i.get_q_resolution_a2()\n");

  setQResolutionApertureType(QResoluationAperture::CIRCULAR, "A1 [mm]",
                             "A2 [mm]", a1, a2,
                             m_constants.getQResolutionA1ToolTipText(),
                             m_constants.getQResolutionA2ToolTipText(), true);
}

/**
 * Setup the GUI for use with a rectangular aperture
 * @param h1: the height of the first aperture
 * @param w1: the width of the first aperture
 * @param h2: the height of the second aperture
 * @param w2: the width of the second aperture
 */
void SANSRunWindow::setupQResolutionRectangularAperture(QString h1, QString w1,
                                                        QString h2,
                                                        QString w2) {
  // Set the QResolution Aperture
  setQResolutionApertureType(QResoluationAperture::RECTANGULAR, "H1 [mm]",
                             "H2 [mm]", h1, h2,
                             m_constants.getQResolutionH1ToolTipText(),
                             m_constants.getQResolutionH2ToolTipText(), false);

  // Set the W1 and W2 values
  m_uiForm.q_resolution_w1_input->setText(w1);
  m_uiForm.q_resolution_w2_input->setText(w2);

  // Set the ToolTip for a1
  m_uiForm.q_resolution_a1_h1_input->setToolTip(
      m_constants.getQResolutionH1ToolTipText());
  m_uiForm.q_resolution_a1_h1_label->setToolTip(
      m_constants.getQResolutionH1ToolTipText());

  // Set the ToolTip for a2
  m_uiForm.q_resolution_a2_h2_input->setToolTip(
      m_constants.getQResolutionH2ToolTipText());
  m_uiForm.q_resolution_a2_h2_label->setToolTip(
      m_constants.getQResolutionH2ToolTipText());
}

/**
 * Setup the GUI for use iwth a rectangular aperture
 */
void SANSRunWindow::setupQResolutionRectangularAperture() {
  auto h1 = retrieveQResolutionGeometry("i.get_q_resolution_h1()\n");
  auto w1 = retrieveQResolutionGeometry("i.get_q_resolution_w1()\n");
  auto h2 = retrieveQResolutionGeometry("i.get_q_resolution_h2()\n");
  auto w2 = retrieveQResolutionGeometry("i.get_q_resolution_w2()\n");

  setupQResolutionRectangularAperture(h1, w1, h2, w2);
}

/**
 * Set the QResolution aperture GUI
 * @param apertureType: the type of the aperture
 * @param a1H1Label: the label for the a1/h1 input
 * @param a2H2Label: the label for the a2/h2 input
 * @param a1H1: the a1H1 value
 * @param a2H2: the a2H2 value
 * @param toolTipA1H1: the tooltip text for the first aperture parameter
 * @param toolTipA2H2: the tooltip text for the second aperture parameter
 * @param w1W2Disabled: if the w1W2Inputs should be disabled
 */
void SANSRunWindow::setQResolutionApertureType(
    QResoluationAperture apertureType, QString a1H1Label, QString a2H2Label,
    QString a1H1, QString a2H2, QString toolTipA1H1, QString toolTipA2H2,
    bool w1W2Disabled) {
  // Set the labels
  m_uiForm.q_resolution_a1_h1_label->setText(a1H1Label);
  m_uiForm.q_resolution_a2_h2_label->setText(a2H2Label);

  // Set the values
  m_uiForm.q_resolution_a1_h1_input->setText(a1H1);
  m_uiForm.q_resolution_a2_h2_input->setText(a2H2);

  // Ensure that the W1 and W2 boxes are not accesible
  m_uiForm.q_resolution_w1_label->setDisabled(w1W2Disabled);
  m_uiForm.q_resolution_w2_label->setDisabled(w1W2Disabled);
  m_uiForm.q_resolution_w1_input->setDisabled(w1W2Disabled);
  m_uiForm.q_resolution_w2_input->setDisabled(w1W2Disabled);

  // Set the QCheckBox to the correct value
  m_uiForm.q_resolution_combo_box->setCurrentIndex(apertureType);

  // Set the ToolTip for a1/a2
  m_uiForm.q_resolution_a1_h1_input->setToolTip(toolTipA1H1);
  m_uiForm.q_resolution_a1_h1_label->setToolTip(toolTipA1H1);

  // Set the ToolTip for a2
  m_uiForm.q_resolution_a2_h2_input->setToolTip(toolTipA2H2);
  m_uiForm.q_resolution_a2_h2_label->setToolTip(toolTipA2H2);
}

/**
 * Write the GUI changes for the QResolution settings to the python code string
 * @param pythonCode: A reference to the python code
 */
void SANSRunWindow::writeQResolutionSettingsToPythonScript(
    QString &pythonCode) {
  // Clear the current settings
  pythonCode += "i.reset_q_resolution_settings()\n";
  const QString lineEnding1 = ")\n";
  const QString lineEnding2 = "')\n";
  // Set usage of QResolution
  auto usageGUI = m_uiForm.q_resolution_group_box->isChecked();
  QString useage = usageGUI ? m_constants.getPythonTrueKeyword()
                            : m_constants.getPythonFalseKeyword();
  pythonCode += "i.set_q_resolution_use(use=" + useage + ")\n";

  // Set collimation length
  auto collimationLength =
      m_uiForm.q_resolution_collimation_length_input->text().simplified();
  writeQResolutionSettingsToPythonScriptSingleEntry(
      collimationLength,
      "i.set_q_resolution_collimation_length(collimation_length=", lineEnding1,
      pythonCode);
  // Set the moderator file
  auto moderatorFile =
      m_uiForm.q_resolution_moderator_input->text().simplified();
  writeQResolutionSettingsToPythonScriptSingleEntry(
      moderatorFile, "i.set_q_resolution_moderator(file_name='", lineEnding2,
      pythonCode);
  // Set the delta r value
  auto deltaR = m_uiForm.q_resolution_delta_r_input->text().simplified();
  writeQResolutionSettingsToPythonScriptSingleEntry(
      deltaR, "i.set_q_resolution_delta_r(delta_r=", lineEnding1, pythonCode);
  // Set the aperture properties depending on the aperture type
  auto a1H1 = m_uiForm.q_resolution_a1_h1_input->text().simplified();
  auto a2H2 = m_uiForm.q_resolution_a2_h2_input->text().simplified();
  if (m_uiForm.q_resolution_combo_box->currentIndex() ==
      QResoluationAperture::CIRCULAR) {
    writeQResolutionSettingsToPythonScriptSingleEntry(
        a1H1, "i.set_q_resolution_a1(a1=", lineEnding1, pythonCode);
    writeQResolutionSettingsToPythonScriptSingleEntry(
        a2H2, "i.set_q_resolution_a2(a2=", lineEnding1, pythonCode);
  } else if (m_uiForm.q_resolution_combo_box->currentIndex() ==
             QResoluationAperture::RECTANGULAR) {
    writeQResolutionSettingsToPythonScriptSingleEntry(
        a1H1, "i.set_q_resolution_h1(h1=", lineEnding1, pythonCode);
    writeQResolutionSettingsToPythonScriptSingleEntry(
        a2H2, "i.set_q_resolution_h2(h2=", lineEnding1, pythonCode);
    // Set the W1 and W2 parameters
    auto w1 = m_uiForm.q_resolution_w1_input->text().simplified();
    writeQResolutionSettingsToPythonScriptSingleEntry(
        w1, "i.set_q_resolution_w1(w1=", lineEnding1, pythonCode);
    auto w2 = m_uiForm.q_resolution_w2_input->text().simplified();
    writeQResolutionSettingsToPythonScriptSingleEntry(
        w2, "i.set_q_resolution_w2(w2=", lineEnding1, pythonCode);
  } else {
    g_log.error("SANSRunWindow: Tried to select a QResolution aperture which "
                "does not seem to exist");
  }
}

/**
 * Write a single line of python code for Q Resolution
 * @param value: The value to set
 * @param code_entry: tye python method to run
 * @param lineEnding: the line ending
 * @param py_code: the code segment to which we want to append
 */
void SANSRunWindow::writeQResolutionSettingsToPythonScriptSingleEntry(
    QString value, QString code_entry, const QString lineEnding,
    QString &py_code) const {
  if (!value.isEmpty()) {
    py_code += code_entry + value + lineEnding;
  }
}

/**
 * Handle a chagne of the QResolution aperture selection
 * @param aperture: the current index
 */
void SANSRunWindow::handleQResolutionApertureChange(int aperture) {
  if (aperture == QResoluationAperture::CIRCULAR) {
    setupQResolutionCircularAperture();
  } else if (aperture == QResoluationAperture::RECTANGULAR) {
    setupQResolutionRectangularAperture();
  } else {
    g_log.error("SANSRunWindow: Tried to select a QResolution aperture which "
                "does not seem to exist");
  }
}

/**
 * Initialize the QResolution settings
 */
void SANSRunWindow::initQResolutionSettings() {
  // Connect the change of the change of the aperture
  QObject::connect(m_uiForm.q_resolution_combo_box,
                   SIGNAL(currentIndexChanged(int)), this,
                   SLOT(handleQResolutionApertureChange(int)));

  // Set the Tooltips for Moderator
  const QString moderator("The full path to the moderator file.");
  m_uiForm.q_resolution_moderator_input->setToolTip(moderator);
  m_uiForm.q_resolution_moderator_label->setToolTip(moderator);

  // Set the ToolTip for the Collimation length
  const QString collimationLength("The collimation length in m.");
  m_uiForm.q_resolution_collimation_length_input->setToolTip(collimationLength);
  m_uiForm.q_resolution_collimation_length_label->setToolTip(collimationLength);

  // Set the ToolTip for Delta R
  const QString deltaR("The delta r in mm.");
  m_uiForm.q_resolution_delta_r_input->setToolTip(deltaR);
  m_uiForm.q_resolution_delta_r_label->setToolTip(deltaR);

  // Set the ToolTip for w1
  const QString w1("The width of the first aperture in mm.");
  m_uiForm.q_resolution_w1_input->setToolTip(w1);
  m_uiForm.q_resolution_w1_label->setToolTip(w1);

  // Set the ToolTip for w2
  const QString w2("The width of the second aperture in mm.");
  m_uiForm.q_resolution_w2_input->setToolTip(w2);
  m_uiForm.q_resolution_w2_label->setToolTip(w2);

  // Set the dropdown menu
  const QString aperture("Select if a circular or rectangular aperture \n"
                         "should be used");
  m_uiForm.q_resolution_combo_box->setToolTip(aperture);

  // Set the ToolTip for a1
  m_uiForm.q_resolution_a1_h1_input->setToolTip(
      m_constants.getQResolutionA1ToolTipText());
  m_uiForm.q_resolution_a1_h1_label->setToolTip(
      m_constants.getQResolutionA1ToolTipText());

  // Set the ToolTip for a2
  m_uiForm.q_resolution_a2_h2_input->setToolTip(
      m_constants.getQResolutionA2ToolTipText());
  m_uiForm.q_resolution_a2_h2_label->setToolTip(
      m_constants.getQResolutionA2ToolTipText());
}

/**
 * Initialize the background corrections, ie reset all fields
 */
void SANSRunWindow::initializeBackgroundCorrection() {
  m_uiForm.sansBackgroundCorrectionWidget->resetEntries();
}

/**
 * Retrieve background correction settings and set them in the UI
 */
void SANSRunWindow::retrieveBackgroundCorrection() {
  // Get all settings from the python side
  auto timeDetector = retrieveBackgroundCorrectionSetting(true, false);
  auto timeMonitor = retrieveBackgroundCorrectionSetting(true, true);
  auto uampDetector = retrieveBackgroundCorrectionSetting(false, false);
  auto uampMonitor = retrieveBackgroundCorrectionSetting(false, true);

  // Apply the settings to the background correction widget
  m_uiForm.sansBackgroundCorrectionWidget->setDarkRunSettingForTimeDetectors(
      timeDetector);
  m_uiForm.sansBackgroundCorrectionWidget->setDarkRunSettingForTimeMonitors(
      timeMonitor);
  m_uiForm.sansBackgroundCorrectionWidget->setDarkRunSettingForUampDetectors(
      uampDetector);
  m_uiForm.sansBackgroundCorrectionWidget->setDarkRunSettingForUampMonitors(
      uampMonitor);
}

/**
 * Get a single background correction setting
 * @param isTime: if is time or uamp
 * @param isMon: if is monitor or detector
 * @returns a settings object
 */
SANSBackgroundCorrectionSettings
SANSRunWindow::retrieveBackgroundCorrectionSetting(bool isTime, bool isMon) {
  std::map<QString, QString> commandMap = {
      {"run_number", ""}, {"is_mean", ""}, {"is_mon", ""}, {"mon_number", ""}};

  auto createPythonScript = [](bool isTime, bool isMon, QString component) {
    return "i.get_background_correction(is_time = " +
           convertBoolToPythonBoolString(isTime) +
           ", is_mon=" + convertBoolToPythonBoolString(isMon) +
           ", component='" + component + "')";
  };

  for (auto &command : commandMap) {
    auto element =
        runPythonCode(createPythonScript(isTime, isMon, command.first));
    element = element.simplified();
    if (element != m_constants.getPythonEmptyKeyword()) {
      command.second = element;
    }
  }

  QString runNumber = commandMap["run_number"];
  bool useMean = convertPythonBoolStringToBool(commandMap["is_mean"]);
  bool useMon = convertPythonBoolStringToBool(commandMap["is_mon"]);
  QString monNumber = commandMap["mon_number"];

  return SANSBackgroundCorrectionSettings(runNumber, useMean, useMon,
                                          monNumber);
}

/**
 * Sends the background correction user setting
 * @param pythonCode: the python code to attaceh the new commands
 */
void SANSRunWindow::writeBackgroundCorrectionToPythonScript(
    QString &pythonCode) {
  // Clear the stored settings. Else we will overwrite settings
  runPythonCode("i.clear_background_correction()");

  // Get the settings
  auto timeDetectors = m_uiForm.sansBackgroundCorrectionWidget
                           ->getDarkRunSettingForTimeDetectors();
  auto timeMonitors = m_uiForm.sansBackgroundCorrectionWidget
                          ->getDarkRunSettingForTimeMonitors();

  auto uampDetectors = m_uiForm.sansBackgroundCorrectionWidget
                           ->getDarkRunSettingForUampDetectors();
  auto uampMonitors = m_uiForm.sansBackgroundCorrectionWidget
                          ->getDarkRunSettingForUampMonitors();

  addBackgroundCorrectionToPythonScript(pythonCode, timeDetectors, true);
  addBackgroundCorrectionToPythonScript(pythonCode, timeMonitors, true);

  addBackgroundCorrectionToPythonScript(pythonCode, uampDetectors, false);
  addBackgroundCorrectionToPythonScript(pythonCode, uampMonitors, false);
}

/**
 * Add specific background correction setting to python script
 * @param pythonCode: the python code to attaceh the new commands
 * @param setting: a background correction settings object
 * @param isTimeBased: flag if it is time-based
 */
void SANSRunWindow::addBackgroundCorrectionToPythonScript(
    QString &pythonCode,
    MantidQt::CustomInterfaces::SANSBackgroundCorrectionSettings setting,
    bool isTimeBased) {

  QString newSetting =
      "i.set_background_correction(run_number='" + setting.getRunNumber() +
      "'," + "is_time_based=" + convertBoolToPythonBoolString(isTimeBased) +
      "," + "is_mon=" + convertBoolToPythonBoolString(setting.getUseMon()) +
      "," + "is_mean=" + convertBoolToPythonBoolString(setting.getUseMean()) +
      "," + "mon_numbers = '" + setting.getMonNumber() + "')\n";

  pythonCode += newSetting;
}

/**
 * Check if the user file has a valid extension
 */
bool SANSRunWindow::hasUserFileValidFileExtension() {
  auto userFile = m_uiForm.userfile_edit->text().trimmed();
  QString checkValidity =
      "i.has_user_file_valid_extension('" + userFile + "')\n";

  QString resultCheckValidity(runPythonCode(checkValidity, false));
  resultCheckValidity = resultCheckValidity.simplified();
  auto isValid = false;
  if (resultCheckValidity == m_constants.getPythonTrueKeyword()) {
    isValid = true;
  }

  if (!isValid) {
    QMessageBox::critical(
        this, "User File extension issue",
        "The specified user file does not seem to have a \n"
        "valid file extension. Make sure that the user file \n"
        "has a .txt extension.");
  }

  return isValid;
}

/**
 * Check if the user file is valid.
 @returns false if it is not valid else true
 */
bool SANSRunWindow::isValidUserFile() {
  // Make sure that user file is valid
  if (!hasUserFileValidFileExtension()) {
    m_cfg_loaded = false;
    return false;
  }

  const std::string facility = ConfigService::Instance().getFacility().name();
  if (facility != "ISIS") {
    return false;
  }

  QString filetext = m_uiForm.userfile_edit->text().trimmed();
  if (filetext.isEmpty()) {
    QMessageBox::warning(this, "Error loading user file",
                         "No user file has been specified");
    m_cfg_loaded = false;
    return false;
  }

  QFile user_file(filetext);
  if (!user_file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, "Error loading user file",
                          "Could not open user file \"" + filetext + "\"");
    m_cfg_loaded = false;
    return false;
  }
  user_file.close();

  return true;
}

void SANSRunWindow::updateIDFInfo(const QString &command) {
  QString resultIdf(runPythonCode(command, false));
  resultIdf = resultIdf.simplified();
  if (resultIdf != m_constants.getPythonEmptyKeyword() &&
      !resultIdf.isEmpty()) {
    m_uiForm.current_idf_path->setText(resultIdf);
  }
}

void SANSRunWindow::updateIDFFilePathForBatch() {

  if (m_uiForm.batch_table->rowCount() == 0) {
    return;
  }
  // We base the IDF entry on the sample scatter entry of the first row
  auto *table_item = m_uiForm.batch_table->item(0, 0);
  auto scatter_sample_run = table_item->text();
  QString getIdf = "i.get_idf_path_for_run(\"" + scatter_sample_run + "\")\n";
  updateIDFInfo(getIdf);
}

void SANSRunWindow::updateIDFFilePath() {
  QString getIdf = "i.get_current_idf_path_in_reducer()\n";
  updateIDFInfo(getIdf);
}

void SANSRunWindow::onUpdateGeometryRequest() {
  auto sampleWidth = m_uiForm.sample_width->text();
  auto sampleHeight = m_uiForm.sample_height->text();
  auto sampleThickness = m_uiForm.sample_thick->text();
  auto geometryID = m_uiForm.sample_geomid->currentText();
  auto geometryName = geometryID.mid(3);

  emit sendGeometryInformation(geometryName, sampleHeight, sampleWidth,
                               sampleThickness);
}

} // namespace CustomInterfaces
} // namespace MantidQt
