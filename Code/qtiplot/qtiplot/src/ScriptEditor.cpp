//---------------------------------------------
// Includes
//-----------------------------------------------
#include "ScriptEditor.h"

// Qt
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QAction>

#include <iostream>

//***************************************************************************
//
// ScriptEditor class
//
//***************************************************************************
//------------------------------------------------
// Public member functions
//------------------------------------------------
/**
 * Constructor
 * @param parent The parent widget (can be NULL)
 */
ScriptEditor::ScriptEditor(QWidget *parent) : 
  QsciScintilla(parent), m_filename("")
{
  connect(this, SIGNAL(textChanged()), this, SLOT(update()));
  
  // Undo action
  m_undo = new QAction(tr("&Undo"), this);
  m_undo->setShortcut(tr("Ctrl+Z"));
  connect(m_undo, SIGNAL(activated()), this, SLOT(undo()));
  connect(this, SIGNAL(undoAvailable(bool)), m_undo, SLOT(setEnabled(bool)));
  // Redo action
  m_redo = new QAction(tr("&Redo"), this);
  m_redo->setShortcut(tr("Ctrl+Y"));
  connect(m_redo, SIGNAL(activated()), this, SLOT(redo()));
  connect(this, SIGNAL(redoAvailable(bool)), m_redo, SLOT(setEnabled(bool)));

  //Cut
  m_cut = new QAction(tr("C&ut"), this);
  m_cut->setShortcut(tr("Ctrl+X"));
  connect(m_cut, SIGNAL(activated()), this, SLOT(cut()));
  connect(this, SIGNAL(copyAvailable(bool)), m_cut, SLOT(setEnabled(bool)));

  //Copy
  m_copy = new QAction(tr("&Copy"), this);
  m_copy->setShortcut(tr("Ctrl+C"));
  connect(m_copy, SIGNAL(activated()), this, SLOT(copy()));
  connect(this, SIGNAL(copyAvailable(bool)), m_copy, SLOT(setEnabled(bool)));

  //Paste
  m_paste = new QAction(tr("&Paste"), this);
  m_paste->setShortcut(tr("Ctrl+V"));
  connect(m_paste, SIGNAL(activated()), this, SLOT(paste()));

  //Update the editor
  update();
}

/**
 * Destructor
 */
ScriptEditor::~ScriptEditor()
{
}

//-----------------------------------------------
// Public slots
//-----------------------------------------------
/**
 * Save the text to the given filename
 * @param filename The filename to use
 */
bool ScriptEditor::saveScript(const QString & filename)
{
  if( filename.isEmpty() )
  {
    return false;
  }
  
  QFile file(filename);
  if( !file.open(QIODevice::WriteOnly) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
			  tr("Could not open file \"%1\" for writing.").arg(filename));
    return false;
  }

  QTextStream writer(&file);
  writer.setEncoding(QTextStream::UnicodeUTF8);
  writer << text();
  file.close();

  return true;
}

//-----------------------------------------------------
// Private slots
//-----------------------------------------------------
/**
 * Update the editor
 */
void ScriptEditor::update()
{
  emit undoAvailable(isUndoAvailable());
  emit redoAvailable(isRedoAvailable());
}

