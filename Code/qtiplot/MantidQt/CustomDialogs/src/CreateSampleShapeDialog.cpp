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
  AlgorithmDialog(parent), m_setup_functions(), m_details_map()
{
}

/**
 * Destructor
 */
CreateSampleShapeDialog::~CreateSampleShapeDialog()
{
  QMutableMapIterator<BinaryTreeWidgetItem*, ShapeDetails*> itr(m_details_map);
  while( itr.hasNext() ) {
    itr.next();
    ShapeDetails* obj = itr.value();
    itr.remove();
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
  
  //Setup the function pointer map
  m_setup_functions.clear();
  m_setup_functions["sphere"] = &CreateSampleShapeDialog::setupSphereDetails;
  m_setup_functions["cylinder"] = &CreateSampleShapeDialog::setupCylinderDetails;

  //The binary tree
  m_shapeTree = new BinaryTreeWidget(this);
  m_shapeTree->setColumnCount(1);
  m_shapeTree->setHeaderLabel("");
  m_shapeTree->setContextMenuPolicy(Qt::CustomContextMenu);
  m_shapeTree->insertTopLevelItem(0, new BinaryTreeWidgetItem(QStringList("complete-shape")));
  m_shapeTree->setSelectionBehavior(QAbstractItemView::SelectItems);
  connect(m_shapeTree, SIGNAL(customContextMenuRequested(const QPoint &)), 
	  this, SLOT(handleTreeContextMenuRequest(const QPoint &)));
  connect(m_shapeTree, SIGNAL(itemSelectionChanged()), this, SLOT(setupDetailsBox()));
  
  m_uiForm.shape_box->layout()->addWidget(m_shapeTree);
}

/**
 * Retrieve the input from the dialog
 */
void CreateSampleShapeDialog::parseInput()
{
  BinaryTreeWidgetItem* root_item = m_shapeTree->root();
  m_shapeTree->traverseByPreorder(root_item);

}

/**
 * This slot is called when a context menu is requested inside the tree widget
 */
void CreateSampleShapeDialog::handleTreeContextMenuRequest(const QPoint & pos)
{
  QMenu *context_menu = new QMenu(m_shapeTree);
  //pos is in widget coordinates
  QTreeWidgetItem *item = m_shapeTree->itemAt(pos);
  if( !item ) return;
  
  QMenu *submenu = new QMenu("Add child shape");
  QStringList shapes = m_setup_functions.keys();
  QStringListIterator itr(shapes);
  while( itr.hasNext() )
  {
    submenu->addAction(new QAction(itr.next(), submenu));
  }

  connect(submenu, SIGNAL(triggered(QAction*)), this, SLOT(addChildShape(QAction*)));
  
  context_menu->addMenu(submenu);
  context_menu->popup(QCursor::pos());
}

/**
 * Add a new child shape
 * @param shape The action that emitted the signal
 */
void CreateSampleShapeDialog::addChildShape(QAction *shape)
{
  //Get the selected item
  BinaryTreeWidgetItem *parent = dynamic_cast<BinaryTreeWidgetItem*>(m_shapeTree->selectedItems()[0]);
  if( parent->childCount() == 2 ) return;

  BinaryTreeWidgetItem *child = new BinaryTreeWidgetItem(QStringList(shape->text()));
  //Bit-wise AND with negated value of Qt::ItemIsEditable i.e. disable editing
  child->setFlags(parent->flags() & ~Qt::ItemIsEditable);
  parent->addChildItem(child);

  //Reset properites of the parent to reflect the fact that it is not a primitive shape
  QFont font = parent->font(0);
  font.setBold(true);
  parent->setFont(0, font);
  parent->setData(0, Qt::DisplayRole, "intersection");
  parent->setData(0, Qt::UserRole, 0);
  parent->setFlags(parent->flags() | Qt::ItemIsEditable);

  m_shapeTree->setCurrentItem(child);
  m_shapeTree->expandAll();
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
  if( m_setup_functions.contains(shapename) )
  {
    ShapeDetails *obj = NULL; 
    if( m_details_map.contains(item) )
    {
      obj = m_details_map.value(item);
    }
    else 
    {
      // MemFuncGetter is a typedef for a function pointer
      MemFuncGetter details_func = m_setup_functions.value(shapename);
      // The '->*' operator has low precedence, hence the need for brackets
      obj = (this->*details_func)();
      m_details_map.insert(item, obj);
    }
    //Set it as the currently displayed widget
    m_uiForm.details_scroll->setWidget(obj);    
  }
  
}

//---------------------------------------------
// Details tab set up functions
//--------------------------------------------
/**
 * Setup the details box for a sphere
 * @param item The sphere item in the list
 * @return A pointer to the details object
 */
ShapeDetails* CreateSampleShapeDialog::setupSphereDetails() const
{
  return new SphereDetails;
}

/**
 * Setup the details box for a cylinder
 * @param item The sphere item in the list
 */
ShapeDetails* CreateSampleShapeDialog::setupCylinderDetails() const
{
  return new CylinderDetails;
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
 * Recurse through the tree in a pre-order manner
 */
void BinaryTreeWidget::traverseByPreorder(BinaryTreeWidgetItem* node)
{
  // For the time begin just print the string that we get
  QString itext = node->text(0);
  if( itext.startsWith("i") ) itext = "x";
  else if( itext.startsWith("u") ) itext = "+";
  else if( itext.startsWith("d") ) itext = "-";
  else {}

  std::cerr << itext.toStdString() << " ";
  if( node->leftChild() ) traverseByPreorder(node->leftChild());
  if( node->rightChild() ) traverseByPreorder(node->rightChild());
}


//------------------------------------------------
// ComboBoxDelegate
//------------------------------------------------
/// A user defined data type
int ComboBoxDelegate::g_idata_role = 100;

/**
 * Default constructor
 */
ComboBoxDelegate::ComboBoxDelegate(QWidget *parent) : QItemDelegate(parent) 
{
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
					  const QModelIndex &) const
{
  QComboBox *editor = new QComboBox(parent);
  editor->addItem("union");
  editor->addItem("intersection");
  editor->addItem("difference");
  
  return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  int value = index.model()->data(index, g_idata_role).toInt();
  
  QComboBox *combo_box = qobject_cast<QComboBox*>(editor);
  combo_box->setCurrentIndex(value);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
				   const QModelIndex &index) const
{
  QComboBox *combo_box = static_cast<QComboBox*>(editor);
  int boxitem = combo_box->currentIndex();
  QString value = combo_box->itemText(boxitem);

  model->setData(index, boxitem, g_idata_role);
  model->setData(index, value, Qt::DisplayRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, 
					   const QModelIndex &) const
{
  editor->setGeometry(option.rect);
}
