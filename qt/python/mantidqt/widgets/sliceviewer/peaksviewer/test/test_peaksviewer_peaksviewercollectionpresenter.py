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
from mantid.dataobjects import PeaksWorkspace
from mantidqt.widgets.sliceviewer.peaksviewer \
    import PeaksViewerPresenter, PeaksViewerCollectionPresenter, PeaksViewerModel


class FakePeaksModelFactory(MagicMock):
    def __call__(self, name, fg, bg):
        super().__call__(name, fg, bg)
        return PeaksViewerModel(create_mock_peaks_workspace(name), fg, bg)


def create_mock_peaks_workspace(name="peaks"):
    """Create a mock object that looks like a PeaksWorkspace"""
    mock = create_autospec(PeaksWorkspace)
    mock.name.return_value = name
    return mock


@patch(
    "mantidqt.widgets.sliceviewer.peaksviewer.presenter.create_peaksviewermodel",
    new_callable=FakePeaksModelFactory)
class PeaksViewerCollectionPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------

    def test_append_constructs_PeaksViewerPresenter_for_call(self, mock_create_model):
        # mock 2 models with the correct names and assume  create_peaksviewermodel makes them
        names = ["peaks1", 'peaks2']
        presenter = PeaksViewerCollectionPresenter(MagicMock())

        for name in names:
            presenter.append_peaksworkspace(name)

        self.assertEqual(names, presenter.workspace_names())

    def test_append_uses_unused_fg_color(self, mock_create_model):
        names = ["peaks1", 'peaks2']
        # setup with 1 workspace
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        presenter.append_peaksworkspace(names[0])

        mock_create_model.assert_called_once_with(names[0],
                                                  PeaksViewerCollectionPresenter.FG_COLORS[0],
                                                  PeaksViewerCollectionPresenter.DEFAULT_BG_COLOR)
        mock_create_model.reset_mock()
        presenter.append_peaksworkspace(names[1])
        mock_create_model.assert_called_once_with(names[1],
                                                  PeaksViewerCollectionPresenter.FG_COLORS[1],
                                                  PeaksViewerCollectionPresenter.DEFAULT_BG_COLOR)

    def test_remove_removes_named_workspace(self, mock_create_model):
        names = ["peaks1", 'peaks2', 'peaks3']
        presenter = PeaksViewerCollectionPresenter(MagicMock())
        for name in names:
            presenter.append_peaksworkspace(name)

        presenter.remove_peaksworkspace(names[1])

        self.assertEqual([names[0], names[2]], presenter.workspace_names())

    def test_notify_called_for_each_subpresenter(self, mock_create_model):
        presenter = PeaksViewerCollectionPresenter(MagicMock())

        with patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter"
                   ) as presenter_mock:
            presenter_mock.side_effect = [MagicMock(), MagicMock()]
            child_presenters = [presenter.append_peaksworkspace(f'peaks_{i}') for i in range(2)]
            presenter.notify(PeaksViewerPresenter.Event.OverlayPeaks)
            for child in child_presenters:
                child.notify.assert_called_once_with(PeaksViewerPresenter.Event.OverlayPeaks)


if __name__ == '__main__':
    unittest.main()
