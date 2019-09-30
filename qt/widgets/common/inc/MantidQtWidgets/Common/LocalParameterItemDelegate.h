// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_LOCALPARAMETERITEMDELEGATE_H_
#define MANTIDWIDGETS_LOCALPARAMETERITEMDELEGATE_H_

#include <QStyledItemDelegate>

namespace MantidQt {
namespace MantidWidgets {

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
  void setAllValues(double /*_t1*/);
  void fixParameter(int /*_t1*/, bool /*_t2*/);
  void setAllFixed(bool /*_t1*/);
  void setTie(int /*_t1*/, QString /*_t2*/);
  void setTieAll(QString /*_t1*/);
  void setValueToLog(int /*_t1*/);
  void setAllValuesToLog();

protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private slots:
  void doSetValueToLog(int /*i*/);
  void doSetAllValuesToLog();

private:
  bool eventFilter(QObject *obj, QEvent *ev) override;
  EditLocalParameterDialog *owner() const;
  mutable LocalParameterEditor *m_currentEditor;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDWIDGETS_LOCALPARAMETERITEMDELEGATE_H_*/
