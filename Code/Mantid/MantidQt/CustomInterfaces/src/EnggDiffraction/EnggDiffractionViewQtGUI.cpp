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

const double EnggDiffractionViewQtGUI::s_defaultRebinWidth = -0.0005;

/**
 * Default constructor.
 *
 * @param parent Parent window (most likely the Mantid main app window).
 */
EnggDiffractionViewQtGUI::EnggDiffractionViewQtGUI(QWidget *parent)
    : UserSubWindow(parent), IEnggDiffractionView(), m_presenter(NULL) {}

EnggDiffractionViewQtGUI::~EnggDiffractionViewQtGUI() {}

void EnggDiffractionViewQtGUI::initLayout() {
  // setup container ui
  m_ui.setupUi(this);
  // add tab contents and set up their ui's
  QWidget *wCalib = new QWidget(m_ui.tabMain);
  m_uiTabCalib.setupUi(wCalib);
  m_ui.tabMain->addTab(wCalib, QString("Calibration"));
  QWidget *wSettings = new QWidget(m_ui.tabMain);
  m_uiTabSettings.setupUi(wSettings);
  m_ui.tabMain->addTab(wSettings, QString("Settings"));

  readSettings();

  // basic UI setup, connect signals, etc.
  doSetupGeneralWidgets();
  doSetupTabCalib();
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
  const std::string vanadiumRun = "236516";
  const std::string ceriaRun = "241391";

  // line edits with calibration parameters
  m_uiTabCalib.lineEdit_vanadium_num->setText(
      QString::fromStdString(vanadiumRun));
  m_uiTabCalib.lineEdit_ceria_num->setText(QString::fromStdString(ceriaRun));

  m_uiTabCalib.lineEdit_new_vanadium_num->setText(
      QString::fromStdString(vanadiumRun));
  m_uiTabCalib.lineEdit_new_ceria_num->setText(
      QString::fromStdString(ceriaRun));

  // push button signals/slots
  connect(m_uiTabCalib.pushButton_load_calib, SIGNAL(released()), this,
          SLOT(loadCalibrationClicked()));

  connect(m_uiTabCalib.pushButton_new_calib, SIGNAL(released()), this,
          SLOT(calibrateClicked()));
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

  // push button signals/slots
  connect(m_uiTabSettings.pushButton_browse_input_dir_calib, SIGNAL(released()),
          this, SLOT(browseInputDirCalib()));

  connect(m_uiTabSettings.pushButton_browse_input_dir_raw, SIGNAL(released()),
          this, SLOT(browseInputDirRaw()));

  connect(m_uiTabSettings.pushButton_browse_pixel_calib_filename,
          SIGNAL(released()), this, SLOT(browsePixelCalibFilename()));

  connect(m_uiTabSettings.pushButton_browse_template_gsas_prm,
          SIGNAL(released()), this, SLOT(browseTemplateGSAS_PRM()));
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

  QString lastPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();

  m_currentCalibFilename =
      qs.value("current-calib-filename", "").toString().toStdString();

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
      qs.value("rebin-calib", s_defaultRebinWidth).toFloat();

  restoreGeometry(qs.value("interface-win-geometry").toByteArray());
  qs.endGroup();
}

void EnggDiffractionViewQtGUI::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsGroup));

  qs.setValue("current-calib-filename", "");

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

std::string EnggDiffractionViewQtGUI::getRBNumber() const {
  return "not available";
}

void EnggDiffractionViewQtGUI::loadCalibrationClicked() {
  const QString calExt =
      QString("Supported formats: CSV, NXS "
              "(*.csv *.nxs *.nexus);;"
              "Comma separated values text file with calibration table "
              "(*.csv);;"
              "Nexus file with calibration table "
              "(*.nxs *.nexus);;"
              "Other extensions/all files (*.*)");

  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    QString prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString filename = (QFileDialog::getOpenFileName(
      this, tr("Open calibration file"), prevPath, calExt));

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
      filename);

  m_presenter->notify(IEnggDiffractionPresenter::LoadExistingCalib);
}

/**
 * Build a suggested name for a new calibration, by appending instrument name,
 * relevant run numbers, etc., like: ENGINX_241391_236516_both_banks.par
 *
 * @param vanNo number of the Vanadium run
 * @param ceriaNo number of the Ceria run
 *
 * @return Suggested name for a new calibration file, following
 * ENGIN-X practices
 */
std::string EnggDiffractionViewQtGUI::buildCalibrateSuggestedFilename(
    const std::string &vanNo, const std::string &ceriaNo) const {
  std::string instStr = "ENGINX";
  if ("ENGIN-X" != currentInstrument()) {
    instStr = "UNKNOWN_INST";
  }

  // default extension for calibration files
  const std::string calibExt = ".prm";
  std::string sugg = instStr + "_" + vanNo + "_" + ceriaNo + "_"
                                                             "_both_banks" +
                     "." + calibExt;

  return sugg;
}

void EnggDiffractionViewQtGUI::calibrateClicked() {
  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString vanNo = m_uiTabCalib.lineEdit_new_vanadium_num->text();
  if (vanNo.isEmpty()) {
    userWarning(
        "Error in the inputs",
        "The Vanadium number cannot be empty and must be an integer number");
  }
  QString ceriaNo = m_uiTabCalib.lineEdit_new_ceria_num->text();
  if (ceriaNo.isEmpty()) {
    userWarning(
        "Error in the inputs",
        "The Ceria number cannot be empty and must be an integer number");
  }
  std::string fname = buildCalibrateSuggestedFilename(vanNo.toStdString(),
                                                      ceriaNo.toStdString());

  // append dir (basename) + filename
  QDir path(prevPath);
  QString suggFilename = path.filePath(QString::fromStdString(fname));

  const QString calibExt = QString("Supported formats: IPARM file for GSAS "
                                   "(*.prm *.par *.iparm);;"
                                   "Other extensions/all files (*.*)");
  QString filename = QFileDialog::getSaveFileName(
      this, tr("Please select the name of the calibration file"), suggFilename,
      calibExt);

  if (filename.isEmpty()) {
    return;
  }

  m_presenter->notify(IEnggDiffractionPresenter::CalcCalib);
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
  const QString calExt =
      QString("Comma separated values text file with calibration table "
              "(*.csv);;"
              "Nexus file with calibration table "
              "(*.nxs *.nexus);;"
              "Supported formats: CSV, NXS "
              "(*.csv *.nxs *.nexus);;"
              "Other extensions/all files (*.*)");

  QString prevPath = QString::fromStdString(m_calibSettings.m_inputDirCalib);
  if (prevPath.isEmpty()) {
    QString prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }

  QString filename = (QFileDialog::getOpenFileName(
      this, tr("Open pixel calibration (full calibration) file"), prevPath,
      calExt));

  m_calibSettings.m_pixelCalibFilename = filename.toStdString();
  m_uiTabSettings.lineEdit_pixel_calib_filename->setText(
      QString::fromStdString(m_calibSettings.m_pixelCalibFilename));
}

void EnggDiffractionViewQtGUI::browseTemplateGSAS_PRM() {
  const QString iparStr = QString("GSAS instrument parameters file "
                                  "(*.prm *.par *.tiff *.ipar *.iparam);;"
                                  "Other extensions/all files (*.*)");

  QString prevPath = QString::fromStdString(m_calibSettings.m_templateGSAS_PRM);
  QString path(QFileDialog::getOpenFileName(
      this, tr("Open GSAS IPAR template file"), prevPath, iparStr));

  if (path.isEmpty()) {
    return;
  }

  m_calibSettings.m_templateGSAS_PRM = path.toStdString();
  m_uiTabSettings.lineEdit_template_gsas_prm->setText(
      QString::fromStdString(m_calibSettings.m_templateGSAS_PRM));
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
  if (false /* TODO: get this from user settings */) {
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

  if (answer == QMessageBox::AcceptRole) {
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
