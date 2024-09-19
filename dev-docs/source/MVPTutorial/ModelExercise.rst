====================
Model (MVP) Exercise
====================

In the previous section we did not need to update the view. However,
when changing the list of allowed line colour options it is better to
update the model. The view should then be able to be updated by the
presenter to only show the line colours allowed in the model. To
achieve this you will need to add:

#. A function to the model containing a list of allowed
   line colours and returning them when called.
#. A method in the view to update the ComboBox values to match some
   input values.
#. In the constructor of the presenter get the allowed colours from
   the model and pass them to the view.

See :ref:`here <ModelExerciseSolution>` for the solution.
