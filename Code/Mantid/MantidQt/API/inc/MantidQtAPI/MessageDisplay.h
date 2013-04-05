#ifndef MESSAGEDISPLAY_H_
#define MESSAGEDISPLAY_H_

//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/Message.h"
#include "MantidQtAPI/QtSignalChannel.h"

#include <QColor>
#include <QWidget>

//----------------------------------------------------------
// Forward declarations
//----------------------------------------------------------
class QAction;
class QActionGroup;
class QPoint;
class QSignalMapper;
class QTextEdit;

namespace MantidQt
{
  namespace API
  {
    //----------------------------------------------------------
    // Forward declarations
    //----------------------------------------------------------

    /** @class MessageDisplay
     * Provides a widget for display messages in a text box
     * It deals with Message objects which in turn hide whether
     * a message is a framework Poco message or a simple string
     */
    class MessageDisplay : public QWidget
    {
      Q_OBJECT

    public:
      /// Controls whether the display is allowed to set the log levels
      enum LogLevelControl {
        EnableLogLevelControl = 0,
        DisableLogLevelControl = 1
      };

      /// Default constructor
      MessageDisplay(LogLevelControl logLevelControl=DisableLogLevelControl,
                     QWidget *parent=NULL);
      ///Destructor
      ~MessageDisplay();

      // Setup logging framework connections
      void attachLoggingChannel();

    public slots:
      /// Convenience method for appending message at fatal level
      void appendFatal(const QString & text);
      /// Convenience method for appending message at error level
      void appendError(const QString & text);
      /// Convenience method for appending message at warning level
      void appendWarning(const QString & text);
      /// Convenience method for appending message at notice level
      void appendNotice(const QString & text);
      /// Convenience method for appending message at information level
      void appendInformation(const QString & text);
      /// Convenience method for appending message at debug level
      void appendDebug(const QString & text);

      /// Write a message after the current contents
      void append(const Message & msg);

      /// Replace the display text with the given contents
      void replace(const Message & msg);
      /// Clear all of the text
      void clear();

    private slots:
      /// Provide a custom context menu
      void showContextMenu(const QPoint & event);
      /// Set the global logging level
      void setGlobalLogLevel(int priority);

    private:
      Q_DISABLE_COPY(MessageDisplay);
      /// Setup the actions
      void initActions();
      /// Set the properties of the text display
      void setupTextArea();
      /// Sets the text color for the given priority
      void setTextColor(const Message::Priority priority);
      /// Returns the text color for a given priority
      QColor textColor(const Message::Priority priority) const;
      /// Appends the given text & makes sure it can be seen
      void appendText(const QString & text);

      /// Are we allowed to affect the log level
      LogLevelControl m_logLevelControl;
      /// A reference to the
      QtSignalChannel *m_logChannel;
      /// The actual widget holding the text
      QTextEdit * m_textDisplay;
      /// Mutually exclusive log actions
      QActionGroup *m_loglevels;
      /// Map action signal to log level parameter
      QSignalMapper *m_logLevelMapping;
      /// Log level actions
      QAction  *m_error,*m_warning,*m_notice, *m_information, *m_debug;
    };

  }
}

#endif //MESSAGEDISPLAY_H_
