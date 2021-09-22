# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest

from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.widgets.instrumentview.io import InstrumentViewEncoder


class DecoderFactoryTest(unittest.TestCase):
    def setUp(self):
        DecoderFactory.register_decoder(InstrumentViewEncoder)

    def test_find_decoder_can_find_an_decoder(self):
        self.assertNotEqual(None, DecoderFactory.find_decoder("InstrumentView"))
