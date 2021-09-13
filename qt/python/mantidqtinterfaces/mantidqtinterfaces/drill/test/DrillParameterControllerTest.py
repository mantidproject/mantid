# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.model.DrillParameterController \
        import DrillParameterController, DrillControllerSignals


class DrillParameterControllerTest(unittest.TestCase):

    def setUp(self):
        #mock queue
        patch = mock.patch(
                "mantidqtinterfaces.drill.model.DrillParameterController.queue")
        self.mQueue = patch.start()
        self.addCleanup(patch.stop)
        self.mQueue = self.mQueue.Queue.return_value
        # mock sapi
        patch = mock.patch(
                'mantidqtinterfaces.drill.model.DrillParameterController.sapi')
        self.mSapi = patch.start()
        self.addCleanup(patch.stop)

        self.controller = DrillParameterController("test")

    def test_signals(self):
        self.assertTrue(isinstance(self.controller.signals,
                                   DrillControllerSignals))

    def test_check(self):
        param = mock.Mock()
        self.controller.check(param)
        self.mQueue.put.assert_called_once_with(param)

    def test_stop(self):
        self.controller._running = True
        self.controller.join = mock.Mock()
        self.controller.stop()
        self.assertEqual(self.controller._running, False)
        self.controller.join.assert_called_once()


if __name__ == "__main__":
    unittest.main()
