# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import AlgorithmManager, FileProperty, FileAction, FrameworkManagerImpl
from mantid.kernel import Direction


class FilePropertyTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManagerImpl.Instance()

    def test_constructor_with_name_and_default_and_action(self):
        prop = FileProperty("LoadProperty", "", FileAction.Load)
        self.assertNotEquals("", prop.isValid)
        self.assertEqual(Direction.Input, prop.direction)

    def test_constructor_with_name_and_default_and_action_and_exts_list(self):
        prop = FileProperty("LoadProperty", "", FileAction.Load, ['.nxs', '.raw'])
        self.assertNotEquals("", prop.isValid)
        self.assertEqual(Direction.Input, prop.direction)
        allowed = prop.allowedValues
        self.assertTrue('.nxs' in allowed)
        self.assertTrue('.raw' in allowed)

    def test_constructor_with_name_and_default_and_action_and_single_ext(self):
        prop = FileProperty("LoadProperty", "", FileAction.Load, '.nxs')
        self.assertNotEquals("", prop.isValid)
        self.assertEqual(Direction.Input, prop.direction)
        allowed = prop.allowedValues
        self.assertTrue('.nxs' in allowed)

    def test_constructor_with_name_and_default_and_action_and_single_ext_and_direction(self):
        prop = FileProperty("LoadProperty", "", FileAction.Load, ['.nxs'], Direction.InOut)
        self.assertNotEquals("", prop.isValid)
        self.assertEqual(Direction.InOut, prop.direction)

    def test_alg_get_property_converts_to_this(self):
        alg = AlgorithmManager.createUnmanaged("LoadRaw")
        alg.initialize()
        prop = alg.getProperty("Filename")
        self.assertEqual(type(prop), FileProperty)
        self.assertTrue('value' in dir(prop)) # Do we have a value method

if __name__ == '__main__':
    unittest.main()
