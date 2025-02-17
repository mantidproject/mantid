# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest

from mantid.api import AlgorithmFactory, AlgorithmFactoryObserver, PythonAlgorithm
from unittest import mock


class ToyAlgorithm(PythonAlgorithm):
    def category(self):
        return "Examples"

    def PyInit(self):
        pass

    def PyExec(self):
        pass


class AnotherToyAlgorithm(PythonAlgorithm):
    def category(self):
        return "Examples"

    def PyInit(self):
        pass

    def PyExec(self):
        pass


class FakeAlgorithmFactoryObserver(AlgorithmFactoryObserver):
    # This method is going to be mocked out but needs to be present to actually get the hook from the C++ side
    def updateHandle(self):
        pass


class AlgorithmFactoryObserverTest(unittest.TestCase):
    def setUp(self):
        self.fake_class = FakeAlgorithmFactoryObserver()
        self.fake_class.updateHandle = mock.MagicMock()
        self._alg_factory = AlgorithmFactory.Instance()
        self._alg_factory.disableNotifications()

    def tearDown(self):
        self.fake_class.observeUpdate(False)

    def test_updateHandle_is_not_called_by_when_notifications_disabled(self):
        self.fake_class.observeUpdate(True)
        self._alg_factory.subscribe(ToyAlgorithm)
        self.assertTrue(
            self.fake_class.updateHandle.call_count == 0,
            "Did not expect updateHandle to be called unless notifications are explicitly enabled "
            "in AlgorithmFactory. updateHandle was "
            "called {} times.".format(self.fake_class.updateHandle.call_count),
        )

    def test_updateHandle_is_called(self):
        self.fake_class.observeUpdate(True)
        self._alg_factory.enableNotifications()
        self._alg_factory.subscribe(ToyAlgorithm)
        self._alg_factory.subscribe(AnotherToyAlgorithm)
        self.assertTrue(
            self.fake_class.updateHandle.call_count == 2,
            "Subscribed two algorithms, so expected updateHandle to be called twice when notifications"
            " are explicitly enabled in AlgorithmFactory. updateHandle was "
            "called {} times.".format(self.fake_class.updateHandle.call_count),
        )

    def test_updateHandle_is_not_called_if_not_observing_update(self):
        self._alg_factory.enableNotifications()
        self._alg_factory.subscribe(ToyAlgorithm)
        self.assertTrue(
            self.fake_class.updateHandle.call_count == 0,
            "Expected updateHandle not to be called when observeUpdate is False. updateHandle was called {} times.".format(
                self.fake_class.updateHandle.call_count
            ),
        )


if __name__ == "__main__":
    unittest.main()
