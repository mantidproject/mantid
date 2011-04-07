#include "FirstTimeSetup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"

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
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirm()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(facilitySelected(const QString &)));
  connect(m_uiForm.pbMUD, SIGNAL(clicked()), this, SLOT(openManageUserDirectories()));

  // Populate list of facilities
  m_uiForm.cbFacility->clear();
  QStringList faclist = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("supported.facilities")).split(";");
  for ( QStringList::iterator it = faclist.begin(); it != faclist.end(); ++it )
  {
    m_uiForm.cbFacility->addItem(*it);
  }
}

void FirstTimeSetup::confirm()
{
  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
  std::string filename = config.getUserFilename();
  config.setString("default.facility", m_uiForm.cbFacility->currentText().toStdString());
  config.setString("default.instrument", m_uiForm.cbInstrument->currentText().toStdString());
  config.saveConfig(filename);

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