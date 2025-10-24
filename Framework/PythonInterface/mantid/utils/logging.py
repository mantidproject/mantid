# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# local packages
from mantid.kernel import ConfigService

# standard packages
from contextlib import contextmanager
import io
import sys


@contextmanager
def capture_logs(level=None) -> io.StringIO:
    r"""
    A context manager that redirects logging messages to string buffer
    and returns a StringIO buffer.

    Usage:
        with capture_logs(level='debug') as logs:
            some code that outputs messages with mantid.kernel.logger
            assert 'some query message' in logs.getvalue()

    Can be used to assert if an algorithm emitted a particular log message by one of its
    methods not exposed to the python API. This is an indirect way of testing those private
    methods.

    @param str level: set a particular logging level within the scope of the context manager. If
        ``None``, the level's unchanged. One of 'debug', 'information', 'notice',
        'warning', 'error'. If ``level`` is specified, the pre-existing level in
         configuration option 'logging.loggers.root.level' is reinstated upon leaving the
         scope of the context manager. If this option was unset, the 'notice' level is set.

    @return io.StringIO: A reference to the temporary StringIO buffer object
    """
    try:
        # backup the logging channel and sys.stdout
        config = ConfigService.Instance()
        backup = dict(channel=config["logging.channels.consoleChannel.class"], stdout=sys.stdout)
        # backup the logging level?
        if level:
            if level.lower() not in ["debug", "information", "notice", "warning", "error"]:
                raise ValueError("Unknown logging level")
            current_level = config["logging.loggers.root.level"]
            backup["level"] = current_level if current_level else "notice"
            config["logging.loggers.root.level"] = level
        # redirect messages to log_file
        config["logging.channels.consoleChannel.class"] = "PythonStdoutChannel"
        str_buffer = io.StringIO()
        sys.stdout = str_buffer
        yield str_buffer
    finally:
        str_buffer.close()
        config["logging.channels.consoleChannel.class"] = backup["channel"]
        if level:
            config["logging.loggers.root.level"] = backup["level"]
        sys.stdout = backup["stdout"]


def log_to_python(level=None, pattern=None) -> None:
    r"""
    Modify Mantid's logger to forward messages to Python's logging framework instead
    of outputting them itself. This allows users to configure the logger from Python
    and merge logs from different sources.

    Note that this function does not configure Python's logging system.

    @param str level: Logging level for the *Mantid* logger. Should be set to a low value
        to forward all potentially relevant messages to Python and let *Python's* logger
        filter out undesired messages. Possible values: 'trace', 'debug',
        'information', 'notice', 'warning', 'error', 'critical', 'fatal'.
    @param str pattern: A logging pattern to format messages before forwarding them (example: '[%s] %t')
        See https://github.com/pocoproject/poco/wiki/Poco::Util::Application-Logging-Configuration#logging-format-placeholders
    """
    config = ConfigService.Instance()
    if level is not None:
        config["logging.loggers.root.level"] = level
    if pattern is not None:
        config["logging.channels.consoleChannel.formatter"] = "f1"
        config["logging.formatters.f1.class"] = "PatternFormatter"
        config["logging.formatters.f1.pattern"] = pattern
    # Important: Do this one last because it triggers re-init of logging system!
    config["logging.channels.consoleChannel.class"] = "PythonLoggingChannel"
