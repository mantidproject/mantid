
#ifndef SCRIPTEDITOR_H_
#define SCRIPTEDITOR_H_

//----------------------------------
// Includes
//----------------------------------
#include <Qsci/qsciscintilla.h>
#include <QDialog>
#include <QTextDocument>
#include "WidgetDllOption.h"

//----------------------------------
// Forward declarations
//----------------------------------
class QAction;
class QMenu;
class QKeyEvent;
class QMouseEvent;
class QsciAPIs;

/**
 * A small wrapper around a QStringList to manage a command history
 */
struct EXPORT_OPT_MANTIDQT_MANTIDWIDGETS CommandHistory
{
  ///Default constructor
  CommandHistory() : m_commands(), m_hist_maxsize(1000), m_current(0) {}
  /// Add a block of lines
  void addCode(QString block);
  /// Add a command. 
  void add(QString command);
  /// Is there a previous command
  bool hasPrevious() const;
  /// Get the item pointed to by the current index and move it up one
  QString getPrevious() const;
  /// Is there a command next on the stack
  bool hasNext() const;
  /// Get the item pointed to by the current index and move it down one
  QString getNext() const;

private:
  /// Store a list of command strings
  QStringList m_commands;
  /// History size
  int m_hist_maxsize;
  /// Index "pointer"
  mutable int m_current;
};

/** 
    This class provides an area to write scripts. It inherits from QScintilla to use
    functionality such as auto-indent and if supported, syntax highlighting.
        
    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>   
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ScriptEditor : public QsciScintilla
{
  // Qt macro
  Q_OBJECT;

public:
  /// Constructor
  ScriptEditor(QWidget* parent = 0, QsciLexer* lexer = NULL);
  ///Destructor
  ~ScriptEditor();

  /// Add actions applicable to an edit menu
  void populateFileMenu(QMenu &fileMenu);
  /// Add actions applicable to an edit menu
  void populateEditMenu(QMenu &editMenu);
  /// Add actions applicable to a window menu
  void populateWindowMenu(QMenu &windowMenu);

  // Set a new code lexer for this object
  void setLexer(QsciLexer *);
  // Size hint
  QSize sizeHint() const;
  // Unhide base class method to avoid intel compiler warning
  using QsciScintilla::setText;
  /// Set the text on a given line number
  void setText(int lineno, const QString& text,int index=0);
  ///Capture key presses
  void keyPressEvent(QKeyEvent* event);
  /// The current filename
  inline QString fileName() const
  {
    return m_filename;
  }
  /**
   * Set a new file name
   * @param filename :: The new filename
   */
  inline void setFileName(const QString & filename)
  {
    m_filename = filename;
  }
  

  /// Override so that ctrl + mouse wheel will zoom in and out
  void wheelEvent( QWheelEvent * e );

  /// Return a pointer to the object responsible for code completion
  inline QsciAPIs * scintillaAPI() const
  {
    return m_completer;
  }
  
public slots:
  /// Save the script, opening a dialog
  void saveAs();
  /// Save to the current filename, opening a dialog if blank
  void saveToCurrentFile();
  /// Save a the text to the given filename
  bool saveScript(const QString & filename);

  /// Update the editor
  void update();
  /// Set the marker state
  void setMarkerState(bool enabled);
  /// Update the marker on this widget
  void updateMarker(int lineno, bool success);
  /// Refresh the autocomplete information base on a new set of keywords
  void updateCompletionAPI(const QStringList & keywords);
  /// Print the text within the widget
  void print();
  
  /// Override the zoomIn slot
  virtual void zoomIn();
  /// Override the zoomIn slot
  virtual void zoomIn(int level);
  /// Override the zoomOut slot
  virtual void zoomOut();
  /// Override the zoomOut slot
  virtual void zoomOut(int level);

signals:
  /// Inform observers that undo information is available
  void undoAvailable(bool);
  /// Inform observers that redo information is available
  void redoAvailable(bool);

private:
  /// Create actions
  void initActions();

  /// Settings group
  QString settingsGroup() const;
  /// Read settings from persistent store
  void readSettings();
  /// Write settings from persistent store
  void writeSettings();
  /// Forward a KeyPress event to QsciScintilla base class. Necessary due to bug in QsciScintilla
  void forwardKeyPressToBase(QKeyEvent *event);

  /// The file name associated with this editor
  QString m_filename;

  /// Saving actions
  QAction *m_save, *m_saveAs;
  /// Each editor needs its own undo/redo etc
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste, *m_print;
  /// Zoom in/out actions
  QAction *m_zoomIn,*m_zoomOut;
  /// The margin marker 
  int m_progressArrowKey;
  /// A pointer to a QsciAPI object that handles the code completion
  QsciAPIs *m_completer;
  /// The colour of the marker for a success state
  static QColor g_success_colour;
  /// The colour of the marker for an error state
  static QColor g_error_colour;
  /// previous key
  int m_previousKey;
  /// How many times the zoom level is changed
  int m_zoomLevel;
 
};






#endif //SCRIPTEDITOR_H_
