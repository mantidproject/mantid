//----------------------------------
// Includes
//----------------------------------
#include "MantidCustomActionDialog.h"
#include "../ApplicationWindow.h"
#include "MantidQtAPI/InterfaceManager.h"

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
  setWindowTitle(tr("MantidPlot") + " - " + tr("Custom Menus"));
  resize(555,390);

  m_appWindow = static_cast<ApplicationWindow*>(parent);

  //Set up the layout
  init();

  //Populate the menu list
  refreshMenuTree();

  //Populate the list of customised interfaces
  QStringList user_windows = MantidQt::API::InterfaceManager::Instance().getUserSubWindowKeys();
  QStringListIterator itr(user_windows);
  while( itr.hasNext() )
  {
    QString name = itr.next();
    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(name));
    item->setData(0, Qt::UserRole, name);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
    m_customUITree->addTopLevelItem(item);
  }

//   m_menuTree->setContextMenuPolicy(Qt::CustomContextMenu);
//   connect(m_menuTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint&)));
  
  //Use the delete key
//   QShortcut *delete_key = new QShortcut(QKeySequence(Qt::Key_Delete), this);
//   connect(delete_key, SIGNAL(activated()), this, SLOT(deleteKeyPressed()));
 
}

//----------------------------------
// Private methods
//----------------------------------
/**
 * Initialize the layout of the dialog
 */
void MantidCustomActionDialog::init()
{
  
  QVBoxLayout *menu_side = new QVBoxLayout;
  //  menu_side->addStretch();
  QGroupBox *menuSelection = new QGroupBox("Custom Menus");
  m_menuTree = new ActionTreeWidget;
  m_menuTree->setColumnCount(1);
  m_menuTree->setHeaderLabel("Name");
  m_menuTree->setSelectionMode(QAbstractItemView::SingleSelection);
  QGridLayout *menubox_layout = new QGridLayout;
  menubox_layout->addWidget(m_menuTree, 0, 0);

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

  menubox_layout->addWidget(menuButtons, 1, 0, Qt::AlignHCenter);
  menuSelection->setLayout(menubox_layout);
  menu_side->addWidget(menuSelection);
  //  menu_side->addStretch();

  //A button to import selections
  QPushButton *importBtn = new QPushButton(">>");
  importBtn->setFixedWidth(35);

  // The file list
  QGroupBox *fileSelection = new QGroupBox("Item Selection");
  QGridLayout *item_box_layout = new QGridLayout;

  m_fileTree = new ActionTreeWidget;
  m_fileTree->setColumnCount(1);
  m_fileTree->setColumnWidth(0, 25);
  m_fileTree->setIndentation(10);
  m_fileTree->setHeaderLabel("Scripts");
  m_fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  item_box_layout->addWidget(m_fileTree);

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

  item_box_layout->addWidget(fileButtons, 1, 0, Qt::AlignHCenter);

  // Custom UI tree
  m_customUITree = new ActionTreeWidget;
  m_customUITree->setColumnCount(1);
  m_customUITree->setColumnWidth(0, 25);
  m_customUITree->setIndentation(10);
  m_customUITree->setHeaderLabel("Custom Interfaces");
  m_customUITree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  item_box_layout->addWidget(m_customUITree, 2, 0);

  fileSelection->setLayout(item_box_layout);

  QHBoxLayout *top_row_layout = new QHBoxLayout;
  top_row_layout->addWidget(fileSelection);
  //I have no idea what units this is in, the documentation doesn't specify them
  top_row_layout->addSpacing(4);
  top_row_layout->addWidget(importBtn);
  top_row_layout->addSpacing(4);

  top_row_layout->addLayout(menu_side);


   //Main layout
  QVBoxLayout *mainlayout = new QVBoxLayout(this);
  mainlayout->addLayout(top_row_layout);

  QPushButton *buttonCancel = new QPushButton(tr("&Close"));
  QDialogButtonBox *buttonBox = new QDialogButtonBox;
  buttonBox->addButton(buttonCancel, QDialogButtonBox::DestructiveRole);
 
  //Connections for buttons
  connect(plusMenu, SIGNAL(clicked()), this, SLOT(addMenuClicked()));
  connect(minusMenu, SIGNAL(clicked()), this, SLOT(removeMenuClicked()));
  connect(plusFile, SIGNAL(clicked()), this, SLOT(addFileClicked()));
  connect(minusFile, SIGNAL(clicked()), this, SLOT(removeFileClicked()));
  
  //Import scripts
  connect(importBtn, SIGNAL(clicked()), this, SLOT(importAllSelected()));
  
  //Update menus and actions if a field is edited in the menu tree
  connect(m_menuTree, SIGNAL(textChange(QTreeWidgetItem*)), this, SLOT(itemTextChanged(QTreeWidgetItem*))); 
  //Close the dialog
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
  
  mainlayout->addWidget(buttonBox);

}

/**
 * Import from the script file tree
 */
void MantidCustomActionDialog::importFromFileTree()
{
  importItems(m_fileTree->selectedItems(), true);
  m_fileTree->clearSelection();
}


/** 
 * Import from the custom window tree
 */
void MantidCustomActionDialog::importFromCustomTree()
{
  importItems(m_customUITree->selectedItems(), false);
  m_customUITree->clearSelection();
}

/**
 * Import all selections
 */
void MantidCustomActionDialog::importAllSelected()
{
  importFromFileTree();
  importFromCustomTree();  
}

/**
 * Import the selections
 * @param custom_items The selected items
 * @param remove Whether to remove it from the list after the import
 */
void MantidCustomActionDialog::importItems(const QList<QTreeWidgetItem*> & custom_items, bool remove)
{
  if( custom_items.isEmpty() )
  {
    return;
  }

  QTreeWidgetItem *menu = NULL;
  if( m_menuTree->topLevelItemCount() == 1)
  {
    menu = m_menuTree->topLevelItem(0);
  }
  else if( m_menuTree->selectedItems().isEmpty() )
  {
    QMessageBox::information(this, "Import Selection", 
			     "Error: No menu has been selected");
    return;
  }
  //Single selection is the only possibility
  else
  {
    menu = m_menuTree->selectedItems()[0];
  }
  
  //If the selected item in the menu tree is a child of a menu then
  //assume the user means to select the menu
  if( menu->parent() ) menu = menu->parent();

  QTreeWidgetItem *custom;
  foreach(custom, custom_items)
  {
    QTreeWidgetItem *action = custom->clone();
    if( remove ) delete custom;
    menu->addChild(action);
    QString menuName = menu->text(0);
    QString itemName = action->text(0);
    QString action_data = action->data(0, Qt::UserRole).toString();
    m_appWindow->addUserMenuAction( menuName, itemName, action_data);
  }
  //Refresh the
  refreshMenuTree();    
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
void ActionTreeWidget::dataChanged(const QModelIndex & topLeft, const QModelIndex &)
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
