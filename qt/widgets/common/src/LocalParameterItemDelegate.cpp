// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/LocalParameterItemDelegate.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/LocalParameterEditor.h"

#include <QPainter>

namespace MantidQt {
namespace MantidWidgets {

/// Constructor.
LocalParameterItemDelegate::LocalParameterItemDelegate(EditLocalParameterDialog *parent)
    : QStyledItemDelegate(parent), m_currentEditor(nullptr) {}

/// Create a custom editor LocalParameterEditor.
QWidget *LocalParameterItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                                  const QModelIndex &index) const {
  auto row = index.row();
  m_currentEditor = new LocalParameterEditor(parent, row, owner()->getValue(row), owner()->isFixed(row),
                                             owner()->getTie(row), owner()->getConstraint(row),
                                             owner()->areOthersFixed(row), owner()->areAllOthersFixed(row),
                                             owner()->areOthersTied(row), owner()->isLogCheckboxTicked());
  connect(m_currentEditor, SIGNAL(setAllValues(double)), this, SIGNAL(setAllValues(double)));
  connect(m_currentEditor, SIGNAL(fixParameter(int, bool)), this, SIGNAL(fixParameter(int, bool)));
  connect(m_currentEditor, SIGNAL(setAllFixed(bool)), this, SIGNAL(setAllFixed(bool)));
  connect(m_currentEditor, SIGNAL(setTie(int, QString)), this, SIGNAL(setTie(int, QString)));
  connect(m_currentEditor, SIGNAL(setTieAll(QString)), this, SIGNAL(setTieAll(QString)));
  connect(m_currentEditor, SIGNAL(setConstraint(int, QString)), this, SIGNAL(setConstraint(int, QString)));
  connect(m_currentEditor, SIGNAL(setConstraintAll(QString)), this, SIGNAL(setConstraintAll(QString)));
  connect(m_currentEditor, SIGNAL(setValueToLog(int)), this, SLOT(doSetValueToLog(int)));
  connect(m_currentEditor, SIGNAL(setAllValuesToLog()), this, SLOT(doSetAllValuesToLog()));
  connect(owner(), SIGNAL(logOptionsChecked(bool)), m_currentEditor, SLOT(setLogOptionsEnabled(bool)));
  m_currentEditor->installEventFilter(const_cast<LocalParameterItemDelegate *>(this));
  return m_currentEditor;
}

/// Initialize the editor with the current data in the cell.
void LocalParameterItemDelegate::setEditorData(QWidget * /*editor*/, const QModelIndex & /*index*/) const {
  // Needs to be empty to prevent Qt's default behaviour.
}

/// Update the data in the cell with the text in the editor.
void LocalParameterItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                              const QModelIndex &index) const {
  QStyledItemDelegate::setModelData(editor->layout()->itemAt(0)->widget(), model, index);
}

/// Re-implemented to resolve an issue: if the parent dialog closes with
/// the editor is active any changes in it get ignored.
bool LocalParameterItemDelegate::eventFilter(QObject *obj, QEvent *ev) {
  if (ev->type() == QEvent::WindowDeactivate) {
    // Force to save the changes to the underlying model.
    emit commitData(m_currentEditor);
    return true;
  }
  return QStyledItemDelegate::eventFilter(obj, ev);
}

/// Paint the table cell.
void LocalParameterItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const {
  auto const tie = owner()->getTie(index.row());

  if (!tie.isEmpty()) {
    auto rect = option.rect;
    auto dHeight = (option.rect.height() - option.fontMetrics.height()) / 2;
    rect.adjust(0, dHeight, 0, -dHeight);
    painter->drawText(rect, tie);
  } else {
    QStyledItemDelegate::paint(painter, option, index);
  }
}

/// Cast the parent to EditLocalParameterDialog. Get access to parameter
/// values and fixes.
EditLocalParameterDialog *LocalParameterItemDelegate::owner() const {
  return static_cast<EditLocalParameterDialog *>(parent());
}

/**
 * Slot: close the editor and re-emit the signal
 * @param i :: [input] Index of row
 */
void LocalParameterItemDelegate::doSetValueToLog(int i) {
  if (m_currentEditor) {
    closeEditor(m_currentEditor);
    m_currentEditor = nullptr;
  }
  emit setValueToLog(i);
}

/**
 * Slot: close the editor and re-emit the signal
 */
void LocalParameterItemDelegate::doSetAllValuesToLog() {
  if (m_currentEditor) {
    closeEditor(m_currentEditor);
    m_currentEditor = nullptr;
  }
  emit setAllValuesToLog();
}

/**
 * Data is about to be pasted into the table.
 * Prepare for this by:
 *   - closing the editor (if one is open)
 */
void LocalParameterItemDelegate::prepareForPastedData() {
  if (m_currentEditor) {
    closeEditor(m_currentEditor);
    m_currentEditor = nullptr;
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
