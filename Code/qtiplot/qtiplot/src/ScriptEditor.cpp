//---------------------------------------------
// Includes
//-----------------------------------------------
#include "ScriptEditor.h"

// Qt
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QAction>
#include <QPrintDialog>
#include <QPrinter>

// std
#include <cmath>

//***************************************************************************
//
// ScriptEditor class
//
//***************************************************************************

// The colour for a success marker
QColor ScriptEditor::g_success_colour = QColor("lightgreen");
// The colour for an error marker
QColor ScriptEditor::g_error_colour = QColor("red");

//------------------------------------------------
// Public member functions
//------------------------------------------------
/**
 * Constructor
 * @param parent The parent widget (can be NULL)
 */
ScriptEditor::ScriptEditor(QWidget *parent) : 
  QsciScintilla(parent), m_filename(""), m_marker_handle(markerDefine(QsciScintilla::RightArrow))
{
  //Editor properties
  setAutoIndent(true);
  setMarginLineNumbers(1,true);
  
  // Update for a text change
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

  //Print
  m_print = new QAction(tr("&Print script"), this);
  m_print->setShortcut(tr("Ctrl+P"));
  connect(m_print, SIGNAL(activated()), this, SLOT(print()));

  //Update the editor
  update();
}

/**
 * Destructor
 */
ScriptEditor::~ScriptEditor()
{
}

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

//-----------------------------------------------
// Public slots
//-----------------------------------------------
/**
 * Update the editor
 */
void ScriptEditor::update()
{
  emit undoAvailable(isUndoAvailable());
  emit redoAvailable(isRedoAvailable());

  setMarginWidth( 1, 38 + 5*std::log10(static_cast<double>(lines())) );
}

/**
 * Update the arrow marker to point to the correct line and colour it depending on the error state
 * @param lineno The line to place the marker at. A negative number will clear all markers
 * @param success If false, the marker will turn red
 */
void ScriptEditor::updateMarker(int lineno, bool success)
{
  markerDeleteAll();
  if( lineno < 0 ) return;

  if( success )
  {
    setMarkerBackgroundColor(g_success_colour, m_marker_handle);
  }
  else
  {
    setMarkerBackgroundColor(g_error_colour, m_marker_handle);
  }
  ensureLineVisible(lineno);
  markerAdd(lineno - 1, m_marker_handle);
}

/**
 * Print the current text
 */
void ScriptEditor::print()
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Script"));
  if (print_dlg->exec() != QDialog::Accepted) 
  {
    return;
  }
  QTextDocument document(text());
  document.print(&printer);
}


