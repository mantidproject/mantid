====================================
Extracting Information from the View
====================================

The presenter will need a way to obtain information from the
view. This can be done by **get** methods, as with normal
classes. Here is a simple example of how a **get** method for the view can
be used.

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    from qtpy import QtWidgets, QtCore, QtGui


    class view(QtWidgets.QWidget):

        doSomethingSignal = QtCore.Signal()

        def __init__(self, parent=None):
            super(view, self).__init__(parent)

            self.button = QtWidgets.QPushButton('Hi', self)
            self.button.setStyleSheet("background-color:lightgrey")
            # connect button to signal
            self.button.clicked.connect(self.btn_click)

            self.label = QtWidgets.QLabel()
            self.label.setText("Button")

            # add widgets to layout
            self.sub_layout = QtWidgets.QHBoxLayout()
            self.sub_layout.addWidget(self.label)            
            self.sub_layout.addWidget(self.button)
 
            grid = QtWidgets.QVBoxLayout(self)
            grid.addLayout(self.sub_layout)
  
            self.value = QtWidgets.QLineEdit()
            grid.addWidget(self.value)  

  
            # set the layout for the view widget
            self.setLayout(grid)
 
        #send signals
        def btn_click(self):
            print ("hellow from view")
            self.doSomethingSignal.emit()

            def getValue(self):
            return float(self.value.text())

The last function ``getValue`` returns the value of the line
edit. Since ``text()`` returns a string the output is type cast into a
float.

The presenter has the following code added to the ``handleButton`` method:

.. code-block:: python

    value = self.view.getValue()
    print("Value is "+str(value))

which gets the value from the view and then prints it to the screen.
