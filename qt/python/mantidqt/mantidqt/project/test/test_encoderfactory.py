# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest

from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.widgets.instrumentview.io import InstrumentViewEncoder
from mantidqt.widgets.workspacedisplay.matrix.io import MatrixWorkspaceDisplayEncoder
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantid.simpleapi import CreateSampleWorkspace
from mantid.api import AnalysisDataService as ADS
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class EncoderFactoryTest(unittest.TestCase):
    def setUp(self):
        EncoderFactory.encoder_dict = {}
        EncoderFactory.register_encoder(InstrumentViewEncoder)
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.instrument_view = InstrumentViewPresenter(ADS.retrieve("ws")).container

    def test_find_encoder_can_find_an_encoder(self):
        self.assertNotEqual(None, EncoderFactory.find_encoder(self.instrument_view))

    def test_register_encoder_has_uniqueness_preserved(self):
        EncoderFactory.register_encoder(InstrumentViewEncoder)
        self.assertEqual(1, len(EncoderFactory.encoder_dict))

    def test_register_encoder_can_find_correct_encoder_from_multiple_encoders(self):
        EncoderFactory.register_encoder(MatrixWorkspaceDisplayEncoder)
        self.assertNotEqual(None, EncoderFactory.find_encoder(self.instrument_view))
        self.assertEqual(2, len(EncoderFactory.encoder_dict))
