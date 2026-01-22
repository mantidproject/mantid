# Adding a Button

In this task a GUI is created containing a single button.

## The View

The below code creates a QWidget containing a single button. When the
button is pressed it will print a message to the terminal screen. It
should be noted that in practice this should be avoided and will be
discussed in [this section](CreatingThePresenter).

First we need to import the relevant components from PyQt and other
modules.

``` python
from qtpy.QtWidgets import QGridLayout, QPushButton, QWidget
from typing import Union
```

We then create the View class as a QWidget. Each view will have a
parent. As a result, the view will automatically be destroyed if the
parent is destroyed (unless the view has been removed, via docking, from
the parent).

``` python
class View(QWidget):

    def __init__(self, parent: Union[QWidget, None]=None):
        super().__init__(parent)
```

Next, inside the constructor, we create a layout and add a button to it

``` python
grid = QGridLayout()
self._button = QPushButton("Hi", self)
self._button.setStyleSheet("background-color:lightgrey")

# connect button to signal
self._button.clicked.connect(self._button_clicked)
# add button to layout
grid.addWidget(self._button)
# set the layout for the view widget
self.setLayout(grid)
```

The above connect statement means that when the button is pressed, the
function [Button Clicked](_button_clicked) is called:

``` python
def _button_clicked(self) -> None:
    print("Hello world")
```

## The Main

To run the GUI we need a 'main' module. Assuming that the above has all
been saved in `view.py`, the `main.py` will contain:

``` python
import sys

from qtpy.QtWidgets import QApplication

from view import View


def _get_qapplication_instance() -> QApplication:
    if app := QApplication.instance():
        return app
    return QApplication(sys.argv)


if __name__ == "__main__" :
    app = _get_qapplication_instance()
    window = View()
    window.show()
    app.exec_()
```

Note that there needs to be a QApplication instance running in the
background to allow you to show your QWidget.

```{tip}
Notice we used a plain <span class="title-ref">QWidget</span> instead of
<span class="title-ref">QMainWindow</span> to build our widget. This has
several advantanges, with the main one being:

- A <span class="title-ref">QWidget</span> can be embedded into a larger
  interface (not possible for a
  <span class="title-ref">QMainWindow</span>).
- A <span class="title-ref">QWidget</span> can be used as a standalone
  interface (<span class="title-ref">QMainWindow</span> can do this
  too).

Using <span class="title-ref">QWidget</span> therefore gives you more
options for how to use the widget in the future.
```
