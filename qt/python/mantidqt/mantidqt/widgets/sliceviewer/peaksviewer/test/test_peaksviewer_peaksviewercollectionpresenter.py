# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import MagicMock, create_autospec, patch

# 3rdparty imports
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace, RenameWorkspace
from mantid.dataobjects import PeaksWorkspace
from mantidqt.widgets.sliceviewer.models.adsobsever import SliceViewerADSObserver
from mantidqt.widgets.sliceviewer.peaksviewer import PeaksViewerPresenter, PeaksViewerCollectionPresenter, PeaksViewerModel


class FakePeaksModelFactory(MagicMock):
    def __call__(self, name, fg, bg):
        super().__call__(name, fg, bg)
        return PeaksViewerModel(create_mock_peaks_workspace(name), fg, bg)


def create_mock_peaks_workspace(name="peaks"):
    """Create a mock object that looks like a PeaksWorkspace"""
    mock = create_autospec(PeaksWorkspace, instance=True)
    mock.name.return_value = name
    return mock


@patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.create_peaksviewermodel", new_callable=FakePeaksModelFactory)
class PeaksViewerCollectionPresenterTest(unittest.TestCase):
    # -------------------- success tests -----------------------------

    def test_append_constructs_PeaksViewerPresenter_for_call(self, mock_create_model):
        # mock 2 models with the correct names and assume  create_peaksviewermodel makes them
        names = ["peaks1", "peaks2"]
        presenter = PeaksViewerCollectionPresenter(MagicMock())

        for name in names:
            presenter.append_peaksworkspace(name)

        self.assertEqual(names, presenter.workspace_names())

    def test_append_uses_unused_fg_color(self, mock_create_model):
        names = ["peaks1", "peaks2"]
        # setup with 1 workspace
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.append_peaksworkspace(names[0])

        mock_create_model.assert_called_once_with(
            names[0], PeaksViewerCollectionPresenter.FG_COLORS[0], PeaksViewerCollectionPresenter.DEFAULT_BG_COLOR
        )
        mock_create_model.reset_mock()
        presenter.append_peaksworkspace(names[1])
        mock_create_model.assert_called_once_with(
            names[1], PeaksViewerCollectionPresenter.FG_COLORS[1], PeaksViewerCollectionPresenter.DEFAULT_BG_COLOR
        )

    def test_remove_removes_named_workspace(self, mock_create_model):
        names = ["peaks1", "peaks2", "peaks3"]
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        for name in names:
            presenter.append_peaksworkspace(name)

        presenter.remove_peaksworkspace(names[1])

        presenter._actions_view.set_peaksworkspace.assert_called_with([names[0], names[2]])
        self.assertEqual([names[0], names[2]], presenter.workspace_names())
        presenter._view.hide.assert_not_called()

    def test_hides_view_when_last_named_workspace_removed(self, mock_create_model):
        names = ["peaks1"]
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.append_peaksworkspace(names[0])

        presenter.remove_peaksworkspace(names[0])

        presenter._actions_view.set_peaksworkspace.assert_called_with([])
        presenter._view.hide.assert_called()

    def test_notify_called_for_each_subpresenter(self, mock_create_model):
        presenter = PeaksViewerCollectionPresenter(MagicMock())

        with patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter") as presenter_mock:
            presenter_mock.side_effect = [MagicMock(), MagicMock()]
            child_presenters = [presenter.append_peaksworkspace(f"peaks_{i}") for i in range(2)]
            presenter.notify(PeaksViewerPresenter.Event.OverlayPeaks)
            for child in child_presenters:
                child.notify.assert_called_once_with(PeaksViewerPresenter.Event.OverlayPeaks)

    def test_ensure_that_the_ads_observer_calls_clear_handle(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.clear_handle = MagicMock()
        self.assertTrue(isinstance(presenter._ads_observer, SliceViewerADSObserver))
        presenter._ads_observer = SliceViewerADSObserver(
            presenter.replace_handle, presenter.rename_handle, presenter.clear_handle, presenter.delete_handle
        )

        CreateSampleWorkspace(OutputWorkspace="ws")
        ADS.clear()

        presenter.clear_handle.assert_called_once()

    def test_ensure_that_the_ads_observer_calls_remove_handle(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.delete_handle = MagicMock()
        self.assertTrue(isinstance(presenter._ads_observer, SliceViewerADSObserver))
        presenter._ads_observer = SliceViewerADSObserver(
            presenter.replace_handle, presenter.rename_handle, presenter.clear_handle, presenter.delete_handle
        )

        CreateSampleWorkspace(OutputWorkspace="ws")
        ADS.remove("ws")

        presenter.delete_handle.assert_called_once_with("ws")

    def test_ensure_that_the_ads_observer_calls_replace_handle(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.replace_handle = MagicMock()
        self.assertTrue(isinstance(presenter._ads_observer, SliceViewerADSObserver))
        presenter._ads_observer = SliceViewerADSObserver(
            presenter.replace_handle, presenter.rename_handle, presenter.clear_handle, presenter.delete_handle
        )

        CreateSampleWorkspace(OutputWorkspace="ws")
        CreateSampleWorkspace(OutputWorkspace="ws")

        presenter.replace_handle.assert_called_once()

    def test_ensure_that_the_ads_observer_calls_rename_handle(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.rename_handle = MagicMock()
        self.assertTrue(isinstance(presenter._ads_observer, SliceViewerADSObserver))
        presenter._ads_observer = SliceViewerADSObserver(
            presenter.replace_handle, presenter.rename_handle, presenter.clear_handle, presenter.delete_handle
        )

        CreateSampleWorkspace(OutputWorkspace="ws")
        RenameWorkspace(InputWorkspace="ws", OutputWorkspace="ws1")

        presenter.rename_handle.assert_called_once_with("ws", "ws1")

    def test_ensure_replace_handle_removes_and_re_adds_workspaces(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.overlay_peaksworkspaces = MagicMock()
        presenter.workspace_names = MagicMock(return_value=["ws"])

        def remove_peaks(ws_name):
            presenter.workspace_names().remove(ws_name)
            return 1

        presenter.remove_peaksworkspace = MagicMock(side_effect=remove_peaks)

        presenter.replace_handle("ws", None)

        presenter.remove_peaksworkspace.assert_called_once_with("ws")
        presenter.overlay_peaksworkspaces.assert_called_once_with(["ws"], index=1)

    def test_ensure_delete_handle_removes_workspace(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.remove_peaksworkspace = MagicMock()
        presenter.workspace_names = MagicMock(return_value=["ws"])

        presenter.delete_handle("ws")

        presenter.remove_peaksworkspace.assert_called_once_with("ws")

    def test_ensure_clear_handle_removes_all_workspaces(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.remove_peaksworkspace = MagicMock()
        presenter.workspace_names = MagicMock(return_value=["ws", "ws1", "ws2"])

        presenter.clear_handle()

        presenter.remove_peaksworkspace.assert_any_call("ws")
        presenter.remove_peaksworkspace.assert_any_call("ws1")
        presenter.remove_peaksworkspace.assert_any_call("ws2")
        self.assertEqual(3, presenter.remove_peaksworkspace.call_count)

    def test_ensure_rename_handle_removes_and_re_adds_the_new_workspace_name(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.workspace_names = MagicMock(return_value=["ws"])

        def remove_peaks(ws_name):
            presenter.workspace_names().remove(ws_name)
            return 7

        presenter.remove_peaksworkspace = MagicMock(side_effect=remove_peaks)
        presenter.overlay_peaksworkspaces = MagicMock()

        presenter.rename_handle("ws", "ws1")

        presenter.remove_peaksworkspace.assert_called_once_with("ws")
        presenter.overlay_peaksworkspaces.assert_called_once_with(["ws1"], index=7)

    def test_ensure_clear_observer_sets_observer_to_none(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter._ads_observer = MagicMock()

        presenter.clear_observer()

        self.assertEqual(presenter._ads_observer, None)

    def test_adding_peak(self, _):
        view = MagicMock()
        presenter = PeaksViewerCollectionPresenter(view)

        # test deactivating peak adding mode from sliceviewer
        presenter.deactivate_peak_add_delete()
        view.peak_actions_view.deactivate_peak_adding.assert_called_once()

        # test adding peak to peaksworkspace
        presenter.child_presenter = MagicMock()
        presenter.add_delete_peak([1, 2, 3])
        presenter.child_presenter().add_peak.assert_called_once_with([1, 2, 3])

        # test deactivate_zoom_pan
        presenter.deactivate_zoom_pan(False)
        view.deactivate_zoom_pan.assert_not_called()
        presenter.deactivate_zoom_pan(True)
        view.deactivate_zoom_pan.assert_called_once()

    def test_deleting_peaks_workspace_disables_add_remove_peak(self, _):
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        self.assertEqual(0, len(presenter._child_presenters))
        name_peaks_ws = "peaks"
        presenter.append_peaksworkspace(name_peaks_ws)
        self.assertEqual(1, len(presenter._child_presenters))
        presenter.remove_peaksworkspace(name_peaks_ws)
        self.assertEqual(0, len(presenter._child_presenters))
        # With no child presenters, any attempt to add or delete a peak
        # will throw an exception since those operations act on the child
        # presenter. Hence if nothing happens here it's working as expected.
        presenter.add_delete_peak([1, 2, 3])


if __name__ == "__main__":
    unittest.main()
