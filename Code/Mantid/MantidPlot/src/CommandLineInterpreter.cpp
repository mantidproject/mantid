#include "CommandLineInterpreter.h"
#include "MantidQtMantidWidgets/ScriptEditor.h"
#include "ScriptingEnv.h"

#include <QVBoxLayout>

/**
 * Construct an object with the given parent
 */
CommandLineInterpreter::CommandLineInterpreter(QWidget *parent)
  : QWidget(parent), m_editor(new ScriptEditor(this,true, NULL)), m_runner()
{
  setFocusProxy(m_editor);
  m_editor->setFocus();
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(m_editor);
  mainLayout->setContentsMargins(0,0,0,0);
  setLayout(mainLayout);
}

/**
 * Setup with a scripting environment
 * @param environ A reference to a scripting environment
 */
void CommandLineInterpreter::setup(const ScriptingEnv & environ)
{
  m_runner = QSharedPointer<Script>(environ.newScript("<CommandLine>",this,Script::Interactive));
  m_editor->setLexer(environ.createCodeLexer());
}

/**
 * Persist the current settings to the store
 */
void CommandLineInterpreter::saveSettings() const
{

}

/**
 * Copy to the clipboard
 */
void CommandLineInterpreter::copy()
{
  m_editor->copy();
}

/**
 * Paste from the clipboard
 */
void CommandLineInterpreter::paste()
{
  m_editor->paste();
}
