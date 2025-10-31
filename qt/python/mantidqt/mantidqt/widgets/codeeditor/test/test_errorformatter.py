# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

#
#  This file is part of the mantidqt package
# std imports
import sys
import traceback
import unittest

# third-party imports

# local imports
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter


class ErrorFormatterTest(unittest.TestCase):
    def test_syntax_error(self):
        try:
            exec("if:")  # noqa: S102
        except SyntaxError:
            exc_type, exc_value = sys.exc_info()[:2]
            formatter = ErrorFormatter()
            error = formatter.format(exc_type, exc_value, None)

        expected = """  File "<string>", line 1
    if:
      ^
SyntaxError: invalid syntax
"""
        self.assertEqual(expected, error)

    def test_standard_exception(self):
        code = """
def foo():
    def bar():
        # raises a NameError
        y = _local + 1
    # call inner
    bar()
foo()
"""
        try:
            exec(code)  # noqa: S102
        except NameError:
            exc_type, exc_value, tb = sys.exc_info()
            formatter = ErrorFormatter()
            error = formatter.format(exc_type, exc_value, traceback.extract_tb(tb))
            del tb

        # stacktrace will contain file names that are not portable so don't do equality check
        error_lines = error.splitlines()
        # python 3 has a slightly different format of the exception error so just check it looks
        # approximate correct
        expected_lines = [
            "NameError:.*'_local'.*",
            r'  File ".*test_errorformatter.py", line \d+, in test_standard_exception',
            "    exec(.*).*",
            r"    \^+",
            r'  File "<string>", line \d+, in <module>',
            r'  File "<string>", line \d+, in foo',
            r'  File "<string>", line \d+, in bar',
        ]
        for produced, expected in zip(error_lines, expected_lines, strict=True):
            self.assertRegex(produced, expected)


if __name__ == "__main__":
    unittest.main()
