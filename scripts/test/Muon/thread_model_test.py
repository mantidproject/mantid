import unittest
import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from qtpy.QtWidgets import QApplication

from Muon.GUI.Common.thread_model import ThreadModel


# def check_model_has_correct_attributes(self):
#     pass

class testModel:

    def __init__(self):
        self._data = None

    def loadData(self, data):
        self._data = data

    def execute(self):
        for item in self.data:
            print(item)


class LoadFileWidgetViewTest(unittest.TestCase):

    def setUp(self):
        self.model = testModel()
        self.thread = ThreadModel(self.model)

    def mock_model(self):
        model = mock.Mock()
        model.loadData = mock.Mock(side_effect=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        model.execute = mock.Mock()
        return model

    def test_that_loadData_called_in_model_with_correct_inputs(self):
        self.model.loadData = mock.Mock()

        self.thread.loadData([1, 2, 3, 4, 5])

        self.assertEqual(self.model.loadData.call_count, 1)
        self.assertEqual(self.model.loadData.call_args_list[0][0][0], [1,2,3,4,5])


    def test_that_execute_is_called_in_model_when_thread_is_started(self):
        self.model.execute = mock.Mock()

        self.thread.start()
        self.thread.wait()

        self.assertEqual(self.model.execute.call_count, 1)

if __name__ == '__main__':
    QT_APP = QApplication([])
    unittest.main(buffer=False, verbosity=2)
    QT_APP.exec_()
