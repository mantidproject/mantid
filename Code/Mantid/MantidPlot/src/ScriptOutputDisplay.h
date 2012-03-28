#ifndef SCRIPTOUTPUTDISPLAY_H_
#define SCRIPTOUTPUTDISPLAY_H_

#include <QTextEdit>

/**
 * Defines a read-only text area that can be used
 * to output messages
 */
class ScriptOutputDisplay : public QTextEdit
{
  Q_OBJECT

public:
  /// Constructor
  ScriptOutputDisplay(QWidget *parent = NULL);

  /// Is there anything here
  bool isEmpty() const;

  /// Add actions applicable to an edit menu
  void populateEditMenu(QMenu &editMenu);

public slots:
  /// Clear the text
  void clear();
  /// Print the text within the window
  void print();
  /// Save the output to a file
  void saveToFile(const QString & filename = "");
  /// Change the title based on the script's execution state
  void setScriptIsRunning(bool running);
  /// Display an output message that is not an error
  void displayMessage(const QString & msg);
  /// Disply an output message with a timestamp & border
  void displayMessageWithTimestamp(const QString & msg);
  /// Display an error message
  void displayError(const QString & msg);

private slots:
  /// Context menu slot
  void showContextMenu(const QPoint & pos);

private:
  enum MessageType { Standard, Error };
  /// Setup the cursor for a new message
  void prepareForNewMessage(const MessageType msgType);
  /// Add the timestamp formatting
  QString addTimestamp(const QString &);
  /// Append new text
  void appendText(const QString & txt);
  /// Create the action pointers
  void initActions();
  /// Reset the default font
  void resetFont();

private:
  /// Copy action
  QAction *m_copy;
  /// Clear action
  QAction *m_clear;
  /// Save action
  QAction *m_save;
};

#endif /* SCRIPTOUTPUTDISPLAY_H_ */
