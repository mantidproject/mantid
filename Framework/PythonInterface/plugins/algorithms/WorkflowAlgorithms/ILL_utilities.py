# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid import mtd
from mantid.simpleapi import DeleteWorkspace


class Cleanup:
    """A class to manage intermediate workspace cleanup."""

    OFF = 'Cleanup OFF'
    ON = 'Cleanup ON'

    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        """Initialize an instance of the class."""
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == self.ON
        self._protected = set()
        self._toBeDeleted = set()

    def cleanup(self, *args):
        """Delete the workspaces listed in *args."""
        for ws in args:
            self._delete(ws)

    def cleanupLater(self, *args):
        """Mark the workspaces listed in *args to be cleaned up later."""
        for arg in args:
            self._toBeDeleted.add(str(arg))

    def finalCleanup(self):
        """Delete all workspaces marked to be cleaned up later."""
        for ws in self._toBeDeleted:
            self._delete(ws)

    def protect(self, *args):
        """Mark the workspaces listed in *args to be never deleted."""
        for arg in args:
            self._protected.add(str(arg))

    def _delete(self, ws):
        """Delete the given workspace in ws if it is not protected, and
        deletion is actually turned on.
        """
        if not self._doDelete:
            return
        try:
            ws = str(ws)
        except RuntimeError:
            return
        if ws not in self._protected and mtd.doesExist(ws):
            DeleteWorkspace(Workspace=ws, EnableLogging=self._deleteAlgorithmLogging)


class NameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = '__' + prefix if cleanupMode == Cleanup.ON else prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'


class Report:
    """A logger for final report generation."""

    _LEVEL_NOTICE = 0
    _LEVEL_WARNING = 1
    _LEVEL_ERROR = 2

    class _Entry:
        """A private class for report entries."""

        def __init__(self, text, loggingLevel):
            """Initialize an entry."""
            self._text = text
            self._loggingLevel = loggingLevel

        def contents(self):
            """Return the contents of this entry."""
            return self._text

        def level(self):
            """Return the logging level of this entry."""
            return self._loggingLevel

    def __init__(self):
        """Initialize a report."""
        self._entries = list()

    def error(self, text):
        """Add an error entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_ERROR))

    def notice(self, text):
        """Add an information entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_NOTICE))

    def warning(self, text):
        """Add a warning entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_WARNING))

    def toLog(self, log):
        """Write this report to a log."""
        for entry in self._entries:
            loggingLevel = entry.level()
            if loggingLevel == Report._LEVEL_NOTICE:
                log.notice(entry.contents())
            elif loggingLevel == Report._LEVEL_WARNING:
                log.warning(entry.contents())
            elif loggingLevel == Report._LEVEL_ERROR:
                log.error(entry.contents())
