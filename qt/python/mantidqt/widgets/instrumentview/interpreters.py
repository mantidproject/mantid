# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from mantidqt.widgets.instrumentview.interpreterimports import InstrumentWidgetEncoder, InstrumentWidgetDecoder
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantid.api import AnalysisDataService as ADS


class InstrumentViewAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    tags = ["InstrumentView", "InstrumentWidget"]


class Decoder(InstrumentViewAttributes):
    def __init__(self):
        super(Decoder, self).__init__()
        self.widget_decoder = InstrumentWidgetDecoder()

    def decode(self, obj_dic, project_path=None):
        if obj_dic is None or project_path is None:
            return None
        # Make the widget
        ws = ADS.retrieve(obj_dic["workspaceName"])
        instrument_view_presenter = InstrumentViewPresenter(ws)
        instrument_widget = instrument_view_presenter.view.layout().itemAt(0).widget()

        #  Then decode
        self.widget_decoder.decode(obj_dic, instrument_widget, project_path)

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags


class Encoder(InstrumentViewAttributes):
    def __init__(self):
        super(Encoder, self).__init__()
        self.widget_encoder = InstrumentWidgetEncoder()

    def encode(self, obj, project_path=None):
        if obj is None or project_path is None:
            return None
        instrument_widget = obj.layout().itemAt(0).widget()
        encoded_instrumentview = self.widget_encoder.encode(instrument_widget, project_path)

        # A color is passed to python as a QColor for certain objects and it needs to be fixed
        r, g, b, a = encoded_instrumentview["surface"]["backgroundColor"].getRgb()
        encoded_instrumentview["surface"]["backgroundColor"] = {"red": r, "green": g, "blue": b, "alpha": a}

        return encoded_instrumentview

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags
