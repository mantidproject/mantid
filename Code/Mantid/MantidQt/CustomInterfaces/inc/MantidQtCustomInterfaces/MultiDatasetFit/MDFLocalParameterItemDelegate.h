#ifndef MDFLOCALPARAMETERITEMDELEGATE_H_
#define MDFLOCALPARAMETERITEMDELEGATE_H_

#include <QStyledItemDelegate>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

// Forward declarations.
class EditLocalParameterDialog;
class LocalParameterEditor;

/**
 * A custom item delegate - an object controlling display and
 * editing of a cell in a table widget.
 *
 * Re-implemented:
 *  - paint(...) method shows which parameters are fixed.
 *  - createEditor(...) method creates a custom editor for parameter values.
 */
class LocalParameterItemDelegate: public QStyledItemDelegate
{
  Q_OBJECT
public:
  LocalParameterItemDelegate(EditLocalParameterDialog *parent = NULL);
  QWidget* createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const;
  void setEditorData(QWidget * editor, const QModelIndex & index) const;
  void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const;
signals:
  void setAllValues(double);
  void fixParameter(int,bool);
  void setAllFixed(bool);
protected:
  void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
private:
  bool eventFilter(QObject * obj, QEvent * ev);
  EditLocalParameterDialog *owner() const;
  mutable LocalParameterEditor* m_currentEditor;
};


} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFLOCALPARAMETERITEMDELEGATE_H_*/
