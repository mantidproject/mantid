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
            x_data = np.linspace(0.0, 10.0, 100)
            y_data = np.sin(x_data)
            self.plot_view.addData(x_data, y_data, "b", "x")

            grid.addWidget(self.plot_view)

            self.options_view = view.view(parent=self)

            grid.addWidget(self.options_view)          
            self.setLayout(grid)

The important thing to note here is that when the PlotView and View
are created the parent is set to be the masterView.

The main only needs to import the masterView:

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import PyQt4.QtGui as QtGui 
    import PyQt4.QtCore as QtCore

    import sys

    import masterView


    """
    A wrapper class for setting the main window
    """
    class demo(QtGui.QMainWindow):
        def __init__(self, parent=None):
            super(demo, self).__init__(parent)

            self.window = QtGui.QMainWindow()
            my_view = masterView.MasterView()
            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

	def qapp():
            if QtGui.QApplication.instance():
                _app = QtGui.QApplication.instance()
	    else:
		_app = QtGui.QApplication(sys.argv)
	    return _app

    app = qapp()
    window = demo()
    window.show()
    app.exec_()

