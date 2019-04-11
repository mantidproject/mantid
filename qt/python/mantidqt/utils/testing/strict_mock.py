# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import unittest

from mantid.py3compat.mock import Mock, PropertyMock


class StrictMock(Mock):
    """
    This mock automatically sets "spec_set={}", which will cause errors when methods
    that have NOT been declared on the mock are called. This prevents spelling errors
    and captures any unexpected calls to the mock.
    """

    def __init__(self, *args, **kwargs):
        if "spec_set" not in kwargs:
            # specifies that ANY calls that are not explicitly declared on the mock
            # will throw an AttributeError, instead of return another Mock
            kwargs["spec_set"] = {}

        super(StrictMock, self).__init__(*args, **kwargs)


class StrictContextManagerMock(unittest.TestCase):
    """
    This mock adds the methods needed for a ContextManager.
    It inherits from TestCase for the assert methods, but does NOT call the constructor of the TestCase class!

    The TestCase assertions methods are not extracted into a base class,
    so there is no other way to gain access to them
    """

    def __init__(self):
        # does not call the super constructor class - this prevents an error for missing test cases, as
        # none are declared in the class
        self.context_entered = False
        self.context_exited = False

    def __call__(self, *args, **kwargs):
        return self

    def __enter__(self):
        self.context_entered = True
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.context_exited = True

    def assert_context_triggered(self):
        self.assertTrue(self.context_entered)
        self.assertTrue(self.context_exited)


class StrictPropertyMock(PropertyMock):
    """
    This mock automatically sets the spec of the PropertyMock to match the spec of the
    return_value object. Sharing the spec between the two objects forces the mock to verify
    that the attributes accessed on the return_value object actually exist, and will raise
    AttributeErrors if they don't, thus preventing unexpected calls passing silently.
    """

    def __init__(self, *args, **kwargs):
        if "return_value" in kwargs:
            kwargs["spec_set"] = kwargs["return_value"]

        super(StrictPropertyMock, self).__init__(*args, **kwargs)
