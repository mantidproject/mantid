# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from unittest.mock import Mock, MagicMock
from mantidqt.utils.testing.strict_mock import StrictMock

try:
    from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModelType

    class MockQTableViewModel:
        def __init__(self):
            self.type = MatrixWorkspaceTableViewModelType.x
            self.createIndex = Mock()
            self.data = Mock()

except ImportError:
    pass


class MockQTableHeader(object):
    def __init__(self):
        self.addAction = Mock()


class MockQSelection:
    def __init__(self):
        self.mock_item_range = MockQItemRange()
        self.first = Mock(return_value=self.mock_item_range)


class MockQItemRange(object):
    def __init__(self):
        self.top = Mock(return_value=0)
        self.bottom = Mock(return_value=2)
        self.left = Mock(return_value=0)
        self.right = Mock(return_value=2)


class MockQSelectionModel:
    def __init__(self, has_selection=True):
        self.hasSelection = Mock(return_value=has_selection)
        self.selectedRows = None
        self.selectedColumns = None
        self.currentIndex = None
        self.mock_selection = MockQSelection()
        self.selection = Mock(return_value=self.mock_selection)


class MockQTableView:
    def __init__(self):
        self.setContextMenuPolicy = Mock()
        self.addAction = Mock()
        self.mock_horizontalHeader = MockQTableHeader()
        self.mock_verticalHeader = MockQTableHeader()
        self.horizontalHeader = Mock(return_value=self.mock_horizontalHeader)
        self.verticalHeader = Mock(return_value=self.mock_verticalHeader)
        self.setModel = Mock()
        self.mock_model = MockQTableViewModel()

        self.model = Mock(return_value=self.mock_model)

        self.mock_selection_model = MockQSelectionModel()

        self.selectionModel = Mock(return_value=self.mock_selection_model)


class MockViewport:
    def __init__(self):
        self.update = Mock()


class MockQTab:
    def __init__(self):
        self.mock_viewport = MockViewport()
        self.viewport = Mock(return_value=self.mock_viewport)


class MockQModelIndexSibling:
    TEST_SIBLING_DATA = "MANTID_TEST_SIBLING_DATA"

    def __init__(self):
        self.data = Mock(return_value=self.TEST_SIBLING_DATA)


class MockQModelIndex:
    def __init__(self, row, column):
        self.row = Mock(return_value=row)
        self.column = Mock(return_value=column)
        self.mock_sibling = MockQModelIndexSibling()
        self.sibling = Mock(return_value=self.mock_sibling)


class MockQtEvent:
    def __init__(self):
        self.accept = Mock()


class MockQtSignal:
    def __init__(self):
        self.emit = Mock()


class MockQStatusBar(object):
    def __init__(self):
        self.showMessage = Mock()


class MockQClipboard(object):
    def __init__(self):
        self.setText = Mock()
        self.Clipboard = 3


class MockQButton(object):
    def __init__(self):
        self.mock_clicked_signal = MockQtSignal()
        self.clicked = Mock(return_value=self.mock_clicked_signal)
        self.setEnabled = MagicMock()


class MockQWidget(object):
    def __init__(self):
        self.addWidget = StrictMock()
        self.replaceWidget = StrictMock()
        self.widget = StrictMock()
        self.hide = StrictMock()
        self.show = StrictMock()
        self.close = StrictMock()
        self.exec_ = StrictMock()
