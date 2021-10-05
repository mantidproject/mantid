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
import os
import sys
import tempfile


@contextmanager
def to_file(level=None, filename=None):
    r"""
    A context manager that redirects logging messages to a possibly temporary file.  If
    temporary, the file is erased upon leaving the scope of the context manager.

    Usage:
        with to_file(level='debug') as log_file:
            some code that outputs messages with mantid.kernel.logger
            assert 'some query message' in open(log_file,'r')

    Can be used to assert if an algorithm emitted a particular log message by one of its
    methods not exposed to the python API. This is an indirect way of testing those private
    methods.

    @param str level: set a particular logging level within the scope of the context manager. If
        ``None``, the level's unchanged. One of 'debug', 'information', 'notice',
        'warning', 'error'. If ``level`` is specified, the pre-existing level in
         configuration option 'logging.loggers.root.level' is reinstated upon leaving the
         scope of the context manager. If this option was unset, the 'notice' level is set.
    @param str filename: name of the output log file. If ``None``, a temporary file is created that
        will be erased upon leaving the scope of the context manager.

    @return str: name of the output log file. If ``filename`` is passed, just return this value,
        otherwise return the name of a temporary file.
    """
    try:
        # backup the logging channel and sys.stdout
        config = ConfigService.Instance()
        backup = dict(channel=config['logging.channels.consoleChannel.class'],
                      stdout=sys.stdout)
        # backup the logging level?
        if level:
            assert level.lower() in ['debug', 'information', 'notice', 'warning', 'error']
            current_level = config['logging.loggers.root.level']
            backup['level'] = current_level if current_level else 'notice'
            config['logging.loggers.root.level'] = level
        # create temporary file, if necessary
        if filename is None:
            _, log_file = tempfile.mkstemp(suffix='.log')
        else:
            log_file = filename
        # redirect messages to log_file
        config['logging.channels.consoleChannel.class'] = 'PythonStdoutChannel'
        sys.stdout = open(log_file, 'w', buffering=1)
        yield log_file
    finally:
        config['logging.channels.consoleChannel.class'] = backup['channel']
        if level:
            config['logging.loggers.root.level'] = backup['level']
        if filename is None:
            os.remove(log_file)  # delete the temporary log file
        sys.stdout = backup['stdout']
