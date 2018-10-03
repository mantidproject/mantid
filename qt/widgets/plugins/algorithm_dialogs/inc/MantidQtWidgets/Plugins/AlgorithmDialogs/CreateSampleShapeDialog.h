// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPEDIALOG_H_
#define MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPEDIALOG_H_

//---------------------------
// Includes
//--------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_CreateSampleShapeDialog.h"

#include <QHash>
#include <QItemDelegate>
#include <QMap>
#include <QPoint>
#include <QTreeWidget>
#include <QVector>

//-----------------------------------
// Qt Forward declarations
//---------------------------------
class QCloseEvent;

namespace MantidQt {
namespace CustomDialogs {

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
*/
class CreateSampleShapeDialog : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  CreateSampleShapeDialog(QWidget *parent = nullptr);

  /// Destructor
  ~CreateSampleShapeDialog() override;

private slots:
  /// Context menu request
  void handleTreeContextMenuRequest(const QPoint &pos);
  /// Toggle the flag on
  void toggleShapeComplement(bool state);
  /// Add a new shape
  void addShape(QAction *shape);
  /// Add operation node based on menu action
  void addOperation(QAction *shape);
  /// Connects to the delete slot
  void handleDeleteRequest();
  /// Remove an item from the tree (recursive)
  void removeItem(BinaryTreeWidgetItem *item);
  /// Setup the details box based currently selected item
  void setupDetailsBox();
  ///  Change item data
  void changeTreeData(BinaryTreeWidgetItem *item, int data);
  /// Update the object within the 3D widget
  void update3DView();

private:
  /// Initialize the layout
  void initLayout() override;
  /// Get the input out of the dialog
  void parseInput() override;
  /// Find the parent
  BinaryTreeWidgetItem *getSelectedItem();
  /// Create a details widget based upon the shape name given
  ShapeDetails *createDetailsWidget(const QString &shapename) const;
  /// Construct the XML from the current tree
  QString constructShapeXML() const;

private:
  /// The form generated with Qt Designer
  Ui::CreateSampleShapeDialog m_uiForm;
  /// A pointer to the model for the shape tree
  BinaryTreeWidget *m_shapeTree;
  /// A map of shape names to instantiator objects
  QHash<QString, BaseInstantiator *> m_setup_map;
  /// A map of QTreeWidgetItem objects to their details objects
  QMap<BinaryTreeWidgetItem *, ShapeDetails *> m_details_map;
  /// A map of QTreeWidgetItem objects to their operation objects
  QMap<BinaryTreeWidgetItem *, Operation *> m_ops_map;
  /// The 3D object viewer
  MantidGLWidget *m_object_viewer;
};

/**
 * A custom item to use in the BinaryTree widget
 */
class BinaryTreeWidgetItem : public QTreeWidgetItem {

public:
  /// Default Constructor
  BinaryTreeWidgetItem(int type = QTreeWidgetItem::UserType);

  /// Constructor taking a string list and an optional type
  BinaryTreeWidgetItem(const QStringList &strings,
                       int type = QTreeWidgetItem::UserType);

  /// Add a child item
  bool addChildItem(BinaryTreeWidgetItem *child);

  /// A pointer to the left child
  BinaryTreeWidgetItem *leftChild() const;

  /// A pointer to the right child
  BinaryTreeWidgetItem *rightChild() const;

private:
  /// The index of the left child (0 or 1)
  int m_left_index;
  /// The index of the right child (0 or 1)
  int m_right_index;
};

/**
 * A widget to implement a binary tree display.
 */
class BinaryTreeWidget : public QTreeWidget {
  Q_OBJECT

public:
  /// Default constructor
  BinaryTreeWidget(QWidget *parent = nullptr);

  // Return the root of the binary tree
  BinaryTreeWidgetItem *root() const;

  /// Recurse through the tree in a post-order
  void traverseInPostOrder(BinaryTreeWidgetItem *node,
                           QList<BinaryTreeWidgetItem *> &expression);

/// Called when the data in the model is changed
#if QT_VERSION < 0x050000
  void dataChanged(const QModelIndex &topLeft,
                   const QModelIndex &bottomRight) override;
#else
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QVector<int> &roles = QVector<int>()) override;
#endif

signals:
  /// Emitted when data has changed
  void treeDataChange(BinaryTreeWidgetItem *item, int data);
};

/**
 * A custom delegate class used for item editing
 */
class ComboBoxDelegate : public QItemDelegate {
  Q_OBJECT

public:
  /// Default constructor
  ComboBoxDelegate(QWidget *parent = nullptr);
  /// Create an editor for the item
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  /// Set the data for the editor when it has been created
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  /// Set the data for the model when editing has finished
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  /// Ensure that the editor has the correct geometry when it is created
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_CREATESAMPLESHAPE_H_
