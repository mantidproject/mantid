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
from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.widgets.instrumentview.io import Encoder, Decoder


class InstrumentViewEncoderTest(unittest.TestCase):
    def setUp(self):
        self.encoder = Encoder()

    def test_encoder_is_in_encoder_factory(self):
        # Shows that the decoder has been registered on import of something from mantidqt.widget.instrumentview
        found_encoder = EncoderFactory.find_encoder("InstrumentView")
        self.assertIs(Encoder, found_encoder)

    def test_encoder_encode_function_returns_none_when_obj_is_none(self):
        return_value = self.encoder.encode(None)
        self.assertIs(None, return_value)


class InstrumentViewDecoderTest(unittest.TestCase):
    def setUp(self):
        self.decoder = Decoder()

    def test_decoder_is_in_decoder_factory(self):
        # Shows that the decoder has been registered on import of something from mantidqt.widget.instrumentview
        found_decoder = DecoderFactory.find_decoder("InstrumentView")
        self.assertIs(Decoder, found_decoder)

    def test_decoder_decode_function_returns_none_when_obj_is_none(self):
        return_value = self.decoder.decode(None)
        self.assertIs(None, return_value)
