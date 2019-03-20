# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.
from mantid.api import AnalysisDataService as ADS  # noqa
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay


class TableWorkspaceDisplayAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    tags = ["TableWorkspaceDisplayView"]


class TableWorkspaceDisplayEncoder(TableWorkspaceDisplayAttributes):
    def __init__(self):
        super(TableWorkspaceDisplayEncoder, self).__init__()

    def encode(self, obj, _=None):
        obj = obj.presenter.view
        return {"workspace": obj.presenter.model.ws.name(),
                "markedColumns": self._encode_marked_columns(obj.presenter.model.marked_columns),
                "windowName": obj.presenter.name}

    @staticmethod
    def _encode_marked_columns(marked_columns):
        as_y_err = []
        for y_err in marked_columns.as_y_err:
            as_y_err.append({"column": y_err.column, "relatedY": y_err.related_y_column,
                             "labelIndex": y_err.label_index})

        return {"as_x": marked_columns.as_x, "as_y": marked_columns.as_y, "as_y_err": as_y_err}

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags


class TableWorkspaceDisplayDecoder(TableWorkspaceDisplayAttributes):
    def __init__(self):
        super(TableWorkspaceDisplayDecoder, self).__init__()

    @staticmethod
    def decode(obj_dic, _=None):
        import matplotlib.pyplot as plt

        pres = TableWorkspaceDisplay(ADS.retrieve(obj_dic["workspace"]), name=obj_dic["windowName"], plot=plt)
        pres.model.marked_columns.as_x = obj_dic["markedColumns"]["as_x"]
        pres.model.marked_columns.as_y = obj_dic["markedColumns"]["as_y"]

        error_columns = []
        for y_err in obj_dic["markedColumns"]["as_y_err"]:
            error_columns.append(ErrorColumn(column=y_err["column"], label_index=y_err["labelIndex"],
                                             related_y_column=y_err["relatedY"]))
        pres.model.marked_columns.as_y_err = error_columns

        pres.update_column_headers()

        return pres.container

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags
