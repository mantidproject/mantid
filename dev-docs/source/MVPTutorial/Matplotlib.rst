.. _Matplotlib:

==================
Matplotlib and MVP
==================

The next step towards the goal of creating a GUI that allows users to
manipulate a sine wave plot, is to produce the plot itself.

For the purposes of this tutorial it is assumed that the user is
familiar with Matplotlib, if not see `Matplotlib documentation
<https://matplotlib.org/users/pyplot_tutorial.html>`_.

The Matplotlib functions could be considered to be a Model or a View
and there is no correct answer to which. It could be a Model as it
does something with the data, however it could be considered to be a
view as (in this case) it is used purely as a visual
representation. This ambiguity is why MVP is only a pattern and not a
set of rules. On this occasion it has been decided that it should be a
View.

This view will exist alongside with the view from the exercise. So we
will need to call it something different, such as PlotView.

The View has the following imports:

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore
    import matplotlib.pyplot as plt

    from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas

The fourth line imports Matplotlib and the last line allows it to
interface with the GUI.

The main class is shown below and contains methods for adding data to
the plot and creating an empty plot (no data).

.. code-block:: python

    class PlotView(QtGui.QWidget):
        def __init__(self, parent=None):
            super(PlotView, self).__init__(parent)

            self.figure = plt.figure()
            grid = QtGui.QVBoxLayout(self)
            self.draw() 
            self.canvas = self.getWidget()
            grid.addWidget(self.canvas)
            self.setLayout(grid) 

        def draw(self):
            ax = self.figure.add_subplot(111)
            ax.clear()
            ax.set_xlim([0.0, 10.5])
            ax.set_ylim([-1.05, 1.05])
            ax.set_xlabel("time ($s$)")
            ax.set_ylabel("$f(t)$")
            return ax

        def getWidget(self):
            return FigureCanvas(self.figure)

        def addData(self, xvalues, yvalues, grid_lines, colour, marker):
            ax = self.draw()
            ax.grid(grid_lines)
            ax.plot(xvalues, yvalues, color=colour, marker=marker, linestyle="--") 
            self.canvas.draw()

The ``draw`` method creates the plot area without any data. The widget
is obtained from the ``getWidget`` function. The final method adds
data to the plot area, the ``self.canvas.draw()`` updates the plot
area so it will contain the data.
