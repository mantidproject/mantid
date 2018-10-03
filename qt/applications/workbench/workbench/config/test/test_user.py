# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import

import os
from unittest import TestCase, main

from workbench.config.user import UserConfig


class ConfigUserTest(TestCase):

    @classmethod
    def setUpClass(cls):
        if not hasattr(cls, 'cfg'):
            defaults = {
                'main': {
                    'a_default_key': 100,
                    'bool_option': False,
                    'bool_option2': True
                },
            }
            cls.cfg = UserConfig(cls.__name__, cls.__name__, defaults)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.cfg.filename)
            del cls.cfg
        except OSError:
            pass

    # ----------------------------------------------
    # Success tests
    # ----------------------------------------------

    def test_stored_value_not_in_defaults_is_retrieved(self):
        self.cfg.set('main', 'key1', 1)
        self.assertEqual(1, self.cfg.get('main', 'key1'))

    def test_value_not_in_settings_retrieves_default_if_it_exists(self):
        self.assertEqual(100, self.cfg.get('main', 'a_default_key'))
        self.assertEqual(100, self.cfg.get('main/a_default_key'))

    def test_boolean_with_default_false_can_be_retrieved(self):
        self.assertEqual(False, self.cfg.get('main', 'bool_option'))
        self.assertEqual(False, self.cfg.get('main/bool_option'))

        self.assertEqual(True, self.cfg.get('main/bool_option2'))

    def test_has_keys(self):
        self.assertTrue(self.cfg.has('main', 'a_default_key'))
        self.assertTrue(self.cfg.has('main/a_default_key'))
        self.assertFalse(self.cfg.has('main', 'missing-key'))
        self.assertFalse(self.cfg.has('main/missing-key'))

    def test_remove_key(self):
        self.cfg.set('main', 'key1', 1)
        self.assertTrue(self.cfg.has('main/key1'))
        self.cfg.remove('main/key1')
        self.assertFalse(self.cfg.has('main/key1'))

    # ----------------------------------------------
    # Failure tests
    # ----------------------------------------------

    def test_get_raises_error_with_invalid_section_type(self):
        self.assertRaises(TypeError, self.cfg.get, 1, 'key1')

    def test_get_raises_error_with_invalid_option_type(self):
        self.assertRaises(TypeError, self.cfg.get, 'section', 1)

    def test_get_raises_keyerror_with_no_saved_setting_or_default(self):
        self.assertRaises(KeyError, self.cfg.get, 'main', 'missing-key')
        self.assertRaises(KeyError, self.cfg.get, 'main/missing-key')

    def test_set_raises_error_with_invalid_section_type(self):
        self.assertRaises(TypeError, self.cfg.set, 1, 'key1', 1)

    def test_set_raises_error_with_invalid_option_type(self):
        self.assertRaises(TypeError, self.cfg.set, 'section', 1, 1)


if __name__ == '__main__':
    main()
