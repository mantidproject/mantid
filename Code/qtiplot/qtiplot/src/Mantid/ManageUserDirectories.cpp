#include "ManageUserDirectories.h"
#include "MantidKernel/ConfigService.h"
// #include "../ApplicationWindow.h"
#include <QtGui>

#include "../ConfigDialog.h"

ManageUserDirectories::ManageUserDirectories(QWidget *parent) : QDialog(parent)
{
	m_uiForm.setupUi(this);
  initLayout();
}

ManageUserDirectories::~ManageUserDirectories() { /* ... */ }

void ManageUserDirectories::initLayout()
{
  loadProperties();

  // Make Connections
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbManageOtherProperties, SIGNAL(clicked()), this, SLOT(otherPropertiesClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirmClicked()));

  connect(m_uiForm.pbAddDir, SIGNAL(clicked()), this, SLOT(addDataDir()));
  connect(m_uiForm.pbRemDir, SIGNAL(clicked()), this, SLOT(remDataDir()));
  connect(m_uiForm.pbMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  
  connect(m_uiForm.pbSaveBrowse, SIGNAL(clicked()), this, SLOT(selectSaveDir()));
}

void ManageUserDirectories::loadProperties()
{
  m_userPropFile = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getUserFilename()).trimmed();

  // get data search directories and populate the list widget (lwDataSearchDirs)
  QString dataDirs = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).trimmed();
  m_dataDirs = dataDirs.split(";",QString::SkipEmptyParts);
  m_uiForm.lwDataSearchDirs->clear();
  m_uiForm.lwDataSearchDirs->addItems(m_dataDirs);

  // set flag of whether to search the data archive
  QString archive = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.searcharchive")).trimmed();
  if ( archive == "On" )
    m_uiForm.ckSearchArchive->setChecked(true);
  else
    m_uiForm.ckSearchArchive->setChecked(false);

  // default save directory
  m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory")).trimmed();
  m_uiForm.leDefaultSave->setText(m_saveDir);
}
void ManageUserDirectories::saveProperties()
{
  QString newSearchArchive;
  QString newDataDirs;
  QString newSaveDir;

  if ( m_uiForm.ckSearchArchive->isChecked() )
    newSearchArchive = "On";
  else
    newSearchArchive = "Off";

  QStringList dataDirs;
  for ( int i = 0; i < m_uiForm.lwDataSearchDirs->count() ; i++ )
    dataDirs.append(m_uiForm.lwDataSearchDirs->item(i)->text());

  newDataDirs = dataDirs.join(";");
  newDataDirs.replace('\\', '/');

  newSaveDir = m_uiForm.leDefaultSave->text();
  newSaveDir.replace('\\', '/');

  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();

  config.setString("datasearch.searcharchive", newSearchArchive.toStdString());
  config.setString("datasearch.directories", newDataDirs.toStdString());
  config.setString("defaultsave.directory", newSaveDir.toStdString());
  config.saveConfig(m_userPropFile.toStdString());
}

// SLOTS
void ManageUserDirectories::helpClicked()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/ManageUserDirectories"));
}
void ManageUserDirectories::otherPropertiesClicked()
{
  ConfigDialog* dialog = new ConfigDialog(this->nativeParentWidget());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->gotoMantidDirectories();
  dialog->setFocus();
  dialog->show();
}
void ManageUserDirectories::cancelClicked()
{
  this->close();
}
void ManageUserDirectories::confirmClicked()
{
  saveProperties();
  this->close();
}
void ManageUserDirectories::addDataDir()
{
  QSettings settings;
  QString lastDirectory = settings.value("ManageUserSettings/last_directory", "").toString();

  QString newDir = QFileDialog::getExistingDirectory(this,
    tr("Select New Data Directory"),
    lastDirectory,
    QFileDialog::ShowDirsOnly );

  if ( newDir != "" )
  {
    settings.setValue("ManageUserSettings/last_directory", newDir);
    m_uiForm.lwDataSearchDirs->addItem(newDir);
  }

}
void ManageUserDirectories::remDataDir()
{
  QList<QListWidgetItem*> selected = m_uiForm.lwDataSearchDirs->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    delete selected[i];
  }
}
void ManageUserDirectories::moveUp()
{
  QList<QListWidgetItem*> selected = m_uiForm.lwDataSearchDirs->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    int index = m_uiForm.lwDataSearchDirs->row(selected[i]);
    if ( index != 0 )
    {
      QListWidgetItem* move = m_uiForm.lwDataSearchDirs->takeItem(index);
      m_uiForm.lwDataSearchDirs->insertItem(index-1, move);
    }
    m_uiForm.lwDataSearchDirs->setCurrentItem(selected[i]);
  }
}
void ManageUserDirectories::moveDown()
{
  int count = m_uiForm.lwDataSearchDirs->count();
  QList<QListWidgetItem*> selected = m_uiForm.lwDataSearchDirs->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    int index = m_uiForm.lwDataSearchDirs->row(selected[i]);
    if ( index != ( count - 1 ) )
    {
      QListWidgetItem* move = m_uiForm.lwDataSearchDirs->takeItem(index);
      m_uiForm.lwDataSearchDirs->insertItem(index+1, move);
    }
    m_uiForm.lwDataSearchDirs->setCurrentItem(selected[i]);
  }
}
void ManageUserDirectories::selectSaveDir()
{
  QSettings settings;
  QString lastDirectory = settings.value("ManageUserSettings/last_directory", "").toString();

  QString newDir = QFileDialog::getExistingDirectory(this,
    tr("Select New Default Save Directory"),
    lastDirectory,
    QFileDialog::ShowDirsOnly );

  if ( newDir != "" )
  {
    settings.setValue("ManageUserSettings/last_directory", newDir);
    m_uiForm.leDefaultSave->setText(newDir);
  }

}
