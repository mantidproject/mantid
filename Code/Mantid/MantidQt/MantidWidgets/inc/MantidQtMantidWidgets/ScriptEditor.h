
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
class FindReplaceDialog;

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

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>   
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ScriptEditor : public QsciScintilla
{
  // Qt macro
  Q_OBJECT

public:
  /**
   * Exception type to indicate that saving was cancelled
   */
  class SaveCancelledException : public std::exception
  {
  public:
    /// Return a message
    const char* what() const throw() { return "File saving was cancelled"; }
  };


public:
  /// Constructor
  ScriptEditor(QWidget* parent = 0, QsciLexer* lexer = NULL,
               const QString & settingsGroup = "");
  ///Destructor
  ~ScriptEditor();

  /// Set the name of the group to save the settings for
  void setSettingsGroup(const QString & name);
  /// Settings group
  QString settingsGroup() const;
  /// Read settings from persistent store
  void readSettings();
  /// Write settings from persistent store
  void writeSettings();

  /// Set a new code lexer for this object
  void setLexer(QsciLexer *);
  // Make the object resize to margin to fit the contents
  void setAutoMarginResize();
  /// Enable the auto complete
  void enableAutoCompletion();
  /// Disable the auto complete
  void disableAutoCompletion();

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
  void saveScript(const QString & filename);

  /// Ensure the margin width is big enough to hold everything + padding
  void padMargin();
  /// Set the marker state
  void setMarkerState(bool enabled);
  /// Update the progress marker
  void updateProgressMarker(int lineno, bool error);
  /// Mark the progress arrow as an error
  void markExecutingLineAsError();
  /// Refresh the autocomplete information base on a new set of keywords
  void updateCompletionAPI(const QStringList & keywords);
  /// Print the text within the widget
  void print();
  /// Raise find replace dialog
  virtual void showFindReplaceDialog();

  /// Override zoomTo slot
  virtual void zoomTo(int level);

signals:
  /// Inform observers that undo information is available
  void undoAvailable(bool);
  /// Inform observers that redo information is available
  void redoAvailable(bool);
  /// Emitted when a zoom in is requested
  void textZoomedIn();
  /// Emitted when a zoom in is requested
  void textZoomedOut();

protected:
  /// Write to the given device
  virtual void writeToDevice(QIODevice & device) const;
  
  virtual void dropEvent(QDropEvent *de);
  virtual void dragMoveEvent(QDragMoveEvent *de);
  virtual void dragEnterEvent(QDragEnterEvent *de);

private slots:

private:
  /// Forward a KeyPress event to QsciScintilla base class. Necessary due to bug in QsciScintilla
  void forwardKeyPressToBase(QKeyEvent *event);

  /// The file name associated with this editor
  QString m_filename;

  /// The margin marker 
  int m_progressArrowKey;
  /// Hold the line number of the currently executing line
  int m_currentExecLine;
  /// A pointer to a QsciAPI object that handles the code completion
  QsciAPIs *m_completer;
  /// The colour of the marker for a success state
  static QColor g_success_colour;
  /// The colour of the marker for an error state
  static QColor g_error_colour;
  /// previous key
  int m_previousKey;
  /// A pointer to the find replace dialog
  FindReplaceDialog *m_findDialog;
  /// Name of group that the settings are stored under
  QString m_settingsGroup;
};






#endif //SCRIPTEDITOR_H_
