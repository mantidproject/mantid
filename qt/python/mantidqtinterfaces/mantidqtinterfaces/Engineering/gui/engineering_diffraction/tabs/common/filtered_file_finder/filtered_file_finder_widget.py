# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections.abc import Callable, Iterable
from fnmatch import fnmatch
from os.path import splitext
from typing import Any

from qtpy import QtWidgets, QtCore
from qtpy.QtWidgets import QSizePolicy

from mantidqt.widgets.filefinderwidget import FileFinderWidget


class FileFilterProxyModel(QtCore.QSortFilterProxyModel):
    text_filter: str | None = None

    def __init__(self, parent=None):
        super().__init__(parent)

    def filterAcceptsRow(self, source_row, source_parent):
        if self.text_filter is None:
            return True
        model = self.sourceModel()
        index0 = model.index(source_row, 0, source_parent)
        fname = model.fileName(index0)
        fname = splitext(fname)[0]

        return model.isDir(index0) or fnmatch(fname, self.text_filter)

    def sort(self, column, order):
        self.sourceModel().sort(column, order)


class FilteredFileFinderWidget(QtWidgets.QWidget):
    """
    Wrapper for FileFinderWidget with a built-in filters
    """

    filters: dict[str, QtWidgets.QComboBox]
    filter_generator: Callable[[dict[str, str]], str]

    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)
        self.setLayout(QtWidgets.QVBoxLayout())

        self.finder = FileFinderWidget()
        self.layout().addWidget(self.finder)

        self.filter_row = QtWidgets.QWidget()
        self.layout().addWidget(self.filter_row)
        self.filter_row.setLayout(QtWidgets.QHBoxLayout())
        self.filter_label = QtWidgets.QLabel("Browse filters:")
        self.filter_row.layout().addWidget(self.filter_label)
        self.filter_row.layout().addSpacerItem(QtWidgets.QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Minimum))

        self.finder.setUseNativeWidget(False)
        self.proxy_model = FileFilterProxyModel()
        self.finder.setProxyModel(self.proxy_model)
        self.filters = {}
        self.filter_generator = lambda _: "*"

    def __getattr__(self, attr: str) -> Any:
        """
        Pass any undefined attributes and methods to the FileFinderWidget
        """
        if attr not in self.__dict__:
            return getattr(self.finder, attr)
        return super().__getattr__(attr)

    def add_filter(self, name: str, options: Iterable[str]) -> None:
        """
        Add a QComboBox dropdown with the list of options
        """
        new_filter = QtWidgets.QComboBox()
        new_filter.addItems(list(options))
        new_filter.setToolTip(f"{name} Filter")
        new_filter.currentIndexChanged.connect(self._handle_filter_change)
        self.filters[name] = new_filter
        self.filter_row.layout().addWidget(new_filter)

    def set_filter_generator(self, filter_generator: Callable[[dict[str, str]], str]) -> None:
        """
        Set a function that converts the selected options into a search string
        """
        self.filter_generator = filter_generator
        self._handle_filter_change()

    def set_filter_option(self, filter_name: str, option_name: str) -> None:
        """
        Set a given filter to a given option
        """
        filter_widget = self.filters[filter_name]
        index = filter_widget.findText(option_name)
        filter_widget.setCurrentIndex(index)

    def _handle_filter_change(self) -> None:
        filter_options = {name: widget.currentText() for name, widget in self.filters.items()}
        text_filter = self.filter_generator(filter_options)
        self.proxy_model.text_filter = text_filter
