=======
Layouts
=======

In the previous task a label was added to the view. However, the label
appeared below the button. It would be sensible to place the label
next to the button. This is possible by using **layouts**.

So far we have used the vertical layout (``QtWidgets.QVBoxLayout``) and we
will now use the horizontal layout. It is possible to add sub-layouts
to a layout, which we will do here by adding a horizontal layout
(``QtWidgets.QHBoxLayout``) to the vertical one. The order in which
widgets are added to the layout will determine their location.

In the view we will replace the ``__init__`` with the following:

.. code-block:: python

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
