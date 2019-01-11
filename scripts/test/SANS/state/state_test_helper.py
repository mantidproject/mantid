# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


def assert_validate_error(caller, error_type, obj):
    try:
        obj.validate()
        raised_correct = False
    except error_type:
        raised_correct = True
    except:  # noqa
        raised_correct = False
    caller.assertTrue(raised_correct)


def assert_raises_nothing(caller, obj):
    try:
        obj.validate()
    except:  # noqa
        caller.fail()
