# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.sliceviewer.io import SliceViewerDecoder, SliceViewerEncoder
from mantidqt.widgets.sliceviewer.views.view import SliceViewerView

SLICEVIEWER_DICT = {"workspace": "ws"}


@start_qapplication
class SliceViewerIOTest(unittest.TestCase):
    def setUp(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.view = SliceViewerDecoder().decode(SLICEVIEWER_DICT)

    def tearDown(self):
        self.view.close()
        ADS.clear()

    def test_decoder_returns_view_of_workspace(self):
        self.assertIsInstance(self.view, SliceViewerView)
        self.assertEqual("ws", self.view.presenter.model.ws.name())

    def test_encoder_returns_complete_dictionary(self):
        self.assertEqual(SLICEVIEWER_DICT, SliceViewerEncoder().encode(self.view))

    def test_encoder_and_decoder_are_registered(self):
        self.assertIsInstance(EncoderFactory.find_encoder(self.view), SliceViewerEncoder)
        self.assertIsInstance(DecoderFactory.find_decoder("SliceViewerView"), SliceViewerDecoder)


if __name__ == "__main__":
    unittest.main()
