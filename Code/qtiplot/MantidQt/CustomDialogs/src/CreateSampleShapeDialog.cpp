//---------------------------
// Includes
//--------------------------
#include "MantidQtCustomDialogs/CreateSampleShapeDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomDialogs/SampleShapeHelpers.h"

#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QShortcut>
#include <QCloseEvent>

#include <iostream>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(CreateSampleShapeDialog);
}
}

// Just to save writing this everywhere 
using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
CreateSampleShapeDialog::CreateSampleShapeDialog(QWidget *parent) :
  AlgorithmDialog(parent), m_setup_map(), m_details_map(), m_ops_map()
{
}

/**
 * Destructor
 */
CreateSampleShapeDialog::~CreateSampleShapeDialog()
{
  // Delete the objects created. The shape details static counter is decremented when
  // its destructor is called
  QMutableMapIterator<BinaryTreeWidgetItem*, ShapeDetails*> itr(m_details_map);
  while( itr.hasNext() ) 
  {
    itr.next();
    ShapeDetails* obj = itr.value();
    itr.remove();
    delete obj;
  }
  QMutableMapIterator<BinaryTreeWidgetItem*, Operation*> itrb(m_ops_map);
  while( itrb.hasNext() ) 
  {
    itrb.next();
    Operation *obj = itrb.value();
    itrb.remove();
    delete obj;
  }
}

/**
 * Set up the dialog
 */
void CreateSampleShapeDialog::initLayout()
{
  //The main setup function
  m_uiForm.setupUi(this);
  
  m_setup_map.clear();
  m_setup_map["sphere"] = new ShapeDetailsInstantiator<SphereDetails>;
  m_setup_map["cylinder"] = new ShapeDetailsInstantiator<CylinderDetails>;
  m_setup_map["infinite cylinder"] = new ShapeDetailsInstantiator<InfiniteCylinderDetails>();


  //The binary tree
  m_shapeTree = new BinaryTreeWidget(this);
  m_shapeTree->setColumnCount(1);
  m_shapeTree->setHeaderLabel("");
  m_shapeTree->setContextMenuPolicy(Qt::CustomContextMenu);
  m_shapeTree->setSelectionBehavior(QAbstractItemView::SelectItems);
  m_shapeTree->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(m_shapeTree, SIGNAL(customContextMenuRequested(const QPoint &)), 
	  this, SLOT(handleTreeContextMenuRequest(const QPoint &)));
  connect(m_shapeTree, SIGNAL(itemSelectionChanged()), this, SLOT(setupDetailsBox()));
  connect(m_shapeTree, SIGNAL(treeDataChange(BinaryTreeWidgetItem*, int)), this, 
	  SLOT(changeTreeData(BinaryTreeWidgetItem*, int)));

  QPushButton *add_btn = new QPushButton("Add");
  QMenu *add_menu = new QMenu(add_btn);
  QMenu *add_op = new QMenu("Operation");
  add_op->addAction(new QAction("intersection", add_op));
  add_op->addAction(new QAction("union", add_op));
  add_op->addAction(new QAction("difference", add_op));
  connect(add_op, SIGNAL(triggered(QAction*)), this, SLOT(addOperation(QAction*)));
  add_menu->addMenu(add_op);
    
  QMenu *add_shape = new QMenu("Child Shape");
  QStringList shapes = m_setup_map.keys();
  QStringListIterator itr(shapes);
  while( itr.hasNext() )
  {
    add_shape->addAction(new QAction(itr.next(), add_shape));
  }
  connect(add_shape, SIGNAL(triggered(QAction*)), this, SLOT(addShape(QAction*)));
  add_menu->addMenu(add_shape);
  add_btn->setMenu(add_menu);

  QHBoxLayout *bottom = new QHBoxLayout;
  bottom->addWidget(add_btn);
  bottom->addStretch();

  //Shape box layout
  QVBoxLayout *shape_box_layout = new QVBoxLayout;
  shape_box_layout->addWidget(m_shapeTree);
  shape_box_layout->addLayout(bottom);
  m_uiForm.shape_box->setLayout(shape_box_layout);
  
  QShortcut *delete_key = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(delete_key, SIGNAL(activated()), this, SLOT(handleDeleteRequest()));

  // Check input workspace property. If there are available workspaces then
  // these have been set as allowed values
  std::vector<std::string> workspaces = getAlgorithmProperty("InputWorkspace")->allowedValues();
  for( std::vector<std::string>::const_iterator itr = workspaces.begin(); itr != workspaces.end(); ++itr )
  {
    m_uiForm.wksp_opt->addItem(QString::fromStdString(*itr));
  }

  QLabel *validlbl = getValidatorMarker("InputWorkspace");
  m_uiForm.bottomlayout->insertWidget(2, validlbl);
}

/**
 * Retrieve the input from the dialog
 */
void CreateSampleShapeDialog::parseInput()
{
  QString shapexml;
  // First construct the XML that builds each separately defined shape
  QMapIterator<BinaryTreeWidgetItem*, ShapeDetails*> detitr(m_details_map);
  while( detitr.hasNext() )
  {
    detitr.next();
    shapexml += detitr.value()->writeXML() + "\n";
  }
  
  QList<BinaryTreeWidgetItem*> postfix_exp;
  //Build expression list
  m_shapeTree->traverseInPostOrder(m_shapeTree->root(), postfix_exp);

  QListIterator<BinaryTreeWidgetItem*> expitr(postfix_exp);
  QStringList inter_results;
  while( expitr.hasNext() )
  {
    BinaryTreeWidgetItem* item = expitr.next();
    if( m_details_map.contains(item) )
    {
      inter_results.append(m_details_map.value(item)->getShapeID());
    }
    else if( m_ops_map.contains(item) )
    {
      int rcount = inter_results.count();
      QString left = inter_results.at(rcount - 2);
      QString right = inter_results.at(rcount - 1);
      QString result = m_ops_map.value(item)->toString(left, right);
      // Remove left result and replace the right with the result
      inter_results.removeAt(rcount - 2);
      //List has now been reduced in size by 1
      inter_results.replace(rcount - 2, result);
    }
    else
    {
      shapexml = "";
      break;
    }
  }
  
  if( shapexml.isEmpty() ) return;
  assert( inter_results.size() == 1 );
  //  std::cerr << inter_results.at(0).toStdString() << "\n";

  shapexml += "<algebra val=\"" + inter_results.at(0) + "\" />";
  addPropertyValueToMap("ShapeXML", shapexml);
  
  // Get workspace value
  addPropertyValueToMap("InputWorkspace", m_uiForm.wksp_opt->currentText());
}

/**
 * This slot is called when a context menu is requested inside the tree widget
 */
void CreateSampleShapeDialog::handleTreeContextMenuRequest(const QPoint & pos)
{
  QMenu *context_menu = new QMenu(m_shapeTree);
  //pos is in widget coordinates
  //  QTreeWidgetItem *item = m_shapeTree->itemAt(pos);
  //  if( !item ) return;
  
  QMenu *add_op = new QMenu("Add operation");
  add_op->addAction(new QAction("intersection", add_op));
  add_op->addAction(new QAction("union", add_op));
  add_op->addAction(new QAction("difference", add_op));
  connect(add_op, SIGNAL(triggered(QAction*)), this, SLOT(addOperation(QAction*)));
  context_menu->addMenu(add_op);

  QMenu *submenu = new QMenu("Add child shape");
  QStringList shapes = m_setup_map.keys();
  QStringListIterator itr(shapes);
  while( itr.hasNext() )
  {
    submenu->addAction(new QAction(itr.next(), submenu));
  }
  connect(submenu, SIGNAL(triggered(QAction*)), this, SLOT(addShape(QAction*)));
  
  context_menu->addMenu(submenu);
  context_menu->addSeparator();

  QAction *remove = new QAction("Delete", context_menu);
  connect(remove, SIGNAL(triggered()), this, SLOT(handleDeleteRequest()));
  context_menu->addAction(remove);

  context_menu->popup(QCursor::pos());
}

void CreateSampleShapeDialog::changeTreeData(BinaryTreeWidgetItem* item, int data)
{
  if( m_ops_map.contains(item) )
  {
    m_ops_map.value(item)->binaryop = data;
  }
}

BinaryTreeWidgetItem* CreateSampleShapeDialog::getSelectedItem()
{
  if( !m_shapeTree->selectedItems().isEmpty() )
  {
    /// Single selections are the only ones allowed
    return dynamic_cast<BinaryTreeWidgetItem*>(m_shapeTree->selectedItems()[0]);
  }
  
  // Check if tree is empty
  if( m_shapeTree->topLevelItemCount() == 0 )
  {
    // Give back the invisible root item
    return m_shapeTree->root();
  }
  else
  {
    QMessageBox::information(this, "CreateSampleShape", "Please select an item in the list as a parent.");
  }
  return NULL;
}

/**
 * Add a new child shape
 * @param shape The action that emitted the signal
 */
void CreateSampleShapeDialog::addShape(QAction *shape)
{
  // Get the selected item
  BinaryTreeWidgetItem *parent = getSelectedItem();
  if( parent && parent->childCount() == 2 ) return;

  BinaryTreeWidgetItem *child = new BinaryTreeWidgetItem(QStringList(shape->text()));
  child->setFlags(child->flags() & ~Qt::ItemIsEditable);

  if( m_shapeTree->topLevelItemCount() == 0 )
  {
    m_shapeTree->insertTopLevelItem(0, child);
  }
  else
  {
    parent->addChildItem(child);
  }

  // This calls setupDetails
  m_shapeTree->setCurrentItem(child);
  m_shapeTree->expandAll();
}

/** 
 * Add operation node based on menu action
 */
void CreateSampleShapeDialog::addOperation(QAction *opt)
{
  //Get the selected item
  BinaryTreeWidgetItem *parent = getSelectedItem();
  if( parent && parent->childCount() == 2 ) return;

//   if( !m_ops_map.contains(parent) )
//   {
//     QMessageBox::information(this, "CreateSampleShape", "An operation must be the child of an operation.");  
//     return;
//   }
  
  BinaryTreeWidgetItem *child = new BinaryTreeWidgetItem;
  QFont font = child->font(0);
  font.setBold(true);
  child->setFont(0, font);
  child->setData(0, Qt::DisplayRole, opt->text());
  int opcode(0);
  if( opt->text().startsWith("u") ) opcode = 1;
  else if( opt->text().startsWith("d") ) opcode = 2;
  else opcode = 0;

  child->setData(0, Qt::UserRole, opcode);
  child->setFlags(child->flags() | Qt::ItemIsEditable);
  
  if( m_shapeTree->topLevelItemCount() == 0 )
  {
    m_shapeTree->insertTopLevelItem(0, child);
  }
  else
  {
    parent->addChildItem(child);
  }

  m_ops_map.insert(child, new Operation(opcode));
  // This calls setupDetails if necessary
  m_shapeTree->setCurrentItem(child);
  m_shapeTree->expandAll();
  
}

/**
 * Handle a delete signal
 */
void CreateSampleShapeDialog::handleDeleteRequest()
{
  BinaryTreeWidgetItem *item = getSelectedItem();
  if( !item ) return;
  removeItem(item);
}

/**
 * Remove an item and all children from the tree
 */
void CreateSampleShapeDialog::removeItem(BinaryTreeWidgetItem *item)
{
  if( !item ) return;

  //Recursively remove children
  if( item->childCount() > 0 ) 
  {
    while( item->childCount() > 0 )
    {
      if( item->leftChild() ) removeItem(item->leftChild());
      if( item->rightChild() ) removeItem(item->rightChild());
    }
  }
  
  if( m_details_map.contains(item) )
  {
    ShapeDetails *obj = m_details_map.take(item);
    delete obj;
  }
  else if( m_ops_map.contains(item) )
  {
    Operation *obj = m_ops_map.take(item);
    delete obj;
  }
  else return;

  if( item->parent() )
  {
    item->parent()->removeChild(item);
  }
  else
  {
    m_shapeTree->takeTopLevelItem(m_shapeTree->indexOfTopLevelItem(item));
  }
}

/**
 * Setup the layout for the details box based upon the item given
 */
void CreateSampleShapeDialog::setupDetailsBox()
{
  QList<QTreeWidgetItem*> selection = m_shapeTree->selectedItems();
  if( selection.isEmpty() ) return;
  
  // Remove the current widget if one exists in the scroll area
  if( m_uiForm.details_scroll->widget() ) m_uiForm.details_scroll->takeWidget();

  BinaryTreeWidgetItem *item = dynamic_cast<BinaryTreeWidgetItem*>(selection[0]);
  QString shapename = item->text(0);
  if( m_setup_map.contains(shapename) )
  {
    ShapeDetails *obj = NULL; 
    if( m_details_map.contains(item) )
    {
      obj = m_details_map.value(item);
    }
    else 
    {
      obj = createDetailsWidget(shapename);
      m_details_map.insert(item, obj);
    }
    //Set it as the currently displayed widget
    m_uiForm.details_scroll->setWidget(obj);    
  }
  
}

/**
 * Create the correct type of details box for the shape
 * @param shapename The name of the shape for which to create a widget 
 * @return A pointer to the details object
 */
ShapeDetails* CreateSampleShapeDialog::createDetailsWidget(const QString & shapename) const
{
  if( m_setup_map.contains(shapename) )
  {
    return m_setup_map.value(shapename)->createInstance();
  }
  return NULL;
}

//=================================================================
//=================================================================

//------------------------------------------------
// BinaryTreeWidgetItem
//------------------------------------------------
/**
 * Default constructor
 * @param type The type of the item
 */
BinaryTreeWidgetItem::BinaryTreeWidgetItem(int type) 
  : QTreeWidgetItem(type), m_left_index(0), m_right_index(1)
{
}

/**
 * Construct an item with a string list of column texts to add
 * @param A list of strings to appear as the column texts
 * @param type Qt or User defined 
 */
BinaryTreeWidgetItem::BinaryTreeWidgetItem(const QStringList & strings, int type) 
  : QTreeWidgetItem(strings, type), m_left_index(0), m_right_index(1)
{
}

/**
 * Add a child item. This will only succeed if there are fewer than 2 children currently
 */
bool BinaryTreeWidgetItem::addChildItem(BinaryTreeWidgetItem* child)
{
  if( childCount() >= 2 ) return false;
  
  // Call sub-class function
  this->addChild(child);
  return true;
}

/**
 * A pointer to the left child. It can be NULL
 */
BinaryTreeWidgetItem* BinaryTreeWidgetItem::leftChild() const
{
  return dynamic_cast<BinaryTreeWidgetItem*>(this->child(m_left_index));
}

/**
 * A pointer to the right child. It can be NULL
 */
BinaryTreeWidgetItem* BinaryTreeWidgetItem::rightChild() const
{
  return dynamic_cast<BinaryTreeWidgetItem*>(this->child(m_right_index));
}

//------------------------------------------------
// BinaryTreeWidget
//------------------------------------------------

/**
 * Default constructor
 */
BinaryTreeWidget::BinaryTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
  ComboBoxDelegate *delegate = new ComboBoxDelegate(this);
  setItemDelegate(delegate);
}

/**
 * Gets the root item for the tree
 */
BinaryTreeWidgetItem* BinaryTreeWidget::root() const
{
  return dynamic_cast<BinaryTreeWidgetItem*>(invisibleRootItem()->child(0));
}

/**
 * A recursive function that builds an expression from the binary tree by traversing it in a post order fashion
 * @param node The parent node
 * @param expression The expression list to build
 */
void BinaryTreeWidget::traverseInPostOrder(BinaryTreeWidgetItem* node, QList<BinaryTreeWidgetItem*> & expression)
{
  if( !node ) return;
  //  For the time begin just print the string that we get
  if( node->leftChild() ) traverseInPostOrder(node->leftChild(), expression);
  if( node->rightChild() ) traverseInPostOrder(node->rightChild(), expression);

  // Append this to the list
  expression.append(node);
}

void BinaryTreeWidget::dataChanged(const QModelIndex & topLeft, const QModelIndex &)
{
  emit treeDataChange(dynamic_cast<BinaryTreeWidgetItem*>(itemFromIndex(topLeft)), 
		      topLeft.data(Qt::UserRole).toInt());
}

//------------------------------------------------
// ComboBoxDelegate
//------------------------------------------------
/**
 * Default constructor
 */
ComboBoxDelegate::ComboBoxDelegate(QWidget *parent) : QItemDelegate(parent) 
{
}

/**
 * Create an editor for a tree item
 * @param parent The parent widget
 */
QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
					  const QModelIndex &) const
{
  QComboBox *editor = new QComboBox(parent);
  editor->addItem("intersection");
  editor->addItem("union");
  editor->addItem("difference");
  
  return editor;
}

/**
 * Set the data for the editor when it has been created
 * @param editor The editor in question
 * @param index The model item in question
 */
void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  int value = index.model()->data(index, Qt::UserRole).toInt();
  
  QComboBox *combo_box = qobject_cast<QComboBox*>(editor);
  combo_box->setCurrentIndex(value);
}

/**
 * Set the data for the model when editing is finished
 * @param editor The editor in question
 * @param model The model in question
 * @param index The index for the model given
 */

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
				   const QModelIndex &index) const
{
  QComboBox *combo_box = static_cast<QComboBox*>(editor);
  int boxitem = combo_box->currentIndex();
  QString value = combo_box->itemText(boxitem);

  model->setData(index, boxitem, Qt::UserRole);
  model->setData(index, value, Qt::DisplayRole);
}

/**
 * Set the appropriate geometry for the widget
 * @param editor The editor in question
 * @param option The style option
 */
void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, 
					   const QModelIndex &) const
{
  editor->setGeometry(option.rect);
}
