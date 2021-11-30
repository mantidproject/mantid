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
        backup = dict(channel=config['logging.channels.consoleChannel.class'], stdout=sys.stdout)
        # backup the logging level?
        if level:
            assert level.lower() in ['debug', 'information', 'notice', 'warning', 'error']
            current_level = config['logging.loggers.root.level']
            backup['level'] = current_level if current_level else 'notice'
            config['logging.loggers.root.level'] = level
        # redirect messages to log_file
        config['logging.channels.consoleChannel.class'] = 'PythonStdoutChannel'
        str_buffer = io.StringIO()
        sys.stdout = str_buffer
        yield str_buffer
    finally:
        str_buffer.close()
        config['logging.channels.consoleChannel.class'] = backup['channel']
        if level:
            config['logging.loggers.root.level'] = backup['level']
        sys.stdout = backup['stdout']
