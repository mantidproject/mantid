# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
"""
There was some time spent looking into using jedi for the completions.
jedi performs static analysis on code, which aims to provide completions
from modules and objects without needing to run any code. Because of
this, jedi needed to be updated almost every time the cursor position
was changed. This reduced the script editors performance especially when
analyzing large modules like numpy (not to an unusable level, but it was
noticeable).

Further, jedi could not statically pick-up many of the simpleapi
algorithms, which are defined dynamically, so we had to provide these
ourselves anyway. jedi also caused workbench to crash when launching
through PyCharm's debugger using Python 2 (it ran fine when launched
normally - I think the cause was something to do with the debugging
process interfering with jedi's subprocess).

For these reasons it was agreed jedi would be dropped, possibly
revisiting when we move to Python 3.
"""

from __future__ import (absolute_import, unicode_literals)

import ast
import inspect
import re
import sys
from keyword import kwlist as python_keywords
from collections import namedtuple
from six import PY2
if PY2:  # noqa
    from inspect import getargspec as getfullargspec
else:  # noqa
    from inspect import getfullargspec

from mantidqt.widgets.codeeditor.editor import CodeEditor

ArgSpec = namedtuple("ArgSpec", "args varargs keywords defaults")


def get_builtin_argspec(builtin):
    """
    Get the call tips for a builtin function from its docstring
    :param builtin builtin: The builtin to generate call tips for
    :return inspect.ArgSpec: The ArgSpec of the builtin. If no function
        description is available in the docstring return None
    """
    doc_string = builtin.__doc__
    if not doc_string:
        return None
    func_descriptor = doc_string.split('\n')[0].strip()
    if re.search(builtin.__name__ + "\([\[\*a-zA-Z_].*\)", func_descriptor):
        args_string = func_descriptor[func_descriptor.find('(') + 1:func_descriptor.rfind(')')]
        all_args_list = args_string.split(', ')
        args = []
        defaults = []
        for arg in all_args_list:
            if '=' in arg:
                args.append(arg.split('=')[0].strip())
                defaults.append(arg.split('=')[1].strip())
            else:
                args.append(arg)
        arg_spec = ArgSpec(args, None, None, defaults)
        return arg_spec


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
        try:
            args_obj = inspect.getargs(func.__code__)
            argspec = ArgSpec(args_obj.args, args_obj.varargs, args_obj.varkw, defaults=None)
        except (TypeError, AttributeError, ValueError):
            if inspect.isbuiltin(func):
                argspec = get_builtin_argspec(func)
                if not argspec:
                    return ''
            else:
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
        call_tip = "({})".format(', '.join(args))
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


def generate_call_tips(definitions, prepend_module_name=None):
    """
    Generate call tips for a dictionary of object definitions (eg. globals()).
    The call tips generated are of the form:
        Load(InputWorkspace, [OutputWorkspace], ...)
    where squared braces denote key-word arguments.

    :param dict definitions: Dictionary with names of python objects as keys and
        the objects themselves as values
    :param prepend_module_name: bool, None, or str. Prepend the name of the module to the call tips
        default is None, if string given that string will act as the module name
    :returns list: A list of call tips
    """
    if not isinstance(definitions, dict):
        return []
    call_tips = []
    for name, py_object in definitions.items():
        if name.startswith('_'):
            continue
        if prepend_module_name is True:
            prepend_module_name = py_object.__module__
        if inspect.isfunction(py_object) or inspect.isbuiltin(py_object):
            if not prepend_module_name:
                call_tips.append(name + get_function_spec(py_object))
            else:
                call_tips.append(prepend_module_name + '.' + name + get_function_spec(py_object))
            continue
        # Ignore modules or we get duplicates of methods/classes that are imported
        # in outer scopes, e.g. numpy.array and numpy.core.array
        if inspect.ismodule(py_object):
            continue
        for attr in dir(py_object):
            try:
                f_attr = getattr(py_object, attr)
            except Exception:
                continue
            if attr.startswith('_'):
                continue
            if hasattr(f_attr, 'im_func') or inspect.isfunction(f_attr) or inspect.ismethod(f_attr):
                call_tip = name + '.' + attr + get_function_spec(f_attr)
            else:
                call_tip = name + '.' + attr
            if prepend_module_name:
                call_tips.append(prepend_module_name + '.' + call_tip)
    return call_tips


def get_line_number_from_index(string, index):
    return string[:index].count('\n')


def get_module_import_alias(import_name, text):
    for node in ast.walk(ast.parse(text)):
        if isinstance(node, ast.alias) and node.name == import_name:
            return node.asname
    return import_name


class CodeCompleter(object):
    """
    This class generates autocompletions for Workbench's script editor.
    It generates autocompletions from environment globals. These completions
    are updated on every successful script execution.
    """
    def __init__(self, editor, env_globals=None):
        self.editor = editor
        self.env_globals = env_globals

        # A dict gives O(1) lookups and ensures we have no duplicates
        self._completions_dict = dict()
        if "from mantid.simpleapi import *" in self.editor.text():
            self._add_to_completions(self._get_module_call_tips('mantid.simpleapi'))
        if re.search("^#{0}import .*numpy( |,|$)", self.editor.text(), re.MULTILINE):
            self._add_to_completions(self._get_module_call_tips('numpy'))
        if re.search("^#{0}import .*pyplot( |,|$)", self.editor.text(), re.MULTILINE):
            self._add_to_completions(self._get_module_call_tips('matplotlib.pyplot'))
        self._add_to_completions(python_keywords)

        self.editor.enableAutoCompletion(CodeEditor.AcsAPIs)
        self.editor.updateCompletionAPI(self.completions)

    @property
    def completions(self):
        return list(self._completions_dict.keys())

    def _get_completions_from_globals(self):
        return generate_call_tips(self.env_globals, prepend_module_name=True)

    def _add_to_completions(self, completions):
        for completion in completions:
            self._completions_dict[completion] = True

    def update_completion_api(self):
        self._add_to_completions(self._get_completions_from_globals())
        self.editor.updateCompletionAPI(self.completions)

    def _get_module_call_tips(self, module):
        """
        Get the call tips for a given module. If the module cannot be
        found in sys.modules return an empty list
        :param str module: The name of the module
        :return list: A list of call tips for the module
        """
        try:
            module = sys.modules[module]
        except KeyError:
            return []
        module_name = get_module_import_alias(module.__name__, self.editor.text())
        return generate_call_tips(module.__dict__, module_name)
