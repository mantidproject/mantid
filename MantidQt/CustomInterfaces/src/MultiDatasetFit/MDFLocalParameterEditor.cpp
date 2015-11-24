#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterEditor.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QDoubleValidator>
#include <QEvent>
#include <QKeyEvent>
#include <QInputDialog>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{
/// Constructor
/// @param parent :: Parent widget.
/// @param index :: Index of the spectrum which parameter is edited.
/// @param fixed :: Is the parameter fixed initially?
/// @param tie :: Parameter's current tie (or empty string).
LocalParameterEditor::LocalParameterEditor(QWidget *parent, int index,
                                           double value, bool fixed,
                                           QString tie, bool othersFixed,
                                           bool allOthersFixed)
    : QWidget(parent), m_index(index), m_value(QString::number(value)),
      m_fixed(fixed), m_tie(tie), m_othersFixed(othersFixed),
      m_allOthersFixed(allOthersFixed) {
  auto layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->setContentsMargins(0,0,0,0);

  m_editor = new QLineEdit(parent);
  m_editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  m_editor->setToolTip("Edit local parameter value. Press F to fix/unfix it.");
  this->setFocusPolicy(Qt::StrongFocus);
  this->setFocusProxy(m_editor);

  m_button = new QPushButton("&Set");
  m_button->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
  m_button->setFocusPolicy(Qt::NoFocus);

  layout->addWidget(m_editor);
  layout->addWidget(m_button);
  layout->setStretch(0,1);
  layout->setStretch(1,0);

  auto setMenu = new QMenu(this);

  m_setAllAction = new QAction("Set to all",this);
  m_setAllAction->setToolTip("Set all parameters to this value");
  connect(m_setAllAction,SIGNAL(activated()),this,SLOT(setAll()));
  setMenu->addAction(m_setAllAction);

  setMenu->addSeparator();
  m_fixAction = new QAction(m_fixed? "Unfix" : "Fix", this);
  m_fixAction->setToolTip("Fix value of this parameter");
  connect(m_fixAction,SIGNAL(activated()),this,SLOT(fixParameter()));
  setMenu->addAction(m_fixAction);

  m_fixAllAction = new QAction("Fix all",this);
  m_fixAllAction->setToolTip("Fix all parameters.");
  connect(m_fixAllAction,SIGNAL(activated()),this,SLOT(fixAll()));
  setMenu->addAction(m_fixAllAction);

  m_unfixAllAction = new QAction("Unfix all",this);
  m_unfixAllAction->setToolTip("Unfix all parameters.");
  connect(m_unfixAllAction,SIGNAL(activated()),this,SLOT(unfixAll()));
  setMenu->addAction(m_unfixAllAction);

  setMenu->addSeparator();
  m_setTieAction = new QAction("Set tie",this);
  m_setTieAction->setToolTip("Set a tie for this parameter.");
  connect(m_setTieAction,SIGNAL(activated()),this,SLOT(setTie()));
  setMenu->addAction(m_setTieAction);

  m_removeTieAction = new QAction("Remove tie",this);
  m_removeTieAction->setToolTip("Remove the tie for this parameter.");
  connect(m_removeTieAction,SIGNAL(activated()),this,SLOT(removeTie()));
  setMenu->addAction(m_removeTieAction);

  m_setTieToAllAction = new QAction("Set tie to all",this);
  m_setTieToAllAction->setToolTip("Set this tie for all parameters.");
  connect(m_setTieToAllAction,SIGNAL(activated()),this,SLOT(setTieAll()));
  setMenu->addAction(m_setTieToAllAction);

  m_removeAllTiesAction = new QAction("Remove all ties",this);
  m_removeAllTiesAction->setToolTip("Remove ties for all parameters.");
  connect(m_removeAllTiesAction,SIGNAL(activated()),this,SLOT(removeAllTies()));
  setMenu->addAction(m_removeAllTiesAction);

  m_button->setMenu(setMenu);

  m_editor->installEventFilter(this);

  setEditorState();
}

/// Send a signal to set all parameters to the value in the editor.
void LocalParameterEditor::setAll()
{
  double value = m_editor->text().toDouble();
  emit setAllValues(value);
}

/// Toggle the fix state of the current parameter.
void LocalParameterEditor::fixParameter()
{
  m_fixed = !m_fixed;
  setEditorState();
  emit fixParameter(m_index, m_fixed);
}

/// Send a signal to fix all parameters.
void LocalParameterEditor::fixAll()
{
  m_fixed = true;
  m_allOthersFixed = true;
  m_othersFixed = true;
  setEditorState();
  emit setAllFixed(true);
}

/// Send a signal to unfix all parameters.
void LocalParameterEditor::unfixAll()
{
  m_fixed = false;
  m_allOthersFixed = false;
  m_othersFixed = false;
  setEditorState();
  emit setAllFixed(false);
}

/// Send a signal to tie a parameter.
void LocalParameterEditor::setTie()
{
  QInputDialog input;
  input.setWindowTitle("Set a tie.");
  input.setTextValue(m_tie);
  if (input.exec() == QDialog::Accepted) {
    m_tie = input.textValue();
    emit setTie(m_index, m_tie);
  }
  setEditorState();
}

/// Send a signal to remove a tie.
void LocalParameterEditor::removeTie()
{
  m_tie = "";
  emit setTie(m_index, "");
  setEditorState();
}

/// Set all ties for all parameters
void LocalParameterEditor::setTieAll()
{
  emit setTieAll(m_tie);
  setEditorState();
}

/// Remove ties form all parameters
void LocalParameterEditor::removeAllTies()
{
  emit setTieAll("");
  setEditorState();
}

/// Filter events in the line editor to emulate a shortcut (F to fix/unfix).
bool LocalParameterEditor::eventFilter(QObject *, QEvent *evn)
{
  if ( evn->type() == QEvent::KeyPress )
  {
    auto keyEvent = static_cast<QKeyEvent*>(evn);
    if (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::ControlModifier)
    {
      fixParameter();
      return true;
    }
    if (m_tie.isEmpty())
    {
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
void LocalParameterEditor::setEditorState()
{
  m_fixAction->setText(m_fixed ? "Unfix" : "Fix");
  m_unfixAllAction->setEnabled(m_fixed || m_othersFixed);
  m_fixAllAction->setEnabled((!m_fixed) || (!m_allOthersFixed));

  if (m_tie.isEmpty()) {
    auto validator = new QDoubleValidator(this);
    validator->setDecimals(16);
    m_editor->setValidator(validator);
    m_editor->setText(m_value);
  } else {
    m_editor->setValidator(nullptr);
    m_editor->setText(m_tie);
  }
}

} // MDF
} // CustomInterfaces
} // MantidQt

