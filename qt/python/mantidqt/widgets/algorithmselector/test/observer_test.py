# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import

import unittest

from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantid.py3compat import mock
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget
from mantidqt.widgets.algorithmselector.algorithm_factory_observer import AlgorithmSelectorFactoryObserver


class ToyAlgorithm(PythonAlgorithm):

    def category(self):
        return "Examples"

    def PyInit(self):
        pass

    def PyExec(self):
        pass


class AlgorithmSelectorWidgetMock(AlgorithmSelectorWidget):
    """
    This class removes Qt depencies from AlgorithmSelectorWidget
    for easier testing
    """
    def __init__(self):
        self.afo = AlgorithmSelectorFactoryObserver(self)


class ObserverTest(unittest.TestCase):
    def setUp(self):
        self._alg_factory = AlgorithmFactory.Instance()
        self._alg_factory.enableNotifications()

    def test_can_toggle_algorithm_factory_notifications(self):
        widget = AlgorithmSelectorWidgetMock()
        widget.refresh = mock.MagicMock()
        widget.observeUpdate(True)
        widget.observeUpdate(False)

        AlgorithmFactory.subscribe(ToyAlgorithm)

        self.assertTrue(widget.refresh.call_count == 0,
                        "We expect the widget to be refreshed when the Algorithm Factory "
                        "subscribes to a new algorithm. refresh was called "
                        "{} times after subscription.".format(widget.refresh.call_count))

    def test_that_selector_is_refreshed_on_alg_load(self):
        widget = AlgorithmSelectorWidgetMock()
        widget.refresh = mock.MagicMock()
        widget.observeUpdate(True)
        AlgorithmFactory.subscribe(ToyAlgorithm)

        self.assertTrue(widget.refresh.call_count == 1,
                        "We expect the widget to be refreshed when the Algorithm Factory "
                        "subscribes to a new algorithm. refresh was called "
                        "{} times after subscription.".format(widget.refresh.call_count))


if __name__ == '__main__':
    unittest.main()
