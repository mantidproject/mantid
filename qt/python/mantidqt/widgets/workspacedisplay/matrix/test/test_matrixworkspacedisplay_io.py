# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

from mantidqt.utils.qt.testing import GuiTest
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.widgets.workspacedisplay.matrix.io import MatrixWorkspaceDisplayDecoder, MatrixWorkspaceDisplayEncoder
from mantidqt.widgets.workspacedisplay.matrix import StatusBarView, MatrixWorkspaceDisplay


MATRIXWORKSPACEDISPLAY_DICT = {"workspace": "ws"}


class MatrixWorkspaceDisplayEncoderTest(GuiTest):
    def setUp(self):
        self.ws = CreateSampleWorkspace(OutputWorkspace="ws")
        self.encoder = MatrixWorkspaceDisplayEncoder()
        self.view = MatrixWorkspaceDisplayDecoder().decode(MATRIXWORKSPACEDISPLAY_DICT)

    def test_encoder_returns_complete_dictionary(self):
        self.assertEqual(MATRIXWORKSPACEDISPLAY_DICT, self.encoder.encode(self.view))


class MatrixWorkspaceDisplayDecoderTest(GuiTest):
    def setUp(self):
        self.ws = CreateSampleWorkspace(OutputWorkspace="ws")
        self.decoder = MatrixWorkspaceDisplayDecoder()

    def test_decoder_returns_view(self):
        decoded_object = self.decoder.decode(MATRIXWORKSPACEDISPLAY_DICT)
        self.assertEqual(decoded_object.__class__, StatusBarView)
        self.assertEqual(decoded_object.presenter.__class__, MatrixWorkspaceDisplay)

    def test_decoder_returns_workspace(self):
        view = self.decoder.decode(MATRIXWORKSPACEDISPLAY_DICT)
        self.assertEqual(self.ws.name(), view.presenter.model._ws.name())
