#include "FirstTimeSetup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtAPI/ManageUserDirectories.h"

#include <QDesktopServices>
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
  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(facilitySelected(const QString &)));
  connect(m_uiForm.pbMUD, SIGNAL(clicked()), this, SLOT(openManageUserDirectories()));

  connect(m_uiForm.clbReleaseNotes, SIGNAL(clicked()), this, SLOT(openReleaseNotes()));
  connect(m_uiForm.clbSampleDatasets, SIGNAL(clicked()), this, SLOT(openSampleDatasets()));
  connect(m_uiForm.clbMantidIntroduction, SIGNAL(clicked()), this, SLOT(openMantidIntroduction()));
  connect(m_uiForm.clbPythonIntroduction, SIGNAL(clicked()), this, SLOT(openPythonIntroduction()));
  connect(m_uiForm.clbPythonInMantid, SIGNAL(clicked()), this, SLOT(openPythonInMantid()));
  connect(m_uiForm.clbExtendingMantid, SIGNAL(clicked()), this, SLOT(openExtendingMantid()));

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
}

void FirstTimeSetup::confirm()
{
  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
  std::string filename = config.getUserFilename();
  config.setString("default.facility", m_uiForm.cbFacility->currentText().toStdString());
  config.setString("default.instrument", m_uiForm.cbInstrument->currentText().toStdString());
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

void FirstTimeSetup::facilitySelected(const QString & facility)
{
  Mantid::Kernel::ConfigService::Instance().setString("default.facility", facility.toStdString());
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
