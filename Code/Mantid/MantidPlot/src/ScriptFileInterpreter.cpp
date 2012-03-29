#include "ScriptFileInterpreter.h"
#include "ScriptOutputDisplay.h"
#include "ScriptingEnv.h"
#include "MantidQtMantidWidgets/ScriptEditor.h"

#include <QVBoxLayout>
#include <QFileInfo>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QPushButton>

/**
 * Construct a widget
 * @param parent :: The parent widget
 */
ScriptFileInterpreter::ScriptFileInterpreter(QWidget *parent)
  : QWidget(parent), m_editor(new ScriptEditor(this, NULL)),
    m_messages(new ScriptOutputDisplay), m_runner(),
    m_execAll(NULL), m_execSelect(NULL)
{
  setFocusProxy(m_editor);
  m_editor->setFocus();

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(m_editor);
  mainLayout->addWidget(m_messages);
  mainLayout->setContentsMargins(0,0,0,0);
  setLayout(mainLayout);

  initActions();

  connect(m_editor, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
}

/// Destroy the object
ScriptFileInterpreter::~ScriptFileInterpreter()
{
}

/**
 * Make sure the widget is ready to be deleted, i.e. saved etc
 */
void ScriptFileInterpreter::prepareToClose()
{
  if( !isScriptModified() ) return;

  QMessageBox msgBox(this);
  msgBox.setModal(true);
  msgBox.setWindowTitle("MantidPlot");
  msgBox.setText(tr("The current script has been modified."));
  msgBox.setInformativeText(tr("Save changes?"));
  msgBox.addButton(QMessageBox::Save);
  QPushButton *saveAsButton = msgBox.addButton("Save As...", QMessageBox::AcceptRole);
  msgBox.addButton(QMessageBox::Discard);
  int ret = msgBox.exec();
  if( msgBox.clickedButton() == saveAsButton )
  {
    m_editor->saveAs();
  }
  else if( ret == QMessageBox::Save )
  {
    m_editor->saveToCurrentFile();
  }
  else
  {
    m_editor->setModified(false);
  }
}

/**
 * Set up the widget from a given scripting environment
 * @param environ :: A pointer to the current scripting environment
 * @param identifier :: A string identifier, used mainly in error messages to identify the
 * current script
 */
void ScriptFileInterpreter::setup(const ScriptingEnv & environ, const QString & identifier)
{
  setupEditor(environ, identifier);
  setupScriptRunner(environ, identifier);
}

/**
 * @return The string containing the filename of the script
 */
QString ScriptFileInterpreter::filename() const
{
  return m_editor->fileName();
}

/**
 * Has the script been modified
 * @return True if the script has been modified
 */
bool ScriptFileInterpreter::isScriptModified() const
{
  return m_editor->isModified();
}

/**
 * Populate a menu with editing items
 * @param fileMenu A reference to a menu object that is to be populated
 */
void ScriptFileInterpreter::populateFileMenu(QMenu &fileMenu)
{
  m_editor->populateFileMenu(fileMenu);

}

/**
 * Populate a menu with editing items
 * @param editMenu A reference to a menu object that is to be populated
 */
void ScriptFileInterpreter::populateEditMenu(QMenu &editMenu)
{
  m_editor->populateEditMenu(editMenu);
  editMenu.insertSeparator();
  m_messages->populateEditMenu(editMenu);
}

/**
 * Fill execute menu
 */
void ScriptFileInterpreter::populateExecMenu(QMenu &execMenu)
{
  execMenu.addAction(m_execSelect);
  execMenu.addAction(m_execAll);
}

/**
 * Fill a window menu
 * @param windowMenu
 */
void ScriptFileInterpreter::populateWindowMenu(QMenu &windowMenu)
{
  m_editor->populateWindowMenu(windowMenu);
}


/**
 * Execute the whole script in the editor. This is always asynchronous
 */
void ScriptFileInterpreter::executeAll()
{
  executeCode(m_editor->text());
}

/**
 * Execute the current selection from the editor. This is always asynchronous
 */
void ScriptFileInterpreter::executeSelection()
{
  executeCode(m_editor->selectedText());
}



/**
 * Save the current script in the editor to a file
 * @param filename :: The filename to save the script in
 */
void ScriptFileInterpreter::saveScript(const QString & filename)
{
  m_editor->saveScript(filename);
}

/**
 * Save the current output text to a file
 * @param filename :: The filename to save the script in
 */

void ScriptFileInterpreter::saveOutput(const QString & filename)
{
  m_messages->saveToFile(filename);
}

//-----------------------------------------------------------------------------
// Private members
//-----------------------------------------------------------------------------
/**
 * Create action objects
 */
void ScriptFileInterpreter::initActions()
{
  m_execSelect = new QAction(tr("E&xecute"), this);
  m_execSelect->setShortcut(tr("Ctrl+Return"));
  connect(m_execSelect, SIGNAL(activated()), this, SLOT(executeSelection()));

  m_execAll = new QAction(tr("Execute &All"), this);
  m_execAll->setShortcut(tr("Ctrl+Shift+Return"));
  connect(m_execAll, SIGNAL(activated()), this, SLOT(executeAll()));
}

/**
 * @param environ :: A pointer to the current scripting environment
 * @param identifier :: A string identifier, used mainly in error messages to identify the
 * current script
 */
void ScriptFileInterpreter::setupEditor(const ScriptingEnv & environ, const QString & identifier)
{
  if(QFileInfo(identifier).exists())
  {
    readFileIntoEditor(identifier);
    m_editor->setFileName(identifier);
  }
  m_editor->setLexer(environ.createCodeLexer());
  m_editor->setCursorPosition(0,0);
}

/**
 * @param environ :: A pointer to the current scripting environment
 * @param identifier :: A string identifier, used mainly in error messages to identify the
 * current script
 */
void ScriptFileInterpreter::setupScriptRunner(const ScriptingEnv & environ, const QString & identifier)
{
  m_runner = QSharedPointer<Script>(environ.newScript(identifier,this, Script::Interactive));
  connect(m_runner.data(), SIGNAL(started(const QString &)), m_messages, SLOT(displayMessageWithTimestamp(const QString &)));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), m_messages, SLOT(displayMessageWithTimestamp(const QString &)));
  connect(m_runner.data(), SIGNAL(print(const QString &)), m_messages, SLOT(displayMessage(const QString &)));
}

/**
 * Replace the contents of the editor with the given file
 * @param filename
 * @return True if the read succeeded, false otherwise
 */
bool ScriptFileInterpreter::readFileIntoEditor(const QString & filename)
{
  m_editor->setFileName(filename);
  QFile scriptFile(filename);
  if(!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text))
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"),
        tr("Could not open file \"%1\" for reading.").arg(filename));
    return false;
  }
  m_editor->read(&scriptFile);
  m_editor->setModified(false);
  scriptFile.close();
  return true;
}

/**
 * Use the current Script object to execute the code asynchronously
 * @param code :: The code string to run
 */
void ScriptFileInterpreter::executeCode(const QString & code)
{
  if( code.isEmpty() ) return;
  m_runner->executeAsync(code);

}
