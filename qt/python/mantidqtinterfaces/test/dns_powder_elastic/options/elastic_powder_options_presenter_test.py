# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter import DNSCommonOptionsPresenter
from mantidqtinterfaces.dns_powder_elastic.options.elastic_powder_options_presenter import DNSElasticPowderOptionsPresenter
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_fake_param_dict


class DNSElasticPowderOptionsPresenterTest(unittest.TestCase):
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
        cls.view.sig_get_wavelength = mock.Mock()
        cls.view.sig_get_wavelength.connect = mock.Mock()
        cls.view.get_state = mock.Mock(
            return_value={
                "wavelength": 4.74,
                "get_wavelength": False,
            }
        )
        cls.view.raise_error = mock.Mock()
        cls.view.set_state = mock.Mock()
        cls.view.show_statusmessage = mock.Mock()
        cls.presenter = DNSElasticPowderOptionsPresenter(view=cls.view, model=cls.model, parent=cls.parent, name="elastic_powder_options")
        cls.presenter.param_dict = get_fake_param_dict()
        cls.presenter.param_dict["tof_powder_options"] = {"123": 123}

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSElasticPowderOptionsPresenter)
        self.assertIsInstance(self.presenter, DNSCommonOptionsPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.view.sig_get_wavelength.connect.assert_called_once_with(self.presenter._determine_wavelength)

    @patch(
        "mantidqtinterfaces.dns_powder_elastic.options."
        "elastic_powder_options_presenter."
        "DNSElasticPowderOptionsPresenter._determine_wavelength"
    )
    def test_process_request(self, mock_wavel):
        self.view.get_state.return_value = {"wavelength": 4.74, "get_wavelength": False}
        self.presenter.process_request()
        mock_wavel.assert_not_called()
        self.view.get_state.return_value = {"wavelength": 4.74, "get_wavelength": True}
        self.presenter.process_request()
        mock_wavel.assert_called_once()

    def test_process_commandline_request(self):
        self.view.reset_mock()
        self.presenter.process_commandline_request({"det_efficiency": 1})
        self.view.set_single_state_by_name.assert_called_with("det_efficiency", 1)


if __name__ == "__main__":
    unittest.main()
