#ifndef MESSAGEDISPLAY_H_
#define MESSAGEDISPLAY_H_

//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/QtSignalChannel.h"

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
    class Message;

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
