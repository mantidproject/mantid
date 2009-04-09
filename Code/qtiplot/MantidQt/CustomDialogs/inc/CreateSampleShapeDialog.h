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

namespace MantidQt
{
namespace CustomDialogs
{

class BinaryTreeWidget;
class BinaryTreeWidgetItem;
class ShapeDetails;

/** 
    This class gives specialised dialog for the sample shape definition
    algorithm

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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

public slots:
  ///Context menu request
  void handleTreeContextMenuRequest(const QPoint & pos);

  ///Add a new child shape
  void addChildShape(QAction *shape);

  // Setup the details box based currently selected item
  void setupDetailsBox();

private:

  /// Initialize the layout
  virtual void initLayout();
  
  /// Get the input out of the dialog
  virtual void parseInput();

  /**@name Details setup functions */
  //@{
  /// Setup the box for a sphere
  ShapeDetails* setupSphereDetails() const;

  /// Setup the box for a cylinder
  ShapeDetails* setupCylinderDetails() const;
  //@}

private:
  /// The form generated with Qt Designer
  Ui::CreateSampleShapeDialog m_uiForm;

  /// A pointer to the model for the shape tree
  BinaryTreeWidget *m_shapeTree;

  /// A map of shape names to function pointers
  typedef ShapeDetails* (CreateSampleShapeDialog::*MemFuncGetter)() const;
  QHash<QString, MemFuncGetter> m_setup_functions;

  ///A map of QTreeWidgetItem objects to their details objects
  QMap<BinaryTreeWidgetItem*, ShapeDetails*> m_details_map;
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

  /// Check that we have a full binary tree
  bool isFullTree() const;

  //Return the root of the binary tree
  BinaryTreeWidgetItem* root() const;

  /// Recurse through the tree in a pre-order
  void traverseByPreorder(BinaryTreeWidgetItem* node);
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

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
			const QModelIndex &index) const;
  
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
		    const QModelIndex &index) const;
  
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
  /// A custom data role
  static int g_idata_role;
};

}
}

#endif //MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPE_H_
