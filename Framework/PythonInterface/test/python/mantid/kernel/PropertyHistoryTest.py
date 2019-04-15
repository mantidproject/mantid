# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import PropertyHistory


class PropertyHistoryTest(unittest.TestCase):

    def test_history_construction(self):
        prop_name = 'TestName'
        prop_value = 'TestValue'
        prop_type = 'str'
        is_default = True
        direction = 0

        hist = PropertyHistory(prop_name, prop_value, prop_type, is_default, direction)
        self.assertEqual(prop_name, hist.name())
        self.assertEqual(prop_value, hist.value())
        self.assertEqual(prop_type, hist.type())
        self.assertEqual(is_default, hist.isDefault())
        self.assertEqual(direction, hist.direction())


if __name__ == '__main__':
    unittest.main()
