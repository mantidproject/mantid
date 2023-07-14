# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantid.api import AnalysisDataService as ADS


class MatrixWorkspaceDisplayAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    _tags = ["MatrixWorkspaceDisplayView"]


class MatrixWorkspaceDisplayEncoder(MatrixWorkspaceDisplayAttributes):
    def __init__(self):
        super(MatrixWorkspaceDisplayEncoder, self).__init__()

    @staticmethod
    def encode(obj, _=None):
        return {"workspace": obj.presenter.model._ws.name()}

    @classmethod
    def tags(cls):
        return cls._tags


class MatrixWorkspaceDisplayDecoder(MatrixWorkspaceDisplayAttributes):
    def __init__(self):
        super(MatrixWorkspaceDisplayDecoder, self).__init__()

    @staticmethod
    def decode(obj_dic, _=None):
        import matplotlib.pyplot as plt

        pres = MatrixWorkspaceDisplay(ADS.retrieve(obj_dic["workspace"]), plot=plt)
        return pres.container

    @classmethod
    def tags(cls):
        return cls._tags
