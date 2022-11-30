# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS

from mantid.dataobjects import Workspace2D


class SliceViewerBaseModel:
    def __init__(self, ws: Workspace2D):
        self._ws = ws

    @property
    def ws(self):
        return self._ws

    @ws.setter
    def ws(self, val):
        self._ws = val
