# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""

import unittest
from unittest import mock

from mantidqtinterfaces.dns.data_structures.dns_observer import \
    DNSObserver
from mantidqtinterfaces.dns.options.common_options_presenter import \
    DNSCommonOptionsPresenter


class DNSCommonOptionsPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    view = None
    model = None
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.Mock()
        cls.model = mock.Mock()
        cls.presenter = DNSCommonOptionsPresenter(view=cls.view,
                                                  model=cls.model,
                                                  parent=cls.parent,
                                                  name='common_options')

        cls.view.deactivate_get_wavelength = mock.Mock()
        cls.view.raise_error = mock.Mock()
        cls.model.determine_wavelength = mock.Mock(return_value=[4.74, {}])
        cls.view.get_state = mock.Mock(return_value={})

    def setUp(self):
        self.view.raise_error.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSObserver)

    def test_determine_wavelength(self):
        self.presenter.param_dict = {'file_selector': {'full_data': []}}
        testv = self.presenter._determine_wavelength()
        self.view.raise_error.assert_called_once()
        self.view.raise_error.reset_mock()
        self.assertIsNone(testv)
        self.presenter.param_dict = {'file_selector': {'full_data': [123]}}
        self.model.determine_wavelength.return_value = [
            4.74, {
                'wavelength_varies': 1
            }
        ]
        testv = self.presenter._determine_wavelength()
        self.view.raise_error.assert_called_once()
        self.view.raise_error.reset_mock()
        self.model.determine_wavelength.return_value = [4.74, {}]
        self.view.deactivate_get_wavelength.assert_called_once()
        self.view.deactivate_get_wavelength.reset_mock()
        self.view.raise_error.assert_not_called()
        testv = self.presenter._determine_wavelength()
        self.view.deactivate_get_wavelength.assert_not_called()
        self.assertEqual(testv, 4.74)


if __name__ == '__main__':
    unittest.main()
