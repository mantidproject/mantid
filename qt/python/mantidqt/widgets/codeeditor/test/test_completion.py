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
from mantidqt.widgets.codeeditor.completion import CodeCompleter, get_function_spec
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
                                           "numpy\.asarray\(a, \[dtype\], .*\)")

    def test_pyplot_call_tips_generated_if_imported_in_script(self):
        self._run_check_call_tip_generated("import matplotlib.pyplot as plt\n# My code",
                                           "matplotlib.pyplot\.figure\(\[num\], .*\)")

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


if __name__ == '__main__':
    unittest.main()
