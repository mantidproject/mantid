=======
Layouts
=======

In the previous task a label was added to the view. However, the label
appeared below the button. It would be sensible to place the label
next to the button. This is possible by using **layouts**.

So far we have used the vertical layout (``QtGui.QVBoxLayout``) and we
will now use the horizontal layout. It is possible to add sub-layouts
to a layout, which we will do here by adding a horizontal layout to
the vertical one. The order in which widgets are added to the layout
will determine their location.

In the view we will replace the ``__init__`` with the following:

.. code-block:: python

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

        # set the layout for the view widget
        self.setLayout(grid)
