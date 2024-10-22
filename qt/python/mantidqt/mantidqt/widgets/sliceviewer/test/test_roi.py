import unittest
import numpy as np
from mantidqt.widgets.sliceviewer.models.roi import _adjust_xmin_xmax_for_event_workspace
from mantid.simpleapi import CreateSampleWorkspace


class TestROIDefs(unittest.TestCase):
    def setUp(self) -> None:
        np.random.seed(13)

    def test_adjust_xmin_xmax_for_event_workspace(self):
        event_ws = CreateSampleWorkspace(WorkspaceType="Event")
        ws_index_min = np.random.randint(event_ws.getNumberHistograms())
        xmin = xmax = np.random.uniform(event_ws.getTofMin(), event_ws.getTofMax())
        new_xmin, new_xmax = _adjust_xmin_xmax_for_event_workspace(event_ws, xmin, xmax, ws_index_min)
        self.assertEqual(new_xmin, event_ws.readX(ws_index_min)[event_ws.yIndexOfX(xmin)])
        self.assertEqual(new_xmax, event_ws.readX(ws_index_min)[event_ws.yIndexOfX(xmin) + 1])

    def test_adjust_xmin_xmax_for_event_workspace_for_unequal_xvalues(self):
        event_ws = CreateSampleWorkspace(WorkspaceType="Event")
        ws_index_min = np.random.randint(event_ws.getNumberHistograms())
        xmin = np.random.uniform(event_ws.getTofMin(), event_ws.getTofMax() - 1)
        xmax = np.random.uniform(xmin + 1, event_ws.getTofMax())
        new_xmin, new_xmax = _adjust_xmin_xmax_for_event_workspace(event_ws, xmin, xmax, ws_index_min)
        self.assertEqual(new_xmin, xmin)
        self.assertEqual(new_xmax, xmax)

    def test_no_adjust_to_xmin_xmax_for_non_event_workspace(self):
        ws = CreateSampleWorkspace()
        ws_index_min = np.random.randint(ws.getNumberHistograms())
        xmin = np.random.uniform(ws.readX(ws_index_min)[0], ws.readX(ws_index_min)[-1] - 1)
        xmax = np.random.uniform(xmin + 1, ws.readX(ws_index_min)[-1])
        new_xmin, new_xmax = _adjust_xmin_xmax_for_event_workspace(ws, xmin, xmax, ws_index_min)
        self.assertEqual(new_xmin, xmin)
        self.assertEqual(new_xmax, xmax)

    def test_no_adjust_to_xmin_xmax_for_non_event_workspace_equal_xvalues(self):
        ws = CreateSampleWorkspace()
        ws_index_min = np.random.randint(ws.getNumberHistograms())
        xmin = xmax = np.random.uniform(ws.readX(ws_index_min)[0], ws.readX(ws_index_min)[-1])
        new_xmin, new_xmax = _adjust_xmin_xmax_for_event_workspace(ws, xmin, xmax, ws_index_min)
        self.assertEqual(new_xmin, xmin)
        self.assertEqual(new_xmax, xmax)


if __name__ == "__main__":
    unittest.main()
