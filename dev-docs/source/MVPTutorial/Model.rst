==============
A Simple Model
==============

Models are used to do the 'hard sums' within the GUI. It is possible
to have multiple models within a single presenter, as we will see with
this example. For this example we have kept the models fairly simple.

The first model generates the data for the user:

.. code-block:: python

    import numpy as np

    class PlotModel:

        def __init__(self):
            self._x_data = np.linspace(0.0, 10.0, 100)
            self._y_data = []

        def generate_y_data(self, freq: float, phi: float) -> None:
            self._y_data = np.sin(freq * self._x_data + phi)

        def get_x_data(self) -> np.ndarray:
            return self._x_data

        def get_y_data(self) -> np.ndarray:
            return self._y_data

The model methods can be split into three types: initialisation, a
calculate button and get methods.
