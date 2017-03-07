#include "ScriptFileInterpreter.h"
#include "ScriptOutputDisplay.h"
#include "ScriptingEnv.h"
#include "MantidQtMantidWidgets/ScriptEditor.h"
#include <QAction>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <qscilexer.h>

#include <stdexcept>

//-----------------------------------------------------------------------------
// ScriptFileInterpreter
//-----------------------------------------------------------------------------

/**
 * Construct a widget
 * @param parent :: The parent widget
 */
ScriptFileInterpreter::ScriptFileInterpreter(QWidget *parent,
                                             const QString &settingsGroup)
    : QWidget(parent), m_splitter(new QSplitter(Qt::Vertical, this)),
      m_editor(new ScriptEditor(this, NULL, settingsGroup)),
      m_messages(new ScriptOutputDisplay), m_status(new QStatusBar),
      m_runner() {

  // Initialise line wrapping to include visual arrow indicator
  m_editor->setWrapVisualFlags(QsciScintilla::WrapFlagByText);

  setupChildWidgets();

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showContextMenu(const QPoint &)));

  connect(m_editor, SIGNAL(textZoomedIn()), m_messages, SLOT(zoomUp()));
  connect(m_editor, SIGNAL(textZoomedOut()), m_messages, SLOT(zoomDown()));
  connect(m_messages, SIGNAL(textZoomedIn()), m_editor, SLOT(zoomIn()));
  connect(m_messages, SIGNAL(textZoomedOut()), m_editor, SLOT(zoomOut()));
  connect(m_editor, SIGNAL(textZoomedIn()), this, SLOT(emitZoomIn()));
  connect(m_editor, SIGNAL(textZoomedOut()), this, SLOT(emitZoomOut()));
  connect(m_messages, SIGNAL(textZoomedIn()), this, SLOT(emitZoomIn()));
  connect(m_messages, SIGNAL(textZoomedOut()), this, SLOT(emitZoomOut()));
}

/// Destroy the object
ScriptFileInterpreter::~ScriptFileInterpreter() {}

/**
 * Check if the interpreter is running and the script is saved
 * @return True if the interpreter should be closed
 */
bool ScriptFileInterpreter::shouldClose() {
  ScriptCloseDialog dialog(*this, this);
  return dialog.shouldScriptClose();
}

/// Convert tabs in selection to spaces
void ScriptFileInterpreter::tabsToSpaces() {
  int selFromLine, selFromInd, selToLine, selToInd;

  m_editor->getSelection(&selFromLine, &selFromInd, &selToLine, &selToInd);
  if (selFromLine == -1)
    m_editor->selectAll();

  QString text = m_editor->selectedText();

  // Use the tab space count for each converted character
  QString spaces = "";

  for (int i = 0; i < m_editor->tabWidth(); ++i)
    spaces = spaces + " ";

  text = text.replace("\t", spaces, Qt::CaseInsensitive);
  m_editor->replaceSelectedText(text);
}

/// Convert spaces in selection to tabs
void ScriptFileInterpreter::spacesToTabs() {
  int selFromLine, selFromInd, selToLine, selToInd;

  m_editor->getSelection(&selFromLine, &selFromInd, &selToLine, &selToInd);
  if (selFromLine == -1)
    m_editor->selectAll();

  QString text = m_editor->selectedText();

  // Use the tab space count for each converted characters
  QString spaces = "";

  for (int i = 0; i < m_editor->tabWidth(); ++i)
    spaces = spaces + " ";

  text = text.replace(spaces, "\t", Qt::CaseInsensitive);
  m_editor->replaceSelectedText(text);
}

/// Set a font
void ScriptFileInterpreter::setFont(const QString &fontFamily) {
  // This needs to check if the font exists and use default if not
  QFontDatabase database;

  // Select saved choice. If not available, use current font
  QString fontToUse = m_editor->lexer()->defaultFont().family();

  if (database.families().contains(fontFamily))
    fontToUse = fontFamily;

  QFont defaultFont = m_editor->lexer()->defaultFont();
  defaultFont.setFamily(fontToUse);
  m_editor->lexer()->setDefaultFont(defaultFont);

  // Check through all styles until it starts creating new ones (they match the
  // default style)
  // On each, copy the font and change only the family
  int count = 0;
  while (m_editor->lexer()->font(count) != m_editor->lexer()->defaultFont()) {
    QFont font = m_editor->lexer()->font(count);
    font.setFamily(fontToUse);
    m_editor->lexer()->setFont(font, count);

    count++;
  }
}

/// Toggle replacing tabs with whitespace
void ScriptFileInterpreter::toggleReplaceTabs(bool state) {
  m_editor->setIndentationsUseTabs(!state);
}

/// Number of spaces to insert for a tab
void ScriptFileInterpreter::setTabWhitespaceCount(int count) {
  m_editor->setTabWidth(count);
}

/// Toggles the whitespace on/off
void ScriptFileInterpreter::toggleWhitespace(bool state) {
  m_editor->setEolVisibility(state);

  if (state)
    m_editor->setWhitespaceVisibility(
        QsciScintilla::WhitespaceVisibility::WsVisible);
  else
    m_editor->setWhitespaceVisibility(
        QsciScintilla::WhitespaceVisibility::WsInvisible);
}

/// Comment a block of code
void ScriptFileInterpreter::comment() { toggleComment(true); }

/// Uncomment a block of code
void ScriptFileInterpreter::uncomment() { toggleComment(false); }

void ScriptFileInterpreter::toggleComment(bool addComment) {
  // Get selected text
  std::string whitespaces(" \t\f\r");
  int selFromLine, selFromInd, selToLine, selToInd;

  m_editor->getSelection(&selFromLine, &selFromInd, &selToLine, &selToInd);

  // Expand selection to first character on start line to the end char on second
  // line.
  if (selFromLine == -1) {
    m_editor->getCursorPosition(&selFromLine, &selFromInd);
    selToLine = selFromLine;
  }

  // For each line, select it, copy the line into a new string minus the comment
  QString replacementText = "";

  // If it's adding comment, check all lines to find the lowest index for a non
  // space character
  int minInd = -1;
  if (addComment) {
    for (int i = selFromLine; i <= selToLine; ++i) {
      std::string thisLine = m_editor->text(i).toUtf8().constData();
      int lineFirstChar =
          static_cast<int>(thisLine.find_first_not_of(whitespaces + "\n"));

      if (minInd == -1 || (lineFirstChar != -1 && lineFirstChar < minInd)) {
        minInd = lineFirstChar;
      }
    }
  }

  for (int i = selFromLine; i <= selToLine; ++i) {
    std::string thisLine = m_editor->text(i).toUtf8().constData();

    int lineFirstChar =
        static_cast<int>(thisLine.find_first_not_of(whitespaces + "\n"));

    if (lineFirstChar != -1) {
      if (addComment) {
        thisLine.insert(minInd, "#");
      } else {
        // Check that the first character is a #
        if (thisLine[lineFirstChar] ==
            '#') // Remove the comment, or ignore to add as is
        {
          thisLine = thisLine.erase(lineFirstChar, 1);
        }
      }
    }

    replacementText = replacementText + QString::fromStdString(thisLine);
  }

  m_editor->setSelection(selFromLine, 0, selToLine,
                         m_editor->lineLength(selToLine));
  m_editor->replaceSelectedText(replacementText);
  m_editor->setCursorPosition(selFromLine, selFromInd);
}

/**
 * Show custom context menu event
 */
void ScriptFileInterpreter::showContextMenu(const QPoint &clickPoint) {
  QMenu context(this);
  context.addAction("&Save", m_editor, SLOT(saveToCurrentFile()));

  auto *copyAction = context.addAction("&Copy", m_editor, SLOT(copy()));
  context.insertSeparator(copyAction);
  context.addAction("C&ut", m_editor, SLOT(cut()));
  context.addAction("P&aste", m_editor, SLOT(paste()));

  auto *execAction =
      context.addAction("E&xecute Selection", this, SLOT(executeSelection()));
  context.insertSeparator(execAction);
  context.addAction("Execute &All", this, SLOT(executeAll()));

  context.exec(this->mapToGlobal(clickPoint));
}

/**
 * Set the status bar when the script is executing
 */
void ScriptFileInterpreter::setExecutingStatus() {
  m_status->showMessage("Status: Executing...");
  m_editor->setReadOnly(true);
}

/**
 * Set the status bar when the script is stopped
 */
void ScriptFileInterpreter::setStoppedStatus() {
  m_status->showMessage("Status: Stopped");
  m_editor->setReadOnly(false);
}

void ScriptFileInterpreter::emitZoomIn() { emit textZoomedIn(); }
void ScriptFileInterpreter::emitZoomOut() { emit textZoomedOut(); }

/**
 * Set up the widget from a given scripting envment
 * @param env :: A pointer to the current scripting envment
 * @param identifier :: A string identifier, used mainly in error messages to
 * identify the
 * current script
 */
void ScriptFileInterpreter::setup(const ScriptingEnv &env,
                                  const QString &identifier) {
  setupEditor(env, identifier);
  setupScriptRunner(env, identifier);
  connect(m_runner.data(),
          SIGNAL(autoCompleteListGenerated(const QStringList &)), m_editor,
          SLOT(updateCompletionAPI(const QStringList &)));
  m_runner->generateAutoCompleteList();
  connect(m_runner.data(), SIGNAL(currentLineChanged(int, bool)), m_editor,
          SLOT(updateProgressMarker(int, bool)));
}

/**
 * @return The string containing the filename of the script
 */
QString ScriptFileInterpreter::filename() const { return m_editor->fileName(); }

/**
 * Has the script been modified
 * @return True if the script has been modified
 */
bool ScriptFileInterpreter::isScriptModified() const {
  return m_editor->isModified();
}

/// Is the script running
bool ScriptFileInterpreter::isExecuting() const {
  return m_runner->isExecuting();
}

/// Save to the currently stored name
void ScriptFileInterpreter::saveToCurrentFile() {
  m_editor->saveToCurrentFile();
  m_runner->setIdentifier(m_editor->fileName());
}

/// Save to a different name
void ScriptFileInterpreter::saveAs() {
  m_editor->saveAs();
  m_runner->setIdentifier(m_editor->fileName());
}

/**
 * Save the current script in the editor to a file
 * @param filename :: The filename to save the script in
 */
void ScriptFileInterpreter::saveScript(const QString &filename) {
  m_editor->saveScript(filename);
  m_runner->setIdentifier(m_editor->fileName());
}

/**
 * Save the current output text to a file
 * @param filename :: The filename to save the script in
 */

void ScriptFileInterpreter::saveOutput(const QString &filename) {
  m_messages->saveToFile(filename);
}

/**
 * Print script
 */
void ScriptFileInterpreter::printScript() { m_editor->print(); }

/// Print the output
void ScriptFileInterpreter::printOutput() { m_messages->print(); }

/// Undo
void ScriptFileInterpreter::undo() { m_editor->undo(); }
/// Redo
void ScriptFileInterpreter::redo() { m_editor->redo(); }

/// Copy from the editor
void ScriptFileInterpreter::copy() { m_editor->copy(); }

/// Cut from the editor
void ScriptFileInterpreter::cut() { m_editor->cut(); }
/// Paste into the editor
void ScriptFileInterpreter::paste() { m_editor->paste(); }
/// Find in editor
void ScriptFileInterpreter::showFindReplaceDialog() {
  m_editor->showFindReplaceDialog();
}

/**
 * Execute the whole script in the editor. Always clears the contents of the
 * local variable dictionary first.
 */
void ScriptFileInterpreter::executeAll(const Script::ExecutionMode mode) {
  m_runner->clearLocals();
  executeCode(ScriptCode(m_editor->text()), mode);
}

/**
 * Execute the current selection from the editor.
 */
void ScriptFileInterpreter::executeSelection(const Script::ExecutionMode mode) {
  if ((m_editor->hasSelectedText() && (!m_editor->selectedText().isEmpty()))) {
    int firstLineOffset(0), unused(0);
    m_editor->getSelection(&firstLineOffset, &unused, &unused, &unused);
    executeCode(ScriptCode(m_editor->selectedText(), firstLineOffset), mode);
  } else {
    executeAll(mode);
  }
}

/**
 * The envment has to support this behaviour or is does nothing
 */
void ScriptFileInterpreter::abort() { m_runner->abort(); }

/**
 */
void ScriptFileInterpreter::clearVariables() { m_runner->clearLocals(); }

/// Toggles the progress reports on/off
void ScriptFileInterpreter::toggleProgressReporting(bool state) {
  if (state)
    m_runner->enableProgressReporting();
  else {
    m_editor->setMarkerState(false);
    m_runner->disableProgressReporting();
  }
}

/// Toggles the code folding on/off
void ScriptFileInterpreter::toggleCodeFolding(bool state) {
  if (state) {
    m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);
  } else {
    m_editor->setFolding(QsciScintilla::NoFoldStyle);
  }
}

/// Toggles soft wrapping of text on/off;
void ScriptFileInterpreter::toggleLineWrapping(bool state) {
  if (state) {
    m_editor->setWrapMode(QsciScintilla::WrapWord);
  } else {
    m_editor->setWrapMode(QsciScintilla::WrapNone);
  }
}

//-----------------------------------------------------------------------------
// Private members
//-----------------------------------------------------------------------------
/**
 * Create the splitter and layout for the child widgets
 */
void ScriptFileInterpreter::setupChildWidgets() {
  m_splitter->addWidget(m_editor);
  m_splitter->addWidget(m_messages);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(m_splitter);
  mainLayout->addWidget(m_status);
  setLayout(mainLayout);

  setFocusProxy(m_editor);
  m_editor->setFocus();
}

/**
 * @param env :: A pointer to the current scripting envment
 * @param identifier :: A string identifier, used mainly in error messages to
 * identify the
 * current script
 */
void ScriptFileInterpreter::setupEditor(const ScriptingEnv &env,
                                        const QString &identifier) {
  if (QFileInfo(identifier).exists()) {
    readFileIntoEditor(identifier);
  }
  m_editor->setLexer(env.createCodeLexer());
  m_editor->setSettingsGroup("ScriptWindow");
  m_editor->padMargin();
  m_editor->setAutoMarginResize();
  m_editor->enableAutoCompletion();
  m_editor->setCursorPosition(0, 0);

  connect(m_editor, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
  connect(m_editor, SIGNAL(modificationChanged(bool)), this,
          SIGNAL(editorModificationChanged(bool)));
  connect(m_editor, SIGNAL(undoAvailable(bool)), this,
          SIGNAL(editorUndoAvailable(bool)));
  connect(m_editor, SIGNAL(redoAvailable(bool)), this,
          SIGNAL(editorRedoAvailable(bool)));
}

/**
 * @param env :: A pointer to the current scripting envment
 * @param identifier :: A string identifier, used mainly in error messages to
 * identify the
 * current script
 */
void ScriptFileInterpreter::setupScriptRunner(const ScriptingEnv &env,
                                              const QString &identifier) {
  m_runner = QSharedPointer<Script>(
      env.newScript(identifier, this, Script::Interactive));

  connect(m_runner.data(), SIGNAL(started(const QString &)), this,
          SLOT(setExecutingStatus()));
  connect(m_runner.data(), SIGNAL(started(const QString &)), m_messages,
          SLOT(displayMessageWithTimestamp(const QString &)));
  connect(m_runner.data(), SIGNAL(started(const QString &)), this,
          SIGNAL(executionStarted()));

  connect(m_runner.data(), SIGNAL(finished(const QString &)), m_messages,
          SLOT(displayMessageWithTimestamp(const QString &)));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this,
          SLOT(setStoppedStatus()));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this,
          SIGNAL(executionStopped()));

  connect(m_runner.data(), SIGNAL(print(const QString &)), m_messages,
          SLOT(displayMessage(const QString &)));

  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)),
          m_messages, SLOT(displayError(const QString &)));
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)),
          this, SLOT(setStoppedStatus()));
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)),
          this, SIGNAL(executionStopped()));
}

/**
 * Replace the contents of the editor with the given file
 * @param filename
 * @return True if the read succeeded, false otherwise
 */
bool ScriptFileInterpreter::readFileIntoEditor(const QString &filename) {
  m_editor->setFileName(filename);
  QFile scriptFile(filename);
  if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(
        this, tr("MantidPlot - File error"),
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
void ScriptFileInterpreter::executeCode(const ScriptCode &code,
                                        const Script::ExecutionMode mode) {
  if (code.isEmpty())
    return;
  if (mode == Script::Asynchronous) {
    try {
      m_runner->executeAsync(code);
    } catch (std::runtime_error &exc) {
      QMessageBox::critical(this, "MantidPlot", exc.what());
    }
  } else if (mode == Script::Serialised) {
    m_runner->execute(code);
  } else {
    QMessageBox::warning(this, "MantidPlot", "Unknown script execution mode");
  }
}

//-----------------------------------------------------------------------------
// ScriptCloseDialog
//-----------------------------------------------------------------------------
ScriptCloseDialog::ScriptCloseDialog(ScriptFileInterpreter &interpreter,
                                     QWidget *parent)
    : QWidget(parent), m_msgBox(new QMessageBox(this)),
      m_interpreter(interpreter) {
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_msgBox);
  setLayout(layout);
}

/**
 * Raise the the dialog as modal if necessary and ask the user
 * if the script should close
 * @return True or False depending on whether the script should be closed
 */
bool ScriptCloseDialog::shouldScriptClose() {
  const bool executing(m_interpreter.isExecuting()),
      modified(m_interpreter.isScriptModified());
  // Is the dialog even necessary?
  if (!modified && !executing)
    return true;

  m_msgBox->setModal(true);
  m_msgBox->setWindowTitle("MantidPlot");
  m_msgBox->setIcon(QMessageBox::Question);

  QString filename = m_interpreter.filename();
  bool close(true);
  if (modified) {
    m_msgBox->addButton(QMessageBox::Save);
    m_msgBox->addButton(QMessageBox::Cancel);
    m_msgBox->addButton(QMessageBox::Discard);
    m_msgBox->setDefaultButton(QMessageBox::Save);
    if (filename.isEmpty()) {
      m_msgBox->setText(QString("Save changes before closing?"));
      m_msgBox->button(QMessageBox::Save)->setText("Save As");
    } else {
      m_msgBox->setText(
          QString("Save changes to '%1' before closing?").arg(filename));
    }
    if (executing) {
      m_msgBox->setInformativeText(tr("The script will be aborted."));
    }
    // show dialog
    int ret = m_msgBox->exec();
    try {
      switch (ret) {
      case QMessageBox::Save:
        m_interpreter.saveToCurrentFile();
        close = true;
        break;
      case QMessageBox::Discard:
        close = true;
        break;
      default:
        close = false;
        break;
      }
    } catch (ScriptEditor::SaveCancelledException &) {
      close = false;
    }
  } else if (executing) {
    if (filename.isEmpty()) {
      m_msgBox->setText(tr("Abort and close?"));
    } else {
      m_msgBox->setText(
          tr("Abort '%1' and close?").arg(m_interpreter.filename()));
    }
    m_msgBox->addButton(QMessageBox::Abort);
    m_msgBox->addButton(QMessageBox::Cancel);
    m_msgBox->setDefaultButton(QMessageBox::Abort);
    int ret = m_msgBox->exec();
    switch (ret) {
    case QMessageBox::Abort:
      m_interpreter.m_runner->abort();
      close = true;
      break;
    default:
      close = false;
      break;
    }
  }
  return close;
}
