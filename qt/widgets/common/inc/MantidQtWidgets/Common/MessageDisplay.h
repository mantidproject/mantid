#ifndef MESSAGEDISPLAY_H_
#define MESSAGEDISPLAY_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidQtWidgets/Common/Configurable.h"
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/QtSignalChannel.h"

#include <QHash>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QWidget>

//----------------------------------------------------------
// Forward declarations
//----------------------------------------------------------
class QAction;
class QActionGroup;
class QPlainTextEdit;
class QPoint;
class QSettings;
class QShowEvent;
class QSignalMapper;

namespace MantidQt {
namespace MantidWidgets {

/** @class MessageDisplay
 * Provides a widget for display messages in a text box
 * It deals with Message objects which in turn hide whether
 * a message is a framework Poco message or a simple string.
 * It can connect to the Mantid logging framework if required
 */
class EXPORT_OPT_MANTIDQT_COMMON MessageDisplay : public QWidget,
                                                  public Configurable {
  Q_OBJECT
  Q_PROPERTY(QString source READ source WRITE setSource)

public:
  // Configurable interface
  void readSettings(const QSettings &storage) override;
  void writeSettings(QSettings *storage) override;

public:
  /// Default constructor with optional parent
  MessageDisplay(QWidget *parent = nullptr);
  MessageDisplay(const MessageDisplay &) = delete;
  MessageDisplay &operator=(const MessageDisplay &) = delete;
  /// Destructor
  ~MessageDisplay() override;

  // Setup logging framework connections
  void attachLoggingChannel(int logLevel = 0);
  /// If set, only Mantid log messages from this source are emitted
  void setSource(const QString &source);
  /// Get the current source are emitted
  inline const QString &source() const { return m_logChannel->source(); }

signals:
  /// Indicate that a message of error or higher has been received.
  void errorReceived(const QString &text);
  /// Indicate that a message of warning or higher has been received.
  void warningReceived(const QString &text);

public slots:
  /// Convenience method for appending message at fatal level
  void appendFatal(const QString &text);
  /// Convenience method for appending message at error level
  void appendError(const QString &text);
  /// Convenience method for appending message at warning level
  void appendWarning(const QString &text);
  /// Convenience method for appending message at notice level
  void appendNotice(const QString &text);
  /// Convenience method for appending message at information level
  void appendInformation(const QString &text);
  /// Convenience method for appending message at debug level
  void appendDebug(const QString &text);
  /// Write a message after the current contents
  void append(const Message &msg);
  /// Replace the display text with the given contents
  void replace(const Message &msg);
  /// Clear all of the text
  void clear();
  /// Move the text cursor to after the last character
  QTextCursor moveCursorToEnd();
  /// Returns true if scroll-bar is at the bottom of widget
  bool isScrollbarAtBottom() const;
  /// Scroll to the bottom of the text
  void scrollToTop();
  /// Scroll to the bottom of the text
  void scrollToBottom();

private slots:
  /// Provide a custom context menu
  void showContextMenu(const QPoint &event);
  /// Set the global logging level
  void setLogLevel(int priority);

private:
  /// Setup the actions
  void initActions();
  /// Initialize the text formats
  void initFormats();
  /// Set the properties of the text display
  void setupTextArea();
  /// Return format for given log level
  QTextCharFormat format(const Message::Priority priority) const;

  /// A reference to the log channel
  QtSignalChannel *m_logChannel;
  /// The actual widget holding the text
  QPlainTextEdit *m_textDisplay;
  /// Map priority to text formatting
  QHash<Message::Priority, QTextCharFormat> m_formats;
  /// Mutually exclusive log actions
  QActionGroup *m_loglevels;
  /// Map action signal to log level parameter
  QSignalMapper *m_logLevelMapping;
  /// Log level actions
  QAction *m_error, *m_warning, *m_notice, *m_information, *m_debug;
};
}
}

#endif // MESSAGEDISPLAY_H_
