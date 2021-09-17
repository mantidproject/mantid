# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from abc import ABC, abstractmethod


class TableWorkspaceDataPresenterBase(ABC):
    """
    Presenter to handle just displaying data from a table-like object.
    Useful for other widgets wishing to embed just the table display
    """

    __slots__ = ("model", "view")

    def __init__(self, model=None, view=None):
        """
        :param model: A reference to the model holding the table data
        :param view: A reference to the view that is displayed to the user
        """
        self.model = model
        self.view = view

    def refresh(self):
        """
        Fully refresh the display. Updates column headers and reloads the data
        """
        self.update_column_headers()
        self.load_data(self.view)

    @abstractmethod
    def update_column_headers(self):
        """
        Update column headers of the table
        """
        pass

    @abstractmethod
    def load_data(self, table):
        """
        Load new data into in the input table
        """
        pass
