.. _Matplotlib:

==================
Matplotlib and MVP
==================

The next step towards the goal of creating a GUI that allows users to
manipulate a sine wave plot, is to produce the plot itself.

For the purposes of this tutorial it is assumed that the user is
familiar with Matplotlib, if not see `Matplotlib documentation
<https://matplotlib.org/users/pyplot_tutorial.html>`_.

The Matplotlib functions could be considered to be a model or a view
and there is no correct answer to which. It could be a model as it
does something with the data, however it could be considered to be a
view as (in this case) it is used purely as a visual
representation. This ambiguity is why MVP is only a pattern and not a
set of rules. On this occasion it has been decided that it should be a
view.

This view will exist alongside with the view from the exercise. So we
will need to call it something different, such as PlotView. The python
file will be named `plot_view.py` for the rest of this tutorial.

The view has the following imports:

.. code-block:: python

    from numpy import ndarray
    from qtpy.QtWidgets import QWidget, QVBoxLayout
    from typing import Union

    import matplotlib.pyplot as plt
    from matplotlib.axes import Axes
    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas

The fourth line imports Matplotlib and the last line allows it to
interface with the GUI.

The PlotView class is shown below and contains methods for adding data to
the plot and creating an empty plot (no data).

.. code-block:: python

    class PlotView(QWidget):
        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)

            self._figure = plt.figure()
            grid = QVBoxLayout(self)
            self.draw()
            self._canvas = FigureCanvas(self._figure)
            grid.addWidget(self._canvas)
            self.setLayout(grid)

        def draw(self) -> Axes:
            ax = self._figure.add_subplot(111)
            ax.clear()
            ax.set_xlim([0.0, 10.5])
            ax.set_ylim([-1.05, 1.05])
            ax.set_xlabel("time ($s$)")
            ax.set_ylabel("$f(t)$")
            return ax

        def plot_data(self, xvalues: ndarray, yvalues: ndarray, visible: bool, colour: str, marker: str) -> None:
            ax = self.draw()
            ax.grid(visible)
            ax.plot(xvalues, yvalues, color=colour, marker=marker, linestyle="--")
            self._canvas.draw()

The ``draw`` method creates the plot area without any data. The widget
is created using the ``FigureCanvas`` class. The final method adds
data to the plot area, the ``self._canvas.draw()`` updates the plot
area so it will contain the data.
