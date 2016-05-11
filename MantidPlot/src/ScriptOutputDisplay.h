#ifndef SCRIPTOUTPUTDISPLAY_H_
#define SCRIPTOUTPUTDISPLAY_H_

#include <QTextEdit>

/**
 * Defines a "read-only" text area that can be used
 * to output messages
 */
class ScriptOutputDisplay : public QTextEdit
{
  Q_OBJECT

public:
  /// Constructor
  explicit ScriptOutputDisplay(QWidget *parent = NULL);

  /// Is there anything here
  bool isEmpty() const;

  /// Add actions applicable to an edit menu
  void populateEditMenu(QMenu &editMenu);
  ///Capture key presses
  void keyPressEvent(QKeyEvent *event) override;
  //squash dragging ability
  void mouseMoveEvent(QMouseEvent *e) override;
  //prevent middle mouse clicks from pasting
  void mouseReleaseEvent(QMouseEvent *e) override;
  /// capture ctrl_up or down to zoom
  void wheelEvent(QWheelEvent *e) override;
  //sets the zoom to a specific level
  void setZoom(int value);
  /// zooms the text size
  void zoom(int range=1);
  /// returns the current zoom level
  int zoomLevel();
public slots:
  //zooms in, not called ZoomIn to avoid clashing with the base ZoomIn that does not work
  void zoomUp();  
  //zooms in, not called ZoomIn to avoid clashing with the base ZoomIn that does not work
  void zoomDown();
  /// Print the text within the window
  void print();
  /// Save the output to a file
  void saveToFile(const QString & filename = "");
  /// Display an output message that is not an error
  void displayMessage(const QString & msg);
  /// Display an output message with a timestamp & border
  void displayMessageWithTimestamp(const QString & msg);
  /// Display an error message
  void displayError(const QString & msg);

signals:
  /// Emitted when a zoom in is requested
  void textZoomedIn();
  /// Emitted when a zoom out is requested
  void textZoomedOut();

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
  ///original default font size
  int m_origFontSize;
  ///current zoom level
  int m_zoomLevel;
};

#endif /* SCRIPTOUTPUTDISPLAY_H_ */
