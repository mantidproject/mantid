# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

from qtpy.QtWidgets import QPushButton


class QtAssertionsHelper(object):
    """
    Provides common assertions for Qt specific cases, such as:
    - Checking the number of connections a QObject has on a signal
    - Finding the object by objectName and asserting the connections
    """

    def assert_connected(self, owner, signal, times):
        self.assertEqual(times, owner.receivers(signal))

    def assert_connected_once(self, owner, signal):
        return self.assert_connected(owner, signal, 1)

    def assert_object_connected_once(self, parent, object_name, object_type=QPushButton, object_signal="clicked"):
        object = parent.findChild(object_type, object_name)
        self.assert_connected_once(object, getattr(object, object_signal))

    def assert_object_not_connected(self, parent, object_name, object_type=QPushButton, object_signal="clicked"):
        object = parent.findChild(object_type, object_name)
        self.assert_connected(object, getattr(object, object_signal), times=0)
