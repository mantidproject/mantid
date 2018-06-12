====================
Model (MVP) Exercise
====================

In the previous section we did not need to update the View. However,
the Model contains a dictionary which contains the allowed colour
options. The GUI should show the same allowed values as the the
Model. To achieve this you will need to add:

1. A method to the Model for getting the allowed colours

2. A method in the View to update the ComboBox values to match some
input values

3. In the initialisation of the Presenter get the allowed colours from
the Model and pass them to the View

See `here <ModelExerciseSolution.html>`_ for the solution. 
