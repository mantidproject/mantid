.. _AddButton:

===============
Adding a Button
===============

In this task a GUI is created containing a single button.

The View
########

The below code creates a QWidget containing a single button. When the
button is pressed it will print a message to the terminal screen. It
should be noted that in practice this should be avoided and will be
discussed in :ref:`this section <ReceivingSignalFromView>`.

First we need to import the relevant packages, this includes PyQt.

.. code-block:: python

    from __future__ import (absolute_import,division,print_function)
    from qtpy import QtWidgets, QtCore, QtGui

We then create the View class as a QWidget. Each view will have a
parent. As a result, the view will automatically be destroyed if the
parent is destroyed (unless the view has been removed, via docking,
from the parent).

.. code-block:: python

    class view(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(view, self).__init__(parent)

Next we create a layout and add a button to it

.. code-block:: python

    grid = QtGui.QGridLayout()
    self.button = QtWidgets.QPushButton('Hi', self)
    self.button.setStyleSheet("background-color:lightgrey")

    # connect button to signal
    self.button.clicked.connect(self.btn_click)
    # add button to layout
    grid.addWidget(self.button)
    # set the layout for the view widget
    self.setLayout(grid)

The above connect statement means that when the button is pressed, the
function ``btn_click`` is called:

.. code-block:: python

    def btn_click(self):
        print("Hello world")

The Main
########

To run the GUI we need a 'main' module. Assuming that the above has
all been saved in ``view.py``, the ``main.py`` will contain:

.. code-block:: python

    from __future__ import (absolute_import,division,print_function)

    from qtpy import QtWidgets

    import sys
   
    import view

    """
    A wrapper class for setting the main window
    """
    class demo(QtWidgets.QMainWindow):
        def __init__(self,parent=None):
            super(demo,self).__init__(parent)

            self.window=QtWidgets.QMainWindow()
            my_view = view.view()
            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

    def qapp():
        if QtWidgets.QApplication.instance():
            app = QtWidgets.QApplication.instance()
        else:
            app = QtWidgets.QApplication(sys.argv)
        return app

    app = qapp()
    window = demo()
    window.show()
    app.exec_()
