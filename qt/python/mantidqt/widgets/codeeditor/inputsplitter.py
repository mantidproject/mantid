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

# 3rd party imports
from IPython.core.inputsplitter import InputSplitter as IPyInputSplitter

# local imports


class InputSplitter(IPyInputSplitter):
    r"""A specialized version of IPython's input splitter.

    The major differences between this and the base version are:

      - push considers a SyntaxError as incomplete code
      - push_accepts_more returns False when the indentation has return flush
        regardless of whether the statement is a single line
    """

    def push(self, lines):
        """Push one or more lines of input.

        This stores the given lines and returns a status code indicating
        whether the code forms a complete Python block or not.

        Any exceptions generated in compilation are swallowed, but if an
        exception was produced, the method returns True.

        Parameters
        ----------
        lines : string
          One or more lines of Python input.

        Returns
        -------
        is_complete : boolean
          True if the current input source (the result of the current input
          plus prior inputs) forms a complete Python execution block.  Note that
          this value is also stored as a private attribute (``_is_complete``), so it
          can be queried at any time.
        """
        self._store(lines)
        source = self.source

        # Before calling _compile(), reset the code object to None so that if an
        # exception is raised in compilation, we don't mislead by having
        # inconsistent code/source attributes.
        self.code, self._is_complete = None, None

        # Honor termination lines properly
        if source.endswith('\\\n'):
            return False

        try:
            self._update_indent(lines)
        except TypeError: # _update_indent was changed in IPython 6.0
            self._update_indent()
        except AttributeError: # changed definition in IPython 6.3
            self.get_indent_spaces()
        try:
            self.code = self._compile(source, symbol="exec")
        # Invalid syntax can produce any of a number of different errors from
        # inside the compiler, so we have to catch them all.  Syntax errors
        # immediately produce a 'ready' block, so the invalid Python can be
        # sent to the kernel for evaluation with possible ipython
        # special-syntax conversion.
        except (SyntaxError, OverflowError, ValueError, TypeError,
                MemoryError):
            self._is_complete = False
        else:
            # Compilation didn't produce any exceptions (though it may not have
            # given a complete code object)
            self._is_complete = self.code is not None

        return self._is_complete

    def push_accepts_more(self):
        """Return whether a block of input can accept more input.

        This method is meant to be used by line-oriented frontends, who need to
        guess whether a block is complete or not based solely on prior and
        current input lines.  The InputSplitter considers it has a complete
        block and will not accept more input when either:

        * A SyntaxError is raised

        * The code is complete and consists of a single line or a single
          non-compound statement

        * The code is complete and has a blank line at the end
        """

        # With incomplete input, unconditionally accept more
        # A syntax error also sets _is_complete to True - see push()
        if not self._is_complete:
            return True

        # If there's just a single line or AST node, and we're flush left, as is
        # the case after a simple statement such as 'a=1', we want to execute it
        # straight away.
        if self.indent_spaces == 0:
            return False

        # General fallback - accept more code
        return True
