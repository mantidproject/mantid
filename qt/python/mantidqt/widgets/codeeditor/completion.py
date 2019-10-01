# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import inspect
from six import PY2
if PY2:
    from inspect import getargspec as getfullargspec
else:
    from inspect import getfullargspec

from mantidqt.widgets.codeeditor.editor import CodeEditor


class CodeCompleter:

    def __init__(self, editor, env_globals=None):
        self.editor = editor
        self.env_globals = env_globals

        self.editor.enableAutoCompletion(CodeEditor.AcsAll)

    def update_completion_api(self):
        self.editor.updateCompletionAPI(self.generate_call_tips())

    def generate_call_tips(self):
        if not self.env_globals:
            return []
        call_tips = []
        for name, attr, in self.env_globals.items():
            if inspect.isfunction(attr) or inspect.isbuiltin(attr):
                call_tips.append(name + self.get_function_spec(attr))
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
            arg_str = argspec[1].strip().lstrip('\b')
            defs = []
            # Keyword args
            kwargs = argspec[2]
            if kwargs is not None:
                kwargs = kwargs.strip().lstrip('\b\b')
                if kwargs == 'kwargs':
                    kwargs = '**' + kwargs + '=None'
                arg_str += ',%s' % kwargs
            # Any default argument appears in the string
            # on the rhs of an equal
            for arg in arg_str.split(','):
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
            call_tip = ','.join(args)
            call_tip = '(' + call_tip + ')'
        else:
            # The defaults list contains the default values for the last n arguments
            diff = len(args) - len(defs)
            call_tip = ''
            for index in range(len(args) - 1, -1, -1):
                def_index = index - diff
                if def_index >= 0:
                    call_tip = '[' + args[index] + '],' + call_tip
                else:
                    call_tip = args[index] + "," + call_tip
            call_tip = '(' + call_tip.rstrip(',') + ')'
        return call_tip
