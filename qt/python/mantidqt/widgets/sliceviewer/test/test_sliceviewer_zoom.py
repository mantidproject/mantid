# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
import unittest
from unittest.mock import ANY, MagicMock

# 3rd party imports
#from matplotlib.backend_bases import FigureCanvas
from mantidqt.widgets.sliceviewer.zoom import ScrollZoomMixin


class ScrollZoomMixinTest(unittest.TestCase):
    class Canvas(ScrollZoomMixin, object):
        def __init__(self):
            self.mpl_connect = MagicMock()
            self.mpl_disconnect = MagicMock()

    def test_enable_zoom_connects_to_scroll_event(self):
        canvas = ScrollZoomMixinTest.Canvas()
        axes = MagicMock()
        canvas.enable_zoom_on_scroll(axes)

        canvas.mpl_connect.assert_called_once_with('scroll_event', ANY)
        axes.set_xlim.assert_not_called()
        axes.set_ylim.assert_not_called()

    def test_disable_zoom_without_enable_does_nothing(self):
        canvas = ScrollZoomMixinTest.Canvas()

        canvas.disable_zoom_on_scroll()

    def test_disable_zoom_after_enable_calls_disconnect_with_correct_id(self):
        canvas = ScrollZoomMixinTest.Canvas()
        axes = MagicMock()

        canvas.enable_zoom_on_scroll(axes)
        cid = canvas.scroll_cid
        canvas.disable_zoom_on_scroll()

        canvas.mpl_disconnect(cid)


if __name__ == '__name':
    unittest.main()
