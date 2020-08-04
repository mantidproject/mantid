# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from Interface.ui.drill.model.DrillParameterController \
        import DrillParameterController, ControllerSignals, DrillParameter


class DrillParameterControllerTest(unittest.TestCase):

    def setUp(self):
        # mock sapi
        patch = mock.patch(
                'Interface.ui.drill.model.DrillParameterController.sapi')
        self.mSapi = patch.start()
        self.addCleanup(patch.stop)

        self.controller = DrillParameterController("test")

    def test_signals(self):
        self.assertTrue(isinstance(self.controller.signals, ControllerSignals))

    def test_addParameter(self):
        p = DrillParameter("name", "value", 0)
        self.controller.addParameter(p)
        pGet = self.controller._paramQueue.get()
        self.assertEqual(p, pGet)

    def test_stop(self):
        self.controller._running = True
        self.controller.join = mock.Mock()
        self.controller.stop()
        self.assertEqual(self.controller._running, False)
        self.controller.join.assert_called_once()


if __name__ == "__main__":
    unittest.main()
