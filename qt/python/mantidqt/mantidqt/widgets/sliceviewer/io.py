# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from mantid.api import AnalysisDataService as ADS


class SliceViewerAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    _tags = ["SliceViewerView"]


class SliceViewerEncoder(SliceViewerAttributes):
    @staticmethod
    def encode(obj, _=None):
        return {"workspace": obj.presenter.model.ws.name()}

    @classmethod
    def tags(cls):
        return cls._tags


class SliceViewerDecoder(SliceViewerAttributes):
    @staticmethod
    def decode(obj_dic, _=None):
        # Imported here so that importing the sliceviewer package does not import the whole viewer
        from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
        from workbench.config import CONF

        return SliceViewer(ws=ADS.retrieve(obj_dic["workspace"]), conf=CONF).view

    @classmethod
    def tags(cls):
        return cls._tags
