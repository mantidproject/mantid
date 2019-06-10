.. _ModelExerciseSolution:

=======================
Model Exercise Solution
=======================

The model should now contain the following class:

.. code-block:: python

    class ColourConverter(object):

        def __init__(self):
            self.colour_table = {"red": "r", "blue": "b", "black": "k"}

        def getColourSelection(self):
            return self.colour_table.keys()

The view should contain the following method:

.. code-block:: python

     def setColours(self,options):
         self.colours.clear()
         self.colours.addItems(options)

The presenter initialisation should now be:

.. code-block:: python

     def __init__(self, view, data_model, colour_model):
        self.view = view
        self.data_model = data_model
        self.colour_model = colour_model

        colours = self.colour_model.getColourSelection()
        self.view.setColours(colours)
        # connect statements
        self.view.plotSignal.connect(self.updatePlot)

And the Main module should now pass the two models into the presenter:

.. code-block:: python

    def __init__(self, parent=None):
        super(demo,self).__init__(parent)

        self.window = QtWidgets.QMainWindow()
        my_view = view.view()
        data_model = model.DataGenerator()
        colour_model = model.ColourConvertor()

        self.presenter = presenter.Presenter(my_view, data_model, colour_model)
        # set the view for the main window
        self.setCentralWidget(my_view)
        self.setWindowTitle("view tutorial")
