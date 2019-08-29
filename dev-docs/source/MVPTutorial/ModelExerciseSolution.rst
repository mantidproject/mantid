.. _ModelExerciseSolution:

=======================
Model Exercise Solution
=======================

The model should now contain the following class:

.. code-block:: python

    def line_colours(object):
        colour_table = ["red", "blue", "black"]
        return colour_table

The view should contain the following method:

.. code-block:: python

     def setColours(self,options):
         self.colours.clear()
         self.colours.addItems(options)

The presenter initialisation should now be:

.. code-block:: python

     def __init__(self, view, data_model, colour_list):
        self.view = view
        self.data_model = data_model
        
        self.view.setColours(colour_list)
        # connect statements
        self.view.plotSignal.connect(self.updatePlot)

And the Main module should now pass the two models into the presenter:

.. code-block:: python

    def __init__(self, parent=None):
        super(Demo,self).__init__(parent)

        self.window = QtWidgets.QMainWindow()
        my_view = view.View()
        data_model = model.DataGenerator()
        colour_list = model.line_colours()

        self.presenter = presenter.Presenter(my_view, data_model, colour_list)
        # set the view for the main window
        self.setCentralWidget(my_view)
        self.setWindowTitle("view tutorial")
