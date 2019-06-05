.. _MultipleViews:

==============
Multiple Views
==============


It is possible to have an MVP pattern within other MVP patterns. If
each complete MVP pattern is considered to be a widget then having an
MVP pattern embedded into another MVP is effectively just adding
another widget. This can be very useful for creating small versatile
widgets that may be used in multiple GUIs.

We will combine the View from the exercise and the PlotView from the
previous section into a single view. To achieve this we will create a
'master' view:

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore

    import numpy as np
    import plotView
    import view

    class MasterView(QtGui.QWidget):

        def __init__(self, parent=None):
            super(MasterView, self).__init__(parent)

            grid = QtGui.QVBoxLayout(self)
            self.plot_view = plotView.PlotView(parent=self)
            self.options_view = view.view(parent=self)

            grid.addWidget(self.plot_view)
            grid.addWidget(self.options_view)          
            self.setLayout(grid)

The important thing to note here is that when the PlotView and View
are created the parent is set to be the masterView.

The main only needs to import the masterView:

.. code-block:: python

    class demo(QtGui.QMainWindow):
        def __init__(self, parent=None):
            super(demo, self).__init__(parent)

            self.window = QtGui.QMainWindow()
            my_view = masterView.MasterView()

            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

You may notice that this main does not incorporate our Presenter.
Now that we have embedded our two views into masterView, Presenter
should also be split in the same way.