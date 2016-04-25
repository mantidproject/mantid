#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionViewQtGUI.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "Poco/DirectoryIterator.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <array>
#include <fstream>
#include <random>

#include <boost/lexical_cast.hpp>
#include <Poco/Path.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <qwt_symbol.h>

namespace MantidQt {
namespace CustomInterfaces {

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(EnggDiffractionViewQtGUI)
const double EnggDiffractionViewQtGUI::g_defaultRebinWidth = -0.0005;
int EnggDiffractionViewQtGUI::m_currentType = 0;
int EnggDiffractionViewQtGUI::m_currentRunMode = 0;
int EnggDiffractionViewQtGUI::m_currentCropCalibBankName = 0;
int EnggDiffractionViewQtGUI::m_fitting_bank_Id = 0;
std::vector<std::string> EnggDiffractionViewQtGUI::m_fitting_runno_dir_vec;

const std::string EnggDiffractionViewQtGUI::g_iparmExtStr =
    "GSAS instrument parameters, IPARM file: PRM, PAR, IPAR, IPARAM "
    "(*.prm *.par *.ipar *.iparam);;"
    "Other extensions/all files (*.*)";

const std::string EnggDiffractionViewQtGUI::g_pixelCalibExt =
    "Comma separated values text file with calibration table, CSV"
    "(*.csv);;"
    "Nexus file with calibration table: NXS, NEXUS"
    "(*.nxs *.nexus);;"
    "Supported formats: CSV, NXS "
    "(*.csv *.nxs *.nexus);;"
    "Other extensions/all files (*.*)";

const std::string EnggDiffractionViewQtGUI::g_DetGrpExtStr =
    "Detector Grouping File: CSV "
    "(*.csv *.txt);;"
    "Other extensions/all files (*.*)";

const std::string EnggDiffractionViewQtGUI::m_settingsGroup =
    "CustomInterfaces/EnggDiffractionView";

/**
* Default constructor.
*
* @param parent Parent window (most likely the Mantid main app window).
*/
EnggDiffractionViewQtGUI::EnggDiffractionViewQtGUI(QWidget *parent)
    : UserSubWindow(parent), IEnggDiffractionView(), m_currentInst("ENGINX"),
      m_currentCalibFilename(""), m_focusedDataVector(), m_fittedDataVector(),
      m_peakPicker(NULL), m_zoomTool(NULL), m_presenter(NULL) {}

EnggDiffractionViewQtGUI::~EnggDiffractionViewQtGUI() {
  for (auto curves : m_focusedDataVector) {
    curves->detach();
    delete curves;
  }

  for (auto curves : m_fittedDataVector) {
    curves->detach();
    delete curves;
  }
}

void EnggDiffractionViewQtGUI::initLayout() {
  // setup container ui
  m_ui.setupUi(this);
  // add tab contents and set up their ui's
  QWidget *wCalib = new QWidget(m_ui.tabMain);
  m_uiTabCalib.setupUi(wCalib);
  m_ui.tabMain->addTab(wCalib, QString("Calibration"));

  QWidget *wFocus = new QWidget(m_ui.tabMain);
  m_uiTabFocus.setupUi(wFocus);
  m_ui.tabMain->addTab(wFocus, QString("Focus"));

  QWidget *wPreproc = new QWidget(m_ui.tabMain);
  m_uiTabPreproc.setupUi(wPreproc);
  m_ui.tabMain->addTab(wPreproc, QString("Pre-processing"));

  QWidget *wFitting = new QWidget(m_ui.tabMain);
  m_uiTabFitting.setupUi(wFitting);
  m_ui.tabMain->addTab(wFitting, QString("Fitting"));

  QWidget *wSettings = new QWidget(m_ui.tabMain);
  m_uiTabSettings.setupUi(wSettings);
  m_ui.tabMain->addTab(wSettings, QString("Settings"));

  QComboBox *inst = m_ui.comboBox_instrument;
  m_currentInst = inst->currentText().toStdString();

  setPrefix(m_currentInst);
  readSettings();

  // basic UI setup, connect signals, etc.
  doSetupGeneralWidgets();
  doSetupTabCalib();
  doSetupTabFocus();
  doSetupTabPreproc();
  doSetupTabFitting();
  doSetupTabSettings();

  // presenter that knows how to handle a IEnggDiffractionView should take care
  // of all the logic
  // note that the view needs to know the concrete presenter
  m_presenter.reset(new EnggDiffractionPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IEnggDiffractionPresenter::Start);
  m_presenter->notify(IEnggDiffractionPresenter::RBNumberChange);
}

void EnggDiffractionViewQtGUI::doSetupTabCalib() {
  // Last available runs. This (as well as the empty defaults just
  // above) should probably be made persistent - and encapsulated into a
  // CalibrationParameters or similar class/structure
  const std::string vanadiumRun = "236516";
  const std::string ceriaRun = "241391";
  m_uiTabCalib.lineEdit_new_vanadium_num->setUserInput(
      QString::fromStdString(vanadiumRun));
  m_uiTabCalib.lineEdit_new_ceria_num->setUserInput(
      QString::fromStdString(ceriaRun));
  m_uiTabCalib.lineEdit_cropped_run_num->setUserInput(
      QString::fromStdString(ceriaRun));

  // push button signals/slots
  connect(m_uiTabCalib.pushButton_load_calib, SIGNAL(released()), this,
          SLOT(loadCalibrationClicked()));

  connect(m_uiTabCalib.pushButton_new_calib, SIGNAL(released()), this,
          SLOT(calibrateClicked()));

  connect(m_uiTabCalib.pushButton_new_cropped_calib, SIGNAL(released()), this,
          SLOT(CroppedCalibrateClicked()));

  connect(m_uiTabCalib.comboBox_calib_cropped_bank_name,
          SIGNAL(currentIndexChanged(int)), this,
          SLOT(calibspecNoChanged(int)));

  connect(m_uiTabCalib.lineEdit_new_ceria_num, SIGNAL(fileTextChanged(QString)),
          this, SLOT(updateCroppedCalibRun()));

  connect(m_uiTabCalib.comboBox_calib_cropped_bank_name,
          SIGNAL(currentIndexChanged(int)), this, SLOT(enableSpecNos()));

  enableCalibrateAndFocusActions(true);
}

void EnggDiffractionViewQtGUI::doSetupTabFocus() {

  connect(m_uiTabFocus.pushButton_focus, SIGNAL(released()), this,
          SLOT(focusClicked()));

  connect(m_uiTabFocus.pushButton_focus_cropped, SIGNAL(released()), this,
          SLOT(focusCroppedClicked()));

  connect(m_uiTabFocus.pushButton_texture_browse_grouping_file,
          SIGNAL(released()), this, SLOT(browseTextureDetGroupingFile()));

  connect(m_uiTabFocus.pushButton_focus_texture, SIGNAL(released()), this,
          SLOT(focusTextureClicked()));

  connect(m_uiTabFocus.pushButton_reset, SIGNAL(released()), this,
          SLOT(focusResetClicked()));

  connect(m_uiTabFocus.pushButton_stopFocus, SIGNAL(released()), this,
          SLOT(focusStopClicked()));

  connect(m_uiTabFocus.comboBox_PlotData, SIGNAL(currentIndexChanged(int)),
          this, SLOT(plotRepChanged(int)));

  connect(m_uiTabFocus.comboBox_Multi_Runs, SIGNAL(currentIndexChanged(int)),
          this, SLOT(multiRunModeChanged(int)));

  connect(m_uiTabFocus.checkBox_FocusedWS, SIGNAL(clicked()), this,
          SLOT(plotFocusStatus()));
}

void EnggDiffractionViewQtGUI::doSetupTabPreproc() {
  connect(m_uiTabPreproc.pushButton_rebin_time, SIGNAL(released()), this,
          SLOT(rebinTimeClicked()));

  connect(m_uiTabPreproc.pushButton_rebin_multiperiod, SIGNAL(released()), this,
          SLOT(rebinMultiperiodClicked()));
}

void EnggDiffractionViewQtGUI::doSetupTabFitting() {

  connect(m_uiTabFitting.pushButton_fitting_browse_run_num, SIGNAL(released()),
          this, SLOT(browseFitFocusedRun()));

  connect(m_uiTabFitting.lineEdit_pushButton_run_num, SIGNAL(editingFinished()),
          this, SLOT(fittingRunNoChanged()));

  connect(m_uiTabFitting.lineEdit_pushButton_run_num, SIGNAL(returnPressed()),
          this, SLOT(fittingRunNoChanged()));

  connect(this, SIGNAL(getBanks()), this, SLOT(fittingRunNoChanged()));

  connect(m_uiTabFitting.comboBox_bank, SIGNAL(currentIndexChanged(int)), this,
          SLOT(fittingBankIdChanged(int)));

  connect(m_uiTabFitting.comboBox_bank, SIGNAL(currentIndexChanged(int)), this,
          SLOT(setListWidgetBank(int)));

  connect(m_uiTabFitting.listWidget_fitting_bank_preview,
          SIGNAL(currentRowChanged(int)), this,
          SLOT(fittingListWidgetBank(int)));

  connect(m_uiTabFitting.listWidget_fitting_bank_preview,
          SIGNAL(currentRowChanged(int)), this, SLOT(setBankIdComboBox(int)));

  connect(m_uiTabFitting.comboBox_bank, SIGNAL(currentIndexChanged(int)), this,
          SLOT(setBankDir(int)));

  connect(m_uiTabFitting.pushButton_fitting_browse_peaks, SIGNAL(released()),
          this, SLOT(browsePeaksToFit()));

  connect(m_uiTabFitting.pushButton_fit, SIGNAL(released()), this,
          SLOT(fitClicked()));

  // add peak by clicking the button
  connect(m_uiTabFitting.pushButton_select_peak, SIGNAL(released()),
          SLOT(setPeakPick()));

  connect(m_uiTabFitting.pushButton_add_peak, SIGNAL(released()),
          SLOT(addPeakToList()));

  connect(m_uiTabFitting.pushButton_save_peak_list, SIGNAL(released()),
          SLOT(savePeakList()));

  m_uiTabFitting.dataPlot->setCanvasBackground(Qt::white);
  m_uiTabFitting.dataPlot->setAxisTitle(QwtPlot::xBottom,
                                        "d-Spacing (A)");
  m_uiTabFitting.dataPlot->setAxisTitle(QwtPlot::yLeft, "Counts (us)^-1");
  QFont font("MS Shell Dlg 2", 8);
  m_uiTabFitting.dataPlot->setAxisFont(QwtPlot::xBottom, font);
  m_uiTabFitting.dataPlot->setAxisFont(QwtPlot::yLeft, font);

  // constructor of the peakPicker
  // XXX: Being a QwtPlotItem, should get deleted when m_ui.plot gets deleted
  // (auto-delete option)
  m_peakPicker =
      new MantidWidgets::PeakPicker(m_uiTabFitting.dataPlot, Qt::red);
  setPeakPickerEnabled(false);

  m_zoomTool = new QwtPlotZoomer(
      QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner,
      QwtPicker::AlwaysOff, m_uiTabFitting.dataPlot->canvas());
  m_zoomTool->setRubberBandPen(QPen(Qt::black));
  setZoomTool(false);
}

void EnggDiffractionViewQtGUI::doSetupTabSettings() {
  // line edits that display paths and the like
  m_uiTabSettings.lineEdit_input_dir_calib->setText(
      QString::fromStdString(m_calibSettings.m_inputDirCalib));
  m_uiTabSettings.lineEdit_input_dir_raw->setText(
      QString::fromStdString(m_calibSettings.m_inputDirRaw));
  m_uiTabSettings.lineEdit_pixel_calib_filename->setText(
      QString::fromStdString(m_calibSettings.m_pixelCalibFilename));
  m_uiTabSettings.lineEdit_template_gsas_prm->setText(
      QString::fromStdString(m_calibSettings.m_templateGSAS_PRM));
  m_calibSettings.m_forceRecalcOverwrite = false;
  m_uiTabSettings.checkBox_force_recalculate_overwrite->setChecked(
      m_calibSettings.m_forceRecalcOverwrite);

  m_uiTabSettings.lineEdit_dir_focusing->setText(
      QString::fromStdString(m_focusDir));

  // push button signals/slots
  connect(m_uiTabSettings.pushButton_browse_input_dir_calib, SIGNAL(released()),
          this, SLOT(browseInputDirCalib()));

  connect(m_uiTabSettings.pushButton_browse_input_dir_raw, SIGNAL(released()),
          this, SLOT(browseInputDirRaw()));

  connect(m_uiTabSettings.pushButton_browse_pixel_calib_filename,
          SIGNAL(released()), this, SLOT(browsePixelCalibFilename()));

  connect(m_uiTabSettings.pushButton_browse_template_gsas_prm,
          SIGNAL(released()), this, SLOT(browseTemplateGSAS_PRM()));

  connect(m_uiTabSettings.pushButton_browse_dir_focusing, SIGNAL(released()),
          this, SLOT(browseDirFocusing()));
}

void EnggDiffractionViewQtGUI::doSetupGeneralWidgets() {
  enableTabs(false);

  // change instrument
  connect(m_ui.comboBox_instrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
  connect(m_ui.pushButton_help, SIGNAL(released()), this, SLOT(openHelpWin()));
  // note connection to the parent window, otherwise an empty frame window
  // may remain open and visible after this close
  connect(m_ui.pushButton_close, SIGNAL(released()), this->parent(),
          SLOT(close()));

  connect(m_ui.lineEdit_RBNumber, SIGNAL(editingFinished()), this,
          SLOT(RBNumberChanged()));
}

void EnggDiffractionViewQtGUI::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  m_ui.lineEdit_RBNumber->setText(
      qs.value("user-params-RBNumber", "").toString());

  m_uiTabCalib.lineEdit_current_vanadium_num->setText(
      qs.value("user-params-current-vanadium-num", "").toString());
  m_uiTabCalib.lineEdit_current_ceria_num->setText(
      qs.value("user-params-current-ceria-num", "").toString());
  QString calibFname = qs.value("current-calib-filename", "").toString();
  m_uiTabCalib.lineEdit_current_calib_filename->setText(calibFname);
  m_currentCalibFilename = calibFname.toStdString();

  m_uiTabCalib.lineEdit_new_vanadium_num->setText(
      qs.value("user-params-new-vanadium-num", "").toString());
  m_uiTabCalib.lineEdit_new_ceria_num->setText(
      qs.value("user-params-new-ceria-num", "").toString());

  m_uiTabCalib.groupBox_calib_cropped->setChecked(
      qs.value("user-params-calib-cropped-group-checkbox", false).toBool());

  m_uiTabCalib.lineEdit_cropped_run_num->setText(
      qs.value("user-params-new-ceria-num", "").toString());

  m_uiTabCalib.lineEdit_cropped_run_num->setReadOnly(true);

  m_uiTabCalib.comboBox_calib_cropped_bank_name->setCurrentIndex(0);

  m_uiTabCalib.lineEdit_cropped_spec_nos->setText(
      qs.value("user-params-calib-cropped-spectrum-nos", "").toString());

  m_uiTabCalib.lineEdit_cropped_customise_bank_name->setText(
      qs.value("user-params-calib-cropped-customise-name", "cropped")
          .toString());

  m_uiTabCalib.checkBox_PlotData_Calib->setChecked(
      qs.value("user-param-calib-plot-data", true).toBool());

  // user params - focusing
  m_uiTabFocus.lineEdit_run_num->setUserInput(
      qs.value("user-params-focus-runno", "").toString());

  qs.beginReadArray("user-params-focus-bank_i");
  qs.setArrayIndex(0);
  m_uiTabFocus.checkBox_focus_bank1->setChecked(
      qs.value("value", true).toBool());
  qs.setArrayIndex(1);
  m_uiTabFocus.checkBox_focus_bank2->setChecked(
      qs.value("value", true).toBool());
  qs.endArray();

  m_uiTabFocus.lineEdit_cropped_run_num->setUserInput(
      qs.value("user-params-focus-cropped-runno", "").toString());

  m_uiTabFocus.lineEdit_cropped_spec_nos->setText(
      qs.value("user-params-focus-cropped-spectrum-nos", "").toString());

  m_uiTabFocus.lineEdit_texture_run_num->setUserInput(
      qs.value("user-params-focus-texture-runno", "").toString());

  m_uiTabFocus.lineEdit_texture_grouping_file->setText(
      qs.value("user-params-focus-texture-detector-grouping-file", "")
          .toString());

  m_uiTabFocus.groupBox_cropped->setChecked(
      qs.value("user-params-focus-cropped-group-checkbox", false).toBool());

  m_uiTabFocus.groupBox_texture->setChecked(
      qs.value("user-params-focus-texture-group-checkbox", false).toBool());

  m_uiTabFocus.checkBox_FocusedWS->setChecked(
      qs.value("user-params-focus-plot-ws", true).toBool());

  m_uiTabFocus.comboBox_PlotData->setCurrentIndex(0);

  m_uiTabFocus.comboBox_Multi_Runs->setCurrentIndex(0);

  // pre-processing (re-binning)
  m_uiTabPreproc.MWRunFiles_preproc_run_num->setUserInput(
      qs.value("user-params-preproc-runno", "").toString());

  m_uiTabPreproc.doubleSpinBox_time_bin->setValue(
      qs.value("user-params-time-bin", 0.1).toDouble());

  m_uiTabPreproc.spinBox_nperiods->setValue(
      qs.value("user-params-nperiods", 2).toInt());

  m_uiTabPreproc.doubleSpinBox_step_time->setValue(
      qs.value("user-params-step-time", 1).toDouble());

  // user params - fitting
  m_uiTabFitting.lineEdit_pushButton_run_num->setText(
      qs.value("user-params-fitting-focused-file", "").toString());
  m_uiTabFitting.comboBox_bank->setCurrentIndex(0);
  m_uiTabFitting.lineEdit_fitting_peaks->setText(
      qs.value("user-params-fitting-peaks-to-fit", "").toString());
  m_uiTabFitting.listWidget_fitting_bank_preview->setCurrentRow(0);

  // settings
  QString lastPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  // TODO: as this is growing, it should become << >> operators on
  // EnggDiffCalibSettings
  m_calibSettings.m_inputDirCalib =
      qs.value("input-dir-calib-files", lastPath).toString().toStdString();
  m_calibSettings.m_inputDirRaw =
      qs.value("input-dir-raw-files", lastPath).toString().toStdString();
  const std::string fullCalib = guessDefaultFullCalibrationPath();
  m_calibSettings.m_pixelCalibFilename =
      qs.value("pixel-calib-filename", QString::fromStdString(fullCalib))
          .toString()
          .toStdString();
  // 'advanced' block
  m_calibSettings.m_forceRecalcOverwrite =
      qs.value("force-recalc-overwrite", false).toBool();
  const std::string templ = guessGSASTemplatePath();
  m_calibSettings.m_templateGSAS_PRM =
      qs.value("template-gsas-prm", QString::fromStdString(templ))
          .toString()
          .toStdString();
  m_calibSettings.m_forceRecalcOverwrite =
      qs.value("rebin-calib", g_defaultRebinWidth).toBool();

  // 'focusing' block
  m_focusDir = qs.value("focus-dir").toString().toStdString();

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void EnggDiffractionViewQtGUI::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("user-params-RBNumber", m_ui.lineEdit_RBNumber->text());

  qs.setValue("user-params-current-vanadium-num",
              m_uiTabCalib.lineEdit_current_vanadium_num->text());
  qs.setValue("user-params-current-ceria-num",
              m_uiTabCalib.lineEdit_current_ceria_num->text());
  qs.setValue("current-calib-filename",
              m_uiTabCalib.lineEdit_current_calib_filename->text());

  qs.setValue("user-params-new-vanadium-num",
              m_uiTabCalib.lineEdit_new_vanadium_num->getText());
  qs.setValue("user-params-new-ceria-num",
              m_uiTabCalib.lineEdit_new_ceria_num->getText());

  qs.setValue("user-params-calib-cropped-group-checkbox",
              m_uiTabCalib.groupBox_calib_cropped->isChecked());

  qs.setValue("user-params-calib-cropped-spectrum-nos",
              m_uiTabCalib.lineEdit_cropped_spec_nos->text());

  qs.setValue("user-params-calib-cropped-customise-name",
              m_uiTabCalib.lineEdit_cropped_customise_bank_name->text());

  qs.setValue("user-param-calib-plot-data",
              m_uiTabCalib.checkBox_PlotData_Calib->isChecked());

  // user params - focusing
  qs.setValue("user-params-focus-runno",
              m_uiTabFocus.lineEdit_run_num->getText());

  qs.beginWriteArray("user-params-focus-bank_i");
  qs.setArrayIndex(0);
  qs.setValue("value", m_uiTabFocus.checkBox_focus_bank1->isChecked());
  qs.setArrayIndex(1);
  qs.setValue("value", m_uiTabFocus.checkBox_focus_bank2->isChecked());
  qs.endArray();

  qs.setValue("user-params-focus-cropped-runno",
              m_uiTabFocus.lineEdit_cropped_run_num->getText());
  qs.setValue("user-params-focus-cropped-spectrum-nos",
              m_uiTabFocus.lineEdit_cropped_spec_nos->text());

  qs.setValue("user-params-focus-texture-runno",
              m_uiTabFocus.lineEdit_texture_run_num->getText());
  qs.setValue("user-params-focus-texture-detector-grouping-file",
              m_uiTabFocus.lineEdit_texture_grouping_file->text());

  qs.setValue("user-params-focus-cropped-group-checkbox",
              m_uiTabFocus.groupBox_cropped->isChecked());

  qs.setValue("user-params-focus-texture-group-checkbox",
              m_uiTabFocus.groupBox_texture->isChecked());

  qs.setValue("value", m_uiTabFocus.checkBox_FocusedWS->isChecked());

  // pre-processing (re-binning)
  qs.setValue("user-params-preproc-runno",
              m_uiTabPreproc.MWRunFiles_preproc_run_num->getText());

  qs.setValue("user-params-time-bin",
              m_uiTabPreproc.doubleSpinBox_time_bin->value());

  qs.setValue("user-params-nperiods", m_uiTabPreproc.spinBox_nperiods->value());

  qs.value("user-params-step-time",
           m_uiTabPreproc.doubleSpinBox_step_time->value());

  // fitting tab

  qs.setValue("user-params-fitting-focused-file",
              m_uiTabFitting.lineEdit_pushButton_run_num->text());
  qs.setValue("user-params-fitting-peaks-to-fit",
              m_uiTabFitting.lineEdit_fitting_peaks->text());

  // TODO: this should become << >> operators on EnggDiffCalibSettings
  qs.setValue("input-dir-calib-files",
              QString::fromStdString(m_calibSettings.m_inputDirCalib));
  qs.setValue("input-dir-raw-files",
              QString::fromStdString(m_calibSettings.m_inputDirRaw));
  qs.setValue("pixel-calib-filename",
              QString::fromStdString(m_calibSettings.m_pixelCalibFilename));
  // 'advanced' block
  qs.setValue("force-recalc-overwrite", m_calibSettings.m_forceRecalcOverwrite);
  qs.setValue("template-gsas-prm",
              QString::fromStdString(m_calibSettings.m_templateGSAS_PRM));
  qs.setValue("rebin-calib", m_calibSettings.m_rebinCalibrate);

  // 'focusing' block
  qs.setValue("focus-dir", QString::fromStdString(m_focusDir));

  qs.setValue("interface-win-geometry", saveGeometry());
  qs.endGroup();
}

std::string EnggDiffractionViewQtGUI::guessGSASTemplatePath() const {
  // Inside the mantid installation target directory:
  // scripts/Engineering/template_ENGINX_241391_236516_North_and_South_banks.par
  Poco::Path templ =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
  templ = templ.makeParent();
  templ.append("scripts");
  templ.append("Engineering");
  templ.append("template_ENGINX_241391_236516_North_and_South_banks.par");
  return templ.toString();
}

std::string EnggDiffractionViewQtGUI::guessDefaultFullCalibrationPath() const {
  // Inside the mantid installation target directory:
  // scripts/Engineering/ENGINX_full_pixel_calibration_vana194547_ceria193749.csv
  Poco::Path templ =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
  templ = templ.makeParent();
  templ.append("scripts");
  templ.append("Engineering");
  templ.append("calib");
  templ.append("ENGINX_full_pixel_calibration_vana194547_ceria193749.csv");
  return templ.toString();
}
void EnggDiffractionViewQtGUI::userWarning(const std::string &err,
                                           const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void EnggDiffractionViewQtGUI::userError(const std::string &err,
                                         const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

std::string EnggDiffractionViewQtGUI::askNewCalibrationFilename(
    const std::string &suggestedFname) {
  // append dir (basename) + filename
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  QDir path(prevPath);
  QString suggestion = path.filePath(QString::fromStdString(suggestedFname));
  QString choice = QFileDialog::getSaveFileName(
      this, tr("Please select the name of the calibration file"), suggestion,
      QString::fromStdString(g_iparmExtStr));

  return choice.toStdString();
}

std::string EnggDiffractionViewQtGUI::getRBNumber() const {
  return m_ui.lineEdit_RBNumber->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::currentVanadiumNo() const {
  return m_uiTabCalib.lineEdit_current_vanadium_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::currentCeriaNo() const {
  return m_uiTabCalib.lineEdit_current_ceria_num->text().toStdString();
}

std::vector<std::string> EnggDiffractionViewQtGUI::newVanadiumNo() const {
  return qListToVector(m_uiTabCalib.lineEdit_new_vanadium_num->getFilenames(),
                       m_uiTabCalib.lineEdit_new_vanadium_num->isValid());
}

std::vector<std::string> EnggDiffractionViewQtGUI::newCeriaNo() const {
  return qListToVector(m_uiTabCalib.lineEdit_new_ceria_num->getFilenames(),
                       m_uiTabCalib.lineEdit_new_ceria_num->isValid());
}

std::string EnggDiffractionViewQtGUI::currentCalibFile() const {
  return m_uiTabCalib.lineEdit_current_calib_filename->text().toStdString();
}

void EnggDiffractionViewQtGUI::newCalibLoaded(const std::string &vanadiumNo,
                                              const std::string &ceriaNo,
                                              const std::string &fname) {

  m_uiTabCalib.lineEdit_current_vanadium_num->setText(
      QString::fromStdString(vanadiumNo));
  m_uiTabCalib.lineEdit_current_ceria_num->setText(
      QString::fromStdString(ceriaNo));
  m_uiTabCalib.lineEdit_current_calib_filename->setText(
      QString::fromStdString(fname));

  if (!fname.empty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        QString::fromStdString(fname));
  }
}

void EnggDiffractionViewQtGUI::enableCalibrateAndFocusActions(bool enable) {
  // calibrate
  m_uiTabCalib.groupBox_make_new_calib->setEnabled(enable);
  m_uiTabCalib.groupBox_current_calib->setEnabled(enable);
  m_uiTabCalib.groupBox_calib_cropped->setEnabled(enable);
  m_uiTabCalib.pushButton_new_cropped_calib->setEnabled(enable);
  m_ui.pushButton_close->setEnabled(enable);
  m_uiTabCalib.checkBox_PlotData_Calib->setEnabled(enable);

  // focus
  m_uiTabFocus.lineEdit_run_num->setEnabled(enable);

  m_uiTabFocus.groupBox_cropped->setEnabled(enable);
  m_uiTabFocus.groupBox_texture->setEnabled(enable);

  m_uiTabFocus.pushButton_focus->setEnabled(enable);
  m_uiTabFocus.checkBox_FocusedWS->setEnabled(enable);
  m_uiTabFocus.checkBox_SaveOutputFiles->setEnabled(enable);
  m_uiTabFocus.comboBox_Multi_Runs->setEnabled(enable);

  m_uiTabFocus.pushButton_focus->setEnabled(enable);
  m_uiTabFocus.pushButton_stopFocus->setDisabled(enable);

  // pre-processing
  m_uiTabPreproc.MWRunFiles_preproc_run_num->setEnabled(enable);
  m_uiTabPreproc.pushButton_rebin_time->setEnabled(enable);
  m_uiTabPreproc.pushButton_rebin_multiperiod->setEnabled(enable);

  // fitting
  m_uiTabFitting.pushButton_fitting_browse_run_num->setEnabled(enable);
  m_uiTabFitting.lineEdit_pushButton_run_num->setEnabled(enable);
  m_uiTabFitting.pushButton_fitting_browse_peaks->setEnabled(enable);
  m_uiTabFitting.lineEdit_fitting_peaks->setEnabled(enable);
  m_uiTabFitting.pushButton_fit->setEnabled(enable);
}

void EnggDiffractionViewQtGUI::enableTabs(bool enable) {
  for (int ti = 0; ti < m_ui.tabMain->count(); ++ti) {
    m_ui.tabMain->setTabEnabled(ti, enable);
  }
}

std::vector<std::string> EnggDiffractionViewQtGUI::currentPreprocRunNo() const {
  return qListToVector(
      m_uiTabPreproc.MWRunFiles_preproc_run_num->getFilenames(),
      m_uiTabPreproc.MWRunFiles_preproc_run_num->isValid());
}

double EnggDiffractionViewQtGUI::rebinningTimeBin() const {
  return m_uiTabPreproc.doubleSpinBox_time_bin->value();
}

size_t EnggDiffractionViewQtGUI::rebinningPulsesNumberPeriods() const {
  return m_uiTabPreproc.spinBox_nperiods->value();
}

double EnggDiffractionViewQtGUI::rebinningPulsesTime() const {
  return m_uiTabPreproc.doubleSpinBox_step_time->value();
}

void EnggDiffractionViewQtGUI::setBankDir(int idx) {

  if (m_fitting_runno_dir_vec.size() >= size_t(idx)) {

    std::string bankDir = m_fitting_runno_dir_vec[idx];
    Poco::Path fpath(bankDir);

    setfittingRunNo(QString::fromUtf8(bankDir.c_str()));
  }
}

std::string EnggDiffractionViewQtGUI::fittingRunNoFactory(std::string bank,
                                                          std::string fileName,
                                                          std::string &bankDir,
                                                          std::string fileDir) {

  std::string genDir = fileName.substr(0, fileName.size() - 1);
  Poco::Path bankFile(genDir + bank + ".nxs");
  if (bankFile.isFile()) {
    bankDir = fileDir + genDir + bank + ".nxs";
  }
  return bankDir;
}

std::string EnggDiffractionViewQtGUI::readPeaksFile(std::string fileDir) {
  std::string fileData = "";
  std::string line;
  std::string comma = ", ";

  std::ifstream peakFile(fileDir);

  if (peakFile.is_open()) {
    while (std::getline(peakFile, line)) {
      fileData += line;
      if (!peakFile.eof())
        fileData += comma;
    }
    peakFile.close();
  }

  else
    fileData = "";

  return fileData;
}

void EnggDiffractionViewQtGUI::setDataVector(
    std::vector<boost::shared_ptr<QwtData>> &data, bool focused) {
  if (focused) {
    dataCurvesFactory(data, m_focusedDataVector, focused);
  } else {
    dataCurvesFactory(data, m_fittedDataVector, focused);
  }
}

void EnggDiffractionViewQtGUI::dataCurvesFactory(
    std::vector<boost::shared_ptr<QwtData>> &data,
    std::vector<QwtPlotCurve *> &dataVector, bool focused) {

  // clear vector
  for (auto curves : dataVector) {
    if (curves) {
      curves->detach();
      delete curves;
    }
  }

  if (dataVector.size() > 0)
    dataVector.clear();
  resetView();

  // dark colours could be removed so the colored peaks stand out more
  const std::array<QColor, 16> QPenList{
      {Qt::white, Qt::red, Qt::darkRed, Qt::green, Qt::darkGreen, Qt::blue,
       Qt::darkBlue, Qt::cyan, Qt::darkCyan, Qt::magenta, Qt::darkMagenta,
       Qt::yellow, Qt::darkYellow, Qt::gray, Qt::darkGray, Qt::lightGray}};

  std::mt19937 gen;
  std::uniform_int_distribution<std::size_t> dis(0, QPenList.size() - 1);

  for (size_t i = 0; i < data.size(); i++) {
    auto *peak = data[i].get();

    QwtPlotCurve *dataCurve = new QwtPlotCurve();
    dataCurve->setStyle(QwtPlotCurve::Lines);
    if (!focused) {
      auto randIndex = dis(gen);
      dataCurve->setPen(QPen(QPenList[randIndex], 1));
    }
    dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    dataVector.push_back(dataCurve);

    dataVector[i]->setData(*peak);
    dataVector[i]->attach(m_uiTabFitting.dataPlot);
  }

  m_uiTabFitting.dataPlot->replot();
  m_zoomTool->setZoomBase();
  // enable zoom & select peak btn after the plotting on graph
  setZoomTool(true);
  m_uiTabFitting.pushButton_select_peak->setEnabled(true);
  data.clear();
}

void EnggDiffractionViewQtGUI::setPeakPickerEnabled(bool enabled) {
  m_peakPicker->setEnabled(enabled);
  m_peakPicker->setVisible(enabled);
  m_uiTabFitting.dataPlot->replot(); // PeakPicker might get hidden/shown
  m_uiTabFitting.pushButton_add_peak->setEnabled(enabled);
  if (enabled) {
    QString btnText = "Reset Peak Selector";
    m_uiTabFitting.pushButton_select_peak->setText(btnText);
  }
}

void EnggDiffractionViewQtGUI::setPeakPicker(
    const IPeakFunction_const_sptr &peak) {
  m_peakPicker->setPeak(peak);
  m_uiTabFitting.dataPlot->replot();
}

double EnggDiffractionViewQtGUI::getPeakCentre() const {
  auto peak = m_peakPicker->peak();
  auto centre = peak->centre();
  return centre;
}

void EnggDiffractionViewQtGUI::fittingWriteFile(const std::string &fileDir) {
  std::ofstream outfile(fileDir.c_str());
  if (!outfile) {
    userWarning("File not found",
                "File " + fileDir + " , could not be found. Please try again!");
  } else {
    auto expPeaks = m_uiTabFitting.lineEdit_fitting_peaks->text();
    outfile << expPeaks.toStdString();
  }
}

void EnggDiffractionViewQtGUI::setZoomTool(bool enabled) {
  m_zoomTool->setEnabled(enabled);
}

void EnggDiffractionViewQtGUI::resetView() {
  // Resets the view to a sensible default
  // Auto scale the axis
  m_uiTabFitting.dataPlot->setAxisAutoScale(QwtPlot::xBottom);
  m_uiTabFitting.dataPlot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}

void EnggDiffractionViewQtGUI::plotFocusedSpectrum(const std::string &wsName) {
  std::string pyCode =
      "win=plotSpectrum('" + wsName + "', 0, error_bars=False, type=0)";

  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();
  m_logMsgs.emplace_back("Plotted output focused data, with status string " +
                         status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

void EnggDiffractionViewQtGUI::plotWaterfallSpectrum(
    const std::string &wsName) {
  // parameter of list ?
  std::string pyCode =
      "plotSpectrum('" + wsName +
      "', 0, error_bars=False, type=0, waterfall=True, window=win)";
  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();
  m_logMsgs.emplace_back("Plotted output focused data, with status string " +
                         status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

void EnggDiffractionViewQtGUI::plotReplacingWindow(const std::string &wsName,
                                                   const std::string &spectrum,
                                                   const std::string &type) {
  std::string pyCode = "win=plotSpectrum('" + wsName + "', " + spectrum +
                       ", error_bars=False, type=" + type +
                       ", window=win, clearWindow=True)";
  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();

  m_logMsgs.emplace_back("Plotted output focused data, with status string " +
                         status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

void EnggDiffractionViewQtGUI::plotVanCurvesCalibOutput() {
  std::string pyCode =
      "van_curves_ws = workspace(\"engggui_vanadium_curves_ws\")\n"
      "win = plotSpectrum(van_curves_ws, [0, 1, 2])";

  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();

  m_logMsgs.push_back(
      "Plotted output calibration vanadium curves, with status string " +
      status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

void EnggDiffractionViewQtGUI::plotDifcZeroCalibOutput(
    const std::string &pyCode) {

  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();

  m_logMsgs.push_back(
      "Plotted output calibration ceria peaks, with status string " + status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

void EnggDiffractionViewQtGUI::resetFocus() {
  m_uiTabFocus.lineEdit_run_num->setText("");
  m_uiTabFocus.checkBox_focus_bank1->setChecked(true);
  m_uiTabFocus.checkBox_focus_bank2->setChecked(true);

  m_uiTabFocus.lineEdit_cropped_run_num->setText("");
  m_uiTabFocus.lineEdit_cropped_spec_nos->setText("");

  m_uiTabFocus.groupBox_cropped->setChecked(false);
  m_uiTabFocus.groupBox_texture->setChecked(false);

  m_uiTabFocus.lineEdit_texture_run_num->setText("");
  m_uiTabFocus.lineEdit_texture_grouping_file->setText("");
}

void EnggDiffractionViewQtGUI::writeOutCalibFile(
    const std::string &outFilename, const std::vector<double> &difc,
    const std::vector<double> &tzero) {
  // TODO: this is horrible and should not last much here.
  // Avoid running Python code
  // Update this as soon as we have a more stable way of generating IPARM
  // files
  // Writes a file doing this:
  // write_ENGINX_GSAS_iparam_file(output_file, difc, zero, ceria_run=241391,
  // vanadium_run=236516, template_file=None):

  // this replace is to prevent issues with network drives on windows:
  const std::string safeOutFname =
      boost::replace_all_copy(outFilename, "\\", "/");
  std::string pyCode = "import EnggUtils\n";
  pyCode += "import os\n";
  // normalize apparently not needed after the replace, but to be double-safe:
  pyCode += "GSAS_iparm_fname= os.path.normpath('" + safeOutFname + "')\n";
  pyCode += "Difcs = []\n";
  pyCode += "Zeros = []\n";
  for (size_t i = 0; i < difc.size(); i++) {
    pyCode +=
        "Difcs.append(" + boost::lexical_cast<std::string>(difc[i]) + ")\n";
    pyCode +=
        "Zeros.append(" + boost::lexical_cast<std::string>(tzero[i]) + ")\n";
  }
  pyCode += "EnggUtils.write_ENGINX_GSAS_iparam_file(GSAS_iparm_fname, Difcs, "
            "Zeros) \n";

  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();

  // g_log.information()
  //     << "Saved output calibration file through Python. Status: " << status
  //     << std::endl;
  m_logMsgs.push_back(
      "Run Python code to save output file, with status string: " + status);
  m_presenter->notify(IEnggDiffractionPresenter::LogMsg);
}

std::string EnggDiffractionViewQtGUI::askExistingCalibFilename() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    QString prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString filename =
      QFileDialog::getOpenFileName(this, tr("Open calibration file"), prevPath,
                                   QString::fromStdString(g_iparmExtStr));

  if (!filename.isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        filename);
  }

  return filename.toStdString();
}

void EnggDiffractionViewQtGUI::loadCalibrationClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::LoadExistingCalib);
}

void EnggDiffractionViewQtGUI::calibrateClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::CalcCalib);
}

void EnggDiffractionViewQtGUI::CroppedCalibrateClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::CropCalib);
}

void EnggDiffractionViewQtGUI::focusClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::FocusRun);
}

void EnggDiffractionViewQtGUI::focusCroppedClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::FocusCropped);
}

void EnggDiffractionViewQtGUI::focusTextureClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::FocusTexture);
}

void EnggDiffractionViewQtGUI::focusResetClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::ResetFocus);
}

void EnggDiffractionViewQtGUI::focusStopClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::StopFocus);
}

void EnggDiffractionViewQtGUI::rebinTimeClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::RebinTime);
}

void EnggDiffractionViewQtGUI::rebinMultiperiodClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::RebinMultiperiod);
}

void EnggDiffractionViewQtGUI::fitClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::FitPeaks);
}

void EnggDiffractionViewQtGUI::browseInputDirCalib() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Open Directory"), prevPath,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (dir.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  m_calibSettings.m_inputDirCalib = dir.toStdString();
  m_uiTabSettings.lineEdit_input_dir_calib->setText(
      QString::fromStdString(m_calibSettings.m_inputDirCalib));
}

void EnggDiffractionViewQtGUI::browseInputDirRaw() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirRaw);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Open Directory"), prevPath,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (dir.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  m_calibSettings.m_inputDirRaw = dir.toStdString();
  m_uiTabSettings.lineEdit_input_dir_raw->setText(
      QString::fromStdString(m_calibSettings.m_inputDirRaw));
}

void EnggDiffractionViewQtGUI::browsePixelCalibFilename() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    QString prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString filename = QFileDialog::getOpenFileName(
      this, tr("Open pixel calibration (full calibration) file"), prevPath,
      QString::fromStdString(g_pixelCalibExt));

  if (filename.isEmpty()) {
    return;
  }

  m_calibSettings.m_pixelCalibFilename = filename.toStdString();
  m_uiTabSettings.lineEdit_pixel_calib_filename->setText(
      QString::fromStdString(m_calibSettings.m_pixelCalibFilename));
}

void EnggDiffractionViewQtGUI::browseTemplateGSAS_PRM() {

  QString prevPath = QString::fromStdString(m_calibSettings.m_templateGSAS_PRM);
  QString path(QFileDialog::getOpenFileName(
      this, tr("Open GSAS IPAR template file"), prevPath,
      QString::fromStdString(g_iparmExtStr)));

  if (path.isEmpty()) {
    return;
  }

  m_calibSettings.m_templateGSAS_PRM = path.toStdString();
  m_uiTabSettings.lineEdit_template_gsas_prm->setText(
      QString::fromStdString(m_calibSettings.m_templateGSAS_PRM));
}

void EnggDiffractionViewQtGUI::browseDirFocusing() {
  QString prevPath = QString::fromStdString(m_focusDir);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Open Directory"), prevPath,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (dir.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  m_focusDir = dir.toStdString();
  m_uiTabSettings.lineEdit_dir_focusing->setText(
      QString::fromStdString(m_focusDir));
}

void EnggDiffractionViewQtGUI::browseTextureDetGroupingFile() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirRaw);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString path(QFileDialog::getOpenFileName(
      this, tr("Open detector grouping file"), prevPath,
      QString::fromStdString(g_DetGrpExtStr)));

  if (path.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  m_uiTabFocus.lineEdit_texture_grouping_file->setText(path);
}

void EnggDiffractionViewQtGUI::browseFitFocusedRun() {
  QString prevPath = QString::fromStdString(m_focusDir);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  std::string nexusFormat = "Nexus file with calibration table: NXS, NEXUS"
                            "(*.nxs *.nexus);;";

  QString path(
      QFileDialog::getOpenFileName(this, tr("Open Focused File "), prevPath,
                                   QString::fromStdString(nexusFormat)));

  if (path.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  setfittingRunNo(path);
  getBanks();
}

void EnggDiffractionViewQtGUI::browsePeaksToFit() {

  try {
    QString prevPath = QString::fromStdString(m_focusDir);
    if (prevPath.isEmpty()) {
      prevPath = MantidQt::API::AlgorithmInputHistory::Instance()
                     .getPreviousDirectory();
    }

    QString path(
        QFileDialog::getOpenFileName(this, tr("Open Peaks To Fit"), prevPath,
                                     QString::fromStdString(g_DetGrpExtStr)));

    if (path.isEmpty()) {
      return;
    }

    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);

    std::string peaksData = readPeaksFile(path.toStdString());

    m_uiTabFitting.lineEdit_fitting_peaks->setText(
        QString::fromStdString(peaksData));
  } catch (...) {
    userWarning("Unable to import the peaks from a file: ",
                "File corrupted or could not be opened. Please try again");
    return;
  }
}

std::vector<std::string> EnggDiffractionViewQtGUI::focusingRunNo() const {
  return qListToVector(m_uiTabFocus.lineEdit_run_num->getFilenames(),
                       m_uiTabFocus.lineEdit_run_num->isValid());
}

std::vector<std::string>
EnggDiffractionViewQtGUI::focusingCroppedRunNo() const {
  return qListToVector(m_uiTabFocus.lineEdit_cropped_run_num->getFilenames(),
                       m_uiTabFocus.lineEdit_cropped_run_num->isValid());
}

std::vector<std::string>
EnggDiffractionViewQtGUI::focusingTextureRunNo() const {
  return qListToVector(m_uiTabFocus.lineEdit_texture_run_num->getFilenames(),
                       m_uiTabFocus.lineEdit_texture_run_num->isValid());
}

std::vector<std::string>
EnggDiffractionViewQtGUI::qListToVector(QStringList list,
                                        bool validator) const {
  std::vector<std::string> vec;
  if (validator) {
    foreach (QString str, list) { vec.push_back(str.toStdString()); }
    return vec;
  } else {
    return vec;
  }
}

std::string EnggDiffractionViewQtGUI::focusingDir() const {
  return m_uiTabSettings.lineEdit_dir_focusing->text().toStdString();
}

std::vector<bool> EnggDiffractionViewQtGUI::focusingBanks() const {
  std::vector<bool> res;
  res.push_back(m_uiTabFocus.checkBox_focus_bank1->isChecked());
  res.push_back(m_uiTabFocus.checkBox_focus_bank2->isChecked());
  return res;
}

std::string EnggDiffractionViewQtGUI::focusingCroppedSpectrumNos() const {
  return m_uiTabFocus.lineEdit_cropped_spec_nos->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::focusingTextureGroupingFile() const {
  return m_uiTabFocus.lineEdit_texture_grouping_file->text().toStdString();
}

bool EnggDiffractionViewQtGUI::focusedOutWorkspace() const {
  return m_uiTabFocus.checkBox_FocusedWS->checkState();
}

bool EnggDiffractionViewQtGUI::plotCalibWorkspace() const {
  return m_uiTabCalib.checkBox_PlotData_Calib->checkState();
}

bool EnggDiffractionViewQtGUI::saveFocusedOutputFiles() const {
  return m_uiTabFocus.checkBox_SaveOutputFiles->checkState();
}

void EnggDiffractionViewQtGUI::plotFocusStatus() {
  if (focusedOutWorkspace()) {
    m_uiTabFocus.comboBox_PlotData->setEnabled(true);
  } else {
    m_uiTabFocus.comboBox_PlotData->setEnabled(false);
  }
}

void EnggDiffractionViewQtGUI::updateCroppedCalibRun() {
  auto ceria = m_uiTabCalib.lineEdit_new_ceria_num->getText();
  m_uiTabCalib.lineEdit_cropped_run_num->setText(ceria);
}

void EnggDiffractionViewQtGUI::calibspecNoChanged(int /*idx*/) {
  QComboBox *BankName = m_uiTabCalib.comboBox_calib_cropped_bank_name;
  if (!BankName)
    return;
  m_currentCropCalibBankName = BankName->currentIndex();
}

void EnggDiffractionViewQtGUI::enableSpecNos() {
  if (m_currentCropCalibBankName == 0) {
    m_uiTabCalib.lineEdit_cropped_spec_nos->setEnabled(true);
    m_uiTabCalib.lineEdit_cropped_customise_bank_name->setEnabled(true);
  } else {
    m_uiTabCalib.lineEdit_cropped_spec_nos->setDisabled(true);
    m_uiTabCalib.lineEdit_cropped_customise_bank_name->setDisabled(true);
  }
}

std::string EnggDiffractionViewQtGUI::currentCalibSpecNos() const {
  return m_uiTabCalib.lineEdit_cropped_spec_nos->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::currentCalibCustomisedBankName() const {
  return m_uiTabCalib.lineEdit_cropped_customise_bank_name->text()
      .toStdString();
}

void EnggDiffractionViewQtGUI::multiRunModeChanged(int /*idx*/) {
  QComboBox *plotType = m_uiTabFocus.comboBox_Multi_Runs;
  if (!plotType)
    return;
  m_currentRunMode = plotType->currentIndex();
}

void EnggDiffractionViewQtGUI::plotRepChanged(int /*idx*/) {
  QComboBox *plotType = m_uiTabFocus.comboBox_PlotData;
  if (!plotType)
    return;
  m_currentType = plotType->currentIndex();
}

void EnggDiffractionViewQtGUI::fittingBankIdChanged(int /*idx*/) {
  QComboBox *BankName = m_uiTabFitting.comboBox_bank;
  if (!BankName)
    return;
  m_fitting_bank_Id = BankName->currentIndex();
}

void EnggDiffractionViewQtGUI::setBankIdComboBox(int idx) {
  QComboBox *bankName = m_uiTabFitting.comboBox_bank;
  bankName->setCurrentIndex(idx);
}

void EnggDiffractionViewQtGUI::setfittingRunNo(QString path) {
  m_uiTabFitting.lineEdit_pushButton_run_num->setText(path);
}

std::string EnggDiffractionViewQtGUI::fittingRunNo() const {
  return m_uiTabFitting.lineEdit_pushButton_run_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::fittingPeaksData() const {
  // this should be moved to Helper or could use the poco string tokenizers
  std::string exptPeaks =
      m_uiTabFitting.lineEdit_fitting_peaks->text().toStdString();
  size_t strLength = exptPeaks.length() - 1;

  if (!exptPeaks.empty()) {

    if (exptPeaks.at(size_t(0)) == ',') {
      exptPeaks.erase(size_t(0), 1);
      strLength -= size_t(1);
    }

    if (exptPeaks.at(strLength) == ',') {
      exptPeaks.erase(strLength, 1);
    }
  }
  return exptPeaks;
}

void EnggDiffractionViewQtGUI::fittingListWidgetBank(int /*idx*/) {

  QListWidget *BankSelected = m_uiTabFitting.listWidget_fitting_bank_preview;
  if (!BankSelected)
    return;
  m_fitting_bank_Id = BankSelected->currentRow();
}

void EnggDiffractionViewQtGUI::setListWidgetBank(int idx) {

  QListWidget *selectBank = m_uiTabFitting.listWidget_fitting_bank_preview;
  selectBank->setCurrentRow(idx);
}

void MantidQt::CustomInterfaces::EnggDiffractionViewQtGUI::
    fittingRunNoChanged() {
  // TODO: much of this should be moved to presenter
  try {
    QString focusedFile = m_uiTabFitting.lineEdit_pushButton_run_num->text();
    std::string strFocusedFile = focusedFile.toStdString();
    // file name
    Poco::Path selectedfPath(strFocusedFile);
    Poco::Path bankDir;

    // handling of vectors
    m_fitting_runno_dir_vec.clear();
    std::string strFPath = selectedfPath.toString();
    std::vector<std::string> splitBaseName = splitFittingDirectory(strFPath);

    if (selectedfPath.isFile() && !splitBaseName.empty()) {

#ifdef __unix__
      bankDir = selectedfPath.parent();
#else
      bankDir = (bankDir).expand(selectedfPath.parent().toString());
#endif

      if (!splitBaseName.empty() && splitBaseName.size() > 3) {
        std::string foc_file = splitBaseName[0] + "_" + splitBaseName[1] + "_" +
                               splitBaseName[2] + "_" + splitBaseName[3];
        std::string strBankDir = bankDir.toString();
        updateFittingDirVec(strBankDir, foc_file);
      }
      // if run number length greater
    } else if (focusedFile.count() > 4) {
      // if given a run number instead
      updateFittingDirVec(m_focusDir, strFocusedFile);
    } else {
      userWarning("Invalid Input", "Invalid directory or run number given. "
                                   "Please try again");
    }

    try {
      // add bank to the combo-box and list view
      addBankItems(splitBaseName, focusedFile);
    } catch (std::runtime_error &re) {
      userWarning("Unable to insert items: ",
                  "Could not add banks to "
                  "combo-box or list widget; " +
                      static_cast<std::string>(re.what()) +
                      ". Please try again");
    }
  } catch (std::runtime_error &re) {
    userWarning("Invalid file", "Unable to select the file; " +
                                    static_cast<std::string>(re.what()));
    return;
  }
}

void EnggDiffractionViewQtGUI::updateFittingDirVec(std::string &bankDir,
                                                   std::string &focusedFile) {

  try {

    std::string cwd(bankDir);
    Poco::DirectoryIterator it(cwd);
    Poco::DirectoryIterator end;
    while (it != end) {
      if (it->isFile()) {
        std::string itFilePath = it->path();
        Poco::Path itBankfPath(itFilePath);

        std::string itbankFileName = itBankfPath.getBaseName();
        // check if it not any other file.. e.g: texture
        if (itbankFileName.find(focusedFile) != std::string::npos) {
          m_fitting_runno_dir_vec.push_back(itFilePath);
        }
      }
      ++it;
    }
  } catch (std::runtime_error &re) {
    userWarning("Invalid file", "File not found in the following directory; " +
                                    bankDir + ". " +
                                    static_cast<std::string>(re.what()));
  }
}

std::vector<std::string>
EnggDiffractionViewQtGUI::splitFittingDirectory(std::string &selectedfPath) {

  Poco::Path PocofPath(selectedfPath);
  std::string selectedbankfName = PocofPath.getBaseName();
  std::vector<std::string> splitBaseName;
  if (selectedbankfName.find("ENGINX_") != std::string::npos) {
    boost::split(splitBaseName, selectedbankfName, boost::is_any_of("_."));
  }
  return splitBaseName;
}

void EnggDiffractionViewQtGUI::addBankItems(
    std::vector<std::string> splittedBaseName, QString selectedFile) {

  if (!m_fitting_runno_dir_vec.empty()) {

    // delete previous bank added to the list
    m_uiTabFitting.comboBox_bank->clear();
    m_uiTabFitting.listWidget_fitting_bank_preview->clear();

    for (size_t i = 0; i < m_fitting_runno_dir_vec.size(); i++) {
      Poco::Path vecFile(m_fitting_runno_dir_vec[i]);
      std::string strVecFile = vecFile.toString();
      // split the directory from m_fitting_runno_dir_vec
      std::vector<std::string> vecFileSplit = splitFittingDirectory(strVecFile);
      // assign the file bank id
      std::string bankID = (vecFileSplit[vecFileSplit.size() - 1]);

      bool isDigit = false;
      for (size_t i = 0; i < bankID.size(); i++) {
        char *str = &bankID[i];
        if (std::isdigit(*str)) {
          isDigit = true;
        }
      }

      if (isDigit) {
        m_uiTabFitting.comboBox_bank->addItem(QString::fromStdString(bankID));
        m_uiTabFitting.listWidget_fitting_bank_preview->addItem(
            QString::fromStdString(bankID));
      } else {
        m_uiTabFitting.comboBox_bank->addItem(QString("Bank %1").arg(i + 1));
        m_uiTabFitting.listWidget_fitting_bank_preview->addItem(
            QString("%1").arg(i + 1));
      }
    }
    m_uiTabFitting.comboBox_bank->setEnabled(true);
    m_uiTabFitting.listWidget_fitting_bank_preview->setEnabled(true);
  } else {
    // upon invalid file
    // disable the widgets when only one related file found
    m_uiTabFitting.comboBox_bank->setEnabled(false);
    m_uiTabFitting.listWidget_fitting_bank_preview->setEnabled(false);

    m_uiTabFitting.comboBox_bank->clear();
    m_uiTabFitting.listWidget_fitting_bank_preview->clear();
  }

  setDefaultBank(splittedBaseName, selectedFile);
}

void EnggDiffractionViewQtGUI::setDefaultBank(
    std::vector<std::string> splittedBaseName, QString selectedFile) {

  if (!splittedBaseName.empty()) {

    std::string bankID = (splittedBaseName[splittedBaseName.size() - 1]);
    auto combo_data =
        m_uiTabFitting.comboBox_bank->findText(QString::fromStdString(bankID));

    if (combo_data > -1) {
      setBankIdComboBox(combo_data);
    } else {
      setfittingRunNo(selectedFile);
    }
  } else {
    setfittingRunNo(selectedFile);
  }
}

void MantidQt::CustomInterfaces::EnggDiffractionViewQtGUI::setPeakPick() {
  auto bk2bk =
      FunctionFactory::Instance().createFunction("BackToBackExponential");
  auto bk2bkFunc = boost::dynamic_pointer_cast<IPeakFunction>(bk2bk);
  // set the peak to BackToBackExponential function
  setPeakPicker(bk2bkFunc);
  setPeakPickerEnabled(true);
}

void MantidQt::CustomInterfaces::EnggDiffractionViewQtGUI::addPeakToList() {

  if (m_peakPicker->isEnabled()) {
    auto peakCentre = getPeakCentre();

    std::stringstream stream;
    stream << std::fixed << std::setprecision(4) << peakCentre;
    auto strPeakCentre = stream.str();

    auto curExpPeaksList = m_uiTabFitting.lineEdit_fitting_peaks->text();

    if (!curExpPeaksList.isEmpty()) {

      std::string expPeakStr = curExpPeaksList.toStdString();
      std::string lastTwoChr = expPeakStr.substr(expPeakStr.size() - 2);
      auto lastChr = expPeakStr.back();
      char comma = ',';
      if (lastChr == comma || lastTwoChr == ", ") {
        curExpPeaksList.append(QString::fromStdString(" " + strPeakCentre));
      } else {
        QString comma = ", ";
        curExpPeaksList.append(comma + QString::fromStdString(strPeakCentre));
      }
      m_uiTabFitting.lineEdit_fitting_peaks->setText(curExpPeaksList);
    }
  }
}

void MantidQt::CustomInterfaces::EnggDiffractionViewQtGUI::savePeakList() {
  // call function in EnggPresenter..

  try {
    QString prevPath = QString::fromStdString(m_focusDir);
    if (prevPath.isEmpty()) {
      prevPath = MantidQt::API::AlgorithmInputHistory::Instance()
                     .getPreviousDirectory();
    }

    QString path(QFileDialog::getSaveFileName(
        this, tr("Save Expected Peaks List"), prevPath,
        QString::fromStdString(g_DetGrpExtStr)));

    if (path.isEmpty()) {
      return;
    }
    const std::string strPath = path.toStdString();
    fittingWriteFile(strPath);
  } catch (...) {
    userWarning("Unable to save the peaks file: ",
                "Invalid file path or or could not be saved. Please try again");
    return;
  }
}

void EnggDiffractionViewQtGUI::instrumentChanged(int /*idx*/) {
  QComboBox *inst = m_ui.comboBox_instrument;
  if (!inst)
    return;
  m_currentInst = inst->currentText().toStdString();
  m_presenter->notify(IEnggDiffractionPresenter::InstrumentChange);
}

void EnggDiffractionViewQtGUI::RBNumberChanged() {
  m_presenter->notify(IEnggDiffractionPresenter::RBNumberChange);
}

void EnggDiffractionViewQtGUI::userSelectInstrument(const QString &prefix) {
  // Set file browsing to current instrument
  setPrefix(prefix.toStdString());
}

void EnggDiffractionViewQtGUI::setPrefix(std::string prefix) {
  QString prefixInput = QString::fromStdString(prefix);
  // focus tab
  m_uiTabFocus.lineEdit_run_num->setInstrumentOverride(prefixInput);
  m_uiTabFocus.lineEdit_texture_run_num->setInstrumentOverride(prefixInput);
  m_uiTabFocus.lineEdit_cropped_run_num->setInstrumentOverride(prefixInput);

  // calibration tab
  m_uiTabCalib.lineEdit_new_ceria_num->setInstrumentOverride(prefixInput);
  m_uiTabCalib.lineEdit_new_vanadium_num->setInstrumentOverride(prefixInput);

  // rebin tab
  m_uiTabPreproc.MWRunFiles_preproc_run_num->setInstrumentOverride(prefixInput);
  m_uiTabCalib.lineEdit_cropped_run_num->setInstrumentOverride(prefixInput);
}

void EnggDiffractionViewQtGUI::closeEvent(QCloseEvent *event) {
  int answer = QMessageBox::AcceptRole;

  QMessageBox msgBox;
  if (false /* TODO: get this from user settings if eventually used */) {
    msgBox.setWindowTitle("Close the engineering diffraction interface");
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
    msgBox.setInformativeText("Are you sure?");
    answer = msgBox.exec();
  }

  if (answer == QMessageBox::AcceptRole && m_ui.pushButton_close->isEnabled()) {
    m_presenter->notify(IEnggDiffractionPresenter::ShutDown);
    event->accept();
  } else {
    event->ignore();
  }
}

void EnggDiffractionViewQtGUI::openHelpWin() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Engineering_Diffraction"));
}

} // namespace CustomInterfaces
} // namespace MantidQt
