# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

import unittest
try:
    from unittest import mock
except ImportError:
    import mock

from mantidqt.widgets.codeeditor.scriptcompatibility import (mantid_api_import_needed,
                                                             mantid_algorithm_used)

PERMISSION_BOX_FUNC = ('mantidqt.widgets.codeeditor.scriptcompatibility.'
                       'permission_box_to_prepend_import')


class ScriptCompatibilityTest(unittest.TestCase):

    def setUp(self):
        self.api_import = "from mantid.simpleapi import *"
        self.test_cases = [self.api_import + "\ntest string",
                           self.api_import + "\nRebin()",
                           "Rebin()",
                           "test string"]

    def gen_fail_msg(self, case, expected_out):
        return "Test case '{}' failed with expected output '{}'" \
               "".format(case, expected_out)

    def test_mantid_api_import_needed(self):
        expected_results = [False, False, True, False]
        with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
            for case, expected_out in zip(self.test_cases, expected_results):
                self.assertEqual(mantid_api_import_needed(case), expected_out,
                                 msg=self.gen_fail_msg(case, expected_out))

    def test_mantid_algorithm_used(self):
        expected_results = [False, True, True, False]
        for case, expected_out in zip(self.test_cases, expected_results):
            self.assertEqual(mantid_algorithm_used(case), expected_out,
                             msg=self.gen_fail_msg(case, expected_out))


if __name__ == '__main__':
    unittest.main()
