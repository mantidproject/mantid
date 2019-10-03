# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import re
import unittest

from mantid.simpleapi import *  # noqa  # we need this so simpleapi is in sys.modules
from mantid.py3compat.mock import Mock, patch
from mantidqt.widgets.codeeditor.completion import CodeCompleter


class CodeCompletionTest(unittest.TestCase):

    def _get_completer(self, text, env_globals=None, enable_jedi=True):
        return CodeCompleter(Mock(text=lambda: text, fileName=lambda: ""), env_globals,
                             enable_jedi=enable_jedi)

    def test_Rebin_call_tips_generated_on_construction_when_api_import_in_script(self):
        completer = self._get_completer("from mantid.simpleapi import *\n# My code")
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertTrue(len(call_tips) > 1)
        self.assertTrue(re.search("Rebin\(InputWorkspace, .*\)", ' '.join(call_tips)))

    @patch('mantidqt.widgets.codeeditor.completion.jedi')
    def test_jedi_is_not_called_on_construction_when_enable_jedi_is_False(self, jedi_mock):
        self._get_completer("import numpy as np\n# My code", enable_jedi=False)
        self.assertEqual(0, jedi_mock.Script.call_count)

    def test_simple_api_call_tips_not_generated_on_construction_if_api_import_not_in_script(self):
        completer = self._get_completer("import numpy as np\n# My code")
        update_completion_api_mock = completer.editor.updateCompletionAPI
        call_tips = update_completion_api_mock.call_args_list[0][0][0]
        self.assertEqual(1, update_completion_api_mock.call_count)
        self.assertFalse(bool(re.search("Rebin\(InputWorkspace, .*\)", ' '.join(call_tips))))

    def test_completion_api_is_updated_with_numpy_completions_when_cursor_position_changed(self):
        completer = self._get_completer("import numpy as np\nnp.ar")
        completer._on_cursor_position_changed(1, 3)
        update_completion_api_mock = completer.editor.updateCompletionAPI
        completions = update_completion_api_mock.call_args_list[0][0][0]
        self.assertTrue(bool(re.search("array<sep>", '<sep>'.join(completions))))

    def test_call_tips_are_still_visible_after_argument_inserted(self):
        completer = self._get_completer("import numpy as np\nnp.array(x, ")
        joined_call_tips = ' '.join(completer._generate_jedi_call_tips(2, 12))
        self.assertTrue(bool(re.search("array\(object, .*\)", joined_call_tips)))

    def test_call_tips_are_not_generated_when_outside_a_function_call(self):
        completer = self._get_completer("import numpy as np\nnp.array(x)")
        self.assertEqual(0, len(completer._generate_jedi_call_tips(2, 11)))

    def test_that_generate_jedi_completions_list_is_not_called_when_cursor_is_inside_bracket(self):
        completer = self._get_completer("import numpy as np\nnp.array(    ")
        with patch.object(completer, '_generate_jedi_completions_list'):
            completer._on_cursor_position_changed(1, 11)
            self.assertEqual(0, completer._generate_jedi_completions_list.call_count)

    def test_that_generate_jedi_completions_list_is_called_when_cursor_after_closed_brackets(self):
        completer = self._get_completer("import numpy as np\nnp.array(x)    ")
        with patch.object(completer, '_generate_jedi_completions_list'):
            completer._on_cursor_position_changed(1, 11)
            self.assertEqual(1, completer._generate_jedi_completions_list.call_count)


if __name__ == '__main__':
    unittest.main()
