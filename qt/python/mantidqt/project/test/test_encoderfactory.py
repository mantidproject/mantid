# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.widgets.instrumentview.io import InstrumentViewEncoder
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantid.simpleapi import CreateSampleWorkspace
from mantid.api import AnalysisDataService as ADS
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class EncoderFactoryTest(unittest.TestCase):
    def setUp(self):
        EncoderFactory.register_encoder(InstrumentViewEncoder)
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.instrument_view = InstrumentViewPresenter(ADS.retrieve("ws")).container

    def test_find_encoder_can_find_an_encoder(self):
        self.assertNotEqual(None, EncoderFactory.find_encoder(self.instrument_view))
