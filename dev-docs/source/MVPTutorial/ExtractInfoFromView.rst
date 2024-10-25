====================================
Extracting Information from the View
====================================

The presenter will need a way to obtain information from the
view. This can be done by **get** methods, as with normal
classes. Here is a simple example of how a **get** method for the view can
be used.

.. code-block:: python

    from qtpy.QtWidgets import QHBoxLayout, QLabel, QLineEdit, QPushButton, QVBoxLayout, QWidget
    from typing import Union


    class View(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)
            self.setWindowTitle("view tutorial")

            # A presenter will be subscribed to the view later
            self._presenter = None

            self._button = QPushButton("Hi", self)
            self._button.setStyleSheet("background-color:lightgrey")
            # connect button to signal
            self._button.clicked.connect(self._button_clicked)

            self._label = QLabel()
            self._label.setText("Button")

            # add widgets to layout
            self._sub_layout = QHBoxLayout()
            self._sub_layout.addWidget(self._label)
            self._sub_layout.addWidget(self._button)

            grid = QVBoxLayout(self)
            grid.addLayout(self.sub_layout)

            self._value = QLineEdit()
            grid.addWidget(self._value)

            # set the layout for the view widget
            self.setLayout(grid)

        def subscribe_presenter(self, presenter) -> None:
            # Subscribe the presenter to the view so we do not need to
            # make a Qt connection between the presenter and view
            self._presenter = presenter

        def _button_clicked(self) -> None:
            print("hello from view")
            self._presenter.handle_button_clicked()

        def get_value(self) -> float:
            return float(self._value.text())

The last function ``get_value`` returns the value of the line
edit. Since ``text()`` returns a string the output is type cast into a
float.

The presenter has the following code added to the ``handle_button_clicked`` method:

.. code-block:: python

    value = self._view.get_value()
    print(f"Value is {value}")

which gets the value from the view and then prints it to the screen.
