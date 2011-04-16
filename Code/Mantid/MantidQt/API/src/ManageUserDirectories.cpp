#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidKernel/ConfigService.h"
#include <QtGui>

using namespace MantidQt::API;

ManageUserDirectories::ManageUserDirectories(QWidget *parent) : QDialog(parent)
{
  setAttribute(Qt::WA_DeleteOnClose);
	m_uiForm.setupUi(this);
  initLayout();
}

ManageUserDirectories::~ManageUserDirectories() {}

void ManageUserDirectories::initLayout()
{
  loadProperties();

  // Make Connections
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirmClicked()));

  connect(m_uiForm.pbAddDirectory, SIGNAL(clicked()), this, SLOT(addDirectory()));
  connect(m_uiForm.pbAddDirectoryPython, SIGNAL(clicked()), this, SLOT(addDirectory()));
  connect(m_uiForm.pbBrowseToDir, SIGNAL(clicked()), this, SLOT(browseToDirectory()));
  connect(m_uiForm.pbBrowseToDirPython, SIGNAL(clicked()), this, SLOT(browseToDirectory()));
  connect(m_uiForm.pbRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbRemDirPython, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbMoveUpPython, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbMoveDownPython, SIGNAL(clicked()), this, SLOT(moveDown()));
  
  connect(m_uiForm.pbSaveBrowse, SIGNAL(clicked()), this, SLOT(selectSaveDir()));
}

void ManageUserDirectories::loadProperties()
{
  m_userPropFile = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getUserFilename()).trimmed();

  // get data search directories and populate the list widget (lwDataSearchDirs)
  QString directories = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).trimmed();
  QStringList list = directories.split(";",QString::SkipEmptyParts);
  m_uiForm.lwDataSearchDirs->clear();
  m_uiForm.lwDataSearchDirs->addItems(list);

  // Do the same thing for the "usersearch.directories" property.
  directories = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("usersearch.directories")).trimmed();
  list = directories.split(";", QString::SkipEmptyParts);
  m_uiForm.lwUserSearchDirs->clear();
  m_uiForm.lwUserSearchDirs->addItems(list);

  // set flag of whether to search the data archive
  QString archive = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.searcharchive")).trimmed();
  if ( archive == "On" )
    m_uiForm.ckSearchArchive->setChecked(true);
  else
    m_uiForm.ckSearchArchive->setChecked(false);

  // default save directory
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory")).trimmed();
  m_uiForm.leDefaultSave->setText(saveDir);
}
void ManageUserDirectories::saveProperties()
{
  QString newSearchArchive;
  QString newDataDirs;
  QString newUserDirs;
  QString newSaveDir;

  if ( m_uiForm.ckSearchArchive->isChecked() )
    newSearchArchive = "On";
  else
    newSearchArchive = "Off";

  QStringList dataDirs;
  QStringList userDirs;

  for ( int i = 0; i < m_uiForm.lwDataSearchDirs->count(); i++ )
  {
    dataDirs.append(m_uiForm.lwDataSearchDirs->item(i)->text());
  }

  for ( int i = 0; i < m_uiForm.lwUserSearchDirs->count(); i++ )
  {
    userDirs.append(m_uiForm.lwUserSearchDirs->item(i)->text());
  }

  newDataDirs = dataDirs.join(";");
  newUserDirs = userDirs.join(";");
  newDataDirs.replace('\\', '/');
  newUserDirs.replace('\\', '/');

  newSaveDir = m_uiForm.leDefaultSave->text();
  newSaveDir.replace('\\', '/');

  Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();

  config.setString("datasearch.searcharchive", newSearchArchive.toStdString());
  config.setString("datasearch.directories", newDataDirs.toStdString());
  config.setString("defaultsave.directory", newSaveDir.toStdString());
  config.setString("usersearch.directories", newUserDirs.toStdString());
  config.saveConfig(m_userPropFile.toStdString());
}

QListWidget* ManageUserDirectories::listWidget()
{
  if ( m_uiForm.tabWidget->currentWidget() == m_uiForm.tabDataSearch )
  {
    return m_uiForm.lwDataSearchDirs;
  }
  else if ( m_uiForm.tabWidget->currentWidget() == m_uiForm.tabPythonDirectories )
  {
    return m_uiForm.lwUserSearchDirs;
  }
  else
  {
    return NULL;
  }
}

// SLOTS
void ManageUserDirectories::helpClicked()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/ManageUserDirectories"));
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

void ManageUserDirectories::addDirectory()
{
  QLineEdit* input(NULL);
  
  if ( m_uiForm.tabWidget->currentWidget() == m_uiForm.tabDataSearch )
  {
    input = m_uiForm.leDirectoryPath;
  }
  else if ( m_uiForm.tabWidget->currentWidget() == m_uiForm.tabPythonDirectories )
  {
    input = m_uiForm.leDirectoryPathPython;
  }

  if ( input->text() != "" )
  {
    listWidget()->addItem(input->text());
    input->clear();
  }
}

void ManageUserDirectories::browseToDirectory()
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
    listWidget()->addItem(newDir);
  }

}
void ManageUserDirectories::remDir()
{
  QList<QListWidgetItem*> selected = listWidget()->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    delete selected[i];
  }
}
void ManageUserDirectories::moveUp()
{
  QListWidget* list = listWidget();
  QList<QListWidgetItem*> selected = list->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    int index = list->row(selected[i]);
    if ( index != 0 )
    {
      QListWidgetItem* move = list->takeItem(index);
      list->insertItem(index-1, move);
    }
    list->setCurrentItem(selected[i]);
  }
}
void ManageUserDirectories::moveDown()
{
  QListWidget* list = listWidget();
  int count = list->count();
  QList<QListWidgetItem*> selected = list->selectedItems();
  for ( int i = 0; i < selected.size(); i++ )
  {
    int index = list->row(selected[i]);
    if ( index != ( count - 1 ) )
    {
      QListWidgetItem* move = list->takeItem(index);
      list->insertItem(index+1, move);
    }
    list->setCurrentItem(selected[i]);
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
/** Opens a manage directories dialog and gives it focus
*  @param the :: parent window, probably the window that called it
*/
void ManageUserDirectories::openUserDirsDialog(QWidget * parent)
{
  ManageUserDirectories *ad = new ManageUserDirectories(parent);
  ad->show();
  ad->setFocus();
}
