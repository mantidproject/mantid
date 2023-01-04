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
from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.widgets.instrumentview.io import InstrumentViewEncoder, InstrumentViewDecoder
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.testing import start_qapplication


INSTRUMENT_VIEW_DICT = {
    "workspaceName": "ws",
    "tabs": {
        "maskTab": {
            "activeType": {"roiOn": False, "groupingOn": False, "maskingOn": True},
            "activeTools": {
                "ellipseButton": False,
                "moveButton": True,
                "pointerButton": False,
                "ringRectangleButton": False,
                "freeDrawButton": False,
                "ringEllipseButton": False,
                "tubeButton": False,
                "pixelButton": False,
            },
            "maskWorkspaceSaved": False,
        },
        "renderTab": {
            "displayWireframe": False,
            "displayLighting": False,
            "labelPrecision": 1,
            "useUCorrection": False,
            "autoScaling": False,
            "colorBar": {"max": "40", "scaleType": 0, "power": "2", "min": "40"},
            "showLabels": True,
            "flipView": False,
            "displayDetectorsOnly": True,
            "displayAxes": False,
            "axesView": 0,
            "showRows": True,
            "useOpenGL": True,
            "showRelativeIntensity": False,
            "freezeRotation": False,
        },
        "treeTab": {"expandedItems": []},
        "pickTab": {
            "freeDraw": False,
            "ringEllipse": False,
            "edit": False,
            "tube": False,
            "peakErase": False,
            "zoom": False,
            "one": True,
            "ringRectangle": False,
            "peakAdd": False,
            "ellipse": False,
            "rectangle": False,
        },
    },
    "surfaceType": 0,
    "actor": {"binMasks": [], "fileName": "viridis", "highlightZeroCounts": False},
    "energyTransfer": [0.0, 99.0],
    "surface": {
        "shapes": [],
        "alignmentInfo": [],
        "backgroundColor": {"blue": 0, "alpha": 255, "green": 0, "red": 0},
        "projection3D": {"viewport": {"rotation": [1.0, 0.0, 0.0, 0.0], "translation": {"xTrans": 0.0, "yTrans": 0.0}, "zoom": 1.0}},
        "projection3DSuccess": True,
    },
    "currentTab": 0,
}


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
