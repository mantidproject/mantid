.. _ReceivingSignalFromView:

================================
Receiving a signal from the view
================================

In the :ref:`Add Button <AddButton>` section we had the response to a button press
within the view. In practice this is not a good implementation. If the
response was more complicated then it would be difficult to maintain
the view as it would become extremely long. Furthermore creating the
look of the GUI is fairly simple and any logic/responses should be
contained within the presenter.

In this section we will make a simple presenter for when a button is
pressed. First we will start with the view:

.. code-block:: python

    from qtpy.QtCore import Signal
    from qtpy.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout, QWidget
    from typing import Union


    class View(QWidget):

        doSomethingSignal = Signal()

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)

            self._button = QPushButton("Hi", self)
            self._button.setStyleSheet("background-color:lightgrey")
            # connect button to signal
            self._button.clicked.connect(self.btn_click)

            self._label = QLabel()
            self._label.setText("Button")

            # add widgets to layout
            self._sub_layout = QHBoxLayout()
            self._sub_layout.addWidget(self._label)
            self._sub_layout.addWidget(self._button)

            grid = QVBoxLayout(self)
            grid.addLayout(self._sub_layout)
            # set the layout for the view widget
            self.setLayout(grid)

        # send signals
        def btn_click(self) -> None:
            print("hello from view")
            self.doSomethingSignal.emit()

The above code has two new additions. The first is the creation of a
custom signal on line eight. It is also possible to pass objects with
the signals. The second addition is that ``btn_click`` now emits the
custom signal and will be caught by the presenter.

The presenter is initialised with the view and must be a member of the
Presenter class. It is therefore possible to change the view by
passing a different one to the presenter. For example you may want to
have the widgets in a grid or in a table. The presenter connects the
custom signal from the view to its own function (``handleButton``).

.. code-block:: python

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

    from qtpy import QtWidgets, QtCore, QtGui

    import sys
    import view
    import presenter


    """
    A wrapper class for setting the main window
    """
    class Demo(QtWidgets.QMainWindow):
        def __init__(self, parent=None):
            super().__init__(parent)

            self.window = QtWidgets.QMainWindow()
            my_view = view.View(self)
            self.my_presenter = presenter.Presenter(my_view)
            # set the view for the main window

            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

    def get_qapplication_instance():
        if QtWidgets.QApplication.instance():
            app = QtWidgets.QApplication.instance()
        else:
            app = QtWidgets.QApplication(sys.argv)
        return app


    app = get_qapplication_instance()
    window = Demo()
    window.show()
    app.exec_()

The view and presenter are both created, but only the presenter has to
be a member of the Demo class. The view is created to be passed to the
presenter and the view could easily be replaced.
