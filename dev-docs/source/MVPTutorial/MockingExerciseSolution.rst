.. _MockingExerciseSolution:

=========================
Mocking Exercise Solution
=========================

.. code-block:: python

    import sys
    import presenter
    import view

    import unittest
    from unittest import mock

    class PresenterTest(unittest.TestCase):
        def setUp(self):
            self.view = mock.create_autospec(view.View)

            # mock view
            self.view.plotSignal = mock.Mock()
            self.view.getColour = mock.Mock(return_value="black")
            self.view.getGridLines =mock.Mock(return_value=True)
            self.view.getFreq =mock.Mock(return_value=3.14)
            self.view.getPhase =mock.Mock(return_value=0.56)
            self.view.buttonPressed = mock.Mock()
            self.view.setTableRow = mock.Mock()
            self.view.addWidgetToTable = mock.Mock()
            self.view.addITemToTable = mock.Mock()

            self.presenter = presenter.Presenter(self.view)

        def test_updatePlot(self):
            self.presenter.updatePlot()
            self.view.getColour.assert_called_once()
            self.view.getGridLines.assert_called_once()
            self.view.getFreq.assert_called_once()
            self.view.getPhase.assert_called_once()

    if __name__ == "__main__":
        unittest.main()
