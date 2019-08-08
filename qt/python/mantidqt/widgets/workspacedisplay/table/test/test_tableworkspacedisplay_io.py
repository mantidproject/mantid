# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from __future__ import (absolute_import, print_function)

import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import Load
from mantidqt.widgets.workspacedisplay.table.io import TableWorkspaceDisplayDecoder, TableWorkspaceDisplayEncoder
from mantidqt.widgets.workspacedisplay.table import StatusBarView


TABLEWORKSPACEDISPLAY_DICT = {"markedColumns": {"as_y": [2], "as_x": [1],
                                                "as_y_err": [{"column": 3, "relatedY": 2, "labelIndex": 0}]},
                              "workspace": "ws", "windowName": "ws"}


@start_qapplication
class TableWorkspaceDisplayEncoderTest(unittest.TestCase):
    def setUp(self):
        self.ws = Load("SavedTableWorkspace.nxs", OutputWorkspace="ws")
        self.encoder = TableWorkspaceDisplayEncoder()
        self.view = TableWorkspaceDisplayDecoder().decode(TABLEWORKSPACEDISPLAY_DICT)

    def test_encoder_returns_complete_dictionary(self):
        self.assertEqual(TABLEWORKSPACEDISPLAY_DICT, self.encoder.encode(self.view))


@start_qapplication
class TableWorkspaceDisplayDecoderTest(unittest.TestCase):
    def setUp(self):
        self.ws = Load("SavedTableWorkspace.nxs", OutputWorkspace="ws")
        self.decoder = TableWorkspaceDisplayDecoder()

    def test_decoder_returns_view(self):
        self.assertEqual(self.decoder.decode(TABLEWORKSPACEDISPLAY_DICT).__class__, StatusBarView)

    def test_decoder_returns_custom_features(self):
        view = self.decoder.decode(TABLEWORKSPACEDISPLAY_DICT)
        self.assertEqual(self.ws.name(), view.presenter.model.ws.name())
        self.assertEqual(TABLEWORKSPACEDISPLAY_DICT["markedColumns"]["as_y"], view.presenter.model.marked_columns.as_y)
        self.assertEqual(TABLEWORKSPACEDISPLAY_DICT["markedColumns"]["as_x"], view.presenter.model.marked_columns.as_x)
        self.assertEqual(
            TABLEWORKSPACEDISPLAY_DICT["markedColumns"]["as_y_err"][0]["column"],
            view.presenter.model.marked_columns.as_y_err[0].column)
        self.assertEqual(
            TABLEWORKSPACEDISPLAY_DICT["markedColumns"]["as_y_err"][0]["relatedY"],
            view.presenter.model.marked_columns.as_y_err[0].related_y_column)
        self.assertEqual(
            TABLEWORKSPACEDISPLAY_DICT["markedColumns"]["as_y_err"][0]["labelIndex"],
            view.presenter.model.marked_columns.as_y_err[0].label_index)
        self.assertEqual(1, len(view.presenter.model.marked_columns.as_y_err))


if __name__ == '__main__':
    unittest.main()
