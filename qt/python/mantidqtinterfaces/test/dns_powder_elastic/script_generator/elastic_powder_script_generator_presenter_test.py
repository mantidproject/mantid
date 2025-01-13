# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

# yapf: disable
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import \
    DNSObserver
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    common_script_generator_presenter import DNSScriptGeneratorPresenter
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_powder_elastic.script_generator.\
    elastic_powder_script_generator_model \
    import DNSElasticPowderScriptGeneratorModel  # yapf: disable
from mantidqtinterfaces.dns_powder_elastic.script_generator.\
    elastic_powder_script_generator_presenter\
    import DNSElasticPowderScriptGeneratorPresenter  # yapf: disable
# yapf: enable


class DNSElasticPowderScriptGeneratorPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access

    view = None
    model = None
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSScriptGeneratorView, instance=True)
        cls.view.sig_progress_canceled.connect = mock.Mock()
        cls.view.sig_generate_script.connect = mock.Mock()
        cls.view.get_state.return_value = {"script_filename": "script.txt", "automatic_filename": False}
        cls.model = mock.create_autospec(DNSElasticPowderScriptGeneratorModel, instance=True)
        cls.model.get_plot_list.return_value = [1, 2, 3]
        cls.presenter = DNSElasticPowderScriptGeneratorPresenter(
            view=cls.view, model=cls.model, name="elastic_powder_script_generator", parent=cls.parent
        )

    def setUp(self):
        self.view.reset_mock()
        self.model.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSElasticPowderScriptGeneratorPresenter)
        self.assertIsInstance(self.presenter, DNSScriptGeneratorPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertTrue(hasattr(self.presenter, "_plot_list"))

    def test_get_option_dict(self):
        test_v = self.presenter.get_option_dict()
        self.view.get_state.assert_called_once()
        self.assertEqual(len(test_v), 6)

    def test_finish_script_run(self):
        self.model._plot_list = None
        self.presenter._finish_script_run()
        self.model.get_plot_list.assert_called_once()
        self.assertEqual(self.presenter._plot_list, [1, 2, 3])


if __name__ == "__main__":
    unittest.main()
