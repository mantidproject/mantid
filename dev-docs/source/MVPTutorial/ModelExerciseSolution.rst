=======================
Model Exercise Solution
=======================

The Model should now contain the following method in the
``ColourConvertor`` class:

.. code-block:: python

    def getColourSelection(self):
        return self.colour_table.keys()

The View should contain the following method:

.. code-block:: python

     def setColours(self,options):
         self.colours.clear()
         self.colours.addItems(options)

The Presenter initialisation should now be:

.. code-block:: python

     def __init__(self, view, data_model, colour_model):
        self.view = view
        self.data_model = data_model
        self.colour_model = colour_model

        colours = self.colour_model.getColourSelection()
        self.view.setColours(colours)
        # connect statements
        self.view.plotSignal.connect(self.updatePlot)
