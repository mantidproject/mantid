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
from mantidqtinterfaces.dns.options.tof_powder_options_presenter\
    import DNSTofPowderOptionsPresenter
from mantidqtinterfaces.dns.tests.helpers_for_testing import (
    get_fake_param_dict, get_fake_tof_binning, get_fake_tof_errors)


class DNSTofPowderOptionsPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    view = None
    model = None
    parent = None
    presenter = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.Mock()
        cls.model = mock.Mock()
        cls.presenter = DNSTofPowderOptionsPresenter(view=cls.view,
                                                     model=cls.model,
                                                     parent=cls.parent,
                                                     name='tof_powder_options')
        cls.view.get_state = mock.Mock(return_value={
            'wavelength': 4.74,
            'get_wavelength': False,
            'dEstep': 0,
        })
        cls.view.raise_error = mock.Mock()
        cls.view.set_state = mock.Mock()
        cls.view.show_status_message = mock.Mock()

        cls.presenter.param_dict = get_fake_param_dict()
        cls.presenter.param_dict['tof_powder_options'] = {'123': 123}
        # fake one file tof set
        cls.model.estimate_q_and_binning = mock.Mock(
            return_value=[get_fake_tof_binning(),
                          get_fake_tof_errors()])

    def setUp(self):
        self.view.raise_error.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSTofPowderOptionsPresenter)
        self.assertIsInstance(self.presenter, DNSCommonOptionsPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)

    def test_estimate_q_and_binning(self):
        testv = self.presenter._estimate_q_and_binning()
        self.assertTrue(testv)
        self.assertEqual(self.view.raise_error.call_count, 2)
        self.view.raise_error.reset_mock()
        self.view.set_state.assert_called_once_with({'123': 123})
        self.presenter.param_dict['file_selector']['full_data'] = False
        testv = self.presenter._estimate_q_and_binning()
        self.assertFalse(testv)
        self.view.raise_error.assert_called_once()

    def test_process_request(self):
        self.presenter.process_request()
        self.view.show_status_message.assert_called_once()
        self.view.raise_error.assert_called_once()


if __name__ == '__main__':
    unittest.main()
