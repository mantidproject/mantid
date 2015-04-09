#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterEditor.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QDoubleValidator>
#include <QEvent>
#include <QKeyEvent>

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
LocalParameterEditor::LocalParameterEditor(QWidget *parent, int index, bool fixed):
  QWidget(parent), m_index(index),m_fixed(fixed)
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

  auto button = new QPushButton("Set");
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

  m_fixAction = new QAction(m_fixed? "Unfix" : "Fix", this);
  m_fixAction->setToolTip("Fix value of this parameter");
  connect(m_fixAction,SIGNAL(activated()),this,SLOT(fixParameter()));
  setMenu->addAction(m_fixAction);

  action = new QAction("Fix all",this);
  action->setToolTip("Fix all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(fixAll()));
  setMenu->addAction(action);

  action = new QAction("Unix all",this);
  action->setToolTip("Unfix all parameters.");
  connect(action,SIGNAL(activated()),this,SLOT(unfixAll()));
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

