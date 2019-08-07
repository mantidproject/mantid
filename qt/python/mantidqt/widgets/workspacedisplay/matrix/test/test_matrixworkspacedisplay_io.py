# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from __future__ import absolute_import

import unittest

from mantid.simpleapi import CreateSampleWorkspace
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.workspacedisplay.matrix.io import MatrixWorkspaceDisplayDecoder, MatrixWorkspaceDisplayEncoder
from mantidqt.widgets.workspacedisplay.matrix import StatusBarView, MatrixWorkspaceDisplay


MATRIXWORKSPACEDISPLAY_DICT = {"workspace": "ws"}


@start_qapplication
class MatrixWorkspaceDisplayEncoderTest(unittest.TestCase):
    def setUp(self):
        self.ws = CreateSampleWorkspace(OutputWorkspace="ws")
        self.encoder = MatrixWorkspaceDisplayEncoder()
        self.view = MatrixWorkspaceDisplayDecoder().decode(MATRIXWORKSPACEDISPLAY_DICT)

    def test_encoder_returns_complete_dictionary(self):
        self.assertEqual(MATRIXWORKSPACEDISPLAY_DICT, self.encoder.encode(self.view))


# matplotlib is not used so patch it out to avoid tkinter errors on older matplotlib versions
@mock.patch('matplotlib.pyplot._backend_selection')
@start_qapplication
class MatrixWorkspaceDisplayDecoderTest(unittest.TestCase):
    def setUp(self):
        self.ws = CreateSampleWorkspace(OutputWorkspace="ws")
        self.decoder = MatrixWorkspaceDisplayDecoder()

    def test_decoder_returns_view(self, _):
        decoded_object = self.decoder.decode(MATRIXWORKSPACEDISPLAY_DICT)
        self.assertEqual(decoded_object.__class__, StatusBarView)
        self.assertEqual(decoded_object.presenter.__class__, MatrixWorkspaceDisplay)

    def test_decoder_returns_workspace(self, _):
        view = self.decoder.decode(MATRIXWORKSPACEDISPLAY_DICT)
        self.assertEqual(self.ws.name(), view.presenter.model._ws.name())
