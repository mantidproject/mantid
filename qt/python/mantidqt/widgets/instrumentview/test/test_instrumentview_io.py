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
from mantidqt.widgets.instrumentview.io import InstrumentViewEncoder, InstrumentViewDecoder
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.testing import start_qapplication


INSTRUMENT_VIEW_DICT = {u'workspaceName': u'ws',
                        u'tabs': {u'maskTab': {
                            u'activeType': {
                                u'roiOn': False, u'groupingOn': False, u'maskingOn': True},
                            u'activeTools': {u'ellipseButton': False, u'moveButton': True,
                                             u'pointerButton': False, u'ringRectangleButton': False,
                                             u'freeDrawButton': False, u'ringEllipseButton': False},
                            u'maskWorkspaceSaved': False},
                            u'renderTab': {u'displayWireframe': False, u'displayLighting': False,
                                           u'labelPrecision': 1, u'useUCorrection': False, u'autoScaling': False,
                                           u'colorBar': {u'max': u'40', u'scaleType': 0, u'power': u'2',
                                                         u'min': u'40'},
                                           u'showLabels': True, u'flipView': False, u'displayDetectorsOnly': True,
                                           u'displayAxes': False, u'axesView': 0, u'showRows': True,
                                           u'useOpenGL': True, u'showRelativeIntensity': False},
                            u'treeTab': {u'expandedItems': []},
                            u'pickTab': {u'freeDraw': False, u'ringEllipse': False, u'edit': False,
                                         u'tube': False, u'peakSelect': False, u'zoom': False, u'one': True,
                                         u'ringRectangle': False, u'peak': False, u'ellipse': False,
                                         u'rectangle': False}}, u'surfaceType': 0,
                        u'actor': {u'binMasks': [], u'fileName': u'viridis'},
                        u'energyTransfer': [0.0, 20000.0],
                        u'surface': {u'shapes': [],
                                     u'alignmentInfo': [],
                                     u'backgroundColor': {u'blue': 0,
                                                          u'alpha': 255,
                                                          u'green': 0,
                                                          u'red': 0},
                                     u'projection3D': {u'viewport': {u'rotation': [1.0, 0.0, 0.0, 0.0],
                                                                     u'translation': {u'xTrans': 0.0,
                                                                                      u'yTrans': 0.0},
                                                                     u'zoom': 1.0}},
                                     u'projection3DSuccess': True},
                        u'currentTab': 0}


@start_qapplication
class InstrumentViewEncoderTest(unittest.TestCase):
    def setUp(self):
        self.encoder = InstrumentViewEncoder()
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.instrumentView = InstrumentViewDecoder().decode(INSTRUMENT_VIEW_DICT)

    def test_encoder_is_in_encoder_factory(self):
        # Shows that the decoder has been registered on import of something from mantidqt.widget.instrumentview
        found_encoder = EncoderFactory.find_encoder(self.instrumentView)
        self.assertIs(InstrumentViewEncoder, found_encoder.__class__)

    def test_encoder_encode_function_returns_none_when_obj_is_none(self):
        self.assertIs(None, self.encoder.encode(None))

    def test_encoder_encodes_a_dict_similar_to_set_dict(self):
        self.maxDiff = None
        self.assertDictEqual(self.encoder.encode(self.instrumentView), INSTRUMENT_VIEW_DICT)


@start_qapplication
class InstrumentViewDecoderTest(unittest.TestCase):
    def setUp(self):
        self.decoder = InstrumentViewDecoder()

    def test_decoder_is_in_decoder_factory(self):
        # Shows that the decoder has been registered on import of something from mantidqt.widget.instrumentview
        found_decoder = DecoderFactory.find_decoder("InstrumentView")
        self.assertIs(InstrumentViewDecoder, found_decoder.__class__)

    def test_decoder_decode_function_returns_none_when_obj_is_none(self):
        self.assertIs(None, self.decoder.decode(None))

    def test_nothing_is_thrown_when_decoding(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.decoder.decode(INSTRUMENT_VIEW_DICT)
