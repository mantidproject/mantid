====================
Model (MVP) Exercise
====================

In the previous section we did not need to update the View. However,
when changing the list of allowed line colour options it is better to
update the Model. The View should then be able to be updated by the
Presenter to only show the line colours allowed in the Model. To
achieve this you will need to add:

1. A second class to the Model containing a dictionary of allowed
   line colours.
2. A method to the Model for getting the allowed colours.
3. A method in the View to update the ComboBox values to match some
   input values.
4. In the initialisation of the Presenter get the allowed colours from
   the Model and pass them to the View.

See :ref:`here <ModelExerciseSolution>` for the solution. 
