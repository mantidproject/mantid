# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import DeleteWorkspace, mtd


class WSCleanup:
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


class WSNameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = '__' + prefix if cleanupMode == WSCleanup.ON else prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'
