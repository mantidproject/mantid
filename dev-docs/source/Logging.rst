.. _Logging:

=======
Logging
=======

.. contents::
  :local:

Overview
--------

The Mantid logging system is a useful tool for garnering more information about the operation of the program.
It is used to output messages from the framework to a channel, for example a file or the screen.
Including calls to the logging system in your code can greatly aid in assessing its running - successful or otherwise.
Logging is provided by the `Mantid::Kernel::Logger <http://doxygen.mantidproject.org/nightly/d2/d78/classMantid_1_1Kernel_1_1Logger.html>`_ class, which is a thin wrapper around the `Poco::Logger <https://pocoproject.org/docs/Poco.Logger.html>`_.

Logging Levels
--------------

There are 7 log levels supported with increasing priority and decreasing verboseness:

+---------------+-----------------------------------------------------------------------------------------------+
| Level         | Description on what to log                                                                    |
+===============+===============================================================================================+
| DEBUG         | Anything that may be useful to understand what the code has been doing for debugging purposes.|
|               | E.g. parameter values, milestones, internal variable values.                                  |
|               | Log at this level often throughout your code.                                                 |
+---------------+-----------------------------------------------------------------------------------------------+
| INFORMATION   | Useful information to relay back to the user of the framework.                                |
+---------------+-----------------------------------------------------------------------------------------------+
| NOTICE        | Really important information that should be displayed to the user, this should be minimal.    |
|               | Algorithms log at this level when starting/finishing. This is the default logging level.      |
+---------------+-----------------------------------------------------------------------------------------------+
| WARNING       | Something was wrong but the framework was able to continue despite the problem.               |
+---------------+-----------------------------------------------------------------------------------------------+
| ERROR         | An error has occurred but the framework is able to handle it and continue.                    |
+---------------+-----------------------------------------------------------------------------------------------+
| CRITICAL      | An important error has occurred, the framework can continue but it is advisable to restart.   |
+---------------+-----------------------------------------------------------------------------------------------+
| FATAL         | An unrecoverable error has occurred and the application will terminate.                       |
+---------------+-----------------------------------------------------------------------------------------------+

Configuring the Log Level
-------------------------

For the logging to work you will need to have configured the logging service. This will occur when you do either of the following:

- Call :code:`FrameworkManager.initialise()`
- Get a reference to the :code:`ConfigService` singleton

When the framework is initialised, it attempts to read a file called :code:`Mantid.properties` that it assumes will be available in the current working directory.
This contains among other things the logging configuration. See the :ref:`properties file <mantid:Properties File>` overview for more information.

Here is an example:

.. code-block:: text

  logging.loggers.root.level = debug
  logging.loggers.root.channel.class = SplitterChannel
  logging.loggers.root.channel.channel1 = consoleChannel
  logging.loggers.root.channel.channel2 = fileChannel
  logging.channels.consoleChannel.class = ConsoleChannel
  logging.channels.consoleChannel.formatter = f1
  logging.channels.fileChannel.class = FileChannel
  logging.channels.fileChannel.path = mantid.log
  logging.channels.fileChannel.formatter.class = PatternFormatter
  logging.channels.fileChannel.formatter.pattern = %Y-%m-%d %H:%M:%S,%i [%I] %p %s - %t
  logging.formatters.f1.class = PatternFormatter
  logging.formatters.f1.pattern = %s-[%p] %t
  logging.formatters.f1.times = UTC

This specifies that the logging comments will go to the console as well as a file called :code:`mantid.log`.
In the example here the level is set to debug, so all the messages will be output.
In production this will usually be set to information.
One could also alter the logging level programmatically using :code:`ConfigService`.
For example, in :code:`python`:

.. code-block:: python

  cfg = ConfigService.Instance()
  cfg.setConsoleLogLevel(5) # notice

Note, that this affects the current session only; to change the level permanently, one needs to save the configuration to the file.

Usage Example in C++
--------------------

In the .h:

.. code-block:: cpp

  #include "Logger.h"

  class A
  {
  private:

    /// Static reference to the logger class
    static Logger& g_log;
  }

In the .cpp:

.. code-block:: cpp

  A::func()
  {
    g_log.error("Log message");
    g_log.information() << "Flightpath found to be " << distance << " metres.\n";
  }

Usage Example in Python
-----------------------

Inside the algorithms:

.. code-block:: python

  self.log().information('Number of scan steps is something')

In general:

.. code-block:: python

  from mantid.kernel import logger
  logger.warning('this is a custom warning')

Tips
----

- If logging data that takes significant resources to generate the message, use the :code:`is(priority)` function of the logger to check if the message would actually be output:

  .. code-block:: cpp

    if (g_log.is(Logger::PRIO_DEBUG))
    {
      // generate message and output to log.
    }

- If you need to dump binary data, use the dump method of the logger. Note that all dump messages are sent at debug level:

  .. code-block:: cpp

    /// Logs the given message at debug level, followed by the data in buffer.
    void dump(const std::string& msg, const void* buffer, std::size_t length);

- Note, that logging can slow down code significantly, so avoid overusing it, especially in large and nested loops.
- In workflow algorithms consider setting an offset to the child algorithm log levels, or disable them completely, otherwise the log output can be too verbose with the low priority levels, such as debug or information.
- Note, that the *Results Log* widget in MantidPlot offers only five options to show the logs (from debug to error).
- Note, that too verbose logs when shown in the *Results Log* can slow down and even freeze the MantidPlot GUI for some time. So choose wisely what log level to show.
