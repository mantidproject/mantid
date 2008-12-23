//----------------------------------
// Includes
//----------------------------------
#include "MantidCustomActionDialog.h"
#include "../ApplicationWindow.h"

#include <QListWidget>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QApplication>
#include <QCursor>
#include <QShortcut>

#include <QMessageBox>
#include <iostream>

//----------------------------------
// Public methods
//----------------------------------
/**
 * Constructor
 * @param parent The parent widget
 * @param flags QT::Window flags pass to QWidget constructor
 */
MantidCustomActionDialog::MantidCustomActionDialog(QWidget* parent, Qt::WFlags flags) : 
  QDialog(parent, flags)
{
  setWindowTitle(tr("MantidPlot") + " - " + tr("Add Custom Action"));

  m_appWindow = static_cast<ApplicationWindow*>(parent);

  m_tree = new QTreeWidget();
  m_tree->setColumnCount(1);
  m_tree->setHeaderLabel("");
  m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    
  //Grid of boxes on the right
  QGroupBox *gb1 = new QGroupBox();
  gb1->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

  QGridLayout *gl1 = new QGridLayout(gb1);
  //Choose file to add to menu

  gl1->addWidget(new QLabel(tr("Single File")), 0, 0);
  fileBox = new QLineEdit;
  gl1->addWidget(fileBox, 0, 1);

  fileBtn = new QPushButton(tr("Browse ..."));
  gl1->addWidget(fileBtn, 0, 2);

  gl1->addWidget(new QLabel(tr("Entire Directory")), 1, 0);
  dirBox = new QLineEdit;
  gl1->addWidget(dirBox, 1, 1);

  dirBtn = new QPushButton(tr("Browse ..."));
  gl1->addWidget(dirBtn, 1, 2);

  gl1->addWidget(new QLabel(tr("Menu")), 2, 0);
  menuList = new QComboBox;
  gl1->addWidget(menuList, 2, 1);

  //Bottom buttons
  QHBoxLayout *bottomButtons = new QHBoxLayout();
  bottomButtons->addStretch();

  buttonAdd = new QPushButton(tr("&Add"));
  buttonAdd->setAutoDefault( true );
  buttonAdd->setToolTip("Add new scripts based upon the options given.");
  bottomButtons->addWidget(buttonAdd);

  buttonRemove = new QPushButton(tr("&Remove"));
  buttonRemove->setAutoDefault(true);
  buttonRemove->setToolTip("Remove the selected item. This does NOT delete the script(s).");
  bottomButtons->addWidget(buttonRemove);
  
  buttonCancel = new QPushButton(tr("&Close"));
  buttonCancel->setAutoDefault( true );
  buttonCancel->setToolTip("Close the dialog");
  bottomButtons->addWidget( buttonCancel );

  //Layouts
  QHBoxLayout *inputlayout = new QHBoxLayout();
  inputlayout->addWidget(m_tree);
  inputlayout->addWidget(gb1);

  QVBoxLayout *mainlayout = new QVBoxLayout(this);
  mainlayout->addLayout(inputlayout);
  mainlayout->addLayout(bottomButtons);

  //initialize buttons and things
  init();

  //Connections for buttons
  connect(fileBtn, SIGNAL(clicked()), this, SLOT(chooseFile()));
  connect(dirBtn, SIGNAL(clicked()), this, SLOT(chooseDirectory()));
  connect(buttonAdd,SIGNAL(clicked()), this, SLOT(addActions()));
  connect(buttonRemove,SIGNAL(clicked()), this, SLOT(removeSelectedItem()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));

  connect(menuList, SIGNAL(activated(const QString &)), this, SLOT(handleComboSelection(const QString &)));
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint&)));
  
  //Use the delete key
  QShortcut *accelRemove = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(accelRemove, SIGNAL(activated()), this, SLOT(removeSelectedItem()));

  
}

//----------------------------------
// Private methods
//----------------------------------
/**
 * Initialize the layout of the dialog
 */
void MantidCustomActionDialog::init()
{
  refreshScriptTree();

  QStringList menus;
  foreach(QString item, m_appWindow->getScriptMap().keys())
  {
    menus << item;
  }
  //  menus.sort();
  menuList->addItems(menus);
  //Option to create a new menu
  menuList->addItem(tr("New menu ..."));
}

/**
 * Add all of the actions specified
 */
void MantidCustomActionDialog::addActions()
{
  if( !validUserInput() ) return;

  if( menuList->currentText() == QString("New menu ...") )
  {
    bool ok(true);
    QString name = QInputDialog::getText(this, tr("Menu name input"),
					 tr("New menu name:"), QLineEdit::Normal,
					 "", &ok);
    if( !validUserInput() ) return;
  }

  if( !dirBox->text().isEmpty() )
  {
    QDir di(dirBox->text());
    QStringList filters;
    filters << "*.py" << "*.PY";
    QStringList scripts = di.entryList(filters, QDir::Files);
    QListIterator<QString> sItr(scripts);
    while( sItr.hasNext() )
    {
      addAction(di.absoluteFilePath(sItr.next()));
    }
  }
  else
  {
    addAction(fileBox->text());
  }  
}

/**
 * Add an action to a menu based on the parameters given in the dialog boxes
 */
void MantidCustomActionDialog::addAction(const QString & scriptPath)
{
  QString menuName(menuList->currentText());
  QString itemText = QFileInfo(scriptPath).baseName();
  //Check that an action of this name doesn't already exist in this menu
  if( m_appWindow->getScriptMap().value(menuName).contains(itemText) )
  {
    QMessageBox::warning(this, tr("MantidPlot") + " - " + tr("Warning"),
			 tr("Duplicate script " + itemText + " not added."));
    return;
  }
  //QAction parent pointer is set in 'addUserMenuAction'
  m_appWindow->addUserMenuAction( menuName, itemText, scriptPath);
  refreshScriptTree();
}

/**
 * Remove a selected action from the menu. This does not remove the underlying script.
 */
void MantidCustomActionDialog::removeSelectedItem()
{
  QTreeWidgetItem* entry = m_tree->selectedItems()[0];
  if( ! entry->parent() )
  {
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setText("The selected item is a top-level menu, removing this will remove all sub-items\nContinue ?");
    msgBox.setWindowTitle("MantidPlot - Remove selected menu");
    if( msgBox.exec() != QMessageBox::Ok ) return;
    m_appWindow->removeUserMenu(entry->text(0));
    menuList->removeItem(menuList->findText(entry->text(0)));
  }
  else
  {
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setText("Remove " + entry->text(0) + "?");
    msgBox.setWindowTitle("MantidPlot - Remove selected item");
    if( msgBox.exec() != QMessageBox::Ok ) return;
    m_appWindow->removeUserMenuAction(entry->parent()->text(0),entry->text(0));
  }
  m_tree->removeItemWidget(entry, 0);
  refreshScriptTree();  
}

/**
 * A slot to handle the signal sent when the 'browse' file button is clicked
 */
void MantidCustomActionDialog::chooseFile()
{
  QString path = QFileDialog::getOpenFileName(this, tr("Choose file"),
					      qApp->applicationDirPath(),	
					      tr("Python Scripts (*.py *.PY)"));
  if( path.isEmpty() ) return;
  fileBox->setText(path);
}

/**
 * A slot to handle the signal sent when the 'browse' directory button is clicked
 */
void MantidCustomActionDialog::chooseDirectory()
{
  QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
					      qApp->applicationDirPath());
  if( path.isEmpty() ) return;
  
  dirBox->setText(path);
}

/**
 * Catches the signal emitted when a selection is made from the combo box and if
 * the user requests a new menu, an input dialog is created.
 */
void MantidCustomActionDialog::handleComboSelection(const QString & text)
{
  if( text != QString("New menu ...") ) return;

  bool ok(true);
  QString name = QInputDialog::getText(this, tr("Menu name input"),
				       tr("New menu name:"), QLineEdit::Normal,
				       "", &ok);
  if( !ok || name.isEmpty() ) return;


  if( menuList->findText(name) != -1 ) return;

  int menuSize = menuList->count();
  if( menuSize == 0 ) {
    menuList->insertItem(0, name);
    menuList->setCurrentIndex(0);
  }
  else {
    menuList->insertItem(menuSize - 1, name);
    menuList->setCurrentIndex(menuSize - 1);
  }

  //Add a new menu
  m_appWindow->addUserMenu(name);
  refreshScriptTree();
 }

bool MantidCustomActionDialog::validUserInput()
{
  QString file = fileBox->text();
  QString dir = dirBox->text();
  if( !dir.isEmpty() && !file.isEmpty() )
  {
    QMessageBox::warning(m_appWindow, tr("MantidPlot") + " - " + tr("Error"),
			  tr("Both a single file and a directory have been specified, please choose one."));
    fileBox->setFocus();
    return false;
  }
  
  if ( dir.isEmpty() && !QFileInfo(file).exists() )
  {
    QMessageBox::warning(m_appWindow, tr("MantidPlot") + " - " + tr("Error"),
			  tr("The file you have specified does not exist, please choose a valid script file."));
    fileBox->setFocus();
    return false;
  }
  QDir di(dir);
  if ( file.isEmpty() && ( !di.exists() || !di.isReadable() ) )
  {
    QMessageBox::warning(m_appWindow, tr("MantidPlot") + " - " + tr("Error"),
			  tr("The directory you have specified does not exist or is not readable."));
    dirBox->setFocus();
    return false;
  }
  
  if( menuList->count() == 1 )
  {
    handleComboSelection(QString("New menu ..."));
  }
  return true;
}

/**
 *(Re)-populate the tree of scripts based on the current layout of the map stored in the ApplicationWindow
 * object
 */
void MantidCustomActionDialog::refreshScriptTree()
{
  m_tree->clear();
  QMapIterator<QString, QStringList> mItr(m_appWindow->getScriptMap());
  while( mItr.hasNext() )
  {
    mItr.next();
    QTreeWidgetItem *menu = new QTreeWidgetItem(QStringList(mItr.key()));
    QListIterator<QString> kItr(mItr.value());
    while( kItr.hasNext() )
    {
      menu->addChild(new QTreeWidgetItem(QStringList(kItr.next())));
    }
    m_tree->addTopLevelItem(menu);    
  } 
}


void MantidCustomActionDialog::popupMenu(const QPoint & pos)
{
  if( ! m_tree->itemAt(pos) ) 
  {
    m_tree->selectionModel()->clear();
    return;
  }

  QMenu *menu = new QMenu(m_tree);
  
  QAction *action = new QAction("Remove item", m_tree);
  connect(action, SIGNAL(triggered()), this, SLOT(removeSelectedItem()));
  menu->addAction(action);
  
  menu->popup(QCursor::pos());
}
