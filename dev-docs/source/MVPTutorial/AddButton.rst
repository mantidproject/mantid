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

First we need to import the relevant components from PyQt and other modules.

.. code-block:: python

    from qtpy.QtWidgets import QGridLayout, QPushButton, QWidget
    from typing import Union

We then create the View class as a QWidget. Each view will have a
parent. As a result, the view will automatically be destroyed if the
parent is destroyed (unless the view has been removed, via docking,
from the parent).

.. code-block:: python

    class View(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)

Next, inside the constructor, we create a layout and add a button to it

.. code-block:: python

   grid = QGridLayout()
   self._button = QPushButton("Hi", self)
   self._button.setStyleSheet("background-color:lightgrey")

   # connect button to signal
   self._button.clicked.connect(self.btn_click)
   # add button to layout
   grid.addWidget(self._button)
   # set the layout for the view widget
   self.setLayout(grid)

The above connect statement means that when the button is pressed, the
function ``btn_click`` is called:

.. code-block:: python

   def btn_click(self) -> None:
       print("Hello world")

The Main
########

To run the GUI we need a 'main' module. Assuming that the above has
all been saved in ``view.py``, the ``main.py`` will contain:

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from view import View


    def get_qapplication_instance() -> QApplication:
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)

    app = get_qapplication_instance()
    window = View()
    window.show()
    app.exec_()


Note that there needs to be a QApplication instance running in the
background to allow you to show your QWidget.

.. tip::

   Notice we used a plain `QWidget` instead of `QMainWindow` to build our widget.
   This has several advantanges, with the main one being:

   - A `QWidget` can be embedded into a larger interface (not possible for a `QMainWindow`).
   - A `QWidget` can be used as a standalone interface (`QMainWindow` can do this too).

   Using `QWidget` therefore gives you more options for how to use the widget in the future.
