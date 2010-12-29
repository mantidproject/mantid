#ifndef MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPEDIALOG_H_
#define MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPEDIALOG_H_

//---------------------------
// Includes
//--------------------------
#include "MantidQtCustomDialogs/ui_CreateSampleShapeDialog.h"
#include "MantidQtAPI/AlgorithmDialog.h"

#include <QTreeWidget>
#include <QItemDelegate>
#include <QPoint>
#include <QHash>
#include <QMap>
#include <QVector>

//-----------------------------------
// Qt Forward declarations
//---------------------------------
class QCloseEvent;

namespace MantidQt
{
namespace CustomDialogs
{

class BinaryTreeWidget;
class BinaryTreeWidgetItem;
class ShapeDetails;
struct BaseInstantiator;
struct Operation;
class MantidGLWidget;

/** 
    This class gives specialised dialog for the sample shape definition
    algorithm

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class CreateSampleShapeDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT

public:
  
  /// Default constructor
  CreateSampleShapeDialog(QWidget *parent = 0);

  /// Destructor
  ~CreateSampleShapeDialog();

private slots:
  /// Context menu request
  void handleTreeContextMenuRequest(const QPoint & pos);
  /// Toggle the flag on 
  void toggleShapeComplement(bool state);
  /// Add a new shape
  void addShape(QAction *shape);
  /// Add operation node based on menu action
  void addOperation(QAction *shape);
  /// Connects to the delete slot
  void handleDeleteRequest();
  /// Remove an item from the tree (recursive)
  void removeItem(BinaryTreeWidgetItem* item);
  /// Setup the details box based currently selected item
  void setupDetailsBox();
  ///  Change item data
  void changeTreeData(BinaryTreeWidgetItem* item, int data);
  /// Update the object within the 3D widget
  void update3DView();

private:
  /// Initialize the layout
  virtual void initLayout();
  /// Get the input out of the dialog
  virtual void parseInput();
  /// Find the parent
  BinaryTreeWidgetItem* getSelectedItem();
  /// Create a details widget based upon the shape name given
  ShapeDetails* createDetailsWidget(const QString & shapename) const;
  /// Construct the XML from the current tree
  QString constructShapeXML() const;

private:
  /// The form generated with Qt Designer
  Ui::CreateSampleShapeDialog m_uiForm;
  /// A pointer to the model for the shape tree
  BinaryTreeWidget *m_shapeTree;
  /// A map of shape names to instantiator objects
  QHash<QString, BaseInstantiator*> m_setup_map;
  ///A map of QTreeWidgetItem objects to their details objects
  QMap<BinaryTreeWidgetItem*, ShapeDetails*> m_details_map;
  ///A map of QTreeWidgetItem objects to their operation objects
  QMap<BinaryTreeWidgetItem*, Operation*> m_ops_map;
  ///The 3D object viewer
  MantidGLWidget *m_object_viewer;
};

/**
 * A custom item to use in the BinaryTree widget
 */
class BinaryTreeWidgetItem : public QTreeWidgetItem
{

public:
  /// Default Constructor
  BinaryTreeWidgetItem(int type = QTreeWidgetItem::UserType);

  /// Constructor taking a string list and an optional type
  BinaryTreeWidgetItem(const QStringList & strings, int type = QTreeWidgetItem::UserType);

  /// Add a child item 
  bool addChildItem(BinaryTreeWidgetItem *child);

  /// A pointer to the left child
  BinaryTreeWidgetItem* leftChild() const;

  /// A pointer to the right child
  BinaryTreeWidgetItem* rightChild() const;

private:
  /// The index of the left child (0 or 1)
  int m_left_index;
  /// The index of the right child (0 or 1)
  int m_right_index;
};

/**
 * A widget to implement a binary tree display.
 */
class BinaryTreeWidget : public QTreeWidget
{
  Q_OBJECT
  
public:
  /// Default constructor
  BinaryTreeWidget(QWidget *parent = 0);

  //Return the root of the binary tree
  BinaryTreeWidgetItem* root() const;

  /// Recurse through the tree in a post-order
  void traverseInPostOrder(BinaryTreeWidgetItem* node, QList<BinaryTreeWidgetItem*> & expression);

  /// Called when the data in the model is changed
  void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight );

signals:
  /// Emitted when data has changed
  void treeDataChange(BinaryTreeWidgetItem* item, int data);
};

/**
 * A custom delegate class used for item editing
 */
class ComboBoxDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  /// Default constructor
  ComboBoxDelegate(QWidget *parent = 0);
  /// Create an editor for the item
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
			const QModelIndex &index) const;
  ///Set the data for the editor when it has been created
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  ///Set the data for the model when editing has finished
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
  /// Ensure that the editor has the correct geometry when it is created
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, 
			    const QModelIndex &index) const;
};

}
}

#endif //MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPE_H_
