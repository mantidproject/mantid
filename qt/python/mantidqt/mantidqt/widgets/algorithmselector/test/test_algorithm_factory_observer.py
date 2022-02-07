# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import threading
import unittest
from unittest.mock import MagicMock, patch

from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.algorithmselector.algorithm_factory_observer import AlgorithmSelectorFactoryObserver


class ToyAlgorithm(PythonAlgorithm):
    def category(self):
        return "Examples"

    def PyInit(self):
        pass

    def PyExec(self):
        pass


@start_qapplication
class AlgorithmFactoryObserverTest(unittest.TestCase):
    def setUp(self):
        self._alg_factory = AlgorithmFactory.Instance()
        self._alg_factory.enableNotifications()

    def test_can_toggle_algorithm_factory_notifications(self):
        notifyee = MagicMock()
        # variable keeps object alive
        observer = AlgorithmSelectorFactoryObserver(notifyee)
        observer.observeUpdate(False)

        AlgorithmFactory.subscribe(ToyAlgorithm)

        self.assertTrue(notifyee.refresh.call_count == 0,
                        "We expect the widget to be refreshed when the Algorithm Factory "
                        "subscribes to a new algorithm. refresh was called "
                        "{widget.refresh.call_count} times after subscription.")

    def test_that_selector_is_refreshed_on_alg_sibscribe_from_same_thread(self):
        notifyee = MagicMock()
        # variable keeps object alive
        _ = AlgorithmSelectorFactoryObserver(notifyee)  # noqa: F841

        AlgorithmFactory.subscribe(ToyAlgorithm)

        self.assertTrue(notifyee.refresh.call_count == 1,
                        f"We expect the widget to be refreshed when the Algorithm Factory "
                        f"subscribes to a new algorithm. refresh was called "
                        f"{notifyee.refresh.call_count} times after subscription.")

    @patch("mantidqt.widgets.algorithmselector.algorithm_factory_observer.QAppThreadCall")
    def test_that_selector_is_refreshed_on_alg_subscribe_from_a_different_thread(
            self, mock_qappcall):
        notifyee = MagicMock()
        # variable keeps object alive
        _ = AlgorithmSelectorFactoryObserver(notifyee)  # noqa: F841
        mock_qappcall.assert_called_once()

        subscriber = threading.Thread(target=AlgorithmFactory.subscribe, args=[ToyAlgorithm])
        subscriber.start()
        subscriber.join()

        self.assertTrue(mock_qappcall.call_count == 1,
                        f"We expect the widget to be refreshed when the Algorithm Factory "
                        f"subscribes to a new algorithm. refresh was called "
                        f"{mock_qappcall.refresh.call_count} times after subscription.")


if __name__ == '__main__':
    unittest.main()
