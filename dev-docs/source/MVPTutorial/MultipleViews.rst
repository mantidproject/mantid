.. _MultipleViews:

==============
Multiple Views
==============


It is possible to have an MVP pattern within other MVP patterns. If
each complete MVP pattern is considered to be a widget then having an
MVP pattern embedded into another MVP is effectively just adding
another widget. This can be very useful for creating small versatile
widgets that may be used in multiple GUIs.

We will combine the View widget from the exercise and the PlotView widget from the
previous section into a single view. To achieve this we will create a
'main' view:

.. code-block:: python

    from qtpy import QtWidgets, QtCore, QtGui

    import numpy as np
    import plot_view
    import view

    class MainView(QtWidgets.QWidget):

        def __init__(self, parent=None):
            super().__init__(parent)

            grid = QtWidgets.QVBoxLayout(self)
            self.plot_view = plot_view.PlotView(parent=self)
            self.options_view = view.View(parent=self)

            grid.addWidget(self.plot_view)
            grid.addWidget(self.options_view)
            self.setLayout(grid)

The important thing to note here is that when the PlotView and View
are created the parent is set to be the MainView.

The main only needs to import the main_view:

.. code-block:: python

    class Demo(QtWidgets.QMainWindow):
        def __init__(self, parent=None):
            super().__init__(parent)

            self.window = QtWidgets.QMainWindow()
            my_view = main_view.MainView()

            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

You may notice that this main does not incorporate the presenter.
Now that we have embedded our two views into MainView, Presenter
should also be split in the same way.
