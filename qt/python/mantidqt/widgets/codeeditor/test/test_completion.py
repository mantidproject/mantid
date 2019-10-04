# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import re
import unittest

from mantid.simpleapi import Rebin  # noqa  # needed to sys.modules can pick up Rebin
from mantid.py3compat.mock import Mock
from mantidqt.widgets.codeeditor.completion import CodeCompleter, get_function_spec
from testhelpers import assertRaisesNothing


class CodeCompletionTest(unittest.TestCase):

    def _get_completer(self, text, env_globals=None):
        return CodeCompleter(Mock(text=lambda: text, fileName=lambda: ""), env_globals)

    def test_Rebin_call_tips_generated_on_construction_when_api_import_in_script(self):
        completer = self._get_completer("from mantid.simpleapi import *\n# My code")
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertGreater(len(call_tips), 1)
        self.assertTrue(re.search("Rebin\(InputWorkspace, .*\)", ' '.join(call_tips)))

    def test_simple_api_call_tips_not_generated_on_construction_if_api_import_not_in_script(self):
        completer = self._get_completer("import numpy as np\n# My code")
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertFalse(bool(re.search("Rebin\(InputWorkspace, .*\)", ' '.join(call_tips))))

    def test_nothing_raised_when_getting_completions_from_a_not_imported_module(self):
        completer = self._get_completer("# My code")
        assertRaisesNothing(self, completer._get_module_call_tips, 'this.doesnt.exist')

    def test_get_function_spec_returns_expected_string_for_explicit_args(self):
        def my_new_function(arg1, arg2, kwarg1=None, kwarg2=0):
            pass
        self.assertEqual("(arg1, arg2, [kwarg1], [kwarg2])",
                         get_function_spec(my_new_function))

    def test_get_function_spec_returns_expected_string_for_implicit_args(self):
        def my_new_function(*args, **kwargs):
            pass
        self.assertEqual("(args, [**kwargs])",
                         get_function_spec(my_new_function))


if __name__ == '__main__':
    unittest.main()
