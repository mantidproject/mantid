# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)


from mantid.api import AnalysisDataService as ADS
from mantidqt.utils.qt import import_qt

# local imports
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter

# Import widget from C++ wrappers
_InstrumentWidgetEncoder = import_qt('._instrumentview', 'mantidqt.widgets.instrumentview',
                                     'InstrumentWidgetEncoder')
_InstrumentWidgetDecoder = import_qt('._instrumentview', 'mantidqt.widgets.instrumentview',
                                     'InstrumentWidgetDecoder')


class InstrumentViewAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    tags = ["InstrumentView", "InstrumentWidget"]


class InstrumentViewDecoder(InstrumentViewAttributes):
    def __init__(self):
        super(InstrumentViewDecoder, self).__init__()
        self.widget_decoder = _InstrumentWidgetDecoder()

    def decode(self, obj_dic, project_path=None):
        """
        Decode a InstrumentView Dictionary from project Save and return the object created
        :param obj_dic: Dict; A dictionary containing the information for an InstrumentView
        :param project_path: String; The location of the project save location
        :return: InstrumentView's View; The View object with correct state is returned.
        """
        load_mask = True

        if obj_dic is None:
            return None

        if project_path is None:
            project_path = ""
            load_mask = False

        # Make the widget
        ws = ADS.retrieve(obj_dic["workspaceName"])
        instrument_view = InstrumentViewPresenter(ws).container
        instrument_widget = instrument_view.widget

        #  Then 'decode' set the values from the dictionary
        self.widget_decoder.decode(obj_dic, instrument_widget, project_path, load_mask)

        # Show the end result
        return instrument_view

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags


class InstrumentViewEncoder(InstrumentViewAttributes):
    def __init__(self):
        super(InstrumentViewEncoder, self).__init__()
        self.widget_encoder = _InstrumentWidgetEncoder()

    def encode(self, obj, project_path=None):
        """
        Encode a InstrumentView object and return a dictionary containing it's state
        :param obj: InstrumentView; The window object
        :param project_path: String; The path to where the project is being saved
        :return: Dict; Containing the details of the instrument view
        """
        save_mask = True

        if obj is None:
            return None

        if project_path is None:
            project_path = ""
            save_mask = False

        instrument_widget = obj.layout().itemAt(0).widget()
        encoded_instrumentview = self.widget_encoder.encode(instrument_widget, project_path, save_mask)

        # A color is passed to python as a QColor for certain objects and it needs to be fixed
        r, g, b, a = encoded_instrumentview["surface"]["backgroundColor"].getRgb()
        encoded_instrumentview["surface"]["backgroundColor"] = {"red": r, "green": g, "blue": b, "alpha": a}

        return encoded_instrumentview

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags
