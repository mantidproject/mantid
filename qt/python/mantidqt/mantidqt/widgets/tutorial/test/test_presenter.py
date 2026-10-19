# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from unittest.mock import MagicMock, patch

from mantidqt.widgets.tutorial.presenter import TutorialPresenter

file_path = "mantidqt.widgets.tutorial.presenter"


class FakePresenter(TutorialPresenter):
    TUTORIAL_SETTINGS_KEY = "FakeInterface"

    def __init__(self, view, offer_tutorial=True):
        self.setup_tutorial(view, offer_tutorial)

    def tutorial_chapters(self):
        return ()

    def make_tutorial_sandbox_factory(self):
        return MagicMock()


@patch(file_path + ".run_tutorial")
class TutorialPresenterTest(unittest.TestCase):
    """The tutorial builds a whole second interface, so every test here stubs ``run_tutorial`` out.
    What is under test is when the presenter asks for one, not what the tour then does."""

    def test_opening_it_from_the_toolbar_does_not_mark_it_as_already_seen(self, mock_run_tutorial):
        # asking for the tutorial should not change whether it appears on startup. The slot the
        # button gets takes no arguments, so ``clicked(bool)`` cannot be what decides that
        view = MagicMock()
        with patch(file_path + ".should_show_on_startup", return_value=False):
            FakePresenter(view)

        view.set_on_tutorial_clicked.call_args.args[0]()

        self.assertFalse(mock_run_tutorial.call_args.kwargs["mark_as_seen"])

    def test_a_second_request_raises_the_running_tutorial_rather_than_starting_another(self, mock_run_tutorial):
        presenter = FakePresenter(MagicMock(), offer_tutorial=False)
        presenter.open_tutorial()

        presenter.open_tutorial()

        mock_run_tutorial.assert_called_once()
        # the shell is the window; the sandbox interface inside it is only a child widget
        mock_run_tutorial.return_value.shell.raise_.assert_called_once_with()

    def test_closing_the_interface_closes_a_tutorial_it_started(self, mock_run_tutorial):
        # the tour drives a second interface, which would otherwise be left with nothing to return to
        presenter = FakePresenter(MagicMock(), offer_tutorial=False)
        presenter.open_tutorial()

        presenter.close_tutorial()

        mock_run_tutorial.return_value.close.assert_called_once_with()

    @patch(file_path + ".should_show_on_startup", return_value=True)
    def test_closing_the_interface_before_the_offer_arrives_cancels_it(self, mock_should_show, mock_run_tutorial):
        # the first-open offer is deferred to the event loop, so it can arrive after the interface
        # has been closed - at which point there is no window left to run a tour over
        presenter = FakePresenter(MagicMock())
        presenter.close_tutorial()

        presenter.open_tutorial(mark_as_seen=True)  # what the deferred timer would do

        mock_run_tutorial.assert_not_called()

    @patch(file_path + ".should_show_on_startup", return_value=False)
    def test_it_is_not_offered_once_it_has_been_seen(self, mock_should_show, mock_run_tutorial):
        FakePresenter(MagicMock())

        mock_run_tutorial.assert_not_called()

    @patch(file_path + ".should_show_on_startup", return_value=True)
    def test_it_is_not_offered_when_the_interface_is_the_tutorials_own(self, mock_should_show, mock_run_tutorial):
        # a tutorial inside a tutorial, recursively, on a first ever open
        view = MagicMock()

        FakePresenter(view, offer_tutorial=False)

        mock_run_tutorial.assert_not_called()
        view.set_tutorial_button_visible.assert_called_once_with(False)


if __name__ == "__main__":
    unittest.main()
