#include "MantidKernel/Logger.h"
#include "MantidKernel/ThreadSafeLogStream.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/NullStream.h>
#include <iostream>
#include <sstream>

namespace Mantid
{
namespace Kernel
{
  // Initialize the static members
  Logger::LoggerList* Logger::m_loggerList = NULL;
  Mutex* Logger::mutexLoggerList = NULL;
  Poco::NullOutputStream* Logger::m_nullStream = NULL;

  /** Constructor
   * @param name :: The class name invoking this logger
   */
  Logger::Logger(const std::string& name) : m_enabled(true)
  {
    m_name = name;
    m_log=&Poco::Logger::get(m_name);
    m_logStream = new Mantid::Kernel::ThreadSafeLogStream(*m_log);
  }

  /// Destructor
  Logger::~Logger()
  {
    delete (m_logStream);
  }

  /// Sets the Loggername to a new value.
  void Logger::setName(std::string newName)
  {
    //delete the log stream
    delete (m_logStream);
    //reassign m_log
    m_name = newName;
    m_log = &Poco::Logger::get(m_name);
    //create a new Logstream
    m_logStream = new Mantid::Kernel::ThreadSafeLogStream(*m_log);
  }

  /** Returns true if the log is enabled
   *
   *  @retval true - logging is enabled
   *  @retval false - all messages are ignored.
   */
  bool Logger::getEnabled() const
  {
    return m_enabled;
  }

  /** set if the logging is enabled
   *
   *  @param enabled ::  true - logging is enabled, false - all messages are ignored.
   */
  void Logger::setEnabled(const bool enabled)
  {
    m_enabled = enabled;
  }

  /** If the Logger's log level is at least PRIO_FATAL, creates a Message with
   *  priority PRIO_FATAL and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::fatal(const std::string& msg)
  {
    log(msg, PRIO_FATAL);
  }

  /** If the Logger's log level is at least PRIO_ERROR, creates a Message with priority
   *  PRIO_ERROR and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::error(const std::string& msg)
  {
    log(msg, PRIO_ERROR);
  }

  /** If the Logger's log level is at least PRIO_WARNING, creates a Message with
   *  priority PRIO_WARNING and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::warning(const std::string& msg)
  {
    log(msg, PRIO_WARNING);
  }

  /** If the Logger's log level is at least PRIO_NOTICE, creates a Message with
   *  priority PRIO_NOTICE and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::notice(const std::string& msg)
  {
    log(msg, PRIO_NOTICE);
  }

  /** If the Logger's log level is at least PRIO_INFORMATION, creates a Message with
   *  priority PRIO_INFORMATION and the given message text and sends it to the
   *  attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::information(const std::string& msg)
  {
    log(msg, PRIO_INFORMATION);
  }

  /** If the Logger's log level is at least PRIO_DEBUG, creates a Message with priority
   *  PRIO_DEBUG and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::debug(const std::string& msg)
  {
    log(msg, PRIO_DEBUG);
  }

  /** Logs the given message at debug level, followed by the data in buffer.
   *
   *  The data in buffer is written in canonical hex+ASCII form:
   *  Offset (4 bytes) in hexadecimal, followed by sixteen space-separated,
   *  two column, hexadecimal bytes, followed by the same sixteen bytes as
   *  ASCII characters.
   *  For bytes outside the range 32 .. 127, a dot is printed.
   *  Note all Dump messages go out at Debug message level
   *
   *  @param msg :: The message to log
   *  @param buffer :: the binary data to log
   *  @param length :: The length of the binaary data to log
   */
  void Logger::dump(const std::string& msg, const void* buffer, std::size_t length)
  {
    if(m_enabled)
    {
      try
      {
        m_log->dump(msg,buffer,length);
      }
      catch (std::exception& e)
      {
        //failures in logging are not allowed to throw exceptions out of the logging class
        std::cerr << e.what();
      }
    }
  }

  /** Returns true if at least the given log level is set.
   *  @param level :: The logging level it is best to use the Logger::Priority enum (7=debug, 6=information, 4=warning, 3=error, 2=critical, 1=fatal)
   *  @return true if at least the given log level is set.
   */
  bool Logger::is(int level) const
  {
    bool retVal = false;
    try
    {
      retVal = m_log->is(level);
    }
    catch (std::exception& e)
    {
      //failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
    return retVal;
  }

  void Logger::setLevel(int level)
  {
    try
    {
      m_log->setLevel(level);
    }
    catch (std::exception& e)
    {
      //failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
  }

  /// Sets the Logger's log level using a symbolic value.
  ///
  /// @param level :: Valid values are: fatal, critical, error, warning, notice, information, debug
  void Logger::setLevel(const std::string& level)
  {
    try
    {
      m_log->setLevel(level);
    }
    catch (std::exception& e)
    {
      //failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
  }

  int Logger::getLevel() const
  {
    return m_log->getLevel();
  }

  /** This class implements an ostream interface to the Logger for fatal messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::fatal()
  {
    if (m_enabled)
    {
      return m_logStream->fatal();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** This class implements an ostream interface to the Logger for error messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::error()
  {
    if (m_enabled)
    {
      return m_logStream->error();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** This class implements an ostream interface to the Logger for warning messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::warning()
  {
    if (m_enabled)
    {
      return m_logStream->warning();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** This class implements an ostream interface to the Logger for notice messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::notice()
  {
    if (m_enabled)
    {
      return m_logStream->notice();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** This class implements an ostream interface to the Logger for information messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::information()
  {
    if (m_enabled)
    {
      return m_logStream->information();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** This class implements an ostream interface to the Logger for debug messages.
   *
   * The stream's buffer appends all characters written to it
   * to a string. As soon as a CR or LF (std::endl) is written,
   * the string is sent to the Logger.
   * @returns an std::ostream reference.
   */
  std::ostream& Logger::debug()
  {
    if (m_enabled)
    {
      return m_logStream->debug();
    }
    else
    {
      return *m_nullStream;
    }
  }

  /** releases resources and deletes this object.
   */
  void Logger::release()
  {
    destroy(*this);
  }

  /** Deletes the logger and clears it from the cache.
   *
   *  @param logger :: The logger to destroy.
   */
  void Logger::destroy(Logger& logger)
  {
    if (m_loggerList)
    {
      try
      { mutexLoggerList->lock(); }
      catch(Poco::SystemException &)
      {}

      LoggerList::iterator it = m_loggerList->find(&logger);
      if (it != m_loggerList->end())
      {
        delete(*it);
        m_loggerList->erase(it);
      }

      try
      { mutexLoggerList->unlock(); }
      catch(Poco::SystemException &)
      {}
    }
  }

  /** Shuts down the logging framework and releases all Loggers.
   * Static method.
   */
  void Logger::shutdown()
  {
    try
    {
      //first release the POCO loggers
      Poco::Logger::shutdown();

      //now delete our static cache of loggers
      if (m_loggerList)
      {
        for (LoggerList::iterator it = m_loggerList->begin(); it != m_loggerList->end(); ++it)
        {
          delete(*it);
        }
        delete m_loggerList;
        m_loggerList = 0;
      }

      //delete the NullChannel
      if (m_nullStream)
      {
        delete(m_nullStream);
        m_nullStream=0;
      }
      // Finally delete the mutex
      delete mutexLoggerList;
    }
    catch (std::exception& e)
    {
      //failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
  }

  /** Returns a reference to the Logger with the given name.
   *  This logger is stored until in a static list until it is destroyed, released or Logger::shutdown is called.
   *
   *  @param name :: The name of the logger to use - this is usually the class name.
   *  @return a reference to the Logger with the given name.
   */
  Logger& Logger::get(const std::string& name)
  {
    Logger* pLogger = new Logger(name);
    // MG: This method can be called to initialize static logger which means
    // it may get called before mutexLoggerList has been initialized, i.e. the
    // usual static initialization order problem.
    if( mutexLoggerList == NULL ) mutexLoggerList = new Mutex();
    try
    { mutexLoggerList->lock(); }
    catch(Poco::SystemException &)
    {}

    //assert the nullSteam
    if(!m_nullStream)
    {
      m_nullStream = new Poco::NullOutputStream;
    }

    //assert the loggerlist
    if (!m_loggerList)
    {
      m_loggerList = new LoggerList;
    }
    //insert the newly created logger
    m_loggerList->insert(pLogger);

    try
    { mutexLoggerList->unlock(); }
    catch(Poco::SystemException &)
    {}

    return *pLogger;
  }

  /**
   * Log a given message at a given priority
   * @param message :: The message to log
   * @param priority :: The priority level
   */
  void Logger::log(const std::string message, Logger::Priority priority)
  {
    if( !m_enabled ) return;

    try
    {
      switch( priority )
      {
      case PRIO_FATAL: m_log->fatal(message);
      break;
      case PRIO_ERROR: m_log->error(message);
      break;
      case PRIO_WARNING: m_log->warning(message);
      break;
      case PRIO_NOTICE: m_log->notice(message);
      break;
      case PRIO_INFORMATION: m_log->information(message);
      break;
      case PRIO_DEBUG: m_log->debug(message);
      break;
      default:
        break;
      }
    }
    catch(std::exception& e)
    {
      // Failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
  }
  

} // namespace Kernel
} // Namespace Mantid

