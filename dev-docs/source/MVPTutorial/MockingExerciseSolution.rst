.. _MockingExerciseSolution:

=========================
Mocking Exercise Solution
=========================

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import sys
    import presenter
    import view

    import unittest
    from mantid.py3compat import mock

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
            assert(self.view.getColour.call_count == 1)
            assert(self.view.getGridLines.call_count == 1)
            assert(self.view.getFreq.call_count == 1)
            assert(self.view.getPhase.call_count == 1)

    if __name__ == "__main__":
        unittest.main()
