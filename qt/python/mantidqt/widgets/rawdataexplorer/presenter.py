# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import os
import re

from .model import RawDataExplorerModel
from .view import RawDataExplorerView


class RawDataExplorerPresenter():
    """
    TODO
    """
    def __init__(self, parent=None, view=None, model=None):
        """
        Initialise the presenter, creating the view and model, and
        setting the initial plot list
        :param global_figure_manager: The GlobalFigureManager class
        :param view: Optional - a view to use instead of letting the
                     class create one (intended for testing)
        :param model: Optional - a model to use instead of letting
                      the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        if view is None:
            self.view = RawDataExplorerView(parent)
        else:
            self.view = view
        if model is None:
            self.model = RawDataExplorerModel(self)
        else:
            self.model = model

