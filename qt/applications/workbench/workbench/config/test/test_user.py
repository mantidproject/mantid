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


class ConfigUserManager(object):
    def __init__(self, test_name_hint, defaults=None):
        """

        :param test_name_hint: Used to prevent config file collisions when
                               tests are ran in parallel
        :param defaults: Defaults to be used in the config file
        """
        if not defaults:
            self.defaults = {
                'main': {
                    'a_default_key': 100,
                    'bool_option': False,
                    'bool_option2': True
                },
            }
        else:
            self.defaults = defaults

        self.name = ConfigUserManager.__name__ + test_name_hint

    def __enter__(self):
        self.cfg = UserConfig(self.name, self.name, self.defaults)
        return self.cfg

    def __exit__(self, exc_type, exc_val, exc_tb):
        try:
            os.remove(self.cfg.filename)
        except:
            # File does not exist so it has been deleted already
            # This seems to happen when the whole module is ran in PyCharm
            # however it couldn't be replicated through CTest
            pass


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

    def test_lowercase_bool_loaded_correctly(self):
        defaults = {
            'main': {
                'a_default_key': 100,
                'lowercase_bool_option': 'false',
                'lowercase_bool_option2': 'true'
            },
        }
        with ConfigUserManager("lowercase", defaults) as cfg:
            self.assertFalse(cfg.get('main/lowercase_bool_option'))
            self.assertTrue(cfg.get('main/lowercase_bool_option2'))

    def test_all_keys_with_specified_group(self):
        with ConfigUserManager("allkeys") as cfg:
            keys = cfg.all_keys('main')
            self.assertEqual(3, len(keys))

            # order of the keys isn't guaranteed, so it's just checked that they are
            # inside the list of all keys
            self.assertTrue("a_default_key" in keys)
            self.assertTrue("bool_option" in keys)
            self.assertTrue("bool_option2" in keys)

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

    def test_set_raises_when_key_is_not_str_and_second_is_none(self):
        self.assertRaises(TypeError, self.cfg.set, 123, None)


if __name__ == '__main__':
    main()
