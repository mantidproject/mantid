// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include <QDialog>
#include <QTextDocument>
#include <Qsci/qsciscintilla.h>

//----------------------------------
// Forward declarations
//----------------------------------
class FindReplaceDialog;

class QAction;
class QMenu;
class QMimeData;
class QKeyEvent;
class QMouseEvent;
class QsciAPIs;

/**
    This class provides an area to write scripts. It inherits from QScintilla to
   use
    functionality such as auto-indent and if supported, syntax highlighting.

    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON ScriptEditor : public QsciScintilla {
  // Qt macro
  Q_OBJECT

public:
  /**
   * Exception type to indicate that saving was cancelled
   */
  class SaveCancelledException : public std::exception {
  public:
    /// Return a message
    const char *what() const noexcept override { return m_msg.c_str(); }

  private:
    std::string m_msg{"File Saving was cancelled"};
  };

public:
  ScriptEditor(const QString &lexerName, const QFont &font = QFont(), QWidget *parent = nullptr);
  ScriptEditor(QWidget *parent = nullptr, QsciLexer *lexer = nullptr, QString settingsGroup = "");
  /// Destructor
  ~ScriptEditor() override;

  /// Set the name of the group to save the settings for
  void setSettingsGroup(const QString &name);
  /// Settings group
  QString &settingsGroup() const;
  /// Read settings from persistent store
  void readSettings();
  /// Write settings from persistent store
  void writeSettings();

  /// Set a new code lexer for this object
  void setLexer(QsciLexer * /*codelexer*/) override final;
  // Make the object resize to margin to fit the contents
  void setAutoMarginResize();
  /// Enable the auto complete. Default is for backwards compatability
  /// with existing code
  void enableAutoCompletion(AutoCompletionSource source = QsciScintilla::AcsAPIs);
  /// Disable the auto complete
  void disableAutoCompletion();

  // Size hint
  QSize sizeHint() const override;
  // Unhide base class method to avoid intel compiler warning
  using QsciScintilla::setText;
  /// Set the text on a given line number
  void setText(int lineno, const QString &text, int index = 0);
  /// Capture key presses
  void keyPressEvent(QKeyEvent *event) override;
  /// The current filename
  const inline QString &fileName() const { return m_filename; }
  /// Set a new file name
  void setFileName(const QString &filename);

  /// Override so that ctrl + mouse wheel will zoom in and out
  void wheelEvent(QWheelEvent *e) override;

  /// Clear keyboard shortcut binding
  void clearKeyBinding(const QString &keyCombination);

  /// Return a pointer to the object responsible for code completion
  inline QsciAPIs *scintillaAPI() const { return m_completer; }

  /// Replace all occurences of a string
  void replaceAll(const QString &search, const QString &replace, bool regex, bool caseSensitive, bool matchWords,
                  bool wrap, bool forward = true);

  /// Get the current zoom factor
  int getZoom() const;

  // C++ does not recognise QT macros
  // cppcheck-suppress unknownMacro
public slots:
  /// Save the script, opening a dialog
  void saveAs();
  /// Save to the current filename, opening a dialog if blank
  void saveToCurrentFile();
  /// Save a the text to the given filename
  void saveScript(const QString &filename);

  /// Ensure the margin width is big enough to hold everything + padding
  void padMargin();
  /// Set the marker state
  void setMarkerState(bool enabled);
  /// Update the progress marker potentially from a separate thread
  void updateProgressMarkerFromThread(int lineno, bool error = false);
  /// Update the progress marker
  void updateProgressMarker(int lineno, bool error = false);
  /// Mark the progress arrow as an error
  void markExecutingLineAsError();
  /// Refresh the autocomplete information base on a new set of keywords
  void updateCompletionAPI(const QStringList &keywords);
  /// Print the text within the widget
  void print();
  /// Raise find replace dialog
  virtual void showFindReplaceDialog();
  /// Mark the file as modified
  void markFileAsModified();

signals:
  /// Inform observers that undo information is available
  void undoAvailable(bool /*_t1*/);
  /// Inform observers that redo information is available
  void redoAvailable(bool /*_t1*/);
  /// Emitted when a zoom in is requested
  void textZoomedIn();
  /// Emitted when a zoom out is requested
  void textZoomedOut();
  /// Notify that the filename has been modified
  void fileNameChanged(const QString &fileName);
  /// Progress has been made in script execution
  void progressMade(const int progress);
  /// The editor now has focus
  void editorFocusIn(const QString &filename);

protected:
  /// Write to the given device
  virtual void writeToDevice(QIODevice &device) const;

  void dropEvent(QDropEvent *de) override;
  void dragMoveEvent(QDragMoveEvent *de) override;
  void dragEnterEvent(QDragEnterEvent *de) override;
  QByteArray fromMimeData(const QMimeData *source, bool &rectangular) const override;
  void focusInEvent(QFocusEvent *fe) override;

private slots:

private:
  /// Forward a KeyPress event to QsciScintilla base class. Necessary due to bug
  /// in QsciScintilla
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
