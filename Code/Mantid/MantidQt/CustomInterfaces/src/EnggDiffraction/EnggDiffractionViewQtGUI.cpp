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
  QString path =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  m_calibSettings.m_inputDirCalib = path.toStdString();
  m_calibSettings.m_inputDirRaw = path.toStdString();
  m_calibSettings.m_pixelCalibFilename = "";

  // +scripts/Engineering/template_ENGINX_241391_236516_North_and_South_banks.par
  Poco::Path templ =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
  templ = templ.makeParent();
  templ.append("scripts");
  templ.append("Engineering");
  templ.append("template_ENGINX_241391_236516_North_and_South_banks.par");
  m_calibSettings.m_templateGSAS_PRM = templ.toString();

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

void EnggDiffractionViewQtGUI::readSettings() {}

void EnggDiffractionViewQtGUI::saveSettings() const {}

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

  m_presenter->notify(IEnggDiffractionPresenter::LoadExistingCalib);
}

void EnggDiffractionViewQtGUI::calibrateClicked() {
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
}

void EnggDiffractionViewQtGUI::browsePixelCalibFilename() {
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
      this, tr("Open pixel calibration (full calibration) file"), prevPath,
      calExt));

  m_calibSettings.m_pixelCalibFilename = filename.toStdString();
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
