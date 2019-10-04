# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import inspect
import re
import sys
from six import PY2
if PY2:  # noqa
    from inspect import getargspec as getfullargspec
else:  # noqa
    from inspect import getfullargspec

import jedi

from mantidqt.widgets.codeeditor.editor import CodeEditor


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


def generate_call_tips(definitions):
    """
    Generate call tips for a dictionary of object definitions (eg. globals()).
    The call tips generated are of the form:
        Load(InputWorkspace, [OutputWorkspace], ...)
    where squared braces denote key-word arguments.

    :param dict definitions: Dictionary with names of python objects as keys and
        the objects themselves as values
    :returns list: A list of call tips
    """
    if not isinstance(definitions, dict):
        return []
    call_tips = []
    for name, py_object in definitions.items():
        if name.startswith('_'):
            continue
        if inspect.isfunction(py_object) or inspect.isbuiltin(py_object):
            call_tips.append(name + get_function_spec(py_object))
            continue
        for attr in dir(py_object):
            try:
                f_attr = getattr(py_object, attr)
            except Exception:
                continue
            if attr.startswith('_'):
                continue
            if hasattr(f_attr, 'im_func') or inspect.isfunction(f_attr) or inspect.ismethod(f_attr):
                call_tips.append(name + '.' + attr + get_function_spec(f_attr))
            else:
                call_tips.append(name + '.' + attr)
    return call_tips


class CodeCompleter:
    """
    This class generates autocompletions for Workbench's script editor.
    It generates autocompletions from two sources:

    - environment globals, this picks up Mantid's algorithms and can give
      suggestions based on a variable's type. These completions are updated
      on every successful script execution.

    - jedi, this package performs static analysis on a given source
      script. This provides excellent autocompletion for packages
      e.g. numpy and matplotlib. These are updated every time the editor's
      cursor is moved and there is no need for the user to have run any code.

    Generating from jedi is turned OFF by default because it crashes when
    launched with Python 2 through PyCharm's debugger. There is no such
    problem in Python 3 however.
    """
    def __init__(self, editor, env_globals=None, enable_jedi=False):
        self.editor = editor
        self.env_globals = env_globals
        self._enable_jedi = enable_jedi
        if self._enable_jedi:
            self.editor.enableAutoCompletion(CodeEditor.AcsAPIs)
        else:
            self.editor.enableAutoCompletion(CodeEditor.AcsAll)

        self._interpreter_completions = dict()
        if "from mantid.simpleapi import *" in self.editor.text():
            self._add_to_interpreter_completions(self._get_mantid_simple_api_call_tips())

        if enable_jedi:
            jedi.api.preload_module('numpy', 'matplotlib.pyplot', 'np')
            self._add_to_interpreter_completions(self._generate_jedi_completions_list())
            self.editor.cursorPositionChanged.connect(self._on_cursor_position_changed)

        self.editor.updateCompletionAPI(list(self._interpreter_completions.keys()))

    def _get_completions_from_globals(self):
        return generate_call_tips(self.env_globals)

    def _add_to_interpreter_completions(self, completions):
        for completion in completions:
            self._interpreter_completions[completion] = True

    def _update_completion_api(self):
        self._add_to_interpreter_completions(self._get_completions_from_globals())
        self.editor.updateCompletionAPI(list(self._interpreter_completions.keys()))

    def _get_mantid_simple_api_call_tips(self):
        simple_api_module = sys.modules['mantid.simpleapi']
        return generate_call_tips(simple_api_module.__dict__)

    # Jedi completions generation
    def _generate_jedi_script(self, line_no=None, col_no=None):
        return jedi.Script(self.editor.text(),
                           line=line_no,
                           column=col_no,
                           path=self.editor.fileName(),
                           sys_path=sys.path)

    def _generate_jedi_completions_list(self, line_no=None, col_no=None):
        completions = self._generate_jedi_script(line_no, col_no).completions()
        completions_list = []
        for completion in completions:
            if not completion.name.startswith('_'):
                if completion.full_name:
                    completions_list.append(completion.full_name)
                else:
                    completions_list.append(completion.name)
        return completions_list

    def _generate_jedi_call_tips(self, line_no=None, col_no=None):
        call_sigs = self._generate_jedi_script(line_no, col_no).call_signatures()
        call_tips = []
        for signature in call_sigs:
            if signature.name.startswith('_'):
                continue
            args = []
            for param in signature.params:
                arg = param.description.replace('param ', '').replace('\n', ' ')
                if arg:
                    args.append(arg)

            if not args:
                continue

            call_tip = "{}({})".format(signature.name, ', '.join(args))
            call_tips.append(call_tip)
            if call_tips:
                # We want call tips to be persistent
                self._add_to_interpreter_completions(call_tips)

        return call_tips

    def _on_cursor_position_changed(self, line_no, col_no):
        """
        Slot to be executed when editor's cursor position has been changed.
        Note that Qt's line numbering starts at 0, whilst jedi's starts at 1.
        :param int line_no: The cursor's new line number
        :param int col_no: The cursor's new column number
        """
        line = self.editor.text().split('\n')[line_no][:col_no]
        if not line or col_no < 2 or (len(line.strip()) < 2):
            return

        # Don't get jedi completions unless we are two characters clear of the
        # last non-alpha character
        last_var_separator = re.search("[^a-zA-Z_\d\s:]| ", line[::-1])
        if last_var_separator:
            var = line[len(line) - last_var_separator.end() + 1:]
            if len(var) < 2 or var[0].isdigit():
                return

        jedi_completions = []
        if line.strip() and not (line.rstrip()[-1] == '(' and col_no >= line.rfind('(')):
            print("Completions")
            jedi_completions = self._generate_jedi_completions_list(line_no + 1, col_no)
        elif line.rfind(')') < line.rfind('('):
            print("Call tips")
            call_tips = self._generate_jedi_call_tips(line_no + 1, col_no)
            jedi_completions.extend(call_tips)

        if jedi_completions:
            self.editor.updateCompletionAPI(
                set(jedi_completions + self._interpreter_completions.keys()))
