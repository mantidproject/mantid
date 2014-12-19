#include "FirstTimeSetup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtAPI/ManageUserDirectories.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>

FirstTimeSetup::FirstTimeSetup(QWidget *parent) : QDialog(parent)
{
  m_uiForm.setupUi(this);
  initLayout();
}

FirstTimeSetup::~FirstTimeSetup()
{
}

void FirstTimeSetup::initLayout()
{
  this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
  this->setWindowTitle(this->windowTitle() + " " + Mantid::Kernel::MantidVersion::version());

  setFixedSize(size());
  m_uiForm.lblVersion->setText(m_uiForm.lblVersion->text() + Mantid::Kernel::MantidVersion::version());

  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirm()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));

  connect(m_uiForm.pbMUD, SIGNAL(clicked()), this, SLOT(openManageUserDirectories()));
  connect(m_uiForm.clbReleaseNotes, SIGNAL(clicked()), this, SLOT(openReleaseNotes()));
  connect(m_uiForm.clbSampleDatasets, SIGNAL(clicked()), this, SLOT(openSampleDatasets()));
  connect(m_uiForm.clbMantidIntroduction, SIGNAL(clicked()), this, SLOT(openMantidIntroduction()));
  connect(m_uiForm.clbPythonIntroduction, SIGNAL(clicked()), this, SLOT(openPythonIntroduction()));
  connect(m_uiForm.clbPythonInMantid, SIGNAL(clicked()), this, SLOT(openPythonInMantid()));
  connect(m_uiForm.clbExtendingMantid, SIGNAL(clicked()), this, SLOT(openExtendingMantid()));

  //set first use
  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  const bool doNotShowUntilNextRelease = settings.value("DoNotShowUntilNextRelease", 0).toInt();
  settings.endGroup();
  m_uiForm.chkDoNotShowUntilNextRelease->setChecked(doNotShowUntilNextRelease);

  // Populate list of facilities
  m_uiForm.cbFacility->clear();
  auto faclist = Mantid::Kernel::ConfigService::Instance().getFacilityNames();
  for ( auto it = faclist.begin(); it != faclist.end(); ++it )
  {
    m_uiForm.cbFacility->addItem(QString::fromStdString(*it));
  }

  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
  std::string facility = config.getString("default.facility", true);
  m_uiForm.cbFacility->setCurrentIndex(m_uiForm.cbFacility->findText(
    QString::fromStdString(facility)));

  //set instrument
  std::string instrument = config.getString("default.instrument", true);
  m_uiForm.cbInstrument->updateInstrumentOnSelection(false);
  m_uiForm.cbInstrument->setCurrentIndex(m_uiForm.cbInstrument->findText(
    QString::fromStdString(instrument)));
  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(facilitySelected(const QString &)));

  //set chkAllowUsageData
  std::string isUsageReportEnabled = config.getString("usagereports.enabled", "1");
  if (isUsageReportEnabled == "0")
  {
    m_uiForm.chkAllowUsageData->setChecked(false);
  }
  connect(m_uiForm.chkAllowUsageData, SIGNAL(stateChanged (int)), this, SLOT(allowUsageDataStateChanged(int)));

  QString stlyeName = QApplication::style()->metaObject()->className();
  if(stlyeName!="QWindowsVistaStyle")
  {
    //add stylesheet formatting for other environemnts
    QString ss =  this->styleSheet();
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

void FirstTimeSetup::confirm()
{
  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
  std::string filename = config.getUserFilename();
  config.setString("default.facility", m_uiForm.cbFacility->currentText().toStdString());
  config.setString("default.instrument", m_uiForm.cbInstrument->currentText().toStdString());
  config.setString("usagereports.enabled", (m_uiForm.chkAllowUsageData->isChecked()? "1" : "0"));
  config.saveConfig(filename);
   
  QSettings settings;
  settings.beginGroup("Mantid/FirstUse");
  settings.setValue("DoNotShowUntilNextRelease", (m_uiForm.chkDoNotShowUntilNextRelease->isChecked()? 1 : 0));
  settings.setValue("LastVersion", QString::fromStdString(Mantid::Kernel::MantidVersion::releaseNotes()));
  settings.endGroup();

  // Close the dialog
  this->close();
}

void FirstTimeSetup::cancel()
{
  // Close the dialog without saving any changes
  this->close();
}

void FirstTimeSetup::allowUsageDataStateChanged(int checkedState)
{
  if (checkedState == Qt::Unchecked)
  {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Mantid: Report Usage Data ");
    msgBox.setText("Are you sure you want to disable reporting usage data?");
    msgBox.setInformativeText("All usage data is anonymous and untraceable.\n"
      "We use the usage data to inform the future development of Mantid.\n"
      "If you click \"Yes\" aspects you need risk being deprecated in "
      "future versions if we think they are not used.\n\n"
      "Are you sure you still want to disable reporting usage data?\n"
      "Please click \"No\".");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);
    
    int ret = msgBox.exec();
    if ((ret == QMessageBox::No) || (ret == QMessageBox::NoButton))
    {
      // No was clicked, or no button was clicked
      // set the checkbox back to checked
      m_uiForm.chkAllowUsageData->setCheckState(Qt::Checked);
    }
  }
}

void FirstTimeSetup::facilitySelected(const QString & facility)
{
  m_uiForm.cbInstrument->fillWithInstrumentsFromFacility(facility);
}

void FirstTimeSetup::openManageUserDirectories()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void FirstTimeSetup::openReleaseNotes()
{
  QDesktopServices::openUrl(QUrl(QString::fromStdString(Mantid::Kernel::MantidVersion::releaseNotes())));
}

void FirstTimeSetup::openSampleDatasets()
{
  QDesktopServices::openUrl(QUrl("http://download.mantidproject.org"));
}
void FirstTimeSetup::openMantidIntroduction()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Mantid_Basic_Course"));
}
void FirstTimeSetup::openPythonIntroduction()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Introduction_To_Python"));
}
void FirstTimeSetup::openPythonInMantid()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Python_In_Mantid"));
}
void FirstTimeSetup::openExtendingMantid()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Extending_Mantid_With_Python"));
}
