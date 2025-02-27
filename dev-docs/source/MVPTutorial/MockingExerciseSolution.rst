.. _MockingExerciseSolution:

=========================
Mocking Exercise Solution
=========================

.. code-block:: python

    import sys
    from presenter import Presenter
    from view import View

    import unittest
    from unittest import mock

    class PresenterTest(unittest.TestCase):
        def setUp(self):
            self._view = mock.create_autospec(View, instance=True)

            # mock view
            self._view.get_colour = mock.Mock(return_value="black")
            self._view.get_grid_lines =mock.Mock(return_value=True)
            self._view.get_freq =mock.Mock(return_value=3.14)
            self._view.get_phase =mock.Mock(return_value=0.56)
            self._view._button_clicked = mock.Mock()
            self._view._set_table_row = mock.Mock()
            self._view._add_widget_to_table = mock.Mock()
            self._view._add_item_to_table = mock.Mock()

            self._presenter = Presenter(self._view)

        def test_handle_update_plot(self):
            self._presenter.handle_update_plot()
            self._view.get_colour.assert_called_once()
            self._view.get_grid_lines.assert_called_once()
            self._view.get_freq.assert_called_once()
            self._view.get_phase.assert_called_once()

    if __name__ == "__main__":
        unittest.main()
