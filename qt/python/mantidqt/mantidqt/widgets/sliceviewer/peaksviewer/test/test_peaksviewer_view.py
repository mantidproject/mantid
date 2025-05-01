# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest

# 3rdparty imports
from mantidqt.widgets.sliceviewer.peaksviewer.model import PeaksViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer.view import PeaksViewerView
from mantidqt.widgets.sliceviewer.peaksviewer.presenter import PeaksViewerPresenter, PeaksWorkspaceDataPresenter
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtCore import Qt
from testhelpers import WorkspaceCreationHelper


@start_qapplication
class PeaksViewerViewTest(unittest.TestCase):
    def test_sort_by_value_not_string(self):
        npeaks = 5
        peaks_ws = WorkspaceCreationHelper.createPeaksWorkspace(npeaks)
        PeaksViewerView.frame = None  # override to avoid call to SliceViewer
        view = PeaksViewerView(None, None)
        model = PeaksViewerModel(peaks_ws, fg_color="r", bg_color="w")
        presenter = PeaksViewerPresenter(model, view)
        table_view = view.table_view
        table_model = table_view.model()

        # Very difficult to simulate mouse click to sort - trust Qt to do that
        # We are more interested that the sort is based on value no a string comparison
        # check a few columns
        for column_index in (7, 16):  # tof & qlab
            table_model.sort(column_index, Qt.DescendingOrder)

            self.assertEqual(npeaks, view.table_view.rowCount())
            # assert sort has happened
            col_values = [table_model.index(i, column_index).data(PeaksWorkspaceDataPresenter.DATA_SORT_ROLE) for i in range(npeaks)]
            self.assertTrue(
                all(col_values[i + 1] < col_values[i] for i in range(npeaks - 1)),
                msg="TOF values have not been sorted into descending order",
            )

        view.close()
        del presenter


if __name__ == "__main__":
    unittest.main()
