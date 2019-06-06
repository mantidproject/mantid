.. _ReceivingSignalFromView:

================================
Receiving a signal from the view
================================

In the :ref:`Add Button <AddButton>` section we had the response to a button press
within the View. In practice this is not a good implementation. If the
response was more complicated then it would be difficult to maintain
the View as it would become extremely long. Furthermore creating the
look of the GUI is fairly simple and any logic/responses should be
contained within the Presenter.

In this section we will make a simple Presenter for when a button is
pressed. First we will start with the View:

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore


    class view(QtGui.QWidget):

        doSomethingSignal = QtCore.pyqtSignal()

        def __init__(self, parent=None):
            super(view, self).__init__(parent)

            self.button = QtGui.QPushButton('Hi', self)
            self.button.setStyleSheet("background-color:lightgrey")
            # connect button to signal
            self.button.clicked.connect(self.btn_click)

            self.label = QtGui.QLabel()
            self.label.setText("Button")

            # add widgets to layout
            self.sub_layout = QtGui.QHBoxLayout()
            self.sub_layout.addWidget(self.label)            
            self.sub_layout.addWidget(self.button)
 
            grid = QtGui.QVBoxLayout(self)
            grid.addLayout(self.sub_layout)
            # set the layout for the view widget
            self.setLayout(grid)
 
        #send signals
        def btn_click(self):
            print ("hellow from view")
            self.doSomethingSignal.emit()

The above code has two new additions. The first is the creation of a
custom signal on line eight. It is also possible to pass objects with
the signals. The second addition is that ``btn_click`` now emits the
custom signal and will be caught by the Presenter.

The Presenter is initialised with the View and must be a member of the
Presenter class. It is therefore possible to change the View by
passing a different one to the Presenter. For example you may want to
have the widgets in a grid or in a table. The Presenter connects the
custom signal from the View to its own function (``handleButton``).

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    class Presenter(object):

        # pass the view and model into the presenter
        def __init__(self, view):
            self.view = view

            self.view.doSomethingSignal.connect(self.handleButton)             
       
        # handle signals 
        def handleButton(self):
            print("hello world, from the presenter")

The main is now:

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import PyQt4.QtGui as QtGui 
    import PyQt4.QtCore as QtCore

    import sys
    import view
    import presenter


    """
    A wrapper class for setting the main window
    """
    class demo(QtGui.QMainWindow):
        def __init__(self, parent=None):
            super(demo, self).__init__(parent)

            self.window = QtGui.QMainWindow()
            my_view = view.view(self)
            self.my_presenter = presenter.Presenter(my_view)
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

The View and Presenter are both created, but only the Presenter has to
be a member of the demo class. The View is created to be passed to the
Presenter and the View could easily be replaced.
