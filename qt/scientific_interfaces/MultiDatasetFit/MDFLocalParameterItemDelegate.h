#ifndef MDFLOCALPARAMETERITEMDELEGATE_H_
#define MDFLOCALPARAMETERITEMDELEGATE_H_

#include <QStyledItemDelegate>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

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
class LocalParameterItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit LocalParameterItemDelegate(
      EditLocalParameterDialog *parent = nullptr);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void prepareForPastedData();

signals:
  void setAllValues(double);
  void fixParameter(int, bool);
  void setAllFixed(bool);
  void setTie(int, QString);
  void setTieAll(QString);
  void setValueToLog(int);
  void setAllValuesToLog();

protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private slots:
  void doSetValueToLog(int);
  void doSetAllValuesToLog();

private:
  bool eventFilter(QObject *obj, QEvent *ev) override;
  EditLocalParameterDialog *owner() const;
  mutable LocalParameterEditor *m_currentEditor;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*MDFLOCALPARAMETERITEMDELEGATE_H_*/
