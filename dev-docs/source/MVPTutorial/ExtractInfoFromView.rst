====================================
Extracting Information from the View
====================================

The Presenter will need a way to obtain information from the
View. This can be done by **get** methods, as with normal
classes. Here is a simple example of how a **get** method for the view can
be used.

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import PyQt4.QtGui  as QtGui
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
  
            self.value = QtGui.QLineEdit()
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

The Presenter has the following code added to the ``handleButton`` method:

.. code-block:: python

    value = self.view.getValue()
    print("Value is "+str(value))

which gets the value from the View and then prints it to the screen.
