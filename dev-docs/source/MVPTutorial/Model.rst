==============
A Simple Model
==============

Models are used to do the 'hard sums' within the GUI. It is possible
to have multiple models within a single presenter, as we will see with
this example. For this example we have kept the models fairly simple,
in reality they will be much more complex.

The first model generates the data for the user:

.. code-block:: python

    import numpy as np

    class DataGenerator(object):

        def __init__(self):
            self.x_data = np.linspace(0.0, 10.0, 100)
            self.y_data = []

        def genData(self, freq, phi):
            self.y_data = np.sin(freq * self.x_data + phi)

        def getXData(self):
            return self.x_data

        def getYData(self):
            return self.y_data

The model methods can be split into three types: initialisation, a
calculate button and get methods.

In this case we have a distinct second method. Usually this will be
placed into its own file, however for simplicity we will contain it
within the same file as the above code.
