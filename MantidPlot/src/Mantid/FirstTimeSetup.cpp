// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FirstTimeSetup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "ReportUsageDisableDialog.h"

#include <QDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QUrl>

using MantidQt::API::MantidDesktopServices;

FirstTimeSetup::FirstTimeSetup(QWidget *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);
  initLayout();
}

FirstTimeSetup::~FirstTimeSetup() {}

void FirstTimeSetup::initLayout() {
  this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
  this->setWindowTitle(this->windowTitle() + " " +
                       Mantid::Kernel::MantidVersion::version());

  setFixedSize(size());
  m_uiForm.lblVersion->setText(m_uiForm.lblVersion->text() +
                               Mantid::Kernel::MantidVersion::version());

  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirm()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));

  connect(m_uiForm.pbMUD, SIGNAL(clicked()), this,
          SLOT(openManageUserDirectories()));
  connect(m_uiForm.clbReleaseNotes, SIGNAL(clicked()), this,
          SLOT(openReleaseNotes()));
  connect(m_uiForm.clbSampleDatasets, SIGNAL(clicked()), this,
          SLOT(openSampleDatasets()));
  connect(m_uiForm.clbMantidIntroduction, SIGNAL(clicked()), this,
          SLOT(openMantidIntroduction()));
  connect(m_uiForm.clbPythonIntroduction, SIGNAL(clicked()), this,
          SLOT(openPythonIntroduction()));
  connect(m_uiForm.clbPythonInMantid, SIGNAL(clicked()), this,
          SLOT(openPythonInMantid()));
  connect(m_uiForm.clbExtendingMantid, SIGNAL(clicked()), this,
          SLOT(openExtendingMantid()));

  connect(m_uiForm.lblPrivacyPolicy, SIGNAL(linkActivated(const QString &)),
          this, SLOT(openExternalLink(const QString &)));

  // set first use
  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  const bool doNotShowUntilNextRelease =
      settings.value("DoNotShowUntilNextRelease", 0).toInt();
  settings.endGroup();
  m_uiForm.chkDoNotShowUntilNextRelease->setChecked(doNotShowUntilNextRelease);

  // Populate list of facilities
  m_uiForm.cbFacility->clear();
  auto faclist = Mantid::Kernel::ConfigService::Instance().getFacilityNames();
  for (auto it = faclist.begin(); it != faclist.end(); ++it) {
    m_uiForm.cbFacility->addItem(QString::fromStdString(*it));
  }

  Mantid::Kernel::ConfigServiceImpl &config =
      Mantid::Kernel::ConfigService::Instance();
  std::string facility = config.getString("default.facility", true);
  m_uiForm.cbFacility->setCurrentIndex(
      m_uiForm.cbFacility->findText(QString::fromStdString(facility)));

  // set instrument
  std::string instrument = config.getString("default.instrument", true);
  m_uiForm.cbInstrument->updateInstrumentOnSelection(false);
  m_uiForm.cbInstrument->setCurrentIndex(
      m_uiForm.cbInstrument->findText(QString::fromStdString(instrument)));
  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(facilitySelected(const QString &)));

  // set chkAllowUsageData
  std::string isUsageReportEnabled =
      config.getString("usagereports.enabled", true);
  if (isUsageReportEnabled == "0") {
    m_uiForm.chkAllowUsageData->setChecked(false);
  }
  connect(m_uiForm.chkAllowUsageData, SIGNAL(stateChanged(int)), this,
          SLOT(allowUsageDataStateChanged(int)));

  QString stlyeName = QApplication::style()->metaObject()->className();
  if ((stlyeName == "QMotifStyle") || (stlyeName == "QCDEStyle")) {
    // add stylesheet formatting for other environments
    QString ss = this->styleSheet();
    ss += "\n"
          "QDialog#FirstTimeSetup QCommandLinkButton {"
          " background-color: rgba(255, 255, 255, 0);"
          "  border-radius: 15px;"
          "}"
          "\n"
          "QDialog#FirstTimeSetup QCommandLinkButton:hover {"
          "  background-color: rgba(255, 255, 255, 128);"
          "}";
    this->setStyleSheet(ss);
  }
}

void FirstTimeSetup::confirm() {
  Mantid::Kernel::ConfigServiceImpl &config =
      Mantid::Kernel::ConfigService::Instance();
  std::string filename = config.getUserFilename();
  config.setString("default.facility",
                   m_uiForm.cbFacility->currentText().toStdString());
  config.setString("default.instrument",
                   m_uiForm.cbInstrument->currentText().toStdString());
  config.setString("usagereports.enabled",
                   (m_uiForm.chkAllowUsageData->isChecked() ? "1" : "0"));
  config.saveConfig(filename);

  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  settings.setValue(
      "DoNotShowUntilNextRelease",
      (m_uiForm.chkDoNotShowUntilNextRelease->isChecked() ? 1 : 0));
  settings.setValue(
      "LastVersion",
      QString::fromStdString(Mantid::Kernel::MantidVersion::releaseNotes()));
  settings.endGroup();

  // Close the dialog
  this->close();
}

void FirstTimeSetup::cancel() {
  // Close the dialog without saving any changes
  this->close();
}

void FirstTimeSetup::allowUsageDataStateChanged(int checkedState) {
  if (checkedState == Qt::Unchecked) {
    auto widget = new ReportUsageDisableDialog(this);

    int ret = widget->exec();
    if ((ret == QMessageBox::No) || (ret == QMessageBox::NoButton)) {
      // No was clicked, or no button was clicked
      // set the checkbox back to checked
      m_uiForm.chkAllowUsageData->setCheckState(Qt::Checked);
    }
  }
}

void FirstTimeSetup::facilitySelected(const QString &facility) {
  m_uiForm.cbInstrument->fillWithInstrumentsFromFacility(facility);
}

void FirstTimeSetup::openManageUserDirectories() {
  MantidQt::API::ManageUserDirectories *ad =
      new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
  // cppcheck-suppress memleak
}

void FirstTimeSetup::openReleaseNotes() {
  MantidQt::API::HelpWindow::showPage(
      this, Mantid::Kernel::MantidVersion::releaseNotes());
}

void FirstTimeSetup::openSampleDatasets() {
  MantidDesktopServices::openUrl(QUrl("http://download.mantidproject.org"));
}
void FirstTimeSetup::openMantidIntroduction() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/Mantid_Basic_Course"));
}
void FirstTimeSetup::openPythonIntroduction() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/Introduction_To_Python"));
}
void FirstTimeSetup::openPythonInMantid() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/Python_In_Mantid"));
}
void FirstTimeSetup::openExtendingMantid() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/Extending_Mantid_With_Python"));
}
void FirstTimeSetup::openExternalLink(const QString &link) {
  MantidDesktopServices::openUrl(link);
}
