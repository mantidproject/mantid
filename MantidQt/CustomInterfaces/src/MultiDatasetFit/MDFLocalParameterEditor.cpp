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
LocalParameterEditor::LocalParameterEditor(QWidget *parent, int index, bool fixed, QString tie):
  QWidget(parent), m_index(index),m_fixed(fixed), m_tie(tie)
{
  auto layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->setContentsMargins(0,0,0,0);

  m_editor = new QLineEdit(parent);
  m_editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  auto validator = new QDoubleValidator(this);
  validator->setDecimals(16);
  m_editor->setValidator(validator);
  m_editor->setToolTip("Edit local parameter value. Press F to fix/unfix it.");

  auto button = new QPushButton("&Set");
  button->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);

  this->setFocusPolicy(Qt::NoFocus);
  layout->addWidget(m_editor);
  layout->addWidget(button);
  layout->setStretch(0,1);
  layout->setStretch(1,0);
  this->setFocusProxy(m_editor);
  this->setFocusPolicy(Qt::StrongFocus);

  auto setMenu = new QMenu(this);

  QAction *action = new QAction("Set to all",this);
  action->setToolTip("Set all parameters to this value");
  connect(action,SIGNAL(activated()),this,SLOT(setAll()));
  setMenu->addAction(action);

  setMenu->addSeparator();
  m_fixAction = new QAction(m_fixed? "Unfix" : "Fix", this);
  m_fixAction->setToolTip("Fix value of this parameter");
  connect(m_fixAction,SIGNAL(activated()),this,SLOT(fixParameter()));
  setMenu->addAction(m_fixAction);

  action = new QAction("Fix all",this);
  action->setToolTip("Fix all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(fixAll()));
  setMenu->addAction(action);

  action = new QAction("Unfix all",this);
  action->setToolTip("Unfix all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(unfixAll()));
  setMenu->addAction(action);

  setMenu->addSeparator();
  action = new QAction("Set tie",this);
  action->setToolTip("Set a tie for this parameter.");
  connect(action,SIGNAL(activated()),this,SLOT(setTie()));
  setMenu->addAction(action);

  action = new QAction("Remove tie",this);
  action->setToolTip("Remove the tie for this parameter.");
  connect(action,SIGNAL(activated()),this,SLOT(removeTie()));
  setMenu->addAction(action);

  action = new QAction("Set tie to all",this);
  action->setToolTip("Set this tie for all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(setTieAll()));
  setMenu->addAction(action);

  action = new QAction("Remove all ties",this);
  action->setToolTip("Remove ties for all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(removeAllTies()));
  setMenu->addAction(action);

  button->setMenu(setMenu);

  m_editor->installEventFilter(this);
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
  m_fixAction->setText( m_fixed? "Unfix" : "Fix" );
  emit fixParameter(m_index, m_fixed);
}

/// Send a signal to fix all parameters.
void LocalParameterEditor::fixAll()
{
  emit setAllFixed(true);
}

/// Send a signal to unfix all parameters.
void LocalParameterEditor::unfixAll()
{
  emit setAllFixed(false);
}

/// Send a signal to tie a parameter.
void LocalParameterEditor::setTie()
{
  QInputDialog input;
  input.setWindowTitle("Set a tie.");
  input.setTextValue(m_tie);
  if (input.exec() == QDialog::Accepted) {
    auto tie = input.textValue();
    emit setTie(m_index, tie);
  }
}

/// Send a signal to remove a tie.
void LocalParameterEditor::removeTie()
{
  emit setTie(m_index, "");
}

/// Set all ties for all parameters
void LocalParameterEditor::setTieAll()
{
  emit setTieAll(m_tie);
}

/// Remove ties form all parameters
void LocalParameterEditor::removeAllTies()
{
  emit setTieAll("");
}

/// Filter events in the line editor to emulate a shortcut (F to fix/unfix).
bool LocalParameterEditor::eventFilter(QObject *, QEvent *evn)
{
  if ( evn->type() == QEvent::KeyPress )
  {
    auto keyEvent = static_cast<QKeyEvent*>(evn);
    if ( keyEvent->key() == Qt::Key_F )
    {
      fixParameter();
      return true;
    }
  }
  return false;
}

} // MDF
} // CustomInterfaces
} // MantidQt

