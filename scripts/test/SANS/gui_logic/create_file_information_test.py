# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QCoreApplication
import unittest

from mantid.py3compat import mock
from sans.gui_logic.presenter.create_file_information import create_file_information
from ui.sans_isis.work_handler import WorkHandler

class CreateFileInformationTest(unittest.TestCase):
    def setUp(self):
        self.success_callback = mock.MagicMock()
        self.success_callback_1 = mock.MagicMock()
        self.error_callback = mock.MagicMock()
        self.work_handler = WorkHandler()
        self.qApp = QCoreApplication(['test_app'])

    def test_retieves_file_information_and_passes_to_callback(self):
        create_file_information('LOQ74044', self.error_callback, self.success_callback, self.work_handler, 0)
        self.work_handler.wait_for_done()
        self.qApp.processEvents()

        self.assertEqual(self.success_callback.call_count, 1)
        self.assertEqual(self.error_callback.call_count, 0)

    def test_that_retrieved_file_information_is_correct(self):
        create_file_information('LOQ74044', self.error_callback, self.success_callback, self.work_handler, 0)
        self.work_handler.wait_for_done()
        self.qApp.processEvents()

        file_information = self.success_callback.call_args[0][0]
        self.assertEqual(file_information.is_event_mode(), False)
        self.assertEqual(file_information.get_run_number(), 74044)
        self.assertEqual(file_information.get_thickness(), 1.0)

    def test_that_multiple_threading_calls_at_once_are_handled_cleanly(self):
        create_file_information('LOQ74044', self.error_callback, self.success_callback, self.work_handler, 0)
        create_file_information('LOQ74044', self.error_callback, self.success_callback_1, self.work_handler, 0)
        create_file_information('LOQ74044', self.error_callback, self.success_callback_1, self.work_handler, 1)
        create_file_information('LOQ74044', self.error_callback, self.success_callback_1, self.work_handler, 0)
        create_file_information('LOQ74044', self.error_callback, self.success_callback_1, self.work_handler, 2)
        self.work_handler.wait_for_done()
        self.qApp.processEvents()

        self.assertEqual(self.success_callback.call_count, 0)
        self.assertEqual(self.success_callback_1.call_count, 3)
        self.assertEqual(self.error_callback.call_count, 0)

    @mock.patch('sans.gui_logic.presenter.create_file_information.SANSFileInformationFactory')
    def test_that_error_callback_is_called_on_error_with_correct_message(self, file_information_factory_mock):
        file_information_factory_instance = mock.MagicMock()
        file_information_factory_instance.create_sans_file_information.side_effect = RuntimeError('File Error')
        file_information_factory_mock.return_value = file_information_factory_instance

        create_file_information('LOQ74044', self.error_callback, self.success_callback, self.work_handler, 0)
        self.work_handler.wait_for_done()
        self.qApp.processEvents()

        self.success_callback.assert_called_once_with(None)
        self.assertEqual(self.error_callback.call_count, 1)
        self.assertEqual(str(self.error_callback.call_args[0][0][1]), 'File Error')


if __name__ == '__main__':
    unittest.main()
