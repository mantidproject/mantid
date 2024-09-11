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

    from qtpy.QtWidgets import QVBoxLayout, QWidget
    from typing import Union

    import numpy as np
    from plot_view import PlotView
    from view import View

    class MainView(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)
            self.setWindowTitle("view tutorial")

            grid = QVBoxLayout(self)
            self._plot_view = PlotView(parent=self)
            self._options_view = View(parent=self)

            grid.addWidget(self._plot_view)
            grid.addWidget(self._options_view)
            self.setLayout(grid)

The important thing to note here is that when the PlotView and View
are created the parent is set to be the MainView.

The main only needs to import the main_view:

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from main_view import MainView


    def _get_qapplication_instance() -> QApplication:
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)


    app = _get_qapplication_instance()
    view = MainView()
    view.show()
    app.exec_()

You may notice that this main does not incorporate the presenter.
Now that we have embedded our two views into MainView, Presenter
should also be split in the same way.
