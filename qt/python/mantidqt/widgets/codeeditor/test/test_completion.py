# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, unicode_literals)

import re
import unittest

import matplotlib.pyplot as plt  # noqa
import numpy as np  # noqa

from mantid.simpleapi import Rebin  # noqa  # needed so sys.modules can pick up Rebin
from mantid.py3compat.mock import Mock
from mantidqt.widgets.codeeditor.completion import (CodeCompleter, get_function_spec,
                                                    get_builtin_argspec, get_module_import_alias)
from testhelpers import assertRaisesNothing


class CodeCompletionTest(unittest.TestCase):

    def _get_completer(self, text, env_globals=None):
        return CodeCompleter(Mock(text=lambda: text, fileName=lambda: ""), env_globals)

    def _run_check_call_tip_generated(self, script_text, call_tip_regex):
        completer = self._get_completer(script_text)
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertGreater(len(call_tips), 1)
        self.assertTrue(re.search(call_tip_regex, ' '.join(call_tips)))

    def _run_check_call_tip_not_generated(self, script_text, call_tip_regex):
        completer = self._get_completer(script_text)
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertFalse(bool(re.search(call_tip_regex, ' '.join(call_tips))))

    def test_Rebin_call_tips_generated_on_construction_when_api_import_in_script(self):
        self._run_check_call_tip_generated("from mantid.simpleapi import *\n# My code",
                                           "Rebin\(InputWorkspace, .*\)")

    def test_numpy_call_tips_generated_if_numpy_imported_in_script(self):
        self._run_check_call_tip_generated("import numpy as np\n# My code",
                                           "np\.asarray\(a, \[dtype\], .*\)")

    def test_pyplot_call_tips_generated_if_imported_in_script(self):
        self._run_check_call_tip_generated("import matplotlib.pyplot as plt\n# My code",
                                           "plt\.figure\(\[num\], .*\)")

    def test_simple_api_call_tips_not_generated_on_construction_if_api_import_not_in_script(self):
        self._run_check_call_tip_not_generated("import numpy as np\n# My code", "Rebin")

    def test_numpy_call_tips_not_generated_if_its_not_imported(self):
        self._run_check_call_tip_not_generated("# My code", "numpy")

    def test_pyplot_call_tips_not_generated_if_its_not_imported(self):
        self._run_check_call_tip_not_generated("# My code", "pyplot")

    def test_nothing_raised_when_getting_completions_from_a_not_imported_module(self):
        completer = self._get_completer("# My code")
        assertRaisesNothing(self, completer._get_module_call_tips, 'this.doesnt.exist')

    def test_get_function_spec_returns_expected_string_for_explicit_args(self):
        def my_new_function(arg1, arg2, kwarg1=None, kwarg2=0):
            pass

        self.assertEqual("(arg1, arg2, [kwarg1], [kwarg2])", get_function_spec(my_new_function))

    def test_get_function_spec_returns_expected_string_for_implicit_args(self):
        def my_new_function(*args, **kwargs):
            pass

        self.assertEqual("(args, [**kwargs])", get_function_spec(my_new_function))

    def test_get_builtin_argspec_generates_argspec_for_numpy_builtin(self):
        argspec = get_builtin_argspec(np.zeros)
        self.assertIn("shape, dtype, order", ', '.join(argspec.args))
        self.assertIn("float, 'C'", ', '.join(argspec.defaults))

    def test_get_module_import_alias_finds_import_aliases(self):
        script = ("import numpy as np\n"
                  "from keyword import kwlist as key_word_list\n"
                  "import matplotlib.pyplot as plt\n"
                  "import mymodule.some_func as func, something as _smthing\n"
                  "# import commented.module as not_imported\n"
                  "import thing as _thing # import kwlist2 as kew_word_list2")
        aliases = {
            'numpy': 'np',
            'kwlist': 'key_word_list',
            'matplotlib.pyplot': 'plt',
            'mymodule.some_func': 'func',
            'something': '_smthing',
            'commented.module': 'commented.module',
            'kwlist2': 'kwlist2'  # alias is commented out so expect alias to not be assigned
        }
        for import_name, alias in aliases.items():
            self.assertEqual(alias, get_module_import_alias(import_name, script))


if __name__ == '__main__':
    unittest.main()
