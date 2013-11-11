#include "MantidKernel/Logger.h"
#include "MantidKernel/ThreadSafeLogStream.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>

#ifdef _MSC_VER
  // Disable a flood of warnings about inheriting from std streams
  // See http://connect.microsoft.com/VisualStudio/feedback/details/733720/inheriting-from-std-fstream-produces-c4250-warning
  #pragma warning( push )
  #pragma warning( disable : 4250 )
#endif

#include <Poco/NullStream.h>
#ifdef _MSC_VER
  #pragma warning( pop )
#endif

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
  Logger::Logger(const std::string& name) : m_name(name), m_levelOffset(0), m_enabled(true) 
  {
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

  /** If the Logger's log level is at least Poco::Message::PRIO_FATAL, creates a Message with
   *  priority Poco::Message::PRIO_FATAL and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::fatal(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_FATAL);
  }

  /** If the Logger's log level is at least Poco::Message::PRIO_ERROR, creates a Message with priority
   *  Poco::Message::PRIO_ERROR and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::error(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_ERROR);
  }

  /** If the Logger's log level is at least Poco::Message::PRIO_WARNING, creates a Message with
   *  priority Poco::Message::PRIO_WARNING and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::warning(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_WARNING);
  }

  /** If the Logger's log level is at least Poco::Message::PRIO_NOTICE, creates a Message with
   *  priority Poco::Message::PRIO_NOTICE and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::notice(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_NOTICE);
  }

  /** If the Logger's log level is at least Poco::Message::PRIO_INFORMATION, creates a Message with
   *  priority Poco::Message::PRIO_INFORMATION and the given message text and sends it to the
   *  attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::information(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_INFORMATION);
  }

  /** If the Logger's log level is at least Poco::Message::PRIO_DEBUG, creates a Message with priority
   *  Poco::Message::PRIO_DEBUG and the given message text and sends it to the attached channel.
   *
   *  @param msg :: The message to log.
   */
  void Logger::debug(const std::string& msg)
  {
    log(msg, Poco::Message::PRIO_DEBUG);
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
    return getLogStream(Priority::PRIO_FATAL);
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
    return getLogStream(Priority::PRIO_ERROR);
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
    return getLogStream(Priority::PRIO_WARNING);
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
    return getLogStream(Priority::PRIO_NOTICE);
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
    return getLogStream(Priority::PRIO_INFORMATION);
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
     return getLogStream(Priority::PRIO_DEBUG);
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
      delete(m_nullStream);
      m_nullStream=0;

      // Finally delete the mutex
      delete mutexLoggerList;
      mutexLoggerList = 0;
    }
    catch (std::exception& e)
    {
      //failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << e.what();
    }
  }

  /** Sets the log level for all Loggers created so far, including the root logger.
   * @param level :: the priority level to set for the loggers
   */
  void Logger::setLevelForAll(const int level)
  {
    // "" is the root logger
    Poco::Logger::setLevel("",level);
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
      switch( applyLevelOffset(priority) )
      {
      case Poco::Message::PRIO_FATAL: m_log->fatal(message);
      break;
      case Poco::Message::PRIO_CRITICAL: m_log->critical(message);
      break;
      case Poco::Message::PRIO_ERROR: m_log->error(message);
      break;
      case Poco::Message::PRIO_WARNING: m_log->warning(message);
      break;
      case Poco::Message::PRIO_NOTICE: m_log->notice(message);
      break;
      case Poco::Message::PRIO_INFORMATION: m_log->information(message);
      break;
      case Poco::Message::PRIO_DEBUG: m_log->debug(message);
      break;
      case Poco::Message::PRIO_TRACE: m_log->trace(message);
      break;
      default:
        break;
      }
    }
    catch(std::exception& e)
    {
      // Failures in logging are not allowed to throw exceptions out of the logging class
      std::cerr << "Error in logging framework: " << e.what();
    }
  }

   /**
   * Log a given message at a given priority
   * @param message :: The message to log
   * @param priority :: The priority level
   */
  std::ostream& Logger::getLogStream(Logger::Priority priority)
  {
    if( !m_enabled ) return *m_nullStream;

    switch( applyLevelOffset(priority) )
    {
    case Poco::Message::PRIO_FATAL: return m_logStream->fatal();
    break;
    case Poco::Message::PRIO_CRITICAL:  return m_logStream->critical();
    break;
    case Poco::Message::PRIO_ERROR: return m_logStream->error();
    break;
    case Poco::Message::PRIO_WARNING: return m_logStream->warning();
    break;
    case Poco::Message::PRIO_NOTICE: return m_logStream->notice();
    break;
    case Poco::Message::PRIO_INFORMATION:return m_logStream->information();
    break;
    case Poco::Message::PRIO_DEBUG:  return m_logStream->debug();
    break;
    default:
      return *m_nullStream;
    }

  }

  /**
   * Adjust a log priority level based off the m_levelOffset
   * @param proposedLevel :: The proposed level
   * @returns The offseted level
   */
  Logger::Priority Logger::applyLevelOffset(Logger::Priority proposedLevel) 
  {
    int retVal = proposedLevel;
    //fast exit is offset is 0
    if (m_levelOffset==0)
    {
      return proposedLevel;
    }
    else
    {
      retVal += m_levelOffset;
      if (retVal < static_cast<int>(Priority::PRIO_FATAL))
      {
        retVal = Priority::PRIO_FATAL;
      }
      else if (retVal > static_cast<int>(Priority::PRIO_TRACE))
      {
        retVal = Priority::PRIO_TRACE;
      }
    }
    //Logger::Priority p(retVal);
    return static_cast<Logger::Priority>(retVal);
  }
  
    
  /**
   * Sets the Logger's log offset level.
   * @param level :: The  level offset to use
   */
   void Logger::setLevelOffset(int level)
   {
     m_levelOffset = level;
   }

   /**
   * Gets the Logger's log offset level.
   * @returns The offset level
   */ /// Gets the Logger's log offset level.
   int Logger::getLevelOffset()
   {
     return m_levelOffset;
   }

} // namespace Kernel
} // Namespace Mantid

