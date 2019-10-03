# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import inspect
import sys
from six import PY2
if PY2:  # noqa
    from inspect import getargspec as getfullargspec
else:  # noqa
    from inspect import getfullargspec

import jedi

from mantidqt.widgets.codeeditor.editor import CodeEditor


class CodeCompleter:

    def __init__(self, editor, env_globals=None, enable_jedi=True):
        self.editor = editor
        self.env_globals = env_globals
        self._enable_jedi = enable_jedi
        self.editor.enableAutoCompletion(CodeEditor.AcsAll)

        self._all_completions = dict()
        if "from mantid.simpleapi import *" in self.editor.text():
            self._add_to_completions(self.get_mantid_simple_api_call_tips())

        if enable_jedi:
            jedi.api.preload_module('numpy', 'matplotlib.pyplot')
            self._add_to_completions(self._generate_jedi_completions_list())
            self.editor.cursorPositionChanged.connect(self._on_cursor_position_changed)

        self.editor.updateCompletionAPI(list(self._all_completions.keys()))

    def _add_to_completions(self, completions):
        for completion in completions:
            self._all_completions[completion] = True

    def update_completion_api(self):
        if self._enable_jedi:
            self._add_to_completions(self._generate_jedi_completions_list())
        self.editor.updateCompletionAPI(list(self._all_completions.keys()))

    def get_mantid_simple_api_call_tips(self):
        simple_api_module = sys.modules['mantid.simpleapi']
        return self.generate_call_tips(simple_api_module.__dict__)

    @staticmethod
    def generate_call_tips(env_globals):
        if not env_globals:
            return []
        call_tips = []
        for name, attr, in env_globals.items():
            if name == name.title() and not name.startswith('_'):
                if inspect.isfunction(attr) or inspect.isbuiltin(attr):
                    call_tips.append(name + CodeCompleter.get_function_spec(attr))
        return call_tips

    @staticmethod
    def get_function_spec(func):
        """
        Get the python function signature for the given function object. First
        the args are inspected followed by varargs, which are set by some modules,
        e.g. mantid.simpleapi algorithm functions

        :param func: A Python function object
        :returns: A string containing the function specification
        """
        try:
            argspec = getfullargspec(func)
        except TypeError:
            return ''
        # mantid algorithm functions have varargs set not args
        args = argspec[0]
        if args:
            # For methods strip the self argument
            if hasattr(func, 'im_func'):
                args = args[1:]
            defs = argspec[3]
        elif argspec[1] is not None:
            # Get from varargs/keywords
            arg_str = argspec[1].strip().lstrip('\b').replace(',', ', ')
            defs = []
            # Keyword args
            kwargs = argspec[2]
            if kwargs is not None:
                kwargs = kwargs.strip().lstrip('\b\b')
                if kwargs == 'kwargs':
                    kwargs = '**' + kwargs + '=None'
                arg_str += ', %s' % kwargs
            # Any default argument appears in the string
            # on the rhs of an equal
            for arg in arg_str.split(', '):
                arg = arg.strip()
                if '=' in arg:
                    arg_token = arg.split('=')
                    args.append(arg_token[0])
                    defs.append(arg_token[1])
                else:
                    args.append(arg)
            if len(defs) == 0:
                defs = None
        else:
            return ''

        if defs is None:
            call_tip = ', '.join(args)
            call_tip = '(' + call_tip + ')'
        else:
            # The defaults list contains the default values for the last n arguments
            diff = len(args) - len(defs)
            call_tip = ''
            for index in range(len(args) - 1, -1, -1):
                def_index = index - diff
                if def_index >= 0:
                    call_tip = '[' + args[index] + '], ' + call_tip
                else:
                    call_tip = args[index] + ", " + call_tip
            call_tip = '(' + call_tip.rstrip(', ') + ')'
        return call_tip

    # jedi completions generation
    def _generate_jedi_interpreter(self, line_no=None, col_no=None):
        return jedi.Script(self.editor.text(), line=line_no, column=col_no,
                           path=self.editor.fileName(), sys_path=sys.path)

    def _generate_jedi_completions_list(self, line_no=None, col_no=None):
        completions = self._generate_jedi_interpreter(line_no, col_no).completions()
        completions_list = []
        for completion in completions:
            if self._all_completions.get(completion.name, None) is None:
                self._all_completions[completion.name] = True
                completions_list.append(completion.name)
        return completions_list

    def _generate_jedi_call_tips(self, line_no=None, col_no=None):
        call_sigs = self._generate_jedi_interpreter(line_no, col_no).call_signatures()
        call_tips = []
        for signature in call_sigs:
            args = []
            for param in signature.params:
                arg = param.description.replace('param ', '').replace('\n', ' ')
                if arg:
                    args.append(arg)

            if not args:
                continue

            call_tip = "{}({})".format(signature.name, ', '.join(args))
            if self._all_completions.get(call_tip, None) is None:
                self._all_completions[call_tip] = True
                call_tips.append(call_tip)

        return call_tips

    def _on_cursor_position_changed(self, line_no, col_no):
        """
        Slot to be executed when editor's cursor position has been changed.
        Note that Qt's line numbering starts at 0, whilst jedi's starts at 1.
        :param int line_no: The cursor's new line number
        :param int col_no: The cursor's new column number
        """
        line = self.editor.text().split('\n')[line_no]
        if col_no < 2:
            return

        completions = []
        if line.strip() and not (line.rstrip()[-1] == '(' and col_no >= line.rfind('(')):
            completions = self._generate_jedi_completions_list(line_no + 1, col_no)

        # Since call tips will be generated the first time an open bracket is
        # entered, we need not worry about counting the number of open/closed
        # brackets
        if line.rfind(')') < line.rfind('('):
            call_tips = self._generate_jedi_call_tips(line_no + 1, col_no)
            completions.extend(call_tips)
        if completions:
            self.editor.addToCompletionAPI(completions)
