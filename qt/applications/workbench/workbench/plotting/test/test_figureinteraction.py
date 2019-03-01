# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# system imports
import unittest

# third-party library imports
from mantid.py3compat.mock import MagicMock

# local package imports
from workbench.plotting.figureinteraction import FigureInteraction


class FigureInteractionTest(unittest.TestCase):

    # Success tests

    def test_construction_registers_handler_for_button_press_event(self):
        fig_manager = MagicMock()
        fig_manager.canvas = MagicMock()
        interactor = FigureInteraction(fig_manager)
        fig_manager.canvas.mpl_connect.assert_called_once_with('button_press_event',
                                                               interactor.on_mouse_button_press)

    def test_disconnect_called_for_each_registered_handler(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        fig_manager.canvas = canvas
        interactor = FigureInteraction(fig_manager)
        interactor.disconnect()
        self.assertEqual(interactor.nevents, canvas.mpl_disconnect.call_count)

    # Failure tests

    def test_construction_with_non_qt_canvas_raises_exception(self):
        class NotQtCanvas(object):
            pass

        class FigureManager(object):
            def __init__(self):
                self.canvas = NotQtCanvas()

        self.assertRaises(RuntimeError, FigureInteraction, FigureManager())


if __name__ == "__main__":
    unittest.main()
