# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

from mantid.py3compat.mock import Mock, PropertyMock


class StrictMock(Mock):
    """
    This mock automatically sets "spec_set={}", which will cause errors when methods
    that have NOT been declared on the mock are called. This prevents spelling errors
    and captures any unexpected calls to the mock.

    To declare an expected call, the attribute
    """

    def __init__(self, *args, **kwargs):
        if "spec_set" not in kwargs:
            # specifies that ANY calls that are not explicitly declared on the mock
            # will throw an AttributeError, instead of return another Mock
            kwargs["spec_set"] = {}

        super(StrictMock, self).__init__(*args, **kwargs)


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
