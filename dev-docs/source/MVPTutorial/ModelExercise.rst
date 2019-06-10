====================
Model (MVP) Exercise
====================

In the previous section we did not need to update the view. However,
when changing the list of allowed line colour options it is better to
update the model. The view should then be able to be updated by the
presenter to only show the line colours allowed in the model. To
achieve this you will need to add:

1. A second class to the model containing a dictionary of allowed
   line colours.
2. A method to the model for getting the allowed colours.
3. A method in the view to update the ComboBox values to match some
   input values.
4. In the initialisation of the presenter get the allowed colours from
   the model and pass them to the view.

See :ref:`here <ModelExerciseSolution>` for the solution. 
