// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/LocalParameterEditor.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include <QAction>
#include <QDoubleValidator>
#include <QEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

namespace MantidQt {
namespace MantidWidgets {

/// Constructor
/// @param parent :: Parent widget.
/// @param index :: Index of the spectrum which parameter is edited.
/// @param value :: Current parameter value.
/// @param fixed :: Is the parameter fixed initially?
/// @param tie :: Parameter's current tie (or empty string).
/// @param constraint :: Parameter's current constraint (or empty string).
/// @param othersFixed :: True if some other local parameters are fixed.
/// @param allOthersFixed :: True if all other local parameters are fixed.
/// @param othersTied :: True if there are other tied parameters.
/// @param logOptionsEnabled :: True if the log checkbox is ticked.
LocalParameterEditor::LocalParameterEditor(QWidget *parent, int index,
                                           double value, bool fixed,
                                           QString tie, QString constraint,
                                           bool othersFixed,
                                           bool allOthersFixed, bool othersTied,
                                           bool logOptionsEnabled)
    : QWidget(parent), m_index(index), m_value(QString::number(value, 'g', 16)),
      m_fixed(fixed), m_tie(tie), m_constraint(constraint), m_othersFixed(othersFixed),
      m_allOthersFixed(allOthersFixed), m_othersTied(othersTied) {
  auto layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  m_editor = new QLineEdit(parent);
  m_editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  this->setFocusPolicy(Qt::StrongFocus);
  this->setFocusProxy(m_editor);

  m_button = new QPushButton("&Set");
  m_button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_button->setFocusPolicy(Qt::NoFocus);

  layout->addWidget(m_editor);
  layout->addWidget(m_button);
  layout->setStretch(0, 1);
  layout->setStretch(1, 0);

  auto setMenu = new QMenu(this);

  m_setAllAction = new QAction("Set to all", this);
  m_setAllAction->setToolTip("Set all parameters to this value");
  connect(m_setAllAction, SIGNAL(triggered()), this, SLOT(setAll()));
  setMenu->addAction(m_setAllAction);

  setMenu->addSeparator();
  m_fixAction = new QAction(m_fixed ? "Unfix" : "Fix", this);
  m_fixAction->setToolTip("Fix value of this parameter");
  connect(m_fixAction, SIGNAL(triggered()), this, SLOT(fixParameter()));
  setMenu->addAction(m_fixAction);

  m_fixAllAction = new QAction("Fix all", this);
  m_fixAllAction->setToolTip("Fix all parameters.");
  connect(m_fixAllAction, SIGNAL(triggered()), this, SLOT(fixAll()));
  setMenu->addAction(m_fixAllAction);

  m_unfixAllAction = new QAction("Unfix all", this);
  m_unfixAllAction->setToolTip("Unfix all parameters.");
  connect(m_unfixAllAction, SIGNAL(triggered()), this, SLOT(unfixAll()));
  setMenu->addAction(m_unfixAllAction);

  setMenu->addSeparator();
  m_setTieAction = new QAction("Set tie", this);
  m_setTieAction->setToolTip("Set a tie for this parameter.");
  connect(m_setTieAction, SIGNAL(triggered()), this, SLOT(setTie()));
  setMenu->addAction(m_setTieAction);

  m_removeTieAction = new QAction("Remove tie", this);
  m_removeTieAction->setToolTip("Remove the tie for this parameter.");
  connect(m_removeTieAction, SIGNAL(triggered()), this, SLOT(removeTie()));
  setMenu->addAction(m_removeTieAction);

  m_setTieToAllAction = new QAction("Set tie to all", this);
  m_setTieToAllAction->setToolTip("Set this tie for all parameters.");
  connect(m_setTieToAllAction, SIGNAL(triggered()), this, SLOT(setTieAll()));
  setMenu->addAction(m_setTieToAllAction);

  m_removeAllTiesAction = new QAction("Remove all ties", this);
  m_removeAllTiesAction->setToolTip("Remove ties for all parameters.");
  connect(m_removeAllTiesAction, SIGNAL(triggered()), this,
          SLOT(removeAllTies()));
  setMenu->addAction(m_removeAllTiesAction);

  setMenu->addSeparator();
  m_setConstraintAction = new QAction("Set constraint", this);
  m_setConstraintAction->setToolTip("Set a constraint for this parameter.");
  connect(m_setConstraintAction, SIGNAL(triggered()), this,
          SLOT(setConstraint()));
  setMenu->addAction(m_setConstraintAction);

  m_removeConstraintAction = new QAction("Remove constraint", this);
  m_removeConstraintAction->setToolTip("Remove the constraint for this parameter.");
  connect(m_removeConstraintAction, SIGNAL(triggered()), this,
          SLOT(removeConstraint()));
  setMenu->addAction(m_removeConstraintAction);

  m_setConstraintToAllAction = new QAction("Set constraint to all", this);
  m_setConstraintToAllAction->setToolTip("Set this constraint for all parameters.");
  connect(m_setConstraintToAllAction, SIGNAL(triggered()), this,
          SLOT(setConstraintAll()));
  setMenu->addAction(m_setConstraintToAllAction);

  m_removeAllConstraintsAction = new QAction("Remove all constraints", this);
  m_removeAllConstraintsAction->setToolTip("Remove constraints for all parameters.");
  connect(m_removeAllConstraintsAction, SIGNAL(triggered()), this,
          SLOT(removeAllConstraints()));
  setMenu->addAction(m_removeAllConstraintsAction);

  setMenu->addSeparator();
  m_setToLogAction = new QAction("Set to log", this);
  m_setToLogAction->setToolTip("Set this parameter to a log value.");
  connect(m_setToLogAction, SIGNAL(triggered()), this, SLOT(setToLog()));
  setMenu->addAction(m_setToLogAction);
  m_setToLogAction->setEnabled(logOptionsEnabled);

  m_setAllToLogAction = new QAction("Set all to log", this);
  m_setAllToLogAction->setToolTip(
      "Set all parameters to log value from the relevant workspace");
  connect(m_setAllToLogAction, SIGNAL(triggered()), this,
          SIGNAL(setAllValuesToLog()));
  setMenu->addAction(m_setAllToLogAction);
  m_setAllToLogAction->setEnabled(logOptionsEnabled);

  m_button->setMenu(setMenu);

  m_editor->installEventFilter(this);

  connect(m_editor, SIGNAL(textEdited(const QString &)), this,
          SLOT(updateValue(const QString &)));

  setEditorState();
}

/// Send a signal to set all parameters to the value in the editor.
void LocalParameterEditor::setAll() {
  double value = m_editor->text().toDouble();
  emit setAllValues(value);
}

/// Toggle the fix state of the current parameter.
void LocalParameterEditor::fixParameter() {
  m_fixed = !m_fixed;
  setEditorState();
  emit fixParameter(m_index, m_fixed);
}

/// Send a signal to fix all parameters.
void LocalParameterEditor::fixAll() {
  m_fixed = true;
  m_allOthersFixed = true;
  m_othersFixed = true;
  setEditorState();
  emit setAllFixed(true);
}

/// Send a signal to unfix all parameters.
void LocalParameterEditor::unfixAll() {
  m_fixed = false;
  m_allOthersFixed = false;
  m_othersFixed = false;
  setEditorState();
  emit setAllFixed(false);
}

/// Send a signal to tie a parameter.
void LocalParameterEditor::setTie() {
  auto tie = setTieDialog(m_tie);
  if (!tie.isEmpty()) {
    m_tie = tie;
    emit setTie(m_index, m_tie);
  }
  setEditorState();
}

/// Send a signal to remove a tie.
void LocalParameterEditor::removeTie() {
  m_tie = "";
  emit setTie(m_index, "");
  setEditorState();
}

/// Set all ties for all parameters
void LocalParameterEditor::setTieAll() {
  auto tie = setTieDialog(m_tie);
  if (!tie.isEmpty()) {
    m_tie = tie;
    m_othersTied = true;
    emit setTieAll(m_tie);
  }
  setEditorState();
}

/// Remove ties from all parameters
void LocalParameterEditor::removeAllTies() {
  m_tie = "";
  m_othersTied = false;
  emit setTieAll("");
  setEditorState();
}

void LocalParameterEditor::setConstraint() {
  auto constraint = setTieDialog(m_constraint);
  if (!constraint.isEmpty()) {
    m_constraint = constraint;
    emit setConstraint(m_index, m_constraint);
  }
  setEditorState();
}

void LocalParameterEditor::removeConstraint() {
  m_constraint = "";
  emit setConstraint(m_index, "");
  setEditorState();
}

void LocalParameterEditor::setConstraintAll() {
  auto constraint = setConstraintDialog(m_constraint);
  if (!constraint.isEmpty()) {
    m_constraint = constraint;
    m_othersConstrained = true;
    emit setConstraintAll(m_constraint);
  }
  setEditorState();
}

void LocalParameterEditor::removeAllConstraints() {
  m_constraint = "";
  m_othersConstrained = false;
  emit setConstraintAll("");
  setEditorState();
}

/// Send a signal to set value to log
void LocalParameterEditor::setToLog() { emit setValueToLog(m_index); }

/// Filter events in the line editor to emulate a shortcut (F to fix/unfix).
bool LocalParameterEditor::eventFilter(QObject * /*unused*/, QEvent *evn) {
  if (evn->type() == QEvent::KeyPress) {
    auto keyEvent = static_cast<QKeyEvent *>(evn);
    if (keyEvent->key() == Qt::Key_F &&
        keyEvent->modifiers() == Qt::ControlModifier && m_tie.isEmpty()) {
      fixParameter();
      return true;
    }
    if (m_tie.isEmpty()) {
      m_value = m_editor->text();
    } else {
      m_tie = m_editor->text();
      emit setTie(m_index, m_tie);
    }
  }
  return false;
}

/// Set the state of the editor elements (the line editor and the button)
/// according to the state of the parameter (fixed, tied, etc)
void LocalParameterEditor::setEditorState() {
  bool const isNumber = m_tie.isEmpty();
  bool const isTie = !isNumber;
  bool const hasConstraint = !m_constraint.isEmpty();

  m_setAllAction->setEnabled(isNumber);
  m_fixAction->setText(m_fixed ? "Unfix" : "Fix");
  m_fixAction->setEnabled(isNumber);
  m_unfixAllAction->setEnabled(isNumber && (m_fixed || m_othersFixed));
  m_fixAllAction->setEnabled(isNumber && (!m_fixed || !m_allOthersFixed));

  m_removeTieAction->setEnabled(isTie);
  m_removeAllTiesAction->setEnabled(isTie || m_othersTied);
  m_removeConstraintAction->setEnabled(hasConstraint);
  m_removeAllConstraintsAction->setEnabled(hasConstraint);

  if (isNumber) {
    auto validator = new QDoubleValidator(this);
    validator->setDecimals(16);
    m_editor->setValidator(validator);
    m_editor->setText(m_value);
    m_editor->setToolTip(
        "Edit local parameter value. Press Ctrl+F to fix/unfix it.");
  } else {
    m_editor->setValidator(nullptr);
    m_editor->setText(m_tie);
    m_editor->setToolTip("Edit local parameter tie.");
  }
}

/// Open an input dialog to enter a tie expression.
QString LocalParameterEditor::setTieDialog(QString tie) {
  QInputDialog input;
  input.setWindowTitle("Set a tie.");
  input.setTextValue(tie);
  if (input.exec() == QDialog::Accepted) {
    return input.textValue();
  }
  return "";
}

QString LocalParameterEditor::setConstraintDialog(QString constraint) {
  QInputDialog input;
  input.setWindowTitle("Set a constraint.");
  input.setTextValue(constraint);
  if (input.exec() == QDialog::Accepted) {
    return input.textValue();
  }
  return "";
}

/**
 * SLOT: when user edits value, make sure m_value is updated
 * @param value :: [input] Changed text in the edit box
 */
void LocalParameterEditor::updateValue(const QString &value) {
  m_value = value;
}

/**
 * Slot: when log checkbox state changes, enable/disable the "set to log" and
 * "set all to log" options
 * @param enabled :: [input] Whether to enable or disable options
 */
void LocalParameterEditor::setLogOptionsEnabled(bool enabled) {
  m_setToLogAction->setEnabled(enabled);
  m_setAllToLogAction->setEnabled(enabled);
}
} // namespace MantidWidgets
} // namespace MantidQt
