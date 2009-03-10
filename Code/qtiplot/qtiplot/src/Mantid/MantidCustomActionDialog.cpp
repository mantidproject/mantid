//----------------------------------
// Includes
//----------------------------------
#include "MantidCustomActionDialog.h"
#include "../ApplicationWindow.h"

#include <QTreeWidgetItem>
#include <QPushButton>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QCursor>
#include <QShortcut>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>

//----------------------------------
// Public methods
//----------------------------------
/**
 * Constructor
 * @param parent The parent widget
 * @param flags QT::Window flags pass to QWidget constructor
 */
MantidCustomActionDialog::MantidCustomActionDialog(QWidget* parent, Qt::WFlags flags) : 
  QDialog(parent, flags), m_lastDirectory("")
{
  setWindowTitle(tr("MantidPlot") + " - " + tr("Manage Custom Script Actions"));

  m_appWindow = static_cast<ApplicationWindow*>(parent);

  //Set up the layout
  init();

  //Populate the menu list
  refreshMenuTree();

//   m_menuTree->setContextMenuPolicy(Qt::CustomContextMenu);
//   connect(m_menuTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint&)));
  
  //Use the delete key
 //  QShortcut *accelRemove = new QShortcut(QKeySequence(Qt::Key_Delete), this);
 // connect(accelRemove, SIGNAL(activated()), this, SLOT(removeSelectedItem()));
 
}

//----------------------------------
// Private methods
//----------------------------------
/**
 * Initialize the layout of the dialog
 */
void MantidCustomActionDialog::init()
{
  //The menu tree
  m_menuTree = new ActionTreeWidget;
  m_menuTree->setColumnCount(1);
  m_menuTree->setHeaderLabel("Available Menus");
  m_menuTree->setSelectionMode(QAbstractItemView::SingleSelection);

  QGroupBox *menuSelection = new QGroupBox("Custom Menus");
  QGridLayout *leftLayout = new QGridLayout;
  leftLayout->addWidget(m_menuTree, 0, 0);

  QDialogButtonBox *menuButtons = new QDialogButtonBox;
  QPushButton *plusMenu = new QPushButton("+");
  plusMenu->setDefault(false);
  plusMenu->setAutoDefault(false);
  int buttonWidth = 25;
  plusMenu->setFixedWidth(buttonWidth);
  QPushButton *minusMenu = new QPushButton("-");
  minusMenu->setFixedWidth(buttonWidth);
  minusMenu->setDefault(false);
  minusMenu->setAutoDefault(false);
  menuButtons->addButton(plusMenu, QDialogButtonBox::ActionRole);
  menuButtons->addButton(minusMenu, QDialogButtonBox::ActionRole);

  leftLayout->addWidget(menuButtons, 1, 0, Qt::AlignHCenter);
  menuSelection->setLayout(leftLayout);

  // The file list
  m_fileTree = new ActionTreeWidget;
  m_fileTree->setColumnCount(1);
  m_fileTree->setColumnWidth(0, 25);
  m_fileTree->setIndentation(10);
  m_fileTree->setHeaderLabel("File name");
  m_fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

  //A button to import selections
  QPushButton *importBtn = new QPushButton("<<");
  importBtn->setFixedWidth(35);
  QHBoxLayout *topRowLayout = new QHBoxLayout;
  topRowLayout->addWidget(menuSelection);
  //I have no idea what units this is in, the documentation doesn't specify them
  topRowLayout->addSpacing(4);
  topRowLayout->addWidget(importBtn);
  topRowLayout->addSpacing(4);

  QGroupBox *fileSelection = new QGroupBox("Script Selection");
  QGridLayout *rightLayout = new QGridLayout;
  rightLayout->addWidget(m_fileTree, 0, 0);

  QDialogButtonBox *fileButtons = new QDialogButtonBox;
  QPushButton *plusFile = new QPushButton("+");
  plusFile->setDefault(false);
  plusFile->setAutoDefault(false);
  plusFile->setFixedWidth(buttonWidth);
  QPushButton *minusFile = new QPushButton("-");
  minusFile->setFixedWidth(buttonWidth);
  minusFile->setDefault(false);
  minusFile->setAutoDefault(false);
  fileButtons->addButton(plusFile, QDialogButtonBox::ActionRole);
  fileButtons->addButton(minusFile, QDialogButtonBox::ActionRole);

  rightLayout->addWidget(fileButtons, 1, 0, Qt::AlignHCenter);
  fileSelection->setLayout(rightLayout);
  topRowLayout->addWidget(fileSelection);
  
  //Main layout
  QVBoxLayout *mainlayout = new QVBoxLayout(this);
  mainlayout->addLayout(topRowLayout);

  QPushButton *buttonCancel = new QPushButton(tr("&Close"));
  QDialogButtonBox *buttonBox = new QDialogButtonBox;
  buttonBox->addButton(buttonCancel, QDialogButtonBox::DestructiveRole);
 
  //Connections for buttons
  connect(plusMenu, SIGNAL(clicked()), this, SLOT(addMenuClicked()));
  connect(minusMenu, SIGNAL(clicked()), this, SLOT(removeMenuClicked()));
  connect(plusFile, SIGNAL(clicked()), this, SLOT(addFileClicked()));
  connect(minusFile, SIGNAL(clicked()), this, SLOT(removeFileClicked()));
  
  //Import scripts
  connect(importBtn, SIGNAL(clicked()), this, SLOT(importSelectedScripts()));
  
  //Update menus and actions if a field is edited in the menu tree
  connect(m_menuTree, SIGNAL(textChange(QTreeWidgetItem*)), this, SLOT(itemTextChanged(QTreeWidgetItem*))); 
  //Close the dialog
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
  
  mainlayout->addWidget(buttonBox);

}

/**
 * Import the selected scripts
 */
void MantidCustomActionDialog::importSelectedScripts()
{
  QList<QTreeWidgetItem*> scripts;
  if( m_fileTree->topLevelItemCount() == 1 )
  {
    scripts.append(m_fileTree->topLevelItem(0));
  }
  else if( m_fileTree->selectedItems().isEmpty() )
  {
    QMessageBox::information(this, "Script import", 
			     "Cannot import scripts, none have been selected.");
    return;
  }
  else
  {
    scripts = m_fileTree->selectedItems();
  }

  QTreeWidgetItem *menu = NULL;
  if( m_menuTree->topLevelItemCount() == 1)
  {
    menu = m_menuTree->topLevelItem(0);
  }
  else if( m_menuTree->selectedItems().isEmpty() )
  {
    QMessageBox::information(this, "Script import", 
			     "Cannot import scripts, no menu has been selected.");
    return;
  }
  else
  {
    menu = m_menuTree->selectedItems()[0];
  }
  if( menu->parent() ) menu = menu->parent();

  QTreeWidgetItem *file;
  foreach(file, scripts)
  {
    QTreeWidgetItem *action = file->clone();
    delete file;
    menu->addChild(action);
    QString menuName = menu->text(0);
    QString itemName = action->text(0);
    m_appWindow->addUserMenuAction( menuName, itemName, action->data(0,Qt::UserRole).toString());
  }
  //Refresh the
  refreshMenuTree();    
  m_menuTree->clearSelection();
  m_fileTree->clearSelection();
}

/**
 * Remove a selected action from the menu. This does not remove the underlying script.
 */
void MantidCustomActionDialog::removeMenuClicked()
{
  if( m_menuTree->selectedItems().isEmpty() ) return;

  QTreeWidgetItem* entry = m_menuTree->selectedItems()[0];
  if( ! entry->parent() )
  {
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setText("The selected item is a top-level menu, removing this will remove all sub-items\nContinue ?");
    msgBox.setWindowTitle("Remove selected menu");
    if( msgBox.exec() != QMessageBox::Ok ) return;
    m_appWindow->removeUserMenu(entry->text(0));    
  }
  else
  {
    QMessageBox msgBox;
    msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
    msgBox.setText("Remove " + entry->text(0) + "?");
    msgBox.setWindowTitle("Remove selected item");
    if( msgBox.exec() != QMessageBox::Ok ) return;
    m_appWindow->removeUserMenuAction(entry->parent()->text(0),entry->text(0));
  }
  //  m_menuTree->removeItemWidget(entry, 0);
  delete entry;
  //  refreshMenuTree();  
}

/**
 * A slot to handle the signal sent when the 'browse' file button is clicked
 */
void MantidCustomActionDialog::addFileClicked()
{
  QString openDir = m_lastDirectory;
  if( openDir.isEmpty() ) openDir = qApp->applicationDirPath();
  QStringList newFiles = QFileDialog::getOpenFileNames(this, tr("Select one or more script files to import"),
						       openDir, tr("Python Scripts (*.py *.PY)"));
  if( newFiles.isEmpty() ) return;
  m_lastDirectory = QFileInfo(newFiles[0]).absoluteDir().path();
  addFileItems(newFiles);
}

/**
 * Remove a file that is in the file list tree
 */
void MantidCustomActionDialog::removeFileClicked()
{
  if( m_fileTree->selectedItems().isEmpty() ) return;

  QList<QTreeWidgetItem*> itemsToRemove = m_fileTree->selectedItems();
  QTreeWidgetItem* item;
  foreach( item, itemsToRemove)
  {
    delete item;
  }
}

/**
 * Add a menu
 */
void MantidCustomActionDialog::addMenuClicked()
{
  bool ok(false);
  QString name = QInputDialog::getText(this, "New menu",
				       tr("Menu name:"), QLineEdit::Normal,
				       "", &ok);
  if( !m_menuTree->findItems(name, Qt::MatchFixedString |Qt::MatchCaseSensitive ).isEmpty() ) return;
  if( ok && !name.isEmpty() ) 
  {
    m_appWindow->addUserMenu(name);
    refreshMenuTree();
  }
}

/**
 *(Re)-populate the tree of menu items based on the current layout of the map stored in the ApplicationWindow
 * object
 */
void MantidCustomActionDialog::refreshMenuTree()
{
  m_menuTree->clear();
  m_widgetMap.clear();
  
  QListIterator<QMenu*> mItr(m_appWindow->getCustomMenus());
  while( mItr.hasNext() )
  {
    QMenu *customMenu = mItr.next();
    QTreeWidgetItem *menu = new QTreeWidgetItem(QStringList(customMenu->title()));
    m_widgetMap.insert(menu, customMenu);
    menu->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
    QList<QAction*> scripts = customMenu->actions();
    QListIterator<QAction*> kItr(scripts);
    while( kItr.hasNext() )
    {
      QAction *action = kItr.next();
      QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(action->text()));
      m_widgetMap.insert(item, action);
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
      item->setData(0, Qt::UserRole, action->data().toString());
      item->setToolTip(0, action->data().toString());
      menu->addChild(item);
    }
    m_menuTree->addTopLevelItem(menu);    
  } 
}

/**
 * Add script items to the file tree
 * @param fileList A QStringList of files to add
 */
void MantidCustomActionDialog::addFileItems(const QStringList& fileList)
{
  QStringList::const_iterator iend = fileList.constEnd();
  for( QStringList::const_iterator itr = fileList.constBegin(); itr != iend; ++itr )
  {
    QString suggestedName = QFileInfo(*itr).baseName();
    if( !m_fileTree->findItems(suggestedName, Qt::MatchFixedString | Qt::MatchCaseSensitive).isEmpty() ) continue;

    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(suggestedName));
    item->setData(0, Qt::UserRole, *itr);
    item->setToolTip(0, *itr);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
    m_fileTree->insertTopLevelItem(0, item);
  }
}

void MantidCustomActionDialog::popupMenu(const QPoint & pos)
{
  if( m_menuTree->itemAt(pos) ) 
  {
    QMenu *menu = new QMenu(m_menuTree);
    
    QAction *action = new QAction("Remove item", m_menuTree);
    connect(action, SIGNAL(triggered()), this, SLOT(removeSelectedItem()));
    menu->addAction(action);
    menu->popup(QCursor::pos());
  }
  else if( m_fileTree->itemAt(pos) )
  {
  }
  else return;
  {
    m_menuTree->selectionModel()->clear();
  }
}

void MantidCustomActionDialog::itemTextChanged(QTreeWidgetItem* item)
{
  if( !m_widgetMap.contains(item) ) return;

  QString newText = item->text(0);
  QObject* entry = m_widgetMap[item];
  if( QMenu* menu = qobject_cast<QMenu*>(entry) )
  {
    menu->setTitle(newText);
  }
  else if( QAction* action = qobject_cast<QAction*>(entry) )
  {
    action->setText(newText);
  }
  else return;
}
//============================================================

//----------------------------------------------
// ActionTreeWidget public methods
//----------------------------------------------
/**
 * Default Constructor
 */
ActionTreeWidget::ActionTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
}

/**
* Data has changed in the widget
*/
void ActionTreeWidget::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
  QTreeWidgetItem *changedItem = itemFromIndex(topLeft);
  emit textChange(changedItem);
}

/**
 * Called when a mouse is clicked
 */
void ActionTreeWidget::mousePressEvent(QMouseEvent* event)
{
  if( !itemAt(mapFromGlobal(event->globalPos())) ) 
  {
    clearSelection();
  }
  QTreeWidget::mousePressEvent(event);
}
