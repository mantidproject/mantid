#  This file is part of the mantidqt package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# std imports
import sys
import traceback
import unittest

# local imports
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter


class ErrorFormatterTest(unittest.TestCase):

    def test_syntax_error(self):
        try:
            exec("if:")
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
            exec(code)
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
            '  File ".*test_errorformatter.py", line 56, in test_standard_exception',
            '    exec(.*)',
            '  File "<string>", line 8, in <module>',
            '  File "<string>", line 7, in foo',
            '  File "<string>", line 5, in bar',
        ]
        for produced, expected in zip(error_lines, expected_lines):
            self.assertRegexpMatches(produced, expected)


if __name__ == "__main__":
    unittest.main()
