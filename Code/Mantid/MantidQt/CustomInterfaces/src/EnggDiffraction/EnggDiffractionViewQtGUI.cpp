#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionViewQtGUI.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <boost/lexical_cast.hpp>

#include <Poco/Path.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(EnggDiffractionViewQtGUI)

const double EnggDiffractionViewQtGUI::g_defaultRebinWidth = -0.0005;

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

const std::string EnggDiffractionViewQtGUI::m_settingsGroup =
    "CustomInterfaces/EnggDiffractionView";

/**
 * Default constructor.
 *
 * @param parent Parent window (most likely the Mantid main app window).
 */
EnggDiffractionViewQtGUI::EnggDiffractionViewQtGUI(QWidget *parent)
    : UserSubWindow(parent), IEnggDiffractionView(), m_currentInst("ENGIN-X"),
      m_currentCalibFilename(""), m_presenter(NULL) {}

EnggDiffractionViewQtGUI::~EnggDiffractionViewQtGUI() {}

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

  QWidget *wSettings = new QWidget(m_ui.tabMain);
  m_uiTabSettings.setupUi(wSettings);
  m_ui.tabMain->addTab(wSettings, QString("Settings"));

  readSettings();

  // basic UI setup, connect signals, etc.
  doSetupGeneralWidgets();
  doSetupTabCalib();
  doSetupTabFocus();
  doSetupTabSettings();

  // presenter that knows how to handle a IEnggDiffractionView should take care
  // of all the logic
  // note that the view needs to know the concrete presenter
  m_presenter.reset(new EnggDiffractionPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IEnggDiffractionPresenter::Start);
}

void EnggDiffractionViewQtGUI::doSetupTabCalib() {
  // Last available runs. This (as well as the empty defaults just
  // above) should probably be made persistent - and encapsulated into a
  // CalibrationParameters or similar class/structure
  const std::string vanadiumRun = "236516";
  const std::string ceriaRun = "241391";
  m_uiTabCalib.lineEdit_new_vanadium_num->setText(
      QString::fromStdString(vanadiumRun));
  m_uiTabCalib.lineEdit_new_ceria_num->setText(
      QString::fromStdString(ceriaRun));

  // push button signals/slots
  connect(m_uiTabCalib.pushButton_load_calib, SIGNAL(released()), this,
          SLOT(loadCalibrationClicked()));

  connect(m_uiTabCalib.pushButton_new_calib, SIGNAL(released()), this,
          SLOT(calibrateClicked()));

  enableCalibrateAndFocusActions(true);
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

void EnggDiffractionViewQtGUI::doSetupTabFocus() {
  connect(m_uiTabFocus.pushButton_focus, SIGNAL(released()), this,
          SLOT(focusClicked()));
}

void EnggDiffractionViewQtGUI::doSetupGeneralWidgets() {
  // change instrument
  connect(m_ui.comboBox_instrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));

  connect(m_ui.pushButton_help, SIGNAL(released()), this, SLOT(openHelpWin()));
  // note connection to the parent window, otherwise an empty frame window
  // may remain open and visible after this close
  connect(m_ui.pushButton_close, SIGNAL(released()), this->parent(),
          SLOT(close()));
}

void EnggDiffractionViewQtGUI::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  m_uiTabCalib.lineEdit_RBNumber->setText(
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

  QString lastPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  // TODO: this should become << >> operators on EnggDiffCalibSettings
  m_calibSettings.m_inputDirCalib =
      qs.value("input-dir-calib-files", lastPath).toString().toStdString();
  m_calibSettings.m_inputDirRaw =
      qs.value("input-dir-raw-files", lastPath).toString().toStdString();
  m_calibSettings.m_pixelCalibFilename =
      qs.value("pixel-calib-filename", "").toString().toStdString();
  // 'advanced' block
  m_calibSettings.m_forceRecalcOverwrite =
      qs.value("force-recalc-overwrite", false).toBool();
  std::string templ = guessGSASTemplatePath();
  m_calibSettings.m_templateGSAS_PRM =
      qs.value("template-gsas-prm", QString::fromStdString(templ))
          .toString()
          .toStdString();
  m_calibSettings.m_forceRecalcOverwrite =
      qs.value("rebin-calib", g_defaultRebinWidth).toFloat();

  // 'focusing' block
  m_focusDir = qs.value("focus-dir").toString().toStdString();

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void EnggDiffractionViewQtGUI::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("user-params-RBNumber", m_uiTabCalib.lineEdit_RBNumber->text());

  qs.setValue("user-params-current-vanadium-num",
              m_uiTabCalib.lineEdit_current_vanadium_num->text());
  qs.setValue("user-params-current-ceria-num",
              m_uiTabCalib.lineEdit_current_ceria_num->text());
  qs.setValue("current-calib-filename",
              m_uiTabCalib.lineEdit_current_calib_filename->text());

  qs.setValue("user-params-new-vanadium-num",
              m_uiTabCalib.lineEdit_new_vanadium_num->text());
  qs.setValue("user-params-new-ceria-num",
              m_uiTabCalib.lineEdit_new_ceria_num->text());

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
  // +scripts/Engineering/template_ENGINX_241391_236516_North_and_South_banks.par
  Poco::Path templ =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
  templ = templ.makeParent();
  templ.append("scripts");
  templ.append("Engineering");
  templ.append("template_ENGINX_241391_236516_North_and_South_banks.par");
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
  return m_uiTabCalib.lineEdit_RBNumber->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::currentVanadiumNo() const {
  return m_uiTabCalib.lineEdit_current_vanadium_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::currentCeriaNo() const {
  return m_uiTabCalib.lineEdit_current_ceria_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::newVanadiumNo() const {
  return m_uiTabCalib.lineEdit_new_vanadium_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::newCeriaNo() const {
  return m_uiTabCalib.lineEdit_new_ceria_num->text().toStdString();
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
  m_uiTabCalib.lineEdit_RBNumber->setEnabled(enable);
  m_uiTabCalib.groupBox_make_new_calib->setEnabled(enable);
  m_uiTabCalib.groupBox_current_calib->setEnabled(enable);

  m_ui.pushButton_close->setEnabled(enable);

  // focus
  m_uiTabFocus.lineEdit_run_num->setEnabled(enable);
  m_uiTabFocus.pushButton_focus->setEnabled(enable);
}

void
EnggDiffractionViewQtGUI::writeOutCalibFile(const std::string &outFilename,
                                            const std::vector<double> &difc,
                                            const std::vector<double> &tzero) {
  // TODO: this is horrible and should not last much here.
  // Avoid running Python code
  // Update this as soon as we have a more stable way of generating IPARM files
  // Writes a file doing this:
  // write_ENGINX_GSAS_iparam_file(output_file, difc, zero, ceria_run=241391,
  // vanadium_run=236516, template_file=None):

  // this replace is to prevent issues with network drives on windows:
  const std::string  safeOutFname= boost::replace_all_copy(outFilename, "\\","/");
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

void EnggDiffractionViewQtGUI::focusClicked() {
  m_presenter->notify(IEnggDiffractionPresenter::FocusRun);
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

std::string EnggDiffractionViewQtGUI::focusingRunNo() const {
  return m_uiTabFocus.lineEdit_run_num->text().toStdString();
}

std::string EnggDiffractionViewQtGUI::focusingDir() const {
  return m_uiTabSettings.lineEdit_dir_focusing->text().toStdString();
}

int EnggDiffractionViewQtGUI::focusingBank() const {
  int idx = m_uiTabFocus.comboBox_bank_num->currentIndex();
  return m_uiTabFocus.comboBox_bank_num->itemData(idx).toInt() + 1;
}

void EnggDiffractionViewQtGUI::instrumentChanged(int /*idx*/) {
  QComboBox *inst = m_ui.comboBox_instrument;
  if (!inst)
    return;

  m_currentInst = inst->currentText().toStdString();
  m_presenter->notify(IEnggDiffractionPresenter::InstrumentChange);
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
